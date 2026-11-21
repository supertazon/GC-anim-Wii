/*
 * smb_records.h - the SMB mode's records (smb.sav) and run log (smb_runs.txt),
 * next to smb.nes. Pure C (stdio only), tested on PC.
 *
 * Times are counted in NES frames, from the first frame Mario can move to the end
 * of the run (the 1-1 flagpole, or Peach's message in the whole game), so they do
 * not depend on the Wii, 50/60 Hz or slowdowns; shown at the NES rate (60.0988 fps).
 * The C-stick angle is kept as the stick's own values at launch (exact, can be given
 * back in the .ini) and shown as one number: how far the view is turned, in degrees.
 */
#ifndef SMB_RECORDS_H
#define SMB_RECORDS_H

#include <stdint.h>

enum { SMB_REC_11 = 0, SMB_REC_FULL = 1 };

typedef struct {
	int have_score, have_time;
	uint32_t score;            /* points */
	uint32_t frames;           /* best time, NES frames */
	int score_ax, score_ay;    /* C-stick angle of each record */
	int score_world, score_level;   /* where the best score's run ended (0: unknown) */
	int score_finished;        /* ... or it finished (1-1 flagpole / Peach's message) */
	int time_ax, time_ay;
} SmbRecord;

typedef struct {
	SmbRecord r[2];            /* SMB_REC_11, SMB_REC_FULL */
	int unlocked;              /* the whole game finished once (original colours) */
} SmbRecords;

/* one run, as it ended */
typedef struct {
	int mode;                  /* SMB_REC_11 / SMB_REC_FULL */
	int finished;              /* 1-1 flagpole / Peach's message; 0: died */
	uint32_t score;
	uint32_t frames;           /* NES frames since Mario could move */
	int world, level;          /* where it ended (1-based) */
	int ax, ay;                /* C-stick at launch */
	int original_colours;
} SmbRun;

void smb_records_load(SmbRecords *r, const char *path);   /* missing file: empty */
int  smb_records_save(const SmbRecords *r, const char *path);
/* updates the records with a run; returns bit 0: new best score, bit 1: new best time */
int  smb_records_add(SmbRecords *r, const SmbRun *run);
/* appends one line to the run log; date "YYYY-MM-DD HH:MM" or NULL */
int  smb_runs_log(const char *path, const SmbRun *run, const char *date);

/* the result under the logo, 1 to 3 lines (out), columns lined up: this run (score,
   its time if finished or where it ended in the whole game, C-stick angle, status:
   "NEW BEST", "NOT SAVED n" or ""), then the best before it (prev), one line: its
   best score, its best time (or where the best score's run ended), an angle, BEST */
int  smb_result_table(const SmbRun *run, int full, const char *status, const SmbRecord *prev,
                      char out[3][64]);
/* "mm:ss.cc" (hours added if needed) */
void smb_time_str(uint32_t frames, char out[16]);
/* how far the view is turned by the C-stick values, in whole degrees */
int  smb_angle_deg(int ax, int ay);
/* the score in points from SMB's RAM (PlayerScoreDisplay $07DD: 6 digits, tens) */
uint32_t smb_score_of(const uint8_t *ram);

#endif
