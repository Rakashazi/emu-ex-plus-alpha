/*
 * t6721.c - Toshiba 6721a Emulation
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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
#include <math.h>
#include <string.h>

#include "lib.h"
#include "maincpu.h"
#include "monitor.h"
#include "snapshot.h"
#include "t6721.h"

/*

    toshiba 6721a (voice output)

    based on code from alankila (parcor.py) and nojoopa (mvtool)

    - 8 kHz 9bit output (output is rendered in 16 bit)
    - generates output for 20ms (or 10ms) out of 6 byte voice data!
    - uses PARCOR voice synthesizing and analyzing method (Nippon Telegraph and Telephon Public Corporation)

*/

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* #define T6721DEBUG */

#define WRITEWAVFILE 0 /* write "test.wav" containing all generated output */

#ifdef T6721DEBUG
#define DBG(x) DBG_STATUS(); printf x;
#define DBGADD(x) printf x
#define DBG_STATUS() \
    printf("t6721: @:%04x apd: %d busy: %d eos: %d playing: %d ", reg_pc, t6721->apd, t6721->busy, t6721->eos, t6721->playing);
#else
#define DBG(x)
#define DBGADD(x)
#define DBG_STATUS()
#endif

#define PARCOR_OUTPUT_HZ       (8000)

#define PARCOR_BUFFER_LEN (0x400)
SWORD ringbuffer[PARCOR_BUFFER_LEN]; /* FIXME */
int ringbuffer_rptr = 0;
int ringbuffer_wptr = 0;
int phrase_sample_len = 0;

#define RBSTATE_STOP 0
#define RBSTATE_PLAY 1
#define RBSTATE_DELAY_SAMPLES (PARCOR_OUTPUT_HZ / 200) /* 5 ms */
static int ringbuffer_state = 0;

float upsmpcnt = 0;
float upsmp = 0;

struct param_s {
    BYTE energy;
    BYTE pitch;
    SWORD k[10];
};
typedef struct param_s param_t;

static param_t p_from = { 0, 0x21 };
static param_t p_to = { 0, 0x21 };
static double p_z[11] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

/* FIXME: MOVE  parcor parameters: energy + pitch + k1..10 */
static WORD parcor_param[1 + 1 + 10];

/*
    words that use 48bits/frame:

    magicvoice rom:

     68 "ing"
     78 "what"
     80 "who"
     105 "is"
     129 "as"
     130 "but"
     148 "thuh"
     151 "with"
     158 "bad"
     164 "because"
     193 "orange"
     209 "may"
     225 "program"

     v364 and WoW do NOT use 48bit frames !
*/

/* parameter lengths (bits) */
/* [1] k0..k9 from speak and spell
 * (48bit frames) 5, 5, 4, 4,  4, 4, 4, 3, 3, 3 (18bits + 21bits = 39bits, 9 free)
 * [2] k0..k9 from "Handbook of Data Compression By David Salomon, Giovanni Motta, David (CON) Bryant
 * (48bit frames) 3, 4, 4, 4,  4, 3, 3, 3, 3, 3 (15bits + 19bits = 34bits, 14 free)
 */
static const int parcor_param_len[2][1 + 1 + 10] = {
    /* FIXME: 48 bits/frame  (this is vaguely guessed and totally wrong)
        12 ? bits for energy+pitch
        21 ? bits for k1..4
        15 ? bits for k5..10
     */
    {6, 6,   6,  5,  5,  5,   3, 3, 3, 2, 2, 2},
#if 0
    { 4, 5,   5,  5,  4, 4,   4, 4, 4, 3, 3, 3 }, /* [1] vaguely recognizeable */
    { 7, 7,   3,  4,  4, 4,   4, 3, 3, 3, 3, 3 }, /* [2] not working */
#endif
    /* 96 bits/frame (these seem correct)
        14 bits for energy+pitch
        38 bits for k1..4
        44 bits for k5..10
     */
    { 7, 7,  10, 10, 10, 8,   8, 8, 7, 7, 7, 7 }
};
static WORD chip_buf = 0;
static int chip_bit_i = 0;
static int param_i = 0;

