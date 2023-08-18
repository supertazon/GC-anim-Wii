#include "globals.h"

//General variables
u8 notfadeout=1;
bool returnToMenu=false;
bool go=true;		// Some video systems take a while to initialize (Wii2HDMI, GCVideo) so we add this little flag to wait a while before launching the animation
//u8 initcnt=0;	// Initial frame counter Uncomment if you want to add a delay at the beginning of the animation

// Camera
f32 camX=200.0f;
f32 camY=-200.0f;
f32 lookX=0.0f;
f32 lookY=1.0f;
f32 ty=0.0f;

// Display corrections: 16:9, resolutions (480, 528 or 264 efbHeight) and 50 Hz
void initGraphics(f32* gH, f32* gV, f32* scaleGCy, f32 posyGC[5], f32* tv1, f32* tv2, f32* tv3){ 
    //GRRLIB_Init();
	
	f32 posyInit;
	
	if (CONF_GetAspectRatio() == CONF_ASPECT_16_9) GX_SetViewport(70.0f, 0.0f, 500.0f, rmode->efbHeight, 0.0f, 1.0f); // Viewport correction for 16:9
	
	if(rmode->efbHeight==480){
		*gH=309.79937f;
		*gV=240.0f;
		*scaleGCy=0.96f;
		posyInit=305.3f;
	}
	else if(rmode->efbHeight==528){
		*gH=303.79937f;
		*gV=216.0f;
		*scaleGCy=1.1f;
		posyInit=357.83f;
	}
	else{ // efbHeight 264
		*gH=309.79937f;
		*gV=240.0f;
		*tv3=0.5f;
		*scaleGCy=0.96f*(*tv3);
		posyInit=173.315f;
	}
		
	if(VIDEO_GetCurrentTvMode()==VI_PAL){ // Modulate speed of the animation if PAL (50 Hz) is detected
		*tv1=1.2f;
		*tv2=1.08f;
	}
	
	for(u8 k=0;k<5;k++) posyGC[k]=posyInit;
	
	GRRLIB_Settings.antialias = true;
}