/*
 * c64tpi.c - IEEE488 interface for the C64.
 *
 * Written by
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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

#include "archdep.h"
#include "c64.h"
#include "c64cart.h" /* for export_t */
#define CARTRIDGE_INCLUDE_SLOT0_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOT0_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "drive.h"
#include "export.h"
#include "lib.h"
#include "log.h"
#include "parallel.h"
#include "maincpu.h"
#include "monitor.h"
#include "resources.h"
#include "tpi.h"
#include "translate.h"
#include "types.h"
#include "util.h"
#include "crt.h"

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "c64tpi.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API

/*
    IEEE488 interface for c64 and c128

    - 4kb ROM, mapped to $8000 in 8k game config

    - the hardware uses a TPI at $DF00-$DF07 (mirrored through $DF08-$DFFF)

    TODO: register description
*/

/* #define DEBUGTPI */

#ifdef DEBUGTPI
#define DBG(x) printf x
#else
#define DBG(x)
#endif

#define mytpi_init tpi_init
#define mytpi_set_int tpi_set_int

/* 4 KB ROM */
#define TPI_ROM_SIZE 0x1000
static BYTE *tpi_rom = NULL;

static tpi_context_t *tpi_context;

/* ---------------------------------------------------------------------*/
static void tpi_io2_store(WORD addr, BYTE data);
static BYTE tpi_io2_read(WORD addr);
static BYTE tpi_io2_peek(WORD addr);
static int tpi_io2_dump(void);

static io_source_t tpi_io2_device = {
    CARTRIDGE_NAME_IEEE488,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0x07,
    1, /* read is always valid */
    tpi_io2_store,
    tpi_io2_read,
    tpi_io2_peek,
    tpi_io2_dump,
    CARTRIDGE_IEEE488,
    0,
    0
};

static io_source_list_t *tpi_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_IEEE488, 0, 1, NULL, &tpi_io2_device, CARTRIDGE_IEEE488
};

/* ---------------------------------------------------------------------*/

static int ieee488_enabled = 0;

static int tpi_extexrom = 0;
static int tpi_extgame = 0;

static int rom_enabled = 1;

int tpi_cart_enabled(void)
{
    return ieee488_enabled;
}

/* ---------------------------------------------------------------------*/

static void tpi_io2_store(WORD addr, BYTE data)
{
    DBG(("TPI io2 w %02x (%02x)\n", addr, data));
    tpicore_store(tpi_context, addr, data);
}

static BYTE tpi_io2_read(WORD addr)
{
    DBG(("TPI io2 r %02x\n", addr));
    return tpicore_read(tpi_context, addr);
}

static BYTE tpi_io2_peek(WORD addr)
{
    return tpicore_peek(tpi_context, addr);
}

static int tpi_io2_dump(void)
{
    mon_out("TPI\n");
    tpicore_dump(tpi_context);
    return 0;
}
/* ---------------------------------------------------------------------*/

int tpi_roml_read(WORD addr, BYTE *value)
{
    if (rom_enabled) {
        *value = tpi_rom[addr & 0xfff];
        return CART_READ_VALID;
    }
    return CART_READ_THROUGH;
}

int tpi_peek_mem(WORD addr, BYTE *value)
{
    if ((addr >= 0x8000) && (addr <= 0x9fff)) {
        if (rom_enabled) {
            *value = tpi_rom[addr & 0xfff];
            return CART_READ_VALID;
        }
    }
    return CART_READ_THROUGH;
}

/* ---------------------------------------------------------------------*/

/*
    Port A (ieee control)

    Port B (ieee data)

    Port C

    bit 7  in  passthrough port exrom line
    bit 6  out (unused ?)
    bit 5  out (unused ?)
    bit 4  out ROML enable
    bit 3  out expansionport exrom line
    bit 2  out (unused ?)
    bit 1  out IEEE (U4, Pin 12)
    bit 0  out IEEE (U4, Pin 18)
*/

static void set_int(unsigned int int_num, int value)
{
}

static void restore_int(unsigned int int_num, int value)
{
}

static void set_ca(tpi_context_t *tpi_context, int a)
{
}

static void set_cb(tpi_context_t *tpi_context, int a)
{
}

static int ieee_is_dev = 1;
static BYTE ieee_is_out = 1;