#define PARMVALUE(ctx, x) (parcor_param[x] >> (16 - parcor_param_len[(ctx)->cond2_framebits][x]))

/* ------------------------------------------------------------------------- */

static void set_dtrd(t6721_state *t6721, int dtrd);
static void set_apd(t6721_state *t6721, int apd);
static void set_eos(t6721_state *t6721, int eos);
static void set_playing(t6721_state *t6721, int playing);

/*****************************************************************************
    WAV Writer (for debugging)
*****************************************************************************/

#if WRITEWAVFILE
static FILE *wav_fp = NULL;
static size_t wavsize = 0;

static const BYTE wav_header[] = {
    'R', 'I', 'F', 'F',
    0, 0, 0, 0, /* filesize - 8 */
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',
    16, 0, 0, 0,
    1, 0,
    1, 0,
    0x40, 0x1f, 0, 0, /* 8000 Hz */
    0x80, 0x3e, 0, 0, /* 8000 Hz * 2B/sample */
    2, 0,
    16, 0,
    'd', 'a', 't', 'a',
    0, 0, 0, 0  /* bytes to follow (== wavsize)*/
};

static int write_header(void)
{
    if (fwrite(wav_header, 1, sizeof(wav_header), wav_fp) < sizeof(wav_header)) {
        fclose(wav_fp);
        wav_fp = NULL;
        return 1;
    }
    return 0;
}

static int fix_header(void)
{
    size_t temp;
    BYTE b[4] = { 0, 0, 0, 0 };

    fseek(wav_fp, 4, SEEK_SET);

    temp = wavsize + sizeof(wav_header) - 8;

    b[0] = (temp >> 0) & 0xff;
    b[1] = (temp >> 8) & 0xff;
    b[2] = (temp >> 16) & 0xff;
    b[3] = (temp >> 24) & 0xff;

    if (fwrite(b, 1, 4, wav_fp) < 4) {
        fclose(wav_fp);
        wav_fp = NULL;
        return 1;
    }

    fseek(wav_fp, 40, SEEK_SET);

    temp = wavsize;

    b[0] = (temp >> 0) & 0xff;
    b[1] = (temp >> 8) & 0xff;
    b[2] = (temp >> 16) & 0xff;
    b[3] = (temp >> 24) & 0xff;

    if (fwrite(b, 1, 4, wav_fp) < 4) {
        fclose(wav_fp);
        wav_fp = NULL;
        return 1;
    }

    return 0;
}

static void wav_close_file(void)
{
    if (wav_fp) {
        fix_header();
        fclose(wav_fp);
        wav_fp = NULL;
    }
}

static int wav_create_file(const char *filename)
{
    if ((wav_fp = fopen(filename, "wb")) == NULL) {
        perror(filename);
        return 1;
    }

    atexit(wav_close_file);

    return write_header();
}

static int wav_write_sample(SWORD data)
{
    BYTE d;

    if (wav_fp == NULL) {
        wav_create_file("test.wav");
    }

    d = (BYTE)(data & 0xff);

    if (fwrite(&d, 1, 1, wav_fp) < 1) {
        return 1;
    }

    wavsize++;

    d = (BYTE)((data >> 8) & 0xff);

    if (fwrite(&d, 1, 1, wav_fp) < 1) {
        return 1;
    }

    wavsize++;

    return 0;
}
#endif

/*****************************************************************************
    Output Ringbuffer
*****************************************************************************/

static void ringbuffer_reset(void)
{
    ringbuffer_rptr = 0;
    ringbuffer_wptr = 0;
    ringbuffer_state = RBSTATE_STOP;
    phrase_sample_len = 0;
}

