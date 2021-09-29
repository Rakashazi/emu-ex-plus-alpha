/*
 * cbm2rom.c - CBM-6x0/7x0 rom code.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "autostart.h"
#include "cbm2.h"
#include "cbm2mem.h"
#include "cbm2rom.h"
#include "crtc.h"
#include "kbdbuf.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "sysfile.h"
#include "tape.h"
#include "types.h"
#include "util.h"


static log_t cbm2rom_log = LOG_ERR;

/* Flag: nonzero if the ROM has been loaded. */
static int rom_loaded = 0;

static tape_init_t tapeinit = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    NULL,
    36 * 8,
    54 * 8,
    55 * 8,
    73 * 8,
    74 * 8,
    100 * 8
};


int cbm2rom_load_chargen(const char *rom_name)
{
    int i;
    unsigned char *temp = NULL;

    if (!rom_loaded) {
        return 0;  /* init not far enough */
    }

    /* Load chargen ROM */
    /* the hardware has an interesting feature in that the inverted characters
       are not contained in the ROM, but some extra logic on the board will
       alter the data based on the address. we create the inverted characters
       here at load time, so we dont have to bother with this in the rest of the
       emulation */
    if (!util_check_null_string(rom_name)) {
        if ((temp = lib_malloc(0x2000)) == NULL) {
            return -1;
        }

        /* try loading 8k first, then 4k. normal "standard" ROMs only have one
           charset in the first 4k, the rest is FF, "localized" ROMs have a 
           second charset in the upper half. */
        memset(temp, 0xff, 0x2000);
        if (sysfile_load(rom_name, temp, 0x2000, 0x2000) < 0) {
            if (sysfile_load(rom_name, temp, 0x1000, 0x1000) < 0) {
                log_error(cbm2rom_log, "Couldn't load character ROM '%s'.", rom_name);
                lib_free(temp);
                return -1;
            }
        }
        /* now create the data the CRTC sees */
        memcpy(mem_chargen_rom, temp, 0x800); /* lowercase, not inverted */
        memcpy(mem_chargen_rom + 0x1000, temp + 0x800, 0x800); /* uppercase, not inverted */
        memcpy(mem_chargen_rom + 0x2000, temp + 0x1000, 0x800); /* lowercase, not inverted */
        memcpy(mem_chargen_rom + 0x3000, temp + 0x1800, 0x800); /* uppercase, not inverted */
        for (i = 0; i < 0x800; i++) {
            mem_chargen_rom[i + 0x800] = temp[i] ^ 0xff; /* lowercase, inverted */
            mem_chargen_rom[i + 0x1800] = temp[i + 0x800] ^ 0xff; /* uppercase, inverted */
            mem_chargen_rom[i + 0x2800] = temp[i + 0x1000] ^ 0xff; /* lowercase, inverted */
            mem_chargen_rom[i + 0x3800] = temp[i + 0x1800] ^ 0xff; /* uppercase, inverted */
        }
    }

    crtc_set_chargen_addr(mem_chargen_rom, CBM2_CHARGEN_ROM_SIZE >> 4);

    lib_free(temp);
    return 0;
}

int cbm2rom_checksum(void)
{
    int i;
    uint16_t sum;

    /* Checksum over top 8 kByte kernal.  */
    for (i = 0xe000, sum = 0; i < 0x10000; i++) {
        sum += mem_rom[i];
    }
    log_message(cbm2rom_log, "Kernal checksum is %d ($%04X).", sum, sum);

    /* Initialize Autostart */
    autostart_init(10, 0);
    return 0;
}

int cbm2rom_load_kernal(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;  /* init not far enough */
    }
    /* De-initialize kbd-buf, autostart and tape stuff here before
       reloading the ROM the traps are installed in.  */
    kbdbuf_init(0, 0, 0, 0);
    autostart_init(0, 0);
    tape_init(&tapeinit);

    /* Load Kernal ROM.  */
    if (!util_check_null_string(rom_name)) {
        if (sysfile_load(rom_name, mem_rom + 0xe000, 0x2000, 0x2000) < 0) {
            log_error(cbm2rom_log, "Couldn't load ROM `%s'.", rom_name);
            return -1;
        }
    }

    return cbm2rom_checksum();
}

