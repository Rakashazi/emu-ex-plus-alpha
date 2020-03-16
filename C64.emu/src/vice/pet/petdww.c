/*
 * petdww.c - PET Double-W hi-res graphics board emulation
 *
 * Written by
 *  Olaf 'Rhialto' Seibert <rhialto@falu.nl>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
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

#include "cartio.h"
#include "cmdline.h"
#include "crtc.h"
#include "interrupt.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "petdww.h"
#include "petmem.h"
#include "pets.h"
#include "piacore.h"
#include "ram.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"

/*
 * A DWW (Double-W) board consists of a PIA and 8 K RAM.
 *
 * The PIA is at $EBx0, and the RAM may be mapped at $EC00-$EFFF.
 * Therefore, petres.IOSize must be 2K.
 *
 * The RAM may also be mapped at $9000-AFFF under software control,
 * so we need to hook into the memory mapping system.
 */

#define DWW_DEBUG_REG       0
#define DWW_DEBUG_RAM       0
#define DWW_DEBUG_GFX       0

/* hi-res graphics memory image.  */
static uint8_t *petdww_ram = NULL;

static log_t petdww_log = LOG_ERR;

static int petdww_activate(void);
static int petdww_deactivate(void);

#if 0
static void petdwwpia_init(void);
#endif
static void petdwwpia_reset(void);
static void pia_reset(void);
static void init_drawing_tables(void);
static void petdww_DRAW_40(uint8_t *p, int xstart, int xend, int scr_rel, int ymod8);
static void petdww_DRAW_80(uint8_t *p, int xstart, int xend, int scr_rel, int ymod8);
static void petdww_DRAW_blank(uint8_t *p, int xstart, int xend, int scr_rel, int ymod8);

#if 0
static void petdwwpia_signal(int line, int edge);
static uint8_t petdwwpia_peek(uint16_t addr);
#endif
static int petdwwpia_snapshot_write_module(snapshot_t *);
static int petdwwpia_snapshot_read_module(snapshot_t *);


/* ------------------------------------------------------------------------- */

/* Flag: Do we enable the PET DWW?  */
int petdww_enabled = 0;

/* Flag: is the memory mapped at $9000? */
static int mem_at_9000;

#define PET_DWW_RAM_SIZE        8192
#define RAM_SIZE_MASK           (PET_DWW_RAM_SIZE - 1)
#define RAM_1K_SIZE_MASK        0x3FF

/* Filename of the PET DWW image.  */
static char *petdww_filename = NULL;

/* Some prototypes are needed */
static uint8_t read_petdww_reg(uint16_t addr);
static uint8_t read_petdww_ec00_ram(uint16_t addr);
static void store_petdww_reg(uint16_t addr, uint8_t value);
static void store_petdww_ec00_ram(uint16_t addr, uint8_t value);
static int petdww_dump(void);

static io_source_t petdww_reg_device = {
    "PETDWW REG",         /* name of the device */
    IO_DETACH_RESOURCE,   /* use resource to detach the device when involved in a read-collision */
    "PETDWW",             /* resource to set to '0' */
    0xeb00, 0xebff, 0x03, /* range for the device, regs:$eb00-$eb03, mirrors:$eb04-$ebff */
    1,                    /* read is always valid */
    store_petdww_reg,     /* store function */
    NULL,                 /* NO poke function */
    read_petdww_reg,      /* read function */
    NULL,                 /* NO peek function */
    petdww_dump,          /* device state information dump function */
    IO_CART_ID_NONE,      /* not a cartridge */
    IO_PRIO_NORMAL,       /* normal priority, device read needs to be checked for collisions */
    0                     /* insertion order, gets filled in by the registration function */
};

