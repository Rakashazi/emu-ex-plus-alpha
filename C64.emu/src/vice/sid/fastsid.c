/*
 * fastsid.c - MOS6581 (SID) emulation.
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Michael Schwendt <sidplay@geocities.com>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "fastsid.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "resources.h"
#include "sid-snapshot.h"
#include "sid.h"
#include "sound.h"
#include "snapshot.h"
#include "types.h"


#include "fixpoint.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* use wavetables (sampled waveforms) */
#define WAVETABLES

/* ADSR state */
#define ATTACK   0
#define DECAY    1
#define SUSTAIN  2
#define RELEASE  3
#define IDLE     4

#ifndef WAVETABLES
/* Current waveform */
#define TESTWAVE          0
#define PULSEWAVE         1
#define SAWTOOTHWAVE      2
#define TRIANGLEWAVE      3
#define NOISEWAVE         4
#define NOWAVE            5
#define RINGWAVE          6
#define PULSETRIANGLEWAVE 7
#define PULSESAWTOOTHWAVE 8
#endif

/* noise magic */
#define NSHIFT(v, n) \
    (((v) << (n))    \
     | ((((v) >> (23 - (n))) ^ (v >> (18 - (n)))) & ((1 << (n)) - 1)))

#define NVALUE(v)                                   \
    (noiseLSB[v & 0xff] | noiseMID[(v >> 8) & 0xff] \
     | noiseMSB[(v >> 16) & 0xff])

#define NSEED 0x7ffff8

#ifdef WAVETABLES

#include "wave6581.h"
#include "wave8580.h"

static WORD wavetable00[2];
static WORD wavetable10[4096];
static WORD wavetable20[4096];
static WORD wavetable30[4096];
static WORD wavetable40[8192];
static WORD wavetable50[8192];
static WORD wavetable60[8192];
static WORD wavetable70[8192];

#endif

/* Noise tables */
#define NOISETABLESIZE 256
static BYTE noiseMSB[NOISETABLESIZE];
static BYTE noiseMID[NOISETABLESIZE];
static BYTE noiseLSB[NOISETABLESIZE];

/* needed data for one voice */
typedef struct voice_s {
    struct sound_s      *s;
    struct voice_s      *vprev;
    struct voice_s      *vnext;
    int nr;

    /* counter value */
    DWORD f;
    /* counter step / sample */
    DWORD fs;
#ifdef WAVETABLES
    /* do we have noise enabled? */
    BYTE noise;
#else
    /* waveform that we use */
    BYTE fm;
    /* pulse threshold compared to the 32-bit counter */
    DWORD pw;
#endif

    /* 31-bit adsr counter */
    DWORD adsr;
    /* adsr counter step / sample */
    SDWORD adsrs;
    /* adsr sustain level compared to the 31-bit counter */
    DWORD adsrz;

    /* does this voice use hard sync? */
    BYTE sync;
    /* does this voice use filter? */
    BYTE filter;
    /* does this structure need updating before next sample? */
    BYTE update;
    /* did we do multiple gate flips after last calculated sample? */
    BYTE gateflip;

    /* ADSR mode */
    BYTE adsrm;
    /* 4-bit attack value */
    BYTE attack;
    /* 4-bit decay value */
    BYTE decay;
    /* 4-bit sustain value */
    BYTE sustain;
    /* 4-bit release value */
    BYTE release;

    /* pointer to registers of this voice */
    BYTE *d;

    /* noise shift register. Note! rv may be 0 to 15 shifts 'behind' the
       real noise shift register value. Remaining shifts are done when
       it is referenced */
    DWORD rv;
#ifdef WAVETABLES
    /* pointer to wavetable data */
    WORD *wt;
    /* 32-bit offset to add to the counter before referencing the wavetable.
       This is used on combined waveforms, when other waveforms are combined
       with pulse */
    DWORD wtpf;
    /* length of wavetable (actually number of shifts needed for 32-bit
       counter) */
    DWORD wtl;
    /* kludge for ring modulation. Set wtr[1] = 0x7fff if ring modulation is
       used */
    WORD wtr[2];
#endif

    signed char filtIO;
    vreal_t filtLow, filtRef;
} voice_t;

