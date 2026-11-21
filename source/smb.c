/*
 * smb.c - the SMB 1-1 mode: background boot, branch from the animation, play,
 * and the two endings that finish the animation. See smb.h.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <unistd.h>
#include "smb.h"
#include <wiiuse/wpad.h>
#include "pad_raw.h"
#include "scene.h"
#include "gcmath.h"
#include "nes/nes.h"
#include "smb_flow.h"
#include "smb_records.h"
#include <time.h>
#include <fat.h>
#include <sdcard/wiisd_io.h>
#include <ogc/usbstorage.h>
#include "ipl_bank.h"
#include "smb_sound.h"
#include "smb_music.h"
#include "smb_audio.h"
#include "sound.h"

#define TILE        54.0f
#define PAN_FRAMES  60     /* camera glide from the logo to the play framing */
#define JUMP_START  10     /* Mario jumps out of the top cube ...            */
#define JUMP_END    40     /* ... and lands on his start position            */

/* SMB RAM (community disassembly names) */
#define RAM_ENGINE   0x000e   /* GameEngineSubroutine */
#define RAM_OPERMODE 0x0770
#define RAM_YHIGH    0x00b5   /* Player_Y_HighPos */
#define ENG_FLAGPOLE 0x04   /* FlagpoleSlide: the flag comes down, he slides */
#define ENG_ENDLEVEL 0x05   /* PlayerEndLevel: he turns round on the pole, then hops off */
#define RAM_PSTATE   0x001d /* Player_State: 0 ground, 1 jump, 2 fall, 3 climbing (pole) */
#define ENG_LOSELIFE 0x06
#define ENG_PLAY     0x08
#define ENG_DEATH    0x0b

#define FW_MAX_PLAY 6
static nes_t nes;
static SmbFlowCfg flow;                   /* 1-1 or the whole game, test start */
static int orig_on;                       /* this run: original colours (Y / 2) */
static char sav_path[160], log_path[160];
static SmbRecords recs;
static SmbView view;
static SmbLayoutCfg lcfg;
static SmbCubeState cubes;
static int have_rom, booted, boot_frame;

/* last frame where the player was visible (pit deaths: he has left the screen) */
static struct {
	int on;
	int16_t x, y;
	int32_t level_x;
	u8 px[SMB_PLY * SMB_PLY];
} last_ply;
static struct {
	int on, left;
	int16_t x, y;
	uint8_t px[SMB_PLY * SMB_PLY];
} jump_ply;

/* ---------------- sound ----------------
 * the music on the IPL's instruments (melody and harmony wave 8, bass wave 11,
 * drums a hit cut from the jingle), the sound effects from the NES (smb_sound.c).
 * The intro: the IPL jingle fades out from 1.5 s to 2.5 s (from its start); the
 * 1-1 theme comes in at 2.0 s at 70 %, first note on the jingle's beat, full level at
 * 3.0 s; it starts at the jingle's tempo (115.6 BPM: 1.156x the theme's 100) and
 * settles to its own over 2 s. */
#define JINGLE_FADE_AT   1.5
#define JINGLE_FADE_LEN  1.0
#define THEME_IN_AFTER   0.5     /* after the jingle's fade starts */
#define THEME_LEVEL0     0.7
#define THEME_FADE_LEN   1.0
#define THEME_TEMPO0     1.156
#define THEME_RAMP_LEN   2.0
#define JINGLE_EIGHTH    0.2595  /* the jingle's eighth notes (115.6 BPM) ... */
#define JINGLE_BEAT0     0.225   /* ... from here */
#define THEME_LEAD_IN    8       /* the recording's first note is 8 frames in */

static ipl_bank_t bank;
static smb_sound_t snd;
static smb_music_t mus, fan;            /* the 1-1 theme; the level-clear fanfare */
/* the death music (DeathMusic $01): its length in frames (the same in every SMB
   release; measured once from a recording of it, as the fanfare is recorded) */
#define SMB_DEATH_MUSIC_FRAMES 181
static smb_seq_t seq, fseq;
static int fan_mode, jingle_start_frame = -1;
static int snd_ready, snd_on, nes_ran, aud_volume, jingle_volume;
static double aud_t, aud_fps = 59.94;      /* seconds since the jingle started */
static double tr_at = -1.0, theme_first, seq_at;
static int seq_started;
static uint8_t intro_ram[0x800];          /* (the game's RAM before it runs: area music) */
static int16_t aud_buf[4096];          /* (stereo: left, right) */

static double smoothd(double t)
{
	if (t <= 0.0) return 0.0;
	if (t >= 1.0) return 1.0;
	return t * t * (3.0 - 2.0 * t);
}

int smb_sound_setup(const u8 *ipl, u32 size, int hz50)
{
	double frame_hz = hz50 ? 50.0 : 59.94;
	if (ipl_bank_load(&bank, ipl, size) != 0)
		return -1;
	/* one video frame of samples per NES frame: produced slightly faster than
	   played at 60 Hz (the NES runs 60.1 frames/s of game per 59.94 shown) */
	smb_sound_init(&snd, &bank, SMB_AUDIO_HZ * 60.0988 / frame_hz);
	snd.pitch_rate = SMB_AUDIO_HZ;
	snd.map[SND_SQ1].wave = 8;
	snd.map[SND_SQ2].wave = 8;
	snd.map[SND_TRI].wave = 11;
	snd.map[SND_NOISE].wave = SND_KIT;
	snd.map[0].gain = snd.map[1].gain = snd.map[2].gain = snd.map[3].gain = 2.0f;
	smb_sound_make_drum(&snd, 0, 140.0f, 150.0f, 4000.0f, 0.7f);   /* (softened, with some punch) */
	snd.kit_hi = snd.kit_lo = SND_DRUM;
	snd.kit_lo_transpose = -7;
	/* stereo: harmony left, melody a little right, drums right, bass in the middle;
	   a 1.8 s reverb on the music (the sound effects stay dry) */
	snd.map[SND_SQ1].pan = 0.1f;
	snd.map[SND_SQ2].pan = -0.4f;
	snd.map[SND_NOISE].pan = 0.3f;
	smb_sound_reverb(&snd, 1.8f, 1.28f);
	snd.music_gain = 0.0f;
	intro_ram[0xf4] = 0x01;                /* AreaMusicBuffer: the overworld theme */
	aud_fps = frame_hz;
	snd_ready = 1;
	return 0;
}

/* one presented frame of sound: the intro transition's fades and tempo, then the
   samples (with the game's frame if it ran) */
static void audio_tick(void)
{
	double t;
	int n;
	if (!snd_on)
		return;
	t = aud_t;
	if (fan_mode)
		goto out;
	/* the jingle fades out */
	if (jingle_volume > 0) {
		double g = 1.0 - smoothd((t - tr_at) / JINGLE_FADE_LEN);
		sound_set_volume((int)(jingle_volume * g + 0.5));
	}
	/* the theme comes in on the jingle's beat once the recording has begun */
	if (!seq_started && t >= seq_at && mus.nframes > THEME_LEAD_IN) {
		smb_seq_start(&seq, &mus, 0.0);
		seq.loop_from = SMB_MUSIC_LOOP_FROM;
		smb_sound_use_seq(&snd, &seq);
		seq_started = 1;
	} else if (!seq_started && t >= seq_at) {
		/* not recorded yet: the next beat */
		theme_first += 2.0 * JINGLE_EIGHTH;
		seq_at = theme_first - THEME_LEAD_IN / 60.0988 / THEME_TEMPO0;
	}
	if (seq_started) {
		snd.seq_rate = THEME_TEMPO0 + (1.0 - THEME_TEMPO0) * smoothd((t - seq_at) / THEME_RAMP_LEN);
		snd.music_gain = (float)(THEME_LEVEL0 + (1.0 - THEME_LEVEL0) *
		                         smoothd((t - (tr_at + THEME_IN_AFTER)) / THEME_FADE_LEN));
	}
out:
	n = smb_sound_frame(&snd, nes_ran ? nes.ram : intro_ram, nes_ran ? nes.cycles : 0,
	                    aud_buf, (int)(sizeof(aud_buf) / sizeof(aud_buf[0])));
	nes_ran = 0;
	smb_audio_push(aud_buf, n);
	aud_t += 1.0 / aud_fps;
}

void smb_intro_audio(int jingle_frames, int armed, int volume)
{
	if (!snd_ready || !have_rom)
		return;
	if (!snd_on) {
		double t = jingle_frames / aud_fps;
		if (!armed || jingle_frames <= 0)
			return;
		/* the transition starts at 1.5 s (or now, if B came later) */
		if (t < JINGLE_FADE_AT)
			return;
		aud_t = t;
		tr_at = t;
		theme_first = JINGLE_BEAT0;
		while (theme_first < tr_at + THEME_IN_AFTER)
			theme_first += JINGLE_EIGHTH;
		seq_at = theme_first - THEME_LEAD_IN / 60.0988 / THEME_TEMPO0;
		jingle_volume = volume;
		aud_volume = (int)(volume / 0.781 + 0.5);   /* (the jingle plays at the IPL's 0.781) */
		if (smb_audio_start(aud_volume) != 0)
			return;
		snd_on = 1;
	}
	audio_tick();
}

void smb_sound_end(void)
{
	if (snd_on) {
		smb_audio_stop();
		snd_on = 0;
	}
}

void smb_set_save_dir(const char *dir)
{
	snprintf(sav_path, sizeof(sav_path), "%.140ssmb.sav", dir);
	snprintf(log_path, sizeof(log_path), "%.140ssmb_runs.txt", dir);
	smb_records_load(&recs, sav_path);
}

/* the end of a run: the records and run log are updated in memory now and written
   when the mode hands back (the cards are not kept mounted during the animation: they
   are mounted for the write, then unmounted, which flushes everything); not for a test
   start. Shown under the logo: one line, the run's minimum */
