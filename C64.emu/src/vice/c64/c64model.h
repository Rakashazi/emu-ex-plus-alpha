/*
 * c64model.h - C64 model detection and setting.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_C64MODEL_H
#define VICE_C64MODEL_H

#include "types.h"

enum {
    C64MODEL_C64_PAL = 0,
    C64MODEL_C64C_PAL,
    C64MODEL_C64_OLD_PAL,

    C64MODEL_C64_NTSC,
    C64MODEL_C64C_NTSC,
    C64MODEL_C64_OLD_NTSC,

    C64MODEL_C64_PAL_N,

/* SX-64 */
    C64MODEL_C64SX_PAL,
    C64MODEL_C64SX_NTSC,

    C64MODEL_C64_JAP,
    C64MODEL_C64_GS,

/* 4064, PET64, EDUCATOR64 */
    C64MODEL_PET64_PAL,
    C64MODEL_PET64_NTSC,

/* max machine */
    C64MODEL_ULTIMAX,

/* This entry needs to always be at the end */
    C64MODEL_NUM
};

#define C64MODEL_UNKNOWN 99

#define OLD_CIA 0
#define NEW_CIA 1

#define OLD_SID 0
#define NEW_SID 1

#define GLUE_DISCRETE  0
#define GLUE_CUSTOM_IC 1

#define BOARD_C64 0
#define BOARD_MAX 1

#define IEC_HARD_RESET 0
#define IEC_SOFT_RESET 1

#define NO_DATASETTE 0
#define HAS_DATASETTE 1

#define NO_IEC  0
#define HAS_IEC 1

#define NO_USERPORT  0
#define HAS_USERPORT 1

#define NO_KEYBOARD  0
#define HAS_KEYBOARD 1

#define NO_CIA2  0
#define HAS_CIA2 1

#define CIATICK_NET     0
#define CIATICK_60HZ    1

typedef struct {
    int vicii_model;
    int sid_model;
    int glue_logic; /* x64sc only */
    int cia1_model;
    int cia2_model;
    int cia_tick;
    int board; /* 0: normal, 1: ultimax */
    int iecreset; /* 1: reset goes to IEC bus (old) 0: only reset IEC on hard reset (new) */
    const char *kernal;
    const char *chargen;
    int kernalrev;
} c64model_details_t;

int c64model_get(void);
void c64model_set(int model);
/* get details for model */
void c64model_set_details(c64model_details_t *details, int model);
/* get model from details */
int c64model_get_model(c64model_details_t *details);

#endif
