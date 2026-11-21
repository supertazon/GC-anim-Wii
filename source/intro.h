/*
 * intro.h - the logic of the GameCube boot animation
 *
 * A C port of the IPL's own per-frame logic (BS2 NTSC 1.2 FUN_8130cca8 /
 * FUN_8130c718 / FUN_8130c64c / FUN_813101f4 / FUN_8130ca40 / FUN_8130fcd0 ...).
 * It reproduces the camera, the "impact" shakes, the rolling cube, the tiles,
 * the alphas of every model and the end fade, frame for frame.
 *
 * The 60 Hz (NTSC/MPAL/PAL 1.1) and 50 Hz (PAL 1.0/1.2) IPLs use different
 * frame counts; both parameter sets are provided.
 */
#ifndef INTRO_H
#define INTRO_H

#include <gccore.h>

#define INTRO_MAX_TILES 64

typedef struct {
	/* cube timing (frames) */
	u8 n14, n16, n18;           /* roll duration (90 deg / 180 deg), extra wait     */
	u8 n22, n23;                /* drop: frames to fall, frames to wait              */
	u8 n35, n36, n37;           /* final jump: rise frames, wait, quarter turns      */
	u8 n3d, n3e;                /* final fall frames, wait                           */
	u8 n54, n57, n58;           /* logo fade-in, letters anim length, letters delay  */
	u8 snd_delay;               /* jingle start delay                                */
	float drop_height;          /* 100 */
	float jump_height;          /* 130 */
	float fall_speed;           /* 30  */
	/* camera shakes (FUN_8130c64c) */
	float kA, kB;               /* amplitude decay per frame */
	s16 kickA, kickB;           /* initial amplitudes        */
	s16 phase_step;
	float mA, mB;               /* vertical translation factors */
	float thrA, thrB;           /* cut-off thresholds        */
	/* end fade */
	int fade_step;
} IntroParams;

extern const IntroParams intro_params_60hz;
extern const IntroParams intro_params_50hz;

typedef struct {
	u8 timer, active, st;
	u8 seq;                     /* order in which the tile was laid (customisation) */
} IntroCell;

typedef struct {
	const IntroParams *p;
	int frame;
	/* G: logo/global state (subset used by the intro) */
	s16 rotY, wobPhase, wobAmp, shA, shB, shPhase, hermT;
	float zoom, zoomVel;
	int logoA;                  /* G+0x5b */
	/* C: cube state */
	int gx, gy, face, mi, mtype, done;
	int frm, frm1a;
	float h, vel, grav;
	int cnt21, c34, c3c, c55, c56;
	float y4c, v40, a44;
	int state, next;
	int flagA0, flagA1, flagA2, snd;
	IntroCell cells[3][32][32];
	int nlaid;                  /* tiles laid so far */
	/* end fade */
	int fade;
	int finished;
	int sound_start;            /* set on the frame the jingle must start */
	/* user camera (C-stick): extra yaw/pitch in s16 binary angles */
	float cam_sx, cam_sy;        /* C-stick, raw PAD_SubStickX/Y (the SMB ending eases it back to 0) */
} IntroState;

typedef struct {
	u8 face, x, y, alpha;
	u8 seq;              /* laying order */
} IntroTile;

typedef struct {
	Mtx view;            /* camera (DAT_814851b0), incl. user camera */
	Mtx world;           /* scene rotation/scale (DAT_814854d0)      */
	Mtx cube;            /* view * small cube model matrix           */
	Mtx big;             /* view * world (big cube, logo)            */
	Mtx tiles;           /* view * world * T(81,81,81)               */
	float light[3];      /* light 0 position in view space           */
	int cube_alpha, big_alpha, logo_alpha, letters_alpha;
	float letters_frame;
	int ntiles;
	IntroTile tile[INTRO_MAX_TILES];
	u8 tile_rgb[3];
	int fade_alpha;      /* black fade quad, 0 = none */
	/* progress values for the colour customisation */
	int nlaid;           /* tiles laid so far                                   */
	float roll_frac;     /* 0..1 through the current roll (1 once rolling ends) */
	float end_fall;      /* 0..1 through the final fall, 1 from the impact on   */
	float end_jump;      /* 0..1 through the final rise + fall                  */
} IntroFrame;

void intro_init(IntroState *s, const IntroParams *p);
void intro_update(IntroState *s);
void intro_frame(IntroState *s, IntroFrame *f);

#endif
