/* Yay0 decompression (Nintendo LZ variant used by the GameCube IPL) */
#include "yay0.h"

static inline u32 rd32(const u8 *p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }
static inline u16 rd16(const u8 *p) { return (u16)(p[0] << 8 | p[1]); }

u32 yay0_size(const u8 *src)
{
	if (src[0] != 'Y' || src[1] != 'a' || src[2] != 'y' || src[3] != '0')
		return 0;
	return rd32(src + 4);
}

void yay0_decompress(const u8 *src, u8 *dst)
{
	u32 size = rd32(src + 4);
	const u8 *mask = src + 16;
	const u8 *link = src + rd32(src + 8);
	const u8 *chunk = src + rd32(src + 12);
	u32 n = 0, bits = 0, m = 0;

	while (n < size) {
		if (bits == 0) {
			m = rd32(mask);
			mask += 4;
			bits = 32;
		}
		if (m & 0x80000000) {
			dst[n++] = *chunk++;
		} else {
			u16 v = rd16(link);
			u32 dist = (v & 0xFFF) + 1, cnt = v >> 12;
			link += 2;
			cnt = cnt ? cnt + 2 : (u32)(*chunk++) + 18;
			while (cnt-- && n < size) {
				dst[n] = dst[n - dist];
				n++;
			}
		}
		m <<= 1;
		bits--;
	}
}
