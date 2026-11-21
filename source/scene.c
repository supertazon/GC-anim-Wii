/*
 * scene.c - drawing one frame of the boot animation
 *
 * Draw order and GX state follow the IPL (FUN_8130d688 for the scene,
 * FUN_81310a94 / FUN_81301bd8 / FUN_81310740 for the tiles, FUN_81301f94
 * for the final fade, FUN_81303354 for the light).
 */
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "scene.h"
#include "j3d.h"
#include "anim.h"
#include "gcmath.h"
#include "recolor.h"
#include "textimg.h"
#include "letters.h"

typedef struct {
	J3DModel model;
	J3DInstance inst;
	GXColorS10 (*tev)[4];
	u8 (*tev_set)[4];
} SceneModel;

static SceneModel letters, big, cube, logo;
static BckAnim bck;
static IpkAnim ipk;
static Mtx *letters_local;
static GXColor *letters_col;
static GXTexObj tile_tex;
static const u8 tile_rgb[3] = {100, 80, 190};   /* IPL tile colour */
static u8 *custom_text;
/* the logo model's RGBA8 textures as loaded (before any recolouring), to recolour
   them again (scene_mario_colours) */
#define MAX_LOGO_TEX 8
static u8 *logo_tex_orig[MAX_LOGO_TEX];

/* custom 3D text (gc-anim.ini [text] letters) */
#define MAXL 16
static struct {
	int n;                     /* 0 = original NINTENDO */
	char ch[MAXL];
	int fill;
	Mtx m[MAXL];
	GXColor front[MAXL], side[MAXL];
	int front_mat[MAXL], side_mat[MAXL];
} txt;

/* colour customisation (gc-anim.ini [colors]) */
#define NT 16                  /* tiles laid along the path */
static struct {
	Config cfg;
	int active;                /* any colour setting given            */
	u8 tile_final[NT][3];      /* final colour of the k-th tile laid  */
	u8 tile_start[3];          /* colour of a freshly laid tile       */
	u8 tile_tgt[NT][3];        /* target colours (cube = tiles)       */
	u8 cube_tgt[3], drop_tgt[3];
	GXColorS10 cube_orig[2];
	int have_cube_orig[2];
	GXColor bg, text;
	int text_custom;
	int buttons;               /* Mario colours: the logo's two "buttons" (scene_mario_colours) */
	u8 button_rgb[2][3];
} col;

static inline u32 rd32(const u8 *p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }
static inline u16 rd16(const u8 *p) { return (u16)(p[0] << 8 | p[1]); }

static int model_init(SceneModel *s, IplRes *r)
{
	int n;
	memset(s, 0, sizeof(*s));
	if (j3d_load(&s->model, r->data, r->size))
		return -1;
	s->inst.model = &s->model;
	s->inst.alpha = 255;
	s->inst.jmtx = calloc(s->model.njoints ? s->model.njoints : 1, sizeof(Mtx));
	n = s->model.nmats ? s->model.nmats : 1;
	s->tev = calloc(n, sizeof(*s->tev));
	s->tev_set = calloc(n, sizeof(*s->tev_set));
	s->inst.tevcolor = (const GXColorS10 (*)[4])s->tev;
	s->inst.tevcolor_set = (const u8 (*)[4])s->tev_set;
	return s->inst.jmtx && s->tev && s->tev_set ? 0 : -1;
}

static void model_free(SceneModel *s)
{
	free(s->inst.jmtx);
	free(s->tev);
	free(s->tev_set);
	j3d_free(&s->model);
}

/* recolour TEV colours k of the given materials */
static void recolor_tev(SceneModel *s, const Recolor *rc, int mat0, int mat1, int kmask)
{
	int mi, k;
	for (mi = mat0; mi <= mat1 && mi < s->model.nmats; mi++)
		for (k = 0; k < 3; k++) {
			GXColorS10 c;
			if (!(kmask & (1 << k)) || !j3d_get_tevcolor(&s->model, mi, k, &c))
				continue;
			recolor_s10(rc, &c);
			s->tev[mi][k] = c;
			s->tev_set[mi][k] = 1;
		}
}


