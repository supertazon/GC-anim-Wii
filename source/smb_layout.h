/*
 * smb_layout.h - turns the NES state of the SMB mode into what the 3D scene draws
 *
 * Pure C (no libogc), so it is tested on PC against the NES core.
 *
 * The NES picture is split in three:
 *  - "ground" metatiles      -> IPL tiles (flat, like the cube's trail), laid by two
 *                               rolling cubes at the right edge
 *  - everything else         -> one flat layer (palette slots, sky transparent),
 *                               sprites included with their NES priorities
 * Ground metatiles are recognised by the nametable tile number of their top-left
 * 8x8 tile (SMB: $B4).
 */
#ifndef SMB_LAYOUT_H
#define SMB_LAYOUT_H

#include <stdint.h>
#include "nes/nes.h"

#define SMB_VIEW_W    256
#define SMB_VIEW_Y0   32          /* the status bar (lines 0-31) is not shown */
#define SMB_VIEW_H    208         /* lines 32..239 */
#define SMB_LAYER_TRANSPARENT 0xff
#define SMB_MAX_META  512
#define SMB_PLY       32          /* player overlay texture size */
#define SMB_AHEAD     8           /* ground columns looked at past the right edge */
#define SMB_LEFT_W    128         /* background drawn left of the picture (16:9 framing) */

enum { SMB_MT_GROUND = 1 };

typedef struct {
	int16_t px;        /* screen x of the metatile's left edge (may be < 0) */
	uint8_t row;       /* metatile row 0..14 (row 2 = first one under the status bar) */
	uint8_t kind;      /* SMB_MT_GROUND */
	int32_t col;       /* absolute level column (16 px), for colour patterns */
	uint8_t ncol;      /* its main NES colour (0..63; 0xff: none): original colours mode */
} SmbMeta;

typedef struct {
	uint8_t ground_tile;       /* nametable tile of a ground metatile's top-left 8x8 */
	int ending;                /* after the flagpole: no background, sprites from the RAM
	                              buffer, without the flags (fireworks and points only) */
	int text_out;              /* text lines (world screen, messages) are taken out of the
	                              picture: smb_text gives them, to be shown face on */
	int fw_out;                /* fireworks are taken out of the picture (shown face on) */
	int no_sprites;            /* the world screen: nothing but its text (shown face on by
	                              smb_text): no sprites, background, floors or left strip */
	int hide_castle;           /* the castle after the pole is not drawn (1-1 mode: the
	                              GameCube logo stands in its place) */
} SmbLayoutCfg;

/* a cube laying ground: resting on column col (absolute), rolling towards col+1
 * (roll 0..1) or jumping over a gap (hop = columns travelled, lift = height in tiles) */
typedef struct {
	int visible;
	int row;
	int32_t col;
	float roll, hop, lift;
} SmbCube;

#define SMB_GW 16                 /* enemy image: 2 x 3 sprites */
#define SMB_GH 24
#define SMB_ENEMIES 6

/* an enemy the game no longer draws at the left edge (it hides them as soon as they
 * start crossing it, but keeps them alive until 72 px past it): drawn from its live
 * RAM position with the image it last had in full view */
typedef struct {
	int on;
	int16_t x, y;              /* NES screen position of img[0] (x may be < 0) */
	uint8_t img[SMB_GW * SMB_GH];   /* palette slot, or transparent */
} SmbGhost;

typedef struct {
	int scroll;                /* horizontal scroll of the playfield, 0..511 */
	int32_t level_x;           /* absolute level x of the screen's left edge (pixels) */
	int32_t next_col;          /* next column the game will draw: ground is known below it */
	int nmeta;
	SmbMeta meta[SMB_MAX_META];
	SmbCube cube[2];           /* top and bottom ground rows */
	int32_t ground_limit[15];  /* per metatile row: ground drawn up to this column (incl.) */
	uint8_t pal[32];           /* NES colour (0..63) of each palette slot, this frame */
	uint8_t text_row[30];      /* screen tile rows holding text (with text_out) */
	uint8_t layer[SMB_VIEW_H * SMB_VIEW_W];   /* palette slot 0..31 (16+ = sprites), or transparent */
	/* the background just left of the picture, from what is still in the nametables
	   (no sprites): NES x -SMB_LEFT_W..-1 */
	uint8_t left[SMB_VIEW_H * SMB_LEFT_W];
	SmbGhost ghost[SMB_ENEMIES];
	int32_t pole_col;          /* flagpole column (absolute), -1 if not in the nametables;
	                              the castle (pole_col + 4 .. + 8) is not drawn: the
	                              GameCube logo stands in its place */
	/* the player's sprites, kept out of the layer so they can be moved on their own
	   (SMB RAM $06E4: OAM offset of the player's 8 sprites) */
	int ply_on;                                /* any player sprite visible */
	int ply_nspr;                              /* player sprites visible (of 8) */
	int16_t ply_x, ply_y;                      /* NES screen position of ply[0] */
	uint8_t ply[SMB_PLY * SMB_PLY];            /* palette slot, or transparent */
} SmbView;