/*
    write one 8000khz sample to the output ringbuffer
    returns 1 on error, 0 on success
*/
static int parcor_output_sample(t6721_state *t6721, SWORD value)
{
    int next_wptr;

    ringbuffer[ringbuffer_wptr] = value;
#if WRITEWAVFILE
    wav_write_sample(value);
#endif
    next_wptr = ringbuffer_wptr + 1;
    if (next_wptr == PARCOR_BUFFER_LEN) {
        next_wptr = 0;
    }
    if (next_wptr != ringbuffer_rptr) {
        ringbuffer_wptr = next_wptr;
        phrase_sample_len++;
        return 0;
    }
    /* DBG(("ringbuffer overflow\n")); */
    return 1;
}

/*
    render one output sample
*/
SWORD output_update_sample(t6721_state *t6721)
{
    static float from, to;
    int next_rptr;
    SWORD this;

    this = (SWORD) ((from * (1.0f - upsmpcnt)) + (to * upsmpcnt));

    upsmpcnt += (1.0f / upsmp);
    if (upsmpcnt >= 1.0f) {
        upsmpcnt -= 1.0f;
        from = to;
        if (ringbuffer_state == RBSTATE_STOP) {
            if (phrase_sample_len > RBSTATE_DELAY_SAMPLES) {
                ringbuffer_state = RBSTATE_PLAY;
            }
        } else {
            if (phrase_sample_len > 0) {
                next_rptr = ringbuffer_rptr + 1;
                if (next_rptr == PARCOR_BUFFER_LEN) {
                    next_rptr = 0;
                }
                if (next_rptr != ringbuffer_wptr) {
                    ringbuffer_rptr = next_rptr;
                    phrase_sample_len--;
                }
            } else {
                ringbuffer_state = RBSTATE_STOP;
            }
        }
        to = (float) ringbuffer[ringbuffer_rptr];
    }

    return this;
}

static int framestretch[0x10] =
{
    100, /* 0 : 1,0  */
     70, /* 1 : 0,7  (1.4% faster) */
     80, /* 2 : 0,8  */
     90, /* 3 : 0,9  */
    100, /* 4 : 1,0  (default) */
    110, /* 5 : 1,1  */
    120, /* 6 : 1,2  */
    130, /* 7 : 1,3  */
    140, /* 8 : 1,4  */
    150, /* 9 : 1,5  */
    155, /* a : 1,55 (0.65% slower) */
    100, /* b : 1,0  */
    100, /* c : 1,0  */
    100, /* d : 1,0  */
    100, /* e : 1,0  */
    100  /* f : 1,0  */
};

static int get_frame_samples(t6721_state *t6721)
{
    return ((t6721->cycles_per_sec * t6721->cond2_framelen * framestretch[t6721->speed]) / (100 * 100));
}

static int get_subframe_samples(t6721_state *t6721)
{
    return ((PARCOR_OUTPUT_HZ * t6721->cond2_framelen * framestretch[t6721->speed]) / (100 * 100 * 8));
}

/*****************************************************************************
    PARCOR Synthesis
*****************************************************************************/

static double filter(int i, double ef, double k)
{
    double ef_prev = ef + k * p_z[1 + i - 1];
    p_z[1 + i] = p_z[1 + i - 1] - k * ef_prev;
    return ef_prev;
}

/* -------------------------------------------------------------------------
    render one PARCOR subframe
    returns 1 on error, 0 on success
   ------------------------------------------------------------------------- */