static void reset(tpi_context_t *tpi_context)
{
    /* assuming input after reset */
    parallel_cpu_set_atn(0);
    parallel_cpu_set_ndac(0);
    parallel_cpu_set_nrfd(0);
    parallel_cpu_set_dav(0);
    parallel_cpu_set_eoi(0);
    parallel_cpu_set_bus(0xff);

    ieee_is_dev = 1;
    ieee_is_out = 1;
}

static void store_pa(tpi_context_t *tpi_context, BYTE byte)
{
    if (byte != tpi_context->oldpa) {
        BYTE tmp = ~byte;

        ieee_is_dev = byte & 0x01;
        ieee_is_out = byte & 0x02;

        parallel_cpu_set_bus((BYTE)(ieee_is_out ? tpi_context->oldpb : 0xff));

        if (ieee_is_out) {
            parallel_cpu_set_ndac(0);
            parallel_cpu_set_nrfd(0);
            parallel_cpu_set_dav((BYTE)(tmp & 0x10));
            parallel_cpu_set_eoi((BYTE)(tmp & 0x20));
        } else {
            parallel_cpu_set_nrfd((BYTE)(tmp & 0x80));
            parallel_cpu_set_ndac((BYTE)(tmp & 0x40));
            parallel_cpu_set_dav(0);
            parallel_cpu_set_eoi(0);
        }
        if (ieee_is_dev) {
            parallel_cpu_set_atn(0);
        } else {
            parallel_cpu_set_atn((BYTE)(tmp & 0x08));
        }
    }
}

static void store_pb(tpi_context_t *tpi_context, BYTE byte)
{
    parallel_cpu_set_bus((BYTE)(ieee_is_out ? byte : 0xff));
}

static void undump_pa(tpi_context_t *tpi_context, BYTE byte)
{
    BYTE tmp = ~byte;
    ieee_is_dev = byte & 0x01;
    ieee_is_out = byte & 0x02;

    parallel_cpu_set_bus((BYTE)(ieee_is_out ? tpi_context->oldpb : 0xff));

    if (ieee_is_out) {
        parallel_cpu_set_ndac(0);
        parallel_cpu_set_nrfd(0);
        parallel_cpu_set_dav((BYTE)(tmp & 0x10));
        parallel_cpu_set_eoi((BYTE)(tmp & 0x20));
    } else {
        parallel_cpu_set_nrfd((BYTE)(tmp & 0x80));
        parallel_cpu_set_ndac((BYTE)(tmp & 0x40));
        parallel_cpu_set_dav(0);
        parallel_cpu_set_eoi(0);
    }
    if (ieee_is_dev) {
        parallel_cpu_restore_atn(0);
    } else {
        parallel_cpu_restore_atn((BYTE)(tmp & 0x08));
    }
}

static void undump_pb(tpi_context_t *tpi_context, BYTE byte)
{
    parallel_cpu_set_bus((BYTE)(ieee_is_out ? byte : 0xff));
}

static void store_pc(tpi_context_t *tpi_context, BYTE byte)
{
    int exrom = ((byte & 0x08) ? 0 : 1); /* bit 3, 1 = active */
    rom_enabled = ((byte & 0x10) ? 1 : 0); /* bit 4, 1 = active */
    /* passthrough support */
    DBG(("TPI store_pc %02x (rom enabled: %d exrom: %d game: %d)\n", byte, rom_enabled, exrom ^ 1, tpi_extgame));
    cart_config_changed_slot0((BYTE)((exrom << 1) | tpi_extgame), (BYTE)((exrom << 1) | tpi_extgame), CMODE_READ);
}

static void undump_pc(tpi_context_t *tpi_context, BYTE byte)
{
}

static BYTE read_pa(tpi_context_t *tpi_context)
{
    BYTE byte;

    drive_cpu_execute_all(maincpu_clk);

    byte = 0xff;
    if (ieee_is_out) {
        if (parallel_nrfd) {
            byte &= 0x7f;
        }
        if (parallel_ndac) {
            byte &= 0xbf;
        }
    } else {
        if (parallel_dav) {
            byte &= 0xef;
        }
        if (parallel_eoi) {
            byte &= 0xdf;
        }
    }
    if (ieee_is_dev) {
        if (parallel_atn) {
            byte &= 0xf7;
        }
    }

    byte = (byte & ~(tpi_context->c_tpi)[TPI_DDPA]) | (tpi_context->c_tpi[TPI_PA] & tpi_context->c_tpi[TPI_DDPA]);

    return byte;
}

