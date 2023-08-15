/*===========================================
	GC intro animation recreation by supertazon
============================================*/
#include <grrlib.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>
#include <asndlib.h>
#include <fat.h>
#include <sdcard/wiisd_io.h>

#include "descrambler.h"
#include "yay0dec.h"
#include "afcdecoder.h"
#include "bti.h"
#include "globals.h"
#include "logos.h"
#include "cubes.h"
#include "tiles.h"

#define PRIILOADER_TOMENU       0x50756E65 // Credits to Aurelio for the defines
#define PRIILOADER_MAGICADDR    ((vu32*)0x8132FFFB)

void bootSysMenu();		// Credits to Aurelio

Mtx44 perspective;
u8 stage=0;			// Counter of stage (ie which general position the rolling cube is at)
f32 tv1=1.0f;	// Adjustment parameter for PAL (50 Hz), mainly used for speed of animation
f32 tv2=1.0f;	// Adjustment parameter for PAL (50 Hz), mainly used for speed of animation
f32 tv3=1.0f;	// Adjustment parameter for small resolution (efbHeight 264)
f32 gV,gH;		// guOrtho Vertical (top and bottom) and Horizontal (left and right) parameters for orthographic projection

//GAMECUBE init parameters
f32 posyInit;			
f32 scaleGCy;

// Alpha values of each element that will fade in / fade out
f32 alphaSC=255.0f;		// Small Cube alpha value
f32 alphaBC=0.0f;		// Big Cube alpha value
f32 alphaT[15]={0.0f};	// Tile alpha values

u8 cnt=0; // Terrible name, frame counter to manage the position of the camera in the LAST part of the animation when the small cube hits the big translucent cube again

