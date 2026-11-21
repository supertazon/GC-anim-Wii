/*
 * video.c - video mode and anti-aliased two-pass rendering (see video.h)
 *
 * Mirrors the IPL: render mode at 0x8137f138 (NTSC) / 0x81382470 (PAL),
 * FUN_81301834 (frame start), FUN_813019d0 / FUN_81301970 / FUN_81301a10
 * (the two half copies), FUN_813014b8 (projection).
 */
#include <string.h>
#include <malloc.h>
#include "video.h"

#define FIFO_SIZE (256 * 1024)

static void *gp_fifo;

/* IPL interlaced AA filter (a little deflicker); progressive uses the SDK AA filter */
static const u8 vf_interlaced[7] = {8, 8, 10, 12, 10, 8, 8};
static const u8 vf_progressive[7] = {4, 8, 12, 16, 12, 8, 4};
static const u8 aa_pattern[12][2] = {
	{3, 2}, {9, 6}, {3, 10}, {3, 2}, {9, 6}, {3, 10},
	{9, 2}, {3, 6}, {9, 10}, {9, 2}, {3, 6}, {9, 10}
};

void video_init(Video *v, int force_43)
{
	GXRModeObj *pref;
	GXColor black = {0, 0, 0, 0xFF};
	u32 tv;
	int i;

	memset(v, 0, sizeof(*v));
	VIDEO_Init();
	pref = VIDEO_GetPreferredMode(NULL);
	tv = VIDEO_GetCurrentTvMode();

	v->progressive = (pref->viTVMode & 3) == VI_PROGRESSIVE;
	v->hz50 = (tv == VI_PAL) && !v->progressive;
	v->widescreen = !force_43 && CONF_GetAspectRatio() == CONF_ASPECT_16_9;

	memset(&v->rmode, 0, sizeof(v->rmode));
	if (v->hz50) {
		v->rmode.viTVMode = VI_TVMODE_PAL_INT;
		v->rmode.xfbHeight = 520;
		v->rmode.viYOrigin = (VI_MAX_HEIGHT_PAL - 520) / 2;
		v->rmode.viHeight = 520;
	} else {
		u32 base = (tv == VI_PAL || tv == VI_EURGB60) ? VI_EURGB60 : (tv == VI_MPAL ? VI_MPAL : VI_NTSC);
		v->rmode.viTVMode = VI_TVMODE(base, v->progressive ? VI_PROGRESSIVE : VI_INTERLACE);
		v->rmode.xfbHeight = 448;
		v->rmode.viYOrigin = (VI_MAX_HEIGHT_NTSC - 448) / 2;
		v->rmode.viHeight = 448;
	}
	v->rmode.fbWidth = 592;
	v->half = v->rmode.xfbHeight / 2;
	v->efb_h = v->half + 2;
	v->rmode.efbHeight = v->efb_h;
	v->rmode.viWidth = 640;
	v->rmode.viXOrigin = (VI_MAX_WIDTH_NTSC - 640) / 2;
	v->rmode.xfbMode = v->progressive ? VI_XFBMODE_SF : VI_XFBMODE_DF;
	v->rmode.field_rendering = GX_FALSE;
	v->rmode.aa = GX_TRUE;
	memcpy(v->rmode.sample_pattern, aa_pattern, sizeof(aa_pattern));
	memcpy(v->rmode.vfilter, v->progressive ? vf_progressive : vf_interlaced, 7);

	VIDEO_Configure(&v->rmode);
	for (i = 0; i < 2; i++) {
		v->xfb[i] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(&v->rmode));
		VIDEO_ClearFrameBuffer(&v->rmode, v->xfb[i], COLOR_BLACK);
	}
	v->cur = 0;
	VIDEO_SetNextFramebuffer(v->xfb[0]);
	VIDEO_SetBlack(TRUE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if (v->rmode.viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();

	gp_fifo = memalign(32, FIFO_SIZE);
	memset(gp_fifo, 0, FIFO_SIZE);
	GX_Init(gp_fifo, FIFO_SIZE);
	GX_SetCopyClear(black, GX_MAX_Z24);
	GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	GX_SetDispCopyYScale(1.0f);
	GX_SetDispCopyDst(v->rmode.fbWidth, v->rmode.xfbHeight);
	GX_SetCopyFilter(GX_TRUE, v->rmode.sample_pattern, GX_TRUE, v->rmode.vfilter);
	GX_SetFieldMode(GX_FALSE, GX_FALSE);
	GX_SetDispCopyGamma(GX_GM_1_0);

	/* IPL: ortho(t=224, b=-224, l=-296, r=292); 16:9 widens the scene horizontally */
	v->ortho_t = 224.0f;
	v->ortho_b = -224.0f;
	if (v->widescreen) {
		float c = (292.0f + -296.0f) * 0.5f, hw = (292.0f - -296.0f) * 0.5f * (4.0f / 3.0f);
		v->ortho_l = c - hw;
		v->ortho_r = c + hw;
	} else {
		v->ortho_l = -296.0f;
		v->ortho_r = 292.0f;
	}
}

void video_begin_pass(Video *v, int pass)
{
	Mtx44 p;
	u16 w = v->rmode.fbWidth;
	int y0 = pass ? v->efb_h - 4 : 0;   /* the bottom half overlaps by 4 lines (AA filter) */

	if (pass == 0) {
		GX_SetViewport(0.0f, 0.0f, (float)w + 0.5f, (float)v->rmode.xfbHeight + 0.5f, 0.0f, 1.0f);
		GX_InvVtxCache();
		GX_InvalidateTexAll();
	}
	GX_SetScissor(0, y0, w, v->efb_h);
	GX_SetScissorBoxOffset(0, y0);
	guOrtho(p, v->ortho_t, v->ortho_b, v->ortho_l, v->ortho_r, 50.0f, 10000.0f);
	GX_LoadProjectionMtx(p, GX_ORTHOGRAPHIC);
}

/* shift what is drawn next by (ox, oy) view units (+y up), within the current pass */
void video_set_offset(Video *v, float ox, float oy)
{
	Mtx44 p;
	guOrtho(p, v->ortho_t - oy, v->ortho_b - oy, v->ortho_l - ox, v->ortho_r - ox, 50.0f, 10000.0f);
	GX_LoadProjectionMtx(p, GX_ORTHOGRAPHIC);
}

void video_end_pass(Video *v, int pass)
{
	u16 w = v->rmode.fbWidth;
	u8 *xfb = (u8 *)v->xfb[v->cur ^ 1];

	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_SetColorUpdate(GX_TRUE);
	if (pass == 0) {
		GX_SetCopyClamp(GX_CLAMP_TOP | GX_CLAMP_BOTTOM);
		GX_SetDispCopySrc(0, 0, w, v->efb_h);
		GX_SetDispCopyDst(w, v->efb_h);
		GX_CopyDisp(xfb, GX_TRUE);
	} else {
		u32 stride = ((w + 15) & ~15) * 2;
		GX_SetCopyClamp(GX_CLAMP_BOTTOM);
		GX_SetDispCopySrc(0, 2, w, v->efb_h - 2);
		GX_SetDispCopyDst(w, v->efb_h - 2);
		GX_CopyDisp(xfb + stride * (v->efb_h - 2), GX_TRUE);
		/* clear the first two EFB lines, which the bottom copy skipped */
		{
			static u8 scratch[640 * 2 * 2] ATTRIBUTE_ALIGN(32);
			GX_SetCopyClamp(GX_CLAMP_TOP | GX_CLAMP_BOTTOM);
			GX_SetDispCopySrc(0, 0, w, 2);
			GX_SetDispCopyDst(w, 2);
			GX_CopyDisp(scratch, GX_TRUE);
		}
		GX_SetDispCopySrc(0, 0, w, v->efb_h);
		GX_SetDispCopyDst(w, v->efb_h);
		GX_SetScissorBoxOffset(0, 0);
		GX_SetScissor(0, 0, w, v->efb_h);
	}
}

void video_present(Video *v)
{
	GX_DrawDone();
	v->cur ^= 1;
	VIDEO_SetNextFramebuffer(v->xfb[v->cur]);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
}

void video_set_background(Video *v, GXColor c)
{
	int y, cb, cr, i;
	u32 yuyv;

	c.a = 0xFF;
	GX_SetCopyClear(c, GX_MAX_Z24);
	/* clear the EFB now, so the very first pass already starts on the background */
	GX_SetCopyClamp(GX_CLAMP_TOP | GX_CLAMP_BOTTOM);
	GX_SetDispCopySrc(0, 0, v->rmode.fbWidth, v->efb_h);
	GX_SetDispCopyDst(v->rmode.fbWidth, v->efb_h);
	GX_CopyDisp(v->xfb[v->cur ^ 1], GX_TRUE);
	GX_DrawDone();

	/* fill both framebuffers with the background (shown while the rest loads) */
	y  = (257 * c.r + 504 * c.g + 98 * c.b) / 1000 + 16;
	cb = (-148 * c.r - 291 * c.g + 439 * c.b) / 1000 + 128;
	cr = (439 * c.r - 368 * c.g - 71 * c.b) / 1000 + 128;
	yuyv = (u32)y << 24 | (u32)cb << 16 | (u32)y << 8 | (u32)cr;
	for (i = 0; i < 2; i++)
		VIDEO_ClearFrameBuffer(&v->rmode, v->xfb[i], yuyv);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
}

void video_black(Video *v)
{
	(void)v;
	VIDEO_SetBlack(TRUE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
}