static BYTE read_pb(tpi_context_t *tpi_context)
{
    BYTE byte;

    drive_cpu_execute_all(maincpu_clk);

    byte = ieee_is_out ? 0xff : parallel_bus;
    byte = (byte & ~(tpi_context->c_tpi)[TPI_DDPB]) | (tpi_context->c_tpi[TPI_PB] & tpi_context->c_tpi[TPI_DDPB]);

    return byte;
}

static BYTE read_pc(tpi_context_t *tpi_context)
{
    BYTE byte = 0xff;

    if (tpi_extexrom) {
        byte &= ~(1 << 7);
    }
    byte = (byte & ~(tpi_context->c_tpi)[TPI_DDPC]) | (tpi_context->c_tpi[TPI_PC] & tpi_context->c_tpi[TPI_DDPC]);
    return byte;
}

/* ---------------------------------------------------------------------*/

void tpi_reset(void)
{
    DBG(("TPI: tpi_reset\n"));
    tpicore_reset(tpi_context);
    cart_config_changed_slot0(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    rom_enabled = 1;
}

void tpi_init(void)
{
    tpi_context->log = log_open(tpi_context->myname);
}

void tpi_shutdown(void)
{
    tpicore_shutdown(tpi_context);
}

void tpi_setup_context(machine_context_t *machine_context)
{
    tpi_context = lib_malloc(sizeof(tpi_context_t));

    tpi_context->prv = NULL;

    tpi_context->context = (void *)machine_context;

    tpi_context->rmw_flag = &maincpu_rmw_flag;
    tpi_context->clk_ptr = &maincpu_clk;

    tpi_context->myname = lib_msprintf("TPI");

    tpicore_setup_context(tpi_context);

    tpi_context->store_pa = store_pa;
    tpi_context->store_pb = store_pb;
    tpi_context->store_pc = store_pc;
    tpi_context->read_pa = read_pa;
    tpi_context->read_pb = read_pb;
    tpi_context->read_pc = read_pc;
    tpi_context->undump_pa = undump_pa;
    tpi_context->undump_pb = undump_pb;
    tpi_context->undump_pc = undump_pc;
    tpi_context->reset = reset;
    tpi_context->set_ca = set_ca;
    tpi_context->set_cb = set_cb;
    tpi_context->set_int = set_int;
    tpi_context->restore_int = restore_int;
}

void tpi_passthrough_changed(export_t *export)
{
    tpi_extexrom = (export)->exrom;
    tpi_extgame = (export)->game;
    DBG(("IEEE488 passthrough changed exrom: %d game: %d\n", tpi_extexrom, tpi_extgame));

    cart_set_port_game_slot0(tpi_extgame);
    cart_port_config_changed_slot0();
}

/* ---------------------------------------------------------------------*/

static char *ieee488_filename = NULL;

static int set_ieee488_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    DBG(("IEEE: set_enabled: (%p) '%s' %d to %d\n", param, ieee488_filename, ieee488_enabled, val));
    if (ieee488_enabled && !val) {
        cart_power_off();
#ifdef DEBUGTPI
        if (tpi_list_item == NULL) {
            DBG(("IEEE: BUG: ieee488_enabled == 1 and tpi_list_item == NULL ?!\n"));
        }
#endif
        lib_free(tpi_rom);
        tpi_rom = NULL;
        export_remove(&export_res);
        io_source_unregister(tpi_list_item);
        tpi_list_item = NULL;
        ieee488_enabled = 0;
        DBG(("IEEE: set_enabled unregistered\n"));
    } else if (!ieee488_enabled && val) {
        if (tpi_rom == NULL) {
            tpi_rom = lib_malloc(TPI_ROM_SIZE);
        }
        if (param) {
            /* if the param is != NULL, then we should load the default image file */
            if (ieee488_filename) {
                if (*ieee488_filename) {
                    DBG(("IEEE: attach default image\n"));
                    if (cartridge_attach_image(CARTRIDGE_IEEE488, ieee488_filename) < 0) {
                        DBG(("IEEE: set_enabled did not register\n"));
                        lib_free(tpi_rom);
                        tpi_rom = NULL;
                        return -1;
                    }
                    /* ieee488_enabled = 1; */ /* cartridge_attach_image will end up calling set_ieee488_enabled again */
                    return 0;
                }
            }
        } else {
            cart_power_off();
            /* if the param is == NULL, then we should actually set the resource */
            if (export_add(&export_res) < 0) {
                DBG(("IEEE: set_enabled did not register\n"));
                lib_free(tpi_rom);
                tpi_rom = NULL;
                return -1;
            } else {
                DBG(("IEEE: set_enabled registered\n"));
                tpi_list_item = io_source_register(&tpi_io2_device);
                ieee488_enabled = 1;
            }
        }
    }

    DBG(("IEEE: set_enabled done: '%s' %d : %d\n", ieee488_filename, val, ieee488_enabled));
    return 0;
}

