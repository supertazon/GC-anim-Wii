/*
 * smb_audio.h - the SMB mode's sound output on the Wii: one stereo ASND voice fed
 * from a ring buffer, the main thread writing one video frame of samples at a time.
 */
#ifndef SMB_AUDIO_H
#define SMB_AUDIO_H

#include <stdint.h>

#define SMB_AUDIO_HZ 32000

int  smb_audio_start(int volume);        /* 0..255 */
void smb_audio_push(const int16_t *pcm, int n);   /* n frames of left, right */
void smb_audio_stop(void);
/* samples waiting to be played (for the producer's pacing) */
int  smb_audio_queued(void);

#endif
