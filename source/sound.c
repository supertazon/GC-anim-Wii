/*
 * sound.c - the boot jingle, decoded from the user's IPL (see ipl_audio.c)
 *
 * The IPL plays a stereo pair of AFC samples at 32 kHz with a sequence volume
 * of 0x6400 (0.781), i.e. 199/255.  One stereo ASND voice keeps both channels
 * sample-accurate.
 */
#include <stdlib.h>
#include <asndlib.h>
#include "sound.h"
#include "ipl_audio.h"

static int16_t *pcm;
static size_t pcm_bytes;
static int rate = 32000;
static s32 voice = -1;

int sound_init(const u8 *ipl, u32 size, int variant)
{
	ipl_audio_t a;
	if (ipl_audio_extract_ex(ipl, size, variant, &a) != IPL_AUDIO_OK)
		return -1;
	pcm = ipl_audio_interleave(&a, 0x8000, &pcm_bytes);
	rate = a.samplerate;
	ipl_audio_free(&a);
	if (!pcm)
		return -2;
	DCFlushRange(pcm, pcm_bytes);
	ASND_Init();
	ASND_Pause(0);
	return 0;
}

void sound_start(int volume)
{
	if (!pcm)
		return;
	voice = ASND_GetFirstUnusedVoice();
	if (voice < 0)
		voice = 0;
	ASND_SetVoice(voice, VOICE_STEREO_16BIT, rate, 0, pcm, pcm_bytes, volume, volume, NULL);
}

void sound_play_from(double seconds, int volume)
{
	size_t off;
	if (!pcm)
		return;
	if (voice >= 0)
		ASND_StopVoice(voice);
	off = (size_t)(seconds * rate) * 4;          /* stereo 16-bit */
	off &= ~(size_t)31;                         /* ASND: 32-byte aligned */
	if (off >= pcm_bytes)
		return;
	voice = ASND_GetFirstUnusedVoice();
	if (voice < 0)
		voice = 0;
	ASND_SetVoice(voice, VOICE_STEREO_16BIT, rate, 0, (u8 *)pcm + off, pcm_bytes - off, volume, volume, NULL);
}

void sound_set_volume(int volume)
{
	if (voice < 0)
		return;
	if (volume < 0) volume = 0;
	if (volume > 255) volume = 255;
	ASND_ChangeVolumeVoice(voice, volume, volume);
}

void sound_stop(void)
{
	if (voice >= 0)
		ASND_StopVoice(voice);
	voice = -1;
}

void sound_free(void)
{
	sound_stop();
	ASND_End();
	if (pcm)
		ipl_audio_free_buf(pcm);
	pcm = NULL;
}