/* needed data for SID */
struct sound_s {
    /* speed factor */
    int factor;

    /* number of voices */
    voice_t v[3];
    /* SID registers */
    BYTE d[32];
    /* is voice 3 enabled? */
    BYTE has3;
    /* 4-bit volume value */
    BYTE vol;

    /* ADSR counter step values for each adsr values */
    SDWORD adrs[16];
    /* sustain values compared to 31-bit ADSR counter */
    DWORD sz[16];

    /* internal constant used for sample rate dependent calculations */
    DWORD speed1;

    /* does this structure need updating before next sample? */
    BYTE update;
#ifdef WAVETABLES
    /* do we have a new sid or an old one? */
    BYTE newsid;
#endif
    /* constants needed to implement write-only register reads */
    BYTE laststore;
    BYTE laststorebit;
    CLOCK laststoreclk;
    /* do we want to use filters? */
    int emulatefilter;

    /* filter variables */
    vreal_t filterDy;
    vreal_t filterResDy;
    BYTE filterType;
    BYTE filterCurType;
    WORD filterValue;
};

/* XXX: check these */
/* table for internal ADSR counter step calculations */
static WORD adrtable[16] =
{
    1, 4, 8, 12, 19, 28, 34, 40, 50, 125, 250, 400, 500, 1500, 2500, 4000
};

/* XXX: check these */
/* table for pseudo-exponential ADSR calculations */
static DWORD exptable[6] =
{
    0x30000000, 0x1c000000, 0x0e000000, 0x08000000, 0x04000000, 0x00000000
};

/* clockcycles for each dropping bit when write-only register read is done */
static DWORD sidreadclocks[9];

static vreal_t lowPassParam[0x800];
static vreal_t bandPassParam[0x800];
static vreal_t filterResTable[16];
static const float filterRefFreq = 44100.0;
static signed char ampMod1x8[256];

/* manage temporary buffers. if the requested size is smaller or equal to the
 * size of the already allocated buffer, reuse it.  */
static SWORD *buf = NULL;
static int blen = 0;

static SWORD *getbuf(int len)
{
    if ((buf == NULL) || (blen < len)) {
        if (buf) {
            lib_free(buf);
        }
        blen = len;
        buf = lib_calloc(len, 1);
    }
    return buf;
}

inline static void dofilter(voice_t *pVoice)
{
    if (!pVoice->filter) {
        return;
    }

    if (pVoice->s->filterType) {
        if (pVoice->s->filterType == 0x20) {
            pVoice->filtLow += REAL_MULT(pVoice->filtRef, pVoice->s->filterDy);
            pVoice->filtRef +=
                REAL_MULT(REAL_VALUE(pVoice->filtIO) - pVoice->filtLow -
                          REAL_MULT(pVoice->filtRef, pVoice->s->filterResDy),
                          pVoice->s->filterDy);
            pVoice->filtIO = (signed char) (REAL_TO_INT(pVoice->filtRef - pVoice->filtLow / 4));
        } else if (pVoice->s->filterType == 0x40) {
            vreal_t sample;
            pVoice->filtLow += (vreal_t)(REAL_MULT(REAL_MULT(pVoice->filtRef,
                                              pVoice->s->filterDy), REAL_VALUE(0.1)));
            pVoice->filtRef += REAL_MULT(REAL_VALUE(pVoice->filtIO) - pVoice->filtLow -
                          REAL_MULT(pVoice->filtRef, pVoice->s->filterResDy),
                          pVoice->s->filterDy);
            sample = pVoice->filtRef - REAL_VALUE(pVoice->filtIO / 8);
            if (sample < REAL_VALUE(-128)) {
                sample = REAL_VALUE(-128);
            }
            if (sample > REAL_VALUE(127)) {
                sample = REAL_VALUE(127);
            }
            pVoice->filtIO = (signed char)(REAL_TO_INT(sample));
        } else {
            int tmp;
            vreal_t sample, sample2;
            pVoice->filtLow += REAL_MULT(pVoice->filtRef, pVoice->s->filterDy );
            sample = REAL_VALUE(pVoice->filtIO);
            sample2 = sample - pVoice->filtLow;
            tmp = (int)(REAL_TO_INT(sample2));
            sample2 -= REAL_MULT(pVoice->filtRef, pVoice->s->filterResDy);
            pVoice->filtRef += REAL_MULT(sample2, pVoice->s->filterDy);

            pVoice->filtIO = pVoice->s->filterType == 0x10
                             ? (signed char)
                             (REAL_TO_INT(pVoice->filtLow)) :
                             (pVoice->s->filterType == 0x30
                              ? (signed char)
                              (REAL_TO_INT(pVoice->filtLow)) :
                              (pVoice->s->filterType == 0x50
                                   ? (signed char)
                                   (REAL_TO_INT(sample) - (tmp >> 1)) :
                                   (pVoice->s->filterType == 0x60
                                   ? (signed char)
                                   tmp :
                                   (pVoice->s->filterType == 0x70
                                   ? (signed char)
                                   (REAL_TO_INT(sample) - (tmp >> 1)) : 0))));
        }
    } else { /* filterType == 0x00 */
        pVoice->filtIO = 0;
    }
}

