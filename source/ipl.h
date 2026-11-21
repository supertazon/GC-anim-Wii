/*
 * ipl.h - access to the user's GameCube IPL dump
 *
 * Nothing from Nintendo is shipped with this program: every model, texture,
 * animation and sound is read at run time from the IPL dump on SD/USB.
 * The resources are located through the IPL's own resource directory and
 * identified by content (names / magics), so all known IPL versions
 * (NTSC 1.0/1.1/1.2, PAL 1.0/1.1/1.2, ...) are supported.
 */
#ifndef IPL_H
#define IPL_H

#include <gccore.h>

typedef struct {
	u8  *data;    /* decompressed, 32-byte aligned (free with ipl_free) */
	u32  size;
} IplRes;

/* the resources used by the boot animation */
typedef struct {
	IplRes letters;    /* J3D model: NINTENDO letters + GAMECUBE text      */
	IplRes letters_bck;/* J3D1bck1 joint animation of the letters model     */
	IplRes letters_ipk;/* IPK1 material colour animation of the letters     */
	IplRes big_cube;   /* J3D model: big translucent cube                   */
	IplRes s_cube;     /* J3D model: small rolling cube                     */
	IplRes logo;       /* J3D model: final cube logo                        */
	IplRes tile_tex;   /* BTI: tile texture (64x64 I8)                      */
} IplAssets;

/* Load "path" (scrambled or already descrambled) into memory.  Returns 0 on success. */
int  ipl_load_file(const char *path, u8 **out, u32 *size);
/* Descramble in place if needed; returns 1 if the buffer looks like a valid IPL. */
int  ipl_prepare(u8 *ipl, u32 size);
/* Extract every asset of the boot animation.  Returns 0 on success. */
int  ipl_extract_assets(const u8 *ipl, u32 size, IplAssets *a);
void ipl_free_assets(IplAssets *a);

#endif
