/*
 * c64dtvflash.c -- C64DTV flash memory emulation.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Daniel Kahlin <daniel@kahlin.net>
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

#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "c64dtvmem.h"
#include "c64memrom.h"
#include "c64mem.h"
#include "cmdline.h"
#include "debug.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "uiapi.h"
#include "util.h"
#include "sysfile.h"
#include "resources.h"
#include "translate.h"

#ifndef AMIGA_SUPPORT
#define DTVROM_NAME_DEFAULT   "dtvrom.bin"
#else
#define DTVROM_NAME_DEFAULT   "PROGDIR:C64DTV/dtvrom.bin"
#endif

#ifdef DEBUG
static log_t c64dtvflash_log = LOG_ERR;
static int flash_log_enabled = 0;
#endif

#define C64_ROM_SIZE 0x200000

/* Filenames of C64DTV RAM/ROM */
static char *c64dtvflash_filename = NULL;

/* (Flash)ROM array */
BYTE c64dtvflash_mem[C64_ROM_SIZE];

/* (Flash)ROM state */
enum {
    FLASH_IDLE=0,
    FLASH_CMD1, FLASH_CMD2, FLASH_CMD3, FLASH_CMD4, FLASH_CMD5,
    FLASH_PRODUCTID,
    FLASH_PROGRAM,
    FLASH_SETCONF,
    FLASH_PROGPROT,
    FLASH_SPPROGRAM
} c64dtvflash_state = FLASH_IDLE;

/* (Flash)ROM sector lockdown */
BYTE c64dtvflash_mem_lock[39];

/* (Flash)ROM image write */
int c64dtvflash_mem_rw = 0;

static int paddr_to_sector(int paddr)
{
    if ((paddr >> 16) == 0x1f) {
        return (((paddr >> 13) & 7) + 31);
    } else {
        return (paddr >> 16);
    }
}

void c64dtvflash_store_direct(int addr, BYTE value)
{
    c64dtvflash_mem[addr] = value;
}

BYTE c64dtvflash_read_direct(int addr)
{
    return c64dtvflash_mem[addr];
}

