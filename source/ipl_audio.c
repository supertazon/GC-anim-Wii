/*
 * ipl_audio.c - extract the GameCube IPL intro jingle from a descrambled IPL
 *
 * How the data is found (see AUDIO.md for details):
 *   1. WSYS  (JAudio wave system "Banks/ipl.ws")  -> wave table: format,
 *      sample rate, base key, offset/size inside the AW, sample count.
 *      Two WINF layouts exist (IPL 1.0: 0x24-byte wave records, 0x44-byte
 *      AW name; 1.1+: 0x2C / 0x70); the count field is located by
 *      validating the records, not by version.
 *   2. AW    (raw AFC sample data "ipl_0.aw") immediately follows the WSYS,
 *      aligned to 32 bytes (validated, with a small fallback scan).
 *   3. SCNE/C-DF (inside WSYS) maps wave index -> wave id.
 *   4. IBNK  ("Banks/ipl.bnk") instrument <program>: the key region
 *      containing key 60 gives the left wave, key 72 the right wave
 *      (the sequence plays L = note 60 pan 0, R = note 72 pan 127).
 *   5. BMS sequence (follows the AW): program and volume tables indexed by
 *      the jingle variant (port 0 written by FUN_81357840(n)).
 *
 * AFC decoder after hcs64's wsyster.c (same algorithm as the GC DSP).
 */
#include "ipl_audio.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#if defined(GEKKO) && !defined(IPL_AUDIO_ALLOC)
/* devkitPPC/newlib: 32-byte aligned buffers, directly usable by the DSP */
#include <malloc.h>
#define IPL_AUDIO_ALLOC(n) memalign(32, (n))
#endif
#ifndef IPL_AUDIO_ALLOC
#define IPL_AUDIO_ALLOC(n) malloc(n)
#endif
#ifndef IPL_AUDIO_FREE
#define IPL_AUDIO_FREE(p) free(p)
#endif

/* Notes played by the jingle tracks (from the BMS sequence, identical in
 * all six known IPLs): left track = key 60, pan 0; right = key 72, pan 127. */
#define KEY_LEFT  60
#define KEY_RIGHT 72

/* ------------------------------------------------------------------ */
/* big-endian readers                                                  */

static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}
static uint16_t be16(const uint8_t *p)
{
    return (uint16_t)((p[0] << 8) | p[1]);
}
static float bef32(const uint8_t *p)
{
    uint32_t u = be32(p);
    float f;
    memcpy(&f, &u, sizeof f);
    return f;
}

static long find_bytes(const uint8_t *d, size_t len, size_t from,
                       const uint8_t *pat, size_t plen, size_t step)
{
    size_t i;
    if (plen > len) return -1;
    for (i = from; i + plen <= len; i += step)
        if (d[i] == pat[0] && memcmp(d + i, pat, plen) == 0) return (long)i;
    return -1;
}

static size_t align32(size_t x) { return (x + 31) & ~(size_t)31; }

/* ------------------------------------------------------------------ */
/* WSYS                                                                */

typedef struct {
    int      fmt;      /* 0 = AFC 9-byte frames */
    int      key;      /* base key */
    float    rate;
    uint32_t off, size, nsamples;
} wave_t;

#define MAX_WAVES 256

typedef struct {
    size_t   pos, size;          /* WSYS absolute position / size */
    int      nwaves;
    wave_t   w[MAX_WAVES];
    int      id[MAX_WAVES];      /* wave id (C-DF) per wave index, -1 unknown */
    size_t   aw;                 /* absolute AW base */
    size_t   aw_end;             /* absolute end of last wave */
} wsys_t;

static int wave_rec_ok(const uint8_t *ws, size_t wsize, uint32_t o)
{
    float r;
    if (o < 0x20 || (size_t)o + 0x20 > wsize) return 0;
    if (ws[o + 1] > 3) return 0;
    r = bef32(ws + o + 4);
    if (!(r >= 4000.0f && r <= 96000.0f)) return 0;
    if (be32(ws + o + 0x0C) == 0 || be32(ws + o + 0x1C) == 0) return 0;
    return 1;
}