static void lerp_rgb(const u8 a[3], const u8 b[3], float t, u8 out[3])
{
	int i;
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;
	for (i = 0; i < 3; i++)
		out[i] = (u8)((float)a[i] + ((float)b[i] - (float)a[i]) * t + 0.5f);
}

/* recolour the IPL tile colour for a target colour (NULL: global adjustments only) */
static void tile_color_for(const u8 *target, u8 out[3])
{
	Recolor rc;
	recolor_setup_target(&rc, target, &col.cfg);
	out[0] = tile_rgb[0]; out[1] = tile_rgb[1]; out[2] = tile_rgb[2];
	recolor_rgb(&rc, out);
}

/* the target colour of the k-th tile, or NULL for the default */
static const u8 *tile_target(const Config *c, int k, u8 tmp[3])
{
	if (c->tiles_rainbow) {
		recolor_hsv(18.75f * (float)k, 0.75f, 0.85f, tmp);
		return tmp;
	}
	if (c->ntiles > 0) {
		int i = c->tiles_spread ? (k * c->ntiles) / NT : k % c->ntiles;
		if (i >= c->ntiles)
			i = c->ntiles - 1;
		return c->tiles[i];
	}
	return c->have_color ? c->color : NULL;
}

static void recolor_model(SceneModel *m, const u8 *target, int mat0, int mat1, int kmask, int textures)
{
	Recolor rc;
	int i;
	recolor_setup_target(&rc, target, &col.cfg);
	if (!rc.active)
		return;
	recolor_tev(m, &rc, mat0, mat1, kmask);
	if (textures)
		for (i = 0; i < m->model.ntex; i++) {
			const u8 *h = j3d_tex_header(&m->model, i);
			if (h[0] == GX_TF_RGBA8)
				recolor_rgba8(&rc, (u8 *)h + rd32(h + 0x1C), rd16(h + 2), rd16(h + 4));
		}
}

static void colors_init(const Config *c)
{
	static const u8 indigo[3] = {95, 80, 190};
	const u8 *base = c->have_color ? c->color : NULL;
	const u8 *logo_t = base;
	u8 tmp[3], logo_buf[3];
	int k;

	col.cfg = *c;
	col.active = c->have_color || c->hue || c->saturation != 100 || c->brightness != 100 ||
	             c->have_cube || c->ntiles || c->tiles_rainbow || c->have_logo || c->have_drop ||
	             c->have_glass;

	/* logo: given, or the colour that would follow the last tile */
	if (c->have_logo)
		logo_t = c->logo;
	else if (c->tiles_rainbow) {
		recolor_hsv(18.75f * NT, 0.75f, 0.85f, logo_buf);
		logo_t = logo_buf;
	} else if (c->ntiles > 0) {
		logo_t = c->tiles[c->tiles_spread ? c->ntiles - 1 : NT % c->ntiles];
	}

	for (k = 0; k < NT; k++) {
		const u8 *t = tile_target(c, k, tmp);
		if (t) {
			col.tile_tgt[k][0] = t[0]; col.tile_tgt[k][1] = t[1]; col.tile_tgt[k][2] = t[2];
		} else {
			col.tile_tgt[k][0] = indigo[0]; col.tile_tgt[k][1] = indigo[1]; col.tile_tgt[k][2] = indigo[2];
		}
		tile_color_for(t, col.tile_final[k]);
	}

	if (c->have_cube && !c->cube_follow)
		memcpy(col.cube_tgt, c->cube, 3);
	else
		memcpy(col.cube_tgt, base ? base : indigo, 3);
	tile_color_for(col.cube_tgt, col.tile_start);
	if (c->have_drop)
		memcpy(col.drop_tgt, c->drop, 3);
	else
		memcpy(col.drop_tgt, logo_t ? logo_t : indigo, 3);
	for (k = 0; k < 2; k++)
		col.have_cube_orig[k] = j3d_get_tevcolor(&cube.model, 0, k, &col.cube_orig[k]);

	/* models recoloured once: logo (TEV colours + RGBA8 textures) and the glass cube */
	recolor_model(&logo, logo_t, 0, 2, 3, 1);
	recolor_model(&big, c->have_glass ? c->glass : logo_t, 0, 1, 1, 0);

	/* background and text */
	col.bg.r = c->have_background ? c->background[0] : 0;
	col.bg.g = c->have_background ? c->background[1] : 0;
	col.bg.b = c->have_background ? c->background[2] : 0;
	col.bg.a = 255;
	if (c->have_text) {
		col.text.r = c->text[0]; col.text.g = c->text[1]; col.text.b = c->text[2];
	} else {
		/* white on dark backgrounds, black on light ones */
		int l = (299 * col.bg.r + 587 * col.bg.g + 114 * col.bg.b) / 1000;
		col.text.r = col.text.g = col.text.b = l > 128 ? 0 : 255;
	}
	col.text.a = 255;
	col.text_custom = c->have_background || c->have_text;
	letters.inst.tex_color_from_ras = col.text_custom;
}