static SmbText result;
static SmbText result_score, result_rest;   /* (the 1-1 flagpole: see run_ended) */
#define VFW_N     6      /* the end of the game: fireworks around the logo (decoration: no points) */
#define VFW_T0    40     /* the first one (frames after the landing) */
#define VFW_EVERY 36     /* then one every */
#define VFW_LEN   24     /* each one's explosion: 3 pictures of 8 frames, then its sound */
#define RES_RISE 45                       /* result: frames to rise into place */
#define RES_HOLD 105                      /* then frames shown before the fade-out */
#define LB_HIST 8                         /* frames of lettering kept clear (its trails) */
static float res_px = 1.5f;               /* its size (view units per pixel) */
static int save_pending;
static SmbRun save_run;
static char save_date[24];

static int save_flush(void);

static void run_ended(int finished, uint32_t frames, int ax, int ay)
{
	const char *lines[3];
	int news = 0, nl = 1, k;
	SmbRecord prev;
	memset(&save_run, 0, sizeof(save_run));
	save_run.mode = flow.full ? SMB_REC_FULL : SMB_REC_11;
	save_run.finished = finished;
	save_run.score = smb_score_of(nes.ram);
	save_run.frames = frames;
	save_run.world = nes.ram[SMB_WORLD] + 1;
	save_run.level = nes.ram[SMB_LEVEL] + 1;
	if (finished && !flow.full)
		save_run.world = save_run.level = 1;   /* (1-1: the game may have gone on to 1-2) */
	save_run.ax = ax;
	save_run.ay = ay;
	save_run.original_colours = orig_on;
	prev = recs.r[save_run.mode];             /* (the best before this run: shown under it) */
	/* (no smb.nes folder: shown as NOT SAVED 1) */
	{
		time_t now = time(NULL);
		struct tm *tm = localtime(&now);
		save_date[0] = 0;
		if (tm)
			strftime(save_date, sizeof(save_date), "%Y-%m-%d %H:%M", tm);
		news = smb_records_add(&recs, &save_run);
		save_pending = 1;
	}
	/* this run, then the best before it (smb_result_table) */
	{
		char status[16] = "";
		static char rows[3][64];
		int err = save_flush();           /* (saved now: STOP or RESET may come before the end) */
		if (err)
			snprintf(status, sizeof(status), "NOT SAVED %d", err);
		else if (((news & 1) && prev.have_score) || ((news & 2) && prev.have_time))
			snprintf(status, sizeof(status), "NEW BEST");   /* (not on the very first run) */
		nl = smb_result_table(&save_run, flow.full, status, &prev, rows);
		for (k = 0; k < nl; k++)
			lines[k] = rows[k];
		/* the 1-1 flagpole: the score alone (it is already up, counting) and the rest
		   of the table with the score's place left blank, to come in around it */
		{
			static char rest0[64], sc[8];
			const char *rl[3], *one[1] = {sc};
			memcpy(sc, rows[0], 6);
			sc[6] = 0;
			snprintf(rest0, sizeof(rest0), "      %.57s", rows[0] + 6);
			rl[0] = rest0;
			for (k = 1; k < nl; k++)
				rl[k] = rows[k];
			smb_text_lines(&nes, one, 1, &result_score);
			smb_text_lines(&nes, rl, nl, &result_rest);
		}
	}
	smb_text_lines(&nes, lines, nl, &result);
}

/* write smb.sav and append to smb_runs.txt (mounting their card for it) */
/* writes smb.sav and appends to smb_runs.txt (mounting the card for it: main has
   unmounted everything before the animation); 0 or an error code shown with the result:
   1 no SD/USB path, 2 card not mounted, 3 smb.sav not written, 4 smb_runs.txt not written */
static int save_flush(void)
{
	int sd, err = 0, tries;
	const char *dev;
	if (!save_pending)
		return 0;
	save_pending = 0;
	sd = !strncmp(sav_path, "sd:", 3);
	if (!sd && strncmp(sav_path, "usb:", 4))
		return 1;
	dev = sd ? "sd" : "usb";
	for (tries = 0; tries < 3; tries++) {
		if (sd ? fatMountSimple("sd", &__io_wiisd) : fatMountSimple("usb", &__io_usbstorage))
			break;
		fatUnmount(dev);               /* (a stale mount, if any) */
		usleep(100000);
	}
	if (tries == 3)
		return 2;
	if (smb_records_save(&recs, sav_path) != 0)
		err = 3;
	if (smb_runs_log(log_path, &save_run, save_date[0] ? save_date : NULL) != 0 && !err)
		err = 4;
	fatUnmount(dev);
	return err;
}

int smb_original_unlocked(void)
{
	return recs.unlocked;
}

void smb_set_original_colours(int on)
{
	orig_on = on && smb_original_unlocked();
}

void smb_set_options(const Config *cfg)
{
	flow.full = cfg->smb_full;
}

u8 *smb_load_rom(const char *path, u32 *size)
{
	FILE *f = fopen(path, "rb");
	long n;
	u8 *b;
	if (!f)
		return NULL;
	fseek(f, 0, SEEK_END);
	n = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (n <= 16 || n > 1024 * 1024) {
		fclose(f);
		return NULL;
	}
	b = malloc((size_t)n);
	if (b && fread(b, 1, (size_t)n, f) != (size_t)n) {
		free(b);
		b = NULL;
	}
	fclose(f);
	*size = (u32)n;
	return b;
}

/* power-on -> Start pressed on the title screen (never shown) -> first frame of play.
 * About 195 NES frames (~0.9 s of emulation on the Wii): done in a low-priority thread
 * that only runs while the animation's own thread waits (GPU, vertical retrace), so the
 * animation never loses a frame to it. */
static lwp_t boot_thread = LWP_THREAD_NULL;
static volatile int boot_running;

static void boot_frames(int n)
{
	while (have_rom && !booted && n-- > 0) {
		nes_set_pad(&nes, 0, smb_flow_boot_pad(&flow, boot_frame));
		nes_run_frame(&nes);
		boot_frame++;
		if (smb_flow_boot_after(&nes, &flow, boot_frame))
			booted = 1;
		else if (boot_frame > 1200 || nes.jammed)
			have_rom = 0;
	}
}

static volatile int boot_ready;

/* booted: start recording the theme from a copy of the game (before anything else
   touches it), then say so; the recording goes on in this thread */
static void after_boot(void)
{
	if (booted && snd_ready) {
		smb_music_begin(&mus, &nes, SMB_MUSIC_LOOP_FROM + SMB_MUSIC_LOOP_LEN, 65536);
		/* the level-clear fanfare: EventMusicQueue ($FC) = EndOfLevelMusic ($20),
		   recorded until it is over (324 frames) */
		if (smb_music_begin(&fan, &nes, 600, 8192) == 0) {
			fan.game->ram[0xfc] = 0x20;
			fan.until_event_end = 1;
		}
	}
	boot_ready = 1;
}

static void *boot_entry(void *arg)
{
	(void)arg;
	boot_frames(2000);
	after_boot();
	while (smb_music_step(&fan, 16))
		;
	while (smb_music_step(&mus, 16))
		;
	boot_running = 0;
	return NULL;
}

/* CRC-32 (the zip / No-Intro one) */
static u32 crc32_buf(const u8 *p, u32 n)
{
	u32 c = 0xffffffffu;
	int k;
	while (n--) {
		c ^= *p++;
		for (k = 0; k < 8; k++)
			c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1u)));
	}
	return ~c;
}

/* only Super Mario Bros. itself (World / Japan, the same ROM: No-Intro CRC-32 of its
   32 KiB PRG + 8 KiB CHR, without the iNES header): the mode reads the game's RAM
   (SMB's own addresses) and plays its 1-1, so a hack, the PAL release or any other
   game is not used - the animation then goes on as without a ROM */
#define SMB_ROM_CRC 0xd445f698u
static int is_smb(const u8 *rom, u32 size)
{
	u32 off = 16 + ((size >= 16 && (rom[6] & 4)) ? 512 : 0);
	if (size < off + 40960 || memcmp(rom, "NES\x1a", 4) != 0)
		return 0;
	return crc32_buf(rom + off, 40960) == SMB_ROM_CRC;
}

int smb_prepare(u8 *rom, u32 rom_size)
{
	have_rom = is_smb(rom, rom_size) && nes_load(&nes, rom, rom_size) == 0;
	if (have_rom) {
		nes_power(&nes);
		smb_floor_install(&nes);           /* (the floors, as the game builds the level) */
	}
	booted = 0;
	boot_frame = 0;
	boot_ready = 0;
	if (!have_rom)
		return -1;
	boot_running = 1;
	if (LWP_CreateThread(&boot_thread, boot_entry, NULL, NULL, 64 * 1024, 8) != 0) {
		boot_thread = LWP_THREAD_NULL;     /* no thread: booted in smb_play instead */
		boot_running = 0;
	}
	return 0;
}

/* (libogc frees a thread as soon as it returns: joining it afterwards can wait on
   whatever thread was given the same handle since - wait for it to say it is done) */
void smb_boot_wait(void)
{
	if (boot_thread != LWP_THREAD_NULL) {
		while (boot_running)
			usleep(1000);
		boot_thread = LWP_THREAD_NULL;
	}
}

void smb_boot_step(int n)
{
	if (boot_thread == LWP_THREAD_NULL)
		boot_frames(n);
}

int smb_branch_point(const IntroState *st)
{
	return have_rom && st->state == 1 && st->face == 2 && st->gx == 18 && st->gy == 18 && st->nlaid == 12;
}

/* Wii Remote 1 held sideways (d-pad on the left) -> NES pad: d-pad as seen that way,
   1 = B (run), 2 = A (jump), + = Start */
static u8 read_wpad(u32 w)
{
	u8 b = 0;
	if (w & WPAD_BUTTON_2) b |= NES_A;
	if (w & WPAD_BUTTON_1) b |= NES_B;
	if (w & WPAD_BUTTON_PLUS) b |= NES_START;
	if (w & WPAD_BUTTON_MINUS) b |= NES_SELECT;
	if (w & WPAD_BUTTON_RIGHT) b |= NES_UP;
	else if (w & WPAD_BUTTON_LEFT) b |= NES_DOWN;
	if (w & WPAD_BUTTON_UP) b |= NES_LEFT;
	else if (w & WPAD_BUTTON_DOWN) b |= NES_RIGHT;
	return b;
}