static int render_subframe(t6721_state *t6721, int sub_i, int voiced)
{
    int i, j;
    static double phase = 0.0;

    double energy = ((p_from.energy * (8 - sub_i)) + (p_to.energy * sub_i)) / (8.0 * 127.0);
    BYTE pitch;
    double phase_inc;

    double k[10];

    double sample, data;

    SWORD output;

    if (voiced) {
        pitch = ((p_from.pitch * (8 - sub_i)) + (p_to.pitch * sub_i)) / 8;
        phase_inc = 1.0 / (double)(pitch);
    } else {
        pitch = 0;
        phase_inc = 0.0;
    }

/* DBG(("voiced %i, energy %f, pitch %02x, phase_inc %f, k: ", voiced, energy, pitch, phase_inc)); */

    for (i = 0; i < 10; ++i) {
        k[i] = ((p_from.k[i] * (8 - sub_i)) + (p_to.k[i] * sub_i)) / (8.0 * 32768.0);
/* DBGADD(("%f, ", k[i])); */
    }
/* DBGADD(("\n")); */

    for (i = 0; i < get_subframe_samples(t6721); ++i) {
        /* sample */
        if (voiced) {
            phase += phase_inc;
            if (phase >= 1.0) {
                phase -= 1.0;
            }
            sample = cos(M_PI * phase * phase);
        } else {
            /* FIXME: implement the actual pseudo random number generator used
                      on the chip */
            sample = lib_float_rand(-1.0, 1.0);
        }
        sample *= energy;

        /* filter */
        data = sample;
        for (j = t6721->cond2_stages - 1; j >= 0; --j) {
            data = filter(j, data, k[j]);
        }

        /* scale to 16bit */
        output = (SWORD)(data * (8192.0f + 2048.0f));

        if (parcor_output_sample(t6721, output)) {
            return 1;
        }
    }

    return 0;
}

static int render_silence(t6721_state *t6721)
{
    int i;

    for (i = 0; i < (get_subframe_samples(t6721) * 8); ++i) {
        if (parcor_output_sample(t6721, 0)) {
            return 1;
        }
    }

    return 0;
}

/* -------------------------------------------------------------------------
    render one PARCOR frame
    returns 1 on error, 0 on success
   ------------------------------------------------------------------------- */
static int parcor_render_frame(t6721_state *t6721, WORD *new_param)
{
    int i, voiced, silent;
    BYTE new_pitch;

    new_pitch = (BYTE)PARMVALUE(t6721, 1);
    voiced = (new_pitch > 0);

    memcpy(&p_from, &p_to, sizeof(p_from));

    p_to.energy = (BYTE)PARMVALUE(t6721, 0);
    silent = ((new_pitch == 0x7e) && (p_to.energy == 1));

    if (!voiced && !silent) {
        new_pitch = p_from.pitch;
    }

    p_to.pitch = new_pitch;

    if (!silent) {
        for (i = 0; i < (voiced ? (12 - 2) : (6 - 2)); ++i) {
            p_to.k[i] = (SWORD)(new_param[i + 2]);
        }

        for (i = 0; i < 8; ++i) {
            if (render_subframe(t6721, i, voiced)) {
                return 1;
            }
        }
    } else {
        if (render_silence(t6721)) {
            return 1;
        }
    }

    return 0;
}

/*****************************************************************************
    handle data input from the DI line
*****************************************************************************/

static int parcor_framelen = 12;
static int parcor_frametype;
static int phrase_samples = 0;

static int zero_frames = 0;

/* read nibbles from FIFO and re-arrange into a parcor frame */
static void reset_di_fifo(void)
{
    chip_buf = 0;
    chip_bit_i = 0;
    param_i = 0;
}

