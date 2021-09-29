/** \file   flash-trap.c
 *
 * @author  Daniel Kahlin <daniel@kahlin.net>
 *
 * Based on code from serial by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 */

/*
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

#ifndef C64DTV
#define C64DTV
#endif

#include <stdio.h>
#include <string.h>

#include "archdep.h"
#include "cmdline.h"
#include "fileio.h"
#include "flash-trap.h"
#include "lib.h"
#include "log.h"
#include "maincpu.h"
#include "mem.h"
#include "traps.h"
#include "types.h"
#include "util.h"
#include "resources.h"

/* Flag: Have traps been installed?  */
static int traps_installed = 0;

/* Pointer to list of traps we are using.  */
static const trap_t *flash_traps;

/* Logging goes here.  */
static log_t flash_log = LOG_ERR;

static fileio_info_t *fi = NULL;

static char *flash_trap_fsflashdir = NULL;

static int flash_trap_trueflashfs;

/* ------------------------------------------------------------------------- */

static int flash_install_traps(void)
{
    if (!traps_installed && flash_traps != NULL) {
        const trap_t *p;

        for (p = flash_traps; p->func != NULL; p++) {
            traps_add(p);
        }
        traps_installed = 1;
    }
    return 0;
}

static int flash_remove_traps(void)
{
    if (traps_installed && flash_traps != NULL) {
        const trap_t *p;

        for (p = flash_traps; p->func != NULL; p++) {
            traps_remove(p);
        }
        traps_installed = 0;
    }
    return 0;
}

int flash_trap_init(const trap_t *trap_list)
{
    flash_log = log_open("FlashTrap");

    /* Remove installed traps, if any.  */
    flash_remove_traps();

    /* Install specified traps.  */
    flash_traps = trap_list;
    flash_install_traps();

    return 0;
}

void flash_trap_shutdown(void)
{
    if (fi) {
        fileio_close(fi);
        fi = NULL;
    }
}

/* ------------------------------------------------------------------------- */

static enum { ST_ENTRY=0, ST_END, ST_EMPTY } seek_state = ST_EMPTY;
static char name[256];
static int name_len = 0;
static uint32_t load_addr = 0;

static void read_name_from_mem(void)
{
    int i;
    uint16_t fname;
    name_len = mem_read(0xB7);
    fname = mem_read(0xBB) | (mem_read(0xBC) << 8);
    for (i = 0; i < name_len; i++) {
        name[i] = mem_read((uint16_t)(fname + i));
    }
    name[i] = 0x00;
}


/* Flash seek next routine replacement.
   Create valid directory entry at $0100
 */
int flash_trap_seek_next(void)
{
    unsigned int i;
    uint8_t direntry[0x20];
    uint32_t entry;

    /* bail out if true fs, not emulated */
    if (flash_trap_trueflashfs) {
        return 0;
    }

    /* if we are reading the very first entry in the flash, do
       initialization stuff. */
    entry = mem_read(0xF8) | (mem_read(0xF9) << 8) | (mem_read(0xFA) << 16);
    if (entry == 0x010000) {
        read_name_from_mem();

        if (name_len == 0) {
            /* the missing filename detection of the original kernal
               requires at least one valid entry to work. */
            name_len = 5;
            memcpy(name, "DUMMY", 5);
            seek_state = ST_ENTRY;
        } else {
            char *path = flash_trap_fsflashdir;
            if (!strlen(path)) {
                path = NULL;
            }

            /* open file */
            if (fi) {
                fileio_close(fi);
            }
            fi = fileio_open(name, path, FILEIO_FORMAT_RAW, FILEIO_COMMAND_READ, FILEIO_TYPE_ANY, NULL);
            if (fi) {
                uint8_t addr[2];
                fileio_read(fi, addr, 2);
                load_addr = addr[0] | (addr[1] << 8);
                seek_state = ST_ENTRY;
            } else {
                seek_state = ST_END;
            }
        }
    }

    switch (seek_state) {
        case ST_ENTRY:
            memset(direntry, 0x00, sizeof(direntry));
            /* copy the actual searched name to force a match */
            memcpy(direntry, name, name_len);

            /* flash_address */
            direntry[0x18] = 0x11;
            direntry[0x19] = 0x10;
            direntry[0x1A] = 0x02;

            /* load_address */
            direntry[0x1B] = (uint8_t)(load_addr & 0xff);
            direntry[0x1C] = (uint8_t)((load_addr >> 8) & 0xff);
            direntry[0x1D] = (uint8_t)((load_addr >> 16) & 0xff);

            /* sys_address (non-standard) */
            direntry[0x1E] = 0x00;
            direntry[0x1F] = 0x00;

            seek_state = ST_END;
            break;

        case ST_END:
            memset(direntry, 0x00, sizeof(direntry));
            seek_state = ST_EMPTY;
            break;

        default:
        case ST_EMPTY:
            memset(direntry, 0xFF, sizeof(direntry));
            break;
    }


    /* write out directory entry to the buffer at $0100-$011F */
    for (i = 0; i < sizeof(direntry); i++) {
        mem_store((uint16_t)(0x0100 + i), direntry[i]);
    }

    return 1;
}

