/*
 * apu.c - NES APU sound synthesis, see apu.h. MIT licence.
 *
 * NTSC 2A03 behaviour (nesdev wiki): pulse timers step every 2 * (period + 1) CPU
 * cycles through an 8-step duty sequence; the triangle every period + 1 cycles through
 * 32 steps (only while both its counters are non-zero); the noise LFSR every table
 * period. Envelopes and the triangle's linear counter are clocked on quarter frames,
 * length counters and sweeps on half frames, by the frame sequencer ($4017).
 * Output: the non-linear mixer, then the console's filters (high-pass 90 Hz and
 * 440 Hz, low-pass 14 kHz).
 */
#include <string.h>
#include <math.h>
#include "apu.h"

static const uint8_t len_table[32] = {
	10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
	12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};
static const uint8_t duty_table[4][8] = {
	{0, 1, 0, 0, 0, 0, 0, 0}, {0, 1, 1, 0, 0, 0, 0, 0},
	{0, 1, 1, 1, 1, 0, 0, 0}, {1, 0, 0, 1, 1, 1, 1, 1}
};
static const uint8_t tri_table[32] = {
	15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};
static const uint16_t noise_table[16] = {
	4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

void apu_init(apu_t *a, double rate)
{
	memset(a, 0, sizeof(*a));
	a->n.lfsr = 1;
	a->n.period = 4;
	a->mask = APU_ALL;
	a->rate = rate;
	a->cyc_per_sample = APU_CPU_HZ / rate;
	a->p[0].timer = a->p[1].timer = 2;
	a->t.timer = 1;
	a->n.timer = 4;
}

/* ---- sweep: target period, and whether it mutes the channel ---- */
static int sweep_target(const apu_pulse_t *p, int ch)
{
	int c = p->period >> p->sw_shift;
	if (p->sw_neg)
		return p->period - c - (ch == 0 ? 1 : 0);   /* pulse 1: ones' complement */
	return p->period + c;
}

static int pulse_muted(const apu_pulse_t *p, int ch)
{
	return p->period < 8 || (!p->sw_neg && sweep_target(p, ch) > 0x7ff);
}

static void env_clock(uint8_t *start, uint8_t *div, uint8_t *vol, uint8_t period, uint8_t loop)
{
	if (*start) {
		*start = 0;
		*vol = 15;
		*div = period;
	} else if (*div == 0) {
		*div = period;
		if (*vol)
			(*vol)--;
		else if (loop)
			*vol = 15;
	} else
		(*div)--;
}

static void quarter_frame(apu_t *a)
{
	int i;
	for (i = 0; i < 2; i++)
		env_clock(&a->p[i].env_start, &a->p[i].env_div, &a->p[i].env_vol, a->p[i].vol, a->p[i].env_loop);
	env_clock(&a->n.env_start, &a->n.env_div, &a->n.env_vol, a->n.vol, a->n.env_loop);
	if (a->t.lin_reload_flag)
		a->t.lin = a->t.lin_reload;
	else if (a->t.lin)
		a->t.lin--;
	if (!a->t.control)
		a->t.lin_reload_flag = 0;
}

static void half_frame(apu_t *a)
{
	int i;
	for (i = 0; i < 2; i++) {
		apu_pulse_t *p = &a->p[i];
		if (p->len && !p->env_loop)
			p->len--;
		/* sweep */
		if (p->sw_div == 0 && p->sw_en && p->sw_shift && !pulse_muted(p, i)) {
			int tgt = sweep_target(p, i);
			if (tgt >= 0)
				p->period = (uint16_t)tgt;
		}
		if (p->sw_div == 0 || p->sw_reload) {
			p->sw_div = p->sw_period;
			p->sw_reload = 0;
		} else
			p->sw_div--;
	}
	if (a->t.len && !a->t.control)
		a->t.len--;
	if (a->n.len && !a->n.env_loop)
		a->n.len--;
}

void apu_write(apu_t *a, uint16_t addr, uint8_t v)
{
	if (addr >= 0x4000 && addr <= 0x4007) {
		int i = (addr >> 2) & 1;
		apu_pulse_t *p = &a->p[i];
		switch (addr & 3) {
		case 0:
			p->duty = v >> 6;
			p->env_loop = (v >> 5) & 1;
			p->const_vol = (v >> 4) & 1;
			p->vol = v & 15;
			break;
		case 1:
			p->sw_en = v >> 7;
			p->sw_period = (v >> 4) & 7;
			p->sw_neg = (v >> 3) & 1;
			p->sw_shift = v & 7;
			p->sw_reload = 1;
			break;
		case 2:
			p->period = (uint16_t)((p->period & 0x700) | v);
			break;
		case 3:
			p->period = (uint16_t)((p->period & 0xff) | ((v & 7) << 8));
			if (a->enable & (1 << i))
				p->len = len_table[v >> 3];
			p->step = 0;
			p->env_start = 1;
			break;
		}
		return;
	}
	switch (addr) {
	case 0x4008:
		a->t.control = v >> 7;
		a->t.lin_reload = v & 0x7f;
		break;
	case 0x400a:
		a->t.period = (uint16_t)((a->t.period & 0x700) | v);
		break;
	case 0x400b:
		a->t.period = (uint16_t)((a->t.period & 0xff) | ((v & 7) << 8));
		if (a->enable & 4)
			a->t.len = len_table[v >> 3];
		a->t.lin_reload_flag = 1;
		break;
	case 0x400c:
		a->n.env_loop = (v >> 5) & 1;
		a->n.const_vol = (v >> 4) & 1;
		a->n.vol = v & 15;
		break;
	case 0x400e:
		a->n.mode = v >> 7;
		a->n.period = noise_table[v & 15];
		break;
	case 0x400f:
		if (a->enable & 8)
			a->n.len = len_table[v >> 3];
		a->n.env_start = 1;
		break;
	case 0x4011:
		a->dmc_level = v & 0x7f;
		break;
	case 0x4015:
		a->enable = v & 0x1f;
		if (!(v & 1)) a->p[0].len = 0;
		if (!(v & 2)) a->p[1].len = 0;
		if (!(v & 4)) a->t.len = 0;
		if (!(v & 8)) a->n.len = 0;
		break;
	case 0x4017:
		a->mode5 = v >> 7;
		a->irq_inhibit = (v >> 6) & 1;
		a->seq_cycle = 0;
		if (a->mode5) {
			quarter_frame(a);
			half_frame(a);
		}
		break;
	}
}

/* the frame sequencer's next event, CPU cycles from seq_cycle (and what it clocks) */
static int32_t seq_next(const apu_t *a, int *quarter, int *half)
{
	static const int32_t ev4[4] = {7457, 14913, 22371, 29829};
	static const int32_t ev5[5] = {7457, 14913, 22371, 29829, 37281};
	int i;
	if (!a->mode5) {
		for (i = 0; i < 4; i++)
			if (a->seq_cycle < ev4[i]) {
				*quarter = 1;
				*half = (i == 1 || i == 3);
				return ev4[i] - a->seq_cycle;
			}
		*quarter = *half = 0;
		return 29830 - a->seq_cycle;            /* wrap */
	}
	for (i = 0; i < 5; i++)
		if (a->seq_cycle < ev5[i]) {
			*quarter = (i != 3);
			*half = (i == 1 || i == 4);
			return ev5[i] - a->seq_cycle;
		}
	*quarter = *half = 0;
	return 37282 - a->seq_cycle;
}

static float mix_now(const apu_t *a)
{
	int i;
	float p = 0.0f, t = 0.0f, n = 0.0f, d = (float)a->dmc_level, pulse = 0.0f, tnd;
	for (i = 0; i < 2; i++) {
		const apu_pulse_t *q = &a->p[i];
		if ((a->mask & (1u << i)) && q->len && !pulse_muted(q, i) && duty_table[q->duty][q->step])
			p += (float)(q->const_vol ? q->vol : q->env_vol);
	}
	if (a->mask & APU_TRI)
		t = (float)tri_table[a->t.step];
	if ((a->mask & APU_NOISE) && a->n.len && !(a->n.lfsr & 1))
		n = (float)(a->n.const_vol ? a->n.vol : a->n.env_vol);
	if (p > 0.0f)
		pulse = 95.88f / (8128.0f / p + 100.0f);
	if (t > 0.0f || n > 0.0f || d > 0.0f)
		tnd = 159.79f / (1.0f / (t / 8227.0f + n / 12241.0f + d / 22638.0f) + 100.0f);
	else
		tnd = 0.0f;
	return pulse + tnd;
}

static int16_t out_sample(apu_t *a, float x)
{
	/* first-order filters at the output rate */
	float k90 = (float)(1.0 / (1.0 + 2.0 * M_PI * 90.0 / a->rate));
	float k440 = (float)(1.0 / (1.0 + 2.0 * M_PI * 440.0 / a->rate));
	float c = (float)(2.0 * M_PI * 14000.0 / a->rate), klp = c / (1.0f + c);
	float y, v;
	a->hp1 = k90 * (a->hp1 + x - a->hp1_in);
	a->hp1_in = x;
	a->hp2 = k440 * (a->hp2 + a->hp1 - a->hp2_in);
	a->hp2_in = a->hp1;
	a->lp += klp * (a->hp2 - a->lp);
	y = a->lp * 40000.0f;
	v = y > 32767.0f ? 32767.0f : y < -32768.0f ? -32768.0f : y;
	return (int16_t)v;
}

int apu_run(apu_t *a, int32_t cycles, int16_t *out)
{
	int nout = 0;
	while (cycles > 0) {
		int q = 0, h = 0, i;
		int32_t dt = cycles, s = seq_next(a, &q, &h);
		float level;
		/* channels not heard are not stepped (their waveform position does not
		   matter; envelopes and counters still follow the frame sequencer): an
		   APU used only for its state (or one silent channel) costs almost nothing */
		if (s < dt) dt = s;
		for (i = 0; i < 2; i++)
			if ((a->mask & (1u << i)) && a->p[i].timer < dt) dt = a->p[i].timer;
		if ((a->mask & APU_TRI) && a->t.timer < dt) dt = a->t.timer;
		if ((a->mask & APU_NOISE) && a->n.timer < dt) dt = a->n.timer;
		if (dt < 1) dt = 1;
		/* the level is constant over dt: integrate it into the output samples */
		level = mix_now(a);
		{
			double rem = (double)dt;
			while (a->acc_cyc + rem >= a->cyc_per_sample) {
				double take = a->cyc_per_sample - a->acc_cyc;
				a->acc += level * take;
				out[nout++] = out_sample(a, (float)(a->acc / a->cyc_per_sample));
				a->acc = 0.0;
				a->acc_cyc = 0.0;
				rem -= take;
			}
			a->acc += level * rem;
			a->acc_cyc += rem;
		}
		/* advance the timers of the channels heard */
		for (i = 0; i < 2; i++) {
			apu_pulse_t *p = &a->p[i];
			if (!(a->mask & (1u << i)))
				continue;
			p->timer -= dt;
			while (p->timer <= 0) {
				p->step = (uint8_t)((p->step + 1) & 7);
				p->timer += 2 * (p->period + 1);
			}
		}
		if (a->mask & APU_TRI) {
			a->t.timer -= dt;
			while (a->t.timer <= 0) {
				if (a->t.lin && a->t.len && a->t.period >= 2)
					a->t.step = (uint8_t)((a->t.step + 1) & 31);
				a->t.timer += a->t.period + 1;
			}
		}
		if (a->mask & APU_NOISE) {
			a->n.timer -= dt;
			while (a->n.timer <= 0) {
				uint16_t fb = (uint16_t)((a->n.lfsr ^ (a->n.lfsr >> (a->n.mode ? 6 : 1))) & 1);
				a->n.lfsr = (uint16_t)((a->n.lfsr >> 1) | (fb << 14));
				a->n.timer += a->n.period;
			}
		}
		a->seq_cycle += dt;
		if (dt == s) {
			if (q) quarter_frame(a);
			if (h) half_frame(a);
			if ((!a->mode5 && a->seq_cycle >= 29830) || (a->mode5 && a->seq_cycle >= 37282))
				a->seq_cycle = 0;
		}
		cycles -= dt;
	}
	return nout;
}
