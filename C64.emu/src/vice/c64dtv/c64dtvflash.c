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
#include "c64dtvmodel.h"
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

#include "c64dtvflash.h"

/* #define DEBUG */

#define DTVROM_PALV2_NAME_DEFAULT   "dtvrom.bin" /* FIXME: rename (who made this?) */
#define DTVROM_PALV3_NAME_DEFAULT   "dtvrom.bin" /* FIXME: rename (who made this?) */
#define DTVROM_NTSCV2_NAME_DEFAULT  "dtvrom.bin" /* FIXME: rename (who made this?) */
#define DTVROM_NTSCV3_NAME_DEFAULT  "dtvrom.bin" /* FIXME: rename (who made this?) */
#define DTVROM_HUMMER_NAME_DEFAULT  "dtvrom.bin" /* FIXME: rename (who made this?) */

static log_t c64dtvflash_log = LOG_ERR;
#ifdef DEBUG
static int flash_log_enabled = 0;
#endif

#define C64_ROM_SIZE 0x200000

/* Filenames of C64DTV RAM/ROM */
static char *c64dtvflash_filename[DTVMODEL_NUM] = { NULL, NULL, NULL, NULL, NULL };

static int c64dtvflash_revision = 0;

/* (Flash)ROM array */
uint8_t c64dtvflash_mem[C64_ROM_SIZE];

/* (Flash)ROM state */
enum {
    FLASH_IDLE=0,
    FLASH_CMD1, FLASH_CMD2, FLASH_CMD3, FLASH_CMD4, FLASH_CMD5,
    FLASH_PRODUCTID,
    FLASH_PROGRAM,
    FLASH_SETCONF,
    FLASH_PROGPROT,
    FLASH_SPPROGRAM
};

uint8_t c64dtvflash_state = (uint8_t)FLASH_IDLE;

/* (Flash)ROM sector lockdown */
uint8_t c64dtvflash_mem_lock[39];

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

void c64dtvflash_store_direct(int addr, uint8_t value)
{
    c64dtvflash_mem[addr] = value;
}

uint8_t c64dtvflash_read_direct(int addr)
{
    return c64dtvflash_mem[addr];
}