/*
    read data (one bit) from the DI line

    returns:
    -1 no data available
     0 normal exit
     1 got PARCOR frame
*/
static int read_di_fifo(t6721_state *t6721)
{
    unsigned int bit;
/*
    if (t6721->dtrd) {
        set_dtrd(t6721, 0);
        return -1;
    }

    set_dtrd(t6721, 1);
*/
    if (t6721->read_data(t6721, &bit) < 1) {
        /* DBGADD(("<*>")); */
        return -1;
    }

    if ((param_i == 1) && (chip_bit_i == 0)) {
        if (PARMVALUE(t6721, 0) == 0) {
            if (bit) {
                reset_di_fifo();
                set_eos(t6721, 1);
                t6721->busy = 0;
                set_playing(t6721, 0);
                parcor_framelen = 1;
                parcor_frametype = T6721_FRAMETYPE_EOS;
                zero_frames = 0;
                return 1;
            } else {
                reset_di_fifo();
                zero_frames++;
                parcor_framelen = 1;
                parcor_frametype = T6721_FRAMETYPE_ZERO;

                if (zero_frames > 1) {
                    set_eos(t6721, 1);
                    t6721->busy = 0;
                    set_playing(t6721, 0);
                    zero_frames = 0;
                }
                return 1;
            }
        }
        zero_frames = 0;
    }

    chip_buf >>= 1;
    chip_buf |= bit ? 0x8000 : 0;
    chip_bit_i++;

/* DBGADD(("(%x)", bit)); */

    if (chip_bit_i < parcor_param_len[t6721->cond2_framebits][param_i]) {
        return 0;
    }

/* DBGADD(("<%03x>", chip_buf)); */

    /* next parm is ready */
    parcor_param[param_i] = chip_buf;
    chip_bit_i = 0;
    chip_buf = 0;

    switch (param_i) {
        case 0:
            parcor_framelen = 12;
            break;
        case 1:
            if (PARMVALUE(t6721, 1) == 0) {
                parcor_framelen = 6;
                parcor_frametype = T6721_FRAMETYPE_UNVOICED;
            } else {
                if ((PARMVALUE(t6721, 0) == 1) && (PARMVALUE(t6721, 1) == 0x7e)) {
                    parcor_frametype = T6721_FRAMETYPE_SILENT;
                } else {
                    parcor_frametype = T6721_FRAMETYPE_VOICED;
                }
            }
            break;
    }

    param_i++;

    if (param_i == parcor_framelen) {
        /* complete frame was read */
        reset_di_fifo();
        return 1;
    }

    return 0;
}

/*****************************************************************************
*****************************************************************************/

static void set_playing(t6721_state *t6721, int playing)
{
    if (t6721->playing != playing) {
        if (playing) {
            DBG(("start playing\n"));
            t6721->playing_delay = 0x10000; /* FIXME: find out real value */
        } else {
            DBG(("stop playing\n"));
            ringbuffer_reset();
            reset_di_fifo(); /* ? */
        }
    }
    t6721->playing = playing;
}

static void set_dtrd(t6721_state *t6721, int dtrd)
{
    t6721->dtrd = dtrd;
    if (t6721->set_dtrd) {
        t6721->set_dtrd(t6721);
    }
}

static void set_apd(t6721_state *t6721, int apd)
{
    t6721->apd = apd;
    if (t6721->set_apd) {
        t6721->set_apd(t6721);
    }
}

static void set_eos(t6721_state *t6721, int eos)
{
    if (t6721->eos != eos) {
        t6721->eos = eos;
        if (eos) {
            t6721->status |= T6721_STATUS_END_OF_SPEECH;
        } else {
            t6721->status &= ~T6721_STATUS_END_OF_SPEECH;
        }
        if (t6721->set_eos) {
            t6721->set_eos(t6721);
        }
    }
    if (eos) {
        /* FIXME: confirm: is this correct ? */
        /* 10ms or 20ms depending on current framelen */
        t6721->eos_samples = get_frame_samples(t6721);
    }
}

/*****************************************************************************
    "run" chip and update output_samples

    FIXME: this perhaps needs some other (per cycle) hook (?)
*****************************************************************************/