static int parse_wsys(const uint8_t *d, size_t len, wsys_t *ws)
{
    static const uint8_t magic[4] = { 'W', 'S', 'Y', 'S' };
    size_t from = 0;

    for (;;) {
        long p = find_bytes(d, len, from, magic, 4, 4);
        const uint8_t *s;
        uint32_t size, winf, wbct, awe;
        uint32_t L;
        int i, n = 0;

        if (p < 0) return IPL_AUDIO_E_NO_WSYS;
        from = (size_t)p + 4;
        s = d + p;
        if ((size_t)p + 0x20 > len) continue;
        size = be32(s + 4);
        winf = be32(s + 0x10);
        wbct = be32(s + 0x14);
        if (size < 0x40 || size > 0x10000 || (size_t)p + size > len) continue;
        if (winf + 0x10 > size || wbct + 0x10 > size) continue;
        if (memcmp(s + winf, "WINF", 4) || memcmp(s + wbct, "WBCT", 4)) continue;
        if (be32(s + winf + 4) < 1) continue;
        awe = be32(s + winf + 8);                 /* first AW entry */
        if (awe + 0x14 > size) continue;

        /* locate the wave-count field after the AW file name */
        for (L = 0x10; L <= 0x80 && awe + L + 4 <= size; L += 4) {
            uint32_t cnt = be32(s + awe + L);
            uint32_t k;
            if (cnt < 1 || cnt > MAX_WAVES) continue;
            if (awe + L + 4 + 4 * cnt > size) continue;
            for (k = 0; k < cnt; k++)
                if (!wave_rec_ok(s, size, be32(s + awe + L + 4 + 4 * k))) break;
            if (k == cnt) { n = (int)cnt; break; }
        }
        if (!n) continue;

        ws->pos = (size_t)p;
        ws->size = size;
        ws->nwaves = n;
        ws->aw_end = 0;
        for (i = 0; i < n; i++) {
            const uint8_t *r = s + be32(s + awe + L + 4 + 4 * i);
            ws->w[i].fmt = r[1];
            ws->w[i].key = r[2];
            ws->w[i].rate = bef32(r + 4);
            ws->w[i].off = be32(r + 8);
            ws->w[i].size = be32(r + 0x0C);
            ws->w[i].nsamples = be32(r + 0x1C);
            ws->id[i] = -1;
            if (ws->w[i].off + ws->w[i].size > ws->aw_end)
                ws->aw_end = ws->w[i].off + ws->w[i].size;
        }

        /* wave ids: WBCT -> SCNE -> C-DF (entries parallel to the WINF list) */
        {
            uint32_t scne = be32(s + wbct + 0x0C);
            if (scne + 0x18 <= size && !memcmp(s + scne, "SCNE", 4)) {
                uint32_t cdf = be32(s + scne + 0x0C);
                if (cdf + 8 <= size && !memcmp(s + cdf, "C-DF", 4)) {
                    uint32_t cnt = be32(s + cdf + 4), k;
                    for (k = 0; k < cnt && k < (uint32_t)n &&
                                cdf + 8 + 4 * k + 4 <= size; k++) {
                        uint32_t e = be32(s + cdf + 8 + 4 * k);
                        if (e + 4 <= size) ws->id[k] = be16(s + e + 2);
                    }
                }
            }
        }
        return IPL_AUDIO_OK;
    }
}

/* AFC sanity: scale nibble of the first frames of each wave must be sane */
static int aw_base_ok(const uint8_t *d, size_t len, const wsys_t *ws, size_t base)
{
    int i, bad = 0, tot = 0;
    if (base + ws->aw_end > len) return 0;
    for (i = 0; i < ws->nwaves; i++) {
        uint32_t f;
        if (ws->w[i].fmt != 0) continue;
        for (f = 0; f < 32 && (f + 1) * 9 <= ws->w[i].size; f++) {
            tot++;
            if ((d[base + ws->w[i].off + f * 9] >> 4) > 12) bad++;
        }
    }
    return tot > 0 && bad * 8 < tot;
}

