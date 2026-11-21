/*
 * smb_sound.c - see smb_sound.h. MIT licence.
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "smb_sound.h"

void smb_sound_init(smb_sound_t *s, const ipl_bank_t *bank, double rate)
{
	int i;
	memset(s, 0, sizeof(*s));
	apu_init(&s->apu, rate);
	s->bank = bank;
	s->rate = rate;
	s->pitch_rate = rate;
	s->music_gain = 1.0f;
	s->out_gain = 0.4f;            /* level next to the IPL jingle */
	s->kit_hi = 6;
	s->kit_lo = 5;
	s->sfx_gain = 1.0f;
	for (i = 0; i < SND_CH; i++) {
		s->map[i].wave = -1;
		s->map[i].gain = 1.0f;
	}
	s->cap = (int)(rate / 20.0) + 64;      /* (room for 3 frames) */
	s->mix = (float *)malloc(sizeof(float) * (size_t)s->cap);
	s->mix_r = (float *)malloc(sizeof(float) * (size_t)s->cap);
	s->sfx = (int16_t *)malloc(sizeof(int16_t) * (size_t)s->cap);
}

void smb_sound_make_drum(smb_sound_t *s, int wave, float at_ms, float len_ms, float lp_hz, float gain)
{
	const ipl_wave_t *src;
	uint32_t st, len, fade, i;
	float y = 0.0f, k;
	free(s->drum.pcm);
	memset(&s->drum, 0, sizeof(s->drum));
	if (wave < 0 || wave >= s->bank->count || !s->bank->w[wave].pcm)
		return;
	src = &s->bank->w[wave];
	st = (uint32_t)((at_ms - 5.0f) * src->rate / 1000.0f);
	len = (uint32_t)(len_ms * src->rate / 1000.0f);
	if (st + len > src->n)
		return;
	s->drum.pcm = (int16_t *)malloc((len + 16) * 2);
	if (!s->drum.pcm)
		return;
	fade = len * 2 / 5;
	/* softened: a one-pole low-pass (twice), a gentler attack, faded out */
	k = 1.0f - (float)exp(-2.0 * M_PI * lp_hz / src->rate);
	for (i = 0; i < len; i++) {
		float g = gain, x = (float)src->pcm[st + i];
		if (i < 96) g *= (float)i / 96.0f;
		if (i >= len - fade) g *= (float)(len - i) / (float)fade;
		y += k * (x - y);
		s->drum.pcm[i] = (int16_t)(y * g);
	}
	/* second pass of the low-pass */
	y = 0.0f;
	for (i = 0; i < len; i++) {
		y += k * ((float)s->drum.pcm[i] - y);
		s->drum.pcm[i] = (int16_t)y;
	}
	s->drum.n = len;
	s->drum.rate = src->rate;
	s->drum.key = 60;
	s->drum.root_hz = 0.0f;
}

int smb_sound_reverb(smb_sound_t *s, float rt60, float wet)
{
	/* Freeverb's tunings (at 44.1 kHz), the right side 23 samples longer */
	static const int combs[RV_COMBS] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
	static const int aps[RV_APS] = {556, 441, 341, 225};
	smb_reverb_t *r = &s->rv;
	double k = s->pitch_rate / 44100.0;
	int side, i, total = 0;
	float *p;
	s->wet = 0.0f;
	free(r->buf);
	memset(r, 0, sizeof(*r));
	if (wet <= 0.0f)
		return 0;
	r->pre_len = (int)(0.025 * s->pitch_rate);          /* 25 ms pre-delay */
	for (side = 0; side < 2; side++) {
		for (i = 0; i < RV_COMBS; i++) {
			r->comb_len[side][i] = (int)((combs[i] + side * 23) * k);
			total += r->comb_len[side][i];
		}
		for (i = 0; i < RV_APS; i++) {
			r->ap_len[side][i] = (int)((aps[i] + side * 23) * k);
			total += r->ap_len[side][i];
		}
	}
	r->buf = (float *)calloc((size_t)(total + r->pre_len), sizeof(float));
	if (!r->buf)
		return -1;
	p = r->buf;
	r->pre = p;
	p += r->pre_len;
	for (side = 0; side < 2; side++) {
		for (i = 0; i < RV_COMBS; i++) {
			r->comb[side][i] = p;
			p += r->comb_len[side][i];
			/* feedback for the decay time: -60 dB after rt60 */
			r->comb_fb[side][i] = (float)pow(10.0, -3.0 * r->comb_len[side][i] / (rt60 * s->pitch_rate));
		}
		for (i = 0; i < RV_APS; i++) {
			r->ap[side][i] = p;
			p += r->ap_len[side][i];
		}
	}
	r->damp = 0.25f;
	s->wet = wet;
	return 0;
}