GXColor scene_background(void)
{
	return col.bg;
}

/* per frame: small cube colour (theme, follow the tiles, blend on the final drop) */
static void cube_color(const IntroFrame *f)
{
	u8 t[3];
	float e;
	Recolor rc;
	int k;

	if (!col.active)
		return;
	if (col.cfg.cube_follow) {
		/* the colour of the tile appearing under it, switching early in each roll */
		int b = f->nlaid > 0 ? f->nlaid - 1 : 0, a = b > 0 ? b - 1 : 0;
		if (b >= NT)
			b = NT - 1;
		if (a >= NT)
			a = NT - 1;
		lerp_rgb(col.tile_tgt[a], col.tile_tgt[b], 4.0f * f->roll_frac, t);
	} else {
		memcpy(t, col.cube_tgt, 3);
	}
	e = col.cfg.drop_blend == DROP_JUMP ? f->end_jump : col.cfg.drop_blend == DROP_FALL ? f->end_fall : 0.0f;
	lerp_rgb(t, col.drop_tgt, e, t);
	recolor_setup_target(&rc, t, &col.cfg);
	for (k = 0; k < 2; k++) {
		GXColorS10 c = col.cube_orig[k];
		if (!col.have_cube_orig[k])
			continue;
		recolor_s10(&rc, &c);
		cube.tev[0][k] = c;
		cube.tev_set[0][k] = 1;
	}
}

/* per tile: freshly laid tiles start in the cube colour and reach their own over "trail" tiles */
static void tile_color(const IntroFrame *f, const IntroTile *t, u8 out[3])
{
	int k = t->seq < NT ? t->seq : NT - 1;
	float a, e;
	const u8 *start = col.tile_start;

	if (col.cfg.trail <= 0) {
		memcpy(out, col.tile_final[k], 3);
		return;
	}
	if (col.cfg.cube_follow)
		start = col.tile_final[k];
	a = ((float)(f->nlaid - 1 - t->seq) + f->roll_frac) / (float)col.cfg.trail;
	e = col.cfg.drop_blend == DROP_JUMP ? f->end_jump : f->end_fall;
	if (a < e)
		a = e;
	lerp_rgb(start, col.tile_final[k], a, out);
}

/* ---- custom letters ---------------------------------------------------- */

/* the IPL's letter slots, left to right: joint 10-s, front shape 2s+1 (mat s), side shape 2s (mat 16-s) */
#define SLOT_JOINT(s) (10 - (s))

static int orig_front_shape(char c)
{
	switch (c) {
	case 'N': return 1;
	case 'I': return 3;
	case 'T': return 7;
	case 'E': return 9;
	case 'D': return 13;
	case 'O': return 15;
	}
	return -1;
}

static void letters_init(const Config *c)
{
	int n = (int)strlen(c->letters);
	while (n > 0 && c->letters[n - 1] == ' ')
		n--;
	txt.n = 0;
	if (n <= 0 || letters.model.njoints < 11 || letters.model.nshapes < 16 || letters.model.nmats < 17)
		return;
	memcpy(txt.ch, c->letters, n);
	txt.n = n;
	txt.fill = c->letters_fill;
	letters.inst.shape_hide = 0xFFFF;      /* shapes 0..15: the original letters */
}

