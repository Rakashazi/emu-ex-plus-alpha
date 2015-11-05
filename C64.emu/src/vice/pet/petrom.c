/*
 * petrom.c
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Andreas Boose <viceteam@t-online.de>
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
#include <string.h>

#include "autostart.h"
#include "kbdbuf.h"
#include "crtc.h"
#include "log.h"
#include "mem.h"
#include "pet.h"
#include "petmem.h"
#include "petrom.h"
#include "pets.h"
#include "resources.h"
#include "sysfile.h"
#include "tape.h"
#include "traps.h"
#include "types.h"
#include "util.h"


int petrom_9_loaded = 0;    /* 1 = $9*** ROM is loaded */
int petrom_A_loaded = 0;    /* 1 = $A*** ROM is loaded */
int petrom_B_loaded = 0;    /* 1 = $B*** ROM or Basic 4 is loaded */

static log_t petrom_log = LOG_ERR;

/* Flag: nonzero if the ROM has been loaded. */
static int rom_loaded = 0;

/* where we save the unchanged PET kernal 1 areas before patching */
static BYTE petmem_2001_patchbuf_f1[256];
static BYTE petmem_2001_patchbuf_f3[256];
static BYTE petmem_2001_patchbuf_f4[256];
static BYTE petmem_2001_patchbuf_f6[256];

/* Tape traps.  */
static const trap_t pet4_tape_traps[] =
{
    {
        "TapeFindHeader",
        0xF5E8,
        0xF5EB,
        { 0x20, 0x9A, 0xF8 },
        tape_find_header_trap,
        rom_read,
        rom_store
    },
    {
        "TapeReceive",
        0xF8E0,
        0xFCC0,
        { 0x20, 0xE0, 0xFC },
        tape_receive_trap,
        rom_read,
        rom_store
    },
    {
        NULL,
        0,
        0,
        { 0, 0, 0 },
        NULL,
        NULL,
        NULL
    }
};

static const trap_t pet3_tape_traps[] =
{
    {
        "TapeFindHeader",
        0xF5A9,
        0xF5AC,
        { 0x20, 0x55, 0xF8 },
        tape_find_header_trap,
        rom_read,
        rom_store
    },
    {
        "TapeReceive",
        0xF89B,
        0xFC7B,
        { 0x20, 0x9B, 0xFC },
        tape_receive_trap,
        rom_read,
        rom_store
    },
    {
        NULL,
        0,
        0,
        { 0, 0, 0 },
        NULL,
        NULL,
        NULL
    }
};

static const trap_t pet2_tape_traps[] =
{
    {
        "TapeFindHeader",
        0xF5B2,
        0xF5B5,
        { 0x20, 0x7F, 0xF8 },
        tape_find_header_trap,
        rom_read,
        rom_store
    },
    {
        "TapeReceive",
        0xF8A5,
        0xFCFB,
        { 0x20, 0x1B, 0xFD },
        tape_receive_trap,
        rom_read,
        rom_store
    },
    {
        NULL,
        0,
        0,
        { 0, 0, 0 },
        NULL,
        NULL,
        NULL
    }
};

static const tape_init_t tapeinit1 = {
    243,
    0x20c,
    0x20b,
    0x219,
    0xe685,
    247,
    229,
    0x20f,
    0x20d,
    pet2_tape_traps,
    36 * 8,
    54 * 8,
    55 * 8,
    73 * 8,
    74 * 8,
    100 * 8
};

static const tape_init_t tapeinit2 = {
    214,
    150,
    157,
    144,
    0xe62e,
    251,
    201,
    0x26f,
    0x9e,
    pet3_tape_traps,
    36 * 8,
    54 * 8,
    55 * 8,
    73 * 8,
    74 * 8,
    100 * 8
};

static const tape_init_t tapeinit4 = {
    214,
    150,
    157,
    144,
    0xe455,
    251,
    201,
    0x26f,
    0x9e,
    pet4_tape_traps,
    36 * 8,
    54 * 8,
    55 * 8,
    73 * 8,
    74 * 8,
    100 * 8
};

