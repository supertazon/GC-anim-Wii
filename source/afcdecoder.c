/* Adapted from wsyster.c, all credits to Adam Gashlin - https://github.com/hcs64/vgm_ripping/blob/master/soundbank/wsyster/wsyster.c */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <gccore.h>
#include <asndlib.h>

/* AFC decoder */
const short afccoef[16][2] =
	{{0,0},
	{0x0800,0},
	{0,0x0800},
	{0x0400,0x0400},
	{0x1000,0xf800},
	{0x0e00,0xfa00},
	{0x0c00,0xfc00},
	{0x1200,0xf600},
	{0x1068,0xf738},
	{0x12c0,0xf704},
	{0x1400,0xf400},
	{0x0800,0xf800},
	{0x0400,0xfc00},
	{0xfc00,0x0400},
	{0xfc00,0},
	{0xf800,0}};

void AFCdecodebuffer
(
 unsigned char *input, /* location of encoded source samples */
 signed short *out,   /* location of destination buffer (16 bits / sample) */
 short * histp,
 short * hist2p
)
{
	int sample;
	short nibbles[16];
	int i,j;
	unsigned char *src;
	char *dst;
	short idx;
	short delta;
	short hist=*histp;
	short hist2=*hist2p;

	dst = (char*)out;

	src=input;
	delta = 1<<(((*src)>>4)&0xf);
	idx = (*src)&0xf;

	src++;

	for(i = 0; i < 16; i = i + 2) {
		j = ( *src & 255) >> 4;
		nibbles[i] = j;
		j = *src & 255 & 15;
		nibbles[i+1] = j;
		src++;
	}

	for(i = 0; i < 16; i = i + 1) {
		if(nibbles[i] >= 8)
			nibbles[i] = nibbles[i] - 16;
	}

	for(i = 0; i<16 ; i = i + 1) {

		sample = (delta * nibbles[i])<<11;
		sample += ((long)hist * afccoef[idx][0]) + ((long)hist2 * afccoef[idx][1]);
		sample = sample >> 11;

		if(sample > 32767) {
			sample = 32767;
		}
		if(sample < -32768) {
			sample = -32768;
		}

		*(short*)dst = (short)sample;
		dst = dst + 2;

		hist2 = hist;
		hist = (short)sample;

	}

	*histp=hist;
	*hist2p=hist2;

	return;
}

/* decoding AFC to an asndlib compatible buffer (based on wsyster.c) */
/* return 0 on success, 1 on failure */
int bufferAFC(u8* inbuffer, const int size, unsigned char sndbuffer[]) {
	int sizeleft;
	int sndbufferoffset=0;
	int inbufferoffset=0;

    struct {
	    unsigned char inbuf[9];
        short outbuf[16];
        short hist,hist2;
    } * chinfo;

    chinfo = calloc(1, sizeof(*chinfo));
	
	for (sizeleft=size;sizeleft>=9;sizeleft-=9) {
		unsigned char * inbuf = chinfo[0].inbuf;
        short * outbuf = chinfo[0].outbuf;
        short * histp = &chinfo[0].hist;
        short * hist2p = &chinfo[0].hist2;

		memcpy(inbuf,inbuffer+inbufferoffset,9);
		inbufferoffset+=9;

		AFCdecodebuffer(inbuf,outbuf,histp,hist2p);
		
        int i;
        for (i = 0; i < 16; i++) {
			memcpy(sndbuffer+sndbufferoffset,&chinfo[0].outbuf[i],sizeof(chinfo[0].outbuf[i]));
			sndbufferoffset+=sizeof(chinfo[0].outbuf[i]);	
        }
	}

    free(chinfo);

	return 0;
}

int playAW(u8* descrambledIPL,u32 offsetL,u32 offsetR) {
	u32 awsize=97920;
	u32 sndsize=348160;

	unsigned char *sndbufferL; // Left and Right channels
	unsigned char *sndbufferR;

	sndbufferL=(unsigned char *) malloc(sndsize);
	sndbufferR=(unsigned char *) malloc(sndsize);

	if (bufferAFC(descrambledIPL+offsetL,awsize,sndbufferL)) return 1;
	if (bufferAFC(descrambledIPL+offsetR,awsize,sndbufferR)) return 1;

	s32 track = ASND_GetFirstUnusedVoice();
	ASND_SetVoice(track, VOICE_MONO_16BIT, 32000, 0, sndbufferL, sndsize, 255, 0, NULL);
	track = ASND_GetFirstUnusedVoice();
	ASND_SetVoice(track, VOICE_MONO_16BIT, 32000, 0, sndbufferR, sndsize, 0, 255, NULL);
	
	free(sndbufferL);
	free(sndbufferR);
	
	return 0;
}