/* Flash load body routine replacement.  */
int flash_trap_load_body(void)
{
    uint32_t addr;
    uint8_t laddr, maddr, haddr;

    /* bail out if true fs, not emulated */
    if (flash_trap_trueflashfs) {
        return 0;
    }

    /* start address */
    addr = mem_read(0xFB) | (mem_read(0xFC) << 8) | (mem_read(0xFD) << 16);

    /* load */
    if (fi) {
        uint8_t b;
        while (fileio_read(fi, &b, 1)) {
            mem_ram[addr & 0x1FFFFF] = b;
            addr++;
        }
        fileio_close(fi);
        fi = NULL;
    }

    /* set exit values for success and return */
    laddr = (uint8_t)(addr & 0xff);
    maddr = (uint8_t)((addr >> 8) & 0xff);
    haddr = (uint8_t)((addr >> 16) & 0xff);
    mem_store((uint16_t)0xFB, laddr);
    mem_store((uint16_t)0xFC, maddr);
    mem_store((uint16_t)0xFD, haddr);
    maincpu_set_x(laddr);
    maincpu_set_y(maddr);
    mem_store((uint16_t)0xAE, laddr);
    mem_store((uint16_t)0xAF, maddr);

    return 1;
}


void flash_traps_reset(void)
{
    seek_state = ST_EMPTY;
    name_len = 0;
    load_addr = 0;
}

/* ------------------------------------------------------------------------- */

static int set_flash_trap_fsflashdir(const char *dirname, void *param)
{
    util_string_set(&flash_trap_fsflashdir, dirname);

    return 0;
}

static int set_flash_trap_trueflashfs(int val, void *param)
{

    flash_trap_trueflashfs = val ? 1 : 0;

    return 0;
}

static const resource_string_t resources_string[] = {
    { "FSFlashDir", FSDEVICE_DEFAULT_DIR, RES_EVENT_NO, NULL,
      &flash_trap_fsflashdir, set_flash_trap_fsflashdir, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "FlashTrueFS", 1, RES_EVENT_SAME, NULL,
      &flash_trap_trueflashfs, set_flash_trap_trueflashfs, NULL },
    RESOURCE_INT_LIST_END
};

int flash_trap_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void flash_trap_resources_shutdown(void)
{
    lib_free(flash_trap_fsflashdir);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-fsflash", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "FSFlashDir", NULL,
      "<Name>", "Use <name> as directory for flash file system device" },
    { "-trueflashfs", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "FlashTrueFS", (void *)1,
      NULL, "Enable true hardware flash file system" },
    { "+trueflashfs", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "FlashTrueFS", (void *)0,
      NULL, "Disable true hardware flash file system" },
    CMDLINE_LIST_END
};

int flash_trap_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