void petrom_unpatch_2001(void)
{
    /* if not patched return */
    if (!petres.rompatch) {
        return;
    }

    log_warning(petrom_log,
                "PET2001 ROM loaded, but patches disabled! "
                "IEEE488 will not work.");

    memcpy(mem_rom + 0x7100, petmem_2001_patchbuf_f1, 0x100);
    memcpy(mem_rom + 0x7300, petmem_2001_patchbuf_f3, 0x100);
    memcpy(mem_rom + 0x7400, petmem_2001_patchbuf_f4, 0x100);
    memcpy(mem_rom + 0x7600, petmem_2001_patchbuf_f6, 0x100);

    petres.rompatch = 0;

    mem_initialize_memory();
}

void petrom_patch_2001(void)
{
    int i;
    int rp;
    const BYTE dat0[] = { 0xa9, 0x60, 0x85, 0xf0, 0x60 };
    const BYTE dat1[] = { 0x20, 0xb6, 0xf0, 0xa5, 0xf0, 0x20, 0x5b, 0xf1,
                          0x20, 0x87, 0xf1, 0x85, 0xf7,
                          0x20, 0x87, 0xf1, 0x85, 0xf8, 0x60 };
    const BYTE dat2[] = { 0x20, 0x7a, 0xf1, 0x20, 0xe6, 0xf6,
                          0xad, 0x0b, 0x02, 0x60};
    const BYTE dat3[] = { 0xa9, 0x61, 0x85, 0xf0, 0x60 };
    const BYTE dat4[] = { 0x20, 0xba, 0xf0, 0xa5, 0xf0, 0x20, 0x2c, 0xf1,
                          0xa5, 0xf7, 0x20, 0x67, 0xf1,
                          0xa5, 0xf8, 0x4c, 0x67, 0xf1 };
    const BYTE dat5[] = { 0xae, 0x0c, 0x02, 0x70, 0x46, 0x20, 0x87, 0xf1};
    const BYTE dat6[] = { 0x20, 0x2c, 0xf1, 0x4c, 0x7e, 0xf1 };

    /* check if already patched */
    if (petres.rompatch) {
        return;
    }

    /* check for ROM version */
    if (petres.kernal_checksum != PET_KERNAL1_CHECKSUM) {
        return;
    }

    /* check whether patch enabled */
    if (!petres.pet2k) {
        log_warning(petrom_log,
                    "PET2001 ROM loaded, but patches not enabled! "
                    "IEEE488 will not work.");
        return;
    }

    log_warning(petrom_log, "patching 2001 ROM to make IEEE488 work!");

    memcpy(petmem_2001_patchbuf_f1, mem_rom + 0x7100, 0x100);
    memcpy(petmem_2001_patchbuf_f3, mem_rom + 0x7300, 0x100);
    memcpy(petmem_2001_patchbuf_f4, mem_rom + 0x7400, 0x100);
    memcpy(petmem_2001_patchbuf_f6, mem_rom + 0x7600, 0x100);

    /* Patch PET2001 IEEE488 routines to make them work */
    mem_rom[0x7471] = mem_rom[0x7472] = 0xea;   /* NOP */
    mem_rom[0x7180] = mem_rom[0x7181] = 0xea;   /* NOP */
    mem_rom[0x73ef] = 0xf8;
    mem_rom[0x73f3] = 0xf7;
    rp = 0xef00;                /* $ef00 */
    mem_rom[0x7370] = rp & 0xff;
    mem_rom[0x7371] = ((rp >> 8) & 0xff);
    for (i = 0; i < 5; i++) {
        petmem_2001_buf_ef[(rp++) & 0xff] = dat0[i];
    }
    mem_rom[0x7379] = rp & 0xff;
    mem_rom[0x737a] = ((rp >> 8) & 0xff);
    for (i = 0; i < 19; i++) {
        petmem_2001_buf_ef[(rp++) & 0xff] = dat1[i];
    }
    mem_rom[0x73cc] = 0x20;
    mem_rom[0x73cd] = rp & 0xff;
    mem_rom[0x73ce] = ((rp >> 8) & 0xff);
    for (i = 0; i < 10; i++) {
        petmem_2001_buf_ef[(rp++) & 0xff] = dat2[i];
    }
    for (i = 0; i < 8; i++) {
        mem_rom[0x7381 + i] = dat5[i];
    }

    mem_rom[0x76c1] = rp & 0xff;
    mem_rom[0x76c2] = ((rp >> 8) & 0xff);
    for (i = 0; i < 5; i++) {
        petmem_2001_buf_ef[(rp++) & 0xff] = dat3[i];
    }
    mem_rom[0x76c7] = rp & 0xff;
    mem_rom[0x76c8] = ((rp >> 8) & 0xff);
    for (i = 0; i < 18; i++) {
        petmem_2001_buf_ef[(rp++) & 0xff] = dat4[i];
    }
    mem_rom[0x76f4] = rp & 0xff;
    mem_rom[0x76f5] = ((rp >> 8) & 0xff);
    for (i = 0; i < 6; i++) {
        petmem_2001_buf_ef[(rp++) & 0xff] = dat6[i];
    }

    strcpy((char*)(petmem_2001_buf_ef + (rp & 0xff)), "vice pet2001 rom patch $ef00-$efff");

    petres.rompatch = 1;

    mem_initialize_memory();
}

