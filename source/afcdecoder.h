#ifndef _AFCDECODER_H_
#define _AFCDECODER_H_

void Descrambler(u8* data, u32 size);
void AFCdecodebuffer(unsigned char *input, signed short *out, short * histp, short * hist2p);
int bufferAFC(u8* inbuffer, const int size, unsigned char sndbuffer[]);
int playAW(u8* descrambledIPL,int offsetL,int offsetR);

#endif