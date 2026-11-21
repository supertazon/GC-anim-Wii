/*===========================================================================
	GC-anim-Wii - the GameCube boot animation as a native Wii program

	Original project by supertazon; rewritten without GRRLIB on top of a
	reverse-engineered model of the IPL's own animation code.  All assets
	(models, textures, animations, sound) are read at run time from the
	user's own GameCube IPL dump; nothing from Nintendo is distributed.

	Two intros, picked by gc-anim.ini [general] intro:
	  ipl    the IPL's own boot animation (with the Super Mario Bros. easter egg)
	  promo  the alternative intro of Nintendo's promotional videos
	Both always end in the System Menu (made to be installed in Priiloader).
===========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <malloc.h>
#include <gccore.h>
#include <fat.h>
#include <asndlib.h>
#include <sdcard/wiisd_io.h>
#include <ogc/usbstorage.h>
#include <ogc/lwp_watchdog.h>
#include <wiiuse/wpad.h>

#include "gcmath.h"
#include "ipl.h"
#include "intro.h"
#include "scene.h"
#include "video.h"
#include "sound.h"
#include "config.h"
#include "ipl_audio.h"
#include "smb.h"
#include "pad_raw.h"
#include "promo.h"
#include "anim_data.h"

#define PRIILOADER_TOMENU    0x50756E65
#define PRIILOADER_MAGICADDR ((vu32 *)0x8132FFFB)
#define PROMO_FPS 29.97003f        /* the promo clip's frame rate */

static volatile int reset_pressed, power_pressed;
static int have_sd, have_usb;

static void reset_cb(u32 irq, void *ctx) { (void)irq; (void)ctx; reset_pressed = 1; }
static void power_cb(void) { power_pressed = 1; }
static void wpad_power_cb(s32 chan) { (void)chan; power_pressed = 1; }
static int have_wpad;

static Config cfg;
static Video vid;

/* the only way out: power off, or the System Menu (never the loader) */
static void leave(void)
{
	if (power_pressed)
		SYS_ResetSystem(SYS_POWEROFF, 0, 0);
	*PRIILOADER_MAGICADDR = PRIILOADER_TOMENU;   /* tell Priiloader not to autoboot us again */
	DCFlushRange((void *)PRIILOADER_MAGICADDR, 4);
	SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
}

static void mount_devices(void)
{
	have_sd = fatMountSimple("sd", &__io_wiisd);
	have_usb = fatMountSimple("usb", &__io_usbstorage);
}

/* a USB drive can still be starting up when the Wii boots (hard drives spinning up): when
   the IPL dump wasn't found and the drive didn't mount, try again for up to 3 s */
static int retry_usb(void)
{
	int i;
	for (i = 0; i < 12 && !have_usb && !reset_pressed && !power_pressed; i++) {
		usleep(250 * 1000);
		have_usb = fatMountSimple("usb", &__io_usbstorage);
	}
	return have_usb;
}

static void unmount_devices(void)
{
	if (have_sd)
		fatUnmount("sd");
	if (have_usb)
		fatUnmount("usb");
	have_sd = have_usb = 0;
}

/* the VI's real refresh rate: 50 Hz, or 60/1.001 Hz (NTSC, PAL60, 480p) */
static float vi_hz(void) { return vid.hz50 ? 50.0f : 60000.0f / 1001.0f; }

/*
 * Every file lives in apps/gc-anim-wii/ (SD first, then USB): the IPL dump, gc-anim.ini,
 * the logotype image, smb.nes and promo.wav.  The IPL dump alone may also be at the root
 * of the SD card or USB drive (load_ipl).  SD and USB work the same; when both hold a
 * file, the SD card's copy is used.  Launched from the Homebrew Channel, the
 * .dol's own folder (argv[0]) is searched first.
 */
#define MAXDIRS 4
static char dirs[MAXDIRS][128];
static int ndirs;

static void add_dir(const char *d)
{
	int i;
	if (ndirs >= MAXDIRS || strlen(d) >= sizeof(dirs[0]))
		return;
	for (i = 0; i < ndirs; i++)
		if (!strcasecmp(dirs[i], d))
			return;
	strcpy(dirs[ndirs++], d);
}

static void init_dirs(const char *argv0)
{
	if (argv0 && argv0[0]) {
		char p[128];
		char *slash;
		if (argv0[0] == '/')
			snprintf(p, sizeof(p), "sd:%s", argv0);
		else
			snprintf(p, sizeof(p), "%s", argv0);
		slash = strrchr(p, '/');
		if (slash && strchr(p, ':')) {
			slash[1] = 0;
			add_dir(p);
		}
	}
	add_dir("sd:/apps/gc-anim-wii/");
	add_dir("usb:/apps/gc-anim-wii/");
}