static io_source_t petdww_ram_ec00_device = {
    "PETDWW RAM",          /* name of the device */
    IO_DETACH_RESOURCE,    /* use resource to detach the device when involved in a read-collision */
    "PETDWW",              /* resource to set to '0' */
    0xec00, 0xecff, 0x3ff, /* range for the device, regs:$ec00-$ecff, 0x3ff is used as the mask so we can use 12 bits of address offset */
    1,                     /* read is always valid */
    store_petdww_ec00_ram, /* store function */
    NULL,                  /* NO poke function */
    read_petdww_ec00_ram,  /* read function */
    NULL,                  /* NO peek function */
    petdww_dump,           /* device state information dump function */
    IO_CART_ID_NONE,       /* not a cartridge */
    IO_PRIO_NORMAL,        /* normal priority, device read needs to be checked for collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

static io_source_t petdww_ram_ed00_device = {
    "PETDWW RAM",          /* name of the device */
    IO_DETACH_RESOURCE,    /* use resource to detach the device when involved in a read-collision */
    "PETDWW",              /* resource to set to '0' */
    0xed00, 0xedff, 0x3ff, /* range for the device, regs:$ed00-$edff, 0x3ff is used as the mask so we can use 12 bits of address offset */
    1,                     /* read is always valid */
    store_petdww_ec00_ram, /* store function */
    NULL,                  /* NO poke function */
    read_petdww_ec00_ram,  /* read function */
    NULL,                  /* NO peek function */
    petdww_dump,           /* device state information dump function */
    IO_CART_ID_NONE,       /* not a cartridge */
    IO_PRIO_NORMAL,        /* normal priority, device read needs to be checked for collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

static io_source_t petdww_ram_ee00_device = {
    "PETDWW RAM",          /* name of the device */
    IO_DETACH_RESOURCE,    /* use resource to detach the device when involved in a read-collision */
    "PETDWW",              /* resource to set to '0' */
    0xee00, 0xeeff, 0x3ff, /* range for the device, regs:$ee00-$eeff, 0x3ff is used as the mask so we can use 12 bits of address offset */
    1,                     /* read is always valid */
    store_petdww_ec00_ram, /* store function */
    NULL,                  /* NO poke function */
    read_petdww_ec00_ram,  /* read function */
    NULL,                  /* NO peek function */
    petdww_dump,           /* device state information dump function */
    IO_CART_ID_NONE,       /* not a cartridge */
    IO_PRIO_NORMAL,        /* normal priority, device read needs to be checked for collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

static io_source_t petdww_ram_ef00_device = {
    "PETDWW RAM",          /* name of the device */
    IO_DETACH_RESOURCE,    /* use resource to detach the device when involved in a read-collision */
    "PETDWW",              /* resource to set to '0' */
    0xef00, 0xefff, 0x3ff, /* range for the device, regs:$ef00-$efff, 0x3ff is used as the mask so we can use 12 bits of address offset */
    1,                     /* read is always valid */
    store_petdww_ec00_ram, /* store function */
    NULL,                  /* NO poke function */
    read_petdww_ec00_ram,  /* read function */
    NULL,                  /* NO peek function */
    petdww_dump,           /* device state information dump function */
    IO_CART_ID_NONE,       /* not a cartridge */
    IO_PRIO_NORMAL,        /* normal priority, device read needs to be checked for collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *petdww_reg_list_item = NULL;
static io_source_list_t *petdww_ram_ec00_list_item = NULL;
static io_source_list_t *petdww_ram_ed00_list_item = NULL;
static io_source_list_t *petdww_ram_ee00_list_item = NULL;
static io_source_list_t *petdww_ram_ef00_list_item = NULL;

static int set_petdww_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (petdww_enabled == val) {
        return 0;
    }

    if (!val) {
        if (petdww_enabled) {
            if (petdww_deactivate() < 0) {
                return -1;
            }
        }
        if (petdww_ram_ec00_list_item) {
            io_source_unregister(petdww_ram_ec00_list_item);
            io_source_unregister(petdww_ram_ed00_list_item);
            io_source_unregister(petdww_ram_ee00_list_item);
            io_source_unregister(petdww_ram_ef00_list_item);
            petdww_ram_ec00_list_item = NULL;
            petdww_ram_ed00_list_item = NULL;
            petdww_ram_ee00_list_item = NULL;
            petdww_ram_ef00_list_item = NULL;
        }
        io_source_unregister(petdww_reg_list_item);
        petdww_reg_list_item = NULL;
        petdww_enabled = 0;
        return 0;
    } else {
        if (!petdww_enabled) {
            if (petdww_activate() < 0) {
                return -1;
            }
        }
        if (!mem_at_9000) {
            petdww_ram_ec00_list_item = io_source_register(&petdww_ram_ec00_device);
            petdww_ram_ed00_list_item = io_source_register(&petdww_ram_ed00_device);
            petdww_ram_ee00_list_item = io_source_register(&petdww_ram_ee00_device);
            petdww_ram_ef00_list_item = io_source_register(&petdww_ram_ef00_device);
        }
        petdww_reg_list_item = io_source_register(&petdww_reg_device);
        petdww_enabled = 1;
        return 0;
    }
}

static int set_petdww_filename(const char *name, void *param)
{
    if (petdww_filename != NULL && name != NULL
        && strcmp(name, petdww_filename) == 0) {
        return 0;
    }

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    if (petdww_enabled) {
        petdww_deactivate();
        util_string_set(&petdww_filename, name);
        petdww_activate();
    } else {
        util_string_set(&petdww_filename, name);
    }

    return 0;
}

static const resource_string_t resources_string[] = {
    { "PETDWWfilename", "", RES_EVENT_NO, NULL,
      &petdww_filename, set_petdww_filename, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "PETDWW", 0, RES_EVENT_SAME, NULL,
      &petdww_enabled, set_petdww_enabled, NULL },
    RESOURCE_INT_LIST_END
};

int petdww_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void petdww_resources_shutdown(void)
{
    lib_free(petdww_filename);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-petdww", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "PETDWW", (resource_value_t)1,
      NULL, "Enable the PET DWW hi-res board" },
    { "+petdww", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "PETDWW", (resource_value_t)0,
      NULL, "Disable the PET DWW hi-res board" },
    { "-petdwwimage", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "PETDWWfilename", NULL,
      "<Name>", "Specify name of PET DWW image" },
    CMDLINE_LIST_END
};

int petdww_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

void petdww_init(void)
{
    petdww_log = log_open("PETDWW");
    init_drawing_tables();
}

void petdww_powerup(void)
{
    if (petdww_ram) {
        ram_init(petdww_ram, PET_DWW_RAM_SIZE);
    }
}

void petdww_reset(void)
{
    petdwwpia_reset();
}

static int petdww_activate(void)
{
    if (petres.IOSize < 2048) {
        log_message(petdww_log, "Cannot enable DWW: IOSize too small (%d but must be 2K)", petres.IOSize);
        return -1;
    }

    if (petres.superpet) {
        log_message(petdww_log, "Cannot enable DWW: not compatible with SuperPET");
        return -1;
    }

    petdww_ram = lib_realloc((void *)petdww_ram, (size_t)PET_DWW_RAM_SIZE);


    log_message(petdww_log, "%dKB of hi-res RAM installed.", PET_DWW_RAM_SIZE >> 10);

    if (!util_check_null_string(petdww_filename)) {
        if (util_file_load(petdww_filename, petdww_ram, (size_t)PET_DWW_RAM_SIZE,
                           UTIL_FILE_LOAD_RAW) < 0) {
            log_message(petdww_log, "Reading PET DWW image %s failed.",
                        petdww_filename);
            if (util_file_save(petdww_filename, petdww_ram, PET_DWW_RAM_SIZE) < 0) {
                log_message(petdww_log, "Creating PET DWW image %s failed.",
                            petdww_filename);
                return -1;
            }
            log_message(petdww_log, "Creating PET DWW image %s.",
                        petdww_filename);
            return 0;
        }
        log_message(petdww_log, "Reading PET DWW image %s.", petdww_filename);
    }

    petdww_powerup();
    petdww_reset();

    return 0;
}

static int petdww_deactivate(void)
{
    if (petdww_ram == NULL) {
        return 0;
    }

    if (!util_check_null_string(petdww_filename)) {
        if (util_file_save(petdww_filename, petdww_ram, PET_DWW_RAM_SIZE) < 0) {
            log_message(petdww_log, "Writing PET DWW image %s failed.",
                        petdww_filename);
            return -1;
        }
        log_message(petdww_log, "Writing PET DWW image %s.", petdww_filename);
    }

    pia_reset();
    lib_free(petdww_ram);
    petdww_ram = NULL;

    return 0;
}

void petdww_shutdown(void)
{
    petdww_deactivate();
}

/* ------------------------------------------------------------------------- */
/* Memory mapping overrides.
 * Since our Double-W board sits on the expansion bus,
 * it can override any internal memory.
 * This is controlled by the $9000 / $EC00 mapping bit.
 */

static read_func_ptr_t save_mem_read_tab[PET_DWW_RAM_SIZE >> 8];
static store_func_ptr_t save_mem_write_tab[PET_DWW_RAM_SIZE >> 8];
static uint8_t *save_mem_base_tab[PET_DWW_RAM_SIZE >> 8];
static int save_mem_limit_tab[PET_DWW_RAM_SIZE >> 8];

static uint8_t dww_ram9000_read(uint16_t addr)
{
    uint16_t min9000 = (addr - 0x9000) & RAM_SIZE_MASK;

#if DWW_DEBUG_RAM
    log_message(petdww_log, "dww_ram9000_read: $%04x == $%02x", addr, petdww_ram[min9000]);
#endif
    return petdww_ram[min9000];
}

static void dww_ram9000_store(uint16_t addr, uint8_t value)
{
    uint16_t min9000 = (addr - 0x9000) & RAM_SIZE_MASK;

#if DWW_DEBUG_RAM
    log_message(petdww_log, "dww_ram9000_store: $%04x := $%02x", addr, value);
#endif
    petdww_ram[min9000] = value;
}

void petdww_override_std_9toa(read_func_ptr_t *mem_read_tab, store_func_ptr_t *mem_write_tab, uint8_t **mem_base_tab, int *mem_limit_tab)
{
    int i;

    /* Check this just in case */
    if (petres.superpet) {
        return;
    }
    /*
     * Check if this was already done.
     * FIXME: What if something else also overrides these entries?
     */
    if (mem_read_tab[0x90] == dww_ram9000_read) {
#if DWW_DEBUG_RAM
        log_message(petdww_log, "petdww_override_std_9toa: override already done");
#endif
        return;
    }
#if DWW_DEBUG_RAM
    if (!mem_at_9000) {
        log_message(petdww_log, "petdww_override_std_9toa: ERROR: memory not mapped here");
        return;
    }
#endif

#if DWW_DEBUG_RAM
    log_message(petdww_log, "petdww_override_std_9toa");
#endif
    /* Setup my RAM at $9000 - $AFFF. */
    for (i = 0x90; i < 0xb0; i++) {
        save_mem_read_tab[i - 0x90] = mem_read_tab[i];
        save_mem_write_tab[i - 0x90] = mem_write_tab[i];
        save_mem_base_tab[i - 0x90] = mem_base_tab[i];
        save_mem_limit_tab[i - 0x90] = mem_limit_tab[i];

        mem_read_tab[i] = dww_ram9000_read;
        mem_write_tab[i] = dww_ram9000_store;
        mem_base_tab[i] = NULL;
        mem_limit_tab[i] = 0;
    }
    maincpu_resync_limits();
}

void petdww_restore_std_9toa(read_func_ptr_t *mem_read_tab, store_func_ptr_t *mem_write_tab, uint8_t **mem_base_tab, int *mem_limit_tab)
{
    int i;

    /* Check if this was already done */
    if (mem_read_tab[0x90] == dww_ram9000_read) {
#if DWW_DEBUG_RAM
        log_message(petdww_log, "petdww_restore_std_9toa");
#endif
        /* Restore access to memory at $9000 - $AFFF. */
        for (i = 0x90; i < 0xb0; i++) {
            mem_read_tab[i] = save_mem_read_tab[i - 0x90];
            mem_write_tab[i] = save_mem_write_tab[i - 0x90];
            mem_base_tab[i] = save_mem_base_tab[i - 0x90];
            mem_limit_tab[i] = save_mem_limit_tab[i - 0x90];
        }
        maincpu_resync_limits();
    }
#if DWW_DEBUG_RAM
    else {
        log_message(petdww_log, "petdww_restore_std_9toa: no need to restore");
    }
#endif
}

/* ------------------------------------------------------------------------- */
/* Renaming exported functions */

#define MYPIA_NAME      "DWWPIA"

#define mypia_init petdwwpia_init
#define mypia_reset petdwwpia_reset
#define mypia_store petdwwpia_store
#define mypia_read petdwwpia_read
#define mypia_peek petdwwpia_peek
#define mypia_snapshot_write_module petdwwpia_snapshot_write_module
#define mypia_snapshot_read_module petdwwpia_snapshot_read_module
#define mypia_signal petdwwpia_signal
#define mypia_dump petdwwpia_dump

static piareg mypia;

/* ------------------------------------------------------------------------- */
/* CPU binding */

static void my_set_int(unsigned int pia_int_num, int a)
{
}

static void my_restore_int(unsigned int pia_int_num, int a)
{
}

#define mycpu_rmw_flag   maincpu_rmw_flag
#define myclk            maincpu_clk
#define mycpu_int_status maincpu_int_status

/* ------------------------------------------------------------------------- */
/* PIA resources.  */

/*

   It seems that in the demo programs, the BASIC versions like to
   POKE in 60200, where the machine language programs use $EB00.

60200 Port A or DDR A           $EB28
$EBx0 1  0 \
      2  1 - RAM block at $EC00 (0-7)
      4  2 /
      8  3 charrom         0 = off 1 = on
      16 4 hires           0 = on  1 = off
      32 5 extra charrom   0 = on  1 = off


60201 Control Register A: bit #3 (worth 4) controls if 60200
$EBx1 accesses the Data Direction Register A (0) or Port A (1).

60202 Port B or DDR B
$EBx2 0 = RAM is visible from $9000 - $AFFF
      1 = RAM is bank-switched in blocks of 1 K in $EC00 - $EFFF

      [Control Register B is never mentioned, so putting 1 in this
       address would access the DDR, creating an output line, which
       after RESET is default 0...]

Typical initialisation sequence:

    poke 60201,0        poke 60200,255          (all outputs)
    poke 60201,4        poke 60200,24 or 25 (16 + 8 + 1)

Demo programs on disk PBE-110A, 110B, 111A, and 111B.
(PBE = PET Benelux Exchange, the Dutch PET user group)

The memory mapping is a bit strange. It seems each 1 K block contains
the pixeldata for 1 bit-line of each text line. This is probably so that
the addressing of the RAM can borrow part of the addressing
logic/signals of the text screen. (The screen addressing cycles through
0-39, then increases the line (= byte offset) which is fetched from
the character ROM; for the graphics, the screen position selects the
byte in a KB and the char ROM offset selects which KB of graphics RAM).

My notes say: to set a pixel:

RE = INT(Y/8): LY = Y - 8*RE    (or Y AND 7)
BY = INT(X/8): BI = X - 8*BY    (or X AND 7)

when memory mapped to $9000:

    L = 36864 + 1024 * LY + 40 * RE + BY
    POKE L, PEEK(L) OR 2^BI

when memory mapped to $EC00:

    POKE 60200,LY + 40 (or 8?)
    L = 60416 + RE * 40 + BY
    POKE L, PEEK(L) OR 2^BI

Unfortunately there is no logical means of expanding the memory
to 16 K, so even in a 80 columns PET the resolution will be the same.

[1] Dubbel-W bord, designed by Ben de Winter and Pieter Wolvekamp
*/

static uint8_t output_porta;
static uint8_t output_portb;

static int mem_bank;
static int hires_off;
static int charrom_on;

/* #define mem_bank              ((output_porta & 0x07)<<10) */
/* #define charrom_on             (output_porta & 0x08) */
/* #define hires_off              (output_porta & 0x10) */
#define extra_charrom_off         (output_porta & 0x20)
/* #define mem_at_ec00          (!(output_portb & 0x01)) */

uint8_t petdwwpia_read(uint16_t addr);
void petdwwpia_store(uint16_t addr, uint8_t byte);

int petdww_mem_at_9000()
{
    return mem_at_9000;
}

static uint8_t read_petdww_reg(uint16_t addr)
{
    /* forward to PIA */
#if DWW_DEBUG_REG
    log_message(petdww_log, "read_petdww_reg: $%04x", addr);
#endif
    return petdwwpia_read(addr);
}

static uint8_t read_petdww_ec00_ram(uint16_t addr)
{
    addr &= RAM_1K_SIZE_MASK;
    addr |= mem_bank;

    return petdww_ram[addr];
}

static void store_petdww_reg(uint16_t addr, uint8_t byte)
{
    /* forward to PIA */
#if DWW_DEBUG_REG
    log_message(petdww_log, "store_petdww_reg: $%04x := $%02x", addr, byte);
#endif
    petdwwpia_store(addr, byte);
}

static void store_petdww_ec00_ram(uint16_t addr, uint8_t byte)
{
#if DWW_DEBUG_REG
    log_message(petdww_log, "store_petdww_ec00_ram: $%04x := $%02x", addr, byte);
#endif
    addr &= RAM_1K_SIZE_MASK;
    addr |= mem_bank;

    petdww_ram[addr] = byte;
}

static int petdww_dump(void)
{
    mon_out("DWW memory is at %s\n", (mem_at_9000) ? "$9000-$93FF" : "$EC00-$EFFF");

    /* TODO: dump pia state */
    return 0;
}

/* Callbacks from piacore to store effective output, taking DDR into account */
static void store_pa(uint8_t byte)
{
    output_porta = byte;

    mem_bank = (byte & 0x07) << 10;
    hires_off = byte & 0x10;
    charrom_on = byte & 0x08;
#if DWW_DEBUG_REG || DWW_DEBUG_RAM
    log_message(petdww_log, "mem_bank    = %04x", mem_bank);
#endif
#if DWW_DEBUG_REG
    log_message(petdww_log, "hires_off   = %04x", hires_off);
    log_message(petdww_log, "charrom_on  = %04x", charrom_on);
#endif
    if (petdww_enabled) {
        if (hires_off) {
            if (charrom_on) {
                crtc_set_hires_draw_callback(NULL);
            } else {
                crtc_set_hires_draw_callback(petdww_DRAW_blank);
            }
        } else {
            if (petres.video == 80) {
                crtc_set_hires_draw_callback(petdww_DRAW_80);
            } else {
                crtc_set_hires_draw_callback(petdww_DRAW_40);
            }
        }
    }
}

static void store_pb(uint8_t byte)
{
    int old_at_9000 = mem_at_9000;

    output_portb = byte;
    mem_at_9000 = !(byte & 0x01);

    if (old_at_9000 != mem_at_9000) {
        if (mem_at_9000) {
            io_source_unregister(petdww_ram_ec00_list_item);
            io_source_unregister(petdww_ram_ed00_list_item);
            io_source_unregister(petdww_ram_ee00_list_item);
            io_source_unregister(petdww_ram_ef00_list_item);
            petdww_ram_ec00_list_item = NULL;
            petdww_ram_ed00_list_item = NULL;
            petdww_ram_ee00_list_item = NULL;
            petdww_ram_ef00_list_item = NULL;
        } else {
            petdww_ram_ec00_list_item = io_source_register(&petdww_ram_ec00_device);
            petdww_ram_ed00_list_item = io_source_register(&petdww_ram_ed00_device);
            petdww_ram_ee00_list_item = io_source_register(&petdww_ram_ee00_device);
            petdww_ram_ef00_list_item = io_source_register(&petdww_ram_ef00_device);
        }
    }

#if DWW_DEBUG_REG
    log_message(petdww_log, "ddrb = %02x, pb = %02x, store_pb = %02x", mypia.ddr_b, mypia.port_b, byte);
#endif
#if DWW_DEBUG_RAM || DWW_DEBUG_REG
    log_message(petdww_log, "mem_at_9000 = %d", mem_at_9000);
#endif
    if (petdww_enabled) {
        read_func_ptr_t *read;
        store_func_ptr_t *write;
        uint8_t **base;
        int *limit;

        get_mem_access_tables(&read, &write, &base, &limit);

        if (mem_at_9000) {
            petdww_override_std_9toa(read, write, base, limit);
        } else {
            petdww_restore_std_9toa(read, write, base, limit);
        }
    }
}

static void undump_pa(uint8_t byte)
{
    store_pa(byte);
}

static void undump_pb(uint8_t byte)
{
    store_pb(byte);
}

static uint8_t read_pa(void)
{
    uint8_t byte;

    byte = ((~mypia.ddr_a) | (mypia.port_a & mypia.ddr_a));

    return byte;
}


static uint8_t read_pb(void)
{
    return mypia.port_b;
}

/* ------------------------------------------------------------------------- */
/* I/O */

static void pia_set_ca2(int a)
{
    /* Not Connected */
}

static void pia_set_cb2(int a)
{
    /* Not Connected */
}

static void pia_reset(void)
{
    mypia.port_a = 0;   /* port A output; piacore assumes input */
    mypia.port_b = 0;   /* port B output; piacore assumes input */
    store_pa(0xff);
    store_pb(0xff);
}

#include "piacore.c"

/* ------------------------------------------------------------------------- */
/* Raster drawing */

/*
 * These tables are the same as in crtc/crtc-draw.c,
 * but their index is bit-reversed, since the DWW hi-res board
 * displays the lsb on the left.
 * The difference is that the shifting of msk is the other direction,
 * and correspondingly the initial set bit is on the other side.
 *
 * Effectively, they expand 4 bits to 4 full bytes, e.g.:
 *
 *  0x0B -> 0100 0101
 *
 * You don't really need 2 tables of 256 entries,
 * you can use 1 table of 16 entries, twice;
 * once for each of the 2 nybbles.
 *
 * This costs some extra shifting and masking but is likely to win by
 * wasting less CPU/L2 cache (the whole table will easily fit in
 * a couple of cache lines).
 *
 * The "w" tables expand each input bit into double-wide bytes:
 * every 4 bits expand to 8 bytes (hence 2 tables are needed).
 */
static uint32_t dww_dwg_table[16];
static uint32_t dww_dwg_table_w0[16], dww_dwg_table_w1[16];

static void init_drawing_tables(void)
{
    int byte, p;
    uint8_t msk;

    for (byte = 0; byte < 0x10; byte++) {
        for (msk = 0x01, p = 0; p < 4; msk <<= 1, p++) {
            *((uint8_t *)(dww_dwg_table + byte) + p)
                = (byte & msk ? 1 : 0);
        }
#if DWW_DEBUG_GFX
        log_message(petdww_log, "init_drawing_tables: %02x -> %08x", byte, dww_dwg_table[byte]);
#endif
    }

    for (byte = 0; byte < 0x10; byte++) {
        for (msk = 0x01, p = 0; p < 4; msk <<= 1, p++) {
            int bit = (byte & msk) ? 1 : 0;
            *((uint8_t *)(dww_dwg_table_w0 + byte) + p) = bit;
            p++;
            *((uint8_t *)(dww_dwg_table_w0 + byte) + p) = bit;
        }
        for (p = 0; p < 4; msk <<= 1, p++) {
            int bit = (byte & msk) ? 1 : 0;
            *((uint8_t *)(dww_dwg_table_w1 + byte) + p) = bit;
            p++;
            *((uint8_t *)(dww_dwg_table_w1 + byte) + p) = bit;
        }
#if DWW_DEBUG_GFX
        log_message(petdww_log, "init_drawing_tables: %02x -> %08x %08x", byte, dww_dwg_table_w1[byte], dww_dwg_table_w0[byte]);

#endif
    }
}

static void petdww_DRAW_40(uint8_t *p, int xstart, int xend, int scr_rel, int ymod8)
{
    if (ymod8 < 8 && xstart < xend) {
        int k = ymod8 * 1024;
        uint8_t *screen_rel = petdww_ram + k + (scr_rel & 0x03FF);
        uint8_t *screen_end = petdww_ram + k + 1024;
        uint32_t *pw = (uint32_t *)p;
        int i;
        int d;

#if DWW_DEBUG_GFX
        log_message(petdww_log, "petdww_DRAW: xstart=%d, xend=%d, ymod8=%d, scr_rel=%04x", xstart, xend, ymod8, scr_rel);
#endif
        /*
         * The DWW board can turn off the normal character ROM.
         * Implement that here by (optionally) overwriting the character
         * bits by the graphics, instead of ORing it,
         * which is slightly inefficient.
         *
         * (The "extra charrom" option isn't implemented at all,
         * since I have no idea how it worked.)
         *
         * The (scr_rel & 0x03FF) and screen_end above help when the
         * start of the screen has been moved up, and at some point we
         * need to wrap around to the beginning.
         */

        if (charrom_on) {
            for (i = xstart; i < xend; i++) {
                if (screen_rel >= screen_end) {
                    screen_rel = petdww_ram + k;
                }
                d = *screen_rel++;

#if DWW_DEBUG_GFX
                log_message(petdww_log, "%2d -> %02x -> %08x", i, d, dww_dwg_table[d]);
#endif
                *pw++ |= dww_dwg_table[d & 0x0f];
                *pw++ |= dww_dwg_table[d >> 4];
            }
        } else {
            for (i = xstart; i < xend; i++) {
                if (screen_rel >= screen_end) {
                    screen_rel = petdww_ram + k;
                }
                d = *screen_rel++;

                *pw++ = dww_dwg_table[d & 0x0f];
                *pw++ = dww_dwg_table[d >> 4];
            }
        }
    }
}

static void petdww_DRAW_80(uint8_t *p, int xstart, int xend, int scr_rel, int ymod8)
{
    if (ymod8 < 8 && xstart < xend) {
        int k = ymod8 * 1024;
        uint8_t *screen_rel = petdww_ram + k + ((scr_rel / 2) & 0x03FF);
        uint8_t *screen_end = petdww_ram + k + 1024;
        uint32_t *pw = (uint32_t *)p;
        int i;
        int d;

#if DWW_DEBUG_GFX
        log_message(petdww_log, "petdww_DRAW: xstart=%d, xend=%d, ymod8=%d, scr_rel=%04x", xstart, xend, ymod8, scr_rel);
#endif
        xstart /= 2;
        xend /= 2;

        /*
         * The DWW board can turn off the normal character ROM.
         * Implement that here by (optionally) overwriting the character
         * bits by the graphics, instead of ORing it,
         * which is slightly inefficient.
         *
         * (The "extra charrom" option isn't implemented at all,
         * since I have no idea how it worked.)
         *
         * The (scr_rel & 0x03FF) and screen_end above help when the
         * start of the screen has been moved up, and at some point we
         * need to wrap around to the beginning.
         */

        if (charrom_on) {
            for (i = xstart; i < xend; i++) {
                if (screen_rel >= screen_end) {
                    screen_rel = petdww_ram + k;
                }
                d = *screen_rel++;

#if DWW_DEBUG_GFX
                log_message(petdww_log, "%2d -> %02x -> %08x", i, d, dww_dwg_table[d]);
#endif
                *pw++ |= dww_dwg_table_w0[d & 0x0f];
                *pw++ |= dww_dwg_table_w1[d & 0x0f];
                *pw++ |= dww_dwg_table_w0[d >> 4];
                *pw++ |= dww_dwg_table_w1[d >> 4];
            }
        } else {
            for (i = xstart; i < xend; i++) {
                if (screen_rel >= screen_end) {
                    screen_rel = petdww_ram + k;
                }
                d = *screen_rel++;

                *pw++ = dww_dwg_table_w0[d & 0x0f];
                *pw++ = dww_dwg_table_w1[d & 0x0f];
                *pw++ = dww_dwg_table_w0[d >> 4];
                *pw++ = dww_dwg_table_w1[d >> 4];
            }
        }
    }
}

static void petdww_DRAW_blank(uint8_t *p, int xstart, int xend, int scr_rel, int ymod8)
{
    uint32_t *pw = (uint32_t *)p;
    int i;

#if DWW_DEBUG_GFX
    log_message(petdww_log, "petdww_DRAW_blank: xstart=%d, xend=%d, ymod8=%d, scr_rel=%04x", xstart, xend, ymod8, scr_rel);
#endif
    /*
     * Implement the combination where both text and gfx are
     * turned off, by overwriting the text pixels.
     *
     * This is slightly inefficient.
     */

    for (i = xstart; i < xend; i++) {
        *pw++ = 0;
        *pw++ = 0;
    }
}

/* ------------------------------------------------------------------------- */
/* native screenshot support */

uint8_t *petdww_crtc_get_active_bitmap(void)
{
    if (petdww_enabled && !hires_off) {
        return petdww_ram;
    }
    return NULL;
}

/* ------------------------------------------------------------------------- */
/* Snapshot support */

static const char module_ram_name[] = "DWWMEM";
#define DWWMEM_DUMP_VER_MAJOR   1
#define DWWMEM_DUMP_VER_MINOR   0

/* Format of the DWW ram snapshot
 *
 * WORD         size, 0 if not allocated
 * BYTE[size]   memory
 *
 */

static int petdww_ram_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, module_ram_name,
                               DWWMEM_DUMP_VER_MAJOR, DWWMEM_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (petdww_ram) {
        SMW_W(m, PET_DWW_RAM_SIZE);
        SMW_BA(m, petdww_ram, PET_DWW_RAM_SIZE);
    } else {
        SMW_W(m, 0);
    }

    snapshot_module_close(m);

    return 0;
}

