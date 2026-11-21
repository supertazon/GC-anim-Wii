/*
 * anim.h - J3D1bck1 joint animation and IPK1 material colour animation
 * (IPL FUN_81307094 / FUN_813072c8 / FUN_81306df0 / FUN_81306ec4)
 */
#ifndef ANIM_H
#define ANIM_H

#include <gccore.h>

typedef struct {
	const u8 *base;     /* ANK1 section */
	u8  shift;
	u16 njoints;
	const u8 *jtab;
	const u8 *scale, *rot, *trans;
} BckAnim;

typedef struct {
	const u8 *base;
	int nmap;
	const u8 *map, *ent;
	const u8 *tab[4];
} IpkAnim;

int  bck_init(BckAnim *a, const u8 *file, u32 size);
/* local matrix of joint j at frame t: euler(T,R) * diag(S) */
void bck_local(const BckAnim *a, int j, float t, Mtx out);
/* the sampled channels of joint j: scale, rotation (s16 binary angles), translation */
void bck_srt(const BckAnim *a, int j, float t, float S[3], int R[3], float T[3]);

int  ipk_init(IpkAnim *a, const u8 *file, u32 size);
/* colour of material mat at frame (60 Hz frames; the IPK time base is seconds = frame/30) */
GXColor ipk_color(const IpkAnim *a, int mat, float frame);

#endif
