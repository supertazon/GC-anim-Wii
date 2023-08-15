#include "globals.h"
#include "models.h"
#include "logos.h"
#include "bti.h"

//NINTENDO positions, rotations, alpha values
f32 posxN[8]={217.6f,186.0f,164.2f,135.6f,107.5f,78.0f,48.4f,19.5f};
f32 posyN=41.5f;
f32 poszN[8]={5.4f,38.0f,59.8f,88.4f,116.5f,146.0f,175.6f,204.5f};
f32 rotxN[8]={55.0f,55.0f,55.0f,55.0f,55.0f,55.0f,55.0f,55.0f}; // finish at 151.0f
f32 rotSpeed=96.0f/36.0f;
f32 rotyN=225.0f;
f32 rotzN=0.0f;
f32 sN=1.0f;
u8 colorN=102;
f32 alphaN[8]={5.0f,5.0f,5.0f,5.0f,5.0f,5.0f,5.0f,5.0f};

//GAMECUBE written logo
u32 gcCol[19]={0xFFFFFF28,0xFFFFFF50,0xFFFFFF6E,0xFFFFFF8C,0xFFFFFFAA,0xFFFFFFB4,0xFFFFFFC8,0xFFFFFFDC,0xFFFFFFF0,0xFFFFFFFF,0xFFFFFF0F,0xFFFFFF1E,0xFFFFFF14,0xFFFFFF32,0xFFFFFF08,0xFFFFFF3C,0xFFFFFF04,0xFFFFFF09,0xFFFFFF23};
f32 posxGC=153.0f;		
f32 scaleGCx=1.02f;

// Logo
f32 rotxL=180.0f;
f32 rotyL=0.0f;
f32 rotzL=0.0f;
f32 posxL=0.0f;
f32 posyL=19.0f;
f32 poszL=0.0f;
f32 alphaL=0.0f;
//f32 scL=124.0f;		

// RGB color for the logo (white = texture color)
u8 lr=255;
u8 lg=255;
u8 lb=255;

void dispLogo(GXTexObj* logoT_tex, GXTexObj* logoL_tex, GXTexObj* logoR_tex){
	u8 k;
		
	// Logo Top
	GX_LoadTexObj(logoT_tex, GX_TEXMAP0);
	GRRLIB_ObjectView(posxL,posyL,poszL,rotxL,rotyL,rotzL,1.0f,1.0f,1.0f);
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 36);
	for (k=0;k<12;k++){
		GX_Position3f32(vLogoT[fLogoT[k][0][0]-1][0], vLogoT[fLogoT[k][0][0]-1][1], vLogoT[fLogoT[k][0][0]-1][2]);
		GX_Normal3f32(vnLogoT[fLogoT[k][0][2]-1][0], vnLogoT[fLogoT[k][0][2]-1][1], vnLogoT[fLogoT[k][0][2]-1][2]);
		GX_Color4u8(lr,lg,lb,alphaL);
		GX_TexCoord2f32(vtLogoT[fLogoT[k][0][1]-1][0], 1.0f-vtLogoT[fLogoT[k][0][1]-1][1]);
		GX_Position3f32(vLogoT[fLogoT[k][1][0]-1][0], vLogoT[fLogoT[k][1][0]-1][1], vLogoT[fLogoT[k][1][0]-1][2]);
		GX_Normal3f32(vnLogoT[fLogoT[k][1][2]-1][0], vnLogoT[fLogoT[k][1][2]-1][1], vnLogoT[fLogoT[k][1][2]-1][2]);
		GX_Color4u8(lr,lg,lb,alphaL);
		GX_TexCoord2f32(vtLogoT[fLogoT[k][1][1]-1][0], 1.0f-vtLogoT[fLogoT[k][1][1]-1][1]);
		GX_Position3f32(vLogoT[fLogoT[k][2][0]-1][0], vLogoT[fLogoT[k][2][0]-1][1], vLogoT[fLogoT[k][2][0]-1][2]);
		GX_Normal3f32(vnLogoT[fLogoT[k][2][2]-1][0], vnLogoT[fLogoT[k][2][2]-1][1], vnLogoT[fLogoT[k][2][2]-1][2]);
		GX_Color4u8(lr,lg,lb,alphaL);
		GX_TexCoord2f32(vtLogoT[fLogoT[k][2][1]-1][0], 1.0f-vtLogoT[fLogoT[k][2][1]-1][1]);
	}
	GX_End();
	
	// Logo Left
	GX_LoadTexObj(logoL_tex, GX_TEXMAP0);
	GRRLIB_ObjectView(posxL,posyL,poszL,rotxL,rotyL,rotzL,1.0f,1.0f,1.0f);
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 30);
	for (k=0;k<10;k++){
		GX_Position3f32(vLogoL[fLogoL[k][0][0]-1][0], vLogoL[fLogoL[k][0][0]-1][1], vLogoL[fLogoL[k][0][0]-1][2]);
		GX_Normal3f32(vnLogoL[fLogoL[k][0][2]-1][0], vnLogoL[fLogoL[k][0][2]-1][1], vnLogoL[fLogoL[k][0][2]-1][2]);
		GX_Color4u8(lr,lg,lb,alphaL);
		GX_TexCoord2f32(vtLogoL[fLogoL[k][0][1]-1][0], 1.0f-vtLogoL[fLogoL[k][0][1]-1][1]);
		GX_Position3f32(vLogoL[fLogoL[k][1][0]-1][0], vLogoL[fLogoL[k][1][0]-1][1], vLogoL[fLogoL[k][1][0]-1][2]);
		GX_Normal3f32(vnLogoL[fLogoL[k][1][2]-1][0], vnLogoL[fLogoL[k][1][2]-1][1], vnLogoL[fLogoL[k][1][2]-1][2]);
		GX_Color4u8(lr,lg,lb,alphaL);
		GX_TexCoord2f32(vtLogoL[fLogoL[k][1][1]-1][0], 1.0f-vtLogoL[fLogoL[k][1][1]-1][1]);
		GX_Position3f32(vLogoL[fLogoL[k][2][0]-1][0], vLogoL[fLogoL[k][2][0]-1][1], vLogoL[fLogoL[k][2][0]-1][2]);
		GX_Normal3f32(vnLogoL[fLogoL[k][2][2]-1][0], vnLogoL[fLogoL[k][2][2]-1][1], vnLogoL[fLogoL[k][2][2]-1][2]);
		GX_Color4u8(lr,lg,lb,alphaL);
		GX_TexCoord2f32(vtLogoL[fLogoL[k][2][1]-1][0], 1.0f-vtLogoL[fLogoL[k][2][1]-1][1]);
	}
	GX_End();
	
	// Logo Right
	GX_LoadTexObj(logoR_tex, GX_TEXMAP0);
	GRRLIB_ObjectView(posxL,posyL,poszL,rotxL,rotyL,rotzL,1.0f,1.0f,1.0f);
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 42);
	for (k=0;k<14;k++){
		GX_Position3f32(vLogoR[fLogoR[k][0][0]-1][0], vLogoR[fLogoR[k][0][0]-1][1], vLogoR[fLogoR[k][0][0]-1][2]);
		GX_Normal3f32(vnLogoR[fLogoR[k][0][2]-1][0], vnLogoR[fLogoR[k][0][2]-1][1], vnLogoR[fLogoR[k][0][2]-1][2]);
		GX_Color4u8(lr,lg,lb,alphaL);
		GX_TexCoord2f32(vtLogoR[fLogoR[k][0][1]-1][0], 1.0f-vtLogoR[fLogoR[k][0][1]-1][1]);
		GX_Position3f32(vLogoR[fLogoR[k][1][0]-1][0], vLogoR[fLogoR[k][1][0]-1][1], vLogoR[fLogoR[k][1][0]-1][2]);
		GX_Normal3f32(vnLogoR[fLogoR[k][1][2]-1][0], vnLogoR[fLogoR[k][1][2]-1][1], vnLogoR[fLogoR[k][1][2]-1][2]);
		GX_Color4u8(lr,lg,lb,alphaL);
		GX_TexCoord2f32(vtLogoR[fLogoR[k][1][1]-1][0], 1.0f-vtLogoR[fLogoR[k][1][1]-1][1]);
		GX_Position3f32(vLogoR[fLogoR[k][2][0]-1][0], vLogoR[fLogoR[k][2][0]-1][1], vLogoR[fLogoR[k][2][0]-1][2]);
		GX_Normal3f32(vnLogoR[fLogoR[k][2][2]-1][0], vnLogoR[fLogoR[k][2][2]-1][1], vnLogoR[fLogoR[k][2][2]-1][2]);
		GX_Color4u8(lr,lg,lb,alphaL);
		GX_TexCoord2f32(vtLogoR[fLogoR[k][2][1]-1][0], 1.0f-vtLogoR[fLogoR[k][2][1]-1][1]);
	}
	GX_End();
}

