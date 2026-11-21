/*
 * recolor.h - optional recolouring of the indigo parts of the animation
 */
#ifndef RECOLOR_H
#define RECOLOR_H

#include <gccore.h>
#include "config.h"

typedef struct {
	int active;
	float dh;     /* hue shift (degrees)  */
	float ks;     /* saturation factor    */
	float kv;     /* value factor         */
} Recolor;

void recolor_setup(Recolor *r, const Config *c);
/* transform mapping the cube indigo (95,80,190) onto target (NULL: only hue/sat/brightness) */
void recolor_setup_target(Recolor *r, const u8 *target, const Config *c);
void recolor_hsv(float h, float s, float v, u8 rgb[3]);
void recolor_rgb(const Recolor *r, u8 rgb[3]);
void recolor_s10(const Recolor *r, GXColorS10 *c);
/* recolour an RGBA8 GX texture in place (w x h, tiled 4x4 AR/GB blocks) */
void recolor_rgba8(const Recolor *r, u8 *tex, u16 w, u16 h);

#endif
