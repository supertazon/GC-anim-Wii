#ifndef _YAY0DEC_H_
#define _YAY0DEC_H_

#include <stdlib.h>
#include <string.h>
#include <gctypes.h>

unsigned int read32(unsigned char * buf);
u8* decompressYay0(u8* inbuffer, u32 yayOffset);
void prepOffsets(u8* iplData, u32 iplSize, u32 yayOffset[], u32 awOffset[]);

#endif