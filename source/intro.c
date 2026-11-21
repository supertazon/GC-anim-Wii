/*
 * intro.c - the logic of the GameCube boot animation (see intro.h)
 *
 * Every function below names the BS2 (NTSC 1.2) routine it reproduces.
 * Field names follow the original structures: "G" = global logo state
 * (0x814853f8), "C" = cube state (0x8148b8d8).
 */
#include <string.h>
#include <math.h>
#include "intro.h"
#include "gcmath.h"

#define TILE      54          /* C+0x59: tile / cube edge length         */
#define TILE_HALF (TILE / 2)

/* move table (IPL data at 0x8137fdd8): {move type, tile flag, tile lifetime} */
static const u8 moves[][3] = {
	{0, 1, 5}, {0, 1, 255}, {3, 1, 255}, {3, 1, 255}, {5, 1, 255}, {0, 2, 255}, {0, 2, 255}, {3, 2, 255},
	{3, 2, 255}, {6, 2, 255}, {0, 3, 255}, {0, 3, 255}, {3, 3, 255}, {3, 3, 255}, {2, 3, 255}, {2, 3, 255},
	{7, 3, 255}, {8, 0, 0}
};

const IntroParams intro_params_60hz = {
	.n14 = 10, .n16 = 16, .n18 = 0,
	.n22 = 5, .n23 = 16,
	.n35 = 18, .n36 = 7, .n37 = 6,
	.n3d = 10, .n3e = 20,
	.n54 = 40, .n57 = 60, .n58 = 5,
	.snd_delay = 0,
	.drop_height = 100.0f, .jump_height = 130.0f, .fall_speed = 30.0f,
	.kA = 0.93f, .kB = 0.925f, .kickA = 4000, .kickB = 12000, .phase_step = 1000,
	.mA = 0.015f, .mB = 0.005f, .thrA = 10.0f, .thrB = 80.0f,
	.fade_step = 12,
};

/* PAL 1.0/1.2 (50 Hz) frame counts, from the PAL BS2 */
const IntroParams intro_params_50hz = {
	.n14 = 8, .n16 = 15, .n18 = 0,
	.n22 = 4, .n23 = 13,
	.n35 = 15, .n36 = 6, .n37 = 6,
	.n3d = 8, .n3e = 17,
	.n54 = 33, .n57 = 50, .n58 = 4,
	.snd_delay = 0,
	.drop_height = 100.0f, .jump_height = 130.0f, .fall_speed = 30.0f,
	.kA = 0.93f, .kB = 0.925f, .kickA = 4000, .kickB = 12000, .phase_step = 1000,
	.mA = 0.015f, .mB = 0.005f, .thrA = 10.0f, .thrB = 80.0f,
	.fade_step = 12,
};

/* ------------------------------------------------------------------------- */
void intro_init(IntroState *s, const IntroParams *p)
{
	float a44;

	memset(s, 0, sizeof(*s));
	s->p = p;
	/* G (FUN_8130c124 / FUN_8130c264) */
	s->wobPhase = 2000;
	s->hermT = 0x3c;
	/* C (FUN_8130ee30 / FUN_8130ecdc) */
	s->gx = 16;
	s->gy = 18;
	s->mtype = moves[0][0];
	s->h = p->drop_height;
	s->grav = (2.0f * p->drop_height) / (float)(p->n22 * p->n22);
	s->y4c = p->jump_height;
	s->v40 = p->fall_speed;
	a44 = (2.0f * p->jump_height) / (float)(p->n3d * p->n3d) - p->fall_speed / (float)p->n3d;
	s->a44 = a44 < 0.0f ? 0.0f : a44;
	s->state = s->next = 0;
	s->flagA0 = s->flagA1 = s->flagA2 = 1;
}

/* FUN_8130c384(0): camera zoom/spin springs at rest during the intro */
static void zoom_camera(IntroState *s)
{
	s->zoomVel = s->zoomVel * 0.8f;
	s->zoom = s->zoom * 0.96f;
	if ((double)s->zoomVel < 0.1)
		s->zoomVel = 0.0f;
	if (s->zoom < 1.035f)
		s->zoom = 0.0f;
	s->rotY = (s16)(int)gm_hermite((float)s->hermT, 0.0f, 0.0f, 0.0f, 60.0f, 0.0f, 0.0f);
	if (++s->hermT > 0x3c)
		s->hermT = 0x3c;
}

