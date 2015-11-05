/*
 * petreu.c - PET RAM and Expansion Unit emulation.
 *
 * Written by
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

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "interrupt.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "petreu.h"
#include "resources.h"
#include "snapshot.h"
#include "translate.h"
#include "types.h"
#include "util.h"

/*
 Offsets of the different PET REU registers
*/
#define PETREU_REGISTER_B       0x00
#define PETREU_REGISTER_A       0x01
#define PETREU_DIRECTION_B      0x02
#define PETREU_DIRECTION_A      0x03
#define PETREU_CONTROL          0x0c

/* PET REU registers */
static BYTE petreu[16];
static BYTE petreu2[16];

static BYTE petreu_bank;

/* PET REU image.  */
static BYTE *petreu_ram = NULL;

/* old PET REU size, unused for now but reserved for
   the future 512kb/1mb/2mb versions */
static int old_petreu_ram_size = 0;

static log_t petreu_log = LOG_ERR;

static int petreu_activate(void);
static int petreu_deactivate(void);

/* ------------------------------------------------------------------------- */

/* Flag: Do we enable the PET REU?  */
int petreu_enabled;

/* PET REU size, unused for now but reserved for
   the future 512kb/1mb/2mb versions */
static int petreu_size = 0;

/* Size of the PET REU in KB.  */
static int petreu_size_kb = 0;

/* Filename of the PET REU image.  */
static char *petreu_filename = NULL;

static int set_petreu_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!val) {
        if (petreu_enabled) {
            if (petreu_deactivate() < 0) {
                return -1;
            }
        }
        petreu_enabled = 0;
        return 0;
    } else {
        if (!petreu_enabled) {
            if (petreu_activate() < 0) {
                return -1;
            }
        }
        petreu_enabled = 1;
        return 0;
    }
}

static int set_petreu_size(int val, void *param)
{
    if (val == petreu_size_kb) {
        return 0;
    }

    switch (val) {
        case 128:
        case 512:
        case 1024:
        case 2048:
            break;
        default:
            log_message(petreu_log, "Unknown PET REU size %d.", val);
            return -1;
    }

    if (petreu_enabled) {
        petreu_deactivate();
        petreu_size_kb = val;
        petreu_size = petreu_size_kb << 10;
        petreu_activate();
    } else {
        petreu_size_kb = val;
        petreu_size = petreu_size_kb << 10;
    }

    return 0;
}

static int set_petreu_filename(const char *name, void *param)
{
    if (petreu_filename != NULL && name != NULL
        && strcmp(name, petreu_filename) == 0) {
        return 0;
    }

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    if (petreu_enabled) {
        petreu_deactivate();
        util_string_set(&petreu_filename, name);
        petreu_activate();
    } else {
        util_string_set(&petreu_filename, name);
    }

    return 0;
}

static const resource_string_t resources_string[] = {
    { "PETREUfilename", "", RES_EVENT_NO, NULL,
      &petreu_filename, set_petreu_filename, NULL },
    { NULL }
};

static const resource_int_t resources_int[] = {
    { "PETREU", 0, RES_EVENT_SAME, NULL,
      &petreu_enabled, set_petreu_enabled, NULL },
    { "PETREUsize", 128, RES_EVENT_SAME, NULL,
      &petreu_size_kb, set_petreu_size, NULL },
    { NULL }
};

int petreu_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void petreu_resources_shutdown(void)
{
    lib_free(petreu_filename);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-petreu", SET_RESOURCE, 0,
      NULL, NULL, "PETREU", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_PETREU,
      NULL, NULL },
    { "+petreu", SET_RESOURCE, 0,
      NULL, NULL, "PETREU", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_PETREU,
      NULL, NULL },
    { "-petreuimage", SET_RESOURCE, 1,
      NULL, NULL, "PETREUfilename", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_PETREU_NAME,
      NULL, NULL },
    { "-petreuramsize", SET_RESOURCE, 1,
      NULL, NULL, "PETREUsize", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_SIZE_IN_KB, IDCLS_PETREU_SIZE,
      NULL, NULL },
    { NULL }
};

int petreu_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

void petreu_init(void)
{
    petreu_log = log_open("PETREU");
}

