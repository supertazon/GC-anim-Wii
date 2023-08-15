#include "yay0dec.h"

/* read big endian */
unsigned int read32(unsigned char * buf) {
	return (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];
}

// Yay0 decompress by Ethan Roseman (ethteck) - https://github.com/ethteck/n64decompress/blob/main/Yay0/decompress.py
u8* decompressYay0(u8* descrambledIPL, u32 yayOffset)
{
	u8 buf[4];
	u32 link_table_idx;
	u32 chunk_idx;
	u32 other_idx=16;
    u32 mask_bit_counter = 0;
    u32 curr_mask = 0;
	u32 idx=0;
	u32 i=0;
	u32 link;
	u32 offset;
	u32 count;
	u32 count_modifier;
    u32 decompressed_size = *(u32*)(descrambledIPL + yayOffset + 4);
    u32 link_table_offset = *(u32*)(descrambledIPL + yayOffset + 8);
    u32 chunk_offset = *(u32*)(descrambledIPL + yayOffset + 12);
	u8* outbuf=(u8*) malloc(decompressed_size);
	u8* yayBuf=(u8*) malloc(decompressed_size); // Turns out the decompressed Yay0 has the same size as the Yay0-compressed file in our cases	
	memcpy(yayBuf,descrambledIPL+yayOffset,decompressed_size);

    link_table_idx = link_table_offset;
    chunk_idx = chunk_offset;

    while (idx < decompressed_size){
        // If we're out of bits, get the next mask
        if (mask_bit_counter == 0){
			memcpy(buf,yayBuf+other_idx,4);
            curr_mask = read32(buf);
			other_idx+=4;
            mask_bit_counter = 32;
		}
        if (curr_mask & 0x80000000){
			memcpy(buf,yayBuf+chunk_idx,1);
            outbuf[idx] = buf[0];
            idx += 1;
            chunk_idx += 1;
		}
        else{
			memcpy(buf,yayBuf+link_table_idx,2);
			link = (buf[0] << 8) | buf[1];
            link_table_idx += 2;

            offset = idx - (link & 0xfff);

            count = link >> 12;

            if(count == 0){
				memcpy(buf,yayBuf+chunk_idx,1);
                count_modifier = buf[0];
                chunk_idx += 1;
                count = count_modifier + 18;
			}
            else{
                count += 2;
			}
            // Copy the block
            for(i=0;i<count;i++){
                outbuf[idx] = outbuf[offset + i - 1];
                idx += 1;
			}
		}
        curr_mask <<= 1;
        mask_bit_counter -= 1;
	}
	
	free(yayBuf);
	
    return outbuf;
}

void prepOffsets(u8* iplData, u32 iplSize, u32 yayOffset[], u32 awOffset[]){
	u32 offset=0;
	while(*(u64*)(iplData + offset) != 0x5961793000006140) offset +=4; // Header of Yay0 with env texture
	yayOffset[0]=offset;
	while(*(u64*)(iplData + offset) != 0x5961793000001020) offset +=4; // Header of Yay0 with tile texture
	yayOffset[1]=offset;
	while(*(u64*)(iplData + offset) != 0x5961793000016F60) offset +=4; // Header of Yay0 with logo textures
	yayOffset[2]=offset;
	while(*(u64*)(iplData + offset) != 0x59617930000064A0) offset +=4; // Header of Yay0 with GAMECUBE texture
	yayOffset[3]=offset;
	while(*(u64*)(iplData + offset) != 0x2000000000001F2A) offset +=4; // First bytes of AW file of right channel
	awOffset[1]=offset;
	awOffset[0]=offset-97920; // Left channel offset is always 97920 (awsize) before right channel offset
}