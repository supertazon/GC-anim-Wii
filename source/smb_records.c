/*
 * smb_records.c - see smb_records.h
 */
#include "smb_records.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

static const char *const mode_key[2] = {"1-1", "full"};

/* the original colours' unlock: not a flag but a code made from the run that earned it
   (the whole game's best time and its C-stick angle, which only a finished game gives),
   so it cannot simply be typed in, nor kept with another time */
static unsigned long long unlock_code(const SmbRecord *full)
{
	static const char salt[] = "GC-anim-Wii/SMB: thank you Mario! but our princess is in another castle";
	unsigned long long h = 1469598103934665603ull;   /* FNV-1a, 64 bits */
	char buf[64];
	int i, n = snprintf(buf, sizeof(buf), "|%lu|%d|%d|", (unsigned long)full->frames, full->time_ax, full->time_ay);
	for (i = 0; salt[i]; i++) { h ^= (unsigned char)salt[i]; h *= 1099511628211ull; }
	for (i = 0; i < n; i++) { h ^= (unsigned char)buf[i]; h *= 1099511628211ull; }
	for (i = 0; salt[i]; i++) { h ^= (unsigned char)salt[i]; h *= 1099511628211ull; }
	return h;
}

void smb_records_load(SmbRecords *r, const char *path)
{
	FILE *f = fopen(path, "r");
	char line[160];
	unsigned long long code = 0;
	int have_code = 0;
	memset(r, 0, sizeof(*r));
	if (!f)
		return;
	while (fgets(line, sizeof(line), f)) {
		char key[48];
		long a = 0, b = 0, c = 0, w = 0, l = 0, fin = 0;
		int m, n;
		if (line[0] == ';' || line[0] == '#')
			continue;
		if (sscanf(line, " unlock = %llx", &code) == 1) {
			have_code = 1;
			continue;
		}
		n = sscanf(line, " %47[^= ] = %ld , %ld , %ld , %ld , %ld , %ld", key, &a, &b, &c, &w, &l, &fin);
		if (n < 2)
			continue;
		if (!strcmp(key, "unlocked"))
			continue;                     /* (an old flag: no longer enough) */
		for (m = 0; m < 2; m++) {
			size_t kl = strlen(mode_key[m]);   /* (not "l": that is the level read above) */
			if (strncmp(key, mode_key[m], kl) || key[kl] != '_')
				continue;
			if (!strcmp(key + kl + 1, "score") && n >= 4) {
				r->r[m].have_score = 1;
				r->r[m].score = (uint32_t)a;
				r->r[m].score_ax = (int)b;
				r->r[m].score_ay = (int)c;
				if (n >= 6) {             /* (written since: where it ended) */
					r->r[m].score_world = (int)w;
					r->r[m].score_level = (int)l;
					r->r[m].score_finished = n >= 7 && fin != 0;
				}
			} else if (!strcmp(key + kl + 1, "time") && n >= 4 && a > 0) {
				r->r[m].have_time = 1;
				r->r[m].frames = (uint32_t)a;
				r->r[m].time_ax = (int)b;
				r->r[m].time_ay = (int)c;
			}
		}
	}
	fclose(f);
	r->unlocked = have_code && r->r[SMB_REC_FULL].have_time && code == unlock_code(&r->r[SMB_REC_FULL]);
}

int smb_records_save(const SmbRecords *r, const char *path)
{
	FILE *f = fopen(path, "w");
	int m;
	char ts[16];
	if (!f)
		return -1;
	fprintf(f, "; GC-anim-Wii, Super Mario Bros. records (written by the program)\n");
	fprintf(f, "; score = points, C-stick x, y, world, level, finished   time = NES frames, C-stick x, y\n");
	for (m = 0; m < 2; m++) {
		if (r->r[m].have_score)
			fprintf(f, "%s_score = %lu, %d, %d, %d, %d, %d\n", mode_key[m], (unsigned long)r->r[m].score,
			        r->r[m].score_ax, r->r[m].score_ay, r->r[m].score_world, r->r[m].score_level,
			        r->r[m].score_finished);
		if (r->r[m].have_time) {
			smb_time_str(r->r[m].frames, ts);
			fprintf(f, "%s_time = %lu, %d, %d   ; %s\n", mode_key[m], (unsigned long)r->r[m].frames,
			        r->r[m].time_ax, r->r[m].time_ay, ts);
		}
	}
	if (r->unlocked && r->r[SMB_REC_FULL].have_time)
		fprintf(f, "unlock = %016llx\n", unlock_code(&r->r[SMB_REC_FULL]));
	return fclose(f) == 0 ? 0 : -1;
}

int smb_records_add(SmbRecords *r, const SmbRun *run)
{
	SmbRecord *x = &r->r[run->mode ? 1 : 0];
	int news = 0;
	if (!x->have_score || run->score > x->score) {
		x->have_score = 1;
		x->score = run->score;
		x->score_ax = run->ax;
		x->score_ay = run->ay;
		x->score_world = run->world;
		x->score_level = run->level;
		x->score_finished = run->finished;
		news |= 1;
	}
	if (run->finished && (!x->have_time || run->frames < x->frames)) {
		x->have_time = 1;
		x->frames = run->frames;
		x->time_ax = run->ax;
		x->time_ay = run->ay;
		news |= 2;
	}
	if (run->finished && run->mode == SMB_REC_FULL)
		r->unlocked = 1;
	return news;
}

