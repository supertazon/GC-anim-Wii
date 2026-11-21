/* nes.c - minimal NES core for GC-anim-Wii's SMB mode (MIT License)
 * See nes.h for scope. CPU is instruction-stepped; the PPU is caught up to the
 * exact cycle of each PPU register access, which keeps vblank/sprite-0 polling
 * loops correct without a per-cycle CPU. */
#include "nes.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/* PPU                                                                 */
/* ------------------------------------------------------------------ */

static uint16_t nt_index(const nes_t *n, uint16_t addr)
{
    addr &= 0x0fff;
    if (n->mirror_vertical) return addr & 0x07ff;
    return (uint16_t)(((addr >> 1) & 0x400) | (addr & 0x3ff));
}

static uint8_t pal_index(uint16_t addr)
{
    uint8_t a = addr & 0x1f;
    if ((a & 0x13) == 0x10) a &= ~0x10;
    return a;
}

uint8_t nes_ppu_peek(const nes_t *n, uint16_t addr)
{
    addr &= 0x3fff;
    if (addr < 0x2000) return n->chr[addr];
    if (addr < 0x3f00) return n->nt[nt_index(n, addr)];
    return n->pal[pal_index(addr)] & 0x3f;
}

static void ppu_write_mem(nes_t *n, uint16_t addr, uint8_t v)
{
    addr &= 0x3fff;
    if (addr < 0x2000) { if (n->chr_is_ram) n->chr_ram[addr] = v; }
    else if (addr < 0x3f00) n->nt[nt_index(n, addr)] = v;
    else n->pal[pal_index(addr)] = v;
}

static void update_nmi(nes_t *n)
{
    uint8_t line = (n->status & 0x80) && (n->ctrl & 0x80);
    if (line && !n->nmi_line) n->nmi_pending = 1;
    n->nmi_line = line;
}

static void inc_y(nes_t *n)
{
    if ((n->v & 0x7000) != 0x7000) { n->v += 0x1000; return; }
    n->v &= ~0x7000;
    uint16_t y = (n->v & 0x03e0) >> 5;
    if (y == 29) { y = 0; n->v ^= 0x0800; }
    else if (y == 31) y = 0;
    else y++;
    n->v = (uint16_t)((n->v & ~0x03e0) | (y << 5));
}

/* is the background pixel at screen x (on the line starting with scroll v/fx) opaque? */
static int bg_opaque(const nes_t *n, uint16_t v, uint8_t fx, int x)
{
    int px = fx + x;
    int coarse = (v & 0x1f) + (px >> 3);
    int nth = (v >> 10) & 1;
    if (coarse >= 32) { coarse -= 32; nth ^= 1; }
    int ntv = (v >> 11) & 1, cy = (v >> 5) & 31, fy = (v >> 12) & 7;
    uint16_t na = (uint16_t)(0x2000 | (ntv << 11) | (nth << 10) | (cy << 5) | coarse);
    uint8_t tile = n->nt[nt_index(n, na)];
    uint16_t pa = (uint16_t)(((n->ctrl & 0x10) ? 0x1000 : 0) + tile * 16 + fy);
    int bit = 7 - (px & 7);
    return ((n->chr[pa] >> bit) & 1) | ((n->chr[pa + 8] >> bit) & 1);
}

/* compute where (if anywhere) sprite 0 hits on visible line `line` */
static int sprite0_dot(const nes_t *n, int line)
{
    if ((n->mask & 0x18) != 0x18) return -1;
    int h = (n->ctrl & 0x20) ? 16 : 8;
    int row = line - (n->oam[0] + 1);
    if (row < 0 || row >= h) return -1;
    uint8_t tile = n->oam[1], attr = n->oam[2], sx = n->oam[3];
    if (attr & 0x80) row = h - 1 - row;
    uint16_t pa;
    if (h == 8) pa = (uint16_t)(((n->ctrl & 0x08) ? 0x1000 : 0) + tile * 16 + row);
    else {
        uint16_t base = (tile & 1) ? 0x1000 : 0;
        uint8_t t = tile & 0xfe;
        if (row >= 8) { t++; row -= 8; }
        pa = (uint16_t)(base + t * 16 + row);
    }
    uint8_t lo = n->chr[pa], hi = n->chr[pa + 8];
    for (int i = 0; i < 8; i++) {
        int bit = (attr & 0x40) ? i : 7 - i;
        if (!(((lo >> bit) & 1) | ((hi >> bit) & 1))) continue;
        int x = sx + i;
        if (x >= 255) break;
        if (x < 8 && (!(n->mask & 0x02) || !(n->mask & 0x04))) continue;
        if (bg_opaque(n, n->v, n->fx, x)) return x + 1;
    }
    return -1;
}

