/*
 * smb_flow.c - see smb_flow.h
 */
#include "smb_flow.h"

#define BOOT_START_FRAME 40      /* Start held for 4 frames from here (the title is ready) */

uint8_t smb_flow_boot_pad(const SmbFlowCfg *c, int f)
{
	if (f < BOOT_START_FRAME || f >= BOOT_START_FRAME + 4)
		return 0;
	(void)c;
	return NES_START;
}

int smb_flow_boot_after(nes_t *n, const SmbFlowCfg *c, int f)
{
	(void)c;
	return f > BOOT_START_FRAME + 4 && n->ram[SMB_OPERMODE] == 1 && n->ram[SMB_ENGINE] == SMB_ENG_PLAY;
}

int smb_flow_frame(nes_t *n, const SmbFlowCfg *c)
{
	uint8_t eng = n->ram[SMB_ENGINE];
	/* one life: 1-ups and 100 coins give none */
	if (n->ram[SMB_LIVES] != 0 && n->ram[SMB_LIVES] < 0x80)
		n->ram[SMB_LIVES] = 0;
	if (n->ram[SMB_OPERMODE] == 1) {
		if (eng == SMB_ENG_DEATH || eng == SMB_ENG_LOSELIFE)
			return SMB_EV_DEATH;
		/* below the screen: a hole (the game's PlayerHole: off a cloud bonus area it
		   is not a death, unless the time is up). Only while the game runs the level:
		   off a cloud area the game reloads the area below, and he is still below the
		   screen while the cloud flag is already cleared */
		if (n->ram[SMB_OPERTASK] == 3 &&
		    n->ram[SMB_YHIGH] >= 2 && n->ram[SMB_YHIGH] < 0x80 &&
		    (!n->ram[SMB_CLOUD] || n->ram[SMB_TIMEUP]))
			return SMB_EV_DEATH;
		if (!c->full && eng == SMB_ENG_ENDLEVEL && n->ram[SMB_PSTATE] != 3)
			return SMB_EV_FLAG;
	}
	/* the end of the game: world 8's messages are all up and the game waits for B
	   (VictoryMode, task PlayerEndWorld, WorldEndTimer over) */
	if (c->full && n->ram[SMB_OPERMODE] == 2 && n->ram[SMB_WORLD] == 7 &&
	    n->ram[SMB_OPERTASK] == 4 && n->ram[SMB_WORLDENDTIMER] == 0)
		return SMB_EV_VICTORY;
	return SMB_EV_NONE;
}

int smb_flow_in_play(const nes_t *n)
{
	/* GameMode tasks: 0 InitializeArea, 1 ScreenRoutines (world screen), 2 setup,
	   3 GameCoreRoutine; VictoryMode draws the level throughout */
	return (n->ram[SMB_OPERMODE] == 1 && n->ram[SMB_OPERTASK] == 3) || n->ram[SMB_OPERMODE] == 2;
}

int smb_flow_in_1_1(const nes_t *n)
{
	return n->ram[SMB_WORLD] == 0 && n->ram[SMB_LEVEL] == 0;
}

int smb_flow_area_id(const nes_t *n)
{
	return (n->ram[SMB_WORLD] << 16) | (n->ram[SMB_LEVEL] << 8) | n->ram[SMB_AREAPTR];
}
