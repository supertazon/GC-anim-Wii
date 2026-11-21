/*
 * smb_music.c - see smb_music.h. MIT licence.
 */
#include <stdlib.h>
#include <string.h>
#include "smb_music.h"

#define SMB_SOUND_ENGINE 0xf2d0     /* SoundEngine (called from the NMI at $80E4) */
#define NTSC_FRAME_CYCLES 29781

static smb_music_t *rec;

static void rec_write(void *user, uint16_t a, uint8_t v, uint64_t cyc)
{
	smb_music_t *m = rec;
	(void)user;
	(void)cyc;
	if (!m || m->nwrites >= m->cap)
		return;
	m->addr[m->nwrites] = a;
	m->val[m->nwrites] = v;
	m->nwrites++;
}

int smb_music_begin(smb_music_t *m, const nes_t *src, uint32_t frames, uint32_t max_writes)
{
	memset(m, 0, sizeof(*m));
	m->want = frames;
	m->cap = max_writes;
	m->first = (uint32_t *)malloc((frames + 1) * sizeof(uint32_t));
	m->addr = (uint16_t *)malloc(max_writes * sizeof(uint16_t));
	m->val = (uint8_t *)malloc(max_writes);
	m->game = (nes_t *)malloc(sizeof(nes_t));
	if (!m->first || !m->addr || !m->val || !m->game) {
		smb_music_free(m);
		m->done = 1;
		return -1;
	}
	m->first[0] = 0;
	*m->game = *src;
	m->game->pc_hook = NULL;                  /* (a copy: it does not build the level) */
	m->game->pc_hook2 = NULL;
	m->game->ctrl &= 0x7f;            /* no NMI: only the sound engine runs */
	m->game->nmi_pending = 0;
	m->game->p |= 0x04;               /* no IRQ */
	m->game->apu_write_cb = rec_write;
	m->game->cb_user = NULL;
	return 0;
}

int smb_music_step(smb_music_t *m, uint32_t frames)
{
	nes_t *n = m->game;
	if (m->done || !n)
		return 0;
	rec = m;
	while (frames-- > 0 && m->nframes < m->want) {
		int guard = 20000;
		/* JSR SoundEngine from address $0000: return address $FFFF on the stack */
		n->ram[0x100 + n->s] = 0xff; n->s--;
		n->ram[0x100 + n->s] = 0xff; n->s--;
		n->pc = SMB_SOUND_ENGINE;
		while (n->pc != 0x0000 && !n->jammed && guard-- > 0)
			nes_step(n);
		if (n->jammed || guard <= 0 || m->nwrites >= m->cap) {
			m->done = 1;              /* keep what is there */
			break;
		}
		m->first[m->nframes + 1] = m->nwrites;
		m->nframes = m->nframes + 1;  /* published once its writes are in */
		if (m->until_event_end && m->nframes > 2 && n->ram[0x7b1] == 0) {
			m->want = m->nframes;     /* the event music is over */
			break;
		}
	}
	rec = NULL;
	if (m->nframes >= m->want)
		m->done = 1;
	if (m->done) {
		free(m->game);
		m->game = NULL;
	}
	return !m->done;
}

int smb_music_record(smb_music_t *m, const nes_t *src, uint32_t frames)
{
	if (smb_music_begin(m, src, frames, frames * 16 + 1024) != 0)
		return -1;
	while (smb_music_step(m, 256))
		;
	return m->nframes == frames ? 0 : -2;
}

void smb_music_free(smb_music_t *m)
{
	free(m->first);
	free(m->addr);
	free(m->val);
	free(m->game);
	memset(m, 0, sizeof(*m));
}

void smb_seq_start(smb_seq_t *q, const smb_music_t *m, double rate_hz)
{
	memset(q, 0, sizeof(*q));
	q->m = m;
	apu_init(&q->apu, rate_hz > 0.0 ? rate_hz : 4000.0);   /* (state only: a low rate) */
	q->apu.mask = 0;
	q->active = m && m->nframes > 0;
}

