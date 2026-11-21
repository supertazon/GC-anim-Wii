/*
 * promo.h - the "promo" GameCube intro (the alternative intro of Nintendo's
 * promotional videos), rendered with GX from the user's own IPL assets.
 */
#ifndef PROMO_H
#define PROMO_H

#include <gccore.h>
#include "ipl.h"
#include "config.h"
#include "video.h"

/* v: GC-anim-Wii's video (video_init done); cfg: gc-anim.ini colours (NULL = the promo's) */
int  promo_init(IplAssets *a, Video *v, const Config *cfg);
/* draw the frame at time t (reference frames: 30 per second) into the XFB being built;
   the caller then calls video_present() */
void promo_draw(float t);
void promo_free(void);

#endif