void petrom_get_kernal_checksum(void)
{
    int i;

    /* Checksum over top 4 kByte PET kernal.  */
    petres.kernal_checksum = 0;
    for (i = 0x7000; i < 0x8000; i++) {
        petres.kernal_checksum += mem_rom[i];
    }
}

void petrom_get_editor_checksum(void)
{
    int i;

    /* 4032 and 8032 have the same kernals, so we have to test more, here
       $E000 - $E800.  */
    petres.editor_checksum = 0;
    for (i = 0x6000; i < 0x6800; i++) {
        petres.editor_checksum += mem_rom[i];
    }
}

void petrom_checksum(void)
{
    static WORD last_kernal = 0;
    static WORD last_editor = 0;
    int delay;

    /* log_message(petrom_log, "editor checksum=%d, kernal checksum=%d",
                   (int) petres.editor_checksum,
                   (int) petres.kernal_checksum); */

    /* ignore the same message popping up more than once - but only
       check when printing, because we need the tape traps etc */

    petres.rom_video = 0;

    resources_get_int("AutostartDelay", &delay);
    if (delay == 0) {
        delay = 3; /* default */
    }

    /* The length of the keyboard buffer might actually differ from 10 - in
       the 4032 and 8032 50Hz editor ROMs it is checked against different
       memory locations (0xe3 and 0x3eb) but by default (power-up) it's 10
       anyway.  AF 30jun1998 */
    if (petres.kernal_checksum == PET_KERNAL4_CHECKSUM) {
        if (petres.kernal_checksum != last_kernal) {
            log_message(petrom_log, "Identified Kernal 4 ROM by checksum.");
        }
        kbdbuf_init(0x26f, 0x9e, 10,
                    (CLOCK)(PET_PAL_CYCLES_PER_RFSH * PET_PAL_RFSH_PER_SEC));
        tape_init(&tapeinit4);
        if (petres.editor_checksum == PET_EDIT4B80_CHECKSUM) {
            if (petres.editor_checksum != last_editor) {
                log_message(petrom_log, "Identified 80 columns editor by checksum.");
            }
            petres.rom_video = 80;
            autostart_init((CLOCK)(delay * PET_PAL_RFSH_PER_SEC * PET_PAL_CYCLES_PER_RFSH),
                           0, 0xa7, 0xc4, 0xc6, -80);
        } else
        if (petres.editor_checksum == PET_EDIT4B40_CHECKSUM
            || petres.editor_checksum == PET_EDIT4G40_CHECKSUM) {
            if (petres.editor_checksum != last_editor) {
                log_message(petrom_log, "Identified 40 columns editor by checksum.");
            }
            petres.rom_video = 40;
            autostart_init((CLOCK)(delay * PET_PAL_RFSH_PER_SEC * PET_PAL_CYCLES_PER_RFSH),
                           0, 0xa7, 0xc4, 0xc6, -40);
        }
    } else if (petres.kernal_checksum == PET_KERNAL2_CHECKSUM) {
        if (petres.kernal_checksum != last_kernal) {
            log_message(petrom_log, "Identified Kernal 2 ROM by checksum.");
        }
        petres.rom_video = 40;
        kbdbuf_init(0x26f, 0x9e, 10,
                    (CLOCK)(PET_PAL_CYCLES_PER_RFSH * PET_PAL_RFSH_PER_SEC));
        autostart_init((CLOCK)(delay * PET_PAL_RFSH_PER_SEC * PET_PAL_CYCLES_PER_RFSH), 0,
                       0xa7, 0xc4, 0xc6, -40);
        tape_init(&tapeinit2);
    } else if (petres.kernal_checksum == PET_KERNAL1_CHECKSUM) {
        if (petres.kernal_checksum != last_kernal) {
            log_message(petrom_log, "Identified Kernal 1 ROM by checksum.");
        }
        petres.rom_video = 40;
        kbdbuf_init(0x20f, 0x20d, 10,
                    (CLOCK)(PET_PAL_CYCLES_PER_RFSH * PET_PAL_RFSH_PER_SEC));
        autostart_init((CLOCK)(delay * PET_PAL_RFSH_PER_SEC * PET_PAL_CYCLES_PER_RFSH), 0,
                       0x224, 0xe0, 0xe2, -40);
        tape_init(&tapeinit1);
    } else {
        log_warning(petrom_log, "Unknown PET ROM.");
    }
    last_kernal = petres.kernal_checksum;
    last_editor = petres.editor_checksum;
}

