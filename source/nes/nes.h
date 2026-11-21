/* nes.h - minimal NES core for GC-anim-Wii's SMB mode (MIT License)
 *
 * Scope: NROM (mapper 0) cartridges, official 6502 opcodes, a PPU that tracks
 * registers / VRAM / OAM / scroll / vblank / sprite-0 hit but never draws pixels,
 * and APU register state (length counters, frame sequencer) plus a write log hook.
 * Rendering is the host's job: it reads nametables, pattern tables, palette,
 * OAM and the per-scanline scroll record exposed below.
 *
 * Portable C99, no allocation, endian-neutral (runs on x86 and Wii PPC).
 */
#ifndef NES_H
#define NES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* controller bits, as read serially by the game: A,B,Select,Start,Up,Down,Left,Right */
enum { NES_A = 0x01, NES_B = 0x02, NES_SELECT = 0x04, NES_START = 0x08,
       NES_UP = 0x10, NES_DOWN = 0x20, NES_LEFT = 0x40, NES_RIGHT = 0x80 };

typedef void (*nes_apu_write_fn)(void *user, uint16_t addr, uint8_t val, uint64_t cpu_cycle);
/* called before the instruction at one of the hook addresses runs (it may read RAM) */
struct nes;
typedef void (*nes_pc_hook_fn)(void *user, const struct nes *n, uint16_t pc);

typedef struct nes {
    /* ---- CPU ---- */
    uint16_t pc;
    uint8_t  a, x, y, s, p;
    uint8_t  nmi_pending;     /* edge-detected NMI waiting for the next instruction boundary */
    uint8_t  jammed;          /* hit an unsupported opcode */
    uint8_t  bad_opcode;
    uint64_t cycles;          /* total CPU cycles */
    uint64_t instr_start;     /* cycle count at start of current instruction */
    uint8_t  instr_len;       /* base cycles of current instruction (for PPU catch-up) */
    uint32_t dma_stall;       /* CPU cycles stolen by OAM DMA during this instruction */

    /* ---- memory ---- */
    uint8_t  ram[0x800];
    const uint8_t *prg;       /* 16 or 32 KiB */
    uint32_t prg_mask;
    const uint8_t *chr;       /* 8 KiB pattern tables (ROM or chr_ram) */
    uint8_t  chr_ram[0x2000];
    uint8_t  chr_is_ram;
    uint8_t  mirror_vertical; /* 1 = vertical mirroring (horizontal scrolling games) */

    /* ---- PPU ---- */
    uint8_t  ctrl, mask, status, oamaddr;
    uint16_t v, t;            /* loopy registers */
    uint8_t  fx, w;
    uint8_t  rbuf, openbus;
    uint8_t  nt[0x800];
    uint8_t  pal[32];
    uint8_t  oam[256];
    int      scanline;        /* 0..261 (261 = pre-render) */
    int      dot;             /* 0..340 */
    uint64_t ppu_cycles;      /* total dots elapsed (3 per CPU cycle) */
    uint64_t frame;           /* incremented at start of vblank */
    uint8_t  odd;
    int      s0_dot;          /* dot on the current line where sprite-0 hit triggers, -1 = none */
    uint8_t  nmi_line;        /* level of (vblank && ctrl.7) for edge detection */

    /* per-visible-line record for the host renderer: state when the line started drawing */
    uint16_t line_v[240];
    uint8_t  line_fx[240];
    uint8_t  line_ctrl[240];
    uint8_t  line_mask[240];

    /* ---- input ---- */
    uint8_t  pad[2];
    uint8_t  pad_shift[2];
    uint8_t  strobe;

    /* ---- APU (state needed by game logic) ---- */
    uint8_t  apu_reg[0x18];
    uint8_t  len[4];          /* length counters: pulse1, pulse2, triangle, noise */
    uint8_t  len_halt[4];
    uint8_t  apu_enable;      /* $4015 write */
    uint8_t  frame_mode5;     /* $4017 bit 7 */
    uint8_t  frame_irq_inhibit;
    uint8_t  frame_irq_flag;
    uint32_t frame_seq_cycle; /* CPU cycles since frame sequencer reset */

    nes_apu_write_fn apu_write_cb;
    void    *cb_user;

    nes_pc_hook_fn pc_hook;   /* NULL: none */
    uint16_t pc_hook_at[2];
    void    *pc_hook_user;
    nes_pc_hook_fn pc_hook2;  /* a second one, at one address (NULL: none) */
    uint16_t pc_hook2_at;
    void    *pc_hook2_user;
} nes_t;

/* load an iNES / NES 2.0 image (header + PRG + CHR). The image must stay alive.
 * returns 0 on success, negative on unsupported image */
int  nes_load(nes_t *n, const uint8_t *image, size_t size);
void nes_power(nes_t *n);              /* power-on state + reset vector */
void nes_set_pad(nes_t *n, int port, uint8_t buttons);
/* run until the start of the next vblank (one video frame); returns CPU cycles executed */
uint32_t nes_run_frame(nes_t *n);
/* single CPU instruction (used by the CPU test harness) */
int  nes_step(nes_t *n);

/* host access to PPU memory (pattern tables, nametables, palette) */
uint8_t nes_ppu_peek(const nes_t *n, uint16_t addr);
/* CPU bus read without side effects (for debuggers / RAM watchers) */
uint8_t nes_cpu_peek(const nes_t *n, uint16_t addr);

/* snapshots: the whole state is the struct itself; pointers into the ROM image
 * stay valid as long as the same image is loaded */
static inline void nes_save(const nes_t *n, nes_t *dst) { *dst = *n; }

#ifdef __cplusplus
}
#endif
#endif
