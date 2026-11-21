/*
 * smb_music.h - SMB's area music, recorded ahead and played back at any tempo.
 *
 * The game makes its music by calling its sound engine (ROM $F2D0, called from its
 * NMI) once per frame. From a copy of the booted game, that routine alone is called
 * a few thousand times (no picture, no game logic: a few ms of emulation), and the
 * APU writes of each call are kept: one entry per music frame. The sequencer then
 * replays them at a variable rate (frames of music per video frame) through its own
 * APU state, which the sampler reads like the live channels. This is what lets the
 * 1-1 theme start at the IPL jingle's tempo and settle to its own.
 * MIT licence. Portable C.
 */
#ifndef SMB_MUSIC_H
#define SMB_MUSIC_H

#include <stdint.h>
#include "nes/nes.h"
#include "nes/apu.h"

typedef struct {
	volatile uint32_t nframes; /* frames recorded so far (published after each frame) */
	volatile int done;         /* recording finished (or failed) */
	uint32_t want;             /* frames to record */
	uint32_t *first;           /* per frame: index of its first write (want + 1 entries) */
	uint16_t *addr;
	uint8_t *val;
	uint32_t nwrites, cap;     /* (fixed capacity: no reallocation while being read) */
	nes_t *game;               /* the copy being run */
	int until_event_end;       /* stop when the event music (RAM $07B1) has ended */
} smb_music_t;

/* record `frames` music frames from a copy of the game in `n` (left untouched), in one go */
int  smb_music_record(smb_music_t *m, const nes_t *n, uint32_t frames);
/* the same in pieces (another thread can play what is already there):
   begin, then step until it returns 0 */
int  smb_music_begin(smb_music_t *m, const nes_t *n, uint32_t frames, uint32_t max_writes);
int  smb_music_step(smb_music_t *m, uint32_t frames);
void smb_music_free(smb_music_t *m);

/* SMB 1-1's overworld theme: everything repeats every 5184 frames from frame 134
   of the recording (checked write for write against the game) */
#define SMB_MUSIC_LOOP_FROM 134
#define SMB_MUSIC_LOOP_LEN  5184

typedef struct {
	const smb_music_t *m;
	apu_t apu;                 /* the music's channels (state only) */
	double cursor;             /* music frames played */
	uint32_t next;             /* next frame to apply */
	uint32_t loop_from;        /* when the end is reached, go on from here (SMB_SEQ_ONCE: stop) */
	int onset[4];              /* note starts in the last step: sq1, sq2, tri, noise */
	int active;
} smb_seq_t;

/* the music live, without the sound effects: a copy of the game that runs only its
   sound engine, once for each time the game runs it (smb_shadow_engine, called as the
   game's engine starts, SoundEngine $F2D0), on the game's state and music queues but
   never a sound effect - so no note of the music is left out for one (the game skips
   the notes of a channel an effect is using). Its APU writes, frame by frame, for
   smb_sound. */
#define SMB_SHADOW_MAX 1024
typedef struct {
	nes_t *game;
	uint16_t a[SMB_SHADOW_MAX];
	uint8_t v[SMB_SHADOW_MAX], f[SMB_SHADOW_MAX];
	int n, frames;             /* writes, engine frames since taken */
} smb_shadow_t;
int  smb_shadow_begin(smb_shadow_t *sh, const nes_t *src);
void smb_shadow_engine(smb_shadow_t *sh, const nes_t *main);
void smb_shadow_free(smb_shadow_t *sh);

#define SMB_SEQ_ONCE 0xffffffffu
void smb_seq_start(smb_seq_t *q, const smb_music_t *m, double rate_hz);
/* advance by `rate` music frames (1.0 = the game's own tempo) */
void smb_seq_step(smb_seq_t *q, double rate);

#endif