/* 15-bit oscillator value */
#ifdef WAVETABLES
inline static DWORD doosc(voice_t *pv)
{
    if (pv->noise) {
        return ((DWORD)NVALUE(NSHIFT(pv->rv, pv->f >> 28))) << 7;
    }
    return pv->wt[(pv->f + pv->wtpf) >> pv->wtl] ^ pv->wtr[pv->vprev->f >> 31];
}
#else
static DWORD doosc(voice_t *pv)
{
    DWORD f = pv->f;

    switch (pv->fm) {
        case PULSESAWTOOTHWAVE:
            if (f <= pv->pw) {
                return 0x0000;
            }
        case SAWTOOTHWAVE:
            return f >> 17;
        case RINGWAVE:
            f ^= pv->vprev->f & 0x80000000;
        case TRIANGLEWAVE:
            if (f < 0x80000000) {
                return f >> 16;
            }
            return 0xffff - (f >> 16);
        case PULSETRIANGLEWAVE:
            if (f <= pv->pw) {
                return 0x0000;
            }
            if (f < 0x80000000) {
                return f >> 16;
            }
            return 0xffff - (f >> 16);
        case NOISEWAVE:
            return ((DWORD)NVALUE(NSHIFT(pv->rv, pv->f >> 28))) << 7;
        case PULSEWAVE:
            if (f >= pv->pw) {
                return 0x7fff;
            }
    }
    return 0x0000;
}
#endif

/* change ADSR state and all related variables */
static void set_adsr(voice_t *pv, BYTE fm)
{
    int i;

    switch (fm) {
        case ATTACK:
            pv->adsrs = pv->s->adrs[pv->attack];
            pv->adsrz = 0;
            break;
        case DECAY:
            /* XXX: fix this */
            if (pv->adsr <= pv->s->sz[pv->sustain]) {
                set_adsr(pv, SUSTAIN);
                return;
            }
            for (i = 0; pv->adsr < exptable[i]; i++) {}
            pv->adsrs = -pv->s->adrs[pv->decay] >> i;
            pv->adsrz = pv->s->sz[pv->sustain];
            if (exptable[i] > pv->adsrz) {
                pv->adsrz = exptable[i];
            }
            break;
        case SUSTAIN:
            if (pv->adsr > pv->s->sz[pv->sustain]) {
                set_adsr(pv, DECAY);
                return;
            }
            pv->adsrs = 0;
            pv->adsrz = 0;
            break;
        case RELEASE:
            if (!pv->adsr) {
                set_adsr(pv, IDLE);
                return;
            }
            for (i = 0; pv->adsr < exptable[i]; i++) {}
            pv->adsrs = -pv->s->adrs[pv->release] >> i;
            pv->adsrz = exptable[i];
            break;
        case IDLE:
            pv->adsrs = 0;
            pv->adsrz = 0;
            break;
    }
    pv->adsrm = fm;
}

/* ADSR counter triggered state change */
static void trigger_adsr(voice_t *pv)
{
    switch (pv->adsrm) {
        case ATTACK:
            pv->adsr = 0x7fffffff;
            set_adsr(pv, DECAY);
            break;
        case DECAY:
        case RELEASE:
            if (pv->adsr >= 0x80000000) {
                pv->adsr = 0;
            }
            set_adsr(pv, pv->adsrm);
            break;
    }
}

