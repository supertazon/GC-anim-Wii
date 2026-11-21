/*
 * scene.h - drawing one frame of the boot animation
 */
#ifndef SCENE_H
#define SCENE_H

#include "ipl.h"
#include "intro.h"
#include "config.h"

int  scene_init(IplAssets *a, const Config *cfg);
/* per frame: evaluate animations for the frame (call once per frame) */
void scene_prepare(const IntroFrame *f);
/* draw the whole scene (called once per AA pass) */
void scene_draw(const IntroFrame *f);
void scene_free(void);
/* the Mario colours (red cube and logo, brown tiles) from now on */
void scene_mario_colours(void);
/* background / clear colour (black unless customised) */
GXColor scene_background(void);

/* SMB mode (smb_render.c) */
void scene_light(const IntroFrame *f);
GXTexObj *scene_tile_texture(void);
void scene_level_rgb(int k, u8 out[3]);
void scene_theme_rgb(u8 out[3]);
/* the logo's lettering (as last prepared), in view space: x0, x1, y0, y1; 0 if hidden */
int  scene_letters_bounds(float b[4]);
void scene_draw_block(Mtx base);
void scene_draw_trail(const IntroFrame *f);

#endif