static void ppu_dot(nes_t *n)
{
    int rendering = n->mask & 0x18;
    int sl = n->scanline, d = n->dot;

    if (sl < 240) {
        if (d == 0) {
            n->line_v[sl] = n->v; n->line_fx[sl] = n->fx;
            n->line_ctrl[sl] = n->ctrl; n->line_mask[sl] = n->mask;
            n->s0_dot = (n->status & 0x40) ? -1 : sprite0_dot(n, sl);
        }
        if (d == n->s0_dot) { n->status |= 0x40; n->s0_dot = -1; }
        if (rendering) {
            if (d == 256) inc_y(n);
            else if (d == 257) n->v = (uint16_t)((n->v & ~0x041f) | (n->t & 0x041f));
        }
    } else if (sl == 241) {
        if (d == 1) { n->status |= 0x80; n->frame++; update_nmi(n); }
    } else if (sl == 261) {
        if (d == 1) { n->status &= 0x1f; update_nmi(n); }
        if (rendering) {
            if (d == 256) inc_y(n);
            else if (d == 257) n->v = (uint16_t)((n->v & ~0x041f) | (n->t & 0x041f));
            else if (d >= 280 && d <= 304) n->v = (uint16_t)((n->v & ~0x7be0) | (n->t & 0x7be0));
        }
    }

    n->ppu_cycles++;
    n->dot++;
    if (sl == 261 && n->dot == 340 && n->odd && rendering) n->dot = 341;  /* odd-frame skip */
    if (n->dot > 340) {
        n->dot = 0;
        if (++n->scanline > 261) { n->scanline = 0; n->odd ^= 1; }
    }
}

/* run the PPU up to (but not including) CPU cycle `cpu_cycle` */
/* the next dot (>= d) of line sl at which ppu_dot does more than count: the dots in
   between are skipped in one go (the PPU's registers cannot change during a catch-up) */
static int ppu_next_event(const nes_t *n, int sl, int d)
{
    if (sl < 240) {
        int e = d == 0 ? 0 : d <= 256 ? 256 : d == 257 ? 257 : 340;
        if (n->s0_dot >= d && n->s0_dot < e) e = n->s0_dot;
        return e;
    }
    if (sl == 241) return d <= 1 ? 1 : 340;
    if (sl == 261) {
        if (d <= 1) return 1;
        if (d <= 256) return 256;
        if (d == 257) return 257;
        if (d < 280) return 280;
        if (d <= 304) return d;
        if (d <= 339) return 339;
        return 340;
    }
    return 340;
}

static void ppu_catchup(nes_t *n, uint64_t cpu_cycle)
{
    uint64_t target = cpu_cycle * 3;
    while (n->ppu_cycles < target) {
        int e = ppu_next_event(n, n->scanline, n->dot);
        uint64_t k = (uint64_t)(e - n->dot);
        if (k == 0) { ppu_dot(n); continue; }
        if (k > target - n->ppu_cycles) k = target - n->ppu_cycles;
        n->dot += (int)k;
        n->ppu_cycles += k;
    }
}

static uint8_t ppu_reg_read(nes_t *n, uint16_t reg)
{
    uint8_t r = n->openbus;
    switch (reg & 7) {
    case 2:
        r = (uint8_t)((n->status & 0xe0) | (n->openbus & 0x1f));
        n->status &= 0x7f; n->w = 0;
        update_nmi(n);
        break;
    case 4: r = n->oam[n->oamaddr]; break;
    case 7: {
        uint16_t a = n->v & 0x3fff;
        if (a < 0x3f00) { r = n->rbuf; n->rbuf = nes_ppu_peek(n, a); }
        else { r = (uint8_t)((nes_ppu_peek(n, a) & 0x3f) | (n->openbus & 0xc0)); n->rbuf = nes_ppu_peek(n, a - 0x1000); }
        n->v = (uint16_t)((n->v + ((n->ctrl & 0x04) ? 32 : 1)) & 0x7fff);
        break;
    }
    default: break;
    }
    n->openbus = r;
    return r;
}

