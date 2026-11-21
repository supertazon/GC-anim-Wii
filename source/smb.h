/*
 * smb.h - the SMB 1-1 mode: the user's own Super Mario Bros. dump, run by the
 * NES core (source/nes/) and drawn inside the boot animation's 3D scene.
 *
 * Flow: smb.nes is booted in the background while the animation plays (the title
 * screen is never shown). If B is pressed before the cube reaches the trail's
 * tile (18,18) on the third face, the trail branches off there: the cube rolls
 * on along the level, a second one drops in, Mario jumps out of the first one and
 * the level is played. On death or at the flagpole the level fades, the camera
 * comes back to the logo and the animation finishes, Mario falling onto (death)
 * or jumping into (flagpole) the small cube. Nothing from the game is bundled.
 */
#ifndef SMB_H
#define SMB_H

#include <gccore.h>
#include "video.h"
#include "config.h"
#include "intro.h"
#include "smb_layout.h"

/* the [smb] settings (1-1 or the whole game, test start); call before smb_prepare */
void smb_set_options(const Config *cfg);
/* the folder of smb.nes (ending in '/'): records smb.sav and the run log smb_runs.txt */
void smb_set_save_dir(const char *dir);
/* original colours: allowed once the whole game has been finished (smb.sav's
   unlock code); set for the coming run before smb_play */
int  smb_original_unlocked(void);
void smb_set_original_colours(int on);
/* read smb.nes into memory (call before the devices are unmounted); NULL if absent */
u8 *smb_load_rom(const char *path, u32 *size);

/* the sound: the IPL's instruments (call before the IPL image is freed); 0 if usable */
int  smb_sound_setup(const u8 *ipl, u32 size, int hz50);
/* every animation frame before the branch: the jingle -> 1-1 theme transition once B is
   armed (jingle_frames: frames since the jingle started, 0 if not yet) */
void smb_intro_audio(int jingle_frames, int armed, int volume);
/* stop the SMB sound (after smb_play) */
void smb_sound_end(void);
/* power on the NES with this ROM and boot it into 1-1 in a background thread; 0 if it
   can be used */
int  smb_prepare(u8 *rom, u32 rom_size);
/* (only if the thread could not be created) up to n boot frames, on this thread */
void smb_boot_step(int n);
/* wait for the boot thread (before freeing the ROM) */
void smb_boot_wait(void);
/* the animation is at the branch point (the cube on the third face's tile (18,18)) */
int  smb_branch_point(const IntroState *st);
/* play from the branch point to the end of the animation; returns 1 when the
 * animation has finished (or Start+Z held / reset / power), 0 if the level could
 * not be started (the animation then simply goes on) */
int  smb_play(Video *vid, const Config *cfg, IntroState *st, int have_sound,
              volatile int *reset_flag, volatile int *power_flag);

/* ---- drawing (smb_render.c) ---- */
typedef struct {
	float dx, dy;              /* world shift, in face tiles */
	int level_alpha;           /* ground + flat layer, 0..255 */
	int clip;                  /* NES x beyond which the level is not built yet (0..256) */
	int show_cubes;            /* the two ground-laying cubes */
	int resting_cube;          /* a cube at rest on the branch tile (fly-back) */
	const IntroFrame *trail;   /* draw the animation's trail tiles (NULL: no) */
	float tdx, tdy;            /* shift of the trail and the resting cube (they can move
	                              independently of the level in the endings) */
	/* the player */
	int ply_show, ply_alpha;
	float ply_scale;
	float ply_rot;             /* spin in the face's plane, radians */
	float ply_face;            /* 0: lies on the face; 1: turned to face the viewer */
	int ply_mirror;            /* drawn mirrored (facing the other way) */
	int32_t ply_level_x;       /* level_x the player's NES position refers to */
	Mtx ply_mtx;               /* matrix (tile space -> view) to draw it with */
	float ply_off[3];          /* extra translation in that space */
} SmbDraw;

void smb_render_init(void);
/* original colours (the NES's palette, the ground in its colour) */
void smb_render_set_colours(int original);
void smb_render_prepare(const SmbView *v);
void smb_render_draw(const IntroFrame *cam, const SmbView *v, const SmbDraw *d);
/* the shift that frames the play area for this scroll position */
void smb_render_play_shift(const IntroFrame *cam, int32_t level_x, float *dx, float *dy);
/* camera tiles matrix with the world shifted */
void smb_render_shifted(const IntroFrame *cam, float dx, float dy, Mtx out);
/* view-space x of NES picture x nes_x, with the world shifted by (dx, dy) */
float smb_render_nes_view_x(const IntroFrame *cam, float dx, float dy, int32_t level_x, float nes_x);
/* view-space y of NES picture point (nes_x, nes_y), with the world shifted by (dx, dy) */
float smb_render_nes_view_y(const IntroFrame *cam, float dx, float dy, int32_t level_x, float nes_x, float nes_y);
void smb_render_nes_view_point(const IntroFrame *cam, float dx, float dy, int32_t level_x, float nes_x, float nes_y,
                               float out[3]);
/* the score (48 x 8, 1 bit per pixel) centred at view (x, y), digits h units high */
void smb_render_score(const uint8_t *bits, float x, float y, float h, const u8 rgb[3], int alpha);
/* the corner's line: 96 x 8 pixels (12 characters) */
void smb_render_hud(const uint8_t *bits, float x, float y, float h, const u8 rgb[3], int alpha);
void smb_render_score_slot(int slot, const uint8_t *bits, float x, float y, float h, const u8 rgb[3], int alpha);
/* a sprite image (SMB_GW x SMB_GH, palette slots) into texture i (0..5), then drawn
 * face-on centred at view (x, y), px view units per NES pixel (uses the level's palettes) */
void smb_render_sprite_upload(int i, const uint8_t *img);
void smb_render_sprite_view(int i, float x, float y, float px, int alpha);
void smb_render_text(const SmbText *t, float x, float y, float px, const u8 rgb[3], int alpha);
/* the same in one of two textures (0 or 1: two texts in one frame) */
void smb_render_text_slot(int slot, const SmbText *t, float x, float y, float px, const u8 rgb[3], int alpha);
/* the world screen's Mario (smb_mario_icon): centred on x, top at y_top */
void smb_render_icon_upload(const uint8_t *img);
void smb_render_icon_view(float x, float y_top, float px, int alpha);
/* tile-space point of the player's feet (unshifted world) */
void smb_render_player_point(const SmbView *v, int32_t level_x, float out[3]);

#endif