/* in: mono, n samples; adds the reverb to l / r */
static void reverb_run(smb_reverb_t *r, const float *in, float *l, float *rr, int n, float wet)
{
	int i, side, c;
	for (i = 0; i < n; i++) {
		float x = r->pre[r->pre_pos];
		r->pre[r->pre_pos] = in[i] * 0.015f;           /* (Freeverb's input gain) */
		if (++r->pre_pos >= r->pre_len)
			r->pre_pos = 0;
		for (side = 0; side < 2; side++) {
			float acc = 0.0f;
			for (c = 0; c < RV_COMBS; c++) {
				float *b = r->comb[side][c];
				int pos = r->comb_pos[side][c];
				float y = b[pos];
				r->comb_lp[side][c] = y + (r->comb_lp[side][c] - y) * r->damp;
				b[pos] = x + r->comb_lp[side][c] * r->comb_fb[side][c];
				if (++pos >= r->comb_len[side][c])
					pos = 0;
				r->comb_pos[side][c] = pos;
				acc += y;
			}
			for (c = 0; c < RV_APS; c++) {
				float *b = r->ap[side][c];
				int pos = r->ap_pos[side][c];
				float y = b[pos];
				b[pos] = acc + y * 0.5f;
				acc = y - acc;
				if (++pos >= r->ap_len[side][c])
					pos = 0;
				r->ap_pos[side][c] = pos;
			}
			if (side == 0)
				l[i] += acc * wet;
			else
				rr[i] += acc * wet;
		}
	}
}

void smb_sound_use_shadow(smb_sound_t *s, smb_shadow_t *sh)
{
	s->sh = sh;
	apu_init(&s->sh_apu, 4000.0);            /* (state only: a low rate) */
	s->sh_apu.mask = 0;
	if (sh) {
		sh->n = 0;
		sh->frames = 0;
	}
}

void smb_sound_use_seq(smb_sound_t *s, smb_seq_t *q)
{
	s->seq = q;
	s->seq_on = q && q->active;
	s->seq_force = 0;
	s->seq_rate = 1.0;
}

void smb_sound_free(smb_sound_t *s)
{
	free(s->drum.pcm);
	s->drum.pcm = NULL;
	free(s->mix);
	free(s->mix_r);
	free(s->sfx);
	free(s->rv.buf);
	s->rv.buf = NULL;
	s->mix = s->mix_r = NULL;
	s->sfx = NULL;
}

void smb_sound_write(void *user, uint16_t addr, uint8_t val, uint64_t cyc)
{
	smb_sound_t *s = (smb_sound_t *)user;
	if (s->nwr < SND_MAX_WRITES) {
		s->wr[s->nwr].a = addr;
		s->wr[s->nwr].v = val;
		s->wr[s->nwr].cyc = cyc;
		s->nwr++;
	}
}

/* a channel's music at the end of the frame: sounding, frequency (Hz), level 0..1 */
static int channel_state(const apu_t *a, int ch, float *freq, float *level)
{
	if (ch <= SND_SQ2) {
		const apu_pulse_t *p = &a->p[ch];
		int vol = p->const_vol ? p->vol : p->env_vol;
		*freq = (float)(APU_CPU_HZ / (16.0 * (p->period + 1)));
		*level = (float)vol / 15.0f;
		return p->len && p->period >= 8 && vol;
	}
	if (ch == SND_TRI) {
		*freq = (float)(APU_CPU_HZ / (32.0 * (a->t.period + 1)));
		*level = 1.0f;
		return a->t.len && a->t.lin && a->t.period >= 2;
	}
	{
		int vol = a->n.const_vol ? a->n.vol : a->n.env_vol;
		*freq = (float)(APU_CPU_HZ / (double)a->n.period);
		*level = (float)vol / 15.0f;
		return a->n.len && vol;
	}
}

static void voice_render(smb_voice_t *v, float *mix, float *mix_r, int n, float gl, float gr)
{
	const ipl_wave_t *w = v->w;
	float a0 = v->amp, da = (v->target - v->amp) / (float)(n > 0 ? n : 1);
	int i;
	if (!w || !w->pcm || (!v->on && v->amp <= 0.0f && v->target <= 0.0f))
		return;
	for (i = 0; i < n; i++) {
		uint32_t ip = (uint32_t)v->pos;
		float fr = (float)(v->pos - (double)ip), x;
		if (ip + 1 >= w->n) {
			v->on = 0;
			v->amp = v->target = 0.0f;
			return;
		}
		x = (float)w->pcm[ip] + ((float)w->pcm[ip + 1] - (float)w->pcm[ip]) * fr;
		x *= a0 + da * (float)i;
		mix[i] += x * gl;
		mix_r[i] += x * gr;
		v->pos += v->step;
		if (w->loop && v->pos >= (double)w->loop_end)
			v->pos -= (double)(w->loop_end - w->loop_start);
	}
	v->amp = v->target;
}