static void ppu_reg_write(nes_t *n, uint16_t reg, uint8_t v)
{
    n->openbus = v;
    switch (reg & 7) {
    case 0:
        n->ctrl = v;
        n->t = (uint16_t)((n->t & 0xf3ff) | ((v & 3) << 10));
        update_nmi(n);
        break;
    case 1: n->mask = v; break;
    case 3: n->oamaddr = v; break;
    case 4: n->oam[n->oamaddr++] = v; break;
    case 5:
        if (!n->w) { n->t = (uint16_t)((n->t & 0xffe0) | (v >> 3)); n->fx = v & 7; n->w = 1; }
        else { n->t = (uint16_t)((n->t & 0x8c1f) | ((v & 7) << 12) | ((v & 0xf8) << 2)); n->w = 0; }
        break;
    case 6:
        if (!n->w) { n->t = (uint16_t)((n->t & 0x00ff) | ((v & 0x3f) << 8)); n->w = 1; }
        else { n->t = (uint16_t)((n->t & 0xff00) | v); n->v = n->t; n->w = 0; }
        break;
    case 7:
        ppu_write_mem(n, n->v, v);
        n->v = (uint16_t)((n->v + ((n->ctrl & 0x04) ? 32 : 1)) & 0x7fff);
        break;
    default: break;
    }
}

/* ------------------------------------------------------------------ */
/* APU (registers + length counters + frame sequencer only)            */
/* ------------------------------------------------------------------ */

static const uint8_t len_table[32] = {
    10,254,20,2,40,4,80,6,160,8,60,10,14,12,26,14,12,16,24,18,48,20,96,22,192,24,72,26,16,28,32,30 };

static void apu_half_frame(nes_t *n)
{
    for (int i = 0; i < 4; i++) if (n->len[i] && !n->len_halt[i]) n->len[i]--;
}

static void apu_tick(nes_t *n, uint32_t cyc)
{
    while (cyc) {
        /* straight to the next cycle the frame sequencer acts on */
        uint32_t c = n->frame_seq_cycle, e;
        if (!n->frame_mode5) e = c < 14913 ? 14913 : c < 29829 ? 29829 : c < 29830 ? 29830 : c + 1;
        else e = c < 14913 ? 14913 : c < 37281 ? 37281 : c < 37282 ? 37282 : c + 1;
        if (e - c > cyc) { n->frame_seq_cycle = c + cyc; return; }
        cyc -= e - c;
        n->frame_seq_cycle = c = e;
        if (!n->frame_mode5) {
            if (c == 14913) apu_half_frame(n);
            else if (c == 29829) {
                apu_half_frame(n);
                if (!n->frame_irq_inhibit) n->frame_irq_flag = 1;
            } else if (c >= 29830) n->frame_seq_cycle = 0;
        } else {
            if (c == 14913) apu_half_frame(n);
            else if (c == 37281) apu_half_frame(n);
            else if (c >= 37282) n->frame_seq_cycle = 0;
        }
    }
}

static void apu_write(nes_t *n, uint16_t a, uint8_t v)
{
    n->apu_reg[a - 0x4000] = v;
    switch (a) {
    case 0x4000: n->len_halt[0] = (v >> 5) & 1; break;
    case 0x4004: n->len_halt[1] = (v >> 5) & 1; break;
    case 0x4008: n->len_halt[2] = (v >> 7) & 1; break;
    case 0x400c: n->len_halt[3] = (v >> 5) & 1; break;
    case 0x4003: if (n->apu_enable & 1) n->len[0] = len_table[v >> 3]; break;
    case 0x4007: if (n->apu_enable & 2) n->len[1] = len_table[v >> 3]; break;
    case 0x400b: if (n->apu_enable & 4) n->len[2] = len_table[v >> 3]; break;
    case 0x400f: if (n->apu_enable & 8) n->len[3] = len_table[v >> 3]; break;
    case 0x4015:
        n->apu_enable = v;
        for (int i = 0; i < 4; i++) if (!(v & (1 << i))) n->len[i] = 0;
        break;
    case 0x4017:
        n->frame_mode5 = (v >> 7) & 1;
        n->frame_irq_inhibit = (v >> 6) & 1;
        if (n->frame_irq_inhibit) n->frame_irq_flag = 0;
        n->frame_seq_cycle = 0;
        if (n->frame_mode5) apu_half_frame(n);
        break;
    default: break;
    }
    if (n->apu_write_cb) n->apu_write_cb(n->cb_user, a, v, n->instr_start + n->instr_len - 1);
}