static void print_voice(char *buf, voice_t *pv)
{
    const char *m = "ADSRI";
#ifdef WAVETABLES
    const char *w = "0123456789abcdef";
#else
    const char *w = "TPSTN-R5";
#endif
    sprintf(buf,
            "#SID: V%d: e=%5.1f%%(%c) w=%6.1fHz(%c) f=%5.1f%% p=%5.1f%%\n",
            pv->nr,
            (double)pv->adsr * 100.0 / (((DWORD)1 << 31) - 1), m[pv->adsrm],
            (double)pv->fs / (pv->s->speed1 * 16),
#ifdef WAVETABLES
            w[pv->d[4] >> 4],
#else
            w[pv->fm],
#endif
            (double)pv->f * 100.0 / ((DWORD) -1),
#ifdef WAVETABLES
            (double)(pv->d[2] + (pv->d[3] & 0x0f) * 0x100) / 40.95
#else
            (double)pv->pw * 100.0 / ((DWORD) -1)
#endif
            );
}

static char *fastsid_dump_state(sound_t *psid)
{
    int i;
    char buf[1024];

    sprintf(buf, "#SID: clk=%ld v=%d s3=%d\n",
            (long)maincpu_clk, psid->vol, psid->has3);

    for (i = 0; i < 3; i++) {
        print_voice(buf + strlen(buf), &psid->v[i]);
    }

    return lib_stralloc(buf);
}

/* update SID structure */
inline static void setup_sid(sound_t *psid)
{
    if (!psid->update) {
        return;
    }

    psid->vol = psid->d[0x18] & 0x0f;
    psid->has3 = ((psid->d[0x18] & 0x80) && !(psid->d[0x17] & 0x04)) ? 0 : 1;

    if (psid->emulatefilter) {
        psid->v[0].filter = psid->d[0x17] & 0x01 ? 1 : 0;
        psid->v[1].filter = psid->d[0x17] & 0x02 ? 1 : 0;
        psid->v[2].filter = psid->d[0x17] & 0x04 ? 1 : 0;
        psid->filterType = psid->d[0x18] & 0x70;
        if (psid->filterType != psid->filterCurType) {
            psid->filterCurType = psid->filterType;
            psid->v[0].filtLow = 0;
            psid->v[0].filtRef = 0;
            psid->v[1].filtLow = 0;
            psid->v[1].filtRef = 0;
            psid->v[2].filtLow = 0;
            psid->v[2].filtRef = 0;
        }
        psid->filterValue = 0x7ff & ((psid->d[0x15] & 7) | ((WORD)psid->d[0x16]) << 3);
        if (psid->filterType == 0x20) {
            psid->filterDy = bandPassParam[psid->filterValue];
        } else {
            psid->filterDy = lowPassParam[psid->filterValue];
        }
        psid->filterResDy = filterResTable[psid->d[0x17] >> 4]
                            - psid->filterDy;
        if (psid->filterResDy < REAL_VALUE(1.0)) {
            psid->filterResDy = REAL_VALUE(1.0);
        }
    } else {
        psid->v[0].filter = 0;
        psid->v[1].filter = 0;
        psid->v[2].filter = 0;
    }
    psid->update = 0;
}