static int set_ieee488_filename(const char *name, void *param)
{
    int enabled;

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }
    DBG(("IEEE: set_name: %d '%s'\n", ieee488_enabled, ieee488_filename));

    util_string_set(&ieee488_filename, name);
    resources_get_int("IEEE488", &enabled);

    if (set_ieee488_enabled(enabled, (void*)1) < 0) {
        lib_free(ieee488_filename);
        ieee488_filename = NULL;
        DBG(("IEEE: set_name done: %d '%s'\n", ieee488_enabled, ieee488_filename));
        return -1;
    }

    DBG(("IEEE: set_name done: %d '%s'\n", ieee488_enabled, ieee488_filename));
    return 0;
}

static const resource_string_t resources_string[] = {
    { "IEEE488Image", "", RES_EVENT_NO, NULL,
      &ieee488_filename, set_ieee488_filename, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "IEEE488", 0, RES_EVENT_SAME, NULL,
      &ieee488_enabled, set_ieee488_enabled, (void *)1 },
    RESOURCE_INT_LIST_END
};

int tpi_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }
    return resources_register_int(resources_int);
}

void tpi_resources_shutdown(void)
{
    lib_free(ieee488_filename);
    ieee488_filename = NULL;
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-ieee488", SET_RESOURCE, 0,
      NULL, NULL, "IEEE488", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_IEEE488_INTERFACE,
      NULL, NULL },
    { "+ieee488", SET_RESOURCE, 0,
      NULL, NULL, "IEEE488", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_IEEE488_INTERFACE,
      NULL, NULL },
    { "-ieee488image", SET_RESOURCE, 1,
      NULL, NULL, "IEEE488Image", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_IEEE488_INTERFACE_NAME,
      NULL, NULL },
    CMDLINE_LIST_END
};

int tpi_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

const char *tpi_get_file_name(void)
{
    return ieee488_filename;
}

void tpi_config_setup(BYTE *rawcart)
{
    DBG(("TPI: config_setup\n"));
    memcpy(tpi_rom, rawcart, TPI_ROM_SIZE);
}

int tpi_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit)
{
    if (rom_enabled) {
        switch (addr & 0xf000) {
            case 0x9000:
                *base = tpi_rom - 0x9000;
                *start = 0x9000;
                *limit = 0x9ffd;
                return CART_READ_VALID;
            case 0x8000:
                *base = tpi_rom - 0x8000;
                *start = 0x8000;
                *limit = 0x8ffd;
                return CART_READ_VALID;
            default:
                break;
        }
    }
    return CART_READ_THROUGH;
}

void tpi_config_init(export_t *export)
{
    DBG(("TPI: tpi_config_init\n"));

    tpi_extexrom = export->exrom;
    tpi_extgame = export->game;

    cart_set_port_exrom_slot0(1);
    cart_set_port_game_slot0(tpi_extgame);
    cart_port_config_changed_slot0();
    rom_enabled = 1;
}

static int tpi_common_attach(void)
{
    DBG(("TPI: tpi_common_attach\n"));
    return set_ieee488_enabled(1, NULL);
}

int tpi_bin_attach(const char *filename, BYTE *rawcart)
{
    DBG(("TPI: tpi_bin_attach\n"));

    if (util_file_load(filename, rawcart, TPI_ROM_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return tpi_common_attach();
}

int tpi_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.size != TPI_ROM_SIZE) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    return tpi_common_attach();
}

void tpi_detach(void)
{
    set_ieee488_enabled(0, NULL);
}

int tpi_enable(void)
{
    return set_ieee488_enabled(1, (void*)1);
}

/* ---------------------------------------------------------------------*/

int tpi_snapshot_read_module(struct snapshot_s *s)
{
    if (tpicore_snapshot_read_module(tpi_context, s) < 0) {
        ieee488_enabled = 0;
        return -1;
    } else {
        ieee488_enabled = 1;
    }
    return 0;
}

int tpi_snapshot_write_module(struct snapshot_s *s)
{
    if (tpicore_snapshot_write_module(tpi_context, s) < 0) {
        return -1;
    }
    return 0;
}
