/*
 * smb_layout.c - see smb_layout.h
 */
#include <string.h>
#include "smb_layout.h"
#include <math.h>

static void left_layer(const nes_t *n, const SmbLayoutCfg *c, SmbView *v);
static void enemy_ghosts(const nes_t *n, SmbView *v);

void smb_layout_defaults(SmbLayoutCfg *c)
{
	c->ground_tile = 0xb4;
	c->ending = 0;
	c->text_out = 0;
	c->fw_out = 0;
	c->no_sprites = 0;
	c->hide_castle = 1;
}

static uint16_t nt_off(const nes_t *n, uint16_t addr);
static uint8_t nt_tile(const nes_t *n, int cx, int cy);

/* SMB: the flagpole's shaft tile */
#define POLE_TILE 0xa2

static int32_t find_pole(const nes_t *n, const SmbView *v)
{
	int32_t col;
	int r;
	for (col = v->next_col - 31; col < v->next_col; col++) {
		if (col < 0)
			continue;
		for (r = 3; r < 12; r++)
			if (nt_tile(n, (int)((col * 2) & 63), r * 2) == POLE_TILE)
				return col;
	}
	return -1;
}

void smb_score_bitmap(const nes_t *n, uint8_t out[48 * 8])
{
	/* status bar row 3, tiles 3..8 (nametable $2000), background pattern table */
	uint16_t pt = (n->line_ctrl[8] & 0x10) ? 0x1000 : 0;
	int i, r, b;
	for (i = 0; i < 6; i++) {
		uint8_t t = n->nt[nt_off(n, (uint16_t)(0x2000 | (3 << 5) | (3 + i)))];
		for (r = 0; r < 8; r++) {
			uint8_t lo = n->chr[pt + t * 16 + r], hi = n->chr[pt + t * 16 + r + 8];
			for (b = 0; b < 8; b++)
				out[r * 48 + i * 8 + b] = (uint8_t)((((lo | hi) >> (7 - b)) & 1));
		}
	}
}

static uint16_t nt_off(const nes_t *n, uint16_t addr)
{
	addr &= 0x0fff;
	return n->mirror_vertical ? (addr & 0x7ff) : (uint16_t)(((addr >> 1) & 0x400) | (addr & 0x3ff));
}

/* nametable tile at ring position (cx 0..63, cy 0..29) of the horizontal playfield */
static uint8_t nt_tile(const nes_t *n, int cx, int cy)
{
	uint16_t a = (uint16_t)(0x2000 | (((cx >> 5) & 1) << 10) | (cy << 5) | (cx & 31));
	return n->nt[nt_off(n, a)];
}


static int line_scroll(const nes_t *n, int line)
{
	uint16_t v = n->line_v[line];
	return ((v & 0x1f) * 8 + n->line_fx[line] + ((v >> 10) & 1) * 256) & 511;
}

/* one sprite row into an 8-pixel span: returns the 2-bit pixels in out[8] */
static void sprite_row(const nes_t *n, const uint8_t *s, int hgt, int rr, uint8_t ctrl, uint8_t out[8])
{
	uint8_t attr = s[2], lo, hi;
	uint16_t pa;
	int j;
	if (attr & 0x80)
		rr = hgt - 1 - rr;
	if (hgt == 8)
		pa = (uint16_t)(((ctrl & 0x08) ? 0x1000 : 0) + s[1] * 16 + rr);
	else {
		uint8_t t = s[1] & 0xfe;
		if (rr >= 8) { t++; rr -= 8; }
		pa = (uint16_t)(((s[1] & 1) ? 0x1000 : 0) + t * 16 + rr);
	}
	lo = n->chr[pa]; hi = n->chr[pa + 8];
	for (j = 0; j < 8; j++) {
		int bit = (attr & 0x40) ? j : 7 - j;
		out[j] = (uint8_t)(((lo >> bit) & 1) | (((hi >> bit) & 1) << 1));
	}
}

/* background opacity of the picture (incl. the 3D ground), filled by smb_layout:
 * a sprite with the "behind background" bit (Mario going into a pipe) is hidden there */
static uint8_t bg_opaque[SMB_VIEW_H][SMB_VIEW_W];

/* the player's 8 sprites into v->ply (bounding box at ply_x/ply_y) */
static void player_overlay(const nes_t *n, SmbView *v)
{
	int off = n->ram[0x6e4] & 0xfc, i, rr, j, bx = 999, by = 999, ex = -999, ey = -999;
	uint8_t ctrl = n->line_ctrl[200];
	int hgt = (ctrl & 0x20) ? 16 : 8;
	memset(v->ply, SMB_LAYER_TRANSPARENT, sizeof(v->ply));
	v->ply_on = 0;
	v->ply_nspr = 0;
	for (i = 0; i < 8; i++) {
		const uint8_t *s = &n->oam[(off + i * 4) & 0xff];
		if (s[0] >= 0xef)
			continue;
		v->ply_nspr++;
		if (s[3] < bx) bx = s[3];
		if (s[0] + 1 < by) by = s[0] + 1;
		if (s[3] + 8 > ex) ex = s[3] + 8;
		if (s[0] + 1 + hgt > ey) ey = s[0] + 1 + hgt;
	}
	if (bx == 999)
		return;
	/* the sprites sit at the bottom middle of the 32x32 box, so its bottom middle is
	   his feet (small or big Mario) and turning the box about its middle keeps him in
	   place horizontally */
	bx -= (SMB_PLY - (ex - bx)) / 2;
	by -= SMB_PLY - (ey - by);
	v->ply_x = (int16_t)bx;
	v->ply_y = (int16_t)by;
	for (i = 0; i < 8; i++) {
		const uint8_t *s = &n->oam[(off + i * 4) & 0xff];
		uint8_t px8[8];
		if (s[0] >= 0xef)
			continue;
		for (rr = 0; rr < hgt; rr++) {
			int yy = s[0] + 1 + rr - by;
			if (yy < 0 || yy >= SMB_PLY)
				continue;
			sprite_row(n, s, hgt, rr, ctrl, px8);
			for (j = 0; j < 8; j++) {
				int xx = s[3] + j - bx, sx = s[3] + j, sl = s[0] + 1 + rr - SMB_VIEW_Y0;
				if (!px8[j] || xx < 0 || xx >= SMB_PLY || v->ply[yy * SMB_PLY + xx] != SMB_LAYER_TRANSPARENT)
					continue;
				if ((s[2] & 0x20) && sx < SMB_VIEW_W && sl >= 0 && sl < SMB_VIEW_H && bg_opaque[sl][sx])
					continue;                            /* behind the background (a pipe) */
				v->ply[yy * SMB_PLY + xx] = (uint8_t)(16 + (s[2] & 3) * 4 + px8[j]);
				v->ply_on = 1;
			}
		}
	}
}