static int locate_aw(const uint8_t *d, size_t len, wsys_t *ws)
{
    size_t base = align32(ws->pos + ws->size), b;
    if (aw_base_ok(d, len, ws, base)) { ws->aw = base; return IPL_AUDIO_OK; }
    for (b = ws->pos + ws->size; b < ws->pos + ws->size + 0x2000; b += 4)
        if (aw_base_ok(d, len, ws, b)) { ws->aw = b; return IPL_AUDIO_OK; }
    return IPL_AUDIO_E_NO_AW;
}

/* ------------------------------------------------------------------ */
/* BMS sequence: program / volume tables for the jingle variants       */

static void parse_seq(const uint8_t *d, size_t len, const wsys_t *ws,
                      int variant, int *prog, uint16_t *vol)
{
    /* left track of a pair: pan 0, call setup sub, set program 0, note 60 */
    static const uint8_t trk[] = { 0x98, 0x03, 0x00, 0xC4, 0x00, 0x00 };
    static const uint8_t tail[] = { 0xAC, 0x06, 0x00, 0x00, KEY_LEFT, 0x01, 0x7F };
    static const uint8_t sub[] = { 0xA4, 0x08, 0x00, 0xA4, 0x09, 0x00 };
    static const uint8_t r28[] = { 0xA4, 0x10, 0x00, 0xAC, 0x11 };
    static const uint8_t r29[] = { 0xA4, 0x12, 0x00, 0xAC, 0x13 };
    size_t from = ws->aw + ws->aw_end, base = 0;
    long t, s, a, b;
    uint32_t subaddr = 0;

    *prog = variant;
    *vol = 0x8000;

    for (;;) {
        t = find_bytes(d, len, from, trk, sizeof trk, 1);
        if (t < 0) return;
        from = (size_t)t + 1;
        if ((size_t)t + 8 + sizeof tail <= len &&
            !memcmp(d + t + 8, tail, sizeof tail)) break;
    }
    subaddr = be16(d + t + 6);

    /* normally the sequence starts right after the AW, 32-byte aligned */
    base = align32(ws->aw + ws->aw_end);
    if (!(base + subaddr + sizeof sub <= len &&
          !memcmp(d + base + subaddr, sub, sizeof sub))) {
        long lo = t > 0x400 ? t - 0x400 : 0;
        s = find_bytes(d, (size_t)t, (size_t)lo, sub, sizeof sub, 1);
        if (s < 0 || (uint32_t)s < subaddr) return;
        base = (size_t)s - subaddr;
    }

    a = find_bytes(d, (size_t)t, base, r28, sizeof r28, 1);
    b = find_bytes(d, (size_t)t, base, r29, sizeof r29, 1);
    if (a < 0 || b < 0) return;
    {
        size_t t28 = base + be16(d + a + 5);
        size_t t29 = base + be16(d + b + 5);
        if (t28 + 3 <= len && t29 + 6 <= len) {
            int p = d[t28 + variant];
            uint16_t v = be16(d + t29 + 2 * variant);
            if (p < 128) *prog = p;
            if (v <= 0x7FFF) *vol = v;
        }
    }
}

/* ------------------------------------------------------------------ */
/* IBNK: instrument -> wave id for a key, plus pitch factor            */

