#ifndef YAY0_H
#define YAY0_H

#include <gccore.h>

/* size of the decompressed data, or 0 if src is not a Yay0 stream */
u32 yay0_size(const u8 *src);
/* decompress into dst (dst must hold yay0_size(src) bytes) */
void yay0_decompress(const u8 *src, u8 *dst);

#endif