static uint8_t apu_status(nes_t *n)
{
    uint8_t r = 0;
    for (int i = 0; i < 4; i++) if (n->len[i]) r |= (uint8_t)(1 << i);
    if (n->frame_irq_flag) r |= 0x40;
    n->frame_irq_flag = 0;
    return r;
}

/* ------------------------------------------------------------------ */
/* CPU bus                                                             */
/* ------------------------------------------------------------------ */



uint8_t nes_cpu_peek(const nes_t *n, uint16_t a)
{
    if (a < 0x2000) return n->ram[a & 0x7ff];
    if (a >= 0x8000) return n->prg[(a - 0x8000) & n->prg_mask];
    return 0;
}

static uint8_t rd(nes_t *n, uint16_t a)
{
    if (a < 0x2000) return n->ram[a & 0x7ff];
    if (a >= 0x8000) return n->prg[(a - 0x8000) & n->prg_mask];
    if (a < 0x4000) { ppu_catchup(n, n->instr_start + n->instr_len - 1); return ppu_reg_read(n, a); }
    if (a == 0x4015) return apu_status(n);
    if (a == 0x4016 || a == 0x4017) {
        int p = a & 1;
        uint8_t r;
        if (n->strobe) r = n->pad[p] & 1;
        else { r = n->pad_shift[p] & 1; n->pad_shift[p] = (uint8_t)((n->pad_shift[p] >> 1) | 0x80); }
        return (uint8_t)(r | 0x40);
    }
    return (uint8_t)(a >> 8);   /* open bus approximation */
}

static void wr(nes_t *n, uint16_t a, uint8_t v)
{
    if (a < 0x2000) { n->ram[a & 0x7ff] = v; return; }
    if (a < 0x4000) { ppu_catchup(n, n->instr_start + n->instr_len - 1); ppu_reg_write(n, a, v); return; }
    if (a == 0x4014) {
        ppu_catchup(n, n->instr_start + n->instr_len - 1);
        uint16_t base = (uint16_t)(v << 8);
        for (int i = 0; i < 256; i++) n->oam[(uint8_t)(n->oamaddr + i)] = rd(n, (uint16_t)(base + i));
        n->dma_stall += 513 + ((n->instr_start + n->instr_len) & 1);
        return;
    }
    if (a == 0x4016) {
        n->strobe = v & 1;
        if (n->strobe) { n->pad_shift[0] = n->pad[0]; n->pad_shift[1] = n->pad[1]; }
        return;
    }
    if (a <= 0x4017) { apu_write(n, a, v); return; }
}

/* ------------------------------------------------------------------ */
/* CPU                                                                 */
/* ------------------------------------------------------------------ */

enum { FC = 0x01, FZ = 0x02, FI = 0x04, FD = 0x08, FB = 0x10, FU = 0x20, FV = 0x40, FN = 0x80 };

static const uint8_t cyc_table[256] = {
/*      0 1 2 3 4 5 6 7 8 9 A B C D E F */
/*0*/   7,6,0,0,3,3,5,0,3,2,2,0,4,4,6,0,
/*1*/   2,5,0,0,4,4,6,0,2,4,2,0,4,4,7,0,
/*2*/   6,6,0,0,3,3,5,0,4,2,2,0,4,4,6,0,
/*3*/   2,5,0,0,4,4,6,0,2,4,2,0,4,4,7,0,
/*4*/   6,6,0,0,3,3,5,0,3,2,2,0,3,4,6,0,
/*5*/   2,5,0,0,4,4,6,0,2,4,2,0,4,4,7,0,
/*6*/   6,6,0,0,3,3,5,0,4,2,2,0,5,4,6,0,
/*7*/   2,5,0,0,4,4,6,0,2,4,2,0,4,4,7,0,
/*8*/   2,6,2,0,3,3,3,0,2,2,2,0,4,4,4,0,
/*9*/   2,6,0,0,4,4,4,0,2,5,2,0,0,5,0,0,
/*A*/   2,6,2,0,3,3,3,0,2,2,2,0,4,4,4,0,
/*B*/   2,5,0,0,4,4,4,0,2,4,2,0,4,4,4,0,
/*C*/   2,6,2,0,3,3,5,0,2,2,2,0,4,4,6,0,
/*D*/   2,5,0,0,4,4,6,0,2,4,2,0,4,4,7,0,
/*E*/   2,6,2,0,3,3,5,0,2,2,2,0,4,4,6,0,
/*F*/   2,5,0,0,4,4,6,0,2,4,2,0,4,4,7,0,
};