static int file_exists(const char *p)
{
	FILE *f = fopen(p, "rb");
	if (!f)
		return 0;
	fclose(f);
	return 1;
}

/* "name.png" -> first existing dirs[i]/name.png; a path with a device ("sd:/...") is kept */
static int resolve(const char *name, char *out, size_t outsz)
{
	int i;
	if (strchr(name, ':')) {
		snprintf(out, outsz, "%s", name);
		return file_exists(out);
	}
	while (*name == '/')
		name++;
	for (i = 0; i < ndirs; i++) {
		snprintf(out, outsz, "%.127s%.100s", dirs[i], name);
		if (file_exists(out))
			return 1;
	}
	return 0;
}

static int load_ipl(u8 **ipl, u32 *size)
{
	static const char *const names[] = {"ipl.bin", "iplusa.bin", "iplpal.bin", "ipljap.bin"};
	char path[192];
	int d;
	unsigned n;

	/* the app folders, then (for the IPL dump only) the root of the SD card or USB drive */
	static const char *const roots[] = {"sd:/", "usb:/"};
	for (d = 0; d < ndirs + 2; d++)
		for (n = 0; n < sizeof(names) / sizeof(names[0]); n++) {
			snprintf(path, sizeof(path), "%.127s%s", d < ndirs ? dirs[d] : roots[d - ndirs], names[n]);
			if (ipl_load_file(path, ipl, size) == 0) {
				if (ipl_prepare(*ipl, *size))
					return 0;
				free(*ipl);
				*ipl = NULL;
			}
		}
	return -1;
}

static void load_config(void)
{
	char p[192];
	int i;
	config_defaults(&cfg);
	for (i = 0; i < ndirs; i++) {
		snprintf(p, sizeof(p), "%.127sgc-anim.ini", dirs[i]);
		if (config_load(&cfg, p) == 0)
			break;
	}
	if (cfg.color_random) {
		/* a seed that changes at each boot: the RTC and the time base since power-on */
		u64 tb = gettime();
		config_randomize(&cfg, (u32)time(NULL) ^ (u32)tb ^ (u32)(tb >> 32) * 0x9e3779b9u);
	}
}

/* [general] delay: a black screen first, for screens / HDMI mods slow to lock onto the
   signal; whole VI frames at the VI's real rate, rounded up */
static void start_delay(int smb)
{
	int n;
	if (cfg.start_delay <= 0)
		return;
	n = (int)((float)cfg.start_delay * vi_hz() / 1000.0f + 0.999f);
	video_black(&vid);
	while (n-- > 0 && !reset_pressed && !power_pressed) {
		if (smb)
			smb_boot_step(2);              /* (SMB boots meanwhile) */
		VIDEO_WaitVSync();
	}
}

/* ================================================================ IPL intro */

/* the IPL's alternate jingles: Z held at power-on on controller 1, or on all four */
static int jingle_variant(int *delay_50, int *delay_60)
{
	u32 held[4];
	int i, all = 1;
	PAD_ScanPads();
	for (i = 0; i < 4; i++) {
		held[i] = PAD_ButtonsHeld(i);
		if (!(held[i] & PAD_TRIGGER_Z))
			all = 0;
	}
	if (all) {
		*delay_60 = 7; *delay_50 = 6;
		return IPL_JINGLE_Z_ALL4;
	}
	if (held[0] & PAD_TRIGGER_Z) {
		*delay_60 = 6; *delay_50 = 5;
		return IPL_JINGLE_Z_PAD1;
	}
	*delay_60 = 0; *delay_50 = 0;
	return IPL_JINGLE_NORMAL;
}

