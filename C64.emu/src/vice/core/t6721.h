/*
 * t6721.h - Toshiba 6721a Emulation
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

#ifndef VICE_T6721_H
#define VICE_T6721_H

#include "types.h"

/* t6721a.pdf:581 status output */
#define T6721_STATUS_END_OF_SPEECH      0x01
#define T6721_STATUS_SYSTEM_POWER_DOWN  0x02
#define T6721_STATUS_ROM_DATA_ERROR     0x04
#define T6721_STATUS_COMMAND_ERROR      0x08

/* t6721a.pdf:577,578 commands */
#define T6721_COMMAND_NOP               0x00
#define T6721_COMMAND_STRT              0x01
#define T6721_COMMAND_STOP              0x02
#define T6721_COMMAND_ADLD              0x03    /* 6 nibbles */
#define T6721_COMMAND_AAGN              0x04
#define T6721_COMMAND_SPLD              0x05    /* 2 nibbles */
#define T6721_COMMAND_CNDT1             0x06    /* 2 nibbles */
#define T6721_COMMAND_CNDT2             0x07    /* 2 nibbles */
#define T6721_COMMAND_RRDM              0x08
#define T6721_COMMAND_SPDN              0x09
#define T6721_COMMAND_APDN              0x0a
#define T6721_COMMAND_SAGN              0x0b
/* 0x0c..0x0f are "redundant" */
#define T6721_COMMAND_UND_0C            0x0c
#define T6721_COMMAND_UND_0D            0x0d
#define T6721_COMMAND_UND_0E            0x0e
#define T6721_COMMAND_UND_0F            0x0f

/* t6721a.pdf:571 codes for speed */
#define T6721_SPEED_0_7        0x01
#define T6721_SPEED_0_8        0x02
#define T6721_SPEED_0_9        0x03
#define T6721_SPEED_1_0        0x04
#define T6721_SPEED_1_1        0x05
#define T6721_SPEED_1_2        0x06
#define T6721_SPEED_1_3        0x07
#define T6721_SPEED_1_4        0x08
#define T6721_SPEED_1_5        0x09
#define T6721_SPEED_1_55       0x0a
/* 0x00, 0x0b..0x0f are undefined */
#define T6721_SPEED_UND_00     0x00
#define T6721_SPEED_UND_0B     0x0b
#define T6721_SPEED_UND_0C     0x0c
#define T6721_SPEED_UND_0D     0x0d
#define T6721_SPEED_UND_0E     0x0e
#define T6721_SPEED_UND_0F     0x0f

/* t6721a.pdf:580 bits of CONDITION 1 */
#define T6721_COND1_LOSS       (1 << 2)
#define T6721_COND1_SHAPE      (1 << 3)
/* bit 0..1 should be 0 */
#define T6721_COND1_UND0       (1 << 0)
#define T6721_COND1_UND1       (1 << 1)

/* t6721a.pdf:580 bits of CONDITION 2 */
#define T6721_COND2_STAGES     (1 << 0) /* filter stages 0: 10 1: 8 */
#define T6721_COND2_REPEAT     (1 << 1) /* repeat 0: available 1: none */
#define T6721_COND2_LENGTH     (1 << 2) /* frame lemgth 0: 20ms 1: 10ms */
#define T6721_COND2_BPFRAME    (1 << 3) /* bits per frame 0: 48 1: 96 */

/* parameter values for CONDITION 1 */
#define T6721_LOSS_DISABLED    0
#define T6721_LOSS_ENABLED     1

#define T6721_SHAPE_PITCH      0
#define T6721_SHAPE_TRIANGLE   1

/* parameter values for CONDITION 2 */
#define T6721_STAGES_8         8
#define T6721_STAGES_10       10

#define T6721_REPEAT_DISABLED  0
#define T6721_REPEAT_ENABLED   1

#define T6721_FRAME_10MS       1
#define T6721_FRAME_20MS       2

#define T6721_FRAME_48BIT      0
#define T6721_FRAME_96BIT      1

/* reading from D0..D3 either gives status or speach ROM */
#define T6721_READMODE_STATUS  0
#define T6721_READMODE_ROM     1

/* t6721a.pdf:595 pin description */

#define T6721_FRAMETYPE_EOS       0 /* 1 */
#define T6721_FRAMETYPE_ZERO      1 /* 1 */
#define T6721_FRAMETYPE_SILENT    2 /* 1 */
#define T6721_FRAMETYPE_UNVOICED  3 /* 6 */
#define T6721_FRAMETYPE_VOICED    4 /* 12 */

typedef struct _t6721_state {
    int cmd_nibbles; /* nibbles left for current command */
    int cmd_current; /* current command */

    int speed;       /* set by speed load cmd */
    int condition1;  /* set by condition1 cmd */
    int condition2;  /* set by condition2 cmd */
    /* individual parameter values set by condition1 and condition 2 */
    int cond1_loss;
    int cond1_shape;
    int cond2_stages;
    int cond2_repeat;
    int cond2_framelen;
    int cond2_framebits;

    int rd, wr; /* input: read and write */
    int rd_last, wr_last; /* internal flags for edge triggered operation */

    int busy;   /* output: BSY busy */
    int apd;    /* output: APD external APD audio power down */
    int eos;    /* output: EOS end of speech */
    int dtrd;   /* output: data read */

    int status; /* output: D0..D3 */

    int playing;  /* chip is playing audio */
    int playing_delay;

    int readmode; /* chip readmode, rom or status */

    int eos_samples; /* samples left until eos is deasserted */

    int samples_per_sec; /* output samples per second */
    int cycles_per_sec; /* CPU/System cycles per second */

    int cycles_done; /* number of cycles the chip has run since last update_output */

    BYTE (*read_data)(struct _t6721_state*, unsigned int *bit); /* input: DI */
    void (*set_apd)(struct _t6721_state*);
    void (*set_eos)(struct _t6721_state*);
    void (*set_dtrd)(struct _t6721_state*);
} t6721_state;

extern void t6721_reset(t6721_state *t6721);
extern void t6721_sound_machine_init(t6721_state *t6721, int samples_per_sec, int cycles_per_sec);
/* read/write from/to  d0..d3 */
extern BYTE t6721_read(t6721_state *t6721); /* read from d0..d3 (status) */
extern void t6721_store(t6721_state *t6721, BYTE data); /* store to d0..d3 (command) */
/* run chip for N cpu/system cycles */
extern void t6721_update_tick(t6721_state *t6721);
extern void t6721_update_ticks(t6721_state *t6721, int ticks);
/* update output sound buffer, run chip (remaining ticks) */
extern void t6721_update_output(t6721_state *t6721, SWORD *buf, int num);

extern int t6721_dump(t6721_state *t6721);

struct snapshot_s;
extern int t6721_snapshot_read_module(struct snapshot_s *s, t6721_state *t6721);
extern int t6721_snapshot_write_module(struct snapshot_s *s, t6721_state *t6721);

#endif
