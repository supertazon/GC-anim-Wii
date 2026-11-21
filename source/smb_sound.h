/*
 * smb_sound.h - the SMB mode's sound: the game's sound effects as the NES makes
 * them (APU synthesis), its music played on the GameCube IPL's instruments.
 *
 * Fed the APU register writes of each emulated frame (nes_t.apu_write_cb) and the
 * game's RAM at the end of the frame, which tells which channels the sound engine
 * is using for an effect (SMB RAM Square1SoundBuffer $F1, Square2SoundBuffer $F2,
 * NoiseSoundBuffer $F3): those are synthesized; the others play music and are turned
 * into notes (pitch, volume, note starts) for the sampler.
 * MIT licence. Portable C (built on PC for the previews too).
 */
#ifndef SMB_SOUND_H
#define SMB_SOUND_H

#include <stdint.h>
#include "nes/apu.h"
#include "ipl_bank.h"
#include "smb_music.h"

enum { SND_SQ1, SND_SQ2, SND_TRI, SND_NOISE, SND_CH };
#define SND_KIT (-3)               /* noise channel: drum kit from the IPL percussion */
#define SND_DRUM 100               /* kit_hi / kit_lo: the made drum (smb_sound_make_drum) */

typedef struct {
	int wave;                  /* IPL wave for the channel's music, -1: the NES sound,
	                              -2: silent (its sound effects still play) */
	float gain;                /* sampler level */
	float pan;                 /* -1 left .. 1 right */
	int transpose;             /* semitones */
} smb_snd_map_t;

typedef struct {
	const ipl_wave_t *w;
	double pos, step;
	float amp, target;
	int on;
} smb_voice_t;

#define SND_MAX_WRITES 512

/* a small stereo reverb (Freeverb's layout: 8 combs + 4 all-passes a side, after a
   pre-delay), on the music only - the sound effects stay dry, as on the NES */
#define RV_COMBS 8
#define RV_APS 4
typedef struct {
	float *buf;                /* all the delay lines */
	int comb_len[2][RV_COMBS], comb_pos[2][RV_COMBS];
	float *comb[2][RV_COMBS], comb_lp[2][RV_COMBS], comb_fb[2][RV_COMBS];
	int ap_len[2][RV_APS], ap_pos[2][RV_APS];
	float *ap[2][RV_APS];
	float *pre;
	int pre_len, pre_pos;
	float damp;
} smb_reverb_t;

typedef struct {
	apu_t apu;
	const ipl_bank_t *bank;
	smb_snd_map_t map[SND_CH];
	smb_voice_t v[SND_CH];
	float music_gain, sfx_gain, out_gain;
	float wet;                 /* reverb level (0: none) */
	smb_reverb_t rv;
	int kit_hi, kit_lo, kit_lo_transpose;   /* SND_KIT: waves for high / low hits */
	ipl_wave_t drum;           /* a drum cut from an IPL stream (owned) */
	/* the area music from the recording (smb_music.h) while the game plays it */
	smb_seq_t *seq;
	double seq_rate;           /* music frames per frame */
	int seq_on;
	int seq_force;             /* the sequencer's music whatever the game plays (silent once over) */
	/* the live music without the sound effects (smb_music.h), when not from seq */
	smb_shadow_t *sh;
	apu_t sh_apu;
	double rate;               /* samples produced per second of game */
	double pitch_rate;         /* rate the samples are played at (pitch of the instruments) */
	/* the frame's writes */
	struct { uint16_t a; uint8_t v; uint64_t cyc; } wr[SND_MAX_WRITES];
	int nwr;
	uint64_t last_cyc;
	int started;
	float *mix, *mix_r;        /* work buffers (left, right) */
	int16_t *sfx;
	int cap;
} smb_sound_t;

void smb_sound_init(smb_sound_t *s, const ipl_bank_t *bank, double rate);
/* the reverb: decay time (s, to -60 dB) and level; 0 level: off */
int  smb_sound_reverb(smb_sound_t *s, float rt60, float wet);
void smb_sound_free(smb_sound_t *s);
/* a drum from a slice of an IPL wave: start (ms), length (ms), low-pass (Hz), gain */
void smb_sound_make_drum(smb_sound_t *s, int wave, float at_ms, float len_ms, float lp_hz, float gain);
/* the live music from this shadow sound engine (NULL: from the game's APU, its notes
   left out under the sound effects, as on the NES) */
void smb_sound_use_shadow(smb_sound_t *s, smb_shadow_t *sh);
/* play the area music from this sequencer (until the game plays other music) */
void smb_sound_use_seq(smb_sound_t *s, smb_seq_t *q);
/* the APU write hook (nes_apu_write_fn) */
void smb_sound_write(void *s, uint16_t addr, uint8_t val, uint64_t cyc);
/* end of an emulated frame: render it (frame_end = nes_t.cycles) into out as
   interleaved stereo (left, right); returns sample frames (max: frames out can hold) */
int  smb_sound_frame(smb_sound_t *s, const uint8_t *ram, uint64_t frame_end, int16_t *out, int max);

#endif
