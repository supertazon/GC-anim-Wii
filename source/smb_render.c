/*
 * smb_render.c - draws the SMB view inside the boot animation's 3D scene
 *
 * The level lies on the plane of the cube's third face (x = 0 in tile space,
 * the face the trail ends on), seen through the animation's camera. World
 * anchor: level pixel (L, y) is at face coordinates (19 + L/16, 5 + y/16): the
 * level starts right after the logo's trail, whose tile (18, 18) (level column
 * -1, row 13) is where it branches off. Nothing is drawn left of it. One 16-px metatile = one IPL tile (54 units). A "shift" (dx, dy) in face
 * units moves the whole world (trail included) under the camera: it frames the
 * play area and follows the NES scroll.
 *   ground metatiles -> IPL tiles, coloured like the trail (endless pattern),
 *                       laid by two small cubes rolling at the right edge
 *   the rest         -> one flat textured layer (CI8 + TLUT of palette slots); in
 *                       every 4-colour NES palette the colours are ranked by
 *                       brightness and given well separated shades of the theme
 */
#include <string.h>
#include <math.h>
#include "smb.h"
#include "scene.h"
#include "gcmath.h"
#include "j3d.h"
#include "smb_palette.h"

#define TILE 54.0f
#define GROUND_SCREEN_Y (-40.0f)    /* screen height (view units) of the ground top, mid-screen */

static u8 layer_tex[SMB_VIEW_W * SMB_VIEW_H] ATTRIBUTE_ALIGN(32);
static u16 tlut[256] ATTRIBUTE_ALIGN(32);
static GXTexObj layer_obj;
static u8 left_tex[SMB_LEFT_W * SMB_VIEW_H] ATTRIBUTE_ALIGN(32);
static GXTexObj left_obj;
static u8 ghost_tex[SMB_ENEMIES][SMB_GW * SMB_GH] ATTRIBUTE_ALIGN(32);
static u8 score_tex[2][48 * 8] ATTRIBUTE_ALIGN(32);   /* 2: the corner's and the result's */
static u8 text_tex[2][SMB_TEXT_W * SMB_TEXT_H] ATTRIBUTE_ALIGN(32);   /* 2: two at once */
static GXTexObj text_obj[2];
#define FW_MAX 6
static u8 fw_tex[FW_MAX][SMB_GW * SMB_GH] ATTRIBUTE_ALIGN(32);
static GXTexObj fw_obj[FW_MAX];
static GXTexObj score_obj[2];
static u8 hud_tex[96 * 8] ATTRIBUTE_ALIGN(32);    /* the corner: score and time */
static GXTexObj hud_obj;
static GXTexObj ghost_obj[SMB_ENEMIES];
static GXTlutObj tlut_obj;
static u8 ply_tex[SMB_PLY * SMB_PLY] ATTRIBUTE_ALIGN(32);
static u8 icon_tex[SMB_PLY * SMB_PLY] ATTRIBUTE_ALIGN(32);   /* the world screen's Mario */
static GXTexObj icon_obj;
static GXTexObj ply_obj;
static uint8_t nes_rgb[64][3];        /* original colours: the NES's own */
static int orig_colours;
static float X0, Y0;                 /* framing: plane coordinates of NES screen pixel (0,0) */
static int plane_ok;

/* brightness of the 64 NES colours, from the PPU's composite signal levels */
static float nes_luma(int c)
{
	static const float lo_v[4] = {0.350f, 0.518f, 0.962f, 1.550f};
	static const float hi_v[4] = {1.094f, 1.506f, 1.962f, 1.962f};
	const float black = 0.518f, white = 1.962f;
	int hue = c & 15, lv = (c >> 4) & 3, p;
	float lo = lo_v[lv], hi = hi_v[lv], y = 0.0f;
	if (hue == 0) lo = hi;
	if (hue == 13) hi = lo;
	if (hue >= 14) lo = hi = black;
	for (p = 0; p < 12; p++)
		y += ((((hue + p) % 12) < 6) ? hi : lo);
	y = (y / 12.0f - black) / (white - black);
	return y < 0.0f ? 0.0f : y > 1.0f ? 1.0f : y;
}

