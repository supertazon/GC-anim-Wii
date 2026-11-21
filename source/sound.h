/* sound.h - the boot jingle, decoded from the user's IPL */
#ifndef SOUND_H
#define SOUND_H

#include <gccore.h>

int  sound_init(const u8 *ipl, u32 size, int variant);
void sound_start(int volume);
void sound_stop(void);
void sound_set_volume(int volume);
void sound_play_from(double seconds, int volume);   /* (re)start the jingle at this point */     /* the playing jingle's volume, 0..255 */
void sound_free(void);

#endif