int smb_sound_frame(smb_sound_t *s, const uint8_t *ram, uint64_t frame_end, int16_t *out, int max)
{
	int ch, i, n = 0, onset[SND_CH] = {0, 0, 0, 0}, paused;
	const apu_t *music_src;
	unsigned sfx = 0, mask;
	/* the channels the sound engine is using for an effect this frame */
	if (ram[0xf1]) sfx |= APU_P1;
	if (ram[0xf2]) sfx |= APU_P2;
	if (ram[0xf3]) sfx |= APU_NOISE;
	/* the pause sound (SMB RAM PauseSoundBuffer $07C6) is on square 1; while the game
	   is paused (GamePauseStatus $0776 bit 0) its music stops, ours waits with it */
	if (frame_end && ram[0x7c6]) sfx |= APU_P1;
	paused = frame_end && (ram[0x776] & 0x01);
	/* heard from the APU: the effects, and channels whose music has no instrument */
	mask = sfx;
	for (ch = 0; ch < SND_CH; ch++)
		if (s->map[ch].wave == -1)
			mask |= 1u << ch;               /* (-2: the channel's music is not heard) */
	s->apu.mask = mask;
	if (!frame_end) {
		/* no game this frame (before play starts): its length of silence, the music
		   (from the recording) still plays */
		n = (int)(s->rate / 59.94 + 0.5);
		s->started = 0;
		s->nwr = 0;
		memset(s->sfx, 0, sizeof(int16_t) * (size_t)n);
		goto music;
	}
	if (!s->started) {
		s->last_cyc = frame_end > 29781 ? frame_end - 29781 : 0;
		if (s->nwr && s->wr[0].cyc < s->last_cyc)
			s->last_cyc = s->wr[0].cyc;
		s->started = 1;
	}
	/* never more than one frame of samples: if the game ran more than one frame
	   since the last one (an ending's first frame), the older part is skipped */
	if (frame_end > s->last_cyc + 29781 + 64)
		s->last_cyc = frame_end - 29781;
	for (i = 0; i < s->nwr; i++) {
		uint16_t a = s->wr[i].a;
		if (s->wr[i].cyc > s->last_cyc) {
			n += apu_run(&s->apu, (int32_t)(s->wr[i].cyc - s->last_cyc), s->sfx + n);
			s->last_cyc = s->wr[i].cyc;
		}
		apu_write(&s->apu, a, s->wr[i].v);
		if (a == 0x4003) onset[SND_SQ1] = 1;
		if (a == 0x4007) onset[SND_SQ2] = 1;
		if (a == 0x400b) onset[SND_TRI] = 1;
		if (a == 0x400f) onset[SND_NOISE] = 1;
	}
	s->nwr = 0;
	if (frame_end > s->last_cyc) {
		n += apu_run(&s->apu, (int32_t)(frame_end - s->last_cyc), s->sfx + n);
		s->last_cyc = frame_end;
	}
music:
	if (n > max / 2) n = max / 2;
	if (n > s->cap) n = s->cap;

	/* the area music: from the recording while the game is playing it (overworld,
	   SMB RAM AreaMusicBuffer $F4 = 1, no EventMusicBuffer $07B1), then live */
	if (s->seq_on && !s->seq_force && (ram[0xf4] != 0x01 || ram[0x7b1] != 0))
		s->seq_on = 0;
	if (s->seq_on && !paused)
		smb_seq_step(s->seq, s->seq_rate);
	else if (s->seq_on)
		for (ch = 0; ch < SND_CH; ch++)
			s->seq->onset[ch] = 0;
	/* the shadow's frames (each: its writes, then a frame of the APU's own clocking) */
	if (s->sh && s->sh->game) {
		static int16_t scratch[2048];
		int f, i2 = 0, sh_on[SND_CH] = {0, 0, 0, 0};
		for (f = 0; f < s->sh->frames; f++) {
			for (; i2 < s->sh->n && s->sh->f[i2] == f; i2++) {
				uint16_t a = s->sh->a[i2];
				apu_write(&s->sh_apu, a, s->sh->v[i2]);
				if (a == 0x4003) sh_on[SND_SQ1] = 1;
				if (a == 0x4007) sh_on[SND_SQ2] = 1;
				if (a == 0x400b) sh_on[SND_TRI] = 1;
				if (a == 0x400f) sh_on[SND_NOISE] = 1;
			}
			apu_run(&s->sh_apu, 29781, scratch);
		}
		s->sh->n = 0;
		s->sh->frames = 0;
		if (!s->seq_on)
			for (ch = 0; ch < SND_CH; ch++)
				onset[ch] = sh_on[ch];
	}
	{
		const apu_t *src = s->seq_on ? &s->seq->apu : (s->sh && s->sh->game) ? &s->sh_apu : &s->apu;
		if (s->seq_on)
			for (ch = 0; ch < SND_CH; ch++)
				onset[ch] = s->seq->onset[ch];
		music_src = src;
	}

	/* the music: one sampler voice per channel, placed left / right */
	memset(s->mix, 0, sizeof(float) * (size_t)n);
	memset(s->mix_r, 0, sizeof(float) * (size_t)n);
	for (ch = 0; ch < SND_CH; ch++) {
		smb_voice_t *v = &s->v[ch];
		const smb_snd_map_t *m = &s->map[ch];
		float f, lv;
		int on;
		int wave = m->wave, tr_semi = m->transpose;
		if (ch == SND_NOISE && wave == SND_KIT) {
			/* drums: the short, high hits on kit_hi, the lower ones on kit_lo */
			if (music_src->n.period <= 64) {
				wave = s->kit_hi;
			} else {
				wave = s->kit_lo;
				tr_semi = s->kit_lo_transpose;
			}
		}
		{
			const ipl_wave_t *w = wave == SND_DRUM ? &s->drum :
			                      (wave >= 0 && wave < s->bank->count) ? &s->bank->w[wave] : NULL;
			if (!w || !w->pcm)
				continue;
			/* (a channel used by an effect: its music goes on, unless it comes from
			   the game's APU, where the effect has replaced it) */
			on = channel_state(music_src, ch, &f, &lv) && (!(sfx & (1u << ch)) || music_src != &s->apu) &&
		     !(s->seq_on && !s->seq->active) &&   /* (a forced sequence that is over: silence) */
		     !(!frame_end && !s->seq_on) &&        /* (no game, no recording: nothing held) */
		     !paused;
			if (v->w != w)
				v->on = 0;              /* (another drum: restart) */
			v->w = w;
		}
		if (on) {
			double key_hz = v->w->root_hz > 0.0f ? (double)v->w->root_hz : 261.63;
			double tr = pow(2.0, (double)tr_semi / 12.0);
			if (ch == SND_NOISE)
				v->step = (double)v->w->rate / s->pitch_rate * tr;   /* drums: unpitched */
			else
				v->step = (double)v->w->rate / s->pitch_rate * (double)f * tr / key_hz;
			if (onset[ch] || !v->on) {
				v->pos = 0.0;            /* a new note */
				v->on = 1;
				v->amp = 0.0f;
			}
			v->target = lv;
		} else {
			v->target = 0.0f;           /* released */
			v->on = v->amp > 0.0f;
		}
		{
			/* equal power, the middle at the mono level */
			float a = (m->pan + 1.0f) * 0.785398f;
			voice_render(v, s->mix, s->mix_r, n, m->gain * 1.41421f * cosf(a), m->gain * 1.41421f * sinf(a));
		}
	}
	{
		float g = 0.25f * s->music_gain;
		for (i = 0; i < n; i++) {
			s->mix[i] *= g;
			s->mix_r[i] *= g;
		}
	}
	if (s->wet > 0.0f && s->rv.buf) {
		/* the reverb of the music (its mono sum), added to both sides */
		static float mono[2048];
		int done = 0;
		while (done < n) {
			int k = n - done > 2048 ? 2048 : n - done;
			for (i = 0; i < k; i++)
				mono[i] = 0.5f * (s->mix[done + i] + s->mix_r[done + i]);
			reverb_run(&s->rv, mono, s->mix + done, s->mix_r + done, k, s->wet);
			done += k;
		}
	}
	for (i = 0; i < n; i++) {
		float d = (float)s->sfx[i] * s->sfx_gain;
		float l = (s->mix[i] + d) * s->out_gain, r = (s->mix_r[i] + d) * s->out_gain;
		out[2 * i] = (int16_t)(l > 32767.0f ? 32767.0f : l < -32768.0f ? -32768.0f : l);
		out[2 * i + 1] = (int16_t)(r > 32767.0f ? 32767.0f : r < -32768.0f ? -32768.0f : r);
	}
	return n;
}