void c64dtvflash_store(int addr, BYTE value)
{
    int i, j, k;
#ifdef DEBUG
    if (flash_log_enabled) {
        log_message(c64dtvflash_log, "flash_store: addr %x, value %x, mode %i\n", addr, value, c64dtvflash_state);
    }
#endif
    switch (c64dtvflash_state) {
        case FLASH_IDLE:
            if (((addr & 0xffe) == 0xaaa) && (value == 0xaa)) {
                c64dtvflash_state = FLASH_CMD1;
            }
            return;
        case FLASH_CMD1:
            if (((addr & 0xffe) == 0x554) && (value == 0x55)) {
                c64dtvflash_state = FLASH_CMD2;
            } else {
                c64dtvflash_state = FLASH_IDLE;
            }
            return;
        case FLASH_CMD2:
            if ((addr & 0xffe) == 0xaaa) {
                switch (value) {
                    case 0x90:
                        c64dtvflash_state = FLASH_PRODUCTID;   /* Product ID Entry */
                        return; 
                    case 0xf0:
                        c64dtvflash_state = FLASH_IDLE;        /* Product ID Exit */
                        return;
                    case 0x80:
                        c64dtvflash_state = FLASH_CMD3;        /* Erase/Single Pulse Program/Lockdown */
                        return;
                    case 0xa0:
                        c64dtvflash_state = FLASH_PROGRAM;     /* Byte/Word Program */
                        return;
                    case 0xd0:
                        c64dtvflash_state = FLASH_SETCONF;     /* Set Configuration Register */
                        return;
                    case 0xc0:
                        c64dtvflash_state = FLASH_PROGPROT;    /* Program/Lock Protection Register */
                        return;
                    default:
                        c64dtvflash_state = FLASH_IDLE;
                        return;
                }
            } else {
                c64dtvflash_state = FLASH_IDLE;
            }
            return;
        case FLASH_PRODUCTID: /* Product ID Mode */
            if (value == 0xf0) {
                c64dtvflash_state = FLASH_IDLE;                   /* Product ID Exit */
            }
            return;
        case FLASH_CMD3: /* Erase/Single Pulse Program/Lockdown */
            if (((addr & 0xffe) == 0xaaa) && (value == 0xaa)) {
                c64dtvflash_state = FLASH_CMD4;
            } else {
                c64dtvflash_state = FLASH_IDLE;
            }
            return;
        case FLASH_CMD4: /* Erase/Single Pulse Program/Lockdown */
            if (((addr & 0xffe) == 0x554) && (value == 0x55)) {
                c64dtvflash_state = FLASH_CMD5;
            } else {
                c64dtvflash_state = FLASH_IDLE;
            }
            return;
        case FLASH_CMD5: /* Erase/Single Pulse Program/Lockdown */
            switch (value) {
                case 0x30: /* Sector Erase */
                    if ((addr >> 16) == 0x1f) {
                        j = (addr & 0x1fe000);
                        k = j + 0x2000;
                    } else {
                        j = (addr & 0x1f0000);
                        k = j + 0x10000;
                    }
                    if (c64dtvflash_mem_lock[paddr_to_sector(addr)]) {
#ifdef DEBUG
                        if (flash_log_enabled) {
                            log_message(c64dtvflash_log, "flash: ignoring erase (locked) %06x-%06x\n", j, k);
                        }
#endif
                    } else {
                        for (i = j; i < k; i++) {
                            c64dtvflash_mem[i] = 0xff;
                        }
#ifdef DEBUG
                        if (flash_log_enabled) {
                            log_message(c64dtvflash_log, "flash: erased %06x-%06x\n", j, k);
                        }
#endif
                    }
                    break;
                case 0x10: /* Chip Erase */
                    for (i = 0; i < 0x200000; i++) {
                        if (!(c64dtvflash_mem_lock[paddr_to_sector(addr)])) {
                            c64dtvflash_mem[i] = 0xff;
                        }
                    }
#ifdef DEBUG
                    if (flash_log_enabled) {
                        log_message(c64dtvflash_log, "flash: chip erased\n");
                    }
#endif
                    break;
                case 0x60: /* Sector Lockdown */
                    c64dtvflash_mem_lock[paddr_to_sector(addr)] = 0xff;
#ifdef DEBUG
                    if (flash_log_enabled) {
                        log_message(c64dtvflash_log, "flash: sector %i lockdown\n", paddr_to_sector(addr));
                    }
#endif
                    break;
                case 0xa0: /* Single Pulse Program Mode */
                    c64dtvflash_state = FLASH_SPPROGRAM;
#ifdef DEBUG
                    if (flash_log_enabled) {
                        log_message(c64dtvflash_log, "flash: entering single pulse program mode\n");
                    }
#endif
                    return;
            }
            c64dtvflash_state = FLASH_IDLE;
            return;
        case FLASH_PROGRAM: /* Byte/Word Program */
            if (c64dtvflash_mem_lock[paddr_to_sector(addr)]) {
#ifdef DEBUG
                if (flash_log_enabled) {
                    log_message(c64dtvflash_log, "flash: ignoring byte program (locked) %02x to %06x\n", value, addr);
                }
#endif
            } else {
                c64dtvflash_mem[addr] &= value;
#ifdef DEBUG
                if (flash_log_enabled) {
                    log_message(c64dtvflash_log, "flash: written %02x to %06x\n", c64dtvflash_mem[addr], addr);                    /* DEBUG */
                }
#endif
            }
            c64dtvflash_state = FLASH_IDLE;
            return;
        case FLASH_SETCONF: /* Set Configuration Register */
            c64dtvflash_state = FLASH_IDLE;
#ifdef DEBUG
            if (flash_log_enabled) {
                log_message(c64dtvflash_log, "flash: set configuration register %02x (unimplemented)\n", value);
            }
#endif
            return;
        case FLASH_PROGPROT: /* Program/Lock Protection Register */
            if ((addr == 0x100) && ((value & 0xf) == 0)) {
#ifdef DEBUG
                if (flash_log_enabled) {
                    log_message(c64dtvflash_log, "flash: lock protection register (unimplemented)\n");
                }
#endif
            } else {
#ifdef DEBUG
                if (flash_log_enabled) {
                    log_message(c64dtvflash_log, "flash: program protection register %x = %02x (unimplemented)\n", addr, value);
                }
#endif
            }
            c64dtvflash_state = FLASH_IDLE;
            return;
        case FLASH_SPPROGRAM: /* Single Pulse Program Mode */
            if (!(c64dtvflash_mem_lock[paddr_to_sector(addr)])) {
                c64dtvflash_mem[addr] &= value;
            }
            return;
#ifdef DEBUG
        default:
            log_message(c64dtvflash_log, "BUG: Unknown flash chip emulation state.");
#endif
    }
}

