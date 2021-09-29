/*
 * pets.h - PET version handling.
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#ifndef VICE_PETS_H
#define VICE_PETS_H

#include "types.h"

#define PET_CHARGEN_NAME        "characters-2.901447-10.bin"
#define SUPERPET_CHARGEN_NAME   "characters.901640-01.bin"

#define PET_KERNAL1NAME  "kernal-1.901439-04-07.bin"
#define PET_KERNAL2NAME  "kernal-2.901465-03.bin"
#define PET_KERNAL4NAME  "kernal-4.901465-22.bin"

#define PET_BASIC1NAME  "basic-1.901439-09-05-02-06.bin"
#define PET_BASIC2NAME  "basic-2.901465-01-02.bin"
#define PET_BASIC4NAME  "basic-4.901465-23-20-21.bin"

#define PET_EDITOR1G40NAME  "edit-1-n.901439-03.bin"
#define PET_EDITOR2G40NAME  "edit-2-n.901447-24.bin"
#define PET_EDITOR2B40NAME  "edit-2-b.901474-01.bin"
#define PET_EDITOR4G40NAME  "edit-4-40-n-50Hz.901498-01.bin"
#define PET_EDITOR4B80NAME  "edit-4-80-b-50Hz.901474-04_.bin"
/* #define PET_EDITOR4B40NAME  "edit-4-b-noCRTC.901474-02.bin" // no CRTC */
#define PET_EDITOR4B40NAME  "edit-4-40-b-50Hz.ts.bin"

#define SUPERPET_6809_A_NAME "waterloo-a000.901898-01.bin"
#define SUPERPET_6809_B_NAME "waterloo-b000.901898-02.bin"
#define SUPERPET_6809_C_NAME "waterloo-c000.901898-03.bin"
#define SUPERPET_6809_D_NAME "waterloo-d000.901898-04.bin"
#define SUPERPET_6809_E_NAME "waterloo-e000.901897-01.bin"
#define SUPERPET_6809_F_NAME "waterloo-f000.901898-05.bin"

#define PET_COLS		80
#define PET_MAP_LINEAR          0
#define PET_MAP_8096            1
#define PET_MAP_8296            2

#define SUPERPET_CPU_6502       0
#define SUPERPET_CPU_6809       1
#define SUPERPET_CPU_PROG       2

#define NUM_6809_ROMS           6       /* at 0x[ABCDEF]000 */
#define PET_6809_ROMSIZE        (NUM_6809_ROMS * 0x1000)

#define RAM_8K    8
#define RAM_16K   16
#define RAM_32K   32
#define RAM_96K   96
#define RAM_128K  128

#define IO_256    256
#define IO_2048   2048

#define NO_CRTC   0
#define HAS_CRTC  1

#define COLS_AUTO 0
#define COLS_40   40
#define COLS_80   80

#define NO_RAM_9  0
#define HAS_RAM_9 1

#define NO_RAM_A  0
#define HAS_RAM_A 1

#define NO_KERNAL_PATCH 0
#define PATCH_2K_KERNAL 1

#define NO_CHARGEN_PATCH 0
#define PATCH_2K_CHARGEN 1

#define NO_EOI     0
#define EOI_BLANKS 1

#define NORMAL_IO   0
#define SUPERPET_IO 1

/* This struct is used to hold the default values for the different models */
typedef struct petinfo_s {
    /* hardware options (resources) */
    int ramSize;
    int IOSize;                 /* 256 Byte / 2k I/O */
    int crtc;                   /* 0 = no CRTC, 1 = has one */
    int video;                  /* 0 = autodetect, 40, or 80 */
    int ramsel9;                /* 0 = open/ROM, 1 = RAM: 8296 JU2 */
    int ramselA;                /* 0 = open/ROM, 1 = RAM: 8296 JU1*/
    int kbd_type;               /* see pet-resources.h */
    int pet2k;                  /* 1 = do PET 2001 kernal patches */
    int pet2kchar;              /* 1 = do PET 2001 chargen patches */
    int eoiblank;               /* 1 = EOI blanks screen */
    int superpet;               /* 1 = enable SuperPET I/O */

    /* ROM image resources */
    const char  *chargenName;   /* Character ROM */
    const char  *kernalName;    /* Kernal ROM $f*** */
    const char  *editorName;    /* $E*** ROM image filename (2k/4k) */
    const char  *basicName;     /* $b/c-$e*** basic ROM (8k/12k) */
    const char  *memBname;      /* $B*** ROM image filename */
    const char  *memAname;      /* $A*** ROM image filename */
    const char  *mem9name;      /* $9*** ROM image filename */
    /* SuperPET resources */
    char        *h6809romName[NUM_6809_ROMS];   /* $[ABCDEF]*** */
} petinfo_t;

/* This struct holds the resources and some other runtime-derived info */
typedef struct petres_s {
    /* hardware options (resources) */
    int ramSize;
    int IOSize;                 /* 256 Byte / 2k I/O */
    int crtc;                   /* 0 = no CRTC, 1 = has one */
    int video;                  /* 0 = autodetect, 40, or 80 */
    int ramsel9;                /* 0 = open/ROM, 1 = RAM: 8296 JU2 */
    int ramselA;                /* 0 = open/ROM, 1 = RAM: 8296 JU1*/
    int kbd_type;               /* 1 = graphics, 0 = business (UK) */
    int pet2k;                  /* 1 = do PET 2001 kernal patches */
    int pet2kchar;              /* 1 = do PET 2001 chargen patches */
    int eoiblank;               /* 1 = EOI blanks screen */
    int superpet;               /* 1 = enable SuperPET I/O */

    /* ROM image resources */
    char        *chargenName;   /* Character ROM */
    char        *kernalName;    /* Kernal ROM $f*** */
    char        *editorName;    /* $E*** ROM image filename (2k/4k) */
    char        *basicName;     /* $b/c-$e*** basic ROM (8k/12k) */
    char        *memBname;      /* $B*** ROM image filename */
    char        *memAname;      /* $A*** ROM image filename */
    char        *mem9name;      /* $9*** ROM image filename */

    /* SuperPET resources */
    char        *h6809romName[NUM_6809_ROMS];   /* $[ABCDEF]*** */
    int superpet_cpu_switch;         /* 0 = 6502, 1 = 6809E, 2 = "prog" */

    /* runtime (derived) variables */
    int videoSize;              /* video RAM size (1k or 2k) */
    int map;                    /* 0 = linear map, 1 = 8096 mapping */
                                /* 2 = 8296 mapping */
    int vmask;                  /* valid CRTC address bits */
    int rompatch;               /* 1 = need $ef** for ROM patch */
    int rom_video;              /* derived from ROM */
    uint16_t basic_start;           /* derived from ROM */
    uint16_t kernal_checksum;       /* derived from ROM */
    uint16_t editor_checksum;       /* derived from ROM */
} petres_t;

extern petres_t petres;

extern int pet_set_model(const char *model_name, void *extra); /* used by cmdline options */

extern int pet_init_ok; /* used in pet.c */

extern int petmem_set_conf_info(const petinfo_t *pi); /* used in petmemsnapshot.c */

#endif
