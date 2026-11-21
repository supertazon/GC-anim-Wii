/*
 * textimg.c - custom "GAMECUBE" logotype from a PNG/BMP/JPG/TGA image
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include "textimg.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_JPEG
#define STBI_ONLY_TGA
#define STBI_NO_SIMD
#define STBI_NO_THREAD_LOCALS
#include "stb_image.h"

static u8 *read_file(const char *path, int *len)
{
	FILE *f = fopen(path, "rb");
	long n;
	u8 *b;
	if (!f)
		return NULL;
	fseek(f, 0, SEEK_END);
	n = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (n <= 0 || n > 16 * 1024 * 1024) {
		fclose(f);
		return NULL;
	}
	b = malloc(n);
	if (b && fread(b, 1, n, f) != (size_t)n) {
		free(b);
		b = NULL;
	}
	fclose(f);
	*len = (int)n;
	return b;
}

u8 *textimg_load_i8(const char *path, u16 w, u16 h)
{
	int len = 0, iw, ih, ch, x, y;
	u8 *file = read_file(path, &len), *rgba, *lin, *tex;
	float scale, ox, oy;

	if (!file)
		return NULL;
	rgba = stbi_load_from_memory(file, len, &iw, &ih, &ch, 4);
	free(file);
	if (!rgba)
		return NULL;

	/* luminance x alpha, then crop to the drawn area so margins don't shrink the text */
	{
		float *lum = malloc(sizeof(float) * iw * ih);
		int x0 = iw, y0 = ih, x1 = -1, y1 = -1, cw, chh;
		if (!lum) {
			stbi_image_free(rgba);
			return NULL;
		}
		for (y = 0; y < ih; y++)
			for (x = 0; x < iw; x++) {
				const u8 *p = rgba + 4 * (y * iw + x);
				float l = (0.299f * p[0] + 0.587f * p[1] + 0.114f * p[2]) * p[3] / 255.0f;
				lum[y * iw + x] = l;
				if (l > 8.0f) {
					if (x < x0) x0 = x;
					if (x > x1) x1 = x;
					if (y < y0) y0 = y;
					if (y > y1) y1 = y;
				}
			}
		if (x1 < 0 || (iw == w && ih == h)) {
			/* nothing drawn, or already the texture's size (e.g. trevor.cafe/logo): use it 1:1 */
			x0 = 0; y0 = 0; x1 = iw - 1; y1 = ih - 1;
		}
		cw = x1 - x0 + 1;
		chh = y1 - y0 + 1;

		/* fit the drawn area inside w x h, centred, area-averaged */
		lin = calloc(w * h, 1);
		if (!lin) {
			free(lum);
			stbi_image_free(rgba);
			return NULL;
		}
		scale = (float)cw / w > (float)chh / h ? (float)cw / w : (float)chh / h;
		ox = (w - cw / scale) * 0.5f;
		oy = (h - chh / scale) * 0.5f;
		for (y = 0; y < h; y++)
			for (x = 0; x < w; x++) {
				float sx0 = (x - ox) * scale, sy0 = (y - oy) * scale;
				float sx1 = sx0 + scale, sy1 = sy0 + scale, acc = 0.0f, n = 0.0f;
				int ix, iy;
				for (iy = (int)floorf(sy0); iy < (int)ceilf(sy1); iy++)
					for (ix = (int)floorf(sx0); ix < (int)ceilf(sx1); ix++) {
						if (ix < 0 || iy < 0 || ix >= cw || iy >= chh)
							continue;
						acc += lum[(iy + y0) * iw + ix + x0];
						n += 1.0f;
					}
				lin[y * w + x] = n > 0.0f ? (u8)(acc / n + 0.5f) : 0;
			}
		free(lum);
	}
	stbi_image_free(rgba);

	/* I8: 8x4 texel blocks */
	tex = memalign(32, ((w + 7) / 8) * ((h + 3) / 4) * 32);
	if (tex) {
		u8 *o = tex;
		int bx, by, j, i;
		for (by = 0; by < h; by += 4)
			for (bx = 0; bx < w; bx += 8)
				for (j = 0; j < 4; j++)
					for (i = 0; i < 8; i++)
						*o++ = (by + j < h && bx + i < w) ? lin[(by + j) * w + bx + i] : 0;
		DCFlushRange(tex, o - tex);
	}
	free(lin);
	return tex;
}
