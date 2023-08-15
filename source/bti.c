// Credits to Aurelio (https://github.com/Aurelio92) for the BTI implementation

#include "bti.h"

extern GRRLIB_drawSettings GRRLIB_Settings;
extern Mtx                 GXmodelView2D;

GRRLIB_texImg*  BTI_GRRLoad (BTIFile *in_bti) {
    GRRLIB_texImg *my_texture = calloc(1, sizeof(GRRLIB_texImg));
	u32 dataSize;
	
    if (my_texture == NULL) {
        return NULL;
    }
	
	dataSize = BTI_GetTextureSize(in_bti->width, in_bti->height, in_bti->imageFormat);
	
	my_texture->data = &((u8*)in_bti)[in_bti->imageDataOffset];
	if (my_texture->data != NULL) {
		my_texture->w= (int) in_bti->width;
		my_texture->h= (int) in_bti->height;
		GRRLIB_SetHandle( my_texture, 0, 0 );
		DCFlushRange(my_texture->data, dataSize);
	}
    return my_texture;
}

void BTI_DrawImg (f32 xpos, f32 ypos, GXTexObj* tex, BTIFile* bti, f32 scaleX, f32 scaleY, u32 color) {
    Mtx       m1, mv;
  
    GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);

    GX_LoadTexObj(tex,      GX_TEXMAP0);
    GX_SetTevOp  (GX_TEVSTAGE0, GX_MODULATE);
    GX_SetVtxDesc(GX_VA_TEX0,   GX_DIRECT);

    guMtxIdentity  (m1);
    guMtxScaleApply(m1, m1, scaleX, scaleY, 1.0);

    const f32 width  = (f32) bti->width * 0.5;
    const f32 height = (f32) bti->height * 0.5;

	guMtxTransApply(m1, m1, xpos+width, ypos+height, 0);
	guMtxConcat(GXmodelView2D, m1, mv);

    GX_LoadPosMtxImm(mv, GX_PNMTX0);
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position3f32(-width, -height, 0);
        GX_Color1u32   (color);
        GX_TexCoord2f32(0, 0);

        GX_Position3f32(width, -height, 0);
        GX_Color1u32   (color);
        GX_TexCoord2f32(1, 0);

        GX_Position3f32(width, height, 0);
        GX_Color1u32   (color);
        GX_TexCoord2f32(1, 1);

        GX_Position3f32(-width, height, 0);
        GX_Color1u32   (color);
        GX_TexCoord2f32(0, 1);
    GX_End();
    GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);

    GX_SetTevOp  (GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetVtxDesc(GX_VA_TEX0,   GX_NONE);
}

u32 BTI_GetTextureSize(u32 width,u32 height,u32 fmt) {
    u32 size = 0;
 
    switch(fmt) {
            case GX_TF_I4:
            case GX_TF_CI4:
            case GX_TF_CMPR:
                size = ((width+7)>>3)*((height+7)>>3)*32;
                break;
            case GX_TF_I8:
            case GX_TF_IA4:
            case GX_TF_CI8:
                size = ((width+7)>>3)*((height+7)>>2)*32;
                break;
            case GX_TF_IA8:
            case GX_TF_CI14:
            case GX_TF_RGB565:
            case GX_TF_RGB5A3:
                size = ((width+3)>>2)*((height+3)>>2)*32;
                break;
            case GX_TF_RGBA8:
                size = ((width+3)>>2)*((height+3)>>2)*32*2;
                break;
            default:
                break;
    }
    return size;
}

s32 BTI_GetTexture(BTIFile* bti, s32 id, GXTexObj *texObj) {
    u32 dataSize;
    u8* btiImageData;
 
    if (!bti) {
        return -1;
    }
    if (!texObj) {
        return -1;
    }
    if (id < 0 || id >= bti->NImages) {
        return -1;
    }
    
    btiImageData = &((u8*)bti)[bti->imageDataOffset];
    dataSize = BTI_GetTextureSize(bti->width, bti->height, bti->imageFormat);
    DCFlushRange(btiImageData, dataSize);
    GX_InitTexObj(texObj, btiImageData, bti->width, bti->height, bti->imageFormat, bti->wrapS, bti->wrapT, 0); //Force no mipmap
    
    return 0;
}