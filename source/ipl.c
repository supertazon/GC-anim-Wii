/*
 * ipl.c - loading the GameCube IPL dump and extracting the boot animation assets
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "ipl.h"
#include "yay0.h"
#include "descrambler.h"

static inline u32 rd32(const u8 *p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }

int ipl_load_file(const char *path, u8 **out, u32 *size)
{
	FILE *f = fopen(path, "rb");
	long len;
	u8 *buf;

	if (!f)
		return -1;
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (len < 0x100000 || len > 0x400000) {       /* IPL dumps are 2 MB */
		fclose(f);
		return -2;
	}
	buf = memalign(32, (len + 31) & ~31);
	if (!buf) {
		fclose(f);
		return -3;
	}
	if (fread(buf, 1, len, f) != (size_t)len) {
		free(buf);
		fclose(f);
		return -4;
	}
	fclose(f);
	*out = buf;
	*size = (u32)len;
	return 0;
}

static int count_magic(const u8 *d, u32 size, const char *m, int max)
{
	u32 i;
	int n = 0;
	for (i = 0; i + 4 <= size && n < max; i += 4)
		if (d[i] == m[0] && d[i + 1] == m[1] && d[i + 2] == m[2] && d[i + 3] == m[3])
			n++;
	return n;
}

int ipl_prepare(u8 *ipl, u32 size)
{
	/* a descrambled IPL contains many "Yay0" streams (aligned to 32 bytes) */
	if (count_magic(ipl, size, "Yay0", 4) < 4)
		Descrambler(ipl + 0x100, size - 0x100);   /* the scrambled area starts at 0x100 */
	return count_magic(ipl, size, "Yay0", 4) >= 4;
}

/* ---- resource directory: u32 count, u32 offsets[count] (relative to the directory) ---- */
static int find_table(const u8 *d, u32 size, u32 *pos, u32 *count)
{
	u32 p;
	for (p = 0; p + 8 < size; p += 4) {
		u32 n = rd32(d + p), o0, i;
		if (n < 40 || n > 400 || p + 4 + 4 * n > size)
			continue;
		o0 = rd32(d + p + 4);
		if (o0 < 4 + 4 * n || o0 >= 4 + 4 * n + 0x400 || p + o0 + 4 > size)
			continue;
		if (memcmp(d + p + o0, "GLH0", 4) && memcmp(d + p + o0, "STH0", 4))
			continue;
		for (i = 1; i < n; i++) {
			u32 o = rd32(d + p + 4 + 4 * i);
			if (o <= rd32(d + p + 4 * i) || p + o >= size)
				break;
		}
		if (i == n) {
			*pos = p;
			*count = n;
			return 0;
		}
	}
	return -1;
}

/* get a copy of resource i (decompressed if needed) */
static int get_res(const u8 *d, u32 size, u32 tab, u32 n, u32 i, IplRes *r)
{
	const u8 *p = d + tab + rd32(d + tab + 4 + 4 * i);
	u32 len = yay0_size(p);

	if (len) {
		r->data = memalign(32, (len + 31) & ~31);
		if (!r->data)
			return -1;
		yay0_decompress(p, r->data);
	} else {
		u32 end = (i + 1 < n) ? tab + rd32(d + tab + 8 + 4 * i) : size;
		len = (u32)(d + end - p);
		if (!memcmp(p, "J3D1", 4) && rd32(p + 8) <= len)
			len = rd32(p + 8);
		r->data = memalign(32, (len + 31) & ~31);
		if (!r->data)
			return -1;
		memcpy(r->data, p, len);
	}
	r->size = len;
	return 0;
}

static int contains(const IplRes *r, const char *s, u32 slen)
{
	u32 i;
	if (r->size < slen)
		return 0;
	for (i = 0; i + slen <= r->size; i++)
		if (r->data[i] == (u8)s[0] && !memcmp(r->data + i, s, slen))
			return 1;
	return 0;
}

static void take(IplRes *dst, IplRes *src)
{
	if (!dst->data) {
		*dst = *src;
		src->data = NULL;
	}
}

int ipl_extract_assets(const u8 *ipl, u32 size, IplAssets *a)
{
	static const u8 tile_hdr[8] = {0x01, 0x00, 0x00, 0x40, 0x00, 0x40, 0x02, 0x02};
	u32 tab, n, i;

	memset(a, 0, sizeof(*a));
	if (find_table(ipl, size, &tab, &n))
		return -1;

	for (i = 0; i < n; i++) {
		IplRes r = {0};
		const u8 *p = ipl + tab + rd32(ipl + tab + 4 + 4 * i);
		int yay = yay0_size(p) != 0;

		/* only Yay0 streams and J3D/IPK files are interesting */
		if (!yay && memcmp(p, "J3D1", 4) && memcmp(p, "IPK1", 4))
			continue;
		if (get_res(ipl, size, tab, n, i, &r))
			return -2;

		if (!memcmp(r.data, "J3D1bmd1", 8)) {
			if (contains(&r, "boot_demo_s_cube", 17))
				take(&a->s_cube, &r);
			else if (contains(&r, "boot_demo_base_cube", 20))   /* incl. the NUL: not "..._cube1" */
				take(&a->big_cube, &r);
			else if (contains(&r, "boot_demo_logo_mark", 19))
				take(&a->logo, &r);
			else if (contains(&r, "boot_demo_mat28", 15))
				take(&a->letters, &r);
		} else if (!memcmp(r.data, "J3D1bck1", 8)) {
			take(&a->letters_bck, &r);
		} else if (!memcmp(r.data, "IPK1", 4)) {
			take(&a->letters_ipk, &r);
		} else if (r.size == 0x1020 && !memcmp(r.data, tile_hdr, 8)) {
			take(&a->tile_tex, &r);
		}
		if (r.data)
			free(r.data);
	}

	if (!a->letters.data || !a->letters_bck.data || !a->letters_ipk.data || !a->big_cube.data ||
	    !a->s_cube.data || !a->logo.data || !a->tile_tex.data) {
		ipl_free_assets(a);
		return -3;
	}
	for (i = 0; i < sizeof(*a) / sizeof(IplRes); i++) {
		IplRes *r = &((IplRes *)a)[i];
		DCFlushRange(r->data, (r->size + 31) & ~31);
	}
	return 0;
}

void ipl_free_assets(IplAssets *a)
{
	u32 i;
	for (i = 0; i < sizeof(*a) / sizeof(IplRes); i++) {
		IplRes *r = &((IplRes *)a)[i];
		if (r->data)
			free(r->data);
		r->data = NULL;
		r->size = 0;
	}
}
