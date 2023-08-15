#ifndef _CUBES_H_
#define _CUBES_H_

#include "bti.h"

void prepsCubeView(f32 speed, f32 fspeed, u8* stage, f32 tv1, f32 tv2, f32* alpha, f32 alphaT[]);	// Animates the coordinates of the small cube and the camera after the first fall
void dispSmallCube(GXTexObj* env_tex, f32 alphaSC);	// Displays the small rotating cube
void dispBigCube(GXTexObj* env_tex, f32 alpha);		// Displays the big translucent cube

#endif