/* ---- floors ----
 * SMB's area renderer (AreaParserCore): the scenery, then the terrain written into
 * MetatileBuffer ($06A1, 13 rows from screen row 2) from the TerrainRenderBits pattern
 * of TerrainControl ($0727); then at RendBBuf ($94D3) "jsr ProcessAreaData" places the
 * objects over it; at $94D6 the column is final. Bit i of a pattern: buffer row i. */
#define SMB_PC_RENDBBUF  0x94d3
#define SMB_PC_COLDONE   0x94d6
static const uint16_t terrain_bits[16] = {
	0x0000, 0x1800, 0x1801, 0x1807, 0x180f, 0x18ff, 0x1f01, 0x1f07,
	0x1f0f, 0x1f81, 0x0001, 0x1f8f, 0x1ff1, 0x18f9, 0x18f1, 0x1fff,
};
#define FLOOR_RING 64
static struct {
	int32_t col;               /* level column, -1: none */
	uint32_t gen;              /* area it belongs to */
	uint16_t rows;             /* floor rows (bit i: buffer row i = metatile row i + 2) */
} floor_ring[FLOOR_RING];
static struct { uint16_t rows; uint8_t mt[13]; } floor_pending;
static uint32_t floor_gen;
static int32_t floor_last_col = -1;

static uint16_t floor_rows_of(uint16_t bits)
{
	uint16_t rows = 0;
	int i;
	for (i = 12; i >= 0 && (bits >> i) & 1; i--)
		rows |= (uint16_t)(1u << i);           /* the run from the bottom up */
	return rows & 0x1800;                      /* its 2 bottom rows (raised floors: their own
	                                              tiles above, as solid columns) */
}

static void floor_hook(void *user, const nes_t *n, uint16_t pc)
{
	int i;
	(void)user;
	if (pc == SMB_PC_RENDBBUF) {
		floor_pending.rows = n->ram[0x743] ? 0 : floor_rows_of(terrain_bits[n->ram[0x727] & 15]);
		for (i = 0; i < 13; i++)
			floor_pending.mt[i] = n->ram[0x6a1 + i];
	} else {
		int32_t col = (int32_t)n->ram[0x725] * 16 + n->ram[0x726];
		uint16_t rows = floor_pending.rows;
		for (i = 0; i < 13; i++)
			if (((rows >> i) & 1) && n->ram[0x6a1 + i] != floor_pending.mt[i])
				rows &= (uint16_t)~(1u << i);      /* an object took its place */
		if (col <= floor_last_col - 2 || col > floor_last_col + 40)
			floor_gen++;                        /* another area: it starts over */
		floor_last_col = col;
		floor_ring[col & (FLOOR_RING - 1)].col = col;
		floor_ring[col & (FLOOR_RING - 1)].gen = floor_gen;
		floor_ring[col & (FLOOR_RING - 1)].rows = rows;
	}
}

void smb_floor_install(nes_t *n)
{
	int i;
	for (i = 0; i < FLOOR_RING; i++)
		floor_ring[i].col = -1;
	floor_gen = 0;
	floor_last_col = -1;
	n->pc_hook = floor_hook;
	n->pc_hook_at[0] = SMB_PC_RENDBBUF;
	n->pc_hook_at[1] = SMB_PC_COLDONE;
	n->pc_hook_user = NULL;
}

int smb_floor_at(int32_t col, int row)
{
	int i = row - 2;
	if (col < 0 || i < 0 || i > 12)
		return 0;
	if (floor_ring[col & (FLOOR_RING - 1)].col != col || floor_ring[col & (FLOOR_RING - 1)].gen != floor_gen)
		return 0;
	return (floor_ring[col & (FLOOR_RING - 1)].rows >> i) & 1;
}

/* ---- text on screen ----
 * SMB's font: tiles $00-$09 digits, $0A-$23 letters, $24 blank, then a few signs
 * ($28 '-', $29 'x', $2B '!', $AF '.'). A screen row is text when it holds nothing else and at
 * least 3 letters (so the warp zone's lone digits and the "x 3" line are not). */
static int screen_tile(const nes_t *n, const SmbView *v, int col, int row)
{
	return nt_tile(n, ((v->scroll >> 3) + col) & 63, row);
}

static int row_is_text(const nes_t *n, const SmbView *v, int r)
{
	int c, letters = 0;
	for (c = 0; c < 32; c++) {
		int t = screen_tile(n, v, c, r);
		if (t >= 0x0a && t <= 0x23)
			letters++;
		else if (t > 0x2b && t != 0xaf)        /* ($AF: the full stop) */
			return 0;
	}
	return letters >= 3;
}

static void find_text_rows(const nes_t *n, const SmbLayoutCfg *c, SmbView *v)
{
	int r;
	memset(v->text_row, 0, sizeof(v->text_row));
	if (!c->text_out)
		return;
	for (r = 4; r < 28; r++)                  /* (rows 0-3: the status bar) */
		v->text_row[r] = (uint8_t)row_is_text(n, v, r);
}

static void glyph(const nes_t *n, uint16_t pt, int t, uint8_t *dst, int stride)
{
	int r, b;
	for (r = 0; r < 8; r++) {
		uint8_t ink = (uint8_t)(n->chr[pt + t * 16 + r] | n->chr[pt + t * 16 + r + 8]);
		for (b = 0; b < 8; b++)
			dst[r * stride + b] = (uint8_t)((ink >> (7 - b)) & 1);
	}
}