int cbm2rom_load_basic(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;  /* init not far enough */
    }
    /* Load BASIC ROM.  */
    if (!util_check_null_string(rom_name)) {
        if ((sysfile_load(rom_name, mem_rom + 0x8000, 0x4000, 0x4000) < 0)) {
            log_error(cbm2rom_log, "Couldn't load BASIC ROM `%s'.",
                      rom_name);
            return -1;
        }
    } else {
        log_warning(cbm2rom_log, "Disabling BASIC by unloading ROM!");
        memset(mem_rom + 0x8000, 0xff, 0x4000);
    }
    return 0;
}

int cbm2rom_load_cart_1(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;  /* init not far enough */
    }
    if (!util_check_null_string(rom_name)) {
        if ((sysfile_load(rom_name, mem_rom + 0x1000, 0x1000, 0x1000) < 0)) {
            log_error(cbm2rom_log, "Couldn't load ROM `%s'.",
                      rom_name);
        }
    } else {
        memset(mem_rom + 0x1000, 0xff, 0x1000);
    }
    return 0;
}

int cbm2rom_load_cart_2(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;  /* init not far enough */
    }
    if (!util_check_null_string(rom_name)) {
        if ((sysfile_load(rom_name, mem_rom + 0x2000, 0x2000, 0x2000) < 0)) {
            log_error(cbm2rom_log, "Couldn't load ROM `%s'.",
                      rom_name);
        }
    } else {
        memset(mem_rom + 0x2000, 0xff, 0x2000);
    }
    return 0;
}

int cbm2rom_load_cart_4(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;  /* init not far enough */
    }
    if (!util_check_null_string(rom_name)) {
        if ((sysfile_load(rom_name, mem_rom + 0x4000, 0x2000, 0x2000) < 0)) {
            log_error(cbm2rom_log, "Couldn't load ROM `%s'.",
                      rom_name);
        }
    } else {
        memset(mem_rom + 0x4000, 0xff, 0x2000);
    }
    return 0;
}

int cbm2rom_load_cart_6(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;  /* init not far enough */
    }
    if (!util_check_null_string(rom_name)) {
        if ((sysfile_load(rom_name, mem_rom + 0x6000, 0x2000, 0x2000) < 0)) {
            log_error(cbm2rom_log, "Couldn't load ROM `%s'.",
                      rom_name);
        }
    } else {
        memset(mem_rom + 0x6000, 0xff, 0x2000);
    }
    return 0;
}

/* Load memory image files. */
int mem_load(void)
{
    int i;
    const char *rom_name = NULL;

    if (cbm2rom_log == LOG_ERR) {
        cbm2rom_log = log_open("CBM2MEM");
    }

    rom_loaded = 1;

    if (resources_get_string("ChargenName", &rom_name) < 0) {
        return -1;
    }
    if (cbm2rom_load_chargen(rom_name) < 0) {
        return -1;
    }

    /* Init Disk/Cartridge ROM with 'unused address' values.  */
    for (i = 0x800; i < 0x8000; i++) {
        mem_rom[i] = 0xff;
    }

    if (resources_get_string("KernalName", &rom_name) < 0) {
        return -1;
    }
    if (cbm2rom_load_kernal(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("BasicName", &rom_name) < 0) {
        return -1;
    }
    if (cbm2rom_load_basic(rom_name) < 0) {
        return -1;
    }

    /* Load extension ROMs.  */
    if (resources_get_string("Cart1Name", &rom_name) < 0) {
        return -1;
    }
    if (cbm2rom_load_cart_1(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("Cart2Name", &rom_name) < 0) {
        return -1;
    }
    if (cbm2rom_load_cart_2(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("Cart4Name", &rom_name) < 0) {
        return -1;
    }
    if (cbm2rom_load_cart_4(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("Cart6Name", &rom_name) < 0) {
        return -1;
    }
    if (cbm2rom_load_cart_6(rom_name) < 0) {
        return -1;
    }

    crtc_set_screen_addr(mem_rom + 0xd000);

    return 0;
}