void petreu_reset(void)
{
    memset(petreu, 0, sizeof(petreu));
    memset(petreu2, 0, sizeof(petreu2));
    petreu_bank = 0;
}

static int petreu_activate(void)
{
    if (!petreu_size) {
        return 0;
    }

    petreu_ram = lib_realloc((void *)petreu_ram, (size_t)petreu_size);

    /* Clear newly allocated RAM.  */
    if (petreu_size > old_petreu_ram_size) {
        memset(petreu_ram, 0, (size_t)(petreu_size - old_petreu_ram_size));
    }

    old_petreu_ram_size = petreu_size;

    log_message(petreu_log, "%dKB unit installed.", petreu_size >> 10);

    if (!util_check_null_string(petreu_filename)) {
        if (util_file_load(petreu_filename, petreu_ram, (size_t)petreu_size,
                           UTIL_FILE_LOAD_RAW) < 0) {
            log_message(petreu_log, "Reading PET REU image %s failed.",
                        petreu_filename);
            if (util_file_save(petreu_filename, petreu_ram, petreu_size) < 0) {
                log_message(petreu_log, "Creating PET REU image %s failed.",
                            petreu_filename);
                return -1;
            }
            log_message(petreu_log, "Creating PET REU image %s.",
                        petreu_filename);
            return 0;
        }
        log_message(petreu_log, "Reading PET REU image %s.", petreu_filename);
    }

    petreu_reset();
    return 0;
}

static int petreu_deactivate(void)
{
    if (petreu_ram == NULL) {
        return 0;
    }

    if (!util_check_null_string(petreu_filename)) {
        if (util_file_save(petreu_filename, petreu_ram, petreu_size) < 0) {
            log_message(petreu_log, "Writing PET REU image %s failed.",
                        petreu_filename);
            return -1;
        }
        log_message(petreu_log, "Writing PET REU image %s.", petreu_filename);
    }

    lib_free(petreu_ram);
    petreu_ram = NULL;
    old_petreu_ram_size = 0;

    return 0;
}

void petreu_shutdown(void)
{
    petreu_deactivate();
}

/* ------------------------------------------------------------------------- */


/* This might be over-simplifying things, returning the
   value without taking timers and interrupts into
   acount, if needed I'll fix this in the future. */
BYTE read_petreu_reg(WORD addr)
{
    BYTE retval;

    retval = petreu[addr & 0xf];

    return retval;
}

BYTE read_petreu2_reg(WORD addr)
{
    BYTE retval;

    if (petreu_size_kb != 128) {
        retval = petreu2[addr & 0xf];
    } else {
        retval = (addr >> 8) & 0xff;
    }

    return retval;
}

/* When direction bits are set to input, the corrosponding
   bits of the latches go high */
static BYTE getrealvalue(BYTE reg, BYTE dir)
{
    BYTE retval;

    retval = reg & dir;
    retval = retval | (~dir);

    return retval;
}

static BYTE get_petreu_ram(WORD addr)
{
    BYTE retval;
    BYTE real_register_b_value;
    BYTE real_register_a_value;

    if (petreu[PETREU_DIRECTION_B] != 0xff
        && petreu[PETREU_DIRECTION_B] != 0x7f) {
        real_register_b_value = getrealvalue(petreu[PETREU_REGISTER_B],
                                             petreu[PETREU_DIRECTION_B]);
    } else {
        real_register_b_value = petreu[PETREU_REGISTER_B];
    }

    if (petreu[PETREU_DIRECTION_A] != 0xff) {
        real_register_a_value = getrealvalue(petreu[PETREU_REGISTER_A],
                                             petreu[PETREU_DIRECTION_A]);
    } else {
        real_register_a_value = petreu[PETREU_REGISTER_A];
    }

    retval = petreu_ram[(petreu_bank << 15) + ((real_register_b_value & 0x7f) << 8) + real_register_a_value];

    return retval;
}