int main(int argc, char *argv[]) {
	GRRLIB_Init();
	FILE* fp;
	u32 iplSize;
	u8* iplData;
	u32 yayOffset[4]={0};
	u32 awOffset[2]={0};
	
	u8* env_bti=(u8*) malloc(16448);
	u8* tile_bti=(u8*) malloc(4128);
	u8* logoT_bti=(u8*) malloc(88992);
	u8* logoL_bti=(u8*) malloc(88992);
	u8* logoR_bti=(u8*) malloc(88992);
	u8* gc_bti=(u8*) malloc(14144);
	
	GXTexObj env_tex, tile_tex, gc_tex, logoT_tex, logoL_tex, logoR_tex;
	
	f32 posyGC[5]={0.0f};
	f32 speed;
	f32 fspeed;
	
	u8 allow=1; // Flag to allow the animation to proceed if SD card is detected an IPL file is opened, skip to bootSysMenu otherwise
	
	// Initialize the SD card
	if(!fatMountSimple("sd", &__io_wiisd)){
		__io_wiisd.shutdown();
		allow=0;
	}

	if(allow){
		fp = fopen("sd:/ipl.bin", "rb");
		if(fp==NULL){
			fclose(fp);
			fp=fopen("sd:/iplusa.bin", "rb");
		}
		if(fp==NULL){
			fclose(fp);
			fp=fopen("sd:/ipljap.bin", "rb");
		}
		if(fp==NULL){
			fclose(fp);
			fp=fopen("sd:/iplpal.bin", "rb");
		}
		if(fp==NULL){
			fclose(fp);
			allow=0;
		}
		else{
			fseek(fp, 0, SEEK_END);
			iplSize = ftell(fp) - 0x100;
			fseek(fp, 0x100, SEEK_SET);
			iplData = (u8*) malloc(iplSize);
			fread(iplData, 1, iplSize, fp);
			fclose(fp);
		
			Descrambler(iplData,iplSize);
		
			prepOffsets(iplData,iplSize,yayOffset,awOffset);
		
			initGraphics(&gH, &gV, &scaleGCy, posyGC, &tv1, &tv2, &tv3);

			u8* envDec=decompressYay0(iplData,yayOffset[0]);
			memcpy(env_bti,envDec+8448,16448);
			free(envDec);
			BTI_GetTexture((BTIFile*)env_bti, 0, &env_tex);

			u8* tileDec=decompressYay0(iplData,yayOffset[1]);
			memcpy(tile_bti,tileDec,4128);
			free(tileDec);
			BTI_GetTexture((BTIFile*)tile_bti, 0, &tile_tex);

			u8* logoDec=decompressYay0(iplData,yayOffset[2]);
			memcpy(logoT_bti,logoDec+5056,88992);
			memcpy(logoL_bti,logoDec+5088,88960);
			memcpy(logoR_bti,logoDec+5120,88928);
			free(logoDec);
			BTI_GetTexture((BTIFile*)logoT_bti, 0, &logoT_tex);
			BTI_GetTexture((BTIFile*)logoL_bti, 0, &logoL_tex);
			BTI_GetTexture((BTIFile*)logoR_bti, 0, &logoR_tex);

			u8* gcDec=decompressYay0(iplData,yayOffset[3]);
			memcpy(gc_bti,gcDec+11616,14144);
			free(gcDec);
			BTI_GetTexture((BTIFile*)gc_bti, 0, &gc_tex);
		
			ASND_Init();
			ASND_Pause(0);
		
			playAW(iplData,awOffset[0],awOffset[1]);
		
			free(iplData);
			fatUnmount("sd");
		
			speed=9.0f*tv1;		// Rotation speed (in degrees, 10 frames per stage [at 60 Hz] to perform a stage of 90 degrees)
			fspeed=30.0f*tv1;	// Final rotation speed when the cube moves up before falling

			GRRLIB_SetBackgroundColour(0x00, 0x00, 0x00, 0xFF); // Black
		}
	}

    while(allow) {		
		GRRLIB_Camera3dSettings(200.0f,camY,200.0f, lookX, lookY, 0.0f, 0,ty,0);

		if(cnt>=18){ // Block that handles NINTENDO display
			// Reset 3D scene and projection parameters
			GRRLIB_3dMode(1,1000,45,1,1);
			guOrtho(perspective,-gV,gV,gH,-gH,1,1000);
			GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);
			dispNintendo(&env_tex, cnt);	// Displays and animates the NINTENDO text at the right moment
		}

		if(stage==21){ // Block that handles GAMECUBE display
			GRRLIB_2dMode();
			dispGamecube(&gc_tex,(BTIFile*) gc_bti,posyGC,scaleGCy,tv3,&alphaSC,&alphaBC,alphaT,&cnt); // At stage=21, the rolling cube has finished its path and we enter the last part of the animation and fadeout
		}
		
		if(cnt>=28){ // Block that handles Logo display
			GRRLIB_3dMode(1,1000,45,1,1);	// Reset 3D mode
			guOrtho(perspective,-gV,gV,gH,-gH,1,1000);
			GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);
			dispLogo(&logoT_tex,&logoL_tex,&logoR_tex);	// Display final logo
		}

		if(cnt<44 && go){ // Block that handles small cube, big cube and tiles display
			GRRLIB_3dMode(1,1000,45,0,1);
			guOrtho(perspective,-gV,gV,gH,-gH,1,1000);
			GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);

			prepsCubeView(speed,fspeed,&stage,tv1,tv2,&alphaBC,alphaT);
			dispSmallCube(&env_tex,alphaSC);
			
			dispBigCube(&env_tex,alphaBC);

			dispTiles(&tile_tex, stage,alphaT);
		}
		
		GX_ClearVtxDesc();						// This block is needed or the animation would not exit to the system menu.
		GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);	// Otherwise it hangs forever.
		GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
		GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
		
		GRRLIB_Render();
		if(returnToMenu) break;
	}
	
	free(env_bti);
	free(tile_bti);
	free(gc_bti);
	free(logoT_bti);
	free(logoL_bti);
	free(logoR_bti);
	ASND_End();
	GRRLIB_Exit();
	bootSysMenu();
	
	return 0;
}

void bootSysMenu(){
    *PRIILOADER_MAGICADDR = PRIILOADER_TOMENU;
    DCFlushRange((void*)PRIILOADER_MAGICADDR, 4);
    SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
}