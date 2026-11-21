/*
 * ipl_bank.h - the GameCube IPL's instrument samples (JAudio WSYS wave bank),
 * taken from the user's own descrambled IPL and decoded (AFC 4-bit ADPCM).
 *
 * Known layout (identical in all 6 IPL dumps): 12 waves; 0-3 and 9-10 are the
 * jingle / menu ambience streams, 4-8 and 11 single-note instruments with a root
 * key (MIDI note at the stored rate) and, for 7, 8 and 11, a loop.
 * MIT licence. Portable C (also built on PC for the previews).
 */
#ifndef IPL_BANK_H
#define IPL_BANK_H

#include <stdint.h>

#define IPL_BANK_MAX 16

typedef struct {
	int16_t *pcm;              /* decoded samples (host order) */
	uint32_t n;                /* samples */
	float rate;                /* Hz */
	uint8_t key;               /* root key (MIDI) as stored - not always the pitch heard */
	float root_hz;             /* the pitch heard at the stored rate (0: percussion) */
	int loop;                  /* 1: loops over [loop_start, loop_end) */
	uint32_t loop_start, loop_end;
} ipl_wave_t;

typedef struct {
	int count;
	ipl_wave_t w[IPL_BANK_MAX];
} ipl_bank_t;

/* decode the bank from a descrambled IPL image (0 = ok) */
int  ipl_bank_load(ipl_bank_t *b, const uint8_t *ipl, uint32_t size);
void ipl_bank_free(ipl_bank_t *b);

#endif