void c64dtvflash_store(int addr, uint8_t value)
{
    int i, j, k;
#ifdef DEBUG
    if (flash_log_enabled) {
        log_message(c64dtvflash_log,
                "flash_store: addr %x, value %x, mode %i\n",
                (unsigned int)addr, value, c64dtvflash_state);

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
                            log_message(c64dtvflash_log,
                                    "flash: ignoring erase (locked) %06x-%06x\n",
                                    (unsigned int)j, (unsigned int)k);
                        }
#endif
                    } else {
                        for (i = j; i < k; i++) {
                            c64dtvflash_mem[i] = 0xff;
                        }
#ifdef DEBUG
                        if (flash_log_enabled) {
                            log_message(c64dtvflash_log,
                                    "flash: erased %06x-%06x\n",
                                    (unsigned int)j, (unsigned int)k);
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
                    log_message(c64dtvflash_log,
                            "flash: ignoring byte program (locked) %02x to %06x\n",
                            value, (unsigned int)addr);
                }
#endif
            } else {
                c64dtvflash_mem[addr] &= value;
#ifdef DEBUG
                if (flash_log_enabled) {
                    log_message(c64dtvflash_log,
                            "flash: written %02x to %06x\n",
                            c64dtvflash_mem[addr], (unsigned int)addr);                    /* what is this? -> DEBUG */
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
                    log_message(c64dtvflash_log,
                            "flash: program protection register %x = %02x (unimplemented)\n",
                            (unsigned int)addr, value);
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

uint8_t c64dtvflash_read(int addr)
{
    if (c64dtvflash_state != FLASH_IDLE) {
#ifdef DEBUG
        if (flash_log_enabled) {
            log_message(c64dtvflash_log,
                    "flash_read: addr %x, mode %i\n",
                    (unsigned int)addr, c64dtvflash_state);
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

static uint8_t buf[0x10000];


void c64dtvflash_create_blank_image(char *filename, int copyroms)
{
    FILE *fd;
    size_t r;
    int i, max = 0x20;

    if (util_check_null_string(filename)) {
#ifdef DEBUG
        log_message(c64dtvflash_log, "No file name given for create_blank_image.");
#endif
        ui_error("No filename!");
        return;
    }

    if (util_check_filename_access(filename) < 0) {
#ifdef DEBUG
        log_message(c64dtvflash_log, "Illegal filename in create_blank_image.");
#endif
        ui_error("Illegal filename!");
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
        ui_error("Error creating file %s!", filename);
        return;
    }

    for (i = 0; i < max; ++i) {
        r = fwrite(buf, 0x10000, 1, fd);
        if (r < 1) {
#ifdef DEBUG
            log_message(c64dtvflash_log, "Error while writing to file %s in create_blank_image.", filename);
#endif
            ui_error("Error writing to file %s!", filename);
            fclose(fd);
            return;
        }
        if ((i == 1) && copyroms) {
            memset(buf, 0xff, (size_t)0x10000);
        }
    }

    ui_message("DTV ROM image created successfully");

    fclose(fd);

    return;
}

/* ------------------------------------------------------------------------- */

int c64dtvflash_rom_loaded = 0;

static int c64dtvflash_load_rom(int idx)
{
    int retval = 0;     /* need to change this when ui gets changed for error indication */
    /*int idx = c64dtvflash_revision;*/ /* DTVFlashRevision */

#ifdef DEBUG
    if (flash_log_enabled) {
        log_message(c64dtvflash_log, "loading ROM");
    }
#endif
    if (!util_check_null_string(c64dtvflash_filename[idx])) {
        if ((retval = sysfile_load(c64dtvflash_filename[idx], machine_name, c64dtvflash_mem, 0, (size_t)0x200000)) < 0) {
            log_error(c64dtvflash_log, "Reading C64DTV ROM rev:%d image: '%s' failed. (ret:%d)",
                      idx, c64dtvflash_filename[idx], retval);
            retval = -1;
        } else {
#ifdef DEBUG
            log_message(c64dtvflash_log, "Read C64DTV ROM image:%d '%s'.", idx, c64dtvflash_filename[idx]);
#endif
            retval = 0; /* no error, sysfile_load returns file length */
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

    c64dtvflash_load_rom(c64dtvflash_revision);  /* DTVFlashRevision */

#ifdef DEBUG
    if (flash_log_enabled) {
        log_message(c64dtvflash_log, "END init");
    }
#endif
}

void c64dtvflash_shutdown(void)
{
    int idx = c64dtvflash_revision; /* DTVFlashRevision */
    if (!util_check_null_string(c64dtvflash_filename[idx])) {
        if (c64dtvflash_mem_rw) {
            if (util_file_save(c64dtvflash_filename[idx], c64dtvflash_mem, 0x200000) < 0) {
                log_error(c64dtvflash_log, "Writing C64DTV ROM image %s failed.", c64dtvflash_filename[idx]);
            } else {
                log_message(c64dtvflash_log, "Wrote C64DTV ROM image %s.", c64dtvflash_filename[idx]);
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
static int set_c64dtvflash_revision(int val, void *param)
{
    int oldval = c64dtvflash_revision;
    if (val < 0 || val > (DTVMODEL_NUM - 1)) {
        c64dtvflash_revision = 0;
        return -1;
    } else {
        c64dtvflash_revision = val;
    }
    if (oldval != c64dtvflash_revision) {
        c64dtvflash_load_rom(c64dtvflash_revision /* DTVFlashRevision */);
    }
    return 0;
}

static int set_c64dtvflash_filename(const char *name, void *param)
{
    int retval = 0;
    int idx = vice_ptr_to_int(param);

    char *complete_path = NULL;

    if (c64dtvflash_filename[idx] != NULL && name != NULL && strcmp(name, c64dtvflash_filename[idx]) == 0) {
        return 0;
    }

    if (c64dtvflash_mem_rw && c64dtvflash_filename[idx] != NULL && *c64dtvflash_filename[idx] != '\0') {
        if (util_file_save(c64dtvflash_filename[idx], c64dtvflash_mem, 0x200000) < 0) {
#ifdef DEBUG
            log_message(c64dtvflash_log, "Writing C64DTV ROM image %s failed.", c64dtvflash_filename[idx]);
#endif
        } else {
#ifdef DEBUG
            log_message(c64dtvflash_log, "Wrote C64DTV ROM image %s.", c64dtvflash_filename[idx]);
#endif
        }
    }

#if 0 /* FIXME: why would we want to do this? one drawback is, that this way we always have "non default"
                values in these resources */
    /* check if the given rom file can be found in a sys dir and set resource with absolute path */
    if (name != NULL && *name != '\0' && !util_file_exists(name)) {
        sysfile_locate(name, "C64DTV", &complete_path);
        if (complete_path != NULL) {
            name = complete_path;
        }
    }
#endif

    util_string_set(&c64dtvflash_filename[idx], name);

    lib_free(complete_path);

    if (c64dtvflash_filename[idx] != NULL && *c64dtvflash_filename[idx] != '\0') {
        retval = c64dtvflash_load_rom(idx);
    }

    /* for now always reset machine, later can become optional */
    if (!retval) {
        machine_trigger_reset(MACHINE_RESET_MODE_POWER_CYCLE);
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
    { "DTVNTSCV2FlashName", DTVROM_NTSCV2_NAME_DEFAULT, RES_EVENT_NO, NULL,
      &c64dtvflash_filename[DTVMODEL_V2_NTSC], set_c64dtvflash_filename, (void*)DTVMODEL_V2_NTSC },
    { "DTVPALV2FlashName", DTVROM_PALV2_NAME_DEFAULT, RES_EVENT_NO, NULL,
      &c64dtvflash_filename[DTVMODEL_V2_PAL], set_c64dtvflash_filename, (void*)DTVMODEL_V2_PAL },
    { "DTVNTSCV3FlashName", DTVROM_NTSCV3_NAME_DEFAULT, RES_EVENT_NO, NULL,
      &c64dtvflash_filename[DTVMODEL_V3_NTSC], set_c64dtvflash_filename, (void*)DTVMODEL_V3_NTSC },
    { "DTVPALV3FlashName", DTVROM_PALV3_NAME_DEFAULT, RES_EVENT_NO, NULL,
      &c64dtvflash_filename[DTVMODEL_V3_PAL], set_c64dtvflash_filename, (void*)DTVMODEL_V3_PAL },
    { "DTVHummerFlashName", DTVROM_HUMMER_NAME_DEFAULT, RES_EVENT_NO, NULL,
      &c64dtvflash_filename[DTVMODEL_HUMMER_NTSC], set_c64dtvflash_filename, (void*)DTVMODEL_HUMMER_NTSC },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "DTVFlashRevision", DTVMODEL_V3_PAL, RES_EVENT_NO, NULL,
      &c64dtvflash_revision, set_c64dtvflash_revision, NULL },
    { "c64dtvromrw", 0, RES_EVENT_SAME, NULL,
      &c64dtvflash_mem_rw, set_c64dtvflash_mem_rw, NULL },
#ifdef DEBUG
    { "DtvFlashLog", 0, RES_EVENT_NO, (resource_value_t)0,
      &flash_log_enabled, set_flash_log, NULL },
#endif
    RESOURCE_INT_LIST_END
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
    int idx;
    for (idx = 0; idx < DTVMODEL_NUM; idx++) {
        lib_free(c64dtvflash_filename[idx]);
    }
}

static const cmdline_option_t cmdline_options[] =
{
    { "-ntscv2romimage", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DTVNTSCV2FlashName", NULL,
      "<Name>", "Specify name of C64DTV NTSC v2 ROM image" },
    { "-palv2romimage", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DTVPALV2FlashName", NULL,
      "<Name>", "Specify name of C64DTV PAL v2 ROM image" },
    { "-ntscv3romimage", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DTVNTSCV3FlashName", NULL,
      "<Name>", "Specify name of C64DTV NTSC v3 ROM image" },
    { "-palv3romimage", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DTVPALV3FlashName", NULL,
      "<Name>", "Specify name of C64DTV PAL v3 ROM image" },
    { "-hummerromimage", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DTVHummerFlashName", NULL,
      "<Name>", "Specify name of C64DTV Hummer ROM image" },
    { "-c64dtvromrw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "c64dtvromrw", (void *)1,
      NULL, "Enable writes to C64DTV ROM image" },
    { "+c64dtvromrw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "c64dtvromrw", (void *)0,
      NULL, "Disable writes to C64DTV ROM image" },
#ifdef DEBUG
    { "-dtvflashlog", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DtvFlashLog", (resource_value_t)1,
      NULL, "Enable DTV flash chip logs." },
    { "+dtvflashlog", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DtvFlashLog", (resource_value_t)0,
      NULL, "Disable DTV flash chip logs." },
#endif
    CMDLINE_LIST_END
};

int c64dtvflash_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