void petrom_convert_chargen(BYTE *charrom)
{
    int i, j;

    /*
     * Make space for inverted versions of the characters,
     * by moving 1K blocks of character data (128 chars each) apart.
     */
    memmove(charrom + 0x1800, charrom + 0x0c00, 0x400);
    memmove(charrom + 0x1000, charrom + 0x0800, 0x400);
    memmove(charrom + 0x0800, charrom + 0x0400, 0x400);

    /*
     * Inverted chargen into second half. This is a PET hardware feature.
     * After that we have 256-char charsets (2K each).
     */
    for (i = 0; i < 0x400; i++) {
        charrom[i + 0x0400] = charrom[i         ] ^ 0xff;
        charrom[i + 0x0c00] = charrom[i + 0x0800] ^ 0xff;
        charrom[i + 0x1400] = charrom[i + 0x1000] ^ 0xff;
        charrom[i + 0x1c00] = charrom[i + 0x1800] ^ 0xff;
    }

    /* now expand 8 byte/char to 16 byte/char charrom for the CRTC */
    for (i = 1023; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            charrom[i * 16 + j] = charrom[i * 8 + j];
        }
        for (j = 7; j >= 0; j--) {
            charrom[i * 16 + 8 + j] = 0;
        }
    }
}

void petrom_convert_chargen_2k(void)
{
    int i, j;

#if 0
    /* This only works right after loading! */
    /* If pet2001 then exchange upper and lower case letters.  */
    for (i = 8; i < (0x1b * 8); i++) {
        j = mem_chargen_rom[0x400 + i];
        mem_chargen_rom[i + 0x400] = mem_chargen_rom[i + 0x600];
        mem_chargen_rom[i + 0x600] = j;
    }
#endif
    /* If pet2001 then exchange upper and lower case letters.  */
    for (i = 16; i < 0x1b0; i++) {
        /* first the not inverted chars */
        j = mem_chargen_rom[0x1000 + i];
        mem_chargen_rom[0x1000 + i] = mem_chargen_rom[0x1400 + i];
        mem_chargen_rom[0x1400 + i] = j;
    }
    /* If pet2001 then exchange upper and lower case letters.  */
    for (i = 16; i < 0x1b0; i++) {
        /* then the inverted chars */
        j = mem_chargen_rom[0x1800 + i];
        mem_chargen_rom[0x1800 + i] = mem_chargen_rom[0x1c00 + i];
        mem_chargen_rom[0x1c00 + i] = j;
    }
}

/*
 * The Waterloo chargen rom is actually 4K, containing 2 sets of
 * characters: Commodore's original, and Waterloo ASCII/APL.
 * Selecting between the 2 is done with CRTC register $0C: $30 for the
 * Waterloo fonts, $10 for the Commodore fonts.
 * (Source: Appendix C of the SuperPET System Overview manual)
 * From crtc/crtc-mem.c:
 *       * Bit 5: use top half of 4K character generator
 */

int petrom_load_chargen(void)
{
    int rsize;
    int numchars;

    if (!rom_loaded) {
        return 0;
    }

    if (util_check_null_string(petres.chargenName)) {
        return 0;
    }

    /* Load chargen ROM - we load 2k with 8 bytes/char, and generate
       the inverted 2k. Then we expand the chars to 16 bytes/char
       for the CRTC, filling the rest with zeros.
       The SuperPET has a 4k ROM. The second half contains ASCII/APL
       characters.
     */

    /* memset(mem_chargen_rom, 1, 0x1000); */
    rsize = sysfile_load(petres.chargenName, mem_chargen_rom, -0x800, 0x1000);
    if (rsize < 0) {
        log_error(petrom_log,
                  "Couldn't load character ROM (%s).", petres.chargenName);
        return -1;
    }

    if (petres.pet2kchar) {
        petrom_convert_chargen_2k();
    }

    petrom_convert_chargen(mem_chargen_rom);

    numchars = (rsize == 0x1000) ? 4 * 256 : 2 * 256;
    crtc_set_chargen_addr(mem_chargen_rom, numchars);

    return 0;
}

