/*
 * vic20rom.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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
#include <string.h>

#include "log.h"
#include "machine.h"
#include "mem.h"
#include "resources.h"
#include "sysfile.h"
#include "types.h"
#include "util.h"
#include "vic20mem.h"
#include "vic20memrom.h"
#include "vic20rom.h"


static log_t vic20rom_log = LOG_ERR;

/* Flag: nonzero if the Kernal and BASIC ROMs have been loaded.  */
static int vicrom_loaded = 0;


int vic20rom_kernal_checksum(void)
{
    int i;
    WORD sum;

    /* Check Kernal ROM.  */
    for (i = 0, sum = 0; i < VIC20_KERNAL_ROM_SIZE; i++) {
        sum += vic20memrom_kernal_rom[i];
    }

    if (sum != VIC20_KERNAL_CHECKSUM) {
        log_warning(vic20rom_log,
                  "Unknown Kernal image.  Sum: %d ($%04X).",
                  sum, sum);
        return -1;
    }
    return 0;
}

int vic20rom_load_kernal(const char *rom_name)
{
    int trapfl;

    if (!vicrom_loaded) {
        return 0;
    }

    /* disable traps before saving the ROM */
    resources_get_int("VirtualDevices", &trapfl);
    resources_set_int("VirtualDevices", 1);

    /* Load Kernal ROM. */
    if (sysfile_load(rom_name,
                     vic20memrom_kernal_rom, VIC20_KERNAL_ROM_SIZE,
                     VIC20_KERNAL_ROM_SIZE) < 0) {
        log_error(vic20rom_log, "Couldn't load kernal ROM.");
        resources_set_int("VirtualDevices", trapfl);
        return -1;
    }

    vic20rom_kernal_checksum();
    memcpy(vic20memrom_kernal_trap_rom, vic20memrom_kernal_rom,
           VIC20_KERNAL_ROM_SIZE);

    resources_set_int("VirtualDevices", trapfl);

    return 0;
}

int vic20rom_basic_checksum(void)
{
    int i;
    WORD sum;

    /* Check Basic ROM. */
    for (i = 0, sum = 0; i < VIC20_BASIC_ROM_SIZE; i++) {
        sum += vic20memrom_basic_rom[i];
    }

    if (sum != VIC20_BASIC_CHECKSUM) {
        log_warning(vic20rom_log,
                  "Unknown Basic image.  Sum: %d ($%04X).",
                  sum, sum);
    }
    return 0;
}

int vic20rom_load_basic(const char *rom_name)
{
    if (!vicrom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load Basic ROM. */
        if (sysfile_load(rom_name,
                         vic20memrom_basic_rom, VIC20_BASIC_ROM_SIZE,
                         VIC20_BASIC_ROM_SIZE) < 0) {
            log_error(vic20rom_log, "Couldn't load basic ROM.");
            return -1;
        }
    }
    return vic20rom_basic_checksum();
}

int vic20rom_load_chargen(const char *rom_name)
{
    if (!vicrom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load chargen ROM. */
        if (sysfile_load(rom_name,
                         vic20memrom_chargen_rom, VIC20_CHARGEN_ROM_SIZE,
                         VIC20_CHARGEN_ROM_SIZE) < 0) {
            log_error(vic20rom_log, "Couldn't load character ROM.");
            return -1;
        }
    }
    return 0;
}

int mem_load(void)
{
    const char *rom_name = NULL;

    if (vic20rom_log == LOG_ERR) {
        vic20rom_log = log_open("VIC20MEM");
    }

    vicrom_loaded = 1;

    if (resources_get_string("KernalName", &rom_name) < 0) {
        return -1;
    }
    if (vic20rom_load_kernal(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("BasicName", &rom_name) < 0) {
        return -1;
    }
    if (vic20rom_load_basic(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("ChargenName", &rom_name) < 0) {
        return -1;
    }
    if (vic20rom_load_chargen(rom_name) < 0) {
        return -1;
    }

    /* patch the kernal respecting the video mode */
    mem_patch_kernal();

    return 0;
}

/************************************************************************/

/* This is a light version of C64's patchrom to change between PAL and
   NTSC kernal
    0: kernal ROM 901486-07 (VIC20 PAL)
    1: kernal ROM 901486-06 (VIC20 NTSC)
*/
#define PATCH_VERSIONS 1

int mem_patch_kernal(void)
{
    static unsigned short const patch_bytes[] = {
        1, 0xE475,
            0xe8,
            0x41,

        2, 0xEDE4,
            0x0c, 0x26,
            0x05, 0x19,

        6, 0xFE3F,
            0x26, 0x8d, 0x24, 0x91, 0xa9, 0x48,
            0x89, 0x8d, 0x24, 0x91, 0xa9, 0x42,

        21, 0xFF5C,
            0xe6, 0x2a, 0x78, 0x1c, 0x49, 0x13, 0xb1, 0x0f,
                0x0a, 0x0e, 0xd3, 0x06, 0x38, 0x03, 0x6a, 0x01,
                0xd0, 0x00, 0x83, 0x00, 0x36,
            0x92, 0x27, 0x40, 0x1a, 0xc6, 0x11, 0x74, 0x0e,
                0xee, 0x0c, 0x45, 0x06, 0xf1, 0x02, 0x46, 0x01,
                0xb8, 0x00, 0x71, 0x00, 0x2a,

        0, 00
    };

    int rev, video_mode;
    short bytes, n, i = 0;
    WORD a;

    if (!vicrom_loaded) {
        return 0;
    }

    if (vic20rom_kernal_checksum() < 0) {
        log_message(LOG_ERR, "VIC20MEM: unknown kernal, cannot patch kernal.");
        return -1;
    }

    resources_get_int("MachineVideoStandard", &video_mode);

    switch (video_mode) {
        case MACHINE_SYNC_PAL:
            rev = 0; /* use kernal 901486-07 */
            break;
        case MACHINE_SYNC_NTSC:
            rev = 1; /* use kernal 901486-06 */
            break;
        default:
            log_message(LOG_ERR, "VIC20MEM: unknown sync, cannot patch kernal.");
            return -1;
    }

    while ((bytes = patch_bytes[i++]) > 0) {
        a = (WORD)patch_bytes[i++];

        i += (bytes * rev); /* select patch */
        for (n = bytes; n--; ) {
            vic20memrom_trap_store(a, (BYTE)patch_bytes[i]);
            rom_store(a++, (BYTE)patch_bytes[i++]);
        }

        i += (bytes * (PATCH_VERSIONS - rev));  /* skip patch */
    }

    log_message(LOG_DEFAULT, "VIC20 kernal patched to 901486-0%d.", 7 - rev);

    return 0;
}