int smb_runs_log(const char *path, const SmbRun *run, const char *date)
{
	FILE *f = fopen(path, "a");
	char ts[16];
	if (!f)
		return -1;
	smb_time_str(run->frames, ts);
	fprintf(f, "%s  %-4s  %-8s %d-%d  score %06lu  time %s  angle %d (%d,%d)  colours %s\n",
	        date ? date : "----------- -----", mode_key[run->mode ? 1 : 0],
	        run->finished ? "FINISHED" : "died", run->world, run->level, (unsigned long)run->score, ts,
	        smb_angle_deg(run->ax, run->ay), run->ax, run->ay, run->original_colours ? "original" : "ipl");
	return fclose(f) == 0 ? 0 : -1;
}

void smb_time_str(uint32_t frames, char out[16])
{
	/* NES (NTSC) frame rate: 39375000 / 655171 Hz = 60.0988 */
	uint64_t cs = (uint64_t)frames * 100u * 655171u / 39375000u;   /* centiseconds */
	unsigned long s = (unsigned long)(cs / 100), h = s / 3600;
	if (h)
		snprintf(out, 16, "%lu:%02lu:%02lu.%02lu", h, (s / 60) % 60, s % 60, (unsigned long)(cs % 100));
	else
		snprintf(out, 16, "%02lu:%02lu.%02lu", s / 60, s % 60, (unsigned long)(cs % 100));
}

int smb_angle_deg(int ax, int ay)
{
	/* the view's eye, as the animation's camera turns it (intro.c cam_rotation):
	   (200, -200, 200) at rest, (200 + x, -200 + y, 200 - x) with the C-stick */
	double a[3] = {200.0, -200.0, 200.0}, b[3];
	double d, la, lb;
	b[0] = 200.0 + ax; b[1] = -200.0 + ay; b[2] = 200.0 - ax;
	d = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
	la = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
	lb = sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
	d /= la * lb;
	if (d > 1.0) d = 1.0;
	if (d < -1.0) d = -1.0;
	return (int)floor(acos(d) * 180.0 / 3.14159265358979 + 0.5);
}

uint32_t smb_score_of(const uint8_t *ram)
{
	uint32_t s = 0;
	int i;
	for (i = 0; i < 6; i++)
		s = s * 10 + (ram[0x7dd + i] % 10);
	return s * 10;              /* (the status bar shows these digits after the first,
	                               then a fixed 0: points are always a multiple of 10) */
}

int smb_result_table(const SmbRun *run, int full, const char *status, const SmbRecord *prev,
                     char out[3][64])
{
	char cell[3][4][16];
	int n = 1, sep, i, j, w[4];
	const int maxc = 32;
	memset(cell, 0, sizeof(cell));
	snprintf(cell[0][0], 16, "%06lu", (unsigned long)run->score);
	if (run->finished)
		smb_time_str(run->frames, cell[0][1]);
	else if (full)
		snprintf(cell[0][1], 16, "WORLD %d-%d", run->world & 0xff, run->level & 0xff);
	snprintf(cell[0][2], 16, "%d^", smb_angle_deg(run->ax, run->ay));
	snprintf(cell[0][3], 16, "%s", status ? status : "");
	/* the best before it, on one line: its best score, its best time (or, the whole
	   game not finished yet, where the best score's run ended), the angle of the best
	   score (of the best time if there is no score) */
	if (prev && (prev->have_score || prev->have_time)) {
		if (prev->have_score)
			snprintf(cell[n][0], 16, "%06lu", (unsigned long)prev->score);
		if (prev->have_time)
			smb_time_str(prev->frames, cell[n][1]);
		else if (full && prev->have_score && prev->score_world > 0)
			snprintf(cell[n][1], 16, "WORLD %d-%d", prev->score_world & 0xff, prev->score_level & 0xff);
		if (prev->have_score)
			snprintf(cell[n][2], 16, "%d^", smb_angle_deg(prev->score_ax, prev->score_ay));
		else
			snprintf(cell[n][2], 16, "%d^", smb_angle_deg(prev->time_ax, prev->time_ay));
		snprintf(cell[n][3], 16, "BEST");
		n++;
	}
	/* columns lined up, two spaces apart; one if too long; then without "WORLD " */
	for (sep = 2; sep >= 0; sep--) {
		int total = 0, cols = 0, gap = sep ? sep : 1;
		for (j = 0; j < 4; j++) {
			w[j] = 0;
			for (i = 0; i < n; i++) {
				const char *x = cell[i][j];
				int l;
				if (sep == 0 && !strncmp(x, "WORLD ", 6))
					x += 6;
				l = (int)strlen(x);
				if (l > w[j]) w[j] = l;
			}
			if (w[j]) {
				total += w[j] + (cols ? gap : 0);
				cols++;
			}
		}
		for (i = 0; i < n; i++) {
			int first = 1, pos = 0;
			for (j = 0; j < 4; j++) {
				const char *x = cell[i][j];
				if (!w[j])
					continue;
				if (sep == 0 && !strncmp(x, "WORLD ", 6))
					x += 6;
				if (!first)
					pos += snprintf(out[i] + pos, (size_t)(64 - pos), "%*s", gap, "");
				first = 0;
				pos += snprintf(out[i] + pos, (size_t)(64 - pos), "%-*.15s", w[j] > 15 ? 15 : w[j], x);
			}
			out[i][pos] = 0;          /* (all the same length: they start together) */
		}
		if (total <= maxc || sep == 0)
			break;
	}
	return n;
}