/* GameCube pad 1 -> NES pad */
static u32 pads_connected;

static u8 read_pad(u32 held)
{
	s8 sx, sy, cx, cy;                        /* (from the stick's real centre) */
	u8 b = 0;
	pad_raw_sticks(0, pads_connected, &sx, &sy, &cx, &cy);
	(void)cx;
	(void)cy;
	if (held & PAD_BUTTON_A) b |= NES_A;
	if (held & PAD_BUTTON_B) b |= NES_B;
	if (held & PAD_BUTTON_START) b |= NES_START;
	if (held & PAD_TRIGGER_Z) b |= NES_SELECT;
	if ((held & PAD_BUTTON_UP) || sy > 40) b |= NES_UP;
	else if ((held & PAD_BUTTON_DOWN) || sy < -40) b |= NES_DOWN;
	if ((held & PAD_BUTTON_LEFT) || sx < -40) b |= NES_LEFT;
	else if ((held & PAD_BUTTON_RIGHT) || sx > 40) b |= NES_RIGHT;
	return b;
}

static float smooth(float t)
{
	if (t <= 0.0f) return 0.0f;
	if (t >= 1.0f) return 1.0f;
	return t * t * (3.0f - 2.0f * t);
}

/* the NES x where the level stops being built: just behind the top cube */
static int build_clip(void)
{
	int32_t x = (view.ground_limit[13] + 1) * 16 - view.level_x;
	if (x > SMB_VIEW_W) x = SMB_VIEW_W;
	if (x < 0) x = 0;
	return (int)x;
}

/* solve M * p = c for p (M: 3x4 affine) */
static void inv_point(Mtx M, const float c[3], float p[3])
{
	Mtx inv;
	guMtxInverse(M, inv);
	gm_multvec(inv, c, p);
}

/* one frame: the animation's scene (if any) shifted by io, the level by lo (view units) */
/* face on, over the level while playing (the whole game): the text lines the camera
   cannot show (world screen with the score, Toad's and Peach's messages, the warp zone),
   and the fireworks after the flagpole, both brought into the picture */
static SmbText ovl_text;
static struct {
	int text_on, nfw, alpha;
	float text_x, text_y, text_px, fwp[FW_MAX_PLAY][2];
	u8 rgb[3];
	int icon_on;                          /* the world screen's Mario */
	float icon_x, icon_y, icon_px;
} ovl;
/* the score in the top-left corner: while he plays, and after the 1-1 flagpole while the
   game counts the time and the fireworks into it (until the result is up) */
static struct {
	int on, alpha;
	float x, y, h;
	uint8_t bits[96 * 8];
	u8 rgb[3];
} hud;

/* the corner's line: the score and, after a clock, the game's time (GameTimerDisplay
   $07F8-$07FA), as the game has them */
static void hud_build(void)
{
	static SmbText ht;
	char line[24];
	const char *one[1] = {line};
	int r, c;
	snprintf(line, sizeof(line), "%06lu  @%d%d%d", (unsigned long)smb_score_of(nes.ram),
	         nes.ram[0x7f8] % 10, nes.ram[0x7f9] % 10, nes.ram[0x7fa] % 10);
	smb_text_lines(&nes, one, 1, &ht);
	for (r = 0; r < 8; r++)
		for (c = 0; c < 96; c++)
			hud.bits[r * 96 + c] = ht.bits[r * SMB_TEXT_W + c];
}

static void hud_draw(void)
{
	if (hud.on && hud.alpha > 0)
		smb_render_hud(hud.bits, hud.x, hud.y, hud.h, hud.rgb, hud.alpha);
}

static void frame_passes_off(Video *vid, const IntroFrame *cam, const SmbDraw *d, const IntroFrame *intro,
                             const float io[2], const float lo[2])
{
	int pass, i;
	for (pass = 0; pass < 2; pass++) {
		video_begin_pass(vid, pass);
		if (intro) {
			video_set_offset(vid, io[0], io[1]);
			scene_draw(intro);
		}
		video_set_offset(vid, lo[0], lo[1]);
		smb_render_draw(cam, &view, d);
		video_set_offset(vid, 0.0f, 0.0f);
		for (i = 0; i < ovl.nfw; i++)
			smb_render_sprite_view(i, ovl.fwp[i][0], ovl.fwp[i][1], 2.0f, ovl.alpha);
		if (ovl.text_on)
			smb_render_text(&ovl_text, ovl.text_x, ovl.text_y, ovl.text_px, ovl.rgb, ovl.alpha);
		if (ovl.icon_on)
			smb_render_icon_view(ovl.icon_x, ovl.icon_y, ovl.icon_px, 255);
		hud_draw();
		video_end_pass(vid, pass);
	}
	audio_tick();
	video_present(vid);
}

/* the logo (its laid tiles) in view space: x0, x1, y0, y1 */
static void logo_bounds(const IntroFrame *f, float b4[4])
{
	float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
	int i, c;
	for (i = 0; i < f->ntiles; i++) {
		const IntroTile *t = &f->tile[i];
		float r28 = (float)((t->x - 16) * 54), r31 = (float)((t->y - 16) * 54);
		float a[2] = {-r28, -(r28 + 54.0f)}, b[2] = {-r31, -(r31 + 54.0f)};
		for (c = 0; c < 4; c++) {
			float q[3], o[3], aa = a[c & 1], bb = b[c >> 1];
			if (t->face == 0) { q[0] = aa; q[1] = 0; q[2] = bb; }
			else if (t->face == 1) { q[0] = bb; q[1] = aa; q[2] = 0; }
			else { q[0] = 0; q[1] = bb; q[2] = aa; }
			gm_multvec((MtxP)f->tiles, q, o);
			if (o[0] < x0) x0 = o[0];
			if (o[0] > x1) x1 = o[0];
			if (o[1] < y0) y0 = o[1];
			if (o[1] > y1) y1 = o[1];
		}
	}
	b4[0] = x0; b4[1] = x1; b4[2] = y0; b4[3] = y1;
}

static int fw_forced;
static int death_elapsed;                 /* game frames of its death music so far */

/* after his death the game goes on unseen until its death music is over (SMB RAM
   EventMusicBuffer $07B1 = DeathMusic 1): heard to its end, the channels then quiet
   (stopping the game mid-tune left its notes held). Returns 0 once over. */
static int death_music_step(void)
{
	if (nes.ram[0x7b1] != 0x01 || (nes.ram[RAM_ENGINE] != ENG_DEATH && nes.ram[RAM_ENGINE] != ENG_PLAY))
		return 0;
	nes_set_pad(&nes, 0, 0);
	nes_run_frame(&nes);
	nes_ran = 1;
	death_elapsed++;
	return 1;
}

/* the flagpole fall: gravity, top speed (view units, per frame) and the fanfare's tempo
   during it (1.5x: the fall is slowed to last as long) */
#define FLAG_G 0.6f
#define FLAG_TOP 22.0f
#define FANFARE_TEMPO 1.5
#define DEATH_G 0.4f
#define DEATH_TOP 14.0f
static float fall_scale = 1.0f;

/* the hold for the flagpole: never up on the screen (no camera ease upwards) - he
   is held where he is if that is below the 20 % mark; the logo then stops with the
   cube's top right there and glides to its place with him once he is on it */
static float flag_hold(float hold, float y_start)
{
	return y_start < hold ? y_start : hold;
}

/* frames of the fall until he is on the cube, worked out ahead with the same steps
   as the fall loop (nothing from the game in them): gravity g, top speed top; flag:
   the flagpole's rules (follow once falling, flag_hold), else the death's (follow
   at once, logo_shift as given) */
static int fall_frames(int flag, float vel, float g, float top, float base, float ly, float hold,
                       float dist, float cube_top, float logo_shift, float screen_b)
{
	float v2 = vel, m2 = 0.0f, l2 = ly, ys = 0.0f, uu, hf = hold, ls = logo_shift;
	int fo = -1, t2;
	for (t2 = 0; t2 < 1200; t2++) {
		float py, sy2;
		v2 -= g;
		if (v2 < -top) v2 = -top;
		m2 += v2;
		py = base + m2;
		sy2 = py + l2;
		if (fo < 0 && (sy2 < screen_b + 90.0f || !flag || v2 < 0.0f)) {
			fo = t2;
			ys = sy2;
			if (flag) {
				hf = flag_hold(hold, ys);
				ls = cube_top - hf;
			}
		}
		uu = 0.0f;
		if (fo >= 0) {
			uu = ((float)(t2 - fo) / 45.0f);
			uu = uu <= 0.0f ? 0.0f : uu >= 1.0f ? 1.0f : uu * uu * (3.0f - 2.0f * uu);
			l2 = ys + (hf - ys) * uu - py;
			if (l2 > dist) l2 = dist;
		}
		if (fo >= 0 && py + l2 <= cube_top - ls + l2 - dist)
			break;                                /* (as in the fall loop) */
	}
	return t2;
}

/* the gravity scale (<= 1) that makes the fall last `want` frames (as close as it
   gets from below), 1 if it is already as long */
static float fall_fit(int flag, float vel, float g, float top, float base, float ly, float hold,
                      float dist, float cube_top, float logo_shift, float screen_b, int want, int *frames)
{
	float lo_s = 0.15f, hi_s = 1.0f, sc;
	int it, t2 = fall_frames(flag, vel, g, top, base, ly, hold, dist, cube_top, logo_shift, screen_b);
	if (t2 + 1 < want) {
		for (it = 0; it < 24; it++) {
			sc = 0.5f * (lo_s + hi_s);
			if (fall_frames(flag, vel, g * sc, top * sc, base, ly, hold, dist, cube_top, logo_shift, screen_b) + 1 > want)
				lo_s = sc;
			else
				hi_s = sc;
		}
		t2 = fall_frames(flag, vel, g * hi_s, top * hi_s, base, ly, hold, dist, cube_top, logo_shift, screen_b);
		*frames = t2;
		return hi_s;
	}
	*frames = t2;
	return 1.0f;
}