int petrom_load_basic(void)
{
    int krsize;
    WORD old_start, new_start;

    if (!rom_loaded) {
        return 0;
    }

    /* Load Kernal ROM.  */
    if (!util_check_null_string(petres.basicName)) {
        const char *name = petres.basicName;

        if ((krsize = sysfile_load(name, mem_rom + 0x3000, 0x2000, 0x3000)) < 0) {
            log_error(petrom_log, "Couldn't load ROM `%s'.", name);
            return -1;
        }

        old_start = petres.basic_start;
        new_start = 0xe000 - krsize;

        petres.basic_start = new_start;

        if (old_start && (new_start > old_start)) {
            if (old_start <= 0xB000 && new_start >= 0xC000) {
                resources_set_string("RomModuleBName", NULL);
            }
        }

        /* setting the _loaded flag to 0 before setting the resource
           does not overwrite ROM! */
        /* if kernal long enough, "unload" expansion ROMs */
        if (petres.basic_start <= 0xb000) {
            petrom_B_loaded = 0;
            resources_set_string("RomModuleBName", NULL);
            petrom_B_loaded = 1;
        }
    }
    return 0;
}

int petrom_load_kernal(void)
{
    int krsize;

    if (!rom_loaded) {
        return 0;
    }

    /* De-initialize kbd-buf, autostart and tape stuff here before
       reloading the ROM the traps are installed in.  */
    /* log_warning(pet_mem_log, "Deinstalling Traps"); */
    kbdbuf_init(0, 0, 0, 0);
    autostart_init(0, 0, 0, 0, 0, 0);
    tape_deinstall();

    /* Load Kernal ROM.  */
    if (!util_check_null_string(petres.kernalName)) {
        const char *name = petres.kernalName;

        if ((krsize = sysfile_load(name, mem_rom + 0x7000, 0x1000, 0x1000)) < 0) {
            log_error(petrom_log, "Couldn't load ROM `%s'.", name);
            return -1;
        }
        petrom_get_kernal_checksum();
        petres.rompatch = 0;
        petrom_patch_2001();
    }

    petrom_checksum();

    return 0;
}

int petrom_load_editor(void)
{
    int rsize, i;

    if (!rom_loaded) {
        return 0;
    }

    /* De-initialize kbd-buf, autostart and tape stuff here before
       reloading the ROM the traps are installed in.  */
    /* log_warning(pet_mem_log, "Deinstalling Traps"); */
    kbdbuf_init(0, 0, 0, 0);
    autostart_init(0, 0, 0, 0, 0, 0);
    tape_deinstall();

    if (!util_check_null_string(petres.editorName)) {
        const char *name = petres.editorName;

        if ((rsize = sysfile_load(name, mem_rom + 0x6000, -0x0800, 0x1000)) < 0) {
            log_error(petrom_log, "Couldn't load ROM `%s'.", name);
            return -1;
        }
        if (rsize == 0x800) {
            for (i = 0x800; i < 0x1000; i++) {
                *(mem_rom + 0x6000 + i) = 0xe0 | (i >> 8);
            }
        }
        petrom_get_editor_checksum();
    }

    petrom_checksum();

    return 0;
}

int petrom_load_rom9(void)
{
    int rsize, i;

    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(petres.mem9name)) {
        if ((rsize = sysfile_load(petres.mem9name, mem_rom + 0x1000, -0x0800, 0x1000)) < 0) {
            log_error(petrom_log, "Couldn't load ROM `%s'.", petres.mem9name);
            return -1;
        }
        if (rsize == 0x800) {
            for (i = 0x800; i < 0x1000; i++) {
                *(mem_rom + 0x1000 + i) = 0x90 | (i >> 8);
            }
        }
        petrom_9_loaded = 1;
    } else {
        if (petres.basic_start >= 0xA000) {
            for (i = 0; i < 16; i++) {
                memset(mem_rom + 0x1000 + (i << 8), 0x90 + i, 256);
            }
        }
        petrom_9_loaded = 0;
    }
    return 0;
}