static GXColor lerp_col(GXColor a, GXColor b, float t)
{
	GXColor r;
	r.r = (u8)(a.r + (b.r - a.r) * t + 0.5f);
	r.g = (u8)(a.g + (b.g - a.g) * t + 0.5f);
	r.b = (u8)(a.b + (b.b - a.b) * t + 0.5f);
	r.a = (u8)(a.a + (b.a - a.a) * t + 0.5f);
	return r;
}

/* per frame: matrix and colours of each custom letter */
static void letters_prepare(float frame)
{
	float tx[8], xc, span, pitch;
	int k, s, n = txt.n;
	float S[3], T[3];
	int R[3];

	for (s = 0; s < 8; s++) {
		bck_srt(&bck, SLOT_JOINT(s), frame, S, R, T);
		tx[s] = T[0];
	}
	xc = 0.5f * (tx[0] + tx[7]);
	span = tx[7] - tx[0];
	pitch = n > 1 ? span / (float)(n - 1) : 0.0f;
	if (!txt.fill && n <= 8)
		pitch = span / 7.0f;               /* NINTENDO's letter pitch */

	for (k = 0; k < n; k++) {
		float f = n > 1 ? (7.0f * (float)k) / (float)(n - 1) : 3.5f;
		int s0 = (int)f, s1;
		float w, S0[3], T0[3], S1[3], T1[3], x;
		int R0[3], R1[3], i, sn;
		Mtx L;

		if (s0 > 7) s0 = 7;
		s1 = s0 < 7 ? s0 + 1 : 7;
		w = f - (float)s0;
		/* the motion of the nearest original letters, blended for in-between positions */
		bck_srt(&bck, SLOT_JOINT(s0), frame, S0, R0, T0);
		bck_srt(&bck, SLOT_JOINT(s1), frame, S1, R1, T1);
		for (i = 0; i < 3; i++) {
			S[i] = S0[i] + (S1[i] - S0[i]) * w;
			R[i] = (int)((float)R0[i] + (float)(R1[i] - R0[i]) * w);
			T[i] = T0[i] + (T1[i] - T0[i]) * w;
		}
		x = n == 8 ? tx[k] : xc + ((float)k - 0.5f * (float)(n - 1)) * pitch;
		gm_euler(L, x, T[1], T[2], R[0], R[1], R[2]);
		if (S[0] != 1.0f || S[1] != 1.0f || S[2] != 1.0f)
			gm_scalecols(L, S[0], S[1], S[2]);
		gm_concat(letters.inst.jmtx[2], L, txt.m[k]);

		sn = w < 0.5f ? s0 : s1;
		txt.front_mat[k] = sn;
		txt.side_mat[k] = 16 - sn;
		txt.front[k] = lerp_col(letters_col[s0], letters_col[s1], w);
		txt.side[k] = lerp_col(letters_col[16 - s0], letters_col[16 - s1], w);
	}
}

static void draw_tris(const s16 *v, int ntri)
{
	int i;
	GX_Begin(GX_TRIANGLES, GX_VTXFMT2, (u16)(3 * ntri));
	for (i = 0; i < 3 * ntri; i++)
		GX_Position3s16(v[3 * i], v[3 * i + 1], v[3 * i + 2]);
	GX_End();
}

static void draw_generated(const s16 *v, int ntri, Mtx m)
{
	Mtx nrm;
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT2, GX_VA_POS, GX_POS_XYZ, GX_S16, 8);
	guMtxInvXpose(m, nrm);
	GX_LoadPosMtxImm(m, GX_PNMTX0);
	GX_LoadNrmMtxImm(nrm, GX_PNMTX0);
	draw_tris(v, ntri);
}