#define SETZN(v) do { uint8_t _v = (v); n->p = (uint8_t)((n->p & ~(FZ|FN)) | (_v ? 0 : FZ) | (_v & FN)); } while (0)

static inline void push(nes_t *n, uint8_t v) { n->ram[0x100 | n->s] = v; n->s--; }
static inline uint8_t pull(nes_t *n) { n->s++; return n->ram[0x100 | n->s]; }

static inline uint16_t rd16(nes_t *n, uint16_t a) { return (uint16_t)(rd(n, a) | (rd(n, (uint16_t)(a + 1)) << 8)); }
static inline uint16_t rd16zp(nes_t *n, uint8_t a) { return (uint16_t)(rd(n, a) | (rd(n, (uint8_t)(a + 1)) << 8)); }

static void interrupt(nes_t *n, uint16_t vector, int brk)
{
    push(n, (uint8_t)(n->pc >> 8)); push(n, (uint8_t)n->pc);
    push(n, (uint8_t)(n->p | FU | (brk ? FB : 0)));
    n->p |= FI;
    n->pc = rd16(n, vector);
}

static void adc(nes_t *n, uint8_t m)
{
    unsigned s = n->a + m + (n->p & FC);
    n->p = (uint8_t)(n->p & ~(FC | FV));
    if (s > 0xff) n->p |= FC;
    if (~(n->a ^ m) & (n->a ^ s) & 0x80) n->p |= FV;
    n->a = (uint8_t)s; SETZN(n->a);
}

static void cmp(nes_t *n, uint8_t r, uint8_t m)
{
    uint8_t d = (uint8_t)(r - m);
    n->p = (uint8_t)((n->p & ~FC) | (r >= m ? FC : 0));
    SETZN(d);
}