/* FUN_8130c718 + FUN_8130c64c: wobble and impact shake amplitudes */
static void shakes(IntroState *s)
{
	const IntroParams *p = s->p;
	float f, f1, f2;

	if (s->zoomVel <= 0.0f)
		f = (float)s->wobAmp * 0.96f;
	else
		f = 30.0f + (float)s->wobAmp;
	if (f < 1.0f)
		f = 0.0f;
	if (0.0f < f)
		s->wobPhase += 7;
	if (5000.0f < f)
		f = 5000.0f;
	s->wobAmp = (s16)(int)f;

	f1 = (float)s->shA * p->kA;
	f2 = (float)s->shB * p->kB;
	if (f1 < p->thrA)
		f1 = 0.0f;
	if (f2 < p->thrB)
		f2 = 0.0f;
	if (0.0f < f1 || 0.0f < f2)
		s->shPhase += p->phase_step;
	s->shA = (s16)(int)f1;
	s->shB = (s16)(int)f2;
}

/* FUN_8130ef60: tile lifetimes */
static void age_tiles(IntroState *s)
{
	int f, x, y;
	for (f = 0; f < 3; f++)
		for (x = 0; x < 32; x++)
			for (y = 0; y < 32; y++) {
				IntroCell *c = &s->cells[f][x][y];
				if (c->st == 2) {
					c->st = c->active = c->timer = 0;
				} else if (c->st == 1) {
					c->st = 0;
				} else if (c->active && c->timer != 0xFF) {
					if (--c->timer == 0)
						c->st = 2;
				}
			}
}

/* FUN_8130f088: move the cube by one tile and drop a tile there */
static void apply_move(IntroState *s)
{
	int t = s->mtype;
	if (t == 2) s->gx = s->gx < 1 ? 0 : s->gx - 1;
	else if (t == 0) s->gx = s->gx < 0x20 ? s->gx + 1 : 0x20;
	else if (t == 1) s->gy = s->gy < 0x20 ? s->gy + 1 : 0x20;
	else if (t == 3) s->gy = s->gy < 1 ? 0 : s->gy - 1;
	else if (t >= 4 && t < 7) {
		int f = t - 4;
		if ((f + 3 - s->face) % 3 == 1 && s->gy == 16) {
			s->gy = s->gx; s->gx = 16; s->face = f;
		}
		if ((f + 3 - s->face) % 3 == 2 && s->gx == 16) {
			s->gx = s->gy; s->gy = 16; s->face = f;
		}
	}
	age_tiles(s);
	if (s->gx < 32 && s->gy < 32) {
		IntroCell *c = &s->cells[s->face][s->gx][s->gy];
		c->timer = moves[s->mi][2];
		c->active = moves[s->mi][1];
		c->st = 1;
		c->seq = (u8)(s->nlaid < 255 ? s->nlaid : 255);
		s->nlaid++;
	}
}

/* FUN_8130f6b8 */
static int step_done(IntroState *s)
{
	const IntroParams *p = s->p;
	int lim, v;
	if (s->mtype < 4) {
		lim = 0x4000;
		v = (s->frm << 14) / (p->n14 - p->n18);
	} else {
		lim = 0x8000;
		v = (s->frm << 15) / (p->n16 - p->n18);
	}
	if (lim < v && s->frm1a == 0)
		s->frm1a = s->frm;
	return s->frm1a > 0 && s->frm1a + p->n18 <= s->frm;
}

/* FUN_8130f760: the initial drop */
static int drop(IntroState *s)
{
	const IntroParams *p = s->p;
	s->h = s->h - s->vel;
	if (s->h < 0.0f) {
		s->h = 0.0f;
		if (s->flagA0) {
			s->shA = p->kickA;
			s->shPhase = 0x7fff;
			s->flagA0 = 0;
		}
	}
	s->vel = s->vel + s->grav;
	if (p->n22 + p->n23 < s->cnt21) {
		s->flagA0 = 1;
		return 1;
	}
	s->cnt21++;
	return 0;
}

/* FUN_81310104: jingle trigger */
static void sound_tick(IntroState *s)
{
	if (s->flagA2 && s->p->snd_delay < s->snd) {
		s->sound_start = 1;
		s->flagA2 = 0;
	}
	if (s->flagA2)
		s->snd++;
}