/* run chip for exactly one CPU/System Cycle */
void t6721_update_tick(t6721_state *t6721)
{
    int res;
#ifdef T6721DEBUG
    int i;
#endif

    /* once asserted, the EOS signal is generated for 20ms */
    if (t6721->eos_samples == 0) {
        set_eos(t6721, 0);
    } else {
        t6721->eos_samples--;
    }

    if (t6721->playing_delay) {
        t6721->playing_delay--;
        return;
    }

    if (phrase_samples) {
        phrase_samples--;
        return;
    }

    if ((t6721->playing == 1) && (t6721->apd == 0) && (t6721->eos == 0)) {
        set_dtrd(t6721, 1);
        res = read_di_fifo(t6721);
        if (res == -1) {
            /* not enough data in FIFO */
        } else if (res == 1) {
            /* got PARCOR frame */
            if ((parcor_frametype == T6721_FRAMETYPE_SILENT) ||
                (parcor_frametype == T6721_FRAMETYPE_VOICED) ||
                (parcor_frametype == T6721_FRAMETYPE_UNVOICED)) {
                parcor_render_frame(t6721, parcor_param);
            }
            /* FIXME: confirm: is this correct ? */
            /* 10ms or 20ms depending on current framelen */
            phrase_samples = get_frame_samples(t6721) - (((t6721->cond2_framebits ? 96 : 48) * 10 * framestretch[t6721->speed]) / 100);
            set_dtrd(t6721, 0);
#ifdef T6721DEBUG
            DBG(("got Frame: "));
            switch (parcor_frametype) {
                case T6721_FRAMETYPE_VOICED:
                    DBGADD(("[voiced]   "));
                    break;
                case T6721_FRAMETYPE_UNVOICED:
                    DBGADD(("[unvoiced] "));
                    break;
                case T6721_FRAMETYPE_EOS:
                    DBGADD(("[eos]      "));
                    break;
                case T6721_FRAMETYPE_ZERO:
                    DBGADD(("[zero]     "));
                    break;
                case T6721_FRAMETYPE_SILENT:
                    DBGADD(("[silent]   "));
                    break;
            }
            for (i = 0; i < parcor_framelen; i++) {
                DBGADD(("%03x ", parcor_param[i] >> (16 - parcor_param_len[t6721->cond2_framebits][i])));
            }
            DBGADD(("\n"));
#endif
        }
    }
}

/* run chip for N CPU/System Cycles */
void t6721_update_ticks(t6721_state *t6721, int ticks)
{
    while (ticks) {
        t6721_update_tick(t6721);
        t6721->cycles_done++;
        ticks--;
    }
}

float up2smp = 0;

/* render num samples into output buffer, run remaining cycles (if any) */
void t6721_update_output(t6721_state *t6721, SWORD *buf, int num)
{
    int i;
    int cycles;

    cycles = (int)((num * up2smp) - t6721->cycles_done);
    if (cycles > 0) {
        /* run chip for remaining cycles */
        t6721_update_ticks(t6721, cycles);
        t6721->cycles_done = 0;
    } else {
        /* carry over remaining cycles to next sound frame */
        t6721->cycles_done = 0 - cycles;
    }

    /* render output samples */
    for (i = 0; i < num; i++) {
        *buf++ = output_update_sample(t6721);
    }
}

/*****************************************************************************
    Chip Command Handling
*****************************************************************************/