int smb_text(const nes_t *n, const SmbView *v, int with_score, SmbText *t)
{
	uint16_t pt = (n->line_ctrl[200] & 0x10) ? 0x1000 : 0;
	int rows[16], nrows = 0, r, c, c0 = 32, c1 = -1, last = -1, i;
	memset(t->bits, 0, sizeof(t->bits));
	t->w = t->h = 0;
	for (r = 4; r < 28 && nrows < 14; r++) {
		if (!v->text_row[r])
			continue;
		if (last >= 0 && r > last + 1)
			rows[nrows++] = -1;               /* one blank line between */
		rows[nrows++] = r;
		last = r;
		for (c = 0; c < 32; c++) {
			int tl = screen_tile(n, v, c, r);
			if (tl != 0x24 && (tl < 0x2c || tl == 0xaf)) {
				if (c < c0) c0 = c;
				if (c > c1) c1 = c;
			}
		}
	}
	if (!nrows)
		return 0;
	if (with_score && nrows < 14) {
		/* the score, from the status bar (row 3, columns 3-8), centred */
		int w = c1 - c0 + 1, s0 = c0 + (w - 6) / 2;
		if (w < 6) { c0 -= (6 - w + 1) / 2; c1 = c0 + 5; s0 = c0; }
		rows[nrows++] = -1;
		rows[nrows++] = -100 - s0;            /* (marks the score line) */
	}
	t->w = (c1 - c0 + 1) * 8;
	t->h = nrows * 8;
	for (i = 0; i < nrows; i++) {
		uint8_t *dst = &t->bits[i * 8 * SMB_TEXT_W];
		if (rows[i] == -1)
			continue;
		if (rows[i] <= -100) {
			int s0 = -100 - rows[i];
			for (c = 0; c < 6; c++) {
				int tl = nt_tile(n, 3 + c, 3);      /* (status bar: nametable $2000) */
				if (s0 + c >= c0 && s0 + c <= c1)
					glyph(n, pt, tl, dst + (s0 + c - c0) * 8, SMB_TEXT_W);
			}
			continue;
		}
		for (c = c0; c <= c1; c++) {
			int tl = screen_tile(n, v, c, rows[i]);
			if (tl != 0x24 && (tl < 0x2c || tl == 0xaf))
				glyph(n, pt, tl, dst + (c - c0) * 8, SMB_TEXT_W);
		}
	}
	return 1;
}

void smb_text_lines(const nes_t *n, const char *const *lines, int nlines, SmbText *t)
{
	/* not in SMB's font: ':' (two dots), '^' (a degree sign), '@' (a clock) */
	static const uint8_t colon[8] = {0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00};
	static const uint8_t degree[8] = {0x30, 0x48, 0x48, 0x30, 0x00, 0x00, 0x00, 0x00};
	static const uint8_t clock[8] = {0x3c, 0x42, 0x91, 0x91, 0x9d, 0x81, 0x42, 0x3c};
	uint16_t pt = (n->line_ctrl[200] & 0x10) ? 0x1000 : 0;
	int i, c, wmax = 0;
	memset(t->bits, 0, sizeof(t->bits));
	for (i = 0; i < nlines; i++) {
		int l = (int)strlen(lines[i]);
		if (l > wmax) wmax = l;
	}
	if (wmax > SMB_TEXT_W / 8) wmax = SMB_TEXT_W / 8;
	if (nlines > (SMB_TEXT_H + 4) / 12) nlines = (SMB_TEXT_H + 4) / 12;
	t->w = wmax * 8;
	t->h = nlines * 12 - 4;               /* (4 pixels between lines) */
	for (i = 0; i < nlines; i++) {
		int l = (int)strlen(lines[i]), x0;
		if (l > wmax) l = wmax;
		x0 = (wmax - l) * 4;                    /* centred */
		for (c = 0; c < l; c++) {
			char ch = lines[i][c];
			uint8_t *dst = &t->bits[i * 12 * SMB_TEXT_W + x0 + c * 8];
			int tl = -1, r, b;
			if (ch >= '0' && ch <= '9') tl = ch - '0';
			else if (ch >= 'A' && ch <= 'Z') tl = 0x0a + ch - 'A';
			else if (ch >= 'a' && ch <= 'z' && ch != 'x') tl = 0x0a + ch - 'a';
			else if (ch == '-') tl = 0x28;
			else if (ch == 'x') tl = 0x29;
			else if (ch == '!') tl = 0x2b;
			else if (ch == '.') tl = 0xaf;
			else if (ch == ':' || ch == '^' || ch == '@') {
				const uint8_t *g = (ch == ':') ? colon : (ch == '^') ? degree : clock;
				for (r = 0; r < 8; r++)
					for (b = 0; b < 8; b++)
						dst[r * SMB_TEXT_W + b] = (uint8_t)((g[r] >> (7 - b)) & 1);
				continue;
			}
			if (tl >= 0)
				glyph(n, pt, tl, dst, SMB_TEXT_W);
		}
	}
}

int smb_floor_row(const nes_t *n, const SmbView *v)
{
	uint16_t pt = (n->line_ctrl[200] & 0x10) ? 0x1000 : 0;
	int votes[30], c, r, best = -1, bv = 0;
	memset(votes, 0, sizeof(votes));
	for (c = 2; c < 14; c++) {
		int top = -1;
		for (r = 27; r >= 4; r--) {
			int tl = screen_tile(n, v, c, r), k, ink = 0;
			for (k = 0; k < 16; k++)
				ink |= n->chr[pt + tl * 16 + k];
			if (ink)
				top = r;
			else if (top >= 0)
				break;                         /* the top of the lowest run */
		}
		if (top >= 0)
			votes[top]++;
	}
	for (r = 0; r < 30; r++)
		if (votes[r] > bv) { bv = votes[r]; best = r; }
	return best;
}

/* the main colour of the metatile in ring column slot (16 px), row r: the NES colour
   most of its pixels have (the backdrop excluded), 0xff if it has none */