void smb_seq_step(smb_seq_t *q, double rate)
{
	static int16_t scratch[2048];
	int ch;
	for (ch = 0; ch < 4; ch++)
		q->onset[ch] = 0;
	if (!q->active)
		return;
	q->cursor += rate;
	while ((double)q->next < q->cursor) {
		uint32_t i, f = q->next;
		if (f >= q->m->nframes) {
			if (!q->m->done) {
				q->cursor = (double)q->next;   /* not recorded yet: wait for it */
				break;
			}
			if (q->loop_from >= q->m->nframes) {
				q->active = 0;               /* played once: over */
				break;
			}
			/* the end of the recording: on from loop_from */
			q->cursor -= (double)(q->m->nframes - q->loop_from);
			q->next = q->loop_from;
			continue;
		}
		for (i = q->m->first[f]; i < q->m->first[f + 1]; i++) {
			uint16_t a = q->m->addr[i];
			apu_write(&q->apu, a, q->m->val[i]);
			if (a == 0x4003) q->onset[0] = 1;
			if (a == 0x4007) q->onset[1] = 1;
			if (a == 0x400b) q->onset[2] = 1;
			if (a == 0x400f) q->onset[3] = 1;
		}
		/* a frame of the APU's own clocking (envelopes, linear counter, lengths) */
		apu_run(&q->apu, NTSC_FRAME_CYCLES, scratch);
		q->next++;
	}
}

/* ---- the live music without the sound effects ---- */

#define SND_RAM(a) (((a) >= 0xf0 && (a) <= 0xff) || ((a) >= 0x7b0 && (a) <= 0x7ca))

static void shadow_write(void *user, uint16_t a, uint8_t v, uint64_t cyc)
{
	smb_shadow_t *sh = (smb_shadow_t *)user;
	(void)cyc;
	if (sh->n < SMB_SHADOW_MAX) {
		sh->a[sh->n] = a;
		sh->v[sh->n] = v;
		sh->f[sh->n] = (uint8_t)sh->frames;
		sh->n++;
	}
}

int smb_shadow_begin(smb_shadow_t *sh, const nes_t *src)
{
	memset(sh, 0, sizeof(*sh));
	sh->game = (nes_t *)malloc(sizeof(nes_t));
	if (!sh->game)
		return -1;
	*sh->game = *src;
	sh->game->pc_hook = NULL;
	sh->game->pc_hook2 = NULL;
	sh->game->ctrl &= 0x7f;
	sh->game->nmi_pending = 0;
	sh->game->p |= 0x04;
	sh->game->apu_write_cb = shadow_write;
	sh->game->cb_user = sh;
	return 0;
}

void smb_shadow_engine(smb_shadow_t *sh, const nes_t *main)
{
	nes_t *n = sh->game;
	int a, guard = 20000;
	if (!n)
		return;
	/* the game's state but the sound engine's own; its music queues as the game's
	   engine is about to read them; no sound effects */
	for (a = 0; a < 0x800; a++)
		if (!SND_RAM(a) && (a < 0x100 || a > 0x1ff))
			n->ram[a] = main->ram[a];
	n->ram[0xfa] = main->ram[0xfa];
	n->ram[0xfb] = main->ram[0xfb];
	n->ram[0xfc] = main->ram[0xfc];
	n->ram[0xfd] = n->ram[0xfe] = n->ram[0xff] = 0;
	/* JSR SoundEngine from address $0000: return address $FFFF on the stack */
	n->ram[0x100 + n->s] = 0xff; n->s--;
	n->ram[0x100 + n->s] = 0xff; n->s--;
	n->pc = SMB_SOUND_ENGINE;
	while (n->pc != 0x0000 && !n->jammed && guard-- > 0)
		nes_step(n);
	if (n->jammed || guard <= 0) {
		free(sh->game);                       /* (lost: the game's own music again) */
		sh->game = NULL;
		return;
	}
	if (sh->frames < 255)
		sh->frames++;
}

void smb_shadow_free(smb_shadow_t *sh)
{
	free(sh->game);
	sh->game = NULL;
}
