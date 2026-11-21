/*
 * config.c - user settings (optional gc-anim.ini on SD/USB)
 *
 * Example:
 *   [general]
 *   intro = ipl          ; ipl | promo (the alternative intro of Nintendo's promo videos)
 *   skip = 1             ; A or Start skips the animation
 *   camera = 1           ; C-stick camera
 *   force_4_3 = 0
 *   volume = 199
 *   delay = 0            ; ms of black screen before the animation starts (0-10000)
 *   [colors]
 *   color = 255,60,60    ; theme colour (or use hue/saturation/brightness), or random
 *   hue = 0
 *   saturation = 100
 *   brightness = 100
 *   cube = 212,175,55    ; small cube (or "tiles": follows the tile it is about to lay)
 *   tiles = rainbow      ; one colour, a list of up to 16, or rainbow
 *   tiles_mode = cycle   ; cycle | spread
 *   trail = 4            ; new tiles start as the cube colour, blend over N tiles
 *   logo = 113,21,33
 *   drop = 113,21,33     ; cube colour at the end of its final drop
 *   drop_blend = fall    ; fall | jump | off
 *   glass = 113,21,33
 *   glass_opacity = 100
 *   background = 0,0,0   ; also the final fade
 *   text = 255,255,255
 *   [text]
 *   letters = WIIBREW    ; replaces NINTENDO (A-Z, spaces; up to 16)
 *   letters_spacing = natural   ; natural | fill
 *   image = logo.png     ; in the app folder, or a full path
 *   [smb]
 *   smb = 1-1            ; 1-1 | full: World 1-1 only, or the whole game
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "config.h"

void config_defaults(Config *c)
{
	memset(c, 0, sizeof(*c));
	c->intro = INTRO_IPL;
	c->allow_skip = 0;
	c->camera = 1;
	c->force_43 = 0;
	c->volume = 199;
	c->start_delay = 0;
	c->hue = 0;
	c->saturation = 100;
	c->brightness = 100;
	c->glass_opacity = 100;
	c->drop_blend = DROP_FALL;
}

static char *trim(char *s)
{
	char *e;
	while (isspace((unsigned char)*s))
		s++;
	e = s + strlen(s);
	while (e > s && isspace((unsigned char)e[-1]))
		*--e = 0;
	return s;
}

static int parse_rgb(const char *v, u8 out[3])
{
	int r, g, b;
	if (v[0] == '#' && strlen(v) >= 7) {
		unsigned x = (unsigned)strtoul(v + 1, NULL, 16);
		out[0] = x >> 16; out[1] = x >> 8; out[2] = x;
		return 1;
	}
	if (sscanf(v, "%d , %d , %d", &r, &g, &b) == 3) {
		out[0] = (u8)(r < 0 ? 0 : r > 255 ? 255 : r);
		out[1] = (u8)(g < 0 ? 0 : g > 255 ? 255 : g);
		out[2] = (u8)(b < 0 ? 0 : b > 255 ? 255 : b);
		return 1;
	}
	return 0;
}

/* "r,g,b r,g,b #rrggbb ..." -> number of colours read */
static int parse_rgb_list(const char *v, u8 out[][3], int max)
{
	int n = 0, k = 0, num[3];
	while (*v && n < max) {
		if (*v == '#') {
			char *e;
			unsigned x = (unsigned)strtoul(v + 1, &e, 16);
			if (e - v >= 7) {
				out[n][0] = x >> 16; out[n][1] = x >> 8; out[n][2] = x;
				n++;
			}
			k = 0;
			v = e;
		} else if (isdigit((unsigned char)*v)) {
			char *e;
			long x = strtol(v, &e, 10);
			num[k++] = x < 0 ? 0 : x > 255 ? 255 : (int)x;
			if (k == 3) {
				out[n][0] = num[0]; out[n][1] = num[1]; out[n][2] = num[2];
				n++;
				k = 0;
			}
			v = e;
		} else {
			v++;
		}
	}
	return n;
}

