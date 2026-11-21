/*
 * letters.h - custom text in place of the 3D "NINTENDO" letters
 *
 * The IPL only has the glyphs N, I, T, E, D and O.  Those are drawn from the IPL's own
 * model; every other capital letter comes from an original block alphabet in the same
 * style (tools/letters_design.py, extruded by tools/make_letters.py into letters_data.c).
 */
#ifndef LETTERS_H
#define LETTERS_H

#include <gccore.h>

typedef struct {
	u16 ncap, nwall;          /* triangles */
	const s16 *cap, *wall;    /* x,y,z per vertex, 8 fractional bits */
} GlyphMesh;

extern const GlyphMesh glyph_mesh[26];   /* 'A'..'Z' */

#endif
