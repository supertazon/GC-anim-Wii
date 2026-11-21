/*
 * anim.c - J3D1bck1 joint animation and IPK1 material colour animation
 */
#include <string.h>
#include "anim.h"
#include "gcmath.h"

static inline u32 rd32(const u8 *p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }
static inline u16 rd16(const u8 *p) { return (u16)(p[0] << 8 | p[1]); }
static inline float rdf(const u8 *p) { u32 v = rd32(p); float f; memcpy(&f, &v, 4); return f; }

/* key value accessors: float keys or s16 keys converted exactly to float */
typedef float (*KeyFn)(const u8 *k, int i);
static float key_f32(const u8 *k, int i) { return rdf(k + 4 * i); }
static float key_s16(const u8 *k, int i) { return (float)(s16)rd16(k + 2 * i); }

/* FUN_81306df0 / FUN_81306ec4: key search + hermite */
static float sample_keys(float t, int count, int tanmode, const u8 *k, KeyFn K)
{
	int st = tanmode ? 4 : 3;
	int last = st * (count - 1), p = 0, n = count;

	if (!(t >= K(k, 0)))
		return K(k, 1);
	if (t >= K(k, last))
		return K(k, last + 1);
	while (n > 1) {
		int h = n >> 1;
		if (K(k, p + h * st) <= t) {
			p += h * st;
			n -= h;
		} else {
			n = h;
		}
	}
	return gm_hermite(t, K(k, p), K(k, p + 1), K(k, p + st - 1),
	                  K(k, p + st), K(k, p + st + 1), K(k, p + st + 2));
}

int bck_init(BckAnim *a, const u8 *file, u32 size)
{
	const u8 *s = file + 0x20;
	if (size < 0x60 || memcmp(file, "J3D1bck1", 8) || memcmp(s, "ANK1", 4))
		return -1;
	a->base = s;
	a->shift = s[9];
	a->njoints = rd16(s + 0xC);
	a->jtab = s + rd32(s + 0x14);
	a->scale = s + rd32(s + 0x18);
	a->rot = s + rd32(s + 0x1C);
	a->trans = s + rd32(s + 0x20);
	return 0;
}

void bck_srt(const BckAnim *a, int j, float t, float S[3], int R[3], float T[3])
{
	const u8 *jt = a->jtab + 0x36 * j;
	int ax;

	for (ax = 0; ax < 3; ax++) {
		const u8 *sc = jt + 0x12 * ax, *ro = sc + 6, *tr = sc + 12;
		u16 cnt, idx, tm;

		cnt = rd16(sc); idx = rd16(sc + 2); tm = rd16(sc + 4);
		if (cnt == 1) S[ax] = rdf(a->scale + 4 * idx);
		else if (cnt == 0) S[ax] = 1.0f;
		else S[ax] = sample_keys(t, cnt, tm, a->scale + 4 * idx, key_f32);

		cnt = rd16(ro); idx = rd16(ro + 2); tm = rd16(ro + 4);
		if (cnt == 1) R[ax] = (s16)((s16)rd16(a->rot + 2 * idx) << a->shift);
		else if (cnt == 0) R[ax] = 0;
		else R[ax] = (s16)((int)sample_keys(t, cnt, tm, a->rot + 2 * idx, key_s16) << a->shift);

		cnt = rd16(tr); idx = rd16(tr + 2); tm = rd16(tr + 4);
		if (cnt == 1) T[ax] = rdf(a->trans + 4 * idx);
		else if (cnt == 0) T[ax] = 0.0f;
		else T[ax] = sample_keys(t, cnt, tm, a->trans + 4 * idx, key_f32);
	}
}

void bck_local(const BckAnim *a, int j, float t, Mtx out)
{
	float S[3], T[3];
	int R[3];
	bck_srt(a, j, t, S, R, T);
	gm_euler(out, T[0], T[1], T[2], R[0], R[1], R[2]);
	if (S[0] != 1.0f || S[1] != 1.0f || S[2] != 1.0f)
		gm_scalecols(out, S[0], S[1], S[2]);
}

int ipk_init(IpkAnim *a, const u8 *file, u32 size)
{
	u32 otab, oent;
	int c;
	if (size < 0x24 || memcmp(file, "IPK1", 4))
		return -1;
	a->base = file;
	otab = rd32(file + 0xC);
	oent = rd32(file + 0x10);
	a->map = file + otab;
	a->nmap = (oent - otab) / 2;
	a->ent = file + oent;
	for (c = 0; c < 4; c++)
		a->tab[c] = file + rd32(file + 0x14 + 4 * c);
	return 0;
}

GXColor ipk_color(const IpkAnim *a, int mat, float frame)
{
	float t = frame / 30.0f;
	const u8 *e;
	u8 out[4];
	int c;
	GXColor col;

	if (mat >= a->nmap) {
		col.r = col.g = col.b = col.a = 255;
		return col;
	}
	e = a->ent + 0x18 * (s16)rd16(a->map + 2 * mat);
	for (c = 0; c < 4; c++) {
		u16 cnt = rd16(e + 6 * c), idx = rd16(e + 6 * c + 2), tm = rd16(e + 6 * c + 4);
		float v;
		if (cnt == 1) v = rdf(a->tab[c] + 4 * idx);
		else if (cnt == 0) v = 0.0f;
		else v = sample_keys(t, cnt, tm, a->tab[c] + 4 * idx, key_f32);
		if (!(v >= 0.0f)) out[c] = 0;
		else if (v <= 255.0f) out[c] = (u8)(int)v;
		else out[c] = 255;
	}
	col.r = out[0]; col.g = out[1]; col.b = out[2]; col.a = out[3];
	return col;
}
