/*
 * config.h - user settings (optional gc-anim.ini on SD/USB)
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <gccore.h>

enum { INTRO_IPL = 0, INTRO_PROMO = 1 };
enum { DROP_FALL = 0, DROP_JUMP = 1, DROP_OFF = 2 };

#define CFG_MAX_TILE_COLORS 16

typedef struct {
	int intro;             /* INTRO_IPL (the IPL's boot animation) or INTRO_PROMO (promo videos) */
	int allow_skip;        /* A / Start skips the animation                                    */
	int camera;            /* C-stick moves the camera                                         */
	int force_43;          /* ignore the console's 16:9 setting                                */
	int volume;            /* jingle volume 0..255 (IPL level: 199)                            */
	int start_delay;       /* ms of black screen before the animation (slow HDMI screens/mods) */
	/* colours (all optional; see README) */
	int color_random;      /* color = random: a new colour scheme at each boot (config_randomize) */
	int have_color;
	u8  color[3];          /* theme colour: default for everything below                       */
	int hue;               /* degrees added to every recoloured colour                         */
	int saturation;        /* percent                                                          */
	int brightness;        /* percent                                                          */
	int have_cube, cube_follow;
	u8  cube[3];           /* small cube (cube = tiles: follows the next tile's colour)         */
	int ntiles;            /* 0 = tiles follow "color"                                          */
	int tiles_rainbow;
	int tiles_spread;      /* 0 = cycle the list, 1 = spread it in bands along the path         */
	u8  tiles[CFG_MAX_TILE_COLORS][3];
	int trail;             /* new tiles start as the cube colour, reach theirs after N tiles    */
	int have_logo;
	u8  logo[3];
	int have_drop;
	u8  drop[3];           /* colour the cube turns into on its final drop                      */
	int drop_blend;        /* DROP_FALL, DROP_JUMP or DROP_OFF                                  */
	int have_glass;
	u8  glass[3];
	int glass_opacity;     /* percent                                                          */
	int have_background;
	u8  background[3];     /* also the colour of the final fade                                 */
	int have_promo_background;
	u8  promo_background[3]; /* the promo intro's background, if not background's              */
	int have_text;
	u8  text[3];           /* NINTENDO / GAMECUBE colour                                        */
	char letters[17];      /* custom 3D text replacing NINTENDO (A-Z and spaces, max 16)       */
	int letters_fill;      /* 0 = NINTENDO's letter pitch (squeezed beyond 8), 1 = NINTENDO's width */
	char text_image[256];  /* image replacing the "GAMECUBE" logotype (PNG/BMP/JPG/TGA)         */
	/* SMB mode (smb.nes) */
	int smb_full;          /* 0: World 1-1 only; 1: the whole game                              */
} Config;

void config_defaults(Config *c);
/* parse an ini file; returns 0 if the file was read */
int  config_load(Config *c, const char *path);
/* color = random: draw a colour scheme (color, cube, tiles, tiles_mode, trail) from seed */
void config_randomize(Config *c, u32 seed);

#endif
