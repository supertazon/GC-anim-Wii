/*
 * smb_palette.c - the NES's 64 colours in RGB, decoded from the PPU's own NTSC
 * signal (the levels nes_luma uses in smb_render.c): each colour is a square wave
 * between two voltages, 12 phases of the colour subcarrier per cycle; its average
 * gives the brightness, its phase and swing the hue and saturation (YIQ -> RGB).
 * Pure C, also used on PC (previews).
 */
#include <math.h>
#include "smb_palette.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define HUE_DEG   105.0f     /* phase alignment: colour $x6 red, $x2 blue (as on a NTSC TV) */
#define SAT       1.5f       /* colour gain (the two fit a reference 2C02 palette within
                                 about 1 level per channel on average) */

static float clamp01(float v)
{
	return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
}

void smb_palette_rgb(uint8_t rgb[64][3])
{
	static const float lo_v[4] = {0.350f, 0.518f, 0.962f, 1.550f};
	static const float hi_v[4] = {1.094f, 1.506f, 1.962f, 1.962f};
	const float black = 0.518f, white = 1.962f;
	int c, p, k;
	for (c = 0; c < 64; c++) {
		int hue = c & 15, lv = (c >> 4) & 3;
		float lo = lo_v[lv], hi = hi_v[lv], y = 0.0f, i = 0.0f, q = 0.0f, out[3];
		if (hue == 0) lo = hi;
		if (hue == 13) hi = lo;
		if (hue >= 14) lo = hi = black;
		for (p = 0; p < 12; p++) {
			float s = ((((hue + p) % 12) < 6) ? hi : lo);
			float v = (s - black) / (white - black);
			float a = (float)M_PI * ((float)p + 0.5f) / 6.0f + HUE_DEG * (float)M_PI / 180.0f;
			y += v;
			i += v * cosf(a);
			q += v * sinf(a);
		}
		y /= 12.0f;
		i = i / 12.0f * SAT;
		q = q / 12.0f * SAT;
		out[0] = y + 0.946882f * i + 0.623557f * q;
		out[1] = y - 0.274788f * i - 0.635691f * q;
		out[2] = y - 1.108545f * i + 1.709007f * q;
		for (k = 0; k < 3; k++)
			rgb[c][k] = (uint8_t)(clamp01(out[k]) * 255.0f + 0.5f);
	}
}
