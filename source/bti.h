// Credits to Aurelio (https://github.com/Aurelio92) for the BTI implementation

#ifndef _BTI_H_
#define _BTI_H_

#include <grrlib.h>
#include <math.h>

struct BTIFile{
    u8 imageFormat;         //0x00
    u8 alphaEnable;         //0x01
    u16 width;              //0x02
    u16 height;             //0x04
    u8 wrapS;               //0x06
    u8 wrapT;               //0x07
    u16 paletteFormat;      //0x08
    u16 NPalettes;          //0x0A
    u32 paletteDataOffset;  //0x0C
    u32 unknown1;           //0x10
    u8 magnificationFilter; //0x14
    u8 minificationFilter;  //0x15
    u16 unknown2;           //0x16
    u8 NImages;             //0x18
    u8 unknown3;            //0x19
    u16 unknown4;           //0x1A
    u32 imageDataOffset;    //0x1C
 
};
typedef struct BTIFile BTIFile;

GRRLIB_texImg*  BTI_GRRLoad(BTIFile *in_bti);
void BTI_DrawImg (f32 xpos, f32 ypos, GXTexObj* tex, BTIFile* bti, f32 scaleX, f32 scaleY, u32 color);
u32 BTI_GetTextureSize(u32 width,u32 height,u32 fmt);
s32 BTI_GetTexture(BTIFile* bti, s32 id, GXTexObj *texObj);

#endif