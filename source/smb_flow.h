/*
 * smb_flow.h - the game's state as the SMB mode follows it: the boot (title screen
 * never shown, optional test start at any world-level), one life, and the events
 * that end the mode (death, the 1-1 flagpole, the end of the game).
 *
 * Pure C (no libogc), so it is tested on PC against the NES core. RAM addresses and
 * names from doppelganger's Comprehensive Super Mario Bros. Disassembly.
 */
#ifndef SMB_FLOW_H
#define SMB_FLOW_H

#include <stdint.h>
#include "nes/nes.h"

/* SMB RAM */
#define SMB_ENGINE        0x000e   /* GameEngineSubroutine */
#define SMB_PSTATE        0x001d   /* Player_State: 0 ground, 1 jump, 2 fall, 3 climbing */
#define SMB_YHIGH         0x00b5   /* Player_Y_HighPos */
#define SMB_AREATYPE      0x074e
#define SMB_AREAPTR       0x0750   /* AreaPointer */
#define SMB_LIVES         0x075a   /* NumberofLives (0 = the last one) */
#define SMB_LEVEL         0x075c   /* LevelNumber (the dash number - 1) */
#define SMB_WORLD         0x075f   /* WorldNumber (0..7) */
#define SMB_AREANUM       0x0760   /* AreaNumber */
#define SMB_CLOUD         0x0743   /* CloudTypeOverride */
#define SMB_TIMEUP        0x0759   /* GameTimerExpiredFlag */
#define SMB_OPERMODE      0x0770   /* 0 title, 1 game, 2 victory, 3 game over */
#define SMB_OPERTASK      0x0772   /* OperMode_Task */
#define SMB_WORLDENDTIMER 0x07a1
#define SMB_CONTINUEWORLD 0x07fd
#define SMB_EVENTMUSIC    0x07b1   /* EventMusicBuffer */

#define SMB_ENG_FLAGPOLE  0x04
#define SMB_ENG_ENDLEVEL  0x05
#define SMB_ENG_LOSELIFE  0x06
#define SMB_ENG_PLAY      0x08
#define SMB_ENG_DEATH     0x0b

typedef struct {
	int full;                  /* 1: the whole game; 0: World 1-1 only */
} SmbFlowCfg;

/* boot: the pad for boot frame f (Start on the title screen) */
uint8_t smb_flow_boot_pad(const SmbFlowCfg *c, int f);
/* boot: call after every boot frame; 1 once the first frame of play is reached */
int  smb_flow_boot_after(nes_t *n, const SmbFlowCfg *c, int f);

enum { SMB_EV_NONE = 0, SMB_EV_DEATH, SMB_EV_FLAG, SMB_EV_VICTORY };

/* after every frame of play: keeps one life, returns what ends the mode, if anything:
 *  - SMB_EV_DEATH: he dies (enemy, lava, time up) or falls into a hole (not off a
 *    cloud bonus area: the game takes him back down)
 *  - SMB_EV_FLAG: (1-1 mode only) down the pole and turned round
 *  - SMB_EV_VICTORY: (full game) Peach's message is complete (world 8) */
int  smb_flow_frame(nes_t *n, const SmbFlowCfg *c);

/* the game is drawing the level (not a black world screen or loading) */
int  smb_flow_in_play(const nes_t *n);
/* still in World 1-1 (its main area or its underground room) */
int  smb_flow_in_1_1(const nes_t *n);
/* an id of the area on screen: changes when a pipe, a vine or a new level is taken */
int  smb_flow_area_id(const nes_t *n);

#endif