static int petdww_ram_read_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;
    uint8_t vmajor, vminor;
    uint16_t ramsize;

    m = snapshot_module_open(s, module_ram_name, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if (vmajor != DWWMEM_DUMP_VER_MAJOR) {
        log_error(petdww_log,
                  "Cannot load DWW RAM module with major version %d",
                  vmajor);
        snapshot_module_close(m);
        return -1;
    }

    SMR_W(m, &ramsize);

    if (ramsize) {
        if (ramsize > PET_DWW_RAM_SIZE) {
            ramsize = PET_DWW_RAM_SIZE;
        }
        petdww_ram = lib_realloc((void *)petdww_ram, (size_t)PET_DWW_RAM_SIZE);
        SMR_BA(m, petdww_ram, ramsize);
    } else {
        lib_free((void *)petdww_ram);
        petdww_ram = NULL;
    }

    snapshot_module_close(m);

    return 0;
}

int petdww_snapshot_write_module(snapshot_t *m)
{
    if (petdwwpia_snapshot_write_module(m) < 0
        || petdww_ram_write_snapshot_module(m) < 0) {
        return -1;
    }
    return 0;
}

int petdww_snapshot_read_module(snapshot_t *m)
{
    if (petdwwpia_snapshot_read_module(m) < 0
        || petdww_ram_read_snapshot_module(m) < 0) {
        return 0;       /* for now, to be able to read old snapshots */
    }
    return 0;
}