/* FUN_813101f4: the cube state machine */
static void cube_tick(IntroState *s)
{
	const IntroParams *p = s->p;

	s->state = s->next;
	sound_tick(s);
	switch (s->state) {
	case 0:
		s->next = drop(s);
		break;
	case 1:
		if (step_done(s)) {
			s->frm = 0;
			s->frm1a = 0;
			s->mi++;
			if (moves[s->mi][0] == 8) {
				s->done = 1;
				age_tiles(s);
			} else {
				apply_move(s);
				s->mtype = moves[s->mi][0];
			}
		}
		if (s->mtype == 7) {
			s->next = 2;
		} else {
			s->next = 1;
			s->frm++;
		}
		break;
	case 2:
		s->c34++;
		s->next = (p->n35 + p->n36 < s->c34) ? 3 : 2;
		break;
	case 3:
		s->y4c = s->y4c - s->v40;
		if (s->y4c < 0.0f) {
			s->y4c = 0.0f;
			if (s->flagA1) {
				s->shB = p->kickB;
				s->shPhase = 0x7fff;
				s->flagA1 = 0;
			}
		}
		s->v40 = s->v40 + s->a44;
		if (p->n3d + p->n3e < s->c3c) {
			s->flagA1 = 1;
			s->next = 4;
		} else {
			s->c3c++;
			s->c56++;
			if (p->n57 + p->n58 <= s->c56)
				s->c56 = p->n57 + p->n58;
			s->next = 3;
		}
		break;
	case 4:
		s->c55++;
		if (p->n54 < s->c55) {
			s->c55 = p->n54;
			s->done = 1;
		}
		s->logoA = (s->c55 * 255) / p->n54;
		s->c56++;
		if (s->c56 < p->n57 + p->n58)
			s->done = 0;
		else
			s->c56 = p->n57 + p->n58;
		if (s->shB != 0)
			s->done = 0;
		break;
	}
}

void intro_update(IntroState *s)
{
	s->sound_start = 0;
	if (!s->done)
		zoom_camera(s);
	shakes(s);
	cube_tick(s);
	/* end: fade to black (FUN_81302508), as when a game disc is ready */
	if (s->done) {
		s->fade += s->p->fade_step;
		if (s->fade > 255) {
			s->fade = 255;
			s->finished = 1;
		}
	}
	s->frame++;
}

/* ------------------------------------------------------------------------- */
/*  matrices                                                                  */
/* ------------------------------------------------------------------------- */

/* FUN_8130c7c8 + scaling of FUN_8130ca40 */
static void world_matrix(IntroState *s, Mtx out)
{
	const IntroParams *p = s->p;
	Mtx m, aux, t;
	float k = (0.0f * s->zoom) / 5000.0f;     /* G+0x24 is 0 after FUN_8130c264 */
	float one_k = 1.0f - k, k1000 = 1000.0f * k;
	float amp = (float)s->wobAmp, A, B, ty, sx, sy, sn;
	int ph = s->wobPhase, r2, r3, r4, rx, rz;

	gm_trans(aux, 15.0f, 15.0f, 15.0f);
	gm_euler(m, 0, 0, 0, 0x2000, (s16)0xe6ec, (s16)0xeaab);   /* isometric view of the big cube */
	gm_concat(m, aux, aux);
	r2 = (int)(k1000 + one_k * (amp * gm_sin(ph * 0x46)));
	r3 = (int)(k1000 + one_k * (amp * gm_sin(ph * 0x3c)));
	r4 = (int)(k1000 + one_k * (amp * gm_sin(ph * 0x5a)));
	gm_euler(m, 0, 0, 0, r4, r3, r2);
	gm_concat(m, aux, m);

	A = (float)s->shA;
	B = (float)s->shB;
	sn = gm_sin(s->shPhase);
	ty = ((p->mA * A) * sn) + ((p->mB * B) * sn);
	rz = (int)(A * sn);
	rx = (int)((-1.0f * B) * sn);
	gm_euler(t, 0, ty, 0, rx, 0, rz);
	gm_concat(t, m, m);
	gm_euler(t, 0, 0, 0, 0, s->rotY, 0);
	gm_concat(t, m, m);

	gm_ident(t);
	gm_concat(m, t, out);
	sx = 1.0f - ((1.0f - 0.5f) * s->zoom) / 5000.0f;
	sy = 1.0f + (1.0f * s->zoom) / 5000.0f;
	gm_scalecols(out, sx, sy, sx);
}

/*
 * C-stick camera, same geometry as GC-anim-Wii v1:
 *   GRRLIB_Camera3dSettings(200+sx, -200+sy, 200-sx, up 0,1,0, target 0,ty,0)
 * with v1's flipped ortho (guOrtho top/bottom and left/right swapped = 180 deg about z).
 * The stick moves the eye of an isometric camera 346 units from its target, so the view
 * turns by at most ~20 deg.  We take the rotation between v1's resting and moved camera
 * (in camera space) and apply it around our own look-at target.
 */
