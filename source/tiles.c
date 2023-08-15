#include "globals.h"
#include "models.h"
#include "tiles.h"

f32 posxT=-55.0f;
f32 posyT=-144.0f;
f32 poszT=-55.0f;
f32 rotxT=0.0f;
f32 rotyT=0.0f;
f32 rotzT=0.0f;
f32 deltaT=53.9f;
u8 tr=99;			// Indigo Tile DEFAULT COLOR
u8 tg=79;
u8 tb=188;

void dispTiles(GXTexObj* tile_tex, u8 stage, f32 alphaT[]){
	u8 k;
	
	if(stage>=3){
		GX_ClearVtxDesc();
		GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
		GX_SetVtxDesc(GX_VA_NRM, GX_DIRECT);
		GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
		GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_TEX0, GX_IDENTITY);
		GX_SetTevOp(GX_TEVSTAGE0,GX_MODULATE);
		GX_LoadTexObj(tile_tex, GX_TEXMAP0);
		GRRLIB_ObjectView(posxT,posyT,poszT+deltaT,rotxT,rotyT,rotzT,1.0f,1.0f,1.0f);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		for (k=0;k<2;k++){
			GX_Position3f32(vTile[fTile[k][0][0]-1][0], vTile[fTile[k][0][0]-1][1], vTile[fTile[k][0][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][0][2]-1][0], vnTile[fTile[k][0][2]-1][1], vnTile[fTile[k][0][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[0]);
			GX_TexCoord2f32(vtTile[fTile[k][0][1]-1][0], 1.0f-vtTile[fTile[k][0][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][1][0]-1][0], vTile[fTile[k][1][0]-1][1], vTile[fTile[k][1][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][1][2]-1][0], vnTile[fTile[k][1][2]-1][1], vnTile[fTile[k][1][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[0]);
			GX_TexCoord2f32(vtTile[fTile[k][1][1]-1][0], 1.0f-vtTile[fTile[k][1][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][2][0]-1][0], vTile[fTile[k][2][0]-1][1], vTile[fTile[k][2][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][2][2]-1][0], vnTile[fTile[k][2][2]-1][1], vnTile[fTile[k][2][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[0]);
			GX_TexCoord2f32(vtTile[fTile[k][2][1]-1][0], 1.0f-vtTile[fTile[k][2][1]-1][1]);
		}
		GX_End();
	}

	if(stage>=4){
		GRRLIB_ObjectView(posxT,posyT,poszT,rotxT,rotyT,rotzT,1.0f,1.0f,1.0f);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		for (k=0;k<2;k++){
			GX_Position3f32(vTile[fTile[k][0][0]-1][0], vTile[fTile[k][0][0]-1][1], vTile[fTile[k][0][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][0][2]-1][0], vnTile[fTile[k][0][2]-1][1], vnTile[fTile[k][0][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[1]);
			GX_TexCoord2f32(vtTile[fTile[k][0][1]-1][0], 1.0f-vtTile[fTile[k][0][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][1][0]-1][0], vTile[fTile[k][1][0]-1][1], vTile[fTile[k][1][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][1][2]-1][0], vnTile[fTile[k][1][2]-1][1], vnTile[fTile[k][1][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[1]);
			GX_TexCoord2f32(vtTile[fTile[k][1][1]-1][0], 1.0f-vtTile[fTile[k][1][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][2][0]-1][0], vTile[fTile[k][2][0]-1][1], vTile[fTile[k][2][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][2][2]-1][0], vnTile[fTile[k][2][2]-1][1], vnTile[fTile[k][2][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[1]);
			GX_TexCoord2f32(vtTile[fTile[k][2][1]-1][0], 1.0f-vtTile[fTile[k][2][1]-1][1]);
		}
		GX_End();
	}

	if(stage>=5){
		GRRLIB_ObjectView(posxT+deltaT,posyT,poszT,rotxT,rotyT,rotzT,1.0f,1.0f,1.0f);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		for (k=0;k<2;k++){
			GX_Position3f32(vTile[fTile[k][0][0]-1][0], vTile[fTile[k][0][0]-1][1], vTile[fTile[k][0][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][0][2]-1][0], vnTile[fTile[k][0][2]-1][1], vnTile[fTile[k][0][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[2]);
			GX_TexCoord2f32(vtTile[fTile[k][0][1]-1][0], 1.0f-vtTile[fTile[k][0][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][1][0]-1][0], vTile[fTile[k][1][0]-1][1], vTile[fTile[k][1][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][1][2]-1][0], vnTile[fTile[k][1][2]-1][1], vnTile[fTile[k][1][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[2]);
			GX_TexCoord2f32(vtTile[fTile[k][1][1]-1][0], 1.0f-vtTile[fTile[k][1][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][2][0]-1][0], vTile[fTile[k][2][0]-1][1], vTile[fTile[k][2][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][2][2]-1][0], vnTile[fTile[k][2][2]-1][1], vnTile[fTile[k][2][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[2]);
			GX_TexCoord2f32(vtTile[fTile[k][2][1]-1][0], 1.0f-vtTile[fTile[k][2][1]-1][1]);
		}
		GX_End();
	}

	if(stage>=6){
		GRRLIB_ObjectView(posxT+2*deltaT,posyT,poszT,rotxT,rotyT,rotzT,1.0f,1.0f,1.0f);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		for (k=0;k<2;k++){
			GX_Position3f32(vTile[fTile[k][0][0]-1][0], vTile[fTile[k][0][0]-1][1], vTile[fTile[k][0][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][0][2]-1][0], vnTile[fTile[k][0][2]-1][1], vnTile[fTile[k][0][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[3]);
			GX_TexCoord2f32(vtTile[fTile[k][0][1]-1][0], 1.0f-vtTile[fTile[k][0][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][1][0]-1][0], vTile[fTile[k][1][0]-1][1], vTile[fTile[k][1][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][1][2]-1][0], vnTile[fTile[k][1][2]-1][1], vnTile[fTile[k][1][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[3]);
			GX_TexCoord2f32(vtTile[fTile[k][1][1]-1][0], 1.0f-vtTile[fTile[k][1][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][2][0]-1][0], vTile[fTile[k][2][0]-1][1], vTile[fTile[k][2][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][2][2]-1][0], vnTile[fTile[k][2][2]-1][1], vnTile[fTile[k][2][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[3]);
			GX_TexCoord2f32(vtTile[fTile[k][2][1]-1][0], 1.0f-vtTile[fTile[k][2][1]-1][1]);
		}
		GX_End();
	}

	if(stage>=7){
		GRRLIB_ObjectView(posxT+3*deltaT-25.5f,-118.4f,-53.8f,rotxT,rotyT,rotzT+90.0f,1.0f,1.0f,1.0f);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		for (k=0;k<2;k++){
			GX_Position3f32(vTile[fTile[k][0][0]-1][0], vTile[fTile[k][0][0]-1][1], vTile[fTile[k][0][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][0][2]-1][0], vnTile[fTile[k][0][2]-1][1], vnTile[fTile[k][0][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[4]);
			GX_TexCoord2f32(vtTile[fTile[k][0][1]-1][0], 1.0f-vtTile[fTile[k][0][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][1][0]-1][0], vTile[fTile[k][1][0]-1][1], vTile[fTile[k][1][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][1][2]-1][0], vnTile[fTile[k][1][2]-1][1], vnTile[fTile[k][1][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[4]);
			GX_TexCoord2f32(vtTile[fTile[k][1][1]-1][0], 1.0f-vtTile[fTile[k][1][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][2][0]-1][0], vTile[fTile[k][2][0]-1][1], vTile[fTile[k][2][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][2][2]-1][0], vnTile[fTile[k][2][2]-1][1], vnTile[fTile[k][2][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[4]);
			GX_TexCoord2f32(vtTile[fTile[k][2][1]-1][0], 1.0f-vtTile[fTile[k][2][1]-1][1]);
		}
		GX_End();
	}

	if(stage>=8){
		GRRLIB_ObjectView(posxT+3*deltaT-25.5f,-118.4f+deltaT,-53.8f,rotxT,rotyT,rotzT+90.0f,1.0f,1.0f,1.0f);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		for (k=0;k<2;k++){
			GX_Position3f32(vTile[fTile[k][0][0]-1][0], vTile[fTile[k][0][0]-1][1], vTile[fTile[k][0][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][0][2]-1][0], vnTile[fTile[k][0][2]-1][1], vnTile[fTile[k][0][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[5]);
			GX_TexCoord2f32(vtTile[fTile[k][0][1]-1][0], 1.0f-vtTile[fTile[k][0][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][1][0]-1][0], vTile[fTile[k][1][0]-1][1], vTile[fTile[k][1][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][1][2]-1][0], vnTile[fTile[k][1][2]-1][1], vnTile[fTile[k][1][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[5]);
			GX_TexCoord2f32(vtTile[fTile[k][1][1]-1][0], 1.0f-vtTile[fTile[k][1][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][2][0]-1][0], vTile[fTile[k][2][0]-1][1], vTile[fTile[k][2][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][2][2]-1][0], vnTile[fTile[k][2][2]-1][1], vnTile[fTile[k][2][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[5]);
			GX_TexCoord2f32(vtTile[fTile[k][2][1]-1][0], 1.0f-vtTile[fTile[k][2][1]-1][1]);
		}
		GX_End();
	}

	if(stage>=9){
		GRRLIB_ObjectView(posxT+3*deltaT-25.5f,-118.4f+2*deltaT,-53.8f,rotxT,rotyT,rotzT+90.0f,1.0f,1.0f,1.0f);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		for (k=0;k<2;k++){
			GX_Position3f32(vTile[fTile[k][0][0]-1][0], vTile[fTile[k][0][0]-1][1], vTile[fTile[k][0][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][0][2]-1][0], vnTile[fTile[k][0][2]-1][1], vnTile[fTile[k][0][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[6]);
			GX_TexCoord2f32(vtTile[fTile[k][0][1]-1][0], 1.0f-vtTile[fTile[k][0][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][1][0]-1][0], vTile[fTile[k][1][0]-1][1], vTile[fTile[k][1][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][1][2]-1][0], vnTile[fTile[k][1][2]-1][1], vnTile[fTile[k][1][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[6]);
			GX_TexCoord2f32(vtTile[fTile[k][1][1]-1][0], 1.0f-vtTile[fTile[k][1][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][2][0]-1][0], vTile[fTile[k][2][0]-1][1], vTile[fTile[k][2][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][2][2]-1][0], vnTile[fTile[k][2][2]-1][1], vnTile[fTile[k][2][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[6]);
			GX_TexCoord2f32(vtTile[fTile[k][2][1]-1][0], 1.0f-vtTile[fTile[k][2][1]-1][1]);
		}
		GX_End();
	}

	if(stage>=10){
		GRRLIB_ObjectView(posxT+3*deltaT-25.5f,-118.4f+2*deltaT,-53.8f+deltaT,rotxT,rotyT,rotzT+90.0f,1.0f,1.0f,1.0f);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		for (k=0;k<2;k++){
			GX_Position3f32(vTile[fTile[k][0][0]-1][0], vTile[fTile[k][0][0]-1][1], vTile[fTile[k][0][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][0][2]-1][0], vnTile[fTile[k][0][2]-1][1], vnTile[fTile[k][0][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[7]);
			GX_TexCoord2f32(vtTile[fTile[k][0][1]-1][0], 1.0f-vtTile[fTile[k][0][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][1][0]-1][0], vTile[fTile[k][1][0]-1][1], vTile[fTile[k][1][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][1][2]-1][0], vnTile[fTile[k][1][2]-1][1], vnTile[fTile[k][1][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[7]);
			GX_TexCoord2f32(vtTile[fTile[k][1][1]-1][0], 1.0f-vtTile[fTile[k][1][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][2][0]-1][0], vTile[fTile[k][2][0]-1][1], vTile[fTile[k][2][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][2][2]-1][0], vnTile[fTile[k][2][2]-1][1], vnTile[fTile[k][2][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[7]);
			GX_TexCoord2f32(vtTile[fTile[k][2][1]-1][0], 1.0f-vtTile[fTile[k][2][1]-1][1]);
		}
		GX_End();
	}

	if(stage>=11){
		GRRLIB_ObjectView(posxT+3*deltaT-25.5f,-118.4f+2*deltaT,-53.8f+2*deltaT,rotxT,rotyT,rotzT+90.0f,1.0f,1.0f,1.0f);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		for (k=0;k<2;k++){
			GX_Position3f32(vTile[fTile[k][0][0]-1][0], vTile[fTile[k][0][0]-1][1], vTile[fTile[k][0][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][0][2]-1][0], vnTile[fTile[k][0][2]-1][1], vnTile[fTile[k][0][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[8]);
			GX_TexCoord2f32(vtTile[fTile[k][0][1]-1][0], 1.0f-vtTile[fTile[k][0][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][1][0]-1][0], vTile[fTile[k][1][0]-1][1], vTile[fTile[k][1][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][1][2]-1][0], vnTile[fTile[k][1][2]-1][1], vnTile[fTile[k][1][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[8]);
			GX_TexCoord2f32(vtTile[fTile[k][1][1]-1][0], 1.0f-vtTile[fTile[k][1][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][2][0]-1][0], vTile[fTile[k][2][0]-1][1], vTile[fTile[k][2][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][2][2]-1][0], vnTile[fTile[k][2][2]-1][1], vnTile[fTile[k][2][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[8]);
			GX_TexCoord2f32(vtTile[fTile[k][2][1]-1][0], 1.0f-vtTile[fTile[k][2][1]-1][1]);
		}
		GX_End();
	}

	if(stage>=12){
		GRRLIB_ObjectView(posxT+3*deltaT-54.0f,-117.1f+2*deltaT,-27.7f+2*deltaT,rotxT,-90.0f,rotzT+90.0f,1.0f,1.0f,1.0f);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		for (k=0;k<2;k++){
			GX_Position3f32(vTile[fTile[k][0][0]-1][0], vTile[fTile[k][0][0]-1][1], vTile[fTile[k][0][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][0][2]-1][0], vnTile[fTile[k][0][2]-1][1], vnTile[fTile[k][0][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[9]);
			GX_TexCoord2f32(vtTile[fTile[k][0][1]-1][0], 1.0f-vtTile[fTile[k][0][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][1][0]-1][0], vTile[fTile[k][1][0]-1][1], vTile[fTile[k][1][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][1][2]-1][0], vnTile[fTile[k][1][2]-1][1], vnTile[fTile[k][1][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[9]);
			GX_TexCoord2f32(vtTile[fTile[k][1][1]-1][0], 1.0f-vtTile[fTile[k][1][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][2][0]-1][0], vTile[fTile[k][2][0]-1][1], vTile[fTile[k][2][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][2][2]-1][0], vnTile[fTile[k][2][2]-1][1], vnTile[fTile[k][2][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[9]);
			GX_TexCoord2f32(vtTile[fTile[k][2][1]-1][0], 1.0f-vtTile[fTile[k][2][1]-1][1]);
		}
		GX_End();
	}
			
	if(stage>=13){
		GRRLIB_ObjectView(posxT+2*deltaT-54.0f,-117.1f+2*deltaT,-27.7f+2*deltaT,rotxT,-90.0f,rotzT+90.0f,1.0f,1.0f,1.0f);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		for (k=0;k<2;k++){
			GX_Position3f32(vTile[fTile[k][0][0]-1][0], vTile[fTile[k][0][0]-1][1], vTile[fTile[k][0][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][0][2]-1][0], vnTile[fTile[k][0][2]-1][1], vnTile[fTile[k][0][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[10]);
			GX_TexCoord2f32(vtTile[fTile[k][0][1]-1][0], 1.0f-vtTile[fTile[k][0][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][1][0]-1][0], vTile[fTile[k][1][0]-1][1], vTile[fTile[k][1][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][1][2]-1][0], vnTile[fTile[k][1][2]-1][1], vnTile[fTile[k][1][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[10]);
			GX_TexCoord2f32(vtTile[fTile[k][1][1]-1][0], 1.0f-vtTile[fTile[k][1][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][2][0]-1][0], vTile[fTile[k][2][0]-1][1], vTile[fTile[k][2][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][2][2]-1][0], vnTile[fTile[k][2][2]-1][1], vnTile[fTile[k][2][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[10]);
			GX_TexCoord2f32(vtTile[fTile[k][2][1]-1][0], 1.0f-vtTile[fTile[k][2][1]-1][1]);
		}
		GX_End();
	}

	if(stage>=14){
		GRRLIB_ObjectView(posxT+deltaT-54.0f,-117.1f+2*deltaT,-27.7f+2*deltaT,rotxT,-90.0f,rotzT+90.0f,1.0f,1.0f,1.0f);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		for (k=0;k<2;k++){
			GX_Position3f32(vTile[fTile[k][0][0]-1][0], vTile[fTile[k][0][0]-1][1], vTile[fTile[k][0][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][0][2]-1][0], vnTile[fTile[k][0][2]-1][1], vnTile[fTile[k][0][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[11]);
			GX_TexCoord2f32(vtTile[fTile[k][0][1]-1][0], 1.0f-vtTile[fTile[k][0][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][1][0]-1][0], vTile[fTile[k][1][0]-1][1], vTile[fTile[k][1][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][1][2]-1][0], vnTile[fTile[k][1][2]-1][1], vnTile[fTile[k][1][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[11]);
			GX_TexCoord2f32(vtTile[fTile[k][1][1]-1][0], 1.0f-vtTile[fTile[k][1][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][2][0]-1][0], vTile[fTile[k][2][0]-1][1], vTile[fTile[k][2][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][2][2]-1][0], vnTile[fTile[k][2][2]-1][1], vnTile[fTile[k][2][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[11]);
			GX_TexCoord2f32(vtTile[fTile[k][2][1]-1][0], 1.0f-vtTile[fTile[k][2][1]-1][1]);
		}
		GX_End();
	}
	
	if(stage>=15){
		GRRLIB_ObjectView(posxT+deltaT-54.0f,-117.1f+deltaT,-27.7f+2*deltaT,rotxT,-90.0f,rotzT+90.0f,1.0f,1.0f,1.0f);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		for (k=0;k<2;k++){
			GX_Position3f32(vTile[fTile[k][0][0]-1][0], vTile[fTile[k][0][0]-1][1], vTile[fTile[k][0][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][0][2]-1][0], vnTile[fTile[k][0][2]-1][1], vnTile[fTile[k][0][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[12]);
			GX_TexCoord2f32(vtTile[fTile[k][0][1]-1][0], 1.0f-vtTile[fTile[k][0][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][1][0]-1][0], vTile[fTile[k][1][0]-1][1], vTile[fTile[k][1][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][1][2]-1][0], vnTile[fTile[k][1][2]-1][1], vnTile[fTile[k][1][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[12]);
			GX_TexCoord2f32(vtTile[fTile[k][1][1]-1][0], 1.0f-vtTile[fTile[k][1][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][2][0]-1][0], vTile[fTile[k][2][0]-1][1], vTile[fTile[k][2][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][2][2]-1][0], vnTile[fTile[k][2][2]-1][1], vnTile[fTile[k][2][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[12]);
			GX_TexCoord2f32(vtTile[fTile[k][2][1]-1][0], 1.0f-vtTile[fTile[k][2][1]-1][1]);
		}
		GX_End();
	}

	if(stage>=16){
		GRRLIB_ObjectView(posxT+deltaT-54.0f,-117.1f,-27.7f+2*deltaT,rotxT,-90.0f,rotzT+90.0f,1.0f,1.0f,1.0f);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		for (k=0;k<2;k++){
			GX_Position3f32(vTile[fTile[k][0][0]-1][0], vTile[fTile[k][0][0]-1][1], vTile[fTile[k][0][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][0][2]-1][0], vnTile[fTile[k][0][2]-1][1], vnTile[fTile[k][0][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[13]);
			GX_TexCoord2f32(vtTile[fTile[k][0][1]-1][0], 1.0f-vtTile[fTile[k][0][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][1][0]-1][0], vTile[fTile[k][1][0]-1][1], vTile[fTile[k][1][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][1][2]-1][0], vnTile[fTile[k][1][2]-1][1], vnTile[fTile[k][1][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[13]);
			GX_TexCoord2f32(vtTile[fTile[k][1][1]-1][0], 1.0f-vtTile[fTile[k][1][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][2][0]-1][0], vTile[fTile[k][2][0]-1][1], vTile[fTile[k][2][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][2][2]-1][0], vnTile[fTile[k][2][2]-1][1], vnTile[fTile[k][2][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[13]);
			GX_TexCoord2f32(vtTile[fTile[k][2][1]-1][0], 1.0f-vtTile[fTile[k][2][1]-1][1]);
		}
		GX_End();
	}

	if(stage>=17){
		GRRLIB_ObjectView(posxT+2*deltaT-54.0f,-117.1f,-27.7f+2*deltaT,rotxT,-90.0f,rotzT+90.0f,1.0f,1.0f,1.0f);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		for (k=0;k<2;k++){
			GX_Position3f32(vTile[fTile[k][0][0]-1][0], vTile[fTile[k][0][0]-1][1], vTile[fTile[k][0][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][0][2]-1][0], vnTile[fTile[k][0][2]-1][1], vnTile[fTile[k][0][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[14]);
			GX_TexCoord2f32(vtTile[fTile[k][0][1]-1][0], 1.0f-vtTile[fTile[k][0][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][1][0]-1][0], vTile[fTile[k][1][0]-1][1], vTile[fTile[k][1][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][1][2]-1][0], vnTile[fTile[k][1][2]-1][1], vnTile[fTile[k][1][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[14]);
			GX_TexCoord2f32(vtTile[fTile[k][1][1]-1][0], 1.0f-vtTile[fTile[k][1][1]-1][1]);
			GX_Position3f32(vTile[fTile[k][2][0]-1][0], vTile[fTile[k][2][0]-1][1], vTile[fTile[k][2][0]-1][2]);
			GX_Normal3f32(vnTile[fTile[k][2][2]-1][0], vnTile[fTile[k][2][2]-1][1], vnTile[fTile[k][2][2]-1][2]);
			GX_Color4u8(tr,tg,tb,alphaT[14]);
			GX_TexCoord2f32(vtTile[fTile[k][2][1]-1][0], 1.0f-vtTile[fTile[k][2][1]-1][1]);
		}
		GX_End();
	}
}