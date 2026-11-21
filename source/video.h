/*
 * video.h - video mode and anti-aliased two-pass rendering
 *
 * The IPL renders the animation with 3-sample anti-aliasing (RGB565 EFB),
 * which halves the usable EFB height, so each frame is drawn twice (top and
 * bottom half) and copied to the XFB in two parts.  The IPL framebuffer is
 * 592 pixels wide, scaled to 640 by the VI; 448 lines at 60 Hz, 520 at 50 Hz.
 *
 * Supported: NTSC/PAL60/MPAL 480i and 480p (60 Hz), PAL 576i (50 Hz),
 * 4:3 and 16:9 (the scene is widened, not stretched).
 */
#ifndef VIDEO_H
#define VIDEO_H

#include <gccore.h>

typedef struct {
	GXRModeObj rmode;
	void *xfb[2];
	int cur;
	int hz50;
	int progressive;
	int widescreen;
	u16 half;          /* xfb lines per pass */
	u16 efb_h;         /* EFB lines per pass (half + 2) */
	float ortho_l, ortho_r, ortho_t, ortho_b;
} Video;

void video_init(Video *v, int force_43);
/* pass 0 = top half, 1 = bottom half: sets scissor, viewport and projection */
void video_begin_pass(Video *v, int pass);
/* shift what is drawn next in this pass by (ox, oy) view units, +y up (0,0: as set up) */
void video_set_offset(Video *v, float ox, float oy);
/* copy the half to the XFB (and clear the EFB) */
void video_end_pass(Video *v, int pass);
/* wait for the GPU, show the frame, wait for the vertical retrace */
void video_present(Video *v);
/* EFB clear colour (the scene background) */
void video_set_background(Video *v, GXColor c);
void video_black(Video *v);

#endif
