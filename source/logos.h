#ifndef _LOGOS_H_
#define _LOGOS_H_

#include "bti.h"

void dispLogo(GXTexObj* logoT_tex, GXTexObj* logoL_tex, GXTexObj* logoR_tex);	// Handles display of final logo
void dispNintendo(GXTexObj* env_tex, u8 cnt);									// Handles animation and display of NINTENDO models
void dispGamecube(GXTexObj* gc_tex, BTIFile* bti, f32 posyGC[], f32 scaleGCy, f32 tv3, f32* alphaC, f32* alpha, f32 alphaT[], u8* cnt); // Handles animation and display of GAMECUBE texture, and fadeout alpha values for all models

#endif