/* update voice structure */
inline static void setup_voice(voice_t *pv)
{
    if (!pv->update) {
        return;
    }

    pv->attack = pv->d[5] / 0x10;
    pv->decay = pv->d[5] & 0x0f;
    pv->sustain = pv->d[6] / 0x10;
    pv->release = pv->d[6] & 0x0f;
#ifndef WAVETABLES
    pv->pw = (pv->d[2] + (pv->d[3] & 0x0f) * 0x100) * 0x100100;
#endif
    pv->sync = pv->d[4] & 0x02 ? 1 : 0;
    pv->fs = pv->s->speed1 * (pv->d[0] + pv->d[1] * 0x100);
#ifdef WAVETABLES
    if (pv->d[4] & 0x08) {
        pv->f = pv->fs = 0;
        pv->rv = NSEED;
    }
    pv->noise = 0;
    pv->wtl = 20;
    pv->wtpf = 0;
    pv->wtr[1] = 0;

    switch ((pv->d[4] & 0xf0) >> 4) {
        case 0:
            pv->wt = wavetable00;
            pv->wtl = 31;
            break;
        case 1:
            pv->wt = wavetable10;
            if (pv->d[4] & 0x04) {
                pv->wtr[1] = 0x7fff;
            }
            break;
        case 2:
            pv->wt = wavetable20;
            break;
        case 3:
            pv->wt = wavetable30;
            if (pv->d[4] & 0x04) {
                pv->wtr[1] = 0x7fff;
            }
            break;
        case 4:
            if (pv->d[4] & 0x08) {
                pv->wt = &wavetable40[4096];
            } else {
                pv->wt = &wavetable40[4096 - (pv->d[2]
                                              + (pv->d[3] & 0x0f) * 0x100)];
            }
            break;
        case 5:
            pv->wt = &wavetable50[pv->wtpf = 4096 - (pv->d[2]
                                                     + (pv->d[3] & 0x0f) * 0x100)];
            pv->wtpf <<= 20;
            if (pv->d[4] & 0x04) {
                pv->wtr[1] = 0x7fff;
            }
            break;
        case 6:
            pv->wt = &wavetable60[pv->wtpf = 4096 - (pv->d[2]
                                                     + (pv->d[3] & 0x0f) * 0x100)];
            pv->wtpf <<= 20;
            break;
        case 7:
            pv->wt = &wavetable70[pv->wtpf = 4096 - (pv->d[2]
                                                     + (pv->d[3] & 0x0f) * 0x100)];
            pv->wtpf <<= 20;
            if (pv->d[4] & 0x04 && pv->s->newsid) {
                pv->wtr[1] = 0x7fff;
            }
            break;
        case 8:
            pv->noise = 1;
            pv->wt = NULL;
            pv->wtl = 0;
            break;
        default:
            /* XXX: noise locking correct? */
            pv->rv = 0;
            pv->wt = wavetable00;
            pv->wtl = 31;
    }
#else
    if (pv->d[4] & 0x08) {
        pv->fm = TESTWAVE;
        pv->pw = pv->f = pv->fs = 0;
        pv->rv = NSEED;
    } else {
        switch ((pv->d[4] & 0xf0) >> 4) {
            case 4:
                pv->fm = PULSEWAVE;
                break;
            case 2:
                pv->fm = SAWTOOTHWAVE;
                break;
            case 1:
                if (pv->d[4] & 0x04) {
                    pv->fm = RINGWAVE;
                } else {
                    pv->fm = TRIANGLEWAVE;
                }
                break;
            case 8:
                pv->fm = NOISEWAVE;
                break;
            case 0:
                pv->fm = NOWAVE;
                break;
            case 5:
                pv->fm = PULSETRIANGLEWAVE;
                break;
            case 6:
                pv->fm = PULSESAWTOOTHWAVE;
                break;
            default:
                pv->fm = NOWAVE;
        }
    }
#endif
    switch (pv->adsrm) {
        case ATTACK:
        case DECAY:
        case SUSTAIN:
            if (pv->d[4] & 0x01) {
                set_adsr(pv, (BYTE)(pv->gateflip ? ATTACK : pv->adsrm));
            } else {
                set_adsr(pv, RELEASE);
            }
            break;
        case RELEASE:
        case IDLE:
            if (pv->d[4] & 0x01) {
                set_adsr(pv, ATTACK);
            } else {
                set_adsr(pv, pv->adsrm);
            }
            break;
    }
    pv->update = 0;
    pv->gateflip = 0;
}

