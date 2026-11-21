/*
 * ipl_audio.h - extract the GameCube IPL intro jingle from a descrambled IPL
 *
 * Self-contained C99, no libogc dependency. Works on big-endian (Wii/GC)
 * and little-endian hosts: all IPL data is read byte-wise as big-endian.
 *
 * The returned PCM is native-endian int16 (big-endian on Wii, which is
 * what ASND/AESND expect).
 *
 * Allocation: memalign(32, n) when built with devkitPPC (GEKKO defined),
 * malloc() otherwise; define IPL_AUDIO_ALLOC / IPL_AUDIO_FREE to override.
 * Every buffer returned is zero-padded up to a multiple of 32 bytes, so it
 * can be handed to ASND/AESND after DCFlushRange().
 *
 * Not reentrant (uses one static ~8 KB scratch table).
 */
#ifndef IPL_AUDIO_H
#define IPL_AUDIO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Which jingle the IPL plays (argument of FUN_81357840 / sequence port 0). */
enum {
    IPL_JINGLE_NORMAL = 0, /* default boot                                   */
    IPL_JINGLE_Z_ALL4 = 1, /* Z held on all four controllers at power-on     */
    IPL_JINGLE_Z_PAD1 = 2  /* Z held on controller 1 only                    */
};

/* Error codes (negative). */
enum {
    IPL_AUDIO_OK           = 0,
    IPL_AUDIO_E_ARG        = -1,
    IPL_AUDIO_E_NO_WSYS    = -2, /* WSYS wave-system not found/invalid       */
    IPL_AUDIO_E_NO_WAVES   = -3, /* WINF wave table not parseable            */
    IPL_AUDIO_E_NO_AW      = -4, /* raw AW sample data not found             */
    IPL_AUDIO_E_NO_BANK    = -5, /* IBNK/instrument for the jingle not found */
    IPL_AUDIO_E_FORMAT     = -6, /* wave is not AFC / rates don't match      */
    IPL_AUDIO_E_NOMEM      = -7
};

typedef struct {
    int16_t *left;        /* nsamples PCM, native endian, 32-byte padded   */
    int16_t *right;
    size_t   nsamples;    /* per channel                                    */
    int      samplerate;  /* playback rate in Hz (32000 for all known IPLs) */
    uint16_t seq_volume;  /* track volume set by the sequence, Q15
                             (0x6400 = 0.781 for the normal jingle);
                             0x8000 if the sequence could not be parsed     */
    int      wave_left;   /* WSYS wave indices used (diagnostics)           */
    int      wave_right;
} ipl_audio_t;

/* Full version: pick the jingle variant, get extra info.
 * Returns IPL_AUDIO_OK or a negative error. On success free with
 * ipl_audio_free(). */
int ipl_audio_extract_ex(const uint8_t *ipl_desc, size_t len, int variant,
                         ipl_audio_t *out);

/* Simple version: normal jingle. *left / *right must be released with
 * IPL_AUDIO_FREE (free() by default) or ipl_audio_free_buf(). */
int ipl_audio_extract(const uint8_t *ipl_desc, size_t len,
                      int16_t **left, int16_t **right,
                      size_t *nsamples, int *samplerate);

void ipl_audio_free(ipl_audio_t *a);
void ipl_audio_free_buf(void *p);

/* Interleave L/R into one LRLR... buffer (for a single stereo voice:
 * guarantees sample-accurate sync). Optional gain in Q15 (0x8000 = 1.0).
 * Returned buffer is 32-byte padded; *bytes receives the size to pass to
 * ASND/AESND (multiple of 32). */
int16_t *ipl_audio_interleave(const ipl_audio_t *a, uint16_t gain_q15,
                              size_t *bytes);

const char *ipl_audio_strerror(int err);

#ifdef __cplusplus
}
#endif
#endif