void dispNintendo(GXTexObj* env_tex, u8 cnt){ 
	u8 k;
	
	//N
	if(alphaN[0]<255.0f && notfadeout) alphaN[0]+=50.0f;
	if(rotxN[0]<150.0f) rotxN[0]+=rotSpeed;
	
	GX_SetTevAlphaIn(GX_TEVSTAGE0,GX_CA_ZERO,GX_CA_TEXA,GX_CA_RASA,GX_CA_RASA);
	GX_SetTevAlphaOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_LoadTexObj(env_tex, GX_TEXMAP0);
	
	GRRLIB_ObjectView(posxN[0],posyN,poszN[0],rotxN[0],rotyN,rotzN,sN,sN,sN);
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 108);
	for (k=0;k<36;k++){
		GX_Position3f32(vN[fN[k][0][0]-1][0], vN[fN[k][0][0]-1][1], vN[fN[k][0][0]-1][2]);
		GX_Normal3f32(vnN[fN[k][0][2]-1][0], vnN[fN[k][0][2]-1][1], vnN[fN[k][0][2]-1][2]);
		GX_Color4u8(colorN,colorN,colorN,alphaN[0]);
		GX_TexCoord2f32(vtN[fN[k][0][1]-1][0], 1.0f-vtN[fN[k][0][1]-1][1]);
		GX_Position3f32(vN[fN[k][1][0]-1][0], vN[fN[k][1][0]-1][1], vN[fN[k][1][0]-1][2]);
		GX_Normal3f32(vnN[fN[k][1][2]-1][0], vnN[fN[k][1][2]-1][1], vnN[fN[k][1][2]-1][2]);
		GX_Color4u8(colorN,colorN,colorN,alphaN[0]);
		GX_TexCoord2f32(vtN[fN[k][1][1]-1][0], 1.0f-vtN[fN[k][1][1]-1][1]);
		GX_Position3f32(vN[fN[k][2][0]-1][0], vN[fN[k][2][0]-1][1], vN[fN[k][2][0]-1][2]);
		GX_Normal3f32(vnN[fN[k][2][2]-1][0], vnN[fN[k][2][2]-1][1], vnN[fN[k][2][2]-1][2]);
		GX_Color4u8(colorN,colorN,colorN,alphaN[0]);
		GX_TexCoord2f32(vtN[fN[k][2][1]-1][0], 1.0f-vtN[fN[k][2][1]-1][1]);
	}
	GX_End();


	//I
	if(cnt>=22){
		if(alphaN[1]<255.0f && notfadeout) alphaN[1]+=50.0f;
		if(rotxN[1]<150.0f) rotxN[1]+=rotSpeed;

		GRRLIB_ObjectView(posxN[1],posyN,poszN[1],rotxN[1],rotyN,rotzN,sN,sN,sN);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 36);
		for (k=0;k<12;k++){
			GX_Position3f32(vI[fI[k][0][0]-1][0], vI[fI[k][0][0]-1][1], vI[fI[k][0][0]-1][2]);
			GX_Normal3f32(vnI[fI[k][0][2]-1][0], vnI[fI[k][0][2]-1][1], vnI[fI[k][0][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[1]);
			GX_TexCoord2f32(vtI[fI[k][0][1]-1][0], 1.0f-vtI[fI[k][0][1]-1][1]);
			GX_Position3f32(vI[fI[k][1][0]-1][0], vI[fI[k][1][0]-1][1], vI[fI[k][1][0]-1][2]);
			GX_Normal3f32(vnI[fI[k][1][2]-1][0], vnI[fI[k][1][2]-1][1], vnI[fI[k][1][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[1]);
			GX_TexCoord2f32(vtI[fI[k][1][1]-1][0], 1.0f-vtI[fI[k][1][1]-1][1]);
			GX_Position3f32(vI[fI[k][2][0]-1][0], vI[fI[k][2][0]-1][1], vI[fI[k][2][0]-1][2]);
			GX_Normal3f32(vnI[fI[k][2][2]-1][0], vnI[fI[k][2][2]-1][1], vnI[fI[k][2][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[1]);
			GX_TexCoord2f32(vtI[fI[k][2][1]-1][0], 1.0f-vtI[fI[k][2][1]-1][1]);
		}
		GX_End();
	}
	
	//N
	if(cnt>=24){
		if(alphaN[2]<255.0f && notfadeout) alphaN[2]+=50.0f;
		if(rotxN[2]<150.0f) rotxN[2]+=rotSpeed;

		GRRLIB_ObjectView(posxN[2],posyN,poszN[2],rotxN[2],rotyN,rotzN,sN,sN,sN);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 108);
		for (k=0;k<36;k++){
			GX_Position3f32(vN[fN[k][0][0]-1][0], vN[fN[k][0][0]-1][1], vN[fN[k][0][0]-1][2]);
			GX_Normal3f32(vnN[fN[k][0][2]-1][0], vnN[fN[k][0][2]-1][1], vnN[fN[k][0][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[2]);
			GX_TexCoord2f32(vtN[fN[k][0][1]-1][0], 1.0f-vtN[fN[k][0][1]-1][1]);
			GX_Position3f32(vN[fN[k][1][0]-1][0], vN[fN[k][1][0]-1][1], vN[fN[k][1][0]-1][2]);
			GX_Normal3f32(vnN[fN[k][1][2]-1][0], vnN[fN[k][1][2]-1][1], vnN[fN[k][1][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[2]);
			GX_TexCoord2f32(vtN[fN[k][1][1]-1][0], 1.0f-vtN[fN[k][1][1]-1][1]);
			GX_Position3f32(vN[fN[k][2][0]-1][0], vN[fN[k][2][0]-1][1], vN[fN[k][2][0]-1][2]);
			GX_Normal3f32(vnN[fN[k][2][2]-1][0], vnN[fN[k][2][2]-1][1], vnN[fN[k][2][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[2]);
			GX_TexCoord2f32(vtN[fN[k][2][1]-1][0], 1.0f-vtN[fN[k][2][1]-1][1]);
		}
		GX_End();
	}

	//T
	if(cnt>=26){
		if(alphaN[3]<255.0f && notfadeout) alphaN[3]+=50.0f;
		if(rotxN[3]<150.0f) rotxN[3]+=rotSpeed;

		GRRLIB_ObjectView(posxN[3],posyN,poszN[3],rotxN[3],rotyN,rotzN,sN,sN,sN);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 84);
		for (k=0;k<28;k++){
			GX_Position3f32(vT[fT[k][0][0]-1][0], vT[fT[k][0][0]-1][1], vT[fT[k][0][0]-1][2]);
			GX_Normal3f32(vnT[fT[k][0][2]-1][0], vnT[fT[k][0][2]-1][1], vnT[fT[k][0][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[3]);
			GX_TexCoord2f32(vtT[fT[k][0][1]-1][0], 1.0f-vtT[fT[k][0][1]-1][1]);
			GX_Position3f32(vT[fT[k][1][0]-1][0], vT[fT[k][1][0]-1][1], vT[fT[k][1][0]-1][2]);
			GX_Normal3f32(vnT[fT[k][1][2]-1][0], vnT[fT[k][1][2]-1][1], vnT[fT[k][1][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[3]);
			GX_TexCoord2f32(vtT[fT[k][1][1]-1][0], 1.0f-vtT[fT[k][1][1]-1][1]);
			GX_Position3f32(vT[fT[k][2][0]-1][0], vT[fT[k][2][0]-1][1], vT[fT[k][2][0]-1][2]);
			GX_Normal3f32(vnT[fT[k][2][2]-1][0], vnT[fT[k][2][2]-1][1], vnT[fT[k][2][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[3]);
			GX_TexCoord2f32(vtT[fT[k][2][1]-1][0], 1.0f-vtT[fT[k][2][1]-1][1]);
		}
		GX_End();
	}
		
	//E
	if(cnt>=28){
		if(alphaN[4]<255.0f && notfadeout) alphaN[4]+=50.0f;
		if(rotxN[4]<150.0f) rotxN[4]+=rotSpeed;
		
		GRRLIB_ObjectView(posxN[4],posyN,poszN[4],rotxN[4],rotyN,rotzN,sN,sN,sN);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 324);
		for (k=0;k<108;k++){
			GX_Position3f32(vE[fE[k][0][0]-1][0], vE[fE[k][0][0]-1][1], vE[fE[k][0][0]-1][2]);
			GX_Normal3f32(vnE[fE[k][0][2]-1][0], vnE[fE[k][0][2]-1][1], vnE[fE[k][0][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[4]);
			GX_TexCoord2f32(vtE[fE[k][0][1]-1][0], 1.0f-vtE[fE[k][0][1]-1][1]);
			GX_Position3f32(vE[fE[k][1][0]-1][0], vE[fE[k][1][0]-1][1], vE[fE[k][1][0]-1][2]);
			GX_Normal3f32(vnE[fE[k][1][2]-1][0], vnE[fE[k][1][2]-1][1], vnE[fE[k][1][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[4]);
			GX_TexCoord2f32(vtE[fE[k][1][1]-1][0], 1.0f-vtE[fE[k][1][1]-1][1]);
			GX_Position3f32(vE[fE[k][2][0]-1][0], vE[fE[k][2][0]-1][1], vE[fE[k][2][0]-1][2]);
			GX_Normal3f32(vnE[fE[k][2][2]-1][0], vnE[fE[k][2][2]-1][1], vnE[fE[k][2][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[4]);
			GX_TexCoord2f32(vtE[fE[k][2][1]-1][0], 1.0f-vtE[fE[k][2][1]-1][1]);
		}
		GX_End();
	}
	
	//N
	if(cnt>=30){
		if(alphaN[5]<255.0f && notfadeout) alphaN[5]+=50.0f;
		if(rotxN[5]<150.0f) rotxN[5]+=rotSpeed;

		GRRLIB_ObjectView(posxN[5],posyN,poszN[5],rotxN[5],rotyN,rotzN,sN,sN,sN);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 108);
		for (k=0;k<36;k++){
			GX_Position3f32(vN[fN[k][0][0]-1][0], vN[fN[k][0][0]-1][1], vN[fN[k][0][0]-1][2]);
			GX_Normal3f32(vnN[fN[k][0][2]-1][0], vnN[fN[k][0][2]-1][1], vnN[fN[k][0][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[5]);
			GX_TexCoord2f32(vtN[fN[k][0][1]-1][0], 1.0f-vtN[fN[k][0][1]-1][1]);
			GX_Position3f32(vN[fN[k][1][0]-1][0], vN[fN[k][1][0]-1][1], vN[fN[k][1][0]-1][2]);
			GX_Normal3f32(vnN[fN[k][1][2]-1][0], vnN[fN[k][1][2]-1][1], vnN[fN[k][1][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[5]);
			GX_TexCoord2f32(vtN[fN[k][1][1]-1][0], 1.0f-vtN[fN[k][1][1]-1][1]);
			GX_Position3f32(vN[fN[k][2][0]-1][0], vN[fN[k][2][0]-1][1], vN[fN[k][2][0]-1][2]);
			GX_Normal3f32(vnN[fN[k][2][2]-1][0], vnN[fN[k][2][2]-1][1], vnN[fN[k][2][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[5]);
			GX_TexCoord2f32(vtN[fN[k][2][1]-1][0], 1.0f-vtN[fN[k][2][1]-1][1]);
		}
		GX_End();
	}

	//D
	if(cnt>=32){
		if(alphaN[6]<255.0f && notfadeout) alphaN[6]+=50.0f;
		if(rotxN[6]<150.0f) rotxN[6]+=rotSpeed;

		GRRLIB_ObjectView(posxN[6],posyN,poszN[6],rotxN[6],rotyN,rotzN,sN,sN,sN);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 288);
		for (k=0;k<96;k++){
			GX_Position3f32(vD[fD[k][0][0]-1][0], vD[fD[k][0][0]-1][1], vD[fD[k][0][0]-1][2]);
			GX_Normal3f32(vnD[fD[k][0][2]-1][0], vnD[fD[k][0][2]-1][1], vnD[fD[k][0][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[6]);
			GX_TexCoord2f32(vtD[fD[k][0][1]-1][0], 1.0f-vtD[fD[k][0][1]-1][1]);
			GX_Position3f32(vD[fD[k][1][0]-1][0], vD[fD[k][1][0]-1][1], vD[fD[k][1][0]-1][2]);
			GX_Normal3f32(vnD[fD[k][1][2]-1][0], vnD[fD[k][1][2]-1][1], vnD[fD[k][1][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[6]);
			GX_TexCoord2f32(vtD[fD[k][1][1]-1][0], 1.0f-vtD[fD[k][1][1]-1][1]);
			GX_Position3f32(vD[fD[k][2][0]-1][0], vD[fD[k][2][0]-1][1], vD[fD[k][2][0]-1][2]);
			GX_Normal3f32(vnD[fD[k][2][2]-1][0], vnD[fD[k][2][2]-1][1], vnD[fD[k][2][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[6]);
			GX_TexCoord2f32(vtD[fD[k][2][1]-1][0], 1.0f-vtD[fD[k][2][1]-1][1]);
		}
		GX_End();
	}

	//O
	if(cnt>=34){
		if(alphaN[7]<255.0f && notfadeout) alphaN[7]+=50.0f;
		if(rotxN[7]<150.0f) rotxN[7]+=rotSpeed;

		GRRLIB_ObjectView(posxN[7],posyN,poszN[7],rotxN[7],rotyN,rotzN,sN,sN,sN);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 480);
		for (k=0;k<160;k++){
			GX_Position3f32(vO[fO[k][0][0]-1][0], vO[fO[k][0][0]-1][1], vO[fO[k][0][0]-1][2]);
			GX_Normal3f32(vnO[fO[k][0][2]-1][0], vnO[fO[k][0][2]-1][1], vnO[fO[k][0][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[7]);
			GX_TexCoord2f32(vtO[fO[k][0][1]-1][0], 1.0f-vtO[fO[k][0][1]-1][1]);
			GX_Position3f32(vO[fO[k][1][0]-1][0], vO[fO[k][1][0]-1][1], vO[fO[k][1][0]-1][2]);
			GX_Normal3f32(vnO[fO[k][1][2]-1][0], vnO[fO[k][1][2]-1][1], vnO[fO[k][1][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[7]);
			GX_TexCoord2f32(vtO[fO[k][1][1]-1][0], 1.0f-vtO[fO[k][1][1]-1][1]);
			GX_Position3f32(vO[fO[k][2][0]-1][0], vO[fO[k][2][0]-1][1], vO[fO[k][2][0]-1][2]);
			GX_Normal3f32(vnO[fO[k][2][2]-1][0], vnO[fO[k][2][2]-1][1], vnO[fO[k][2][2]-1][2]);
			GX_Color4u8(colorN,colorN,colorN,alphaN[7]);
			GX_TexCoord2f32(vtO[fO[k][2][1]-1][0], 1.0f-vtO[fO[k][2][1]-1][1]);
		}
		GX_End();
	}
}

void dispGamecube(GXTexObj* gc_tex, BTIFile* bti, f32 posyGC[], f32 scaleGCy, f32 tv3, f32* alphaC, f32* alpha, f32 alphaT[], u8* cnt){
	u8 k;
	if(*alpha!=255.0f && *cnt<44) *alpha=255.0f;
	switch (*cnt){
		case 0:
			camY-=6.4f;
			ty-=1.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFF00);
			break;
		case 1:
			camY-=20.0f;
			ty-=2.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[0]);
			break;
		case 2:
			camY-=64.0f;
			ty-=4.0f;
			posyGC[0]+=10.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[1]);
			break;
		case 3:
			camY-=50.0f;
			ty-=4.0f;
			posyGC[0]+=5.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[10]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[2]);
			break;
		case 4:
			camY-=44.8f;
			ty-=3.0f;
			posyGC[0]+=5.0f*tv3;
			posyGC[1]+=10.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[11]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			break;
		case 5:
			camY-=44.8f;
			ty-=3.0f;
			posyGC[0]+=5.0f*tv3;
			posyGC[1]+=5.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[10]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[11]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[4]);
			break;
		case 6:
			camY-=6.4f;
			ty-=2.0f;
			posyGC[0]+=5.0f*tv3;
			posyGC[1]+=5.0f*tv3;
			posyGC[2]+=10.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[13]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[5]);
			break;
		case 7:
			camY-=6.4f;
			ty-=1.0f;
			posyGC[0]+=5.0f*tv3;
			posyGC[1]+=5.0f*tv3;
			posyGC[2]+=5.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[14]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[11]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[6]);
			break;
		case 8:
			camY-=6.4f;
			ty-=1.0f;
			posyGC[0]+=4.0f*tv3;
			posyGC[1]+=5.0f*tv3;
			posyGC[2]+=5.0f*tv3;
			posyGC[3]+=10.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[10]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[0]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[1]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[7]);
			break;
		case 9:
			camY-=6.4f;
			ty-=1.0f;
			posyGC[0]+=4.0f*tv3;
			posyGC[1]+=5.0f*tv3;
			posyGC[2]+=5.0f*tv3;
			posyGC[3]+=5.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[16]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[13]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[1]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[8]);
			break;
		case 10:
			posyGC[0]+=4.0f*tv3;
			posyGC[1]+=4.0f*tv3;
			posyGC[2]+=5.0f*tv3;
			posyGC[3]+=5.0f*tv3;
			posyGC[4]+=10.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[17]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[11]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[13]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[1]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 11:
			camY+=3.2f;
			ty+=1.0f;
			posyGC[0]+=4.0f*tv3;
			posyGC[1]+=4.0f*tv3;
			posyGC[2]+=5.0f*tv3;
			posyGC[3]+=5.0f*tv3;
			posyGC[4]+=5.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[10]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[11]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[2]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 12:
			camY+=6.4f;
			ty+=1.0f;
			posyGC[0]+=4.0f*tv3;
			posyGC[1]+=4.0f*tv3;
			posyGC[2]+=4.0f*tv3;
			posyGC[3]+=5.0f*tv3;
			posyGC[4]+=5.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 13:
			camY+=12.8f;
			ty+=1.0f;
			posyGC[0]+=3.0f*tv3;
			posyGC[1]+=4.0f*tv3;
			posyGC[2]+=4.0f*tv3;
			posyGC[3]+=5.0f*tv3;
			posyGC[4]+=5.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 14:
			camY+=13.2f;
			ty+=1.0f;
			posyGC[0]+=3.0f*tv3;
			posyGC[1]+=4.0f*tv3;
			posyGC[2]+=4.0f*tv3;
			posyGC[3]+=4.0f*tv3;
			posyGC[4]+=5.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 15: 
			camY+=22.4f;
			ty+=1.0f;
			posyGC[0]+=2.0f*tv3;
			posyGC[1]+=3.0f*tv3;
			posyGC[2]+=4.0f*tv3;
			posyGC[3]+=4.0f*tv3;
			posyGC[4]+=5.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 16:
			camY+=22.4f;
			ty+=1.0f;
			posyGC[0]+=2.0f*tv3;
			posyGC[1]+=3.0f*tv3;
			posyGC[2]+=4.0f*tv3;
			posyGC[3]+=4.0f*tv3;
			posyGC[4]+=4.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 17:
			camY+=22.4f;
			ty+=1.0f;
			posyGC[0]+=2.0f*tv3;
			posyGC[1]+=2.0f*tv3;
			posyGC[2]+=3.0f*tv3;
			posyGC[3]+=4.0f*tv3;
			posyGC[4]+=4.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 18:
			camY+=13.8f;
			ty+=1.0f;
			posyGC[0]+=1.0f*tv3;
			posyGC[1]+=2.0f*tv3;
			posyGC[2]+=3.0f*tv3;
			posyGC[3]+=4.0f*tv3;
			posyGC[4]+=4.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 19:
			camY+=13.8f;
			ty+=1.0f;
			posyGC[0]+=1.0f*tv3;
			posyGC[1]+=2.0f*tv3;
			posyGC[2]+=2.0f*tv3;
			posyGC[3]+=3.0f*tv3;
			posyGC[4]+=4.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 20:
			camY+=13.8f;
			ty+=1.0f;
			posyGC[0]+=1.0f*tv3;
			posyGC[1]+=1.0f*tv3;
			posyGC[2]+=2.0f*tv3;
			posyGC[3]+=3.0f*tv3;
			posyGC[4]+=4.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 21:
			camY+=13.8f;
			ty+=1.0f;
			posyGC[1]+=1.0f*tv3;
			posyGC[2]+=2.0f*tv3;
			posyGC[3]+=2.0f*tv3;
			posyGC[4]+=3.0f*tv3;
			scaleGCy+=0.015f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 22:
			camY+=13.8f;
			ty+=1.0f;
			posyGC[1]+=1.0f*tv3;
			posyGC[2]+=1.0f*tv3;
			posyGC[3]+=2.0f*tv3;
			posyGC[4]+=3.0f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 23:
			camY+=12.8f;
			ty+=1.0f;
			posyGC[2]+=1.0f*tv3;
			posyGC[3]+=2.0f*tv3;
			posyGC[4]+=2.0f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 24:
			camY+=12.8f;
			ty+=1.0f;
			posyGC[2]+=1.0f*tv3;
			posyGC[3]+=1.0f*tv3;
			posyGC[4]+=2.0f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 25:
			camY+=12.8f;
			ty+=1.0f;
			posyGC[3]+=1.0f*tv3;
			posyGC[4]+=2.0f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 26:
			camY+=12.8f;
			ty+=1.0f;
			posyGC[0]-=0.5f*tv3;
			posyGC[3]+=1.0f*tv3;
			posyGC[4]+=1.0f*tv3;
			scaleGCy-=0.01875f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 27:
			camY+=6.5f;
			ty+=1.0f;
			posyGC[0]-=0.5f*tv3;
			posyGC[4]+=1.0f*tv3;
			scaleGCy-=0.01875f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 28:
			camY+=6.4f;
			ty+=1.0f;
			posyGC[0]-=0.5f*tv3;
			posyGC[1]-=0.5f*tv3;
			posyGC[4]+=1.0f*tv3;
			scaleGCy-=0.01875f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			alphaL+=255.0f/16.0f;
			(*alpha)-=255.0f/16.0f;
			(*alphaC)-=255.0f/16.0f;
			for(k=0;k<15;k++) alphaT[k]-=255.0f/16.0f;
			break;
		case 29:
			camY+=6.4f;
			ty+=0.5f;
			posyGC[0]-=0.5f*tv3;
			posyGC[1]-=0.5f*tv3;
			scaleGCy-=0.01875f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			alphaL+=255.0f/16.0f;
			(*alpha)-=2*255.0f/16.0f;
			(*alphaC)-=255.0f/16.0f;
			for(k=0;k<15;k++) alphaT[k]-=255.0f/16.0f;
			break;
		case 30:
			camY+=6.4f;
			ty+=0.5f;
			posyGC[0]-=0.5f*tv3;
			posyGC[1]-=0.5f*tv3;
			posyGC[2]-=0.5f*tv3;
			scaleGCy-=0.01875f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			alphaL+=255.0f/16.0f;
			(*alpha)-=3*255.0f/16.0f;
			(*alphaC)-=255.0f/16.0f;
			for(k=0;k<15;k++) alphaT[k]-=255.0f/16.0f;
			break;
		case 31:
			camY+=6.4f;
			ty+=0.5f;
			posyGC[0]-=0.5f*tv3;
			posyGC[1]-=0.5f*tv3;
			posyGC[2]-=0.5f*tv3;
			scaleGCy-=0.01875f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			alphaL+=255.0f/16.0f;
			(*alpha)-=4*255.0f/16.0f;
			(*alphaC)-=255.0f/16.0f;
			for(k=0;k<15;k++) alphaT[k]-=255.0f/16.0f;
			break;
		case 32:
			camY+=6.4f;
			ty+=0.5f;
			posyGC[0]-=0.5f*tv3;
			posyGC[1]-=0.5f*tv3;
			posyGC[2]-=0.5f*tv3;
			posyGC[3]-=0.5f*tv3;
			posyGC[4]-=0.5f*tv3;
			scaleGCy-=0.01875f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			alphaL+=255.0f/16.0f;
			(*alpha)-=5*255.0f/16.0f;
			(*alphaC)-=255.0f/16.0f;
			for(k=0;k<15;k++) alphaT[k]-=255.0f/16.0f;
			break;
		case 33:
			camY+=2.0f;
			ty+=0.5f;
			posyGC[0]-=0.5f*tv3;
			posyGC[1]-=0.5f*tv3;
			posyGC[2]-=0.5f*tv3;
			posyGC[3]-=0.5f*tv3;
			posyGC[4]-=0.5f*tv3;
			scaleGCy-=0.01875f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			alphaL+=255.0f/16.0f;
			(*alpha)-=6*255.0f/16.0f;
			(*alphaC)-=255.0f/16.0f;
			for(k=0;k<15;k++) alphaT[k]-=255.0f/16.0f;
			break;
		case 34:
			camY+=1.0f;
			ty+=0.5f;
			posyGC[0]-=0.5f*tv3;
			posyGC[1]-=0.5f*tv3;
			posyGC[2]-=0.5f*tv3;
			posyGC[3]-=0.5f*tv3;
			posyGC[4]-=0.5f*tv3;
			scaleGCy-=0.01875f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			alphaL+=255.0f/16.0f;
			(*alpha)-=7*255.0f/16.0f;
			(*alphaC)-=255.0f/16.0f;
			for(k=0;k<15;k++) alphaT[k]-=255.0f/16.0f;
			break;
		case 35:
			camY+=1.0f;
			ty+=0.5f;
			posyGC[0]-=0.5f*tv3;
			posyGC[1]-=0.5f*tv3;
			posyGC[2]-=0.5f*tv3;
			posyGC[3]-=0.5f*tv3;
			posyGC[4]-=0.5f*tv3;
			scaleGCy-=0.01875f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			alphaL+=255.0f/16.0f;
			(*alpha)-=8*255.0f/16.0f;
			(*alphaC)-=255.0f/16.0f;
			for(k=0;k<15;k++) alphaT[k]-=255.0f/16.0f;
			break;
		case 36:
			camY+=0.5f;
			ty+=0.5f;
			posyGC[0]-=0.5f*tv3;
			posyGC[1]-=0.5f*tv3;
			posyGC[2]-=0.5f*tv3;
			posyGC[3]-=0.5f*tv3;
			posyGC[4]-=0.5f*tv3;
			scaleGCy-=0.01875f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			alphaL+=255.0f/16.0f;
			(*alpha)-=9*255.0f/16.0f;
			(*alphaC)-=255.0f/16.0f;
			for(k=0;k<15;k++) alphaT[k]-=255.0f/16.0f;
			break;
		case 37:
			camY+=0.5f;
			ty+=0.5f;
			posyGC[0]-=0.5f*tv3;
			posyGC[1]-=0.5f*tv3;
			posyGC[2]-=0.5f*tv3;
			posyGC[3]-=0.5f*tv3;
			posyGC[4]-=0.5f*tv3;
			scaleGCy-=0.01875f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			alphaL+=255.0f/16.0f;
			(*alpha)-=10*255.0f/16.0f;
			(*alphaC)-=255.0f/16.0f;
			for(k=0;k<15;k++) alphaT[k]-=255.0f/16.0f;
			break;
		case 38:
			camY+=0.5f;
			ty+=0.5f;
			posyGC[0]-=0.5f*tv3;
			posyGC[1]-=0.5f*tv3;
			posyGC[2]-=0.5f*tv3;
			posyGC[3]-=0.5f*tv3;
			posyGC[4]-=0.5f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			alphaL+=255.0f/16.0f;
			(*alpha)-=11*255.0f/16.0f;
			(*alphaC)-=255.0f/16.0f;
			for(k=0;k<15;k++) alphaT[k]-=255.0f/16.0f;
			break;
		case 39:
			camY+=0.5f;
			ty+=0.5f;
			posyGC[0]-=0.5f*tv3;
			posyGC[1]-=0.5f*tv3;
			posyGC[2]-=0.5f*tv3;
			posyGC[3]-=0.5f*tv3;
			posyGC[4]-=0.5f*tv3;
			scaleGCy-=0.01875f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			alphaL+=255.0f/16.0f;
			(*alpha)-=12*255.0f/16.0f;
			(*alphaC)-=255.0f/16.0f;
			for(k=0;k<15;k++) alphaT[k]-=255.0f/16.0f;
			break;
		case 40:
			camY+=0.5f;
			ty+=0.5f;
			posyGC[0]-=0.5f*tv3;
			posyGC[1]-=0.5f*tv3;
			posyGC[2]-=0.5f*tv3;
			posyGC[3]-=0.5f*tv3;
			posyGC[4]-=0.5f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			alphaL+=255.0f/16.0f;
			(*alpha)-=13*255.0f/16.0f;
			(*alphaC)-=255.0f/16.0f;
			for(k=0;k<15;k++) alphaT[k]-=255.0f/16.0f;
			break;
		case 41:
			camY-=1.0f;
			ty-=0.25f;
			posyGC[0]-=0.5f*tv3;
			posyGC[1]-=0.5f*tv3;
			posyGC[2]-=0.5f*tv3;
			posyGC[3]-=0.5f*tv3;
			posyGC[4]-=0.5f*tv3;
			scaleGCy-=0.01875f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			alphaL+=255.0f/16.0f;
			(*alpha)-=14*255.0f/16.0f;
			(*alphaC)-=255.0f/16.0f;
			for(k=0;k<15;k++) alphaT[k]-=255.0f/16.0f;
			break;
		case 42:
			camY-=1.0f;
			ty-=0.25f;
			posyGC[1]-=0.5f*tv3;
			posyGC[2]-=0.5f*tv3;
			posyGC[3]-=0.5f*tv3;
			posyGC[4]-=0.5f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			alphaL+=255.0f/16.0f;
			(*alpha)-=15*255.0f/16.0f;
			(*alphaC)-=255.0f/16.0f;
			for(k=0;k<15;k++) alphaT[k]-=255.0f/16.0f;
			break;
		case 43:
			camY-=1.0f;
			ty-=0.1f;
			posyGC[1]-=0.5f*tv3;
			posyGC[2]-=0.5f*tv3;
			posyGC[3]-=0.5f*tv3;
			posyGC[4]-=0.5f*tv3;
			scaleGCy-=0.01875f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			alphaL+=255.0f/16.0f;
			(*alpha)=0;
			(*alphaC)-=255.0f/16.0f;
			for(k=0;k<15;k++) alphaT[k]=0.0f;
			break;
		case 44:
			camY-=1.0f;
			ty-=0.1f;
			posyGC[2]-=0.5f*tv3;
			posyGC[3]-=0.5f*tv3;
			posyGC[4]-=0.5f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 45:
			camY-=0.8f;
			ty-=0.1f;
			posyGC[0]+=0.4f*tv3;
			posyGC[2]-=0.5f*tv3;
			posyGC[3]-=0.5f*tv3;
			posyGC[4]-=0.5f*tv3;
			scaleGCy-=0.01875f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 46:
			camY-=0.8f;
			ty-=0.08f;
			posyGC[0]+=0.3f*tv3;
			posyGC[3]-=0.5f*tv3;
			posyGC[4]-=0.5f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 47:
			camY-=0.8f;
			ty-=0.08f;
			posyGC[0]+=0.3f*tv3;
			posyGC[1]+=0.4f*tv3;
			posyGC[3]-=0.5f*tv3;
			posyGC[4]-=0.5f*tv3;
			scaleGCy-=0.01875f;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 48:
			camY-=0.6f;
			ty-=0.08f;
			posyGC[0]+=0.3f*tv3;
			posyGC[1]+=0.3f*tv3;
			posyGC[4]-=0.5f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 49:
			camY-=0.6f;
			ty-=0.07f;
			posyGC[0]+=0.2f*tv3;
			posyGC[1]+=0.3f*tv3;
			posyGC[2]+=0.4f*tv3;
			posyGC[4]-=0.5f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 50:
			camY-=0.6f;
			ty-=0.07f;
			posyGC[0]+=0.2f*tv3;
			posyGC[1]+=0.3f*tv3;
			posyGC[2]+=0.3f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 51:
			camY-=0.5f;
			ty-=0.07f;
			posyGC[0]+=0.2f*tv3;
			posyGC[1]+=0.2f*tv3;
			posyGC[2]+=0.3f*tv3;
			posyGC[3]+=0.4f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 52:
			camY-=0.5f;
			ty-=0.06f;
			posyGC[0]+=0.2f*tv3;
			posyGC[1]+=0.2f*tv3;
			posyGC[2]+=0.3f*tv3;
			posyGC[3]+=0.3f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 53:
			camY-=0.5f;
			ty-=0.06f;
			posyGC[0]+=0.1f*tv3;
			posyGC[1]+=0.2f*tv3;
			posyGC[2]+=0.2f*tv3;
			posyGC[3]+=0.3f*tv3;
			posyGC[4]+=0.4f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 54:
			camY-=0.4f;
			ty-=0.06f;
			posyGC[0]+=0.1f*tv3;
			posyGC[1]+=0.2f*tv3;
			posyGC[2]+=0.2f*tv3;
			posyGC[3]+=0.3f*tv3;
			posyGC[4]+=0.3f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 55:
			camY-=0.4f;
			ty-=0.05f;
			posyGC[0]+=0.1f*tv3;
			posyGC[1]+=0.1f*tv3;
			posyGC[2]+=0.2f*tv3;
			posyGC[3]+=0.2f*tv3;
			posyGC[4]+=0.3f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 56:
			camY-=0.4;
			ty-=0.05f;
			posyGC[0]+=0.1f*tv3;
			posyGC[1]+=0.1f*tv3;
			posyGC[2]+=0.2f*tv3;
			posyGC[3]+=0.2f*tv3;
			posyGC[4]+=0.3f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 57:
			camY-=0.3f;
			ty-=0.05f;
			posyGC[0]+=0.05f*tv3;
			posyGC[1]+=0.1f*tv3;
			posyGC[2]+=0.1f*tv3;
			posyGC[3]+=0.2f*tv3;
			posyGC[4]+=0.2f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 58:
			camY-=0.3f;
			ty-=0.05f;
			posyGC[0]+=0.05f*tv3;
			posyGC[1]+=0.1f*tv3;
			posyGC[2]+=0.1f*tv3;
			posyGC[3]+=0.2f*tv3;
			posyGC[4]+=0.2f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 59:
			camY-=0.3f;
			ty-=0.05f;
			posyGC[0]+=0.05f*tv3;
			posyGC[1]+=0.05f*tv3;
			posyGC[2]+=0.1f*tv3;
			posyGC[3]+=0.1f*tv3;
			posyGC[4]+=0.2f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 60:
			camY-=0.1f;
			ty-=0.05f;
			posyGC[0]+=0.05f*tv3;
			posyGC[1]+=0.05f*tv3;
			posyGC[2]+=0.1f*tv3;
			posyGC[3]+=0.1f*tv3;
			posyGC[4]+=0.2f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 61:
			camY-=0.1f;
			ty-=0.05f;
			posyGC[0]+=0.05f*tv3;
			posyGC[1]+=0.05f*tv3;
			posyGC[2]+=0.05f*tv3;
			posyGC[3]+=0.1f*tv3;
			posyGC[4]+=0.1f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 62:
			camY-=0.1f;
			ty-=0.05f;
			posyGC[0]+=0.05f*tv3;
			posyGC[1]+=0.05f*tv3;
			posyGC[2]+=0.05f*tv3;
			posyGC[3]+=0.1f*tv3;
			posyGC[4]+=0.1f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 63:
			camY-=0.1f;
			ty-=0.05f;
			posyGC[0]+=0.05f*tv3;
			posyGC[1]+=0.05f*tv3;
			posyGC[2]+=0.05f*tv3;
			posyGC[3]+=0.05f*tv3;
			posyGC[4]+=0.1f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 64:
			camY-=0.1f;
			ty-=0.04f;
			posyGC[0]+=0.05f*tv3;
			posyGC[1]+=0.05f*tv3;
			posyGC[2]+=0.05f*tv3;
			posyGC[3]+=0.05f*tv3;
			posyGC[4]+=0.1f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 65:
			camY-=0.05f;
			ty-=0.04f;
			posyGC[0]+=0.05f*tv3;
			posyGC[1]+=0.05f*tv3;
			posyGC[2]+=0.05f*tv3;
			posyGC[3]+=0.05f*tv3;
			posyGC[4]+=0.05f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 66:
			camY-=0.05f;
			ty-=0.04f;
			posyGC[0]+=0.05f*tv3;
			posyGC[1]+=0.05f*tv3;
			posyGC[2]+=0.05f*tv3;
			posyGC[3]+=0.05f*tv3;
			posyGC[4]+=0.05f*tv3;
			BTI_DrawImg(posxGC,posyGC[4],gc_tex,bti,scaleGCx,scaleGCy,gcCol[12]);
			BTI_DrawImg(posxGC,posyGC[3],gc_tex,bti,scaleGCx,scaleGCy,gcCol[18]);
			BTI_DrawImg(posxGC,posyGC[2],gc_tex,bti,scaleGCx,scaleGCy,gcCol[15]);
			BTI_DrawImg(posxGC,posyGC[1],gc_tex,bti,scaleGCx,scaleGCy,gcCol[3]);
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 67:
			camY=-200.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			notfadeout=0;
			break;
		case 68:
			for(k=0;k<8;k++) alphaN[k]-=20.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[9]);
			break;
		case 69:
			alphaL-=5.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[8]);
			break;
		case 70:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFFE6);
			break;
		case 71:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,gcCol[7]);
			break;
		case 72:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFFD2);
			break;
		case 73:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFFC8);
			break;
		case 74:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFFBE);
			break;
		case 75:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFFB4);
			break;
		case 76:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFFAA);
			break;
		case 77:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFFA0);
			break;
		case 78:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFF96);
			break;
		case 79:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFF8C);
			break;
		case 80:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFF82);
			break;
		case 81:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFF78);
			break;
		case 82:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFF6E);
			break;
		case 83:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFF64);
			break;
		case 84:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFF5A);
			break;
		case 85:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFF50);
			break;
		case 86:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFF46);
			break;
		case 87:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFF3C);
			break;
		case 88:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFF32);
			break;
		case 89:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFF28);
			break;
		case 90:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFF1E);
			break;
		case 91:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFF14);
			break;
		case 92:
			alphaL-=10.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFF0A);
			break;
		case 93:
			alphaL-=10.0f;
			for(k=0;k<8;k++) alphaN[k]-=5.0f;
			BTI_DrawImg(posxGC,posyGC[0],gc_tex,bti,scaleGCx,scaleGCy,0xFFFFFF00);
			returnToMenu=true;
			break;
	}
	if(*cnt<93) (*cnt)++;
}