static BYTE get_petreu2_ram(WORD addr)
{
    BYTE retval;
    BYTE real_register_b_value;
    BYTE real_register_a_value;
    BYTE real_bank_value;

    if (petreu[PETREU_DIRECTION_B] != 0xff) {
        real_register_b_value = getrealvalue(petreu[PETREU_REGISTER_B],
                                             petreu[PETREU_DIRECTION_B]);
    } else {
        real_register_b_value = petreu[PETREU_REGISTER_B];
    }

    if (petreu[PETREU_DIRECTION_A] != 0xff) {
        real_register_a_value = getrealvalue(petreu[PETREU_REGISTER_A],
                                             petreu[PETREU_DIRECTION_A]);
    } else {
        real_register_a_value = petreu[PETREU_REGISTER_A];
    }

    if (petreu2[PETREU_DIRECTION_A] != 0xff) {
        real_bank_value = getrealvalue(petreu2[PETREU_REGISTER_A],
                                       petreu2[PETREU_DIRECTION_A]);
    } else {
        real_bank_value = petreu2[PETREU_REGISTER_A];
    }

    real_bank_value = (real_bank_value & ((petreu_size_kb >> 4) - 1));

    retval = petreu_ram[(real_bank_value << 16) + (real_register_b_value << 8) + real_register_a_value];

    return retval;
}

BYTE read_petreu_ram(WORD addr)
{
    if (petreu_size_kb == 128) {
        return get_petreu_ram(addr);
    } else {
        return get_petreu2_ram(addr);
    }
}

void store_petreu_reg(WORD addr, BYTE byte)
{
    petreu[addr & 0xf] = byte;
    if ((petreu[PETREU_CONTROL] & 0xe) != 0xc) {
        petreu_bank = 2;
    } else {
        petreu_bank = 0;
    }
    if ((petreu[PETREU_CONTROL] & 0xe0) != 0xc0) {
        petreu_bank++;
    }
}

void store_petreu2_reg(WORD addr, BYTE byte)
{
    petreu2[addr & 0xf] = byte;
}

static void put_petreu_ram(WORD addr, BYTE byte)
{
    BYTE real_register_b_value;
    BYTE real_register_a_value;

    if (petreu[PETREU_DIRECTION_B] != 0xff
        && petreu[PETREU_DIRECTION_B] != 0x7f) {
        real_register_b_value = getrealvalue(petreu[PETREU_REGISTER_B],
                                             petreu[PETREU_DIRECTION_B]);
    } else {
        real_register_b_value = petreu[PETREU_REGISTER_B];
    }

    if (petreu[PETREU_DIRECTION_A] != 0xff) {
        real_register_a_value = getrealvalue(petreu[PETREU_REGISTER_A],
                                             petreu[PETREU_DIRECTION_A]);
    } else {
        real_register_a_value = petreu[PETREU_REGISTER_A];
    }

    petreu_ram[(petreu_bank << 15) + ((real_register_b_value & 0x7f) << 8)
               + real_register_a_value] = byte;
}

static void put_petreu2_ram(WORD addr, BYTE byte)
{
    BYTE real_register_b_value;
    BYTE real_register_a_value;
    BYTE real_bank_value;

    if (petreu[PETREU_DIRECTION_B] != 0xff) {
        real_register_b_value = getrealvalue(petreu[PETREU_REGISTER_B],
                                             petreu[PETREU_DIRECTION_B]);
    } else {
        real_register_b_value = petreu[PETREU_REGISTER_B];
    }

    if (petreu[PETREU_DIRECTION_A] != 0xff) {
        real_register_a_value = getrealvalue(petreu[PETREU_REGISTER_A],
                                             petreu[PETREU_DIRECTION_A]);
    } else {
        real_register_a_value = petreu[PETREU_REGISTER_A];
    }

    if (petreu2[PETREU_DIRECTION_A] != 0xff) {
        real_bank_value = getrealvalue(petreu2[PETREU_REGISTER_A],
                                       petreu2[PETREU_DIRECTION_A]);
    } else {
        real_bank_value = petreu2[PETREU_REGISTER_A];
    }

    real_bank_value = (real_bank_value & ((petreu_size_kb >> 4) - 1));

    petreu_ram[(real_bank_value << 16) + (real_register_b_value << 8) + real_register_a_value] = byte;
}

void store_petreu_ram(WORD addr, BYTE byte)
{
    if (petreu_size_kb == 128) {
        put_petreu_ram(addr, byte);
    } else {
        put_petreu2_ram(addr, byte);
    }
}