static uint8_t meta_colour(const nes_t *n, int slot, int r)
{
	uint16_t pt = (n->line_ctrl[200] & 0x10) ? 0x1000 : 0;
	int cnt[4] = {0, 0, 0, 0}, tx, ty, k, b, best = 0;
	int cx0 = (slot * 2) & 63, cy0 = r * 2;
	uint8_t attr = 0;
	for (ty = 0; ty < 2; ty++)
		for (tx = 0; tx < 2; tx++) {
			int cx = (cx0 + tx) & 63, cy = cy0 + ty;
			uint8_t tile = nt_tile(n, cx, cy);
			for (k = 0; k < 8; k++) {
				uint8_t lo = n->chr[pt + tile * 16 + k], hi = n->chr[pt + tile * 16 + k + 8];
				for (b = 0; b < 8; b++)
					cnt[((lo >> b) & 1) | (((hi >> b) & 1) << 1)]++;
			}
		}
	{
		/* one attribute for the metatile (16x16 px lies in one 16x16 quadrant) */
		uint16_t a = (uint16_t)(0x2000 | (((cx0 >> 5) & 1) << 10) | 0x3c0 | ((cy0 >> 2) << 3) | ((cx0 & 31) >> 2));
		int shift = ((cy0 & 2) << 1) | (cx0 & 2);
		attr = (uint8_t)((n->nt[nt_off(n, a)] >> shift) & 3);
	}
	for (k = 1; k < 4; k++)
		if (cnt[k] > (best ? cnt[best] : 0))
			best = k;
	if (!best)
		return 0xff;
	return (uint8_t)(n->pal[attr * 4 + best] & 0x3f);
}