/*
    when the WR pin goes high, d0..d3 from data are read by ("written to") the chip
*/
void t6721_store(t6721_state *t6721, BYTE data)
{
/* DBG(("write %2x\n", data)); */
    /* an actual store is performed on Lo->HI transition of WR */
    if ((t6721->wr == 1) && (t6721->wr_last == 0)) {
        if (t6721->cmd_nibbles) {
            switch (t6721->cmd_current) {
                case 0x03: /* ADDRESS LOAD 1/6 */
                    DBG(("arg %2x (ADDRESS LOAD %d/6)\n", data, 6 - t6721->cmd_nibbles));
                    break;
                case 0x05: /* SPEED LOAD 1/2 */
                    DBG(("arg %2x (SPEED LOAD %d/2)\n", data, 3 - t6721->cmd_nibbles));
                    t6721->speed = data;
                    break;
                case 0x06: /* CONDITION 1 1/2 */
                    DBG(("arg %2x (CONDITION 1 %d/2) ", data, 3 - t6721->cmd_nibbles));
                    t6721->condition1 = data;
                    if (t6721->condition1 & T6721_COND1_LOSS) {
                        t6721->cond1_loss = T6721_LOSS_ENABLED;
                    } else {
                        t6721->cond1_loss = T6721_LOSS_DISABLED;
                    }
                    if (t6721->condition1 & T6721_COND1_SHAPE) {
                        t6721->cond1_shape = T6721_SHAPE_TRIANGLE;
                    } else {
                        t6721->cond1_shape = T6721_SHAPE_PITCH;
                    }
                    DBGADD(("loss effect calculation: %d Sound Source Shape: %s\n", t6721->cond1_loss, t6721->cond1_shape ? "Triangle" : "Pitch" ));
                    break;
                case 0x07: /* CONDITION 2 1/2 */
                    DBG(("arg %2x (CONDITION 2 %d/2) ", data, 3 - t6721->cmd_nibbles));
                    t6721->condition2 = data;
                    if (t6721->condition2 & T6721_COND2_STAGES) {
                        t6721->cond2_stages = T6721_STAGES_8;
                    } else {
                        t6721->cond2_stages = T6721_STAGES_10;
                    }
                    if (t6721->condition2 & T6721_COND2_REPEAT) {
                        t6721->cond2_repeat = T6721_REPEAT_DISABLED;
                    } else {
                        t6721->cond2_repeat = T6721_REPEAT_ENABLED;
                    }
                    if (t6721->condition2 & T6721_COND2_LENGTH) {
                        t6721->cond2_framelen = T6721_FRAME_10MS;
                    } else {
                        t6721->cond2_framelen = T6721_FRAME_20MS;
                    }
                    if (t6721->condition2 & T6721_COND2_BPFRAME) {
                        t6721->cond2_framebits = T6721_FRAME_96BIT;
                    } else {
                        t6721->cond2_framebits = T6721_FRAME_48BIT;
                    }
                    DBGADD(("stages: %d repeat: %d len: %d0ms bits: %d\n", t6721->cond2_stages, t6721->cond2_repeat, t6721->cond2_framelen, t6721->cond2_framebits ? 96 : 48 ));
                    break;
            }

            t6721->cmd_nibbles--;
        } else {
            t6721->cmd_current = data;

            switch (data) {
                case 0x00: /* NOP nop */
                    DBG(("cmd %2x (nop)\n", data));
                    t6721->readmode = T6721_READMODE_STATUS;
                    break;
                case 0x01: /* STRT start */
                    DBG(("cmd %2x (start)\n", data));
                    set_playing(t6721, 1);
                    set_eos(t6721, 0);
                    t6721->status &= ~T6721_STATUS_ROM_DATA_ERROR; /* release rom data error status */
                    t6721->readmode = T6721_READMODE_STATUS;
                    break;
                case 0x02: /* STOP stop */
                    DBG(("cmd %2x (stop)\n", data));
                    set_playing(t6721, 0);
                    t6721->readmode = T6721_READMODE_STATUS;
                    break;
                case 0x03: /* ADDRESS LOAD 1/6 */
                    DBG(("cmd %2x (ADDRESS LOAD 1/6)\n", data));
                    t6721->cmd_nibbles = 5;
                    t6721->readmode = T6721_READMODE_STATUS;
                    break;
                case 0x04: /* AAGN AUDIO ASGIN */
                    DBG(("cmd %2x (AUDIO ASGIN)\n", data));
                    set_apd(t6721, 0); /* release audio power down signal */
                    t6721->readmode = T6721_READMODE_STATUS;
                    break;
                case 0x05: /* SPLD SPEED LOAD 1/2 */
                    DBG(("cmd %2x (SPEED LOAD 1/2)\n", data));
                    t6721->cmd_nibbles = 1;
                    t6721->readmode = T6721_READMODE_STATUS;
                    break;
                case 0x06: /* CNDT1 CONDITION 1 1/2 */
                    DBG(("cmd %2x (CONDITION 1 1/2)\n", data));
                    t6721->cmd_nibbles = 1;
                    t6721->readmode = T6721_READMODE_STATUS;
                    break;
                case 0x07: /* CNDT2 CONDITION 2 1/2 */
                    DBG(("cmd %2x (CONDITION 2 1/2)\n", data));
                    t6721->cmd_nibbles = 1;
                    t6721->readmode = T6721_READMODE_STATUS;
                    break;
                case 0x08: /* RRDM ROM READ MODE */
                    DBG(("cmd %2x (ROM READ MODE)\n", data));
                    t6721->readmode = T6721_READMODE_ROM;
                    break;
                case 0x09: /* SPDN system power down */
                    DBG(("cmd %2x (system power down)\n", data));
                    set_playing(t6721, 0);
                    t6721->readmode = T6721_READMODE_STATUS;
                    break;
                case 0x0a: /* APDN AUDIO POWER DOWN */
                    DBG(("cmd %2x (AUDIO POWER DOWN)\n", data));
                    set_apd(t6721, 1); /* raise audio power down signal */
                    t6721->readmode = T6721_READMODE_STATUS;
                    break;
                case 0x0b: /* SAGN system assign */
                    DBG(("cmd %2x (system assign)\n", data));
                    set_playing(t6721, 0);
                    t6721->status &= ~T6721_STATUS_SYSTEM_POWER_DOWN; /* release system power down status */
                    t6721->readmode = T6721_READMODE_STATUS;
                    break;
                /* c-f is "redundant" ? */
                case 0x0c:
                case 0x0d:
                case 0x0e:
                case 0x0f:
                    DBG(("cmd %2x (redundant ?)\n", data));
                    t6721->status |= T6721_STATUS_COMMAND_ERROR; /* raise command error status */
                    break;
            }
        }
    }

    t6721->wr_last = t6721->wr;
}