static void letters_draw(void)
{
	int k;
	for (k = 0; k < txt.n; k++) {
		char c = txt.ch[k];
		int os = orig_front_shape(c);
		if (c < 'A' || c > 'Z')
			continue;
		/* front and back faces */
		j3d_apply_material(&letters.inst, txt.front_mat[k]);
		GX_SetChanMatColor(GX_COLOR0A0, txt.front[k]);
		if (os >= 0)
			j3d_draw_shape_mtx(&letters.inst, os, txt.m[k]);
		else
			draw_generated(glyph_mesh[c - 'A'].cap, glyph_mesh[c - 'A'].ncap, txt.m[k]);
		/* sides */
		j3d_apply_material(&letters.inst, txt.side_mat[k]);
		GX_SetChanMatColor(GX_COLOR0A0, txt.side[k]);
		if (os >= 0)
			j3d_draw_shape_mtx(&letters.inst, os - 1, txt.m[k]);
		else
			draw_generated(glyph_mesh[c - 'A'].wall, glyph_mesh[c - 'A'].nwall, txt.m[k]);
	}
}

int scene_init(IplAssets *a, const Config *cfg)
{
	const u8 *t;

	if (model_init(&letters, &a->letters) || model_init(&big, &a->big_cube) ||
	    model_init(&cube, &a->s_cube) || model_init(&logo, &a->logo))
		return -1;
	if (bck_init(&bck, a->letters_bck.data, a->letters_bck.size) ||
	    ipk_init(&ipk, a->letters_ipk.data, a->letters_ipk.size))
		return -2;
	letters_local = calloc(letters.model.njoints ? letters.model.njoints : 1, sizeof(Mtx));
	letters_col = calloc(letters.model.nmats ? letters.model.nmats : 1, sizeof(GXColor));
	letters.inst.matcolor0 = letters_col;

	/* tile texture: BTI header + data (FUN_8130630c) */
	t = a->tile_tex.data;
	GX_InitTexObj(&tile_tex, (void *)(t + rd32(t + 0x1C)), rd16(t + 2), rd16(t + 4), t[0], t[6], t[7],
	              t[0x10] ? GX_TRUE : GX_FALSE);
	GX_InitTexObjLOD(&tile_tex, t[0x14], t[0x15], 0.125f * (float)(s8)t[0x16], 0.125f * (float)(s8)t[0x17],
	                 0.01f * (float)(s16)rd16(t + 0x1A), t[0x12], t[0x11], t[0x13]);

	{
		int i;
		for (i = 0; i < logo.model.ntex && i < MAX_LOGO_TEX; i++) {
			const u8 *h = j3d_tex_header(&logo.model, i);
			if (h[0] == GX_TF_RGBA8) {
				u32 n = (u32)rd16(h + 2) * rd16(h + 4) * 4;
				logo_tex_orig[i] = malloc(n);
				if (logo_tex_orig[i])
					memcpy(logo_tex_orig[i], h + rd32(h + 0x1C), n);
			}
		}
	}
	colors_init(cfg);
	letters_init(cfg);
	/* optional custom "GAMECUBE" logotype: texture 0 of the letters model */
	if (cfg->text_image[0] && letters.model.ntex > 0) {
		const u8 *h = j3d_tex_header(&letters.model, 0);
		custom_text = textimg_load_i8(cfg->text_image, rd16(h + 2), rd16(h + 4));
		if (custom_text)
			j3d_set_texture(&letters.model, 0, custom_text, rd16(h + 2), rd16(h + 4), GX_TF_I8);
	}
	return 0;
}

void scene_free(void)
{
	model_free(&letters);
	model_free(&big);
	model_free(&cube);
	model_free(&logo);
	free(letters_local);
	free(letters_col);
	free(custom_text);
	{
		int i;
		for (i = 0; i < MAX_LOGO_TEX; i++) {
			free(logo_tex_orig[i]);
			logo_tex_orig[i] = NULL;
		}
	}
}

/* the Mario colours for the rest of the animation (SMB finished in its original
   colours): red (NES $16) cube, logo, tiles, with the two middle cubes of the logo's
   bottom row (and their tiles) and the glass cube in gold ($27), Mario's buttons;
   background and text unchanged */