static void run_ipl(IplAssets *assets, u8 *ipl, u32 ipl_size, int variant, int d50, int d60)
{
	static IntroState st;
	static IntroFrame fr;
	IntroParams params;
	int have_sound = 0;
	u8 *smb_rom = NULL;
	u32 smb_size = 0;
	int smb_armed = 0, jingle_frames = 0;
	u32 pads = 0;

	gm_init();
	if (scene_init(assets, &cfg) != 0) {
		free(ipl);
		unmount_devices();
		return;
	}
	video_set_background(&vid, scene_background());
	have_sound = sound_init(ipl, ipl_size, variant) == 0;
	{
		/* SMB mode: the user's smb.nes, if present (and only the right ROM: smb_prepare
		   checks its CRC-32), is booted during the animation; its music uses the IPL's
		   instruments (decoded before the IPL is freed) */
		char p[192];
		if (resolve("smb.nes", p, sizeof(p))) {
			char *slash = strrchr(p, '/');
			smb_rom = smb_load_rom(p, &smb_size);
			if (slash) {
				slash[1] = '\0';
				smb_set_save_dir(p);       /* (records and run log next to smb.nes) */
			}
		}
		if (smb_rom) {
			/* (connecting takes a moment: started now, it is there for the animation) */
			WPAD_Init();
			WPAD_SetPowerButtonCallback(wpad_power_cb);
			have_wpad = 1;
		}
		if (smb_rom && have_sound)
			smb_sound_setup(ipl, ipl_size, vid.hz50);
	}
	free(ipl);
	unmount_devices();

	params = vid.hz50 ? intro_params_50hz : intro_params_60hz;
	params.snd_delay = vid.hz50 ? d50 : d60;
	intro_init(&st, &params);

	if (smb_rom)
		smb_set_options(&cfg);
	if (smb_rom && smb_prepare(smb_rom, smb_size) != 0) {
		/* not Super Mario Bros. (wrong CRC-32): no SMB mode, B does nothing */
		free(smb_rom);
		smb_rom = NULL;
	}

	start_delay(smb_rom != NULL);

	while (!reset_pressed && !power_pressed) {
		int pass;

		pads = PAD_ScanPads();
		if (cfg.allow_skip && (PAD_ButtonsDown(0) & (PAD_BUTTON_A | PAD_BUTTON_START)))
			break;
		if (cfg.camera) {
			/* C-stick: turns the view like v1 did, springs back when released */
			/* (measured from the stick's real centre: one held while the console
			   started would otherwise keep the view turned) */
			s8 sx, sy, cx, cy;
			pad_raw_sticks(0, pads, &sx, &sy, &cx, &cy);
			st.cam_sx = cx;
			st.cam_sy = cy;
		}

		if (smb_rom) {
			/* SMB mode: boot in the background; B pressed before the branch point arms it
			   (Y: in the game's original colours, once the whole game has been finished) */
			int orig_ok = smb_original_unlocked();
			smb_boot_step(2);
			if (!smb_armed && (PAD_ButtonsHeld(0) & PAD_BUTTON_B))
				smb_armed = 1;
			if (!smb_armed && orig_ok && (PAD_ButtonsHeld(0) & PAD_BUTTON_Y))
				smb_armed = 2;
			if (have_wpad) {
				u32 w;
				WPAD_ScanPads();
				w = WPAD_ButtonsHeld(0);
				if (!smb_armed && (w & WPAD_BUTTON_1))
					smb_armed = 1;            /* (1 on the Wii Remote, as B on the pad) */
				if (!smb_armed && orig_ok && (w & WPAD_BUTTON_2))
					smb_armed = 2;            /* (2, as Y) */
			}
		}

		intro_update(&st);
		if (st.sound_start && have_sound) {
			sound_start(cfg.volume);
			jingle_frames = 1;
		} else if (jingle_frames)
			jingle_frames++;
		if (smb_rom && have_sound)
			smb_intro_audio(jingle_frames, smb_armed, cfg.volume);
		if (smb_armed && smb_branch_point(&st)) {
			smb_set_original_colours(smb_armed == 2);
			smb_armed = 0;
			{
				int done = smb_play(&vid, &cfg, &st, have_sound, &reset_pressed, &power_pressed);
				smb_sound_end();
				if (have_sound && !done)
					sound_set_volume(cfg.volume);     /* not playable: the jingle as it was */
				if (done)
					break;
			}
		}
		if (st.finished)
			break;

		intro_frame(&st, &fr);
		scene_prepare(&fr);
		for (pass = 0; pass < 2; pass++) {
			video_begin_pass(&vid, pass);
			scene_draw(&fr);
			video_end_pass(&vid, pass);
		}
		video_present(&vid);
	}

	video_black(&vid);
	if (smb_rom)
		smb_boot_wait();
	free(smb_rom);
	if (have_sound)
		sound_free();
	scene_free();
	if (have_wpad)
		WPAD_Shutdown();
}

/* ============================================================== promo intro */

/* promo.wav: PCM 16-bit, mono or stereo -> big-endian stereo for ASND */
static s16 *pcm;
static u32 pcm_bytes, pcm_rate;

static u32 le32(const u8 *p) { return p[0] | p[1] << 8 | p[2] << 16 | (u32)p[3] << 24; }
static u16 le16(const u8 *p) { return p[0] | p[1] << 8; }