static int bank_lookup(const uint8_t *d, size_t len, int prog, int key,
                       int *wave_id, float *pitch)
{
    static const uint8_t magic[4] = { 'I', 'B', 'N', 'K' };
    size_t from = 0;
    for (;;) {
        long ib = find_bytes(d, len, from, magic, 4, 4);
        const uint8_t *b;
        uint32_t size, io, nreg, k;
        if (ib < 0) return IPL_AUDIO_E_NO_BANK;
        from = (size_t)ib + 4;
        b = d + ib;
        size = be32(b + 4);
        if ((size_t)ib + size > len || size < 0x24 + 4 * 128) continue;
        if (memcmp(b + 0x20, "BANK", 4)) continue;
        io = be32(b + 0x24 + 4 * prog);
        if (!io || io + 0x30 > size || memcmp(b + io, "INST", 4)) continue;
        nreg = be32(b + io + 0x28);
        for (k = 0; k < nreg && io + 0x2C + 4 * k + 4 <= size; k++) {
            uint32_t r = be32(b + io + 0x2C + 4 * k), nv, j;
            if (r + 0x0C > size) break;
            if (b[r] < key) continue;                 /* key top */
            nv = be32(b + r + 4);
            for (j = 0; j < nv && r + 8 + 4 * j + 4 <= size; j++) {
                uint32_t v = be32(b + r + 8 + 4 * j);
                if (v + 0x10 > size) break;
                if (b[v] < 0x7F) continue;            /* velocity 127 */
                *wave_id = be16(b + v + 6);
                *pitch = bef32(b + v + 0x0C);
                return IPL_AUDIO_OK;
            }
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* AFC ADPCM                                                           */

static const int16_t afc_coef[16][2] = {
    { 0, 0 },          { 0x0800, 0 },     { 0, 0x0800 },     { 0x0400, 0x0400 },
    { 0x1000, -0x800 },{ 0x0e00, -0x600 },{ 0x0c00, -0x400 },{ 0x1200, -0xa00 },
    { 0x1068, -0x8c8 },{ 0x12c0, -0x8fc },{ 0x1400, -0xc00 },{ 0x0800, -0x800 },
    { 0x0400, -0x400 },{ -0x400, 0x0400 },{ -0x400, 0 },     { -0x800, 0 }
};

static void afc_decode(const uint8_t *src, uint32_t nsamples, int16_t *out)
{
    int32_t h1 = 0, h2 = 0;
    uint32_t n = 0;
    while (n < nsamples) {
        int scale = 1 << (src[0] >> 4);
        int idx = src[0] & 0x0F;
        int i;
        for (i = 0; i < 16 && n < nsamples; i++, n++) {
            int nib = (i & 1) ? (src[1 + i / 2] & 0x0F) : (src[1 + i / 2] >> 4);
            int32_t s;
            if (nib >= 8) nib -= 16;
            s = scale * nib * 2048 + h1 * afc_coef[idx][0] + h2 * afc_coef[idx][1];
            s >>= 11;
            if (s > 32767) s = 32767;
            if (s < -32768) s = -32768;
            out[n] = (int16_t)s;
            h2 = h1;
            h1 = s;
        }
        src += 9;
    }
}

static int16_t *decode_wave(const uint8_t *d, size_t len, const wsys_t *ws,
                            int wi, int *err)
{
    const wave_t *w = &ws->w[wi];
    size_t bytes;
    int16_t *buf;
    if (w->fmt != 0 || ((size_t)(w->nsamples + 15) / 16) * 9 > w->size ||
        ws->aw + w->off + w->size > len) {
        *err = IPL_AUDIO_E_FORMAT;
        return NULL;
    }
    bytes = align32((size_t)w->nsamples * 2);
    buf = (int16_t *)IPL_AUDIO_ALLOC(bytes);
    if (!buf) { *err = IPL_AUDIO_E_NOMEM; return NULL; }
    memset(buf, 0, bytes);
    afc_decode(d + ws->aw + w->off, w->nsamples, buf);
    return buf;
}

/* ------------------------------------------------------------------ */

static int wave_index_for_id(const wsys_t *ws, int id)
{
    int i;
    for (i = 0; i < ws->nwaves; i++)
        if (ws->id[i] == id) return i;
    return -1;
}

int ipl_audio_extract_ex(const uint8_t *d, size_t len, int variant,
                         ipl_audio_t *out)
{
    static wsys_t ws; /* ~8 KB, keep it off the (small) homebrew stack */
    int err, prog, idL = -1, idR = -1, wl, wr;
    float pL = 1.0f, pR = 1.0f;
    double rateL, rateR;
    uint16_t vol;

    if (!d || !out || variant < 0 || variant > 2) return IPL_AUDIO_E_ARG;
    memset(out, 0, sizeof *out);

    if ((err = parse_wsys(d, len, &ws)) != IPL_AUDIO_OK) return err;
    if ((err = locate_aw(d, len, &ws)) != IPL_AUDIO_OK) return err;
    parse_seq(d, len, &ws, variant, &prog, &vol);

    if (bank_lookup(d, len, prog, KEY_LEFT, &idL, &pL) == IPL_AUDIO_OK &&
        bank_lookup(d, len, prog, KEY_RIGHT, &idR, &pR) == IPL_AUDIO_OK) {
        wl = wave_index_for_id(&ws, idL);
        wr = wave_index_for_id(&ws, idR);
    } else {
        wl = wr = -1;
    }
    if (wl < 0 || wr < 0) return IPL_AUDIO_E_NO_BANK;

    /* playback rate = wave rate * 2^((note - basekey)/12) * region pitch */
    rateL = ws.w[wl].rate * pow(2.0, (KEY_LEFT - ws.w[wl].key) / 12.0) * pL;
    rateR = ws.w[wr].rate * pow(2.0, (KEY_RIGHT - ws.w[wr].key) / 12.0) * pR;
    if (fabs(rateL - rateR) > 1.0 || rateL < 4000.0) return IPL_AUDIO_E_FORMAT;

    out->left = decode_wave(d, len, &ws, wl, &err);
    if (!out->left) return err;
    out->right = decode_wave(d, len, &ws, wr, &err);
    if (!out->right) { ipl_audio_free(out); return err; }

    /* both tracks of a pair have the same length; use the shorter anyway */
    out->nsamples = ws.w[wl].nsamples < ws.w[wr].nsamples ?
                    ws.w[wl].nsamples : ws.w[wr].nsamples;
    out->samplerate = (int)(rateL + 0.5);
    out->seq_volume = vol;
    out->wave_left = wl;
    out->wave_right = wr;
    return IPL_AUDIO_OK;
}

int ipl_audio_extract(const uint8_t *ipl_desc, size_t len,
                      int16_t **left, int16_t **right,
                      size_t *nsamples, int *samplerate)
{
    ipl_audio_t a;
    int err;
    if (!left || !right || !nsamples || !samplerate) return IPL_AUDIO_E_ARG;
    err = ipl_audio_extract_ex(ipl_desc, len, IPL_JINGLE_NORMAL, &a);
    if (err) return err;
    *left = a.left;
    *right = a.right;
    *nsamples = a.nsamples;
    *samplerate = a.samplerate;
    return IPL_AUDIO_OK;
}

void ipl_audio_free_buf(void *p)
{
    if (p) IPL_AUDIO_FREE(p);
}

void ipl_audio_free(ipl_audio_t *a)
{
    if (!a) return;
    ipl_audio_free_buf(a->left);
    ipl_audio_free_buf(a->right);
    a->left = a->right = NULL;
    a->nsamples = 0;
}

int16_t *ipl_audio_interleave(const ipl_audio_t *a, uint16_t gain_q15,
                              size_t *bytes)
{
    size_t i, sz;
    int16_t *o;
    if (!a || !a->left || !a->right) return NULL;
    sz = align32(a->nsamples * 4);
    o = (int16_t *)IPL_AUDIO_ALLOC(sz);
    if (!o) return NULL;
    memset(o, 0, sz);
    for (i = 0; i < a->nsamples; i++) {
        int32_t l = a->left[i], r = a->right[i];
        if (gain_q15 != 0x8000) {
            l = (l * gain_q15) >> 15;
            r = (r * gain_q15) >> 15;
        }
        o[2 * i] = (int16_t)l;
        o[2 * i + 1] = (int16_t)r;
    }
    if (bytes) *bytes = sz;
    return o;
}

const char *ipl_audio_strerror(int err)
{
    switch (err) {
    case IPL_AUDIO_OK:         return "ok";
    case IPL_AUDIO_E_ARG:      return "bad argument";
    case IPL_AUDIO_E_NO_WSYS:  return "WSYS wave system not found";
    case IPL_AUDIO_E_NO_WAVES: return "WINF wave table not parseable";
    case IPL_AUDIO_E_NO_AW:    return "AW sample data not found";
    case IPL_AUDIO_E_NO_BANK:  return "jingle instrument not found in IBNK";
    case IPL_AUDIO_E_FORMAT:   return "unsupported wave format or rate";
    case IPL_AUDIO_E_NOMEM:    return "out of memory";
    }
    return "unknown error";
}