void scene_mario_colours(void)
{
	static const u8 red[3] = {178, 51, 34}, gold[3] = {231, 158, 38};
	/* the logo's left and right faces, against the tiles' flat colour (measured) */
	static const float face_shade[2] = {0.76f, 0.70f};
	Config m = col.cfg;
	int i, k;
	/* back to the textures as loaded: they are recoloured in place */
	for (i = 0; i < logo.model.ntex && i < MAX_LOGO_TEX; i++)
		if (logo_tex_orig[i]) {
			const u8 *h = j3d_tex_header(&logo.model, i);
			u32 n = (u32)rd16(h + 2) * rd16(h + 4) * 4;
			u8 *d = (u8 *)h + rd32(h + 0x1C);
			memcpy(d, logo_tex_orig[i], n);
		}
	m.have_color = 1; memcpy(m.color, red, 3);
	m.hue = 0; m.saturation = 100; m.brightness = 100;
	m.have_cube = 1; m.cube_follow = 0; memcpy(m.cube, red, 3);
	m.ntiles = NT; m.tiles_rainbow = 0; m.tiles_spread = 0;
	for (k = 0; k < NT; k++)
		memcpy(m.tiles[k], (k == 7 || k == 10) ? gold : red, 3);
	m.have_logo = 1; memcpy(m.logo, red, 3);
	m.have_drop = 1; memcpy(m.drop, red, 3);
	m.have_glass = 1; memcpy(m.glass, gold, 3);
	colors_init(&m);
	{
		u8 g[3];
		tile_color_for(gold, g);
		for (k = 0; k < 2; k++)
			for (i = 0; i < 3; i++)
				col.button_rgb[k][i] = (u8)(g[i] * face_shade[k]);
		col.buttons = 1;
	}
	for (i = 0; i < logo.model.ntex && i < MAX_LOGO_TEX; i++)
		if (logo_tex_orig[i]) {
			const u8 *h = j3d_tex_header(&logo.model, i);
			DCFlushRange((u8 *)h + rd32(h + 0x1C), (u32)rd16(h + 2) * rd16(h + 4) * 4);
		}
	GX_InvalidateTexAll();
}

int scene_letters_bounds(float b[4])
{
	if (letters.inst.alpha <= 0)
		return 0;
	return j3d_bounds(&letters.inst, b);
}

void scene_prepare(const IntroFrame *f)
{
	int j;

	/* letters: joint animation + colour animation (FUN_81304770) */
	for (j = 0; j < letters.model.njoints && j < bck.njoints; j++)
		bck_local(&bck, j, f->letters_frame, letters_local[j]);
	for (j = 0; j < letters.model.nmats; j++) {
		GXColor c = ipk_color(&ipk, j, f->letters_frame);
		if (col.text_custom) {
			/* the IPL's grey levels become levels between background and text colour */
			c.r = (u8)(col.bg.r + ((col.text.r - col.bg.r) * c.r) / 255);
			c.g = (u8)(col.bg.g + ((col.text.g - col.bg.g) * c.g) / 255);
			c.b = (u8)(col.bg.b + ((col.text.b - col.bg.b) * c.b) / 255);
		}
		letters_col[j] = c;
	}
	gm_copy((MtxP)f->view, letters.inst.base);
	letters.inst.alpha = f->letters_alpha;
	j3d_calc_local(&letters.inst, letters_local);
	if (txt.n)
		letters_prepare(f->letters_frame);

	gm_copy((MtxP)f->big, big.inst.base);
	big.inst.alpha = (f->big_alpha * col.cfg.glass_opacity) / 100;
	if (big.inst.alpha > 255)
		big.inst.alpha = 255;
	j3d_calc_rest(&big.inst);

	cube_color(f);
	gm_copy((MtxP)f->cube, cube.inst.base);
	cube.inst.alpha = f->cube_alpha;
	j3d_calc_rest(&cube.inst);

	gm_copy((MtxP)f->big, logo.inst.base);
	logo.inst.alpha = f->logo_alpha;
	j3d_calc_rest(&logo.inst);
}