BYTE c64dtvflash_read(int addr)
{
    if (c64dtvflash_state != FLASH_IDLE) {
#ifdef DEBUG
        if (flash_log_enabled) {
            log_message(c64dtvflash_log, "flash_read: addr %x, mode %i\n", addr, c64dtvflash_state);
        }
#endif
    }
    if (c64dtvflash_state == FLASH_PRODUCTID) { /* Product ID Mode */
        switch (addr) {
            /* Product ID: AT4XBV16XT */
            case 0:
            case 1:
                return 0x1f; /* Manufacturer */
            case 2:
            case 3:
                return 0xc2; /* Device */
            case 6:
            case 7:
                return 0x08; /* Additional Device */
            case 0x100:
            case 0x101:
                return 0xfe; /* Protection Register Lock (unlocked) TODO: configurable */
            /* Protection Register Block A (unique ID) */
            case 0x102:
                return 'x';
            case 0x103:
                return '6';
            case 0x104:
                return '4';
            case 0x105:
                return 'd';
            case 0x106:
                return 't';
            case 0x107:
                return 'v';
            case 0x108:
                return '-';
            case 0x109:
                return 0x10;
            /* Protection Register Block B TODO: configurable */
            case 0x10a:
                return 0xff;
            case 0x10b:
                return 0xff;
            case 0x10c:
                return 0xff;
            case 0x10d:
                return 0xff;
            case 0x10e:
                return 0xff;
            case 0x10f:
                return 0xff;
            case 0x110:
                return 0xff;
            case 0x111:
                return 0xff;
            default:
                if ((addr & ((addr >> 16) == 0x1f ? 0x1fff : 0xffff)) == 4) {
                    return c64dtvflash_mem_lock[paddr_to_sector(addr)]; /* Sector Lockdown */
                } else {
                    return 0xff;
                }
        }
    } else {
        return c64dtvflash_mem[addr];
    }
}

/* ------------------------------------------------------------------------- */

static BYTE buf[0x10000];


void c64dtvflash_create_blank_image(char *filename, int copyroms)
{
    FILE *fd;
    size_t r;
    int i, max = 0x20;

    if (util_check_null_string(filename)) {
#ifdef DEBUG
        log_message(c64dtvflash_log, "No file name given for create_blank_image.");
#endif
        ui_error(translate_text(IDGS_NO_FILENAME));
        return;
    }

    if (util_check_filename_access(filename) < 0) {
#ifdef DEBUG
        log_message(c64dtvflash_log, "Illegal filename in create_blank_image.");
#endif
        ui_error(translate_text(IDGS_ILLEGAL_FILENAME));
        return;
    }

    memset(buf, 0xff, (size_t)0x10000);

    if (copyroms) {
        memcpy(buf + 0xe000, c64dtvflash_mem + 0xe000, C64_KERNAL_ROM_SIZE);
        memcpy(buf + 0xa000, c64dtvflash_mem + 0xa000, C64_BASIC_ROM_SIZE);
        memcpy(buf + 0x1000, c64dtvflash_mem + 0x1000, C64_CHARGEN_ROM_SIZE);
        memcpy(buf + 0x9000, c64dtvflash_mem + 0x9000, C64_CHARGEN_ROM_SIZE);
        memcpy(buf + 0xd000, c64dtvflash_mem + 0xd000, C64_CHARGEN_ROM_SIZE);
    }

    fd = fopen(filename, MODE_WRITE);

    if (fd == NULL) {
#ifdef DEBUG
        log_message(c64dtvflash_log, "Error creating file %s in create_blank_image.", filename);
#endif
        ui_error(translate_text(IDGS_ERROR_CREATING_FILE_S), filename);
        return;
    }

    for (i = 0; i < max; ++i) {
        r = fwrite(buf, 0x10000, 1, fd);
        if (r < 1) {
#ifdef DEBUG
            log_message(c64dtvflash_log, "Error while writing to file %s in create_blank_image.", filename);
#endif
            ui_error(translate_text(IDGS_ERROR_WRITING_TO_FILE_S), filename);
            fclose(fd);
            return;
        }
        if ((i == 1) && copyroms) {
            memset(buf, 0xff, (size_t)0x10000);
        }
    }

    ui_message(translate_text(IDGS_DTV_ROM_CREATED));

    fclose(fd);

    return;
}