static u16 rgb5a3_opaque(int r, int g, int b)
{
	return (u16)(0x8000 | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
}

static u8 theme[3], bgc[3];

static void mix(const u8 a[3], const u8 b[3], float t, int out[3])
{
	int i;
	for (i = 0; i < 3; i++)
		out[i] = (int)(a[i] + (b[i] - a[i]) * t + 0.5f);
}

void smb_render_init(void)
{
	static const u8 white[3] = {255, 255, 255};
	GXColor bg = scene_background();
	int c;
	(void)white;
	scene_theme_rgb(theme);
	smb_palette_rgb(nes_rgb);
	bgc[0] = bg.r; bgc[1] = bg.g; bgc[2] = bg.b;
	for (c = 0; c < 256; c++)
		tlut[c] = 0;                       /* SMB_LAYER_TRANSPARENT and unused: alpha 0 */
	DCFlushRange(tlut, sizeof(tlut));
	GX_InitTlutObj(&tlut_obj, tlut, GX_TL_RGB5A3, 256);
	GX_InitTexObjCI(&layer_obj, layer_tex, SMB_VIEW_W, SMB_VIEW_H, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, GX_TLUT0);
	GX_InitTexObjFilterMode(&layer_obj, GX_NEAR, GX_NEAR);
	GX_InitTexObjCI(&left_obj, left_tex, SMB_LEFT_W, SMB_VIEW_H, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, GX_TLUT0);
	GX_InitTexObjFilterMode(&left_obj, GX_NEAR, GX_NEAR);
	GX_InitTexObj(&score_obj[0], score_tex[0], 48, 8, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_InitTexObj(&score_obj[1], score_tex[1], 48, 8, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_InitTexObjFilterMode(&score_obj[1], GX_NEAR, GX_NEAR);
	{
		int q;
		for (q = 0; q < FW_MAX; q++) {
			GX_InitTexObjCI(&fw_obj[q], fw_tex[q], SMB_GW, SMB_GH, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, GX_TLUT0);
			GX_InitTexObjFilterMode(&fw_obj[q], GX_NEAR, GX_NEAR);
		}
	}
	GX_InitTexObjFilterMode(&score_obj[0], GX_NEAR, GX_NEAR);
	GX_InitTexObj(&hud_obj, hud_tex, 96, 8, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_InitTexObjFilterMode(&hud_obj, GX_NEAR, GX_NEAR);
	{
		int q;
		for (q = 0; q < 2; q++) {
			GX_InitTexObj(&text_obj[q], text_tex[q], SMB_TEXT_W, SMB_TEXT_H, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
			GX_InitTexObjFilterMode(&text_obj[q], GX_NEAR, GX_NEAR);
		}
	}
	{
		int g;
		for (g = 0; g < SMB_ENEMIES; g++) {
			GX_InitTexObjCI(&ghost_obj[g], ghost_tex[g], SMB_GW, SMB_GH, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, GX_TLUT0);
			GX_InitTexObjFilterMode(&ghost_obj[g], GX_NEAR, GX_NEAR);
		}
	}
	GX_InitTexObjCI(&ply_obj, ply_tex, SMB_PLY, SMB_PLY, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, GX_TLUT0);
	GX_InitTexObjFilterMode(&ply_obj, GX_NEAR, GX_NEAR);
	GX_InitTexObjCI(&icon_obj, icon_tex, SMB_PLY, SMB_PLY, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, GX_TLUT0);
	GX_InitTexObjFilterMode(&icon_obj, GX_NEAR, GX_NEAR);
	plane_ok = 0;
}

/* shades for the palette slots of this frame. Per 4-colour palette: near-black
 * colours -> outline shade, white -> white, the others ranked by brightness ->
 * dark / theme / light (2 colours: dark + light, 1 colour: theme) */
/* hue rotation (degrees) and value scale of an RGB colour */
static void hue_shift(const u8 in[3], float deg, float vscale, u8 out[3])
{
	float r = in[0] / 255.0f, g = in[1] / 255.0f, b = in[2] / 255.0f;
	float mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
	float mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
	float d = mx - mn, h = 0.0f, sat = mx > 0.0f ? d / mx : 0.0f, v = mx * vscale, f, p, q, t;
	int i;
	if (d > 0.0f) {
		if (mx == r) h = (g - b) / d;
		else if (mx == g) h = 2.0f + (b - r) / d;
		else h = 4.0f + (r - g) / d;
		h *= 60.0f;
	}
	h += deg;
	while (h < 0.0f) h += 360.0f;
	while (h >= 360.0f) h -= 360.0f;
	if (v > 1.0f) v = 1.0f;
	i = (int)(h / 60.0f);
	f = h / 60.0f - (float)i;
	p = v * (1.0f - sat); q = v * (1.0f - sat * f); t = v * (1.0f - sat * (1.0f - f));
	switch (i) {
	case 0: r = v; g = t; b = p; break;
	case 1: r = q; g = v; b = p; break;
	case 2: r = p; g = v; b = t; break;
	case 3: r = p; g = q; b = v; break;
	case 4: r = t; g = p; b = v; break;
	default: r = v; g = p; b = q; break;
	}
	out[0] = (u8)(r * 255.0f + 0.5f); out[1] = (u8)(g * 255.0f + 0.5f); out[2] = (u8)(b * 255.0f + 0.5f);
}

#define BG_HUE    (-28.0f)     /* background items: hue turned towards blue ... */
#define BG_VALUE  0.80f        /* ... and slightly darker than the sprites        */

static int cyc_frame, cyc_seen[32];
static u8 cyc_last[32];

static void mixi(const int *a, const int *b, float t, int out[3])
{
	int i;
	for (i = 0; i < 3; i++)
		out[i] = (int)((float)a[i] + ((float)b[i] - (float)a[i]) * t + 0.5f);
}

void smb_render_set_colours(int original)
{
	orig_colours = original;
}

static void build_tlut(const SmbView *v)
{
	static const u8 white[3] = {255, 255, 255};
	int g, i, j;
	int outline[3], dark[3], mid[3], light[3];
	int bout[3], bdark[3], bmid[3], blight[3];
	u8 btheme[3], bwhite[3];
	mix(bgc, theme, 0.22f, outline);
	mix(bgc, theme, 0.55f, dark);
	mid[0] = theme[0]; mid[1] = theme[1]; mid[2] = theme[2];
	mix(theme, white, 0.60f, light);
	/* the same ramp for the background palettes, on a shifted theme */
	hue_shift(theme, BG_HUE, BG_VALUE, btheme);
	hue_shift(white, 0.0f, 0.88f, bwhite);
	mix(bgc, btheme, 0.22f, bout);
	mix(bgc, btheme, 0.55f, bdark);
	bmid[0] = btheme[0]; bmid[1] = btheme[1]; bmid[2] = btheme[2];
	mix(btheme, bwhite, 0.55f, blight);

	if (orig_colours) {
		/* the game's own colours */
		for (g = 0; g < 8; g++)
			for (i = 1; i < 4; i++) {
				const uint8_t *c = nes_rgb[v->pal[g * 4 + i] & 0x3f];
				tlut[g * 4 + i] = rgb5a3_opaque(c[0], c[1], c[2]);
			}
		DCFlushRange(tlut, sizeof(tlut));
		return;
	}
	/* background palette entries the game cycles (SMB: the "?" blocks' colour, slot 13) are left
	   out of the brightness ranking, or the other colours of their palette (the used
	   block's brown) would swap shades with each step of the cycle; they get a shade
	   from their own brightness instead, so they still blink */
	cyc_frame++;
	/* (background palettes only: a sprite palette that changes - Mario's, for the fire
	   flower or the world screen's own colours - is ranked like any other, or his
	   colours would all come out alike for two seconds) */
	for (i = 0; i < 16; i++)
		if (v->pal[i] != cyc_last[i]) {
			if (cyc_frame > 1)
				cyc_seen[i] = cyc_frame;
			cyc_last[i] = v->pal[i];
		}

	for (g = 0; g < 8; g++) {
		const int *o_ = g < 4 ? bout : outline, *d_ = g < 4 ? bdark : dark;
		const int *m_ = g < 4 ? bmid : mid, *l_ = g < 4 ? blight : light;
		int idx[3], n = 0;
		float L[4];
		for (i = 1; i < 4; i++) {
			int slot = g * 4 + i, nc = v->pal[slot];
			L[i] = nes_luma(nc);
			if (L[i] < 0.08f)
				tlut[slot] = rgb5a3_opaque(o_[0], o_[1], o_[2]);
			else if (nc == 0x20 || nc == 0x30)
				tlut[slot] = rgb5a3_opaque(255, 255, 255);
			else if (cyc_seen[slot] && cyc_frame - cyc_seen[slot] < 120) {
				/* cycling: dark -> mid -> light along its brightness */
				int c3[3];
				float u = (L[i] - 0.10f) / 0.25f;
				if (u < 1.0f)
					mixi(d_, m_, u < 0.0f ? 0.0f : u, c3);
				else {
					u = (L[i] - 0.35f) / 0.30f;
					mixi(m_, l_, u > 1.0f ? 1.0f : u, c3);
				}
				tlut[slot] = rgb5a3_opaque(c3[0], c3[1], c3[2]);
			} else
				idx[n++] = i;
		}
		/* sort the remaining ones by brightness */
		for (i = 0; i < n; i++)
			for (j = i + 1; j < n; j++)
				if (L[idx[j]] < L[idx[i]]) { int t = idx[i]; idx[i] = idx[j]; idx[j] = t; }
		for (i = 0; i < n; i++) {
			const int *c = n == 1 ? m_ : n == 2 ? (i == 0 ? d_ : l_) : (i == 0 ? d_ : i == 1 ? m_ : l_);
			tlut[g * 4 + idx[i]] = rgb5a3_opaque(c[0], c[1], c[2]);
		}
	}
	DCFlushRange(tlut, sizeof(tlut));
}


/* tile-space point of the face: (xf, yf) in tile units, like a face-2 tile's (x, y) */
static void plane_point(float xf, float yf, float out[3])
{
	out[0] = 0.0f;
	out[1] = -(yf - 16.0f) * TILE;
	out[2] = -(xf - 16.0f) * TILE;
}

/* world position of a level pixel (absolute x, NES screen y) */
#define LEVEL_X0 19.0f        /* face column of level pixel 0 */
static float wx(float level_px) { return LEVEL_X0 + level_px / 16.0f; }
static float wy(float py) { return 5.0f + py / 16.0f; }

/* X0/Y0: where the NES screen's (0,0) must be so that the middle of the ground
 * top sits at (0, GROUND_SCREEN_Y) in this camera */
static void plane_fit(const IntroFrame *cam)
{
	float p0[3], px[3], py[3], v0[3], vx[3], vy[3];
	float ax, ay, bx, by, cx, cy, det, u, w;
	plane_point(0, 0, p0); plane_point(1, 0, px); plane_point(0, 1, py);
	gm_multvec((MtxP)cam->tiles, p0, v0);
	gm_multvec((MtxP)cam->tiles, px, vx);
	gm_multvec((MtxP)cam->tiles, py, vy);
	ax = vx[0] - v0[0]; ay = vx[1] - v0[1];
	bx = vy[0] - v0[0]; by = vy[1] - v0[1];
	cx = 0.0f - v0[0]; cy = GROUND_SCREEN_Y - v0[1];
	det = ax * by - bx * ay;
	u = (cx * by - bx * cy) / det;
	w = (ax * cy - cx * ay) / det;
	X0 = u - 128.0f / 16.0f;
	Y0 = w - 208.0f / 16.0f;
	plane_ok = 1;
}

void smb_render_play_shift(const IntroFrame *cam, int32_t level_x, float *dx, float *dy)
{
	if (!plane_ok)
		plane_fit(cam);
	*dx = X0 - wx((float)level_x);
	*dy = Y0 - wy(0.0f);
}

void smb_render_shifted(const IntroFrame *cam, float dx, float dy, Mtx out)
{
	Mtx t;
	gm_trans(t, 0.0f, -dy * TILE, -dx * TILE);
	gm_concat((MtxP)cam->tiles, t, out);
}

static void nes_view(const IntroFrame *cam, float dx, float dy, int32_t level_x, float nx, float ny, float out[3])
{
	Mtx m;
	float q[3];
	if (!plane_ok)
		plane_fit(cam);
	smb_render_shifted(cam, dx, dy, m);
	plane_point(wx((float)level_x + nx), wy(ny), q);
	gm_multvec(m, q, out);
}

void smb_render_nes_view_point(const IntroFrame *cam, float dx, float dy, int32_t level_x, float nes_x, float nes_y,
                               float out[3])
{
	nes_view(cam, dx, dy, level_x, nes_x, nes_y, out);
}

float smb_render_nes_view_x(const IntroFrame *cam, float dx, float dy, int32_t level_x, float nes_x)
{
	float a[3], b[3];
	/* the picture's top and bottom: the leftmost of the two, if the edge leans */
	nes_view(cam, dx, dy, level_x, nes_x, (float)SMB_VIEW_Y0, a);
	nes_view(cam, dx, dy, level_x, nes_x, 240.0f, b);
	return a[0] < b[0] ? a[0] : b[0];
}

float smb_render_nes_view_y(const IntroFrame *cam, float dx, float dy, int32_t level_x, float nes_x, float nes_y)
{
	float a[3];
	nes_view(cam, dx, dy, level_x, nes_x, nes_y, a);
	return a[1];
}

void smb_render_player_point(const SmbView *v, int32_t level_x, float out[3])
{
	/* the bottom middle of the player's 32x32 box (see player_overlay) */
	plane_point(wx((float)(level_x + v->ply_x) + 16.0f), wy((float)v->ply_y + 32.0f), out);
}

/* CI8 textures are stored in 8x4 tiles */
static void to_ci8(u8 *d, const u8 *src, int w, int h)
{
	int ty, tx, r;
	for (ty = 0; ty < h; ty += 4)
		for (tx = 0; tx < w; tx += 8)
			for (r = 0; r < 4; r++) {
				memcpy(d, &src[(ty + r) * w + tx], 8);
				d += 8;
			}
}

void smb_render_prepare(const SmbView *v)
{
	build_tlut(v);
	to_ci8(layer_tex, v->layer, SMB_VIEW_W, SMB_VIEW_H);
	DCFlushRange(layer_tex, sizeof(layer_tex));
	to_ci8(left_tex, v->left, SMB_LEFT_W, SMB_VIEW_H);
	DCFlushRange(left_tex, sizeof(left_tex));
	{
		int g;
		for (g = 0; g < SMB_ENEMIES; g++)
			if (v->ghost[g].on)
				to_ci8(ghost_tex[g], v->ghost[g].img, SMB_GW, SMB_GH);
		DCFlushRange(ghost_tex, sizeof(ghost_tex));
	}
	to_ci8(ply_tex, v->ply, SMB_PLY, SMB_PLY);
	DCFlushRange(ply_tex, sizeof(ply_tex));
}

static void load_mtx(MtxP m)
{
	Mtx nrm;
	GX_LoadPosMtxImm(m, GX_PNMTX0);
	guMtxInvXpose(m, nrm);
	GX_LoadNrmMtxImm(nrm, GX_PNMTX0);
	GX_SetCurrentMtx(GX_PNMTX0);
}

static void quad(const float p[4][3], const float st[4][2])
{
	int i;
	GX_Begin(GX_QUADS, GX_VTXFMT2, 4);
	for (i = 0; i < 4; i++) {
		GX_Position3f32(p[i][0], p[i][1], p[i][2]);
		GX_TexCoord2f32(st[i][0], st[i][1]);
	}
	GX_End();
}

static void vtx_pos_tex(void)
{
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT2, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT2, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
}

/* ---- the 16:9 left strip fades out: it is the part already passed, which Mario can
 * no longer reach (SMB's left wall is the NES picture's edge) ---- */
#define FADE_W 32.0f                 /* NES px: full -> 25 % */
static float edge_fade(float x)
{
	float f;
	if (x >= 0.0f)
		return 1.0f;
	if (x >= -FADE_W)
		return 1.0f - 0.75f * (-x / FADE_W);
	f = 0.25f * (1.0f - (-x - FADE_W) / FADE_W);
	return f < 0.0f ? 0.0f : f;
}

/* a quad on the face from NES x xa..xb (screen-relative), NES y ya..yb; texture
 * coordinates (h along x, v along y; hv: the texture's s is v, t is h), colour rgb
 * with alpha, cut in 4-px slices where it lies left of the picture so it fades */
static void quad_faded(int32_t level_x, float xa, float xb, float ya, float yb,
                       float ha, float hb, float va, float vb, int hv,
                       const u8 rgb[3], int alpha)
{
	float x = xa;
	while (x < xb) {
		float q = x < 0.0f ? (float)((int)floorf(x / 4.0f) * 4 + 4) : xb, p[4][3], st[4][2];
		float h0, h1, f;
		GXColor c;
		if (q > xb) q = xb;
		if (q <= x) q = xb;
		f = edge_fade((x + q) * 0.5f);
		h0 = ha + (hb - ha) * (x - xa) / (xb - xa);
		h1 = ha + (hb - ha) * (q - xa) / (xb - xa);
		if (f > 0.0f) {
			c.r = rgb[0]; c.g = rgb[1]; c.b = rgb[2]; c.a = (u8)((float)alpha * f);
			GX_SetChanMatColor(GX_COLOR0A0, c);
			plane_point(wx((float)level_x + x), wy(ya), p[0]);
			plane_point(wx((float)level_x + q), wy(ya), p[1]);
			plane_point(wx((float)level_x + q), wy(yb), p[2]);
			plane_point(wx((float)level_x + x), wy(yb), p[3]);
			if (hv) {
				st[0][0] = va; st[0][1] = h0; st[1][0] = va; st[1][1] = h1;
				st[2][0] = vb; st[2][1] = h1; st[3][0] = vb; st[3][1] = h0;
			} else {
				st[0][0] = h0; st[0][1] = va; st[1][0] = h1; st[1][1] = va;
				st[2][0] = h1; st[2][1] = vb; st[3][0] = h0; st[3][1] = vb;
			}
			quad(p, st);
		}
		x = q;
	}
}

/* ground metatiles: the IPL tile, GX state as in scene.c draw_tiles(); drawn up to
 * the cubes (ground_limit), past the NES picture's right edge if they are there */
static void draw_ground(MtxP M, const SmbView *v, const SmbDraw *d)
{
	int i;
	load_mtx(M);
	vtx_pos_tex();
	GX_SetNumChans(1);
	GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
	GX_SetNumTexGens(1);
	GX_SetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_DTTIDENTITY);
	GX_SetNumTevStages(1);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
	GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetColorUpdate(GX_TRUE);
	GX_SetAlphaUpdate(GX_TRUE);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
	GX_SetCullMode(GX_CULL_NONE);
	GX_LoadTexObj(scene_tile_texture(), GX_TEXMAP0);

	for (i = 0; i < v->nmeta; i++) {
		const SmbMeta *m = &v->meta[i];
		u8 rgb[3];
		if (m->kind != SMB_MT_GROUND || m->col > v->ground_limit[m->row])
			continue;                      /* not laid yet: the cubes are still on their way */
		/* past the picture's edges too: left from what is still in the nametables
		   (fading), right as far as the cubes have laid it. Colours continue the
		   trail: level column 0 follows its 12th tile (seq 11). The trail's tiles
		   use texture coordinates 0..2 (texture repeated). */
		if (orig_colours && m->ncol != 0xff)
			memcpy(rgb, nes_rgb[m->ncol], 3);   /* the ground's own colour */
		else
			scene_level_rgb(m->col + 12, rgb);
		quad_faded(v->level_x, (float)m->px, (float)m->px + 16.0f,
		           (float)m->row * 16.0f, (float)m->row * 16.0f + 16.0f,
		           0.0f, 2.0f, 0.0f, 2.0f, 1, rgb, d->level_alpha);
	}
}

/* a small cube resting on world cell (col, row) of the face, rolling towards
 * col + 1 (roll 0..1), lifted by lift tiles; it stands out of the face (+X) */
static void cube_mtx(MtxP M, float col, int row, float roll, float lift, Mtx out)
{
	float y = -(wy((float)row * 16.0f) + 0.5f - 16.0f) * TILE;
	float pz = -(wx(0.0f) + col + 1.0f - 16.0f) * TILE;   /* leading bottom edge */
	float ang = roll * (float)M_PI * 0.5f, cs = cosf(ang), sn = sinf(ang);
	Mtx m;
	m[0][0] = cs;  m[0][1] = 0; m[0][2] = sn;  m[0][3] = 27.0f * cs + 27.0f * sn + lift * TILE;
	m[1][0] = 0;   m[1][1] = 1; m[1][2] = 0;   m[1][3] = y;
	m[2][0] = -sn; m[2][1] = 0; m[2][2] = cs;  m[2][3] = pz - 27.0f * sn + 27.0f * cs;
	gm_concat(M, m, out);
}

static void draw_cubes(MtxP M, const SmbView *v)
{
	int i;
	j3d_global_setup();
	/* back to front: the camera looks down on the face, so the top cube (row 13)
	   is nearer and partly hides the bottom one (row 14) - bottom one first */
	for (i = 1; i >= 0; i--) {
		const SmbCube *k = &v->cube[i];
		Mtx base;
		if (!k->visible)
			continue;
		cube_mtx(M, (float)k->col + k->hop, k->row, k->roll, k->lift, base);
		scene_draw_block(base);
	}
}

void smb_render_resting_cube(MtxP M, int col, int row)
{
	Mtx base;
	j3d_global_setup();
	cube_mtx(M, (float)col, row, 0.0f, 0.0f, base);
	scene_draw_block(base);
}

/* textured quad, TEV: texture x alpha (material colour) */
static void tex_state(int alpha)
{
	GXColor c = {255, 255, 255, (u8)alpha};
	vtx_pos_tex();
	GX_LoadTlut(&tlut_obj, GX_TLUT0);
	GX_SetNumChans(1);
	GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
	GX_SetChanMatColor(GX_COLOR0A0, c);
	GX_SetNumTexGens(1);
	GX_SetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_DTTIDENTITY);
	GX_SetNumTevStages(1);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_ALWAYS, 0);
	GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
	GX_SetCullMode(GX_CULL_NONE);
}

/* everything else: one textured quad over the play area, cut at the cubes */
static void draw_layer(MtxP M, const SmbView *v, const SmbDraw *d)
{
	float x1 = (float)d->clip, p[4][3], st[4][2];
	float y0 = wy((float)SMB_VIEW_Y0), y1 = wy((float)(SMB_VIEW_Y0 + SMB_VIEW_H));
	float a = wx((float)v->level_x), b = wx((float)v->level_x + x1), s1 = x1 / (float)SMB_VIEW_W;
	if (x1 <= 0 || d->level_alpha <= 0)
		return;
	load_mtx(M);
	tex_state(d->level_alpha);
	GX_LoadTexObj(&layer_obj, GX_TEXMAP0);
	plane_point(a, y0, p[0]); st[0][0] = 0;  st[0][1] = 0;
	plane_point(b, y0, p[1]); st[1][0] = s1; st[1][1] = 0;
	plane_point(b, y1, p[2]); st[2][0] = s1; st[2][1] = 1;
	plane_point(a, y1, p[3]); st[3][0] = 0;  st[3][1] = 1;
	quad(p, st);
	/* and the background still in the nametables left of the picture (16:9), fading */
	{
		static const u8 white[3] = {255, 255, 255};
		GX_LoadTexObj(&left_obj, GX_TEXMAP0);
		quad_faded(v->level_x, -(float)SMB_LEFT_W, 0.0f, (float)SMB_VIEW_Y0,
		           (float)(SMB_VIEW_Y0 + SMB_VIEW_H), 0.0f, 1.0f, 0.0f, 1.0f, 0,
		           white, d->level_alpha);
	}
	GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
}

void smb_render_score(const uint8_t *bits, float x, float y, float h, const u8 rgb[3], int alpha)
{
	smb_render_score_slot(0, bits, x, y, h, rgb, alpha);
}

/* (a texture of its own, small: Dolphin's fast texture cache, which hashes only some
   texels, misses a few changed digits in the big text texture - not this one) */
static void small_text(u8 *tex, GXTexObj *obj, int tw, const uint8_t *bits, float x, float y, float h,
                       const u8 rgb[3], int alpha);

void smb_render_score_slot(int slot, const uint8_t *bits, float x, float y, float h, const u8 rgb[3], int alpha)
{
	slot = slot ? 1 : 0;
	small_text(score_tex[slot], &score_obj[slot], 48, bits, x, y, h, rgb, alpha);
}

void smb_render_hud(const uint8_t *bits, float x, float y, float h, const u8 rgb[3], int alpha)
{
	small_text(hud_tex, &hud_obj, 96, bits, x, y, h, rgb, alpha);
}

/* tw x 8 one-bit pixels (bits), centred on x, y, h high */
static void small_text(u8 *tex, GXTexObj *obj, int tw, const uint8_t *bits, float x, float y, float h,
                       const u8 rgb[3], int alpha)
{
	static const float st[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
	float w = h * (float)tw / 8.0f, p[4][3];
	u8 lin[96 * 8];
	Mtx id;
	GXColor c;
	int i;
	if (alpha <= 0)
		return;
	for (i = 0; i < tw * 8; i++)
		lin[i] = bits[i] ? 255 : 0;
	to_ci8(tex, lin, tw, 8);                   /* I8 uses the same 8x4 tiling */
	DCFlushRange(tex, (u32)tw * 8);
	GX_InvalidateTexAll();
	guMtxIdentity(id);
	load_mtx(id);
	vtx_pos_tex();
	GX_SetNumChans(1);
	GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
	c.r = rgb[0]; c.g = rgb[1]; c.b = rgb[2]; c.a = (u8)alpha;
	GX_SetChanMatColor(GX_COLOR0A0, c);
	GX_SetNumTexGens(1);
	GX_SetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_DTTIDENTITY);
	GX_SetNumTevStages(1);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
	GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
	GX_SetCullMode(GX_CULL_NONE);
	GX_LoadTexObj(obj, GX_TEXMAP0);
	p[0][0] = x - w * 0.5f; p[0][1] = y + h * 0.5f; p[0][2] = -100.0f;
	p[1][0] = x + w * 0.5f; p[1][1] = y + h * 0.5f; p[1][2] = -100.0f;
	p[2][0] = x + w * 0.5f; p[2][1] = y - h * 0.5f; p[2][2] = -100.0f;
	p[3][0] = x - w * 0.5f; p[3][1] = y - h * 0.5f; p[3][2] = -100.0f;
	quad(p, st);
}

/* text lines (smb_text), face on: centred on x, top at y, px view units per pixel */
void smb_render_text(const SmbText *t, float x, float y, float px, const u8 rgb[3], int alpha)
{
	smb_render_text_slot(0, t, x, y, px, rgb, alpha);
}

void smb_render_text_slot(int slot, const SmbText *t, float x, float y, float px, const u8 rgb[3], int alpha)
{
	static u8 lin[SMB_TEXT_W * SMB_TEXT_H];
	float w = t->w * px, h = t->h * px, s1 = (float)t->w / SMB_TEXT_W, t1 = (float)t->h / SMB_TEXT_H;
	float st[4][2], p[4][3];
	Mtx id;
	GXColor c;
	int i;
	if (alpha <= 0 || t->w <= 0 || t->h <= 0)
		return;
	for (i = 0; i < SMB_TEXT_W * SMB_TEXT_H; i++)
		lin[i] = t->bits[i] ? 255 : 0;
	slot = slot ? 1 : 0;
	to_ci8(text_tex[slot], lin, SMB_TEXT_W, SMB_TEXT_H);      /* I8 uses the same 8x4 tiling */
	DCFlushRange(text_tex[slot], sizeof(text_tex[slot]));
	GX_InvalidateTexAll();
	guMtxIdentity(id);
	load_mtx(id);
	vtx_pos_tex();
	GX_SetNumChans(1);
	GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
	c.r = rgb[0]; c.g = rgb[1]; c.b = rgb[2]; c.a = (u8)alpha;
	GX_SetChanMatColor(GX_COLOR0A0, c);
	GX_SetNumTexGens(1);
	GX_SetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_DTTIDENTITY);
	GX_SetNumTevStages(1);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
	GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
	GX_SetCullMode(GX_CULL_NONE);
	GX_LoadTexObj(&text_obj[slot], GX_TEXMAP0);
	st[0][0] = 0;  st[0][1] = 0;
	st[1][0] = s1; st[1][1] = 0;
	st[2][0] = s1; st[2][1] = t1;
	st[3][0] = 0;  st[3][1] = t1;
	p[0][0] = x - w * 0.5f; p[0][1] = y;     p[0][2] = -100.0f;
	p[1][0] = x + w * 0.5f; p[1][1] = y;     p[1][2] = -100.0f;
	p[2][0] = x + w * 0.5f; p[2][1] = y - h; p[2][2] = -100.0f;
	p[3][0] = x - w * 0.5f; p[3][1] = y - h; p[3][2] = -100.0f;
	quad(p, (const float (*)[2])st);
}

void smb_render_sprite_upload(int i, const uint8_t *img)
{
	if (i < 0 || i >= FW_MAX)
		return;
	to_ci8(fw_tex[i], img, SMB_GW, SMB_GH);
	DCFlushRange(fw_tex[i], sizeof(fw_tex[i]));
	GX_InvalidateTexAll();
}

void smb_render_sprite_view(int i, float x, float y, float px, int alpha)
{
	static const float st[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
	float w = SMB_GW * px * 0.5f, h = SMB_GH * px * 0.5f, p[4][3];
	Mtx id;
	if (i < 0 || i >= FW_MAX || alpha <= 0)
		return;
	guMtxIdentity(id);
	load_mtx(id);
	tex_state(alpha);
	GX_LoadTexObj(&fw_obj[i], GX_TEXMAP0);
	p[0][0] = x - w; p[0][1] = y + h; p[0][2] = -100.0f;
	p[1][0] = x + w; p[1][1] = y + h; p[1][2] = -100.0f;
	p[2][0] = x + w; p[2][1] = y - h; p[2][2] = -100.0f;
	p[3][0] = x - w; p[3][1] = y - h; p[3][2] = -100.0f;
	quad(p, st);
	GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
}

void smb_render_icon_upload(const uint8_t *img)
{
	to_ci8(icon_tex, img, SMB_PLY, SMB_PLY);
	DCFlushRange(icon_tex, sizeof(icon_tex));
	GX_InvalidateTexAll();
}

void smb_render_icon_view(float x, float y_top, float px, int alpha)
{
	static const float st[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
	float w = SMB_PLY * px * 0.5f, h = SMB_PLY * px, p[4][3];
	Mtx id;
	if (alpha <= 0)
		return;
	guMtxIdentity(id);
	load_mtx(id);
	tex_state(alpha);
	GX_LoadTexObj(&icon_obj, GX_TEXMAP0);
	p[0][0] = x - w; p[0][1] = y_top;     p[0][2] = -100.0f;
	p[1][0] = x + w; p[1][1] = y_top;     p[1][2] = -100.0f;
	p[2][0] = x + w; p[2][1] = y_top - h; p[2][2] = -100.0f;
	p[3][0] = x - w; p[3][1] = y_top - h; p[3][2] = -100.0f;
	quad(p, st);
	GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
}

/* enemies the game hides at the left edge (see SmbGhost) */
static void draw_ghosts(MtxP M, const SmbView *v, const SmbDraw *d)
{
	static const u8 white[3] = {255, 255, 255};
	int g, first = 1;
	if (d->level_alpha <= 0)
		return;
	for (g = 0; g < SMB_ENEMIES; g++) {
		const SmbGhost *e = &v->ghost[g];
		if (!e->on)
			continue;
		if (first) {
			load_mtx(M);
			tex_state(d->level_alpha);
			first = 0;
		}
		GX_LoadTexObj(&ghost_obj[g], GX_TEXMAP0);
		quad_faded(v->level_x, (float)e->x, (float)(e->x + SMB_GW), (float)e->y,
		           (float)(e->y + SMB_GH), 0.0f, 1.0f, 0.0f, 1.0f, 0, white, d->level_alpha);
	}
	if (!first)
		GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
}

/* the player: its own quad, at its NES position in matrix d->ply_mtx, moved by
 * d->ply_off (tile space) and scaled about its feet */
static void draw_player(const SmbView *v, const SmbDraw *d)
{
	float c[3], p[4][3], st[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
	float half = 16.0f / 16.0f * TILE * d->ply_scale, h = 2.0f * TILE * d->ply_scale;
	float cs = cosf(d->ply_rot), sn = sinf(d->ply_rot), my;
	int i;
	if (!d->ply_show || !v->ply_on || d->ply_alpha <= 0)
		return;
	/* reference point: middle of the bottom edge of the 32x32 box */
	plane_point(wx((float)(d->ply_level_x + v->ply_x) + 16.0f), wy((float)v->ply_y + 32.0f), c);
	for (i = 0; i < 3; i++)
		c[i] += d->ply_off[i];
	/* the box on the face: z runs along the level (-), y up */
	p[0][0] = c[0]; p[0][1] = c[1] + h; p[0][2] = c[2] + half;
	p[1][0] = c[0]; p[1][1] = c[1] + h; p[1][2] = c[2] - half;
	p[2][0] = c[0]; p[2][1] = c[1];     p[2][2] = c[2] - half;
	p[3][0] = c[0]; p[3][1] = c[1];     p[3][2] = c[2] + half;
	/* spin in the face's plane about the box's middle */
	my = c[1] + h * 0.5f;
	for (i = 0; i < 4; i++) {
		float y = p[i][1] - my, z = p[i][2] - c[2];
		p[i][1] = my + y * cs - z * sn;
		p[i][2] = c[2] + y * sn + z * cs;
	}
	if (d->ply_mirror) {
		for (i = 0; i < 4; i++)
			st[i][0] = 1.0f - st[i][0];
	}
	if (d->ply_face > 0.0f) {
		/* turn it towards the viewer: in view space, rotate the box about its middle
		   from the face's orientation to the screen's, by ply_face of the way */
		float v[4][3], m[3], ex[3], ey[3], nz[3], q[4], l, th, ax, k;
		Mtx id;
		for (i = 0; i < 4; i++)
			gm_multvec((MtxP)d->ply_mtx, p[i], v[i]);
		for (i = 0; i < 3; i++) {
			m[i] = (v[0][i] + v[1][i] + v[2][i] + v[3][i]) * 0.25f;
			ex[i] = v[1][i] - v[0][i];
			ey[i] = v[0][i] - v[3][i];
		}
		l = sqrtf(ex[0] * ex[0] + ex[1] * ex[1] + ex[2] * ex[2]);
		for (i = 0; i < 3; i++) ex[i] /= l;
		k = ey[0] * ex[0] + ey[1] * ex[1] + ey[2] * ex[2];
		for (i = 0; i < 3; i++) ey[i] -= k * ex[i];
		l = sqrtf(ey[0] * ey[0] + ey[1] * ey[1] + ey[2] * ey[2]);
		for (i = 0; i < 3; i++) ey[i] /= l;
		nz[0] = ex[1] * ey[2] - ex[2] * ey[1];
		nz[1] = ex[2] * ey[0] - ex[0] * ey[2];
		nz[2] = ex[0] * ey[1] - ex[1] * ey[0];
		/* R (rows ex, ey, nz) takes the box's axes onto the screen's: as a quaternion */
		{
			float tr = ex[0] + ey[1] + nz[2];
			if (tr > -0.999f) {
				float w = sqrtf(1.0f + tr) * 0.5f;
				q[0] = w;
				q[1] = (nz[1] - ey[2]) / (4.0f * w);   /* (R21 - R12) / 4w */
				q[2] = (ex[2] - nz[0]) / (4.0f * w);   /* (R02 - R20) / 4w */
				q[3] = (ey[0] - ex[1]) / (4.0f * w);   /* (R10 - R01) / 4w */
			} else {
				q[0] = 1.0f; q[1] = q[2] = q[3] = 0.0f;
			}
		}
		/* part of the way: angle * ply_face about the same axis */
		th = 2.0f * acosf(q[0] > 1.0f ? 1.0f : q[0]);
		ax = sqrtf(q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
		if (ax > 1e-5f) {
			float a = th * d->ply_face, sh = sinf(a * 0.5f) / ax;
			float w = cosf(a * 0.5f), x = q[1] * sh, y = q[2] * sh, z = q[3] * sh;
			float r[3][3] = {
				{1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y)},
				{2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x)},
				{2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y)}};
			for (i = 0; i < 4; i++) {
				float o[3] = {v[i][0] - m[0], v[i][1] - m[1], v[i][2] - m[2]};
				int j;
				for (j = 0; j < 3; j++)
					p[i][j] = m[j] + r[j][0] * o[0] + r[j][1] * o[1] + r[j][2] * o[2];
			}
		} else
			memcpy(p, v, sizeof(v));
		guMtxIdentity(id);
		load_mtx(id);
	} else
		load_mtx((MtxP)d->ply_mtx);
	tex_state(d->ply_alpha);
	GX_LoadTexObj(&ply_obj, GX_TEXMAP0);
	quad(p, st);
	GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
}

void smb_render_draw(const IntroFrame *cam, const SmbView *v, const SmbDraw *d)
{
	Mtx M;
	int i;
	if (!plane_ok)
		plane_fit(cam);
	Mtx MT;
	smb_render_shifted(cam, d->dx, d->dy, M);
	smb_render_shifted(cam, d->tdx, d->tdy, MT);
	scene_light(cam);
	if (d->trail) {
		static IntroFrame f;
		f = *d->trail;
		gm_copy(MT, f.tiles);
		/* the branch frame's last tiles were still fading in: the trail is done now */
		for (i = 0; i < f.ntiles; i++)
			f.tile[i].alpha = 255;
		f.nlaid += 1000;
		scene_draw_trail(&f);
	}
	if (d->level_alpha > 0) {
		draw_ground(M, v, d);
		if (d->show_cubes)
			draw_cubes(M, v);
	}
	if (d->resting_cube)
		smb_render_resting_cube(MT, 2, 13);
	draw_layer(M, v, d);
	draw_ghosts(M, v, d);
	draw_player(v, d);
}
