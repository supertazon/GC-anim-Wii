/*
 * smb_audio.c - see smb_audio.h. Stereo: samples are left/right pairs (one frame =
 * 2 x 16 bits).
 *
 * ASND plays a voice from two buffers: the one playing and the next. The next one is
 * given by ASND_AddVoice as soon as the slot is free (ASND_TestVoiceBufferReady),
 * both from the voice's callback and, every frame, from the main thread (the
 * callback alone came too late: ASND then played a few ms of silence between
 * buffers). Buffers are 64 ms long so that a frame or two of delay never starves
 * the voice. They are filled from a ring buffer that the main thread writes after
 * each frame; if it runs dry the last sample is held, if it overfills the oldest
 * samples are dropped (the game's frame rate and the audio clock drift slightly).
 */
#include <string.h>
#include <gccore.h>
#include <asndlib.h>
#include "smb_audio.h"

#define BLOCK 2048                        /* sample frames per buffer (8 KiB, 64 ms) */
#define NBLOCKS 3                         /* playing, queued, being filled */
#define RING 16384                        /* power of two */
#define START_FILL (BLOCK * 2)            /* samples waiting before the voice starts */
#define HIGH_WATER (BLOCK * 5)

static int16_t blocks[NBLOCKS][BLOCK * 2] ATTRIBUTE_ALIGN(32);
static int16_t ring[RING][2];
static volatile uint32_t rd, wr;
static volatile int cur_block;
static s32 voice = -1;
static int started, volume_set;
static int16_t last[2];

static void fill(int16_t *dst)
{
	int i;
	/* too far ahead of the output: drop the oldest (only this side moves rd) */
	if (wr - rd > HIGH_WATER)
		rd = wr - HIGH_WATER / 2;
	for (i = 0; i < BLOCK; i++) {
		if (rd != wr) {
			last[0] = ring[rd & (RING - 1)][0];
			last[1] = ring[rd & (RING - 1)][1];
			rd++;
		}
		dst[2 * i] = last[0];
		dst[2 * i + 1] = last[1];
	}
	DCFlushRange(dst, BLOCK * 2 * sizeof(int16_t));
}

/* give the voice its next buffer if its slot is free (callback or main thread) */
static void feed(void)
{
	u32 level;
	if (voice < 0 || !started)
		return;
	level = IRQ_Disable();
	if (ASND_TestVoiceBufferReady(voice) == 1) {
		int16_t *b = blocks[cur_block];
		cur_block = (cur_block + 1) % NBLOCKS;
		fill(b);
		ASND_AddVoice(voice, b, BLOCK * 2 * sizeof(int16_t));
	}
	IRQ_Restore(level);
}

static void voice_cb(s32 v)
{
	(void)v;
	feed();
}

int smb_audio_start(int volume)
{
	if (voice >= 0)
		return 0;
	rd = wr = 0;
	last[0] = last[1] = 0;
	started = 0;
	cur_block = 0;
	voice = ASND_GetFirstUnusedVoice();
	if (voice < 0)
		return -1;
	if (volume > 255) volume = 255;
	if (volume < 0) volume = 0;
	volume_set = volume;
	return 0;
}

void smb_audio_push(const int16_t *pcm, int n)
{
	int i;
	if (voice < 0)
		return;
	for (i = 0; i < n; i++) {
		ring[wr & (RING - 1)][0] = pcm[2 * i];
		ring[wr & (RING - 1)][1] = pcm[2 * i + 1];
		wr++;
	}
	if (!started) {
		/* start once two buffers' worth is waiting */
		if (wr - rd < START_FILL)
			return;
		fill(blocks[0]);
		cur_block = 1;
		if (ASND_SetVoice(voice, VOICE_STEREO_16BIT, SMB_AUDIO_HZ, 0, blocks[0], BLOCK * 2 * sizeof(int16_t),
		                  volume_set, volume_set, voice_cb) != SND_OK) {
			voice = -1;
			return;
		}
		started = 1;
	}
	feed();
}

int smb_audio_queued(void)
{
	return (int)(wr - rd);
}

void smb_audio_stop(void)
{
	if (voice >= 0 && started)
		ASND_StopVoice(voice);
	voice = -1;
	started = 0;
}
