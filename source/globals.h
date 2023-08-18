#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <grrlib.h>

extern u8 notfadeout;
extern bool returnToMenu;
extern bool go;	// Some video systems take a while to initialize (Wii2HDMI, GCVideo) so we add this little flag to wait a while before launching the animation
//extern u8 initcnt=0;	// Initial frame counter: Uncomment if you want to add a delay at the beginning of the animation

// Camera
extern f32 camX;
extern f32 camY;
extern f32 lookX;
extern f32 lookY;
extern f32 ty;

void initGraphics(f32* gH, f32* gV, f32* scaleGCy, f32 posyGC[5], f32* tv1, f32* tv2, f32* tv3); // Initializes GRRLIB, resolution and animation speed parameters depending on Wii's region and display settings

#endif