static SWORD fastsid_calculate_single_sample(sound_t *psid, int i)
{
    DWORD o0, o1, o2;
    int dosync1, dosync2;
    voice_t *v0, *v1, *v2;

    setup_sid(psid);
    v0 = &psid->v[0];
    setup_voice(v0);
    v1 = &psid->v[1];
    setup_voice(v1);
    v2 = &psid->v[2];
    setup_voice(v2);

    /* addfptrs, noise & hard sync test */
    dosync1 = 0;
    if ((v0->f += v0->fs) < v0->fs) {
        v0->rv = NSHIFT(v0->rv, 16);
        if (v1->sync) {
            dosync1 = 1;
        }
    }
    dosync2 = 0;
    if ((v1->f += v1->fs) < v1->fs) {
        v1->rv = NSHIFT(v1->rv, 16);
        if (v2->sync) {
            dosync2 = 1;
        }
    }
    if ((v2->f += v2->fs) < v2->fs) {
        v2->rv = NSHIFT(v2->rv, 16);
        if (v0->sync) {
            /* hard sync */
            v0->rv = NSHIFT(v0->rv, v0->f >> 28);
            v0->f = 0;
        }
    }

    /* hard sync */
    if (dosync2) {
        v2->rv = NSHIFT(v2->rv, v2->f >> 28);
        v2->f = 0;
    }
    if (dosync1) {
        v1->rv = NSHIFT(v1->rv, v1->f >> 28);
        v1->f = 0;
    }

    /* do adsr */
    if ((v0->adsr += v0->adsrs) + 0x80000000 < v0->adsrz + 0x80000000) {
        trigger_adsr(v0);
    }
    if ((v1->adsr += v1->adsrs) + 0x80000000 < v1->adsrz + 0x80000000) {
        trigger_adsr(v1);
    }
    if ((v2->adsr += v2->adsrs) + 0x80000000 < v2->adsrz + 0x80000000) {
        trigger_adsr(v2);
    }

    /* oscillators */
    o0 = v0->adsr >> 16;
    o1 = v1->adsr >> 16;
    o2 = v2->adsr >> 16;
    if (o0) {
        o0 *= doosc(v0);
    }
    if (o1) {
        o1 *= doosc(v1);
    }
    if (psid->has3 && o2) {
        o2 *= doosc(v2);
    } else {
        o2 = 0;
    }
    /* sample */
    if (psid->emulatefilter) {
        v0->filtIO = ampMod1x8[(o0 >> 22)];
        dofilter(v0);
        o0 = ((DWORD)(v0->filtIO) + 0x80) << (7 + 15);
        v1->filtIO = ampMod1x8[(o1 >> 22)];
        dofilter(v1);
        o1 = ((DWORD)(v1->filtIO) + 0x80) << (7 + 15);
        v2->filtIO = ampMod1x8[(o2 >> 22)];
        dofilter(v2);
        o2 = ((DWORD)(v2->filtIO) + 0x80) << (7 + 15);
    }

    return (SWORD)(((SDWORD)((o0 + o1 + o2) >> 20) - 0x600) * psid->vol);
}

static int fastsid_calculate_samples(sound_t *psid, SWORD *pbuf, int nr,
                                     int interleave, int *delta_t)
{
    int i;
    SWORD *tmp_buf;

    if (psid->factor == 1000) {
        for (i = 0; i < nr; i++) {
            pbuf[i * interleave] = fastsid_calculate_single_sample(psid, i);
        }
        return nr;
    }
    tmp_buf = getbuf(2 * nr * psid->factor / 1000);
    for (i = 0; i < (nr * psid->factor / 1000); i++) {
        tmp_buf[i * interleave] = fastsid_calculate_single_sample(psid, i);
    }
    memcpy(pbuf, tmp_buf, 2 * nr);
    return nr;
}