/* FUN_81303354: white light at (400,500,200) in world space */
static void setup_light(const IntroFrame *f)
{
	GXLightObj lo;
	GXColor white = {255, 255, 255, 255};
	GX_InitLightPos(&lo, f->light[0], f->light[1], f->light[2]);
	GX_InitLightDir(&lo, f->view[0][3], f->view[1][3], f->view[2][3]);
	GX_InitLightColor(&lo, white);
	GX_LoadLightObj(&lo, GX_LIGHT0);
}

/* FUN_81310a94 / FUN_81301bd8(1,0,0) / FUN_81310528 / FUN_81310740; list/n/rgb: other
   tiles than the frame's, in the colours given (the logo's buttons) */
static void draw_tiles_list(const IntroFrame *f, const IntroTile *list, int n, const u8 (*rgb_over)[3])
{
	static const s16 tc[4][2] = {{0, 0}, {0, 0x4000}, {0x4000, 0x4000}, {0x4000, 0}};
	Mtx nrm;
	int i, v;

	GX_LoadPosMtxImm((MtxP)f->tiles, GX_PNMTX0);
	guMtxInvXpose((MtxP)f->tiles, nrm);
	GX_LoadNrmMtxImm(nrm, GX_PNMTX0);
	GX_SetCurrentMtx(GX_PNMTX0);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 4);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_S16, 13);
	GX_SetNumChans(1);
	GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
	GX_SetNumTexGens(1);
	GX_SetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_DTTIDENTITY);
	GX_SetNumTevStages(1);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	for (i = 1; i < 8; i++)
		GX_SetTevOrder(GX_TEVSTAGE0 + i, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLORNULL);
	GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
	GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetColorUpdate(GX_TRUE);
	GX_SetAlphaUpdate(GX_TRUE);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
	GX_SetCullMode(GX_CULL_NONE);

	for (i = 0; i < n; i++) {
		const IntroTile *t = &list[i];
		GXColor c;
		u8 rgb[3];
		if (rgb_over)
			memcpy(rgb, rgb_over[i], 3);
		else
			tile_color(f, t, rgb);   /* f->tile_rgb, customised */
		c.r = rgb[0]; c.g = rgb[1]; c.b = rgb[2]; c.a = t->alpha;
		s16 r27 = 54 << 4, r29 = 0;
		s16 r28 = (s16)(((t->x - 16) * 54) << 4), r31 = (s16)(((t->y - 16) * 54) << 4);
		s16 a0 = -(r28 + r29), a1 = -(r28 + r27), b0 = -(r31 + r29), b1 = -(r31 + r27);
		s16 p[4][3];

		if (t->face == 0) {
			p[0][0] = a0; p[0][1] = 0; p[0][2] = b0;
			p[1][0] = a1; p[1][1] = 0; p[1][2] = b0;
			p[2][0] = a1; p[2][1] = 0; p[2][2] = b1;
			p[3][0] = a0; p[3][1] = 0; p[3][2] = b1;
		} else if (t->face == 1) {
			p[0][0] = b0; p[0][1] = a0; p[0][2] = 0;
			p[1][0] = b0; p[1][1] = a1; p[1][2] = 0;
			p[2][0] = b1; p[2][1] = a1; p[2][2] = 0;
			p[3][0] = b1; p[3][1] = a0; p[3][2] = 0;
		} else {
			p[0][0] = 0; p[0][1] = b0; p[0][2] = a0;
			p[1][0] = 0; p[1][1] = b0; p[1][2] = a1;
			p[2][0] = 0; p[2][1] = b1; p[2][2] = a1;
			p[3][0] = 0; p[3][1] = b1; p[3][2] = a0;
		}
		GX_SetChanMatColor(GX_COLOR0A0, c);
		GX_LoadTexObj(&tile_tex, GX_TEXMAP0);
		GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
		for (v = 0; v < 4; v++) {
			GX_Position3s16(p[v][0], p[v][1], p[v][2]);
			GX_TexCoord2s16(tc[v][0], tc[v][1]);
		}
		GX_End();
	}
}

static void draw_tiles(const IntroFrame *f)
{
	draw_tiles_list(f, f->tile, f->ntiles, NULL);
}