int config_load(Config *c, const char *path)
{
	FILE *f = fopen(path, "r");
	char line[512];

	if (!f)
		return -1;
	while (fgets(line, sizeof(line), f)) {
		char *s = line, *eq, *k, *v, *cm;
		if ((u8)s[0] == 0xEF && (u8)s[1] == 0xBB && (u8)s[2] == 0xBF)
			s += 3;                    /* UTF-8 BOM (Windows Notepad) */
		if ((cm = strchr(s, ';')) != NULL)
			*cm = 0;
		s = trim(s);
		if (!*s || *s == '[' || *s == '#')
			continue;
		eq = strchr(s, '=');
		if (!eq)
			continue;
		*eq = 0;
		k = trim(s);
		v = trim(eq + 1);
		if (!strcasecmp(k, "intro"))
			c->intro = !strcasecmp(v, "promo") ? INTRO_PROMO : INTRO_IPL;
		else if (!strcasecmp(k, "skip"))
			c->allow_skip = atoi(v) != 0;
		else if (!strcasecmp(k, "camera"))
			c->camera = atoi(v) != 0;
		else if (!strcasecmp(k, "force_4_3"))
			c->force_43 = atoi(v) != 0;
		else if (!strcasecmp(k, "delay"))
			c->start_delay = atoi(v) < 0 ? 0 : atoi(v) > 10000 ? 10000 : atoi(v);
		else if (!strcasecmp(k, "volume"))
			c->volume = atoi(v) < 0 ? 0 : atoi(v) > 255 ? 255 : atoi(v);
		else if (!strcasecmp(k, "color") || !strcasecmp(k, "colour")) {
			c->color_random = !strcasecmp(v, "random");
			c->have_color = !c->color_random && parse_rgb(v, c->color);
		}
		else if (!strcasecmp(k, "hue"))
			c->hue = atoi(v);
		else if (!strcasecmp(k, "saturation"))
			c->saturation = atoi(v);
		else if (!strcasecmp(k, "brightness"))
			c->brightness = atoi(v);
		else if (!strcasecmp(k, "cube")) {
			c->cube_follow = !strcasecmp(v, "tiles") || !strcasecmp(v, "follow");
			c->have_cube = c->cube_follow || parse_rgb(v, c->cube);
		} else if (!strcasecmp(k, "tiles")) {
			c->tiles_rainbow = !strcasecmp(v, "rainbow");
			c->ntiles = c->tiles_rainbow ? 0 : parse_rgb_list(v, c->tiles, CFG_MAX_TILE_COLORS);
		} else if (!strcasecmp(k, "tiles_mode"))
			c->tiles_spread = !strcasecmp(v, "spread");
		else if (!strcasecmp(k, "trail"))
			c->trail = atoi(v) < 0 ? 0 : atoi(v) > 16 ? 16 : atoi(v);
		else if (!strcasecmp(k, "logo"))
			c->have_logo = parse_rgb(v, c->logo);
		else if (!strcasecmp(k, "drop"))
			c->have_drop = parse_rgb(v, c->drop);
		else if (!strcasecmp(k, "drop_blend"))
			c->drop_blend = !strcasecmp(v, "jump") ? DROP_JUMP : !strcasecmp(v, "off") ? DROP_OFF : DROP_FALL;
		else if (!strcasecmp(k, "glass"))
			c->have_glass = parse_rgb(v, c->glass);
		else if (!strcasecmp(k, "glass_opacity"))
			c->glass_opacity = atoi(v) < 0 ? 0 : atoi(v) > 400 ? 400 : atoi(v);
		else if (!strcasecmp(k, "background") || !strcasecmp(k, "bg"))
			c->have_background = parse_rgb(v, c->background);
		else if (!strcasecmp(k, "text"))
			c->have_text = parse_rgb(v, c->text);
		else if (!strcasecmp(k, "letters")) {
			int n = 0;
			const char *q;
			for (q = v; *q && n < 16; q++) {
				char ch = (char)toupper((unsigned char)*q);
				if ((ch >= 'A' && ch <= 'Z') || ch == ' ')
					c->letters[n++] = ch;
			}
			c->letters[n] = 0;
		} else if (!strcasecmp(k, "letters_spacing"))
			c->letters_fill = !strcasecmp(v, "fill");
		else if (!strcasecmp(k, "smb"))
			c->smb_full = !strcasecmp(v, "full");
		else if (!strcasecmp(k, "image")) {
			strncpy(c->text_image, v, sizeof(c->text_image) - 1);
			c->text_image[sizeof(c->text_image) - 1] = 0;
		}
	}
	fclose(f);
	return 0;
}

/* ---- color = random --------------------------------------------------- */

static u32 rng_state;
static u32 rng(void)
{
	/* (splitmix32) */
	u32 z = (rng_state += 0x9e3779b9u);
	z = (z ^ (z >> 16)) * 0x85ebca6bu;
	z = (z ^ (z >> 13)) * 0xc2b2ae35u;
	return z ^ (z >> 16);
}
static int   rnd(int lo, int hi) { return lo + (int)(rng() % (u32)(hi - lo + 1)); }
static float rndf(float lo, float hi) { return lo + (hi - lo) * (float)(rng() >> 8) / 16777216.0f; }