static int load_wav(void)
{
	char path[192];
	FILE *f;
	long len;
	u8 *b;
	u32 p = 12, ch = 0, bits = 0, i, n;

	if (!resolve("promo.wav", path, sizeof(path)) || !(f = fopen(path, "rb")))
		return -1;
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (len < 44 || len > 16 * 1024 * 1024 || !(b = malloc(len))) {
		fclose(f);
		return -1;
	}
	if (fread(b, 1, len, f) != (size_t)len || memcmp(b, "RIFF", 4) || memcmp(b + 8, "WAVE", 4)) {
		fclose(f);
		free(b);
		return -1;
	}
	fclose(f);
	while (p + 8 <= (u32)len) {
		u32 id = p, sz = le32(b + p + 4);
		p += 8;
		if (!memcmp(b + id, "fmt ", 4) && sz >= 16) {
			ch = le16(b + p + 2);
			pcm_rate = le32(b + p + 4);
			bits = le16(b + p + 14);
		} else if (!memcmp(b + id, "data", 4) && ch && bits == 16 && (ch == 1 || ch == 2)) {
			if (sz > (u32)len - p) sz = (u32)len - p;
			n = sz / (2 * ch);
			pcm_bytes = (n * 4 + 31) & ~31;
			pcm = memalign(32, pcm_bytes);
			if (!pcm) break;
			memset(pcm, 0, pcm_bytes);
			for (i = 0; i < n; i++) {
				s16 l = (s16)le16(b + p + i * 2 * ch), r = ch == 2 ? (s16)le16(b + p + i * 4 + 2) : l;
				pcm[2 * i] = l;
				pcm[2 * i + 1] = r;
			}
			DCFlushRange(pcm, pcm_bytes);
			free(b);
			return 0;
		}
		p += sz + (sz & 1);
	}
	free(b);
	return -1;
}

static void run_promo(IplAssets *assets, u8 *ipl)
{
	u32 frame = 0;
	s32 voice = -1;
	float hz = vi_hz();
	int ok;

	free(ipl);
	load_wav();
	video_set_background(&vid, (GXColor){0, 0, 0, 0xFF});
	ok = promo_init(assets, &vid, &cfg) == 0;       /* (reads the custom logotype image) */
	unmount_devices();
	if (!ok) {
		free(pcm);
		return;
	}

	if (pcm) {
		ASND_Init();
		ASND_Pause(0);
		voice = ASND_GetFirstUnusedVoice();
		if (voice < 0) voice = 0;
	}
	start_delay(0);
	if (pcm) {
		/* [general] volume: 199 = the promo's full level */
		int vol = cfg.volume * 255 / 199;
		if (vol > 255) vol = 255;
		ASND_SetVoice(voice, VOICE_STEREO_16BIT, pcm_rate, 0, pcm, pcm_bytes, vol, vol, NULL);
	}

	while (!reset_pressed && !power_pressed) {
		float t = frame * PROMO_FPS / hz;
		PAD_ScanPads();
		if (cfg.allow_skip && (PAD_ButtonsDown(0) & (PAD_BUTTON_A | PAD_BUTTON_START)))
			break;
		if (t > A_END_FRAME && t / PROMO_FPS >= A_AUDIO_SECONDS)
			break;
		promo_draw(t < A_END_FRAME ? t : A_END_FRAME);
		video_present(&vid);
		frame++;
	}
	video_black(&vid);
	if (pcm) {
		ASND_StopVoice(voice);
		ASND_End();
		free(pcm);
	}
	promo_free();
}

/* ===================================================================== main */

int main(int argc, char **argv)
{
	IplAssets assets;
	u8 *ipl = NULL;
	u32 ipl_size = 0;
	int variant = IPL_JINGLE_NORMAL, d50 = 0, d60 = 0, ok;

	SYS_SetResetCallback(reset_cb);
	SYS_SetPowerCallback(power_cb);
	PAD_Init();
	mount_devices();
	init_dirs(argc > 0 && argv ? argv[0] : NULL);
	load_config();
	variant = jingle_variant(&d50, &d60);      /* (Z at power-on; used by the IPL intro only) */

	ok = load_ipl(&ipl, &ipl_size) == 0;
	if (!ok && !have_usb && retry_usb()) {
		load_config();                     /* (gc-anim.ini may be on the drive too) */
		ok = load_ipl(&ipl, &ipl_size) == 0;
	}
	ok = ok && ipl_extract_assets(ipl, ipl_size, &assets) == 0;
	if (ok) {
		/* logotype image: a file name (looked up in the folders above) or a full path;
		   logo.png in those folders is used when none is set */
		char p[256];
		if (resolve(cfg.text_image[0] ? cfg.text_image : "logo.png", p, sizeof(p)))
			snprintf(cfg.text_image, sizeof(cfg.text_image), "%s", p);
		else
			cfg.text_image[0] = 0;
	}

	video_init(&vid, cfg.force_43);
	if (!ok) {
		/* no IPL dump: nothing to show, go straight on */
		free(ipl);
		unmount_devices();
		leave();
		return 0;
	}

	if (cfg.intro == INTRO_PROMO)
		run_promo(&assets, ipl);
	else
		run_ipl(&assets, ipl, ipl_size, variant, d50, d60);
	ipl_free_assets(&assets);

	video_black(&vid);
	leave();
	return 0;
}
