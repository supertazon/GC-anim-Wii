/*
 * recolor.c - optional recolouring of the indigo parts of the animation
 *
 * Every purple colour of the animation (cube TEV colours, tiles, logo TEV
 * colours and the logo's RGBA8 textures) goes through the same HSV transform:
 * hue shift, saturation and value scaling.  A "theme colour" is turned into
 * the transform that maps the small cube's main colour (95,80,190) onto it.
 */
#include <math.h>
#include "recolor.h"

static void rgb2hsv(float r, float g, float b, float *h, float *s, float *v)
{
	float mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
	float mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
	float d = mx - mn;
	*v = mx;
	*s = mx > 0.0f ? d / mx : 0.0f;
	if (d <= 0.0f)
		*h = 0.0f;
	else if (mx == r)
		*h = 60.0f * fmodf((g - b) / d + 6.0f, 6.0f);
	else if (mx == g)
		*h = 60.0f * ((b - r) / d + 2.0f);
	else
		*h = 60.0f * ((r - g) / d + 4.0f);
}

static void hsv2rgb(float h, float s, float v, float *r, float *g, float *b)
{
	float c, x, m, hh;
	h = fmodf(h, 360.0f);
	if (h < 0.0f)
		h += 360.0f;
	c = v * s;
	hh = h / 60.0f;
	x = c * (1.0f - fabsf(fmodf(hh, 2.0f) - 1.0f));
	m = v - c;
	switch ((int)hh) {
	case 0: *r = c; *g = x; *b = 0; break;
	case 1: *r = x; *g = c; *b = 0; break;
	case 2: *r = 0; *g = c; *b = x; break;
	case 3: *r = 0; *g = x; *b = c; break;
	case 4: *r = x; *g = 0; *b = c; break;
	default: *r = c; *g = 0; *b = x; break;
	}
	*r += m; *g += m; *b += m;
}

void recolor_setup_target(Recolor *r, const u8 *target, const Config *c)
{
	r->dh = (float)c->hue;
	r->ks = (float)c->saturation / 100.0f;
	r->kv = (float)c->brightness / 100.0f;
	if (target) {
		float h0, s0, v0, h1, s1, v1;
		rgb2hsv(95 / 255.0f, 80 / 255.0f, 190 / 255.0f, &h0, &s0, &v0);
		rgb2hsv(target[0] / 255.0f, target[1] / 255.0f, target[2] / 255.0f, &h1, &s1, &v1);
		r->dh += h1 - h0;
		r->ks *= s1 / s0;
		r->kv *= v1 / v0;
	}
	r->active = r->dh != 0.0f || r->ks != 1.0f || r->kv != 1.0f;
}

void recolor_setup(Recolor *r, const Config *c)
{
	recolor_setup_target(r, c->have_color ? c->color : NULL, c);
}

void recolor_hsv(float h, float s, float v, u8 rgb[3])
{
	float R, G, B;
	hsv2rgb(h, s, v, &R, &G, &B);
	rgb[0] = (u8)(R * 255.0f + 0.5f);
	rgb[1] = (u8)(G * 255.0f + 0.5f);
	rgb[2] = (u8)(B * 255.0f + 0.5f);
}

static float clamp01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }

static void xform(const Recolor *r, float *R, float *G, float *B)
{
	float h, s, v;
	rgb2hsv(*R, *G, *B, &h, &s, &v);
	hsv2rgb(h + r->dh, clamp01(s * r->ks), clamp01(v * r->kv), R, G, B);
}

void recolor_rgb(const Recolor *r, u8 rgb[3])
{
	float R = rgb[0] / 255.0f, G = rgb[1] / 255.0f, B = rgb[2] / 255.0f;
	if (!r->active)
		return;
	xform(r, &R, &G, &B);
	rgb[0] = (u8)(R * 255.0f + 0.5f);
	rgb[1] = (u8)(G * 255.0f + 0.5f);
	rgb[2] = (u8)(B * 255.0f + 0.5f);
}

void recolor_s10(const Recolor *r, GXColorS10 *c)
{
	u8 rgb[3];
	rgb[0] = c->r < 0 ? 0 : (c->r > 255 ? 255 : c->r);
	rgb[1] = c->g < 0 ? 0 : (c->g > 255 ? 255 : c->g);
	rgb[2] = c->b < 0 ? 0 : (c->b > 255 ? 255 : c->b);
	recolor_rgb(r, rgb);
	c->r = rgb[0]; c->g = rgb[1]; c->b = rgb[2];
}

void recolor_rgba8(const Recolor *r, u8 *tex, u16 w, u16 h)
{
	u32 blocks = ((w + 3) / 4) * ((h + 3) / 4), b;
	int i;
	if (!r->active)
		return;
	for (b = 0; b < blocks; b++) {
		u8 *ar = tex + 64 * b, *gb = ar + 32;
		for (i = 0; i < 16; i++) {
			u8 rgb[3] = {ar[2 * i + 1], gb[2 * i], gb[2 * i + 1]};
			recolor_rgb(r, rgb);
			ar[2 * i + 1] = rgb[0];
			gb[2 * i] = rgb[1];
			gb[2 * i + 1] = rgb[2];
		}
	}
	DCFlushRange(tex, blocks * 64);
}
