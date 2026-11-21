/*
 * ipl_bank.c - see ipl_bank.h. MIT licence.
 *
 * WSYS chunk : "WSYS" u32 size, ..., u32 off_WINF @+0x10
 * WINF       : 1 wave group: char name[0x70] (IPL 1.1+) or [0x44] (NTSC 1.0),
 *              u32 count, u32 off[count] (WSYS-relative)
 * wave entry : u8 ?, u8 fmt, u8 key, u8 ?, f32 rate, u32 aw_off, u32 aw_size,
 *              u32 loop (-1 = yes), u32 loop_start, u32 loop_end, u32 nsamples
 * .aw data   : right after the WSYS chunk, aligned to 0x20
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ipl_bank.h"

static uint32_t be32(const uint8_t *p) { return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]; }

static const int16_t afc_coef[16][2] = {
	{0, 0}, {0x0800, 0}, {0, 0x0800}, {0x0400, 0x0400},
	{0x1000, -0x0800}, {0x0e00, -0x0600}, {0x0c00, -0x0400}, {0x1200, -0x0a00},
	{0x1068, -0x08c8}, {0x12c0, -0x08fc}, {0x1400, -0x0c00}, {0x0800, -0x0800},
	{0x0400, -0x0400}, {-0x0400, 0x0400}, {-0x0400, 0}, {-0x0800, 0}
};

static void afc_decode(const uint8_t *src, uint32_t n, int16_t *out)
{
	int32_t h1 = 0, h2 = 0;
	uint32_t i;
	int k;
	for (i = 0; i < n; i += 16, src += 9) {
		int32_t scale = 1 << (src[0] >> 4);
		int idx = src[0] & 15;
		for (k = 0; k < 16 && i + k < n; k++) {
			int v = (k & 1) ? (src[1 + k / 2] & 15) : (src[1 + k / 2] >> 4);
			int32_t s;
			if (v >= 8) v -= 16;
			s = ((v * scale) << 11) + afc_coef[idx][0] * h1 + afc_coef[idx][1] * h2;
			s >>= 11;
			if (s > 32767) s = 32767;
			if (s < -32768) s = -32768;
			h2 = h1;
			h1 = s;
			out[i + k] = (int16_t)s;
		}
	}
}

/* The pitch actually heard. The stored root keys do not always match it, so the
 * instruments' fundamentals were measured from their spectra (identical in all six
 * IPL dumps; checked by the sample counts): wave 4 sounds C4 (a bell-like tone with
 * inharmonic partials), 7 F4 and 8 C5 (soft, nearly sine), 11 C4 (soft, with
 * vibrato); 5 and 6 are percussion (unpitched). */
static const struct { uint32_t n; float hz; } known_root[IPL_BANK_MAX] = {
	{0, 0.0f}, {0, 0.0f}, {0, 0.0f}, {0, 0.0f},
	{52049, 260.0f}, {31214, 0.0f}, {17821, 0.0f}, {31551, 348.5f},
	{28544, 525.3f}, {0, 0.0f}, {0, 0.0f}, {79249, 260.9f}
};

static float root_of(int i, const ipl_wave_t *w)
{
	if (i < IPL_BANK_MAX && known_root[i].n && known_root[i].n == w->n)
		return known_root[i].hz;
	/* another bank: trust the stored key */
	return (float)(440.0 * pow(2.0, ((double)w->key - 69.0) / 12.0));
}

int ipl_bank_load(ipl_bank_t *b, const uint8_t *rom, uint32_t size)
{
	uint32_t ws = 0, o, wsz, winf, aw, grp, namelen = 0, cnt, i;
	memset(b, 0, sizeof(*b));
	for (o = 0; o + 4 < size; o += 4)
		if (!memcmp(rom + o, "WSYS", 4)) { ws = o; break; }
	if (!ws)
		return -1;
	wsz = be32(rom + ws + 4);
	winf = ws + be32(rom + ws + 0x10);
	aw = (ws + wsz + 0x1f) & ~0x1fu;
	if (winf + 12 > size || memcmp(rom + winf, "WINF", 4))
		return -2;
	grp = ws + be32(rom + winf + 8);
	if (be32(rom + grp + 0x74) == 0x20) namelen = 0x70;
	else if (be32(rom + grp + 0x48) == 0x20) namelen = 0x44;
	if (!namelen)
		return -3;
	cnt = be32(rom + grp + namelen);
	if (cnt > IPL_BANK_MAX)
		cnt = IPL_BANK_MAX;
	for (i = 0; i < cnt; i++) {
		const uint8_t *e = rom + ws + be32(rom + grp + namelen + 4 + 4 * i);
		ipl_wave_t *w = &b->w[i];
		uint32_t rb = be32(e + 4), off = be32(e + 8), sz = be32(e + 0xc);
		float rate;
		memcpy(&rate, &rb, 4);
		w->rate = rate;
		w->key = e[2];
		w->loop = be32(e + 0x10) == 0xffffffffu;
		w->loop_start = be32(e + 0x14);
		w->loop_end = be32(e + 0x18);
		w->n = be32(e + 0x1c);
		if (e[1] != 0 || aw + off + sz > size)
			continue;                           /* not AFC, or out of range */
		w->pcm = (int16_t *)malloc((w->n + 16) * 2);
		if (!w->pcm)
			continue;
		afc_decode(rom + aw + off, w->n, w->pcm);
		if (w->loop && (w->loop_end <= w->loop_start || w->loop_end > w->n))
			w->loop = 0;
		w->root_hz = root_of((int)i, w);
	}
	b->count = (int)cnt;
	return 0;
}

void ipl_bank_free(ipl_bank_t *b)
{
	int i;
	for (i = 0; i < IPL_BANK_MAX; i++)
		free(b->w[i].pcm);
	memset(b, 0, sizeof(*b));
}