int petrom_load_romA(void)
{
    int rsize, i;

    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(petres.memAname)) {
        if ((rsize = sysfile_load(petres.memAname, mem_rom + 0x2000, -0x0800, 0x1000)) < 0) {
            log_error(petrom_log, "Couldn't load ROM `%s'.", petres.memAname);
            return -1;
        }
        if (rsize == 0x800) {
            for (i = 0x800; i < 0x1000; i++) {
                *(mem_rom + 0x2000 + i) = 0xA0 | (i >> 8);
            }
        }
        petrom_A_loaded = 1;
    } else {
        if (petres.basic_start >= 0xB000) {
            for (i = 0; i < 16; i++) {
                memset(mem_rom + 0x2000 + (i << 8), 0xA0 + i, 256);
            }
        }
        petrom_A_loaded = 0;
    }
    return 0;
}

int petrom_load_romB(void)
{
    int rsize, i;

    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(petres.memBname)) {
        if ((rsize = sysfile_load(petres.memBname, mem_rom + 0x3000, -0x0800, 0x1000)) < 0) {
            log_error(petrom_log, "Couldn't load ROM `%s'.",
                      petres.memBname);
            return -1;
        }
        if (rsize == 0x800) {
            for (i = 0x800; i < 0x1000; i++) {
                *(mem_rom + 0x3000 + i) = 0xB0 | (i >> 8);
            }
        }
        petrom_B_loaded = 1;
    } else {
        if (petres.basic_start >= 0xC000) {
            for (i = 0; i < 16; i++) {
                memset(mem_rom + 0x3000 + (i << 8), 0xB0 + i, 256);
            }
            petrom_B_loaded = 0;
        }
    }
    return 0;
}

/*
 * Load a SuperPET 6809 ROM.
 * This is set up so that there is a resource to name an image file for each
 * original 4K (EP)ROM, but if a file is larger than that, it works too
 * (unless you have overlapping files).
 * It suffices to set H6809RomAName="waterloo-everything" if you
 * have a single 24 KB file with all the ROM contents.
 * Whatever is loaded into the I/O range E800..EFFF is automatically
 * ignored by the memory mapping.
 */
int petrom_load_6809rom(int num)
{
    if (!rom_loaded) {
        return 0;
    }

    if (num >= NUM_6809_ROMS) {
        return -1;
    }

    if (!util_check_null_string(petres.h6809romName[num])) {
        int rsize;
        int startoff = num * 0x1000;
        int startaddr = 0xa000 + startoff;
        int maxsize = 0x10000 - startaddr;
        int minsize = (startaddr == 0xE000) ? -0x800 : -0x1000;

        if ((rsize = sysfile_load(petres.h6809romName[num], mem_6809rom + startoff, minsize, maxsize)) < 0) {
            log_error(petrom_log, "Couldn't load 6809 ROM `%s'.",
                      petres.h6809romName[num]);
            return -1;
        }
    }
    return 0;
}

int mem_load(void)
{
    int i;

    if (petrom_log == LOG_ERR) {
        petrom_log = log_open("PETMEM");
    }

    rom_loaded = 1;

    tape_deinstall();

    /* Init ROM with 'unused address' values.  */
    for (i = 0; i < PET_ROM_SIZE; i++) {
        mem_rom[i] = 0x80 + ((i >> 8) & 0xff);
    }

    if (petrom_load_chargen() < 0) {
        return -1;
    }

    if (petrom_load_basic() < 0) {
        return -1;
    }

    if (petrom_load_kernal() < 0) {
        return -1;
    }

    if (petrom_load_editor() < 0) {
        return -1;
    }

    if (petrom_load_rom9() < 0) {
        return -1;
    }

    if (petrom_load_romA() < 0) {
        return -1;
    }

    if (petrom_load_romB() < 0) {
        return -1;
    }

    if (petres.rom_video) {
        log_message(petrom_log, "ROM screen width is %d.",
                    petres.rom_video);
    } else {
        log_message(petrom_log, "ROM screen width is unknown.");
    }

    {
        int i;

        for (i = 0; i < NUM_6809_ROMS; i++) {
            if (petrom_load_6809rom(i) < 0) {
                return -1;
            }
        }
    }

    mem_initialize_memory();

    return 0;
}