static void hsv_rgb(float h, float s, float v, u8 out[3])
{
	float c = v * s, x, m = v - c, r, g, b;
	int i;
	h = fmodf(h, 360.0f);
	if (h < 0) h += 360.0f;
	i = (int)(h / 60.0f);
	x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
	switch (i) {
	case 0:  r = c; g = x; b = 0; break;
	case 1:  r = x; g = c; b = 0; break;
	case 2:  r = 0; g = c; b = x; break;
	case 3:  r = 0; g = x; b = c; break;
	case 4:  r = x; g = 0; b = c; break;
	default: r = c; g = 0; b = x; break;
	}
	out[0] = (u8)((r + m) * 255.0f + 0.5f);
	out[1] = (u8)((g + m) * 255.0f + 0.5f);
	out[2] = (u8)((b + m) * 255.0f + 0.5f);
}

/* a lively colour: never grey, never too dark */
typedef struct { float h, s, v; } Hsv;
static Hsv rnd_hsv(float h) { Hsv c = {h, rndf(0.55f, 1.0f), rndf(0.70f, 1.0f)}; return c; }

void config_randomize(Config *c, u32 seed)
{
	enum { SOLID, GRADIENT, SPECTRUM, MOTIF };
	Hsv a, b, pal[3];
	int scheme, n, i, k, ndist;

	rng_state = seed;
	rng(); rng();
	a = rnd_hsv(rndf(0.0f, 360.0f));
	c->have_cube = c->cube_follow = 0;
	c->ntiles = 0;
	c->tiles_rainbow = 0;
	c->tiles_spread = 0;
	c->trail = 0;
	c->have_color = 1;
	hsv_rgb(a.h, a.s, a.v, c->color);

	k = rnd(0, 99);
	scheme = k < 25 ? SOLID : k < 55 ? GRADIENT : k < 70 ? SPECTRUM : MOTIF;
	switch (scheme) {
	case SOLID:
		/* one colour; sometimes a contrasting small cube */
		if (rnd(0, 2) == 0) {
			b = rnd_hsv(a.h + rndf(120.0f, 240.0f));
			c->have_cube = 1;
			hsv_rgb(b.h, b.s, b.v, c->cube);
		}
		return;
	case GRADIENT:
		/* two colours 60-180 degrees apart, in 4 to 16 bands along the path */
		b = rnd_hsv(a.h + (rnd(0, 1) ? 1.0f : -1.0f) * rndf(60.0f, 180.0f));
		n = rnd(4, CFG_MAX_TILE_COLORS);
		for (i = 0; i < n; i++) {
			float t = (float)i / (float)(n - 1);
			hsv_rgb(a.h + (b.h - a.h) * t, a.s + (b.s - a.s) * t, a.v + (b.v - a.v) * t, c->tiles[i]);
		}
		c->ntiles = n;
		c->tiles_spread = 1;
		break;
	case SPECTRUM:
		/* the colour wheel from a random hue, either way round, all or half of it */
		{
			float span = (rnd(0, 1) ? 360.0f : 180.0f) * (rnd(0, 1) ? 1.0f : -1.0f);
			n = CFG_MAX_TILE_COLORS;
			for (i = 0; i < n; i++)
				hsv_rgb(a.h + span * (float)i / (float)n, 0.75f, 0.9f, c->tiles[i]);
		}
		c->ntiles = n;
		c->tiles_spread = 1;
		break;
	default:
		/* a repeating motif: 2 to 6 tiles, from 2 or 3 colours (complementary or triadic) */
		ndist = rnd(2, 3);
		pal[0] = a;
		for (i = 1; i < ndist; i++)
			pal[i] = rnd_hsv(a.h + (360.0f / (float)ndist) * (float)i + rndf(-20.0f, 20.0f));
		n = rnd(2, 6);
		for (i = 0; i < n; i++) {
			Hsv p = pal[i < ndist ? i : rnd(0, ndist - 1)];
			hsv_rgb(p.h, p.s, p.v, c->tiles[i]);
		}
		c->ntiles = n;
		break;
	}
	/* the small cube: the theme colour, or it takes each tile's colour */
	if (rnd(0, 1)) {
		c->have_cube = 1;
		c->cube_follow = 1;
	}
	c->trail = rnd(0, 2) ? 0 : rnd(1, 4);
}