/* ------------------------------------------------------------------------- */

unsigned int c64dtvflash_rom_loaded = 0;

static int c64dtvflash_load_rom(void)
{
    int retval = 0;     /* need to change this when ui gets changed for error indication */
#ifdef DEBUG
    if (flash_log_enabled) {
        log_message(c64dtvflash_log, "loading ROM");
    }
#endif
    if (!util_check_null_string(c64dtvflash_filename)) {
        if ((retval = util_file_load(c64dtvflash_filename, c64dtvflash_mem, (size_t)0x200000, UTIL_FILE_LOAD_RAW)) < 0) {
#ifdef DEBUG
            log_message(c64dtvflash_log, "Reading C64DTV ROM image %s failed.", c64dtvflash_filename);
#endif
            retval = -1;
        } else {
#ifdef DEBUG
            log_message(c64dtvflash_log, "Read C64DTV ROM image %s.", c64dtvflash_filename);
#endif
        }
    } else {
#ifdef DEBUG
        log_message(c64dtvflash_log, "No C64DTV ROM image filename specified.");
#endif
        retval = -2;
    }


    /* copy ROMs to Flash ROM emulation if no image file specified */
    if (retval) {
#ifdef DEBUG
        if (flash_log_enabled) {
            log_message(c64dtvflash_log, "copy ROMs to Flash");
        }
#endif
        memcpy(c64dtvflash_mem + 0xe000, c64memrom_kernal64_rom,
               C64_KERNAL_ROM_SIZE);
        memcpy(c64dtvflash_mem + 0xa000, c64memrom_basic64_rom,
               C64_BASIC_ROM_SIZE);
        memcpy(c64dtvflash_mem + 0x1000, mem_chargen_rom,
               C64_CHARGEN_ROM_SIZE);
        memcpy(c64dtvflash_mem + 0x9000, mem_chargen_rom,
               C64_CHARGEN_ROM_SIZE);
        memcpy(c64dtvflash_mem + 0xd000, mem_chargen_rom,
               C64_CHARGEN_ROM_SIZE);
    }
    c64dtvflash_rom_loaded = retval;

    return retval;
}

void c64dtvflash_init(void)
{
#ifdef DEBUG
    if (c64dtvflash_log == LOG_ERR) {
        c64dtvflash_log = log_open("C64DTVFLASH");
    }
#endif

    c64dtvflash_load_rom();

#ifdef DEBUG
    if (flash_log_enabled) {
        log_message(c64dtvflash_log, "END init");
    }
#endif
}

void c64dtvflash_shutdown(void)
{
    if (!util_check_null_string(c64dtvflash_filename)) {
        if (c64dtvflash_mem_rw) {
            if (util_file_save(c64dtvflash_filename, c64dtvflash_mem, 0x200000) < 0) {
#ifdef DEBUG
                log_message(c64dtvflash_log, "Writing C64DTV ROM image %s failed.", c64dtvflash_filename);
#endif
            } else {
#ifdef DEBUG
                log_message(c64dtvflash_log, "Wrote C64DTV ROM image %s.", c64dtvflash_filename);
#endif
            }
        }
    }
#ifdef DEBUG
    if (flash_log_enabled) {
        log_message(c64dtvflash_log, "END shutdown");
    }
#endif
    return;
}