void smb_layout(const nes_t *n, const SmbLayoutCfg *c, SmbView *v)
{
	int k, r, x, y, cas0, cas1;
	int poff = n->ram[0x6e4] & 0xfc;
	uint8_t bgop[SMB_VIEW_W];      /* background opaque (incl. 3D tiles), for sprite priority */
	uint8_t skip[64];

	/* ending: the flagpole flag ($30) and the castle's star flag ($31) are not shown
	   (SMB RAM Enemy_ID $16+i, Enemy_SprDataOffset $06E5+i, 6 sprites each) */
	memset(skip, 0, sizeof(skip));
	for (k = 0; k < 6; k++)
		if (n->ram[0x0f + k] && c->ending && (n->ram[0x16 + k] == 0x30 || n->ram[0x16 + k] == 0x31)) {
				int j, o = n->ram[0x6e5 + k] >> 2;
				for (j = 0; j < 6; j++)
					skip[(o + j) & 63] = 1;
			}
	/* fireworks (fw_out: drawn face on): the PPU's OAM was filled a frame ago, with
	   the sprite offsets of then (the game shuffles them every frame), so the blocks of
	   both frames are marked (2), and only the explosion's own tiles are left out
	   there - or the flagpole's flag, in the block of a firework the frame before,
	   would blink */
	if (c->fw_out) {
		static uint8_t prev_on[6], prev_off[6];
		for (k = 0; k < 6; k++) {
			int on = n->ram[0x0f + k] && n->ram[0x16 + k] == 0x16, j;
			if (on || prev_on[k]) {
				int o1 = n->ram[0x6e5 + k] >> 2, o0 = prev_off[k] >> 2;
				for (j = 0; j < 6; j++) {
					if (on) skip[(o1 + j) & 63] |= 2;
					if (prev_on[k]) skip[(o0 + j) & 63] |= 2;
				}
			}
			prev_on[k] = (uint8_t)on;
			prev_off[k] = n->ram[0x6e5 + k];
		}
	}

	/* playfield scroll, taken on a line under the status bar split */
	v->scroll = line_scroll(n, 200);
	find_text_rows(n, c, v);
	/* SMB RAM: ScreenLeft_PageLoc ($071A), ScreenLeft_X_Pos ($071C) */
	v->level_x = (int32_t)n->ram[0x71a] * 256 + n->ram[0x71c];
	/* RAM is one NMI ahead of the picture: align it on the PPU scroll actually used */
	v->level_x -= ((v->level_x - v->scroll + 256) & 511) - 256;

	/* ---- 3D metatiles ---- */
	v->nmeta = 0;
	v->next_col = smb_next_col(n);
	if (v->next_col < (v->level_x >> 4) + 17)
		v->next_col = (v->level_x >> 4) + 17;   /* (one frame at a page change reads low) */
	v->pole_col = find_pole(n, v);
	for (k = -(SMB_LEFT_W / 16) - 1; k <= 16 + SMB_AHEAD; k++) {
		int slot = (v->scroll >> 4) + k;                 /* 16-px column in the 512-px ring */
		int px = slot * 16 - v->scroll;
		int32_t col = (v->level_x + px + 8) >> 4;
		if (col < 0 || col >= v->next_col || col < v->next_col - 31)
			continue;                                    /* not (or no longer) in the nametables */
		for (r = 2; r < 15; r++) {
			int kind = smb_floor_at(col, r) ? SMB_MT_GROUND : 0;
			if (!kind || v->nmeta >= SMB_MAX_META)
				continue;
			v->meta[v->nmeta].px = (int16_t)px;
			v->meta[v->nmeta].row = (uint8_t)r;
			v->meta[v->nmeta].kind = (uint8_t)kind;
			v->meta[v->nmeta].col = (v->level_x + px + 8) >> 4;
			v->meta[v->nmeta].ncol = meta_colour(n, slot, r);
			v->nmeta++;
		}
	}

	player_overlay(n, v);
	if (c->no_sprites) {
		int g;
		v->ply_on = 0;
		v->nmeta = 0;                         /* (the next area's floors are already known) */
		memset(v->left, SMB_LAYER_TRANSPARENT, sizeof(v->left));
		for (g = 0; g < SMB_ENEMIES; g++)
			v->ghost[g].on = 0;
	}
	/* the screen turned off (between a world screen and the level): no floors either */
	if (!(n->line_mask[200] & 0x08))
		v->nmeta = 0;
	left_layer(n, c, v);
	enemy_ghosts(n, v);
	if (c->ending) {
		/* only the game's sprites: no ground, no strip, no edge enemies */
		int g;
		v->nmeta = 0;
		memset(v->left, SMB_LAYER_TRANSPARENT, sizeof(v->left));
		for (g = 0; g < SMB_ENEMIES; g++)
			v->ghost[g].on = 0;
	}

	/* palette RAM of this frame: the layer stores palette slots, coloured by the renderer */
	for (k = 0; k < 32; k++)
		v->pal[k] = n->pal[(k & 3) ? k : 0] & 0x3f;

	/* ---- flat layer: background minus sky and 3D tiles, plus sprites ---- */
	/* (the castle's columns, as picture x: cas0 <= x < cas1) */
	cas0 = cas1 = 0;
	if (v->pole_col >= 0 && c->hide_castle) {
		int32_t a = (v->pole_col + SMB_CASTLE_X0) * 16 - v->level_x;
		int32_t b = (v->pole_col + SMB_CASTLE_X1 + 1) * 16 - v->level_x;
		cas0 = a < -1 ? -1 : a > SMB_VIEW_W ? SMB_VIEW_W : (int)a;
		cas1 = b < -1 ? -1 : b > SMB_VIEW_W ? SMB_VIEW_W : (int)b;
	}
	for (y = 0; y < SMB_VIEW_H; y++) {
		int line = SMB_VIEW_Y0 + y;
		uint16_t lv = n->line_v[line];
		uint8_t fx = n->line_fx[line], ctrl = n->line_ctrl[line], mask = n->line_mask[line];
		uint8_t *row = v->layer + y * SMB_VIEW_W;
		memset(row, SMB_LAYER_TRANSPARENT, SMB_VIEW_W);
		memset(bgop, 0, sizeof bgop);

		if ((mask & 0x08) && !c->ending && !c->no_sprites) {
			int cx0 = lv & 0x1f, cy = (lv >> 5) & 0x1f, fy = (lv >> 12) & 7;
			int nth = (lv >> 10) & 1, ntv = (lv >> 11) & 1;
			uint16_t pt = (ctrl & 0x10) ? 0x1000 : 0;
			for (x = 0; x < SMB_VIEW_W; ) {
				/* one tile at a time: its pattern row and attribute, then its pixels */
				int px = fx + x, tx = cx0 + (px >> 3), h = nth, shift, end, t3;
				uint16_t base, pa;
				uint8_t tile, at, lo, hi, attr;
				end = x + 8 - (px & 7);
				if (end > SMB_VIEW_W) end = SMB_VIEW_W;
				if (tx >= 32) { tx -= 32; h ^= 1; }
				base = (uint16_t)(0x2000 | (ntv << 11) | (h << 10));
				tile = n->nt[nt_off(n, (uint16_t)(base | (cy << 5) | tx))];
				t3 = smb_floor_at((v->level_x + x) >> 4, cy >> 1);
				if (c->text_out && cy < 30 && v->text_row[cy] && (tile < 0x2c || tile == 0xaf) && tile != 0x24) {
					x = end;                             /* text: shown face on */
					continue;
				}
				pa = (uint16_t)(pt + tile * 16 + fy);
				lo = n->chr[pa];
				hi = n->chr[pa + 8];
				if (!t3 && !(lo | hi)) {
					x = end;                             /* all sky */
					continue;
				}
				at = n->nt[nt_off(n, (uint16_t)(base | 0x3c0 | ((cy >> 2) << 3) | (tx >> 2)))];
				shift = ((cy & 2) << 1) | (tx & 2);
				attr = (uint8_t)(((at >> shift) & 3) << 2);
				for (; x < end; x++) {
					int bit;
					uint8_t p;
					if (x < 8 && !(mask & 0x02))
						continue;
					if (t3) {                            /* drawn in 3D: hole in the layer */
						bgop[x] = 1;
						continue;
					}
					if (x >= cas0 && x < cas1 && line < 208)
						continue;                        /* the castle: the logo replaces it */
					bit = 7 - ((fx + x) & 7);
					p = (uint8_t)(((lo >> bit) & 1) | (((hi >> bit) & 1) << 1));
					if (!p)
						continue;                        /* sky */
					row[x] = (uint8_t)(attr | p);
					bgop[x] = 1;
				}
			}
		}

		memcpy(bg_opaque[y], bgop, SMB_VIEW_W);
		if ((mask & 0x10) && !c->no_sprites) {
			int hgt = (ctrl & 0x20) ? 16 : 8, found[64], nf = 0, i;
			uint8_t done[SMB_VIEW_W];
			const uint8_t *oam = c->ending ? &n->ram[0x200] : n->oam;
			/* no 8-per-line limit (no flicker); sprite 0 is SMB's status-bar split marker */
			for (i = 1; i < 64; i++) {
				int rr = line - (oam[i * 4] + 1);
				if (((i * 4 - poff) & 0xff) < 32)
					continue;                            /* the player: drawn on its own */
				if ((skip[i] & 1) || ((skip[i] & 2) && oam[i * 4 + 1] >= 0x66 && oam[i * 4 + 1] <= 0x68))
					continue;                            /* the flags; fireworks (fw_out) */
				if (rr < 0 || rr >= hgt)
					continue;
				found[nf++] = i;
			}
			memset(done, 0, sizeof done);
			for (k = 0; k < nf; k++) {
				const uint8_t *s = &oam[found[k] * 4];
				int rr = line - (s[0] + 1), j;
				uint8_t attr = s[2], lo, hi;
				uint16_t pa;
				if (attr & 0x80)
					rr = hgt - 1 - rr;
				if (hgt == 8)
					pa = (uint16_t)(((ctrl & 0x08) ? 0x1000 : 0) + s[1] * 16 + rr);
				else {
					uint8_t t = s[1] & 0xfe;
					if (rr >= 8) { t++; rr -= 8; }
					pa = (uint16_t)(((s[1] & 1) ? 0x1000 : 0) + t * 16 + rr);
				}
				lo = n->chr[pa]; hi = n->chr[pa + 8];
				for (j = 0; j < 8; j++) {
					int xx = s[3] + j, bit;
					uint8_t p;
					if (xx >= SMB_VIEW_W)
						break;
					if (done[xx])
						continue;
					bit = (attr & 0x40) ? j : 7 - j;
					p = (uint8_t)(((lo >> bit) & 1) | (((hi >> bit) & 1) << 1));
					if (!p || (xx < 8 && !(mask & 0x04)))
						continue;
					done[xx] = 1;
					if ((attr & 0x20) && bgop[xx])
						continue;                        /* behind the background */
					row[xx] = (uint8_t)(16 + (attr & 3) * 4 + p);
				}
			}
		}
	}
}

