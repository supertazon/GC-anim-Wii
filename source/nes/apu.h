/*
 * apu.h - NES APU sound synthesis (2A03, NTSC): pulse 1/2, triangle, noise.
 * (DMC: only the $4011 direct level, which SMB does not use.)
 *
 * MIT licence. Independent of the CPU core: it is fed the register writes the core
 * logs (nes_t.apu_write_cb, with their CPU cycle) and renders 16-bit mono PCM.
 * Channels are timed event by event (no per-cycle loop) and box-filtered into the
 * output rate. Each channel can be muted while still being clocked (its state stays
 * exact), so a caller can render only the sound effects of a game.
 */
#ifndef NES_APU_H
#define NES_APU_H

#include <stdint.h>

#define APU_CPU_HZ 1789773.0

enum { APU_P1 = 1, APU_P2 = 2, APU_TRI = 4, APU_NOISE = 8, APU_ALL = 15 };

typedef struct {
	/* pulse */
	uint8_t duty, step, env_loop, const_vol, vol, env_start, env_div, env_vol;
	uint8_t sw_en, sw_period, sw_neg, sw_shift, sw_reload, sw_div;
	uint16_t period;
	uint8_t len;
	int32_t timer;            /* CPU cycles to the next step */
} apu_pulse_t;

typedef struct {
	uint8_t control, lin_reload, lin, lin_reload_flag, step, len;
	uint16_t period;
	int32_t timer;
} apu_tri_t;

typedef struct {
	uint8_t env_loop, const_vol, vol, env_start, env_div, env_vol, mode, len;
	uint16_t period, lfsr;
	int32_t timer;
} apu_noise_t;

typedef struct {
	apu_pulse_t p[2];
	apu_tri_t t;
	apu_noise_t n;
	uint8_t dmc_level, enable, mode5, irq_inhibit;
	int32_t seq_cycle;        /* frame sequencer position (CPU cycles) */
	unsigned mask;            /* channels heard (APU_P1 | ...) */
	/* output */
	double rate, cyc_per_sample, acc_cyc, acc;
	float hp1, hp1_in, hp2, hp2_in, lp;   /* NES output filters */
} apu_t;

void apu_init(apu_t *a, double rate);
void apu_write(apu_t *a, uint16_t addr, uint8_t v);
/* advance `cycles` CPU cycles; writes the samples produced to out (room for
   cycles / cyc_per_sample + 2), returns how many */
int  apu_run(apu_t *a, int32_t cycles, int16_t *out);

#endif