void c64dtvflash_reset(void)
{
    int i;
    c64dtvflash_state = FLASH_IDLE;
    for (i = 0; i < 39; i++) {
        c64dtvflash_mem_lock[i] = 0;
    }
}

/* ------------------------------------------------------------------------- */

static int set_c64dtvflash_filename(const char *name, void *param)
{
    int retval = 0;

#ifndef AMIGA_SUPPORT
    char *complete_path = NULL;
#endif

    if (c64dtvflash_filename != NULL && name != NULL && strcmp(name, c64dtvflash_filename) == 0) {
        return 0;
    }

    if (c64dtvflash_mem_rw && c64dtvflash_filename != NULL && *c64dtvflash_filename != '\0') {
        if (util_file_save(c64dtvflash_filename, c64dtvflash_mem, 0x200000) < 0) {
#ifdef DEBUG
            log_message(c64dtvflash_log, "Writing C64DTV ROM image %s failed.", c64dtvflash_filename);
#endif
        } else {
#ifdef DEBUG
            log_message(c64dtvflash_log, "Wrote C64DTV ROM image %s.", c64dtvflash_filename);
#endif
        }
    }

#ifndef AMIGA_SUPPORT
    /* check if the given rom file can be found in a sys dir and set resource with absolute path */
    if (name != NULL && *name != '\0' && !util_file_exists(name)) {
        sysfile_locate(name, &complete_path);
        if (complete_path != NULL) {
            name = complete_path;
        }
    }
#endif

    util_string_set(&c64dtvflash_filename, name);

#ifndef AMIGA_SUPPORT
    lib_free(complete_path);
#endif

    if (c64dtvflash_filename != NULL && *c64dtvflash_filename != '\0') {
        retval = c64dtvflash_load_rom();
    }

    /* for now always reset machine, later can become optional */
    if (!retval) {
        machine_trigger_reset(MACHINE_RESET_MODE_HARD);
    }

    return 0;
}

static int set_c64dtvflash_mem_rw(int val, void *param)
{
    c64dtvflash_mem_rw = val ? 1 : 0;

    return 0;
}

#ifdef DEBUG
static int set_flash_log(int val, void *param)
{
    flash_log_enabled = val ? 1 : 0;

    return 0;
}
#endif

static const resource_string_t resources_string[] = {
    { "c64dtvromfilename", DTVROM_NAME_DEFAULT, RES_EVENT_NO, NULL,
      &c64dtvflash_filename, set_c64dtvflash_filename, NULL },
    { NULL }
};

static const resource_int_t resources_int[] = {
    { "c64dtvromrw", 0, RES_EVENT_SAME, NULL,
      &c64dtvflash_mem_rw, set_c64dtvflash_mem_rw, NULL },
#ifdef DEBUG
    { "DtvFlashLog", 0, RES_EVENT_NO, (resource_value_t)0,
      &flash_log_enabled, set_flash_log, NULL },
#endif
    { NULL }
};

int c64dtvflash_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void c64dtvflash_resources_shutdown(void)
{
    lib_free(c64dtvflash_filename);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-c64dtvromimage", SET_RESOURCE, 1,
      NULL, NULL, "c64dtvromfilename", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_C64DTVROM_NAME,
      NULL, NULL },
    { "-c64dtvromrw", SET_RESOURCE, 0,
      NULL, NULL, "c64dtvromrw", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_C64DTVROM_RW,
      NULL, NULL },
    { "+c64dtvromrw", SET_RESOURCE, 0,
      NULL, NULL, "c64dtvromrw", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_C64DTVROM_RW,
      NULL, NULL },
#ifdef DEBUG
    { "-dtvflashlog", SET_RESOURCE, 0,
      NULL, NULL, "DtvFlashLog", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_DTV_FLASH_LOG,
      NULL, NULL },
    { "+dtvflashlog", SET_RESOURCE, 0,
      NULL, NULL, "DtvFlashLog", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_DTV_FLASH_LOG,
      NULL, NULL },
#endif
    { NULL }
};

int c64dtvflash_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
