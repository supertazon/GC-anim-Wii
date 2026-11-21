/*
 * smb_palette.h - the NES's colours in RGB (original colours mode)
 */
#ifndef SMB_PALETTE_H
#define SMB_PALETTE_H

#include <stdint.h>

/* the 64 NES colours, decoded from the PPU's NTSC signal levels */
void smb_palette_rgb(uint8_t rgb[64][3]);

#endif