static void init_filter(sound_t *psid, int freq)
{
    WORD uk;
    vreal_t rk;
    long int si;

    float yMax = 1.0;
    float yMin = (float)0.01;
    float resDyMax = 1.0;
    float resDyMin = 2.0;
    float resDy = resDyMin;

    float yAdd, yTmp;

    float filterFs = 400.0;
    float filterFm = 60.0;
    float filterFt = (float)0.05;

    float filterAmpl = 1.0;

    psid->filterValue = 0;
    psid->filterType = 0;
    psid->filterCurType = 0;
    psid->filterDy = 0;
    psid->filterResDy = 0;

    for (uk = 0, rk = 0; rk < 0x800; rk++, uk++) {
        float h;

        h = (float)((((exp(rk / 2048 * log(filterFs)) / filterFm) + filterFt) * filterRefFreq) / freq);
        if (h < yMin) {
            h = yMin;
        }
        if (h > yMax) {
            h = yMax;
        }
        lowPassParam[uk] = REAL_VALUE(h);
    }

    yMax = (float)0.22;
    yMin = (float)0.002;
    yAdd = (float)((yMax - yMin) / 2048.0);
    yTmp = yMin;

    for (uk = 0, rk = 0; rk < 0x800; rk++, uk++) {
        bandPassParam[uk] = REAL_VALUE((yTmp * filterRefFreq) / freq);
        yTmp += yAdd;
    }

    for (uk = 0; uk < 16; uk++) {
        filterResTable[uk] = REAL_VALUE(resDy);
        resDy -= ((resDyMin - resDyMax ) / 15);
    }

    filterResTable[0] = REAL_VALUE(resDyMin);
    filterResTable[15] = REAL_VALUE(resDyMax);

    /* XXX: if psid->emulatefilter = 0, ampMod1x8 is never referenced */
    if (psid->emulatefilter) {
        filterAmpl = (float)0.7;
    } else {
        filterAmpl = (float)1.0;
    }

    for (uk = 0, si = 0; si < 256; si++, uk++) {
        ampMod1x8[uk] = (signed char)((si - 0x80) * filterAmpl);
    }
}

/* SID initialization routine */
static sound_t *fastsid_open(BYTE *sidstate)
{
    sound_t *psid;

    psid = lib_calloc(1, sizeof(sound_t));

    memcpy(psid->d, sidstate, 32);

    return psid;
}

static int fastsid_init(sound_t *psid, int speed, int cycles_per_sec, int factor)
{
    DWORD i;
    int sid_model;

    psid->factor = factor;

    psid->speed1 = (cycles_per_sec << 8) / speed;
    for (i = 0; i < 16; i++) {
        psid->adrs[i] = 500 * 8 * psid->speed1 / adrtable[i];
        psid->sz[i] = 0x8888888 * i;
    }
    psid->update = 1;

    if (resources_get_int("SidFilters", &(psid->emulatefilter)) < 0) {
        return 0;
    }

    init_filter(psid, speed);
    setup_sid(psid);
    for (i = 0; i < 3; i++) {
        psid->v[i].vprev = &psid->v[(i + 2) % 3];
        psid->v[i].vnext = &psid->v[(i + 1) % 3];
        psid->v[i].nr = i;
        psid->v[i].d = psid->d + i * 7;
        psid->v[i].s = psid;
        psid->v[i].rv = NSEED;
        psid->v[i].filtLow = 0;
        psid->v[i].filtRef = 0;
        psid->v[i].filtIO = 0;
        psid->v[i].update = 1;
        setup_voice(&psid->v[i]);
    }
#ifdef WAVETABLES
    if (resources_get_int("SidModel", &sid_model) < 0) {
        return 0;
    }

    psid->newsid = 0;
    switch (sid_model) {
        default:
        case 0: /* 6581 */
        case 3: /* 6581R4 */
        case 4: /* DTVSID */
            psid->newsid = 0;
            break;
        case 1: /* 8580 */
        case 2: /* 8580 + digi boost */
            psid->newsid = 1;
            break;
    }

    for (i = 0; i < 4096; i++) {
        wavetable10[i] = (WORD)(i < 2048 ? i << 4 : 0xffff - (i << 4));
        wavetable20[i] = (WORD)(i << 3);
        wavetable30[i] = waveform30_8580[i] << 7;
        wavetable40[i + 4096] = 0x7fff;
        if (psid->newsid) {
            wavetable50[i + 4096] = waveform50_8580[i] << 7;
            wavetable60[i + 4096] = waveform60_8580[i] << 7;
            wavetable70[i + 4096] = waveform70_8580[i] << 7;
        } else {
            wavetable50[i + 4096] = waveform50_6581[i >> 3] << 7;
            wavetable60[i + 4096] = 0;
            wavetable70[i + 4096] = 0;
        }
    }
#endif
    for (i = 0; i < NOISETABLESIZE; i++) {
        noiseLSB[i] = (BYTE)((((i >> (7 - 2)) & 0x04) | ((i >> (4 - 1)) & 0x02)
                              | ((i >> (2 - 0)) & 0x01)));
        noiseMID[i] = (BYTE)((((i >> (13 - 8 - 4)) & 0x10)
                              | ((i << (3 - (11 - 8))) & 0x08)));
        noiseMSB[i] = (BYTE)((((i << (7 - (22 - 16))) & 0x80)
                              | ((i << (6 - (20 - 16))) & 0x40)
                              | ((i << (5 - (16 - 16))) & 0x20)));
    }
    for (i = 0; i < 9; i++) {
        sidreadclocks[i] = 13;
    }

    return 1;
}