/* after the flagpole the game goes on unseen: Mario walks into the (hidden) castle,
 * the time is counted into points, 3 fireworks. One frame of it; *quiet counts the
 * frames since all of that is over; returns 0 once the game moves on (next level). */
/* the 1-1 flagpole: the game is held once its count is over (time at 0), before its
   fireworks, until the animation is over and the score is in its place under the
   logo (released by the end's loop) */
static int fw_hold;

static int castle_step(int ending_mode, int *quiet)
{
	int busy, k;
	if (fw_hold && nes.ram[RAM_ENGINE] == ENG_ENDLEVEL &&
	    !(nes.ram[0x7f8] | nes.ram[0x7f9] | nes.ram[0x7fa])) {
		*quiet = 0;                           /* (not over: waiting; a frame without the
		                                         game, as after it: silence) */
		return 1;
	}
	nes_set_pad(&nes, 0, 0);
	nes_run_frame(&nes);
	nes_ran = 1;
	if (nes.ram[RAM_ENGINE] == ENG_ENDLEVEL && !fw_forced && (nes.ram[0x6d7] == 0xff || nes.ram[0x6d7] == 1)) {
		nes.ram[0x6d7] = 3;                   /* FireworksCounter: the game said none (or 1) */
		fw_forced = 1;
	}
	if (nes.ram[RAM_ENGINE] != ENG_ENDLEVEL || nes.ram[RAM_OPERMODE] != 1)
		return 0;
	busy = nes.ram[0x7f8] | nes.ram[0x7f9] | nes.ram[0x7fa] | nes.ram[0x6d7];
	for (k = 0; k < 6; k++)
		if (nes.ram[0x0f + k] && nes.ram[0x16 + k] == 0x16)
			busy = 1;                         /* a firework */
	*quiet = busy ? 0 : *quiet + 1;
	lcfg.ending = ending_mode;
	smb_layout(&nes, &lcfg, &view);
	lcfg.ending = 0;
	return 1;
}

/* one game frame, unseen (the end of the game: its music plays on) */
static void game_step(void)
{
	nes_set_pad(&nes, 0, 0);
	nes_run_frame(&nes);
	nes_ran = 1;
}

/* the result's x (the 1-1 flagpole: the score sliding to its column) and the rest of
   its table coming in around it */
static float fin_score_x;
static const SmbText *fin_ext;
static int fin_ext_alpha;
/* the 1-1 flagpole's score (6 digits), in a small texture of its own: it changes every
   frame (see smb_render_score_slot) */
static int fin_digits_on;
static uint8_t fin_digits[48 * 8];

static void digits_of(const SmbText *t, uint8_t out[48 * 8])
{
	int r, c;
	for (r = 0; r < 8; r++)
		for (c = 0; c < 48; c++)
			out[r * 48 + c] = t->bits[r * SMB_TEXT_W + c];
}

/* the end: the animation, the player (d, drawn without offset), the score, fireworks */
static void frame_passes_final(Video *vid, const IntroFrame *f, const SmbDraw *d, float ox, float oy, const SmbText *score,
                               float sy, int salpha, const u8 srgb[3], int nfw, const float fwp[][2],
                               const int *fwi, float fwpx, int fwa)
{
	int pass, i;
	for (pass = 0; pass < 2; pass++) {
		video_begin_pass(vid, pass);
		video_set_offset(vid, ox, oy);
		scene_draw(f);
		smb_render_draw(f, &view, d);
		video_set_offset(vid, 0.0f, 0.0f);
		if (fin_digits_on && salpha > 0)
			smb_render_score_slot(1, fin_digits, fin_score_x, sy - 4.0f * res_px, 8.0f * res_px, srgb, salpha);
		else if (score && salpha > 0)
			smb_render_text(score, fin_score_x, sy, res_px, srgb, salpha);   /* (sy: its top) */
		if (fin_ext && fin_ext_alpha > 0)
			smb_render_text_slot(1, fin_ext, 0.0f, sy, res_px, srgb, fin_ext_alpha);
		for (i = 0; i < nfw; i++)
			smb_render_sprite_view(fwi[i], fwp[i][0], fwp[i][1], fwpx, fwa);
		hud_draw();
		video_end_pass(vid, pass);
	}
	audio_tick();
	video_present(vid);
}

/* the player's feet in view space, drawn with matrix m (no offset) */
static void player_view_m(Mtx m, int32_t level_x, float out[3])
{
	float feet[3];
	smb_render_player_point(&view, level_x, feet);
	gm_multvec(m, feet, out);
}

static int smb_play_run(Video *vid, const Config *cfg, IntroState *st, int have_sound,
                        volatile int *reset_flag, volatile int *power_flag);

static smb_shadow_t shadow;

static void shadow_hook(void *user, const nes_t *n, uint16_t pc)
{
	(void)pc;
	smb_shadow_engine((smb_shadow_t *)user, n);
}

int smb_play(Video *vid, const Config *cfg, IntroState *st, int have_sound,
             volatile int *reset_flag, volatile int *power_flag)
{
	int r = smb_play_run(vid, cfg, st, have_sound, reset_flag, power_flag);
	nes.pc_hook2 = NULL;
	smb_sound_use_shadow(&snd, NULL);
	smb_shadow_free(&shadow);
	return r;
}