int nes_step(nes_t *n)
{
    n->dma_stall = 0;
    if (n->nmi_pending) {
        n->nmi_pending = 0;
        n->instr_start = n->cycles; n->instr_len = 7;
        interrupt(n, 0xfffa, 0);
        n->cycles += 7;
        apu_tick(n, 7);
        ppu_catchup(n, n->cycles);
        return 7;
    }
    if (n->frame_irq_flag && !(n->p & FI)) {
        n->instr_start = n->cycles; n->instr_len = 7;
        interrupt(n, 0xfffe, 0);
        n->cycles += 7;
        apu_tick(n, 7);
        ppu_catchup(n, n->cycles);
        return 7;
    }

    if (n->pc_hook && (n->pc == n->pc_hook_at[0] || n->pc == n->pc_hook_at[1]))
        n->pc_hook(n->pc_hook_user, n, n->pc);
    if (n->pc_hook2 && n->pc == n->pc_hook2_at)
        n->pc_hook2(n->pc_hook2_user, n, n->pc);
    n->instr_start = n->cycles;
    uint8_t op = rd(n, n->pc);
    n->instr_len = cyc_table[op];
    n->pc++;
    int extra = 0;
    uint16_t addr = 0;
    uint8_t m;

/* addressing helpers */
#define IMM()   (addr = n->pc++)
#define ZP()    (addr = rd(n, n->pc++))
#define ZPX()   (addr = (uint8_t)(rd(n, n->pc++) + n->x))
#define ZPY()   (addr = (uint8_t)(rd(n, n->pc++) + n->y))
#define ABS()   (addr = rd16(n, n->pc), n->pc += 2)
#define ABSX(pen) do { uint16_t b = rd16(n, n->pc); n->pc += 2; addr = (uint16_t)(b + n->x); if (pen && ((b ^ addr) & 0xff00)) extra++; } while (0)
#define ABSY(pen) do { uint16_t b = rd16(n, n->pc); n->pc += 2; addr = (uint16_t)(b + n->y); if (pen && ((b ^ addr) & 0xff00)) extra++; } while (0)
#define INDX()  (addr = rd16zp(n, (uint8_t)(rd(n, n->pc++) + n->x)))
#define INDY(pen) do { uint16_t b = rd16zp(n, rd(n, n->pc++)); addr = (uint16_t)(b + n->y); if (pen && ((b ^ addr) & 0xff00)) extra++; } while (0)
#define BRANCH(cond) do { int8_t off = (int8_t)rd(n, n->pc++); if (cond) { uint16_t np = (uint16_t)(n->pc + off); extra += 1 + (((np ^ n->pc) & 0xff00) ? 1 : 0); n->pc = np; } } while (0)

/* operation groups, parametrised by an addressing block */
#define LOAD(reg, AM)  { AM; reg = rd(n, addr); SETZN(reg); break; }
#define STORE(reg, AM) { AM; wr(n, addr, reg); break; }
#define ALU(expr, AM)  { AM; m = rd(n, addr); expr; break; }
#define RMW(expr, AM)  { AM; m = rd(n, addr); expr; wr(n, addr, m); break; }

#define OP_ORA  n->a |= m; SETZN(n->a)
#define OP_AND  n->a &= m; SETZN(n->a)
#define OP_EOR  n->a ^= m; SETZN(n->a)
#define OP_ADC  adc(n, m)
#define OP_SBC  adc(n, (uint8_t)~m)
#define OP_CMP  cmp(n, n->a, m)
#define OP_CPX  cmp(n, n->x, m)
#define OP_CPY  cmp(n, n->y, m)
#define OP_BIT  n->p = (uint8_t)((n->p & ~(FZ|FV|FN)) | ((n->a & m) ? 0 : FZ) | (m & (FV|FN)))
#define OP_ASL  n->p = (uint8_t)((n->p & ~FC) | (m >> 7)); m = (uint8_t)(m << 1); SETZN(m)
#define OP_LSR  n->p = (uint8_t)((n->p & ~FC) | (m & 1)); m >>= 1; SETZN(m)
#define OP_ROL  { uint8_t c = n->p & FC; n->p = (uint8_t)((n->p & ~FC) | (m >> 7)); m = (uint8_t)((m << 1) | c); SETZN(m); }
#define OP_ROR  { uint8_t c = n->p & FC; n->p = (uint8_t)((n->p & ~FC) | (m & 1)); m = (uint8_t)((m >> 1) | (c << 7)); SETZN(m); }
#define OP_INC  m++; SETZN(m)
#define OP_DEC  m--; SETZN(m)

    switch (op) {
    /* loads */
    case 0xa9: LOAD(n->a, IMM()) case 0xa5: LOAD(n->a, ZP()) case 0xb5: LOAD(n->a, ZPX())
    case 0xad: LOAD(n->a, ABS()) case 0xbd: LOAD(n->a, ABSX(1)) case 0xb9: LOAD(n->a, ABSY(1))
    case 0xa1: LOAD(n->a, INDX()) case 0xb1: LOAD(n->a, INDY(1))
    case 0xa2: LOAD(n->x, IMM()) case 0xa6: LOAD(n->x, ZP()) case 0xb6: LOAD(n->x, ZPY())
    case 0xae: LOAD(n->x, ABS()) case 0xbe: LOAD(n->x, ABSY(1))
    case 0xa0: LOAD(n->y, IMM()) case 0xa4: LOAD(n->y, ZP()) case 0xb4: LOAD(n->y, ZPX())
    case 0xac: LOAD(n->y, ABS()) case 0xbc: LOAD(n->y, ABSX(1))
    /* stores */
    case 0x85: STORE(n->a, ZP()) case 0x95: STORE(n->a, ZPX()) case 0x8d: STORE(n->a, ABS())
    case 0x9d: STORE(n->a, ABSX(0)) case 0x99: STORE(n->a, ABSY(0))
    case 0x81: STORE(n->a, INDX()) case 0x91: STORE(n->a, INDY(0))
    case 0x86: STORE(n->x, ZP()) case 0x96: STORE(n->x, ZPY()) case 0x8e: STORE(n->x, ABS())
    case 0x84: STORE(n->y, ZP()) case 0x94: STORE(n->y, ZPX()) case 0x8c: STORE(n->y, ABS())
    /* ALU */
#define ALU_GROUP(base, OPX) \
    case base+0x09: ALU(OPX, IMM()) case base+0x05: ALU(OPX, ZP()) case base+0x15: ALU(OPX, ZPX()) \
    case base+0x0d: ALU(OPX, ABS()) case base+0x1d: ALU(OPX, ABSX(1)) case base+0x19: ALU(OPX, ABSY(1)) \
    case base+0x01: ALU(OPX, INDX()) case base+0x11: ALU(OPX, INDY(1))
    ALU_GROUP(0x00, OP_ORA)
    ALU_GROUP(0x20, OP_AND)
    ALU_GROUP(0x40, OP_EOR)
    ALU_GROUP(0x60, OP_ADC)
    ALU_GROUP(0xc0, OP_CMP)
    ALU_GROUP(0xe0, OP_SBC)
    case 0xe0: ALU(OP_CPX, IMM()) case 0xe4: ALU(OP_CPX, ZP()) case 0xec: ALU(OP_CPX, ABS())
    case 0xc0: ALU(OP_CPY, IMM()) case 0xc4: ALU(OP_CPY, ZP()) case 0xcc: ALU(OP_CPY, ABS())
    case 0x24: ALU(OP_BIT, ZP()) case 0x2c: ALU(OP_BIT, ABS())
    /* read-modify-write */
#define RMW_GROUP(base, OPX) \
    case base+0x06: RMW(OPX, ZP()) case base+0x16: RMW(OPX, ZPX()) \
    case base+0x0e: RMW(OPX, ABS()) case base+0x1e: RMW(OPX, ABSX(0))
    RMW_GROUP(0x00, OP_ASL)
    RMW_GROUP(0x20, OP_ROL)
    RMW_GROUP(0x40, OP_LSR)
    RMW_GROUP(0x60, OP_ROR)
    RMW_GROUP(0xc0, OP_DEC)
    RMW_GROUP(0xe0, OP_INC)
    case 0x0a: m = n->a; OP_ASL; n->a = m; break;
    case 0x2a: m = n->a; OP_ROL; n->a = m; break;
    case 0x4a: m = n->a; OP_LSR; n->a = m; break;
    case 0x6a: m = n->a; OP_ROR; n->a = m; break;
    /* register ops */
    case 0xaa: n->x = n->a; SETZN(n->x); break;
    case 0xa8: n->y = n->a; SETZN(n->y); break;
    case 0x8a: n->a = n->x; SETZN(n->a); break;
    case 0x98: n->a = n->y; SETZN(n->a); break;
    case 0xba: n->x = n->s; SETZN(n->x); break;
    case 0x9a: n->s = n->x; break;
    case 0xe8: n->x++; SETZN(n->x); break;
    case 0xc8: n->y++; SETZN(n->y); break;
    case 0xca: n->x--; SETZN(n->x); break;
    case 0x88: n->y--; SETZN(n->y); break;
    /* flags */
    case 0x18: n->p &= ~FC; break;
    case 0x38: n->p |= FC; break;
    case 0x58: n->p &= ~FI; break;
    case 0x78: n->p |= FI; break;
    case 0xb8: n->p &= ~FV; break;
    case 0xd8: n->p &= ~FD; break;
    case 0xf8: n->p |= FD; break;
    /* stack */
    case 0x48: push(n, n->a); break;
    case 0x08: push(n, (uint8_t)(n->p | FB | FU)); break;
    case 0x68: n->a = pull(n); SETZN(n->a); break;
    case 0x28: n->p = (uint8_t)((pull(n) & ~FB) | FU); break;
    /* jumps */
    case 0x4c: n->pc = rd16(n, n->pc); break;
    case 0x6c: { uint16_t p = rd16(n, n->pc);   /* page-wrap bug */
                 n->pc = (uint16_t)(rd(n, p) | (rd(n, (uint16_t)((p & 0xff00) | ((p + 1) & 0xff))) << 8)); break; }
    case 0x20: { uint16_t t = rd16(n, n->pc); uint16_t ret = (uint16_t)(n->pc + 1);
                 push(n, (uint8_t)(ret >> 8)); push(n, (uint8_t)ret); n->pc = t; break; }
    case 0x60: { uint16_t lo = pull(n); uint16_t hi = pull(n); n->pc = (uint16_t)(((hi << 8) | lo) + 1); break; }
    case 0x40: { n->p = (uint8_t)((pull(n) & ~FB) | FU); uint16_t lo = pull(n); uint16_t hi = pull(n);
                 n->pc = (uint16_t)((hi << 8) | lo); break; }
    case 0x00: n->pc++; interrupt(n, 0xfffe, 1); break;
    /* branches */
    case 0x10: BRANCH(!(n->p & FN)); break;
    case 0x30: BRANCH(n->p & FN); break;
    case 0x50: BRANCH(!(n->p & FV)); break;
    case 0x70: BRANCH(n->p & FV); break;
    case 0x90: BRANCH(!(n->p & FC)); break;
    case 0xb0: BRANCH(n->p & FC); break;
    case 0xd0: BRANCH(!(n->p & FZ)); break;
    case 0xf0: BRANCH(n->p & FZ); break;
    case 0xea: break;
    default:
        n->jammed = 1; n->bad_opcode = op;
        n->instr_len = 2;
        break;
    }

    int c = n->instr_len + extra;
    n->cycles += (uint64_t)c;
    apu_tick(n, (uint32_t)c);
    if (n->dma_stall) { n->cycles += n->dma_stall; apu_tick(n, n->dma_stall); c += (int)n->dma_stall; }
    ppu_catchup(n, n->cycles);
    (void)addr;
    return c;
}