/* SMB RAM: CurrentPageLoc ($0725), CurrentColumnPos ($0726): the next column the
 * area parser draws into the nametable */
/* the background left of the picture: same tiles/attributes as the playfield, read
 * straight from the nametables at x = scroll - SMB_LEFT_W .. scroll - 1 */
static void left_layer(const nes_t *n, const SmbLayoutCfg *c, SmbView *v)
{
	int y, x, x0;
	int32_t first = (v->next_col - 31) * 16;            /* oldest level x still there */
	uint8_t ctrl = n->line_ctrl[200];
	uint16_t pt = (ctrl & 0x10) ? 0x1000 : 0;
	if (first < 0)
		first = 0;
	memset(v->left, SMB_LAYER_TRANSPARENT, sizeof(v->left));
	/* (level x increases with x: the part still in the nametables starts at x0) */
	x0 = (int)(first - (v->level_x - SMB_LEFT_W));
	if (x0 < 0) x0 = 0;
	for (y = 0; y < SMB_VIEW_H; y++) {
		int line = SMB_VIEW_Y0 + y, cy = line >> 3, fy = line & 7;
		uint8_t *row = v->left + y * SMB_LEFT_W;
		for (x = x0; x < SMB_LEFT_W; ) {
			/* one tile at a time, as for the playfield */
			int rx = (v->scroll - SMB_LEFT_W + x) & 511;     /* x in the 512-px ring */
			int tx = (rx >> 3) & 31, h = (rx >> 8) & 1, end = x + 8 - (rx & 7), shift;
			uint16_t base = (uint16_t)(0x2000 | (h << 10)), pa;
			uint8_t tile, lo, hi, at, attr;
			if (end > SMB_LEFT_W) end = SMB_LEFT_W;
			tile = n->nt[nt_off(n, (uint16_t)(base | (cy << 5) | tx))];
			pa = (uint16_t)(pt + tile * 16 + fy);
			lo = n->chr[pa];
			hi = n->chr[pa + 8];
			if (smb_floor_at((v->level_x - SMB_LEFT_W + x) >> 4, cy >> 1) || !(lo | hi)) {
				x = end;                                 /* floor (IPL tiles) or sky */
				continue;
			}
			at = n->nt[nt_off(n, (uint16_t)(base | 0x3c0 | ((cy >> 2) << 3) | (tx >> 2)))];
			shift = ((cy & 2) << 1) | (tx & 2);
			attr = (uint8_t)(((at >> shift) & 3) << 2);
			for (; x < end; x++) {
				int bit = 7 - ((v->scroll - SMB_LEFT_W + x) & 7);
				uint8_t p = (uint8_t)(((lo >> bit) & 1) | (((hi >> bit) & 1) << 1));
				if (p)
					row[x] = (uint8_t)(attr | p);
			}
		}
	}
}

/* ---- enemies at the left edge ----
 * SMB RAM: Enemy_Flag $0F+i, Enemy_ID $16+i, Enemy_PageLoc $6E+i, Enemy_X_Position
 * $87+i, Enemy_Y_HighPos $B6+i, Enemy_Y_Position $CF+i, Enemy_MovingDir $46+i,
 * Enemy_SprDataOffset $06E5+i (6 sprites in the RAM sprite buffer $0200, which goes
 * with this frame's RAM state). */
static struct {
	int have, cur, last_change, period;
	uint8_t id, dir[2];
	int16_t ox, oy;
	uint8_t img[2][SMB_GW * SMB_GH];
} cache[SMB_ENEMIES];
static int frame_no;

void smb_layout_reset(void)
{
	memset(cache, 0, sizeof(cache));
	frame_no = 0;
}

static void enemy_ghosts(const nes_t *n, SmbView *v)
{
	int k, j, r, b;
	frame_no++;
	for (k = 0; k < SMB_ENEMIES; k++) {
		SmbGhost *g = &v->ghost[k];
		int alive = n->ram[0x0f + k] != 0 && n->ram[0xb6 + k] == 1;
		int32_t ax = (int32_t)n->ram[0x6e + k] * 256 + n->ram[0x87 + k];
		int rel = (int)(ax - v->level_x), ey = n->ram[0xcf + k], off = n->ram[0x6e5 + k];
		int any = 0, bx = 999, by = 999;
		g->on = 0;
		if (!alive || n->ram[0x16 + k] != cache[k].id) {
			if (!alive) { cache[k].have = 0; continue; }
			cache[k].have = 0;                      /* a new enemy in this slot */
			cache[k].id = n->ram[0x16 + k];
		}
		for (j = 0; j < 6; j++) {
			const uint8_t *sp = &n->ram[0x200 + ((off + j * 4) & 0xff)];
			if (sp[0] >= 0xef)
				continue;
			any = 1;
			if (sp[3] < bx) bx = sp[3];
			if (sp[0] + 1 < by) by = sp[0] + 1;
		}
		if (any && rel >= 8 && rel <= 232) {
			/* in full view: keep its image (two alternating frames: walking) */
			uint8_t tmp[SMB_GW * SMB_GH];
			uint8_t ctrl = n->line_ctrl[200];
			memset(tmp, SMB_LAYER_TRANSPARENT, sizeof(tmp));
			for (j = 0; j < 6; j++) {
				const uint8_t *sp = &n->ram[0x200 + ((off + j * 4) & 0xff)];
				uint8_t px8[8];
				if (sp[0] >= 0xef)
					continue;
				for (r = 0; r < 8; r++) {
					int yy = sp[0] + 1 + r - by, pa, bit, xx;
					uint8_t lo, hi;
					if (yy < 0 || yy >= SMB_GH)
						continue;
					pa = ((ctrl & 0x08) ? 0x1000 : 0) + sp[1] * 16 + ((sp[2] & 0x80) ? 7 - r : r);
					lo = n->chr[pa]; hi = n->chr[pa + 8];
					for (b = 0; b < 8; b++) {
						bit = (sp[2] & 0x40) ? b : 7 - b;
						px8[b] = (uint8_t)(((lo >> bit) & 1) | (((hi >> bit) & 1) << 1));
						xx = sp[3] + b - bx;
						if (px8[b] && xx >= 0 && xx < SMB_GW && tmp[yy * SMB_GW + xx] == SMB_LAYER_TRANSPARENT)
							tmp[yy * SMB_GW + xx] = (uint8_t)(16 + (sp[2] & 3) * 4 + px8[b]);
					}
				}
			}
			if (!cache[k].have) {
				memcpy(cache[k].img[0], tmp, sizeof(tmp));
				memcpy(cache[k].img[1], tmp, sizeof(tmp));
				cache[k].cur = 0;
				cache[k].period = 0;
				cache[k].last_change = frame_no;
				cache[k].have = 1;
			} else if (memcmp(tmp, cache[k].img[cache[k].cur], sizeof(tmp))) {
				int was = cache[k].cur ^ 1;
				cache[k].period = frame_no - cache[k].last_change;
				cache[k].last_change = frame_no;
				cache[k].cur = was;
				memcpy(cache[k].img[was], tmp, sizeof(tmp));
			}
			cache[k].dir[cache[k].cur] = n->ram[0x46 + k];
			cache[k].ox = (int16_t)(bx - rel);
			cache[k].oy = (int16_t)(by - ey);
			continue;
		}
		if (any || rel >= 8 || rel < -96 || !cache[k].have)
			continue;                                /* drawn by the game, or nothing to show */
		/* hidden by the game at the left edge: its live position, its last images */
		{
			int i = cache[k].cur, x, y;
			const uint8_t *src;
			if (cache[k].period > 0 && cache[k].period < 32)
				i = ((frame_no - cache[k].last_change) / cache[k].period) & 1 ? cache[k].cur ^ 1 : cache[k].cur;
			src = cache[k].img[i];
			g->on = 1;
			g->x = (int16_t)(rel + cache[k].ox);
			g->y = (int16_t)(ey + cache[k].oy);
			/* turned round since (a Koopa): mirrored */
			if (n->ram[0x46 + k] != cache[k].dir[i]) {
				for (y = 0; y < SMB_GH; y++)
					for (x = 0; x < SMB_GW; x++)
						g->img[y * SMB_GW + x] = src[y * SMB_GW + SMB_GW - 1 - x];
			} else
				memcpy(g->img, src, sizeof(g->img));
		}
	}
}

