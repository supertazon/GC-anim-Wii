/*
 * gcmath.h - matrix/trig helpers reproducing the GameCube IPL's own math
 *
 * The IPL builds a 4096-entry float sine table at boot and every angle in the
 * boot animation is a 16-bit "binary angle" (0x10000 = 360 degrees) looked up
 * in that table.  Reproducing the table (and the exact order of the float
 * operations) is what makes our matrices identical to the original ones.
 */
#ifndef GCMATH_H
#define GCMATH_H

#include <gccore.h>

void  gm_init(void);                 /* build the sine table (call once) */
float gm_sin(int angle);             /* angle: s16 binary angle           */
float gm_cos(int angle);

void gm_ident(Mtx m);
void gm_trans(Mtx m, float x, float y, float z);
void gm_scale(Mtx m, float x, float y, float z);
/* rotation Rz*Ry*Rx (s16 angles) with translation (IPL FUN_81307768) */
void gm_euler(Mtx m, float tx, float ty, float tz, int rx, int ry, int rz);
/* rotation about one axis ('x','y','z') (IPL FUN_813075bc) */
void gm_rotaxis(Mtx m, char axis, int angle);
/* scale the three rotation columns (IPL FUN_81307b08) */
void gm_scalecols(Mtx m, float sx, float sy, float sz);
/* ab = a * b, paired-single version like the IPL's PSMTXConcat */
void gm_concat(Mtx a, Mtx b, Mtx ab);
void gm_copy(Mtx src, Mtx dst);
/* C_MTXLookAt */
void gm_lookat(Mtx m, float px, float py, float pz, float ux, float uy, float uz,
               float tx, float ty, float tz);
/* transform a point */
void gm_multvec(Mtx m, const float in[3], float out[3]);
/* cubic hermite, IPL FUN_813080bc (exact operation order) */
float gm_hermite(float t, float t0, float v0, float m0, float t1, float v1, float m1);

#endif