/* ------------------------------------------------------------------ */
/* system                                                              */
/* ------------------------------------------------------------------ */

int nes_load(nes_t *n, const uint8_t *img, size_t size)
{
    memset(n, 0, sizeof *n);
    if (size < 16 || memcmp(img, "NES\x1a", 4)) return -1;
    unsigned prg_banks = img[4], chr_banks = img[5];
    unsigned mapper = (img[6] >> 4) | (img[7] & 0xf0);
    if ((img[7] & 0x0c) == 0x08) mapper &= 0xff;           /* NES 2.0: upper nibble already in byte 7 */
    if (mapper != 0) return -2;
    size_t off = 16 + ((img[6] & 4) ? 512 : 0);
    size_t prg_size = prg_banks * 16384u, chr_size = chr_banks * 8192u;
    if (prg_banks < 1 || prg_banks > 2 || size < off + prg_size + chr_size) return -3;
    n->prg = img + off;
    n->prg_mask = (uint32_t)prg_size - 1;
    if (chr_size) n->chr = img + off + prg_size;
    else { n->chr_is_ram = 1; n->chr = n->chr_ram; }
    n->mirror_vertical = img[6] & 1;
    return 0;
}

void nes_power(nes_t *n)
{
    memset(n->ram, 0, sizeof n->ram);
    n->a = n->x = n->y = 0;
    n->s = 0xfd;
    n->p = FI | FU;
    n->cycles = 0; n->ppu_cycles = 0;
    n->scanline = 0; n->dot = 0; n->frame = 0; n->odd = 0;
    n->ctrl = n->mask = n->status = n->oamaddr = 0;
    n->v = n->t = 0; n->fx = n->w = 0;
    n->s0_dot = -1;
    if (n->chr_is_ram) n->chr = n->chr_ram;     /* in case the struct was copied */
    n->pc = (uint16_t)(nes_cpu_peek(n, 0xfffc) | (nes_cpu_peek(n, 0xfffd) << 8));
    /* reset sequence takes 7 cycles */
    n->cycles = 7; ppu_catchup(n, 7);
}

void nes_set_pad(nes_t *n, int port, uint8_t buttons) { n->pad[port & 1] = buttons; }

uint32_t nes_run_frame(nes_t *n)
{
    uint64_t f = n->frame, c0 = n->cycles;
    while (n->frame == f && !n->jammed) nes_step(n);
    return (uint32_t)(n->cycles - c0);
}
