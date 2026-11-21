/*
 * gcmath.c - matrix/trig helpers reproducing the GameCube IPL's own math
 * (see gcmath.h).  Compile with -ffp-contract=off: the IPL code uses separate
 * fmuls/fadds, never fused multiply-adds, except inside PSMTXConcat.
 */
#include <math.h>
#include <string.h>
#include "gcmath.h"

#define SIN_BITS 12
#define SIN_N    (1 << SIN_BITS)
#define SIN_SHIFT (16 - SIN_BITS)

static float sin_tab[SIN_N + SIN_N / 4];

void gm_init(void)
{
	/* IPL FUN_81307ec0: tab[i] = (float)sin((float)(2pi_f / N) * (float)i) */
	const float twopi = 6.2831855f;
	float step = (float)((double)twopi / (double)(float)SIN_N);
	int i;
	for (i = 0; i < SIN_N + SIN_N / 4; i++) {
		float a = step * (float)i;
		sin_tab[i] = (float)sin((double)a);
	}
}

float gm_sin(int a) { return sin_tab[(a & 0xFFFF) >> SIN_SHIFT]; }
float gm_cos(int a) { return sin_tab[((a & 0xFFFF) >> SIN_SHIFT) + SIN_N / 4]; }

void gm_ident(Mtx m)
{
	memset(m, 0, sizeof(Mtx));
	m[0][0] = m[1][1] = m[2][2] = 1.0f;
}

void gm_trans(Mtx m, float x, float y, float z)
{
	gm_ident(m);
	m[0][3] = x; m[1][3] = y; m[2][3] = z;
}

void gm_scale(Mtx m, float x, float y, float z)
{
	memset(m, 0, sizeof(Mtx));
	m[0][0] = x; m[1][1] = y; m[2][2] = z;
}

void gm_euler(Mtx m, float tx, float ty, float tz, int rx, int ry, int rz)
{
	float sx = gm_sin(rx), cx = gm_cos(rx);
	float sy = gm_sin(ry), cy = gm_cos(ry);
	float sz = gm_sin(rz), cz = gm_cos(rz);
	float czsy = cz * sy, szsy = sz * sy;

	m[0][0] = cz * cy;
	m[0][1] = (-sz * cx) + (sx * czsy);
	m[0][2] = (sz * sx) + (cx * czsy);
	m[0][3] = tx;
	m[1][0] = sz * cy;
	m[1][1] = (cz * cx) + (sx * szsy);
	m[1][2] = (-cz * sx) + (cx * szsy);
	m[1][3] = ty;
	m[2][0] = -sy;
	m[2][1] = cy * sx;
	m[2][2] = cy * cx;
	m[2][3] = tz;
}

void gm_rotaxis(Mtx m, char axis, int a)
{
	float s = gm_sin(a), c = gm_cos(a);
	gm_ident(m);
	switch (axis) {
	case 'x': case 'X':
		m[1][1] = c; m[1][2] = -s; m[2][1] = s; m[2][2] = c;
		break;
	case 'y': case 'Y':
		m[0][0] = c; m[0][2] = s; m[2][0] = -s; m[2][2] = c;
		break;
	default:
		m[0][0] = c; m[0][1] = -s; m[1][0] = s; m[1][1] = c;
		break;
	}
}

void gm_scalecols(Mtx m, float sx, float sy, float sz)
{
	int r;
	for (r = 0; r < 3; r++) {
		m[r][0] *= sx;
		m[r][1] *= sy;
		m[r][2] *= sz;
	}
}

void gm_concat(Mtx a, Mtx b, Mtx ab)
{
#ifdef GEKKO
	ps_guMtxConcat(a, b, ab);
#else
	c_guMtxConcat(a, b, ab);
#endif
}

void gm_copy(Mtx src, Mtx dst)
{
	memcpy(dst, src, sizeof(Mtx));
}

void gm_lookat(Mtx m, float px, float py, float pz, float ux, float uy, float uz,
               float tx, float ty, float tz)
{
	guVector pos = {px, py, pz}, up = {ux, uy, uz}, tgt = {tx, ty, tz};
	guLookAt(m, &pos, &up, &tgt);
}

void gm_multvec(Mtx m, const float in[3], float out[3])
{
	int r;
	float t[3];
	for (r = 0; r < 3; r++)
		t[r] = m[r][0] * in[0] + m[r][1] * in[1] + m[r][2] * in[2] + m[r][3];
	out[0] = t[0]; out[1] = t[1]; out[2] = t[2];
}

float gm_hermite(float t, float t0, float v0, float m0, float t1, float v1, float m1)
{
	float d   = t - t0;
	float inv = 1.0f / (t1 - t0);
	float a   = inv * (d * d);
	float ai  = a * inv;
	float b   = d * ai;
	float c   = 3.0f * ai;
	float bi  = b * inv;
	float hm0 = d + (b - 2.0f * a);
	float hm1 = b - a;
	float hv0 = 1.0f + (2.0f * bi - c);
	float hv1 = -2.0f * bi + c;
	float tv0 = v0 * hv0, tv1 = v1 * hv1, tm0 = m0 * hm0, tm1 = m1 * hm1;
	return tm1 + (tm0 + (tv0 + tv1));
}