void smb_layout_defaults(SmbLayoutCfg *c);
/* forget the enemy images (call when a level starts) */
void smb_layout_reset(void);
void smb_layout(const nes_t *n, const SmbLayoutCfg *c, SmbView *v);
/* the cubes' own progress, kept between frames */
typedef struct {
	int started;
	float p;                   /* top cube position, in level columns */
	float pb;                  /* bottom cube position */
	int t;                     /* frames since they appeared */
	int32_t last_x;
	int start_col;             /* top cube's first column (absolute) */
	int roll_frames;           /* frames per tile */
	int drop_frames;           /* the bottom cube drops in over this many frames */
	int push;                  /* Mario is playing: push them along if he catches up */
} SmbCubeState;

void smb_cubes_reset(SmbCubeState *st);
/* move the two ground-laying cubes and set ground_limit (call after smb_layout) */
void smb_cubes(const nes_t *n, const SmbLayoutCfg *c, SmbView *v, SmbCubeState *st);
/* is there a ground metatile at absolute level column col, metatile row row? */
int32_t smb_next_col(const nes_t *n);
/* the sprites of enemy slot k (RAM sprite buffer), as an image; 0 if none visible */
int smb_slot_image(const nes_t *n, int k, uint8_t img[SMB_GW * SMB_GH]);
/* the same, with the image's top-left corner on the screen (NES pixels) */
int smb_slot_image_at(const nes_t *n, int k, uint8_t img[SMB_GW * SMB_GH], int *x, int *y);
/* Mario standing, big (big, fire) or small, sprite palette 0 (32 x 32, feet at the
   bottom): the world screen shows him as he is */
int smb_mario_icon(const nes_t *n, int big, uint8_t img[SMB_PLY * SMB_PLY]);
/* a firework as the game draws one, explosion frame 0..2 (16 x 16, top of img) */
int smb_firework_image(const nes_t *n, int frame, uint8_t img[SMB_GW * SMB_GH]);
/* the score as the status bar shows it (6 digits): 1 bit per pixel, 48 x 8 */
void smb_score_bitmap(const nes_t *n, uint8_t out[48 * 8]);

/* the text lines on screen (rows found by smb_layout with text_out: the world screen's
 * "WORLD 1-2", Toad's and Peach's messages, the warp zone's welcome), packed into one
 * bitmap: blank rows between lines kept to one, trimmed left and right. with_score:
 * the score (status bar) under them, on a line of its own. Returns 0 if none. */
#define SMB_TEXT_W 256
#define SMB_TEXT_H 128
typedef struct {
	int w, h;                                  /* pixels used */
	uint8_t bits[SMB_TEXT_W * SMB_TEXT_H];     /* 1: ink */
} SmbText;
int smb_text(const nes_t *n, const SmbView *v, int with_score, SmbText *t);
/* lines of our own (digits, A-Z, space, - . ! : x) in the game's font, each centred */
void smb_text_lines(const nes_t *n, const char *const *lines, int nlines, SmbText *t);

/* the floors (drawn as IPL tiles): the game's own terrain, followed as it builds each
 * column of the level (a hook on its area renderer): the 2 bottom rows (screen rows 13
 * and 14) where the column's terrain pattern reaches the bottom (not a ceiling or a
 * band in the middle; raised floors keep their own tiles above), and where the terrain
 * is still there once the level's objects are placed (holes, pipes, lava, ...).
 * Cloud areas: none (clouds stay).
 * Install once on the game's core (also when it is reset). */
void smb_floor_install(nes_t *n);
/* is (level column, metatile row 2..14) a floor tile in the area on screen? */
int  smb_floor_at(int32_t col, int row);

/* the top of the floor under the left part of the screen (screen tile row), from the
 * picture: the lowest run of non-blank tiles; -1 if there is none */
int smb_floor_row(const nes_t *n, const SmbView *v);
#define SMB_CASTLE_X0 4            /* castle columns, from the pole */
#define SMB_CASTLE_X1 8
int  smb_ground_at(const nes_t *n, const SmbLayoutCfg *c, const SmbView *v, int32_t col, int row);

#endif
