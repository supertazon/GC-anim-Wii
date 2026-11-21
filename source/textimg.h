/*
 * textimg.h - custom "GAMECUBE" logotype from a PNG/BMP/JPG/TGA image
 */
#ifndef TEXTIMG_H
#define TEXTIMG_H

#include <gccore.h>

/* Load an image file and convert it to a w x h I8 GX texture (intensity =
 * luminance x alpha, the image is fitted inside w x h keeping its aspect).
 * Returns a 32-byte aligned buffer (free() it) or NULL. */
u8 *textimg_load_i8(const char *path, u16 w, u16 h);

#endif