BYTE t6721_read(t6721_state *t6721)
{
    int data;

    if (t6721->readmode == T6721_READMODE_STATUS) {
        data = (t6721->status & 0x0f);
    } else {
        /* FIXME: read-through from speech ROM is missing */
        data = 0;
        DBG(("FIXME read-through from speech ROM\n"));
    }

    return data;
}

/*****************************************************************************/

void t6721_reset(t6721_state *t6721)
{
    t6721->cmd_nibbles = 0;
    t6721->cmd_current = 0;

    t6721->speed = 0; /* param for speed load cmd */
    t6721->condition1 = 0;
    t6721->condition2 = 0;

    t6721->busy = 0; /* BSY signal */
    set_apd(t6721, 1);  /* APD signal */
    set_eos(t6721, 0);  /* EOS signal */
    set_dtrd(t6721, 0);  /* DTRD signal */
    set_playing(t6721, 0);  /* stop playing */

    t6721->status = 0;
    t6721->readmode = T6721_READMODE_STATUS;
}

void t6721_sound_machine_init(t6721_state *t6721, int speed, int cycles_per_sec)
{
    DBG(("sound machine init: speed %d cycles/sec: %d\n", speed, cycles_per_sec));
    t6721->samples_per_sec = speed;
    t6721->cycles_per_sec = cycles_per_sec;
    /* ratio for converting ringbuffer -> output */
    upsmp = (((float)speed) / ((float)PARCOR_OUTPUT_HZ));
    /* ratio for converting samples in output buffer -> cpu/system cycles */
    up2smp = (((float)cycles_per_sec) / ((float)speed));
}

void t6721_sound_machine_init_vbr(t6721_state *t6721, int speed, int cycles_per_sec, int factor)
{
    DBG(("sound machine init: speed %d cycles/sec: %d\n", speed, cycles_per_sec));
    t6721->samples_per_sec = speed;
    t6721->cycles_per_sec = (cycles_per_sec * 1000 / factor);
    /* ratio for converting ringbuffer -> output */
    upsmp = (((float)speed) / ((float)PARCOR_OUTPUT_HZ));
    /* ratio for converting samples in output buffer -> cpu/system cycles */
    up2smp = (((float)cycles_per_sec) / ((float)speed));
}

int t6721_dump(t6721_state *t6721)
{
    mon_out("reference cycles per second: %d\n", t6721->cycles_per_sec);
    mon_out("output sample per second:    %d\n", t6721->samples_per_sec);
    mon_out("apd: %d busy: %d eos: %d playing: %d\n", t6721->apd, t6721->busy, t6721->eos, t6721->playing);
    return 0;
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "T6721"

/* FIXME: implement snapshot support */
int t6721_snapshot_write_module(snapshot_t *s, t6721_state *t6721)
{
    return -1;
#if 0
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}

int t6721_snapshot_read_module(snapshot_t *s, t6721_state *t6721)
{
    return -1;
#if 0
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CART_DUMP_VER_MAJOR) || (vminor != CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}