static void cam_rotation(float sx, float sy, Mtx q)
{
	Mtx r0, r1;
	int i, j, k;
	gm_lookat(r0, 200.0f, -200.0f, 200.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	gm_lookat(r1, 200.0f + sx, -200.0f + sy, 200.0f - sx, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	/* q = r1 * r0^T (rotation parts), then conjugate by diag(-1,-1,1) */
	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++) {
			float acc = 0.0f;
			for (k = 0; k < 3; k++)
				acc += r1[i][k] * r0[j][k];
			q[i][j] = ((i == 2) == (j == 2)) ? acc : -acc;
		}
	q[0][3] = q[1][3] = q[2][3] = 0.0f;
}

static void view_matrix(IntroState *s, Mtx out)
{
	/* FUN_813031a4: eye at 224/tan(fovy/2), fovy = 20, y offset -55 */
	float d = 224.0f / (float)tan((double)((3.1415927f * 20.0f) / 360.0f));
	Mtx v, q, t;
	gm_lookat(v, 0.0f, -55.0f, d, 0.0f, 1.0f, 0.0f, 0.0f, -55.0f, 0.0f);
	if (s->cam_sx || s->cam_sy) {
		/* rotate around the look-at target, which is at (0, 0, -d) in camera space */
		cam_rotation(s->cam_sx, s->cam_sy, q);
		gm_trans(t, 0.0f, 0.0f, d);
		gm_concat(t, v, v);
		gm_concat(q, v, v);
		gm_trans(t, 0.0f, 0.0f, -d);
		gm_concat(t, v, out);
	} else {
		gm_copy(v, out);
	}
}

/* FUN_8130f328 / FUN_8130f460 / FUN_8130f834 / FUN_8130fa50 / FUN_8130fcd0 */
static void cube_matrix(IntroState *s, Mtx W, Mtx out)
{
	const IntroParams *p = s->p;
	Mtx M, t;
	int k = (TILE * 3) >> 1;

	if (s->state <= 1) {
		int a, ry = 0;
		if (s->mtype < 4) {
			a = (s->frm << 14) / (p->n14 - p->n18);
			if (a > 0x4000) a = 0x4000;
		} else {
			a = (s->frm << 15) / (p->n16 - p->n18);
			if (a > 0x8000) a = 0x8000;
		}
		gm_ident(M);
		gm_euler(t, -TILE_HALF, TILE_HALF, -TILE_HALF, 0, 0, 0);
		gm_concat(M, t, M);
		gm_rotaxis(t, 'x', (s16)a);
		gm_concat(t, M, M);
		switch (s->mtype) {
		case 2: ry = 0x4000; break;
		case 0: ry = -0x4000; break;
		case 1: ry = 0x7fff; break;
		default: ry = 0; break;
		}
		gm_trans(t, TILE_HALF, 0, TILE_HALF);
		gm_concat(t, M, M);
		gm_euler(t, -TILE_HALF, 0, -TILE_HALF, 0, ry, 0);
		gm_concat(t, M, M);
		gm_trans(t, (float)(-(s->gx - 16) * TILE), 0, (float)(-(s->gy - 16) * TILE));
		gm_concat(t, M, M);
		if (s->face == 1) {
			gm_euler(t, 0, 0, 0, 0x4000, 0, 0x4000);
			gm_concat(t, M, M);
		}
		if (s->face == 2) {
			gm_euler(t, 0, 0, 0, 0, -0x4000, -0x4000);
			gm_concat(t, M, M);
		}
		gm_euler(t, k, k, k, 0, 0, 0);
		gm_concat(t, M, M);
		gm_concat(W, M, M);
		if (s->state == 0) {
			gm_trans(t, 0, s->h, 0);
			gm_concat(t, M, M);
		}
	} else {
		float sc = 1.0f + (0.5f * (float)s->c34) / (float)p->n35;
		unsigned u1, u2;
		float dv4, dv3, tq, y;
		if ((double)sc > 1.5)
			sc = 1.5f;
		u1 = (unsigned)p->n37 * 0x4000;
		u2 = (u1 * (unsigned)s->c34) / p->n35;
		if (u1 < u2)
			u2 = u1;
		gm_scale(M, sc, sc, sc);
		dv4 = (float)(TILE >> 1) - ((((float)s->c34 * (1.0f + sc)) * (float)TILE) / (float)p->n35) * 0.5f;
		dv3 = -((sc * (float)TILE) * 0.5f);
		if (dv4 < dv3)
			dv4 = dv3;
		tq = (sc * (float)-TILE) * 0.5f;
		gm_euler(t, tq, dv4, tq, (s16)u2, 0, 0);
		gm_concat(t, M, M);
		gm_euler(t, 0, 0, 0, 0, -0x4000, -0x4000);
		gm_concat(t, M, M);
		gm_euler(t, k, k, k, 0, 0, 0);
		gm_concat(t, M, M);
		gm_concat(W, M, M);
		if (s->state == 2) {
			int u = p->n35 - s->c34;
			float uf, top = p->jump_height;
			int n4 = p->n35 * p->n35 * p->n35 * p->n35;
			if (u < 0)
				u = 0;
			uf = (float)u;
			y = top - (((uf * uf) * uf) * (top * uf)) / (float)n4;
			if (top < y)
				y = top;
		} else {
			y = s->y4c;
		}
		gm_trans(t, 0, y, 0);
		gm_concat(t, M, M);
	}
	gm_copy(M, out);
}

void intro_frame(IntroState *s, IntroFrame *f)
{
	const IntroParams *p = s->p;
	static const float light_world[3] = {400.0f, 500.0f, 200.0f};
	Mtx t;
	int face, x, y, cubeA;

	view_matrix(s, f->view);
	world_matrix(s, f->world);
	gm_concat(f->view, f->world, f->big);

	cube_matrix(s, f->world, t);
	gm_concat(f->view, t, f->cube);           /* model scale G+4 = 1.0 */

	gm_euler(t, 81, 81, 81, 0, 0, 0);
	gm_concat(f->world, t, t);
	gm_concat(f->view, t, f->tiles);

	gm_multvec(f->view, light_world, f->light);

	/* alphas (FUN_8130d688) */
	cubeA = ((255 - s->logoA) * 255) / 255;
	f->cube_alpha = (255 * cubeA) / 255;
	{
		int a = s->shA + s->shB;
		if (a > 3000)
			a = 3000;
		f->big_alpha = (255 * a) / 3000;
	}
	f->logo_alpha = s->logoA;
	f->letters_alpha = 255;
	f->letters_frame = (p->n58 < s->c56) ? 60.0f * ((float)(s->c56 - p->n58) / (float)p->n57) : 0.0f;

	/* tiles (FUN_81310528) */
	f->ntiles = 0;
	for (face = 0; face < 3; face++)
		for (x = 0; x < 32; x++)
			for (y = 0; y < 32; y++) {
				const IntroCell *c = &s->cells[face][x][y];
				int a = 255;
				if (!c->active)
					continue;
				if (c->st == 1)
					a = (s->frm * 255) / p->n14;
				if (c->st == 2)
					a = ((p->n14 - s->frm) * 255) / p->n14;
				if (a > 255) a = 255;
				if (a < 0) a = 0;
				a = (a * cubeA) / 255;
				if (f->ntiles < INTRO_MAX_TILES) {
					IntroTile *tl = &f->tile[f->ntiles++];
					tl->face = face; tl->x = x; tl->y = y; tl->alpha = (u8)a;
					tl->seq = c->seq;
				}
			}
	f->tile_rgb[0] = 100;
	f->tile_rgb[1] = 80;
	f->tile_rgb[2] = 190;
	f->fade_alpha = (s->fade > 0 && !s->finished) ? s->fade : 0;

	/* progress values for the colour customisation (not part of the IPL) */
	f->nlaid = s->nlaid;
	f->roll_frac = 1.0f;
	if (s->state <= 1 && !s->done) {
		int len = s->mtype < 4 ? p->n14 : p->n16;
		f->roll_frac = s->state == 0 ? 0.0f : (float)s->frm / (float)len;
		if (f->roll_frac > 1.0f)
			f->roll_frac = 1.0f;
	}
	f->end_fall = 0.0f;
	f->end_jump = 0.0f;
	if (s->state == 2) {
		float r = (float)s->c34 / (float)(p->n35 + p->n36);
		f->end_jump = 0.8f * (r > 1.0f ? 1.0f : r);
	} else if (s->state >= 3) {
		float r = s->state == 3 ? 1.0f - s->y4c / p->jump_height : 1.0f;
		f->end_fall = r;
		f->end_jump = 0.8f + 0.2f * r;
	}
}