static int smb_play_run(Video *vid, const Config *cfg, IntroState *st, int have_sound,
                        volatile int *reset_flag, volatile int *power_flag)
{
	static IntroFrame cam, f, af;
	static IntroState apex, sim;
	SmbDraw d;
	float logo_b[4] = {0.0f, 0.0f, 0.0f, 0.0f};   /* logo x0, x1, y0, y1 (view) */
	int t = 0, quit_hold = 0, hz = vid->hz50 ? 50 : 60, ending = 0, k, released = 0;
	float tdx, tdy, play_ox = 0.0f, play_oy = 0.0f, ground_oy = 0.0f;
	int area0, other_area = 0, been_away = 0, ply_n_ref = 0;
	int area_id, left_start = 0, snap_cam = 0, new_area = 0, fade = 255;
	uint32_t run_frames = 0;                  /* NES frames since Mario could move */
	int run_ax, run_ay;
	int32_t prev_level_x = 0;
	float feet[3], jump_from[3] = {0, 0, 0};

	(void)cfg;
	(void)have_sound;
	if (boot_thread != LWP_THREAD_NULL) {
		while (!boot_ready && boot_running)
			usleep(2000);                     /* (booting: the recording may go on) */
	} else {
		boot_frames(2000);
		after_boot();
		while (smb_music_step(&fan, 64))
			;
		while (smb_music_step(&mus, 64))
			;
	}
	if (!have_rom || !booted)
		return 0;                             /* not playable: the animation goes on */

	/* where the jingle is (to play its end when Mario lands on the cube) */
	jingle_start_frame = snd_on ? st->frame - (int)(aud_t * aud_fps + 0.5) : -1;
	fan_mode = 0;
	/* its sound: the APU writes go to the SMB sound (sound effects, live music) */
	nes.apu_write_cb = smb_sound_write;
	nes.cb_user = &snd;
	/* its music from a shadow of its sound engine that never plays an effect: no
	   note of the music left out under Mario's jumps, stomps and bumps */
	if (smb_shadow_begin(&shadow, &nes) == 0) {
		nes.pc_hook2 = shadow_hook;
		nes.pc_hook2_at = 0xf2d0;             /* SoundEngine */
		nes.pc_hook2_user = &shadow;
		smb_sound_use_shadow(&snd, &shadow);
	}
	/* the animation's camera and trail at the branch frame */
	intro_frame(st, &cam);
	scene_prepare(&cam);
	smb_render_init();
	smb_render_set_colours(orig_on);
	smb_layout_defaults(&lcfg);
	lcfg.hide_castle = !flow.full;            /* (1-1 mode: the logo stands in for it) */
	lcfg.text_out = 1;                        /* (text lines and fireworks: face on) */
	lcfg.fw_out = flow.full;
	memset(&ovl, 0, sizeof(ovl));
	memset(&hud, 0, sizeof(hud));
	scene_theme_rgb(ovl.rgb);                 /* (the score's colour under the logo) */
	for (k = 0; k < 3; k++)
		ovl.rgb[k] = (u8)((ovl.rgb[k] + 2 * 255) / 3);
	smb_layout_reset();
	memset(&jump_ply, 0, sizeof(jump_ply));
	fw_forced = 0;
	death_elapsed = 0;
	smb_cubes_reset(&cubes);
	cubes.start_col = -1;                     /* level column -1, row 13 = the trail's tile (18,18) */
	cubes.roll_frames = st->p->n14;           /* the animation's own rolling pace */
	cubes.drop_frames = 8;
	cubes.push = 0;
	smb_layout(&nes, &lcfg, &view);
	smb_cubes(&nes, &lcfg, &view, &cubes);

	/* the animation at the top of the cube's jump: the logo that stands in for the
	   castle, and the one Mario falls onto when he dies */
	sim = *st;
	apex = sim;
	for (k = 0; k < 600; k++) {
		intro_update(&sim);
		if (sim.finished || sim.state >= 3)
			break;
		apex = sim;
	}
	intro_frame(&apex, &af);
	scene_prepare(&af);                       /* (the scene's models stay set for it) */
	logo_bounds(&af, logo_b);

	memset(&d, 0, sizeof(d));
	d.trail = &cam;
	area0 = nes.ram[0x750];
	/* the C-stick view, kept for the whole game: the stick's values at launch (kept
	   with the records) */
	run_ax = (int)st->cam_sx;
	run_ay = (int)st->cam_sy;
	area_id = smb_flow_area_id(&nes);
	left_start = flow.full && !smb_flow_in_1_1(&nes);
	d.level_alpha = 255;
	d.show_cubes = 1;
	d.ply_scale = 1.0f;
	d.ply_alpha = 255;

	/* ---------------- branch + play ---------------- */
	while (!*reset_flag && !*power_flag) {
		u32 held, wheld;
		float e;

		pads_connected = PAD_ScanPads();
		held = PAD_ButtonsHeld(0);
		WPAD_ScanPads();
		wheld = WPAD_ButtonsHeld(0);
		/* quit: Start + Z on the pad, or HOME on the Wii Remote, held 1 s */
		if (((held & PAD_BUTTON_START) && (held & PAD_TRIGGER_Z)) || (wheld & WPAD_BUTTON_HOME)) {
			if (++quit_hold > hz)
				return 1;
		} else
			quit_hold = 0;

		if (t >= JUMP_END) {
			/* Mario stays put until the cubes have rolled out of the picture at their
			   own pace; from then on, if he catches up, he pushes them along */
			if (!released && view.cube[0].col > view.level_x / 16 + 16 && view.cube[1].col > view.level_x / 16 + 16)
				released = 1;
			if (left_start)
				released = 1;                     /* (test start elsewhere: no cubes to wait for) */
			cubes.push = released;
			nes_set_pad(&nes, 0, released ? (u8)(read_pad(held) | read_wpad(wheld)) : 0);
			nes_run_frame(&nes);
			nes_ran = 1;
			if (nes.ram[0x7b1] == 0x01)
				death_elapsed++;
			/* one life; what ends the mode: death, the 1-1 flagpole (slid down,
			   flipped round: he hops off now), the end of the game (Peach's message) */
			if (released)
				run_frames++;
			switch (smb_flow_frame(&nes, &flow)) {
			case SMB_EV_DEATH: ending = 1; break;
			case SMB_EV_FLAG: ending = 2; break;
			case SMB_EV_VICTORY: ending = 3; break;
			default: break;
			}
			/* (the 1-1 flagpole: the run's score is known once the game has counted
			   the time and the fireworks into it, under the logo: the result then) */
			if (ending && ending != 2)
				run_ended(ending != 1, run_frames, run_ax, run_ay);
		}
		prev_level_x = view.level_x;
		/* the world screen, and the game's other steps between two areas (the whole
		   game): only the world screen's text (face on) - not the little Mario, its
		   "x 1", nor the next area's floors, already known before it is shown */
		lcfg.no_sprites = flow.full && nes.ram[RAM_OPERMODE] == 1 && nes.ram[SMB_OPERTASK] != 3;
		smb_layout(&nes, &lcfg, &view);
		/* another area (down a pipe: 1-1's underground room, SMB RAM AreaPointer
		   $0750): its level starts at x 0 too, so the logo's trail and the cubes, left
		   of 1-1's start, would show there - not drawn until back in 1-1 */
		/* (the level x goes back to 0 a few frames before the area pointer changes:
		   either hides them; shown again once back in 1-1 from the other area) */
		/* (the whole game: once out of World 1-1, for good) */
		if (flow.full && !smb_flow_in_1_1(&nes))
			left_start = 1;
		if (smb_flow_area_id(&nes) != area_id) {
			area_id = smb_flow_area_id(&nes);
	left_start = flow.full && !smb_flow_in_1_1(&nes);
			if (flow.full) {
				smb_layout_reset();               /* (a new area: its own enemies) */
				new_area = 1;                     /* (and the camera at once, not gliding) */
			}
		}
		if (left_start)
			other_area = 1;
		else if (nes.ram[0x750] != area0) {
			other_area = 1;
			been_away = 1;
		} else if (t > JUMP_END && view.level_x < prev_level_x - 64)
			other_area = 1;
		else if (other_area && been_away && nes.ram[RAM_ENGINE] == ENG_PLAY)
			other_area = been_away = 0;
		if (!other_area)
			smb_cubes(&nes, &lcfg, &view, &cubes);
		d.trail = other_area ? NULL : &cam;
		if (other_area)
			for (k = 0; k < 15; k++)
				view.ground_limit[k] = 0x7fffffff;   /* (the cubes' limit is 1-1's: all of it) */
		d.show_cubes = !other_area;
		if (view.ply_on && nes.ram[RAM_PSTATE] == 1 && nes.ram[RAM_ENGINE] == ENG_PLAY) {
			/* his jumping pose, kept for the leap off the flagpole */
			jump_ply.on = 1;
			jump_ply.x = view.ply_x;
			jump_ply.y = view.ply_y;
			jump_ply.left = nes.ram[0x33] == 2;
			memcpy(jump_ply.px, view.ply, sizeof(jump_ply.px));
		}
		/* his picture as last seen whole: falling into a hole, his sprites go one by
		   one at the bottom (the count of them seen well inside the picture) */
		if (view.ply_on && view.ply_y + 32 <= 200)
			ply_n_ref = view.ply_nspr;
		if (view.ply_on && view.ply_nspr >= ply_n_ref) {
			last_ply.on = 1;
			last_ply.x = view.ply_x;
			last_ply.y = view.ply_y;
			last_ply.level_x = view.level_x;
			memcpy(last_ply.px, view.ply, sizeof(last_ply.px));
		}
		if (ending)
			break;

		/* camera: glide from the logo to the play framing, then follow the scroll */
		smb_render_play_shift(&cam, view.level_x, &tdx, &tdy);
		e = smooth((float)t / (float)PAN_FRAMES);
		d.dx = d.tdx = tdx * e;
		d.dy = d.tdy = tdy * e;
		d.clip = other_area ? SMB_VIEW_W : build_clip();
		smb_render_shifted(&cam, d.dx, d.dy, d.ply_mtx);
		d.ply_level_x = view.level_x;
		d.ply_off[0] = d.ply_off[1] = d.ply_off[2] = 0.0f;
		d.ply_scale = 1.0f;
		d.ply_show = t >= JUMP_START;
		if (t == JUMP_START) {
			/* he jumps out of the top cube, wherever it has rolled to by now */
			const SmbCube *c = &view.cube[0];
			jump_from[0] = TILE * 0.5f;
			jump_from[1] = -((5.0f + (float)c->row + 0.5f) - 16.0f) * TILE;
			jump_from[2] = -((19.0f + (float)c->col + c->roll + 0.5f) - 16.0f) * TILE;
		}
		if (t < JUMP_END) {
			float u = (float)(t - JUMP_START) / (float)(JUMP_END - JUMP_START);
			if (u < 0) u = 0;
			smb_render_player_point(&view, view.level_x, feet);
			/* pops out of the cube, a high arc with a full spin, lands with a squash */
			for (k = 0; k < 3; k++)
				d.ply_off[k] = (jump_from[k] - feet[k]) * (1.0f - u);
			d.ply_off[1] += 3.5f * TILE * 4.0f * u * (1.0f - u);
			d.ply_rot = u < 0.85f ? 2.0f * (float)M_PI * smooth(u / 0.85f) : 0.0f;
			d.ply_scale = u < 0.25f ? 0.3f + 3.4f * u : u < 0.9f ? 1.15f - 0.15f * (u - 0.25f) / 0.65f : 1.0f;
		} else
			d.ply_rot = 0.0f;
		/* the picture's right edge at the screen's right edge (16:9 shows more than
		   the NES picture): things come in from off-screen, as on the NES */
		{
			float edge = smb_render_nes_view_x(&cam, d.dx, d.dy, view.level_x, (float)SMB_VIEW_W);
			play_ox = vid->ortho_r - edge;
			if (play_ox < 0.0f) play_ox = 0.0f;
			play_ox *= e;
		}
		/* up: like a platformer camera, it follows the height he stands at (stairs,
		   high blocks), not his jumps - SMB RAM Player_State $1D = 0: on the ground.
		   In the air it only moves if his head would leave the top of the screen. */
		if (flow.full && new_area && smb_flow_in_play(&nes) &&
		    !(view.ply_on && nes.ram[RAM_YHIGH] == 1)) {
			/* a new area, and he is not in the picture yet (coming up a vine from below,
			   down a pipe): framed on its floor, as if he stood on it */
			int fr = smb_floor_row(&nes, &view);
			if (fr >= 0) {
				float h = vid->ortho_t - vid->ortho_b;
				float feet = smb_render_nes_view_y(&cam, d.dx, d.dy, view.level_x, 64.0f, (float)(fr * 8));
				ground_oy = (vid->ortho_t - 0.40f * h) - feet;
				if (ground_oy > 0.0f) ground_oy = 0.0f;
				play_oy = ground_oy;
				new_area = 0;
				snap_cam = 0;
			}
		} else if (new_area && smb_flow_in_play(&nes)) {
			snap_cam = 1;                         /* (he is there: framed on him at once) */
			new_area = 0;
		}
		if (t >= JUMP_END && view.ply_on && (!flow.full || (smb_flow_in_play(&nes) && nes.ram[RAM_YHIGH] == 1))) {
			float h = vid->ortho_t - vid->ortho_b, want;
			float head = smb_render_nes_view_y(&cam, d.dx, d.dy, view.level_x,
			                                   (float)view.ply_x + 16.0f, (float)view.ply_y);
			float feet = smb_render_nes_view_y(&cam, d.dx, d.dy, view.level_x,
			                                   (float)view.ply_x + 16.0f, (float)view.ply_y + 32.0f);
			if (nes.ram[0x1d] == 0 || nes.ram[0x1d] == 3) {   /* on the ground, or the pole */
				ground_oy = (vid->ortho_t - 0.40f * h) - feet;   /* feet below 40 % */
				if (ground_oy > 0.0f) ground_oy = 0.0f;
			}
			want = (vid->ortho_t - 0.05f * h) - head;            /* head inside */
			if (want > ground_oy) want = ground_oy;
			if (want > 0.0f) want = 0.0f;
			if (snap_cam)
				play_oy = want;                   /* (just arrived in another area) */
			else
				play_oy += (want - play_oy) * 0.06f;
			snap_cam = 0;
		}
		/* text lines: centred, near the top of the screen (the world screen: bigger,
		   with the score under it) */
		{
			int ws = lcfg.no_sprites;
			float sw = vid->ortho_r - vid->ortho_l;
			ovl.text_on = t >= JUMP_END && smb_text(&nes, &view, ws, &ovl_text);
			if (ovl.text_on) {
				ovl.text_px = ws ? 3.0f : 2.0f;
				if (ovl_text.w * ovl.text_px > 0.9f * sw)
					ovl.text_px = 0.9f * sw / (float)ovl_text.w;
				ovl.text_x = 0.5f * (vid->ortho_l + vid->ortho_r);
				ovl.text_y = vid->ortho_t - 0.12f * (vid->ortho_t - vid->ortho_b);
			}
			/* the world screen: Mario under its lines, as he is (small, big, fire),
			   in his own colours (sprite palette 0 set for it: nothing else of the
			   game's sprites is drawn then) */
			ovl.icon_on = 0;
			if (ws && ovl.text_on) {
				static const uint8_t mario_c[3] = {0x16, 0x27, 0x18}, fire_c[3] = {0x37, 0x27, 0x16};
				uint8_t icon[SMB_PLY * SMB_PLY];
				int st_ = nes.ram[0x756];     /* PlayerStatus: 0 small, 1 big, 2 fire */
				memcpy(&view.pal[17], st_ == 2 ? fire_c : mario_c, 3);
				smb_mario_icon(&nes, st_ != 0, icon);
				smb_render_icon_upload(icon);
				ovl.icon_on = 1;
				ovl.icon_px = ovl.text_px * 1.25f;
				ovl.icon_x = ovl.text_x;
				/* the lines and Mario, together, centred on the screen */
				{
					float th = ovl_text.h * ovl.text_px, gap = 8.0f * ovl.text_px;
					float mh = (st_ != 0 ? 32.0f : 16.0f) * ovl.icon_px;   /* (his height) */
					float top = 0.5f * (vid->ortho_t + vid->ortho_b) + 0.5f * (th + gap + mh);
					ovl.text_y = top;
					/* (the icon's texture is 32 high, feet at the bottom) */
					ovl.icon_y = top - th - gap + (32.0f * ovl.icon_px - mh);
				}
			}
			/* the score in the top-left corner while he plays (the game's own status
			   bar is not shown), inside the TV's safe area */
			hud.on = t >= JUMP_END && !ws && nes.ram[RAM_OPERMODE] == 1;
			if (hud.on) {
				float w = vid->ortho_r - vid->ortho_l, h = vid->ortho_t - vid->ortho_b;
				hud.h = 16.0f;                /* (2 view units per NES pixel) */
				hud.alpha = 255;
				memcpy(hud.rgb, ovl.rgb, 3);
				hud_build();
				hud.x = vid->ortho_l + 0.07f * w + hud.h * 6.0f;   /* (12 characters) */
				hud.y = vid->ortho_t - 0.07f * h - hud.h * 0.5f;
			}
		}
		/* fireworks (the whole game: after a flagpole): over the castle, brought down
		   into the picture if the sky above it is out of it */
		ovl.nfw = 0;
		if (flow.full && nes.ram[RAM_OPERMODE] == 1)
			for (k = 0; k < 6 && ovl.nfw < FW_MAX_PLAY; k++) {
				uint8_t fw_img[SMB_GW * SMB_GH];
				int bx, by;
				float p[3], sky[3], lim = vid->ortho_t - 40.0f;
				if (!nes.ram[0x0f + k] || nes.ram[0x16 + k] != 0x16)
					continue;
				if (!smb_slot_image_at(&nes, k, fw_img, &bx, &by))
					continue;
				smb_render_nes_view_point(&cam, d.dx, d.dy, view.level_x, (float)bx + 8.0f, (float)by + 12.0f, p);
				smb_render_nes_view_point(&cam, d.dx, d.dy, view.level_x, (float)bx + 8.0f, (float)SMB_VIEW_Y0, sky);
				p[0] += play_ox; p[1] += play_oy; sky[1] += play_oy;
				if (sky[1] > lim)
					p[1] -= sky[1] - lim;
				if (p[0] < vid->ortho_l + 30.0f) p[0] = vid->ortho_l + 30.0f;
				if (p[0] > vid->ortho_r - 30.0f) p[0] = vid->ortho_r - 30.0f;
				smb_render_sprite_upload(ovl.nfw, fw_img);
				ovl.fwp[ovl.nfw][0] = p[0];
				ovl.fwp[ovl.nfw][1] = p[1];
				ovl.nfw++;
			}
		/* falling off a cloud area: the picture fades out as he leaves it, and back
		   in with the area below (where he drops in from the top) */
		if (flow.full && nes.ram[SMB_CLOUD] && nes.ram[RAM_YHIGH] >= 2 && nes.ram[RAM_YHIGH] < 0x80) {
			fade -= 24;
			if (fade < 0) fade = 0;
		} else if (fade < 255 && !new_area && smb_flow_in_play(&nes)) {
			fade += 16;
			if (fade > 255) fade = 255;
		}
		d.level_alpha = fade;
		ovl.alpha = d.level_alpha;
		{
			float io0[2] = {0.0f, 0.0f}, lo0[2] = {play_ox, play_oy};
			smb_render_prepare(&view);
			frame_passes_off(vid, &cam, &d, NULL, io0, lo0);
		}
		t++;
	}
	memset(&ovl, 0, sizeof(ovl));               /* (the endings: none of it) */
	hud.on = hud.on && ending == 2;              /* (but the score after the 1-1 flagpole) */
	fw_hold = ending == 2;
	if (!ending)
		return 1;

	/* ---------------- the animation at the top of the big cube's jump ---------------- */
	{
		float top[3], cvw[3], c0[3], cube_top[3];
		float io[2] = {0.0f, 0.0f}, lo[2] = {0.0f, 0.0f};

		*st = apex;
		/* the cube's top: tile space (top) and view space (cube_top) */
		cvw[0] = af.cube[0][3]; cvw[1] = af.cube[1][3]; cvw[2] = af.cube[2][3];
		inv_point(af.tiles, cvw, c0);
		top[0] = c0[0]; top[1] = c0[1] + TILE * 0.8f; top[2] = c0[2];
		gm_multvec(af.tiles, top, cube_top);

		{
			/* ---- death / flagpole ----
			   death: the camera holds still while the game plays its death jump;
			   flagpole: he has slid down and turned round (the game's sequence), and
			   leaps off the pole with a backflip (the castle is not drawn).
			   Then, the same for both: as he is about to leave the screen (falling) the
			   camera follows him and brings him to 20 % from the top, over the cube,
			   turning him to face us; the level goes up and out, a whole black screen
			   goes by, the logo comes up from below with the cube at the top of its
			   jump; the camera stops with the logo in place, he lands on the cube and
			   the animation resumes.
			   Flagpole only: the game goes on unseen (castle, time count, 3 fireworks):
			   the score is shown under the logo as it counts, the fireworks go off around
			   the logo, and the logo is held before the fade-out until it is all over. */
			/* (the end of the game: as the flagpole, from where he stands by Peach; the
			   game goes on unseen meanwhile, its music to its end) */
			int flag = ending == 2 || ending == 3, victory_on = ending == 3;
			float p0[3], pm[3], lx, hold, dist, ly = play_oy, vel = 0.0f, mv = 0.0f;
			float y_start = 0.0f, gb, logo_shift;
			int nes_on = !flag, landed = 0, follow = -1, castle_on = ending == 2, quiet = 0;
			float land_io[2] = {0.0f, 0.0f};
			int death_nspr = ply_n_ref, death_music = !flag, nes_was_on = !flag;
			static uint8_t fw_img[SMB_GW * SMB_GH];
			int fw_slot_pos[6] = {-1, -1, -1, -1, -1, -1}, fw_next = 0;
			float fw_place[6][2];
			u32 fw_seed = ((u32)view.level_x * 2654435761u) ^ ((u32)nes.ram[0x09] << 16) ^ (u32)(nes.ram[0x7f9] * 10 + nes.ram[0x7fa]) ^ 0x9e3779b9u;   /* varies from run to run */
			u8 srgb[3];
			Mtx mp;

			/* played in its original colours: whatever the end (death, the 1-1
			   flagpole, the end of the game), the rest of the animation in Mario's
			   colours (red, gold buttons), from before anything of it shows */
			if (orig_on)
				scene_mario_colours();
			view.ply_on = last_ply.on;
			view.ply_x = last_ply.x;
			view.ply_y = last_ply.y;
			memcpy(view.ply, last_ply.px, sizeof(view.ply));
			smb_render_shifted(&cam, d.dx, d.dy, mp);
			gm_copy(mp, d.ply_mtx);
			d.ply_level_x = view.level_x;
			d.ply_off[0] = d.ply_off[1] = d.ply_off[2] = 0.0f;
			d.ply_rot = 0.0f;
			d.ply_scale = 1.0f;
			d.ply_face = 0.0f;
			d.ply_show = 1;
			if (flag) {
				d.ply_mirror = nes.ram[0x33] == 2;  /* PlayerFacingDir 2 = left: turn him round */
				vel = 9.0f;                         /* the leap off the pole */
				if (jump_ply.on) {
					/* in his jumping pose (as last seen in the level), facing right */
					memcpy(last_ply.px, jump_ply.px, sizeof(last_ply.px));
					memcpy(view.ply, jump_ply.px, sizeof(view.ply));
					d.ply_mirror = jump_ply.left;
				}
			}
			player_view_m(mp, d.ply_level_x, p0);
			lx = cube_top[0] - (p0[0] + play_ox);   /* brought right over the cube */
			/* his head 20 % from the top: feet one sprite height (32 NES px) lower */
			{
				float sh = smb_render_nes_view_y(&cam, d.dx, d.dy, view.level_x, 0.0f, 0.0f) -
				           smb_render_nes_view_y(&cam, d.dx, d.dy, view.level_x, 0.0f, 32.0f);
				hold = vid->ortho_t - 0.2f * (vid->ortho_t - vid->ortho_b) - sh;
			}
			/* the cube at the top of its jump is high on the screen: if he would not be
			   above it, the logo stops lower and glides up to its place once he is on it */
			/* he is held higher if the cube's top would come up past him: the logo then
			   comes to its place before he lands, and nothing moves under him once he is
			   on it (only if his head would leave the screen is the logo stopped lower,
			   gliding up with him after the landing) */
			{
				float sh = smb_render_nes_view_y(&cam, d.dx, d.dy, view.level_x, 0.0f, 0.0f) -
				           smb_render_nes_view_y(&cam, d.dx, d.dy, view.level_x, 0.0f, 32.0f);
				float top_max = vid->ortho_t - 0.03f * (vid->ortho_t - vid->ortho_b) - sh;
				if (!flag && hold < cube_top[1] + 40.0f)
					hold = cube_top[1] + 40.0f;
				if (hold > top_max)
					hold = top_max;
			}
			logo_shift = cube_top[1] + 40.0f - hold;
			if (logo_shift < 0.0f) logo_shift = 0.0f;
			/* the logo is placed so that: the level's lowest point leaves the top of the
			   screen, one screen height of black, then the cube's top enters at the bottom */
			gb = smb_render_nes_view_y(&cam, d.dx, d.dy, view.level_x, 0.0f, 240.0f);
			dist = (vid->ortho_t - gb) + (vid->ortho_t - vid->ortho_b) + (cube_top[1] - vid->ortho_b);
			if (dist < 0.0f) dist = 0.0f;
			scene_prepare(&af);
			scene_theme_rgb(srgb);
			for (k = 0; k < 3; k++)
				srgb[k] = (u8)((srgb[k] + 2 * 255) / 3);

			fall_scale = 1.0f;
			if (ending == 2 && snd_on && fan.done && fan.nframes > 0) {
				/* the level-clear fanfare, played so that it ends exactly when he lands:
				   the fall is worked out ahead (the same steps as below, with nothing
				   from the game in them for the flagpole) and the fanfare's speed set
				   from it */
				float base;
				int t2, want = (int)(fan.nframes / FANFARE_TEMPO + 0.5);
				player_view_m(mp, d.ply_level_x, pm);
				base = pm[1];
				/* the fall slowed (gravity and top speed scaled) until it lasts as long as
				   the fanfare at FANFARE_TEMPO; the tempo is then trimmed to the exact length */
				fall_scale = fall_fit(1, vel, FLAG_G, FLAG_TOP, base, ly, hold, dist, cube_top[1], logo_shift,
				                      vid->ortho_b, want, &t2);
				smb_seq_start(&fseq, &fan, 0.0);
				fseq.loop_from = SMB_SEQ_ONCE;
				smb_sound_use_seq(&snd, &fseq);
				snd.seq_force = 1;                 /* (the game's own fanfare is not heard) */
				snd.seq_rate = (double)fan.nframes / (double)(t2 + 1);
				snd.music_gain = 1.0f;
				fan_mode = 1;
			}
			for (t = 0; t < 1200 && !*reset_flag && !*power_flag && !landed; t++) {
				float sy, u;
				Mtx tr;
				if (victory_on)
					game_step();
				if (castle_on) {
					castle_on = castle_step(0, &quiet);
					if (hud.on)
						hud_build();   /* (the count, in the corner) */
					view.ply_on = 1;
					view.ply_x = last_ply.x;
					view.ply_y = last_ply.y;
					memcpy(view.ply, last_ply.px, sizeof(view.ply));
				}
				if (nes_on) {
					/* the game's own death animation, the camera still */
					float before[3];
					player_view_m(mp, d.ply_level_x, before);
					nes_set_pad(&nes, 0, 0);
					nes_run_frame(&nes);
					nes_ran = 1;
					if (nes.ram[0x7b1] == 0x01)
						death_elapsed++;
					smb_layout(&nes, &lcfg, &view);
					/* (all of him: as he goes out at the bottom, his sprites go one by
					   one and the picture would be cut) */
					if (view.ply_nspr > death_nspr)
						death_nspr = view.ply_nspr;
					if (view.ply_on && view.ply_nspr >= death_nspr && view.ply_y < 216 && nes.ram[RAM_YHIGH] < 2 &&
					    nes.ram[RAM_ENGINE] != ENG_LOSELIFE) {
						last_ply.x = view.ply_x;
						last_ply.y = view.ply_y;
						memcpy(last_ply.px, view.ply, sizeof(last_ply.px));
						player_view_m(mp, d.ply_level_x, pm);
						vel = pm[1] - before[1];
					} else {
						nes_on = 0;               /* off its picture: we carry on */
						if (vel > -2.0f) vel = -2.0f;
					}
					view.ply_on = 1;
					view.ply_x = last_ply.x;
					view.ply_y = last_ply.y;
					memcpy(view.ply, last_ply.px, sizeof(view.ply));
				}
				if (!nes_on && !nes_was_on && death_music)
					death_music = death_music_step();  /* (one game frame per picture) */
				if (!nes_on && nes_was_on && !flag) {
					/* he is off the game's picture: the rest of the fall is timed to end
					   with the death music, the end of the jingle then follows it */
					int t2, want = SMB_DEATH_MUSIC_FRAMES - death_elapsed;
					player_view_m(mp, d.ply_level_x, pm);
					fall_scale = fall_fit(0, vel, DEATH_G, DEATH_TOP, pm[1] + mv, ly, hold, dist, cube_top[1],
					                      logo_shift, vid->ortho_b, want, &t2);
				}
				nes_was_on = nes_on;
				if (!nes_on) {
					/* falling on: gravity, up to a top speed (view units per frame) */
					float g = flag ? FLAG_G : DEATH_G, top = flag ? FLAG_TOP : DEATH_TOP;
					vel -= g * fall_scale;
					if (vel < -top * fall_scale) vel = -top * fall_scale;
					mv += vel;
				}
				if (flag)                             /* the backflip, as out of the cube */
					d.ply_rot = 2.0f * (float)M_PI * smooth((float)t / 40.0f);
				player_view_m(mp, d.ply_level_x, pm);
				pm[1] += mv;                          /* where he is, camera aside */
				sy = pm[1] + ly;                      /* where he is on screen */
				if (follow < 0 && (sy < vid->ortho_b + 90.0f || (!nes_on && !flag) || (flag && vel < 0.0f))) {
					follow = t;                       /* about to leave the screen */
					y_start = sy;
					if (flag) {
						hold = flag_hold(hold, y_start);
						logo_shift = cube_top[1] - hold;
					}
				}
				if (follow >= 0) {
					/* keep him on screen, easing him up to his place over 0.75 s */
					float want;
					u = smooth((float)(t - follow) / 45.0f);
					want = y_start + (hold - y_start) * u;
					/* (from below, or from above: after the leap off the pole he can be
					   high up; the camera then comes up to him a little) */
					ly = want - pm[1];
					if (ly > dist) ly = dist;
					lo[0] = play_ox + lx * u;
					/* he turns to face us over the whole fall */
					d.ply_face = (float)(t - follow) / 75.0f;
					if (d.ply_face > 1.0f) d.ply_face = 1.0f;
				} else {
					u = 0.0f;
					lo[0] = play_ox;
				}
				lo[1] = ly;
				io[0] = lo[0] - play_ox - lx;
				io[1] = ly - dist - logo_shift;
				guMtxTrans(tr, 0.0f, mv, 0.0f);
				gm_concat(tr, mp, d.ply_mtx);
				/* on the cube as soon as its top (where he will stand on it) reaches his
				   feet, the logo in place or still rising: it then glides on up with him */
				if (follow >= 0 && pm[1] + ly <= cube_top[1] + io[1]) {
					/* (this frame's step went a little past it: right on it) */
					mv += cube_top[1] + io[1] - (pm[1] + ly);
					guMtxTrans(tr, 0.0f, mv, 0.0f);
					gm_concat(tr, mp, d.ply_mtx);
					landed = 1;
					land_io[0] = io[0];
					land_io[1] = io[1];
				}
				smb_render_prepare(&view);
				frame_passes_off(vid, &cam, &d, &af, io, lo);
			}

			/* on the cube: a raw cut to the jingle where the animation is (its end) */
			if (snd_on && jingle_start_frame >= 0 && jingle_volume > 0) {
				/* (death too: its music is over by then) */
				if (flag) {
					snd.music_gain = 0.0f;
					snd.wet = 0.0f;               /* (raw: no reverb tail either) */
				}
				fan_mode = 1;                     /* (no more of the intro's fades) */
				sound_play_from((double)(st->frame - jingle_start_frame) / aud_fps, jingle_volume);
			}
			/* the animation resumes: he rides the cube down into the logo and disappears */
			{
				int mirror = d.ply_mirror;
				float res_top = 0.0f, res_bottom = 0.0f, final_lb[4];
				int have_final_lb = 0, settled_at = -1, held = 0;
				int table_at = (ending == 2) ? -1 : 0;   /* when the result table is up */
				uint32_t live_score = 0xffffffffu;
				float hud_vy = 0.0f;
				/* (the table's height, known before it is: the size is set for it from
				   the start, so nothing changes size when it comes) */
				int place_h = (ending == 2)
				              ? (1 + (recs.r[SMB_REC_11].have_score || recs.r[SMB_REC_11].have_time)) * 12 - 4
				              : result.h;
				float lb_hist[LB_HIST], lb_low = 1e9f;
				float csx0 = st->cam_sx, csy0 = st->cam_sy;
				for (k = 0; k < LB_HIST; k++)
					lb_hist[k] = 1e9f;
				view.ply_on = last_ply.on;
				view.ply_x = last_ply.x;
				view.ply_y = last_ply.y;
				memcpy(view.ply, last_ply.px, sizeof(view.ply));
				smb_render_prepare(&view);
				memset(&d, 0, sizeof(d));
				d.ply_show = 1;
				d.ply_alpha = 255;
				d.ply_scale = 1.0f;
				d.ply_level_x = view.level_x;
				d.ply_face = 1.0f;
				d.ply_mirror = mirror;
				smb_render_player_point(&view, view.level_x, feet);
				for (t = 0; !*reset_flag && !*power_flag; t++) {
					static IntroState keep;
					float fwp[6][2];
					int fwi[6], nfw = 0, salpha = 0;
					if (victory_on) {
						/* (its fireworks' sound, as the game's: at the end of each) */
						for (k = 0; k < VFW_N; k++)
							if (t == VFW_T0 + k * VFW_EVERY + VFW_LEN)
								nes.ram[0xfe] |= 0x08;   /* Square2SoundQueue: Sfx_Blast */
						game_step();
					}
					if (castle_on) {
						castle_on = castle_step(1, &quiet);
						view.ply_on = last_ply.on;
						view.ply_x = last_ply.x;
						view.ply_y = last_ply.y;
						memcpy(view.ply, last_ply.px, sizeof(view.ply));
					}
					/* the 1-1 flagpole: the score, counting in the corner, goes up out of
					   the screen as he lands; it comes back from the bottom, centred, with
					   the lettering, still counting, up to its place under it; once the
					   count is over and it is in place, it slides to its column of the
					   result and the rest of the table comes in around it */
					if (hud.on) {
						hud_build();
						hud_vy += 0.3f;
						hud.y += hud_vy;
						if (hud.y - hud.h > vid->ortho_t)
							hud.on = 0;
					}
					if (ending == 2) {
						uint32_t sc = smb_score_of(nes.ram);
						if (fw_hold && settled_at >= 0 && t - settled_at >= RES_RISE)
							fw_hold = 0;              /* (the fireworks now) */
						if (table_at < 0 && sc != live_score) {
							char sl[16];
							const char *one[1] = {sl};
							snprintf(sl, sizeof(sl), "%06lu", (unsigned long)sc);
							smb_text_lines(&nes, one, 1, &result);
							live_score = sc;
						}
						if (table_at < 0 && (!castle_on || quiet >= 40) &&
						    settled_at >= 0 && t - settled_at >= RES_RISE) {
							run_ended(1, run_frames, run_ax, run_ay);
							table_at = t;
						}
					}
					if (death_music)
						death_music = death_music_step();
					/* held before its fade-out while the game counts and fires */
					keep = *st;
					intro_update(st);
					if (st->finished)
						break;
					if (flag && st->fade > 0 && castle_on && quiet < 40)
						*st = keep;
					if (victory_on && st->fade > 0 && t < VFW_T0 + (VFW_N - 1) * VFW_EVERY + VFW_LEN + 30)
						*st = keep;
					/* and until the result has been up long enough to be read */
					if (st->fade > 0 && t < 1500 &&
					    (settled_at < 0 || t - settled_at < RES_RISE + RES_HOLD ||
					     table_at < 0 || t - table_at < RES_HOLD)) {
						*st = keep;
						held = 1;             /* (the animation stands still from here) */
					}
					/* a C-stick view (kept from the animation): back to the IPL's own view
					   over the first second on the cube */
					{
						float u = smooth((float)t / 60.0f);
						st->cam_sx = csx0 * (1.0f - u);
						st->cam_sy = csy0 * (1.0f - u);
					}
					intro_frame(st, &f);
					if (t == 0) {
						/* where the lettering ends up: the animation run on to its end
						   (a copy, in the IPL's own view), measured on its model */
						static IntroState endst;
						static IntroFrame ef;
						int guard = 0;
						endst = *st;
						endst.cam_sx = endst.cam_sy = 0.0f;
						intro_frame(&endst, &ef);
						while (!endst.finished && guard++ < 3000) {
							intro_update(&endst);
							if (!endst.finished)
								intro_frame(&endst, &ef);
						}
						scene_prepare(&ef);
						have_final_lb = scene_letters_bounds(final_lb);
					}
					scene_prepare(&f);
					cvw[0] = f.cube[0][3]; cvw[1] = f.cube[1][3]; cvw[2] = f.cube[2][3];
					gm_copy((MtxP)f.tiles, d.ply_mtx);
					inv_point(f.tiles, cvw, c0);
					for (k = 0; k < 3; k++)
						d.ply_off[k] = c0[k] - feet[k];
					d.ply_off[1] += TILE * 0.8f * (1.0f - f.end_fall);
					d.ply_alpha = (int)(255.0f * (1.0f - f.end_fall));
					d.ply_scale = 1.0f - 0.5f * f.end_fall;
					if (f.end_fall >= 1.0f)
						d.ply_show = 0;
					/* the result (one line: score, time or world, NEW BEST): it rises from
					   below the screen's bottom edge as the lettering comes in and rebounds,
					   and stops under it where the animation leaves it (measured on its
					   model); it is never above the lettering's lowest point over the last
					   frames (its motion trails), so it never crosses it */
					{
						float lb[4], low = 1e9f, oy = land_io[1] * (1.0f - smooth((float)t / 40.0f));
						if (scene_letters_bounds(lb)) {
							if (settled_at < 0)
								settled_at = t;               /* (rises from the lettering's arrival) */
							lb_hist[t % LB_HIST] = lb[2] + oy;
						} else
							lb_hist[t % LB_HIST] = (settled_at < 0) ? 1e9f : vid->ortho_b - 100.0f;
						for (k = 0; k < LB_HIST; k++)
							if (lb_hist[k] < low)
								low = lb_hist[k];
						if (settled_at < 0 && (held || t >= 600))
							settled_at = t;           /* (no lettering measured: at the end) */
						lb_low = low;
					}
					{
						float want = vid->ortho_b + 0.08f * (vid->ortho_t - vid->ortho_b) + 6.0f;
						float u = (settled_at < 0) ? 0.0f : smooth((float)(t - settled_at) / (float)RES_RISE);
						res_px = 1.5f;
						if (have_final_lb && final_lb[2] - 6.0f < want)
							want = final_lb[2] - 6.0f;         /* (6 units under the lettering) */
						if (want - place_h * res_px < vid->ortho_b + 3.0f) {
							res_px = (want - vid->ortho_b - 3.0f) / (float)place_h;   /* (smaller to fit) */
							if (res_px < 0.75f) res_px = 0.75f;
						}
						/* (starts with its top at the bottom edge: wholly off screen) */
						res_top = vid->ortho_b + (want - vid->ortho_b) * u;
						if (res_top > lb_low - 6.0f)
							res_top = lb_low - 6.0f;           /* (kept under the lettering as it moves) */
						if (res_top < vid->ortho_b)
							res_top = vid->ortho_b;
						res_bottom = res_top - place_h * res_px;
					}
					salpha = (settled_at < 0) ? 0 : (t - settled_at) * 255 / 30;
					if (salpha > 255) salpha = 255;
					salpha = salpha * (255 - f.fade_alpha) / 255;
					if (flag) {
						/* the game's fireworks, face on, each at a random place on the
						   screen away from the logo, the lettering and the score (above
						   the logo's bottom, outside it, apart from the others) */
						/* (the end of the game has none: its own, one after another) */
						for (k = 0; k < 6; k++) {
							int vt = t - (VFW_T0 + k * VFW_EVERY);
							int alive = victory_on ? (k < VFW_N && vt >= 0 && vt < VFW_LEN)
							                       : (nes.ram[0x0f + k] && nes.ram[0x16 + k] == 0x16);
							if (!alive) { fw_slot_pos[k] = -1; continue; }
							if (fw_slot_pos[k] < 0) {
								int tries;
								float x = 0.0f, y = 0.0f;
								for (tries = 0; tries < 200; tries++) {
									int j, ok = 1;
									fw_seed = fw_seed * 1103515245u + 12345u;
									x = vid->ortho_l + 50.0f + (float)((fw_seed >> 8) % 10000) / 10000.0f *
									    (vid->ortho_r - vid->ortho_l - 100.0f);
									fw_seed = fw_seed * 1103515245u + 12345u;
									y = logo_b[2] + (float)((fw_seed >> 8) % 10000) / 10000.0f *
									    (vid->ortho_t - 50.0f - logo_b[2]);
									if (x > logo_b[0] - 45.0f && x < logo_b[1] + 45.0f &&
									    y > logo_b[2] - 45.0f && y < logo_b[3] + 45.0f)
										ok = 0;           /* on the logo */
									if (x > -result.w * res_px * 0.5f - 45.0f && x < result.w * res_px * 0.5f + 45.0f &&
									    y > res_bottom - 45.0f && y < res_top + 45.0f)
										ok = 0;           /* on the result */
									for (j = 0; j < 6 && ok; j++)
										if (fw_slot_pos[j] >= 0 && j != k) {
											float ddx = x - fw_place[j][0], ddy = y - fw_place[j][1];
											if (ddx * ddx + ddy * ddy < 90.0f * 90.0f)
												ok = 0;   /* on another one */
										}
									if (ok)
										break;
								}
								fw_place[k][0] = x;
								fw_place[k][1] = y;
								fw_slot_pos[k] = fw_next++;
							}
							if (victory_on ? !smb_firework_image(&nes, vt / 8, fw_img)
							               : !smb_slot_image(&nes, k, fw_img))
								continue;
							smb_render_sprite_upload(nfw, fw_img);
							fwp[nfw][0] = fw_place[k][0];
							fwp[nfw][1] = fw_place[k][1];
							fwi[nfw] = nfw;
							nfw++;
						}
						smb_render_prepare(&view);    /* palettes for the fireworks */
					}
					/* (the 1-1 flagpole's result coming in: the score to its column, the
					   rest around it) */
					fin_score_x = 0.0f;
					fin_ext = NULL;
					fin_digits_on = 0;
					if (ending == 2) {
						digits_of(table_at >= 0 ? &result_score : &result, fin_digits);
						fin_digits_on = 1;
					}
					if (ending == 2 && table_at >= 0) {
						float u = smooth((float)(t - table_at) / 30.0f);
						float col = (-0.5f * (float)result_rest.w + 24.0f) * res_px;   /* (6 digits: 48 px) */
						fin_score_x = col * u;
						fin_ext = &result_rest;
						fin_ext_alpha = (int)(salpha * u);
					}
					frame_passes_final(vid, &f, &d, land_io[0] * (1.0f - smooth((float)t / 40.0f)),
					                   land_io[1] * (1.0f - smooth((float)t / 40.0f)),
					                   (ending == 2 && table_at >= 0) ? &result_score : &result,
					                   res_top, salpha, srgb,
					                   nfw, (const float (*)[2])fwp, fwi, 3.0f, 255 - f.fade_alpha);
				}
			}
		}
	}
	return 1;
}