/* the Mario colours' buttons: the middle cubes of the bottom row of the logo's left and
   right faces (face 1 x 18 y 17, face 2 x 17 y 18: the 8th and 11th tiles laid), drawn
   over the logo as tiles in its place, shaded as the logo's faces are */
static void draw_buttons(const IntroFrame *f)
{
	IntroTile b[2];
	memset(b, 0, sizeof(b));
	b[0].face = 1; b[0].x = 18; b[0].y = 17; b[0].seq = 7;
	b[1].face = 2; b[1].x = 17; b[1].y = 18; b[1].seq = 10;
	b[0].alpha = b[1].alpha = (u8)(f->logo_alpha > 255 ? 255 : f->logo_alpha);
	draw_tiles_list(f, b, 2, (const u8 (*)[3])col.button_rgb);
}

/* FUN_81301f94: black quad over the whole screen (background colour if customised) */
static void draw_fade(int alpha)
{
	static const s16 q[4][2] = {{-400, 400}, {400, 400}, {400, -400}, {-400, -400}};
	Mtx m;
	int v;

	gm_lookat(m, 0, 0, 1700, 0, 1, 0, 0, 0, 0);
	GX_LoadPosMtxImm(m, GX_PNMTX0);
	GX_SetCurrentMtx(GX_PNMTX0);
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetNumChans(1);
	GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
	GX_SetNumTexGens(0);
	GX_SetNumTevStages(1);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	for (v = 1; v < 8; v++)
		GX_SetTevOrder(GX_TEVSTAGE0 + v, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLORNULL);
	GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetAlphaUpdate(GX_TRUE);
	GX_SetColorUpdate(GX_TRUE);
	GX_SetZCompLoc(GX_FALSE);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
	GX_SetCullMode(GX_CULL_NONE);
	GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
	for (v = 0; v < 4; v++) {
		GX_Position3s16(q[v][0], q[v][1], 10);
		GX_Color4u8(col.bg.r, col.bg.g, col.bg.b, (u8)alpha);
	}
	GX_End();
}

void scene_draw(const IntroFrame *f)
{
	setup_light(f);

	/* FUN_8130d688 (phase 1/3): letters, globals, big cube, tiles + small cube, logo */
	j3d_global_setup();
	j3d_draw(&letters.inst);
	if (txt.n)
		letters_draw();
	j3d_global_setup();
	if (f->big_alpha > 0)
		j3d_draw(&big.inst);
	draw_tiles(f);
	j3d_draw(&cube.inst);
	if (f->logo_alpha > 0)
		j3d_draw(&logo.inst);
	if (col.buttons && f->logo_alpha > 0)
		draw_buttons(f);
	if (f->fade_alpha > 0)
		draw_fade(f->fade_alpha);
}

/* ---- SMB mode (smb_render.c): access to the scene's resources ----------- */

void scene_light(const IntroFrame *f)
{
	setup_light(f);
}

GXTexObj *scene_tile_texture(void)
{
	return &tile_tex;
}

/* final colour of the k-th tile of an endless trail (the level's ground) */
void scene_level_rgb(int k, u8 out[3])
{
	u8 tmp[3];
	tile_color_for(tile_target(&col.cfg, k < 0 ? 0 : k, tmp), out);
}

/* the cube's colour as a tile colour: the level's flat layer is shaded with it */
void scene_theme_rgb(u8 out[3])
{
	memcpy(out, col.tile_start, 3);
}

/* the small cube model at base = view * model (the level's hard blocks) */
void scene_draw_block(Mtx base)
{
	/* the animation's own small cube shares this instance: put it back afterwards */
	Mtx keep;
	int alpha = cube.inst.alpha;
	gm_copy(cube.inst.base, keep);
	gm_copy(base, cube.inst.base);
	cube.inst.alpha = 255;
	j3d_calc_rest(&cube.inst);
	j3d_draw(&cube.inst);
	gm_copy(keep, cube.inst.base);
	cube.inst.alpha = alpha;
	j3d_calc_rest(&cube.inst);
}

/* the trail's tiles only, with f->tiles as given (the SMB mode shifts it) */
void scene_draw_trail(const IntroFrame *f)
{
	draw_tiles(f);
}