int smb_slot_image(const nes_t *n, int k, uint8_t img[SMB_GW * SMB_GH])
{
	return smb_slot_image_at(n, k, img, NULL, NULL);
}

int smb_slot_image_at(const nes_t *n, int k, uint8_t img[SMB_GW * SMB_GH], int *ox, int *oy)
{
	int off = n->ram[0x6e5 + k], j, r, b, bx = 999, by = 999, any = 0;
	uint8_t ctrl = n->line_ctrl[200];
	memset(img, SMB_LAYER_TRANSPARENT, SMB_GW * SMB_GH);
	for (j = 0; j < 6; j++) {
		const uint8_t *sp = &n->ram[0x200 + ((off + j * 4) & 0xff)];
		if (sp[0] >= 0xef)
			continue;
		any = 1;
		if (sp[3] < bx) bx = sp[3];
		if (sp[0] + 1 < by) by = sp[0] + 1;
	}
	if (!any)
		return 0;
	if (ox) *ox = bx;
	if (oy) *oy = by;
	for (j = 0; j < 6; j++) {
		const uint8_t *sp = &n->ram[0x200 + ((off + j * 4) & 0xff)];
		if (sp[0] >= 0xef)
			continue;
		for (r = 0; r < 8; r++) {
			int yy = sp[0] + 1 + r - by, pa;
			uint8_t lo, hi;
			if (yy < 0 || yy >= SMB_GH)
				continue;
			pa = ((ctrl & 0x08) ? 0x1000 : 0) + sp[1] * 16 + ((sp[2] & 0x80) ? 7 - r : r);
			lo = n->chr[pa]; hi = n->chr[pa + 8];
			for (b = 0; b < 8; b++) {
				int bit = (sp[2] & 0x40) ? b : 7 - b, xx = sp[3] + b - bx;
				uint8_t p = (uint8_t)(((lo >> bit) & 1) | (((hi >> bit) & 1) << 1));
				if (p && xx >= 0 && xx < SMB_GW && img[yy * SMB_GW + xx] == SMB_LAYER_TRANSPARENT)
					img[yy * SMB_GW + xx] = (uint8_t)(16 + (sp[2] & 3) * 4 + p);
			}
		}
	}
	return 1;
}

int smb_firework_image(const nes_t *n, int frame, uint8_t img[SMB_GW * SMB_GH])
{
	/* as the game's DrawExplosion_Fireworks: one tile (ExplosionTiles), four times,
	   flipped into the four corners, sprite palette 2 */
	static const uint8_t tiles[3] = {0x68, 0x67, 0x66};
	uint16_t pt = (n->line_ctrl[200] & 0x08) ? 0x1000 : 0;
	int q, r, b;
	memset(img, SMB_LAYER_TRANSPARENT, SMB_GW * SMB_GH);
	if (frame < 0 || frame > 2)
		return 0;
	for (q = 0; q < 4; q++) {
		int fx = q & 1, fy = q >> 1;          /* (flips: the right and lower quarters) */
		for (r = 0; r < 8; r++) {
			int pa = pt + tiles[frame] * 16 + (fy ? 7 - r : r);
			uint8_t lo = n->chr[pa], hi = n->chr[pa + 8];
			for (b = 0; b < 8; b++) {
				int bit = fx ? b : 7 - b;
				uint8_t p = (uint8_t)(((lo >> bit) & 1) | (((hi >> bit) & 1) << 1));
				if (p)
					img[(fy * 8 + r) * SMB_GW + fx * 8 + b] = (uint8_t)(16 + 2 * 4 + p);
			}
		}
	}
	return 1;
}

