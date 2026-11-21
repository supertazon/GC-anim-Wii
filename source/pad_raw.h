/*
 * pad_raw.h - GameCube controller sticks measured from their real centre.
 *
 * libogc reports the sticks relative to the "origin" the controller sends when it
 * is connected (at power-on): a stick held then gives a wrong centre, and once it
 * is let go it reads as pushed for as long as the program runs (the C-stick view
 * stayed turned). Here the raw positions are taken from the serial interface's
 * input buffer (just updated by PAD_ScanPads; libogc's default analog mode 3) and
 * measured from the nominal centre (128), with a small dead zone for controllers
 * whose sticks do not rest exactly there.
 */
#ifndef PAD_RAW_H
#define PAD_RAW_H

#include <gccore.h>

#define PAD_RAW_DEADZONE 12

static inline s8 pad_raw_dz(int v)
{
	if (v > PAD_RAW_DEADZONE) v -= PAD_RAW_DEADZONE;
	else if (v < -PAD_RAW_DEADZONE) v += PAD_RAW_DEADZONE;
	else v = 0;
	return (s8)(v > 127 ? 127 : v < -128 ? -128 : v);
}

/* controller chan (0-3), right after PAD_ScanPads (connected: its return value):
   main stick x/y, C-stick x/y (+y up). Only read when this scan got fresh data from
   the controller (the registers then hold that same poll); otherwise the last values
   are kept, and dropped after half a second without data (no controller). */
static inline void pad_raw_sticks(int chan, u32 connected, s8 *sx, s8 *sy, s8 *cx, s8 *cy)
{
	static s8 last[4][4];
	static int missing[4];
	/* SI channel registers: input buffer high / low at 0x04 / 0x08 + 12 * chan */
	volatile u32 *si = (volatile u32 *)0xCD006400;
	chan &= 3;
	if (connected & (1u << chan)) {
		u32 h = si[chan * 3 + 1], l = si[chan * 3 + 2];
		last[chan][0] = pad_raw_dz((int)((h >> 8) & 0xff) - 128);
		last[chan][1] = pad_raw_dz((int)(h & 0xff) - 128);
		last[chan][2] = pad_raw_dz((int)((l >> 24) & 0xff) - 128);
		last[chan][3] = pad_raw_dz((int)((l >> 16) & 0xff) - 128);
		missing[chan] = 0;
	} else if (++missing[chan] > 30) {
		last[chan][0] = last[chan][1] = last[chan][2] = last[chan][3] = 0;
		missing[chan] = 30;
	}
	*sx = last[chan][0];
	*sy = last[chan][1];
	*cx = last[chan][2];
	*cy = last[chan][3];
}

#endif