static void fastsid_close(sound_t *psid)
{
    lib_free(psid);

    if (buf) {
        lib_free(buf);
        buf = NULL;
    }
}


static BYTE fastsid_read(sound_t *psid, WORD addr)
{
    BYTE ret;
    WORD ffix;
    register DWORD rvstore;
    register CLOCK tmp;

    switch (addr) {
        case 0x19:
            /* pot/x */
            ret = 0xff;
            break;
        case 0x1a:
            /* pot/y */
            ret = 0xff;
            break;
        case 0x1b:
            /* osc3 / random */
            ffix = (WORD)(sound_sample_position() * psid->v[2].fs);
            rvstore = psid->v[2].rv;
            if (
#ifdef WAVETABLES
                psid->v[2].noise
#else
                psid->v[2].fm == NOISEWAVE
#endif
                && psid->v[2].f + ffix < psid->v[2].f) {
                psid->v[2].rv = NSHIFT(psid->v[2].rv, 16);
            }
            psid->v[2].f += ffix;
            ret = (BYTE)(doosc(&psid->v[2]) >> 7);
            psid->v[2].f -= ffix;
            psid->v[2].rv = rvstore;
            break;
        case 0x1c:
            ret = (BYTE)(psid->v[2].adsr >> 23);
            break;
        default:
            while ((tmp = psid->laststorebit) &&
                   (tmp = psid->laststoreclk + sidreadclocks[tmp]) < maincpu_clk) {
                psid->laststoreclk = tmp;
                psid->laststore &= 0xfeff >> psid->laststorebit--;
            }
            ret = psid->laststore;
    }

    return ret;
}

static void fastsid_store(sound_t *psid, WORD addr, BYTE byte)
{
    switch (addr) {
        case 4:
            if ((psid->d[addr] ^ byte) & 1) {
                psid->v[0].gateflip = 1;
            }
        case 0:
        case 1:
        case 2:
        case 3:
        case 5:
        case 6:
            psid->v[0].update = 1;
            break;
        case 11:
            if ((psid->d[addr] ^ byte) & 1) {
                psid->v[1].gateflip = 1;
            }
        case 7:
        case 8:
        case 9:
        case 10:
        case 12:
        case 13:
            psid->v[1].update = 1;
            break;
        case 18:
            if ((psid->d[addr] ^ byte) & 1) {
                psid->v[2].gateflip = 1;
            }
        case 14:
        case 15:
        case 16:
        case 17:
        case 19:
        case 20:
            psid->v[2].update = 1;
            break;
        default:
            psid->update = 1;
    }

    psid->d[addr] = byte;
    psid->laststore = byte;
    psid->laststorebit = 8;
    psid->laststoreclk = maincpu_clk;
}

static void fastsid_reset(sound_t *psid, CLOCK cpu_clk)
{
    WORD addr;

    for (addr = 0; addr < 32; addr++) {
        fastsid_store(psid, addr, 0);
    }

    psid->laststoreclk = cpu_clk;
}

static void fastsid_prevent_clk_overflow(sound_t *psid, CLOCK sub)
{
    psid->laststoreclk -= sub;
}

static void fastsid_state_read(sound_t *psid, sid_snapshot_state_t *sid_state)
{
}

static void fastsid_state_write(sound_t *psid, sid_snapshot_state_t *sid_state)
{
}

sid_engine_t fastsid_hooks =
{
    fastsid_open,
    fastsid_init,
    fastsid_close,
    fastsid_read,
    fastsid_store,
    fastsid_reset,
    fastsid_calculate_samples,
    fastsid_prevent_clk_overflow,
    fastsid_dump_state,
    fastsid_state_read,
    fastsid_state_write
};