int smb_mario_icon(const nes_t *n, int big, uint8_t img[SMB_PLY * SMB_PLY])
{
	/* the game's standing frames (PlayerGraphicsTable: "player standing"), 2 tiles
	   wide, 4 high (small: the bottom 2), sprite palette 0; a tile repeated on the
	   right is the left one mirrored (as the game draws it); centred, feet at the bottom */
	static const uint8_t big_t[8] = {0x00, 0x01, 0x4c, 0x4d, 0x4a, 0x4a, 0x4b, 0x4b};
	static const uint8_t small_t[4] = {0x3a, 0x37, 0x4f, 0x4f};
	uint16_t pt = (n->line_ctrl[200] & 0x08) ? 0x1000 : 0;
	int rows = big ? 4 : 2, i, r, b;
	memset(img, SMB_LAYER_TRANSPARENT, SMB_PLY * SMB_PLY);
	for (i = 0; i < rows * 2; i++) {
		const uint8_t *tt = big ? big_t : small_t;
		uint8_t tile = tt[i];
		int mirror = (i & 1) && tt[i] == tt[i - 1];
		int x0 = (SMB_PLY - 16) / 2 + (i & 1) * 8, y0 = SMB_PLY - rows * 8 + (i >> 1) * 8;
		for (r = 0; r < 8; r++) {
			uint8_t lo = n->chr[pt + tile * 16 + r], hi = n->chr[pt + tile * 16 + r + 8];
			for (b = 0; b < 8; b++) {
				int bit = mirror ? b : 7 - b;
				uint8_t p = (uint8_t)(((lo >> bit) & 1) | (((hi >> bit) & 1) << 1));
				if (p)
					img[(y0 + r) * SMB_PLY + x0 + b] = (uint8_t)(16 + p);
			}
		}
	}
	return 1;
}

int32_t smb_next_col(const nes_t *n)
{
	return (int32_t)n->ram[0x725] * 16 + n->ram[0x726];
}

int smb_ground_at(const nes_t *n, const SmbLayoutCfg *c, const SmbView *v, int32_t col, int row)
{
	int32_t x = col * 16;
	/* column -1, row 13 is the trail's branch tile (18,18), where the top cube starts;
	   nothing else exists left of the level (the logo stays intact) */
	if (col < 0)
		return col == -1 && row == 13;
	/* the 512-px nametable ring holds the 32 columns before the next one the game
	   will draw (verified against the whole of 1-1: always valid) */
	(void)x;
	if (col >= v->next_col || col < v->next_col - 32)
		return 0;
	(void)n;
	(void)c;
	return smb_floor_at(col, row);
}

/* the two cubes laying the ground (see smb_layout.h). The top one starts on the
 * branch tile (level column start_col = -1, row 13) and rolls at its own pace,
 * independently of Mario; the bottom one drops in on row 14, column 0, waits for
 * the top one to be over it and rolls under it. They never go past the columns the
 * game has drawn (next_col). Once Mario plays (push set) they are kept past the
 * picture's right edge, so he never catches up with them. They run once. */
void smb_cubes_reset(SmbCubeState *st)
{
	st->started = 0;
}

static void place(const nes_t *n, const SmbLayoutCfg *c, SmbView *v, SmbCube *k, int row, float p)
{
	int32_t c0 = (int32_t)floorf(p), a = -1, b = -1, j;
	float f = p - (float)c0;
	int have_a = 0, have_b = 0;
	for (j = 0; j < 8 && !have_a; j++)
		if (smb_ground_at(n, c, v, c0 - j, row)) { a = c0 - j; have_a = 1; }
	for (j = 1; j < 8 && !have_b; j++)
		if (smb_ground_at(n, c, v, c0 + j, row)) { b = c0 + j; have_b = 1; }
	k->row = row;
	k->roll = 0.0f;
	k->lift = 0.0f;
	k->hop = 0.0f;
	k->visible = have_a;
	if (!have_a) {
		v->ground_limit[row] = c0;
		k->col = c0;
		return;
	}
	v->ground_limit[row] = a;
	k->col = a;
	if (have_b && a == c0 && b == c0 + 1) {
		k->roll = f;                              /* rolling onto the next tile */
	} else if (have_b) {
		float t = (p - (float)a) / (float)(b - a); /* a gap: jump to the next tile */
		k->hop = (float)(b - a) * t;
		k->lift = 4.0f * (0.6f + 0.25f * (float)(b - a)) * t * (1.0f - t);
	}
}

void smb_cubes(const nes_t *n, const SmbLayoutCfg *c, SmbView *v, SmbCubeState *st)
{
	float step = 1.0f / (float)(st->roll_frames > 0 ? st->roll_frames : 10);
	int r;

	if (!st->started) {
		st->p = (float)st->start_col;
		st->pb = (float)st->start_col + 1.0f;      /* the bottom one drops in on the next column */
		st->t = 0;
		st->started = 1;
	} else if (v->level_x < st->last_x - 64) {
		st->p = st->pb = 1.0e6f;                 /* restarted after a death: all built */
	}
	st->last_x = v->level_x;
	st->t++;
	st->p += step;
	if (st->push) {
		/* once out of the picture they stay out of it: Mario can never catch up,
		   the scroll carries them along if he runs faster than they roll */
		float edge = (float)(v->level_x >> 4) + 17.0f;
		if (st->p < edge)
			st->p = edge;
	}
	if (st->p < 1.0e5f && st->p > (float)(v->next_col - 1))
		st->p = (float)(v->next_col - 1);         /* never past what the game has drawn */
	/* the bottom one waits where it dropped until the top one is over it, then
	   they roll together, one on top of the other */
	if (st->p > st->pb || st->p >= 1.0e5f)
		st->pb = st->p;
	for (r = 0; r < 15; r++)
		v->ground_limit[r] = (int32_t)floorf(st->p);
	place(n, c, v, &v->cube[0], 13, st->p);
	place(n, c, v, &v->cube[1], 14, st->pb);
	if (st->t <= st->drop_frames && st->drop_frames > 0) {
		/* the bottom cube falls in: lift 3 tiles -> 0, like a drop */
		float u = (float)st->t / (float)st->drop_frames;
		v->cube[1].lift = 3.0f * (1.0f - u * u);
		v->cube[1].roll = 0.0f;
		v->cube[1].hop = 0.0f;
		v->ground_limit[14] = -1;
	}
	for (r = 0; r < 13; r++)
		v->ground_limit[r] = v->ground_limit[13] > v->ground_limit[14] ? v->ground_limit[13] : v->ground_limit[14];
}
