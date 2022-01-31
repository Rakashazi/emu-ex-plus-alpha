/*
 * ieeeflash64.c - IEEE Flash! 64 interface emulation
 *
 * Written by
 *  Christopher Bongaarts <cab@bongalow.net>
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

#define CARTRIDGE_INCLUDE_SLOT0_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOT0_API
#include "c64mem.h"
#include "c64memrom.h"
#include "c64rom.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "ieeeflash64.h"
#include "export.h"
#include "maincpu.h"
#include "drive.h"
#include "parallel.h"
#include "mc6821core.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "lib.h"
#include "crt.h"
#include "monitor.h"

/*
    IEEE Flash! 64  -  Skyles Electic Works
    - IEEE interface for C64.  Plugs into cartridge port; includes
      pass-through connector.  4 DIP switches, 8K KERNAL replacement ROM,
      6821 PIA mapped at $de00.  Replacement ROM has hooks to serial
      routines that check DIP switches to route to IEEE routines or
      original IEC routines.

    - 6821 pin mappings:
        PA0-7 = IEEE data lines D1-8
        PB0 = dev8 switch
        PB1 = dev9/10 switch
        PB2 = dev4 switch
        PB3 = IFC (reset)
        PB4 = EOI
        PB5 = DAV
        PB6 = NRFD
        PB7 = NDAC
        CA2 = ATN

    8K ROM
    - ROM is mapped to $e000 using ultimax mode, but only when hiram is inactive
      (the cartridge uses a clip to the inside of the computer for this)
*/

/* #define DEBUGTPI */

#ifdef DEBUGTPI
#define DBG(x) printf x
#else
#define DBG(x)
#endif

static int ieeeflash64_switch8 = 0;
static int ieeeflash64_switch910 = 0;
static int ieeeflash64_switch4 = 0;

static int ieeeflash64_enabled = 0;

static int ieeeflash64_extexrom = 0;
static int ieeeflash64_extgame = 0;

static mc6821_state ieeeflash64_6821;

/* bit positions in PIA port B for IEEE signals */
#define PIA_EOI_BIT 0x10
#define PIA_DAV_BIT 0x20
#define PIA_NRFD_BIT 0x40
#define PIA_NDAC_BIT 0x80

/* private ROM storage for slot0 API */
#define IEEEFLASH64_ROM_SIZE 0x2000
static uint8_t *ieeeflash64_rom = NULL;

/* filename of kernal rom */
static char *ieeeflash64_filename = NULL;

/* forward decl needed by set_ieeeflash64_filename */
static int set_ieeeflash64_enabled(int value, void *param);

int ieeeflash64_cart_enabled(void) {
    return ieeeflash64_enabled;
}

static int ieeeflash64_set_switch8(int val, void *param)
{
    ieeeflash64_switch8 = val ? 1 : 0;

    return 0;
}

static int ieeeflash64_set_switch910(int val, void *param)
{
    ieeeflash64_switch910 = val ? 1 : 0;

    return 0;
}

static int ieeeflash64_set_switch4(int val, void *param)
{
    ieeeflash64_switch4 = val ? 1 : 0;

    return 0;
}

static int set_ieeeflash64_filename(const char *name, void *param)
{
    int enabled;

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }
    DBG(("IEEEFlash64: set_name: %d '%s'\n", ieeeflash64_enabled, ieeeflash64_filename));

    util_string_set(&ieeeflash64_filename, name);
    resources_get_int("IEEEFlash64", &enabled);

    if (set_ieeeflash64_enabled(enabled, (void*)1) < 0) {
        lib_free(ieeeflash64_filename);
        ieeeflash64_filename = NULL;
        DBG(("IEEEFlash64: set_name done (failed): %d '%s'\n", ieeeflash64_enabled, ieeeflash64_filename));
        return -1;
    }

    DBG(("IEEEFlash64: set_name done: %d '%s'\n", ieeeflash64_enabled, ieeeflash64_filename));
    return 0;
}

const char *ieeeflash64_get_file_name(void)
{
    return ieeeflash64_filename;
}

/*! \brief integer resources used by the IEEE Flash 64 module */
static const resource_int_t resources_int[] = {
    { "IEEEFlash64Dev8", 0, RES_EVENT_NO, NULL,
      &ieeeflash64_switch8, ieeeflash64_set_switch8, NULL },
    { "IEEEFlash64Dev910", 0, RES_EVENT_NO, NULL,
      &ieeeflash64_switch910, ieeeflash64_set_switch910, NULL },
    { "IEEEFlash64Dev4", 0, RES_EVENT_NO, NULL,
      &ieeeflash64_switch4, ieeeflash64_set_switch4, NULL },
    /* keeping "enable" resource last prevents unnecessary (re)init when loading config file */
    { "IEEEFlash64", 0, RES_EVENT_SAME, (resource_value_t)0,
      &ieeeflash64_enabled, set_ieeeflash64_enabled, (void *)1 },
    RESOURCE_INT_LIST_END
};

static const resource_string_t resources_string[] = {
    { "IEEEFlash64Image", "", RES_EVENT_NO, NULL,
      &ieeeflash64_filename, set_ieeeflash64_filename, NULL },
    RESOURCE_STRING_LIST_END
};

/*! \brief initialize the ieee flash! 64 resources
 \return
   0 on success, else -1.

 \remark
   Registers the string and the integer resources
*/
int ieeeflash64_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }
    return resources_register_int(resources_int);
}

/*! \brief uninitialize the ieee flash 64 resources */
void ieeeflash64_resources_shutdown(void)
{
    lib_free(ieeeflash64_filename);
    ieeeflash64_filename = NULL;
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-ieeeflash64", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IEEEFlash64", (resource_value_t)1,
      NULL, "Enable IEEE Flash! 64" },
    { "+ieeeflash64", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IEEEFlash64", (resource_value_t)0,
      NULL, "Disable IEEE Flash! 64" },
    { "-ieeeflash64dev8", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IEEEFlash64Dev8", (resource_value_t)0,
      NULL, "Route device 8 to IEC bus" },
    { "+ieeeflash64dev8", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IEEEFlash64Dev8", (resource_value_t)1,
      NULL, "Route device 8 to IEEE bus" },
    { "-ieeeflash64dev910", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IEEEFlash64Dev910", (resource_value_t)0,
      NULL, "Route devices 9 and 10 to IEC bus" },
    { "+ieeeflash64dev910", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IEEEFlash64Dev910", (resource_value_t)1,
      NULL, "Route devices 9 and 10 to IEEE bus" },
    { "-ieeeflash64dev4", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IEEEFlash64Dev4", (resource_value_t)0,
      NULL, "Route device 4 to IEC bus" },
    { "+ieeeflash64dev4", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IEEEFlash64Dev4", (resource_value_t)1,
      NULL, "Route device 4 to IEEE bus" },
    { "-ieeeflash64image", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IEEEFlash64Image", NULL,
      "<Name>", "specify ieeeflash64 interface kernal image name" },
    CMDLINE_LIST_END
};

/* ---------------------------------------------------------------------*/

/*! \brief initialize the command-line options'
 \return
   0 on success, else -1.

 \remark
   Registers the command-line options
*/
int ieeeflash64_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
/* ---------------------------------------------------------------------*/

static void ieeeflash64_io1_store(uint16_t addr, uint8_t data);
static uint8_t ieeeflash64_io1_read(uint16_t addr);
static uint8_t ieeeflash64_io1_peek(uint16_t addr);
static int ieeeflash64_io1_dump(void);

static io_source_t ieeeflash64_io1_device = {
    CARTRIDGE_NAME_IEEEFLASH64, /* name of the device */
    IO_DETACH_CART,             /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,      /* does not use a resource for detach */
    0xde00, 0xdeff, 0x03,       /* range for the device, regs:$de00-$de03, mirrors:$de04-$deff */
    1,                          /* read is always valid */
    ieeeflash64_io1_store,      /* store function */
    NULL,                       /* NO poke function */
    ieeeflash64_io1_read,       /* read function */
    ieeeflash64_io1_peek,       /* peek function */
    ieeeflash64_io1_dump,       /* device state information dump function */
    CARTRIDGE_IEEEFLASH64,      /* cartridge ID */
    IO_PRIO_NORMAL,             /* normal priority, device read needs to be checked for collisions */
    0                           /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *ieeeflash64_io_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_IEEEFLASH64, 1, 0, &ieeeflash64_io1_device, NULL, CARTRIDGE_IEEEFLASH64
};

/* ---------------------------------------------------------------------*/
/* the IEEE Flash just uses the PIA for its GPIO functions, so lets just
   patch things in directly here. */

static void ieeeflash64_io1_store(uint16_t addr, uint8_t data)
{
    int port, reg;

    /* DBG(("PIA io1 w %02x (%02x)\n", addr, data)); */
    port = (addr >> 1) & 1; /* rs1 */
    reg = (addr >> 0) & 1;  /* rs0 */

    mc6821core_store(&ieeeflash64_6821, port, reg, data);
}

static uint8_t ieeeflash64_io1_read(uint16_t addr)
{
    int port, reg;

    /* DBG(("PIA io1 r %02x\n", addr)); */
    port = (addr >> 1) & 1; /* rs1 */
    reg = (addr >> 0) & 1;  /* rs0 */
    return mc6821core_read(&ieeeflash64_6821, port, reg);
}

static uint8_t ieeeflash64_io1_peek(uint16_t addr)
{
    int port, reg;

    port = (addr >> 1) & 1; /* rs1 */
    reg = (addr >> 0) & 1;  /* rs0 */
    return mc6821core_peek(&ieeeflash64_6821, port, reg);
}

static int ieeeflash64_io1_dump(void)
{
    mon_out("PIA io1\n");
    mc6821core_dump(&ieeeflash64_6821);
    return 0;
}
/* ---------------------------------------------------------------------*/

static void pia_set_ca2(mc6821_state *ctx)
{
    parallel_cpu_set_atn((uint8_t)((ctx->CA2) ? 0 : 1));
}

static void pia_reset(void)
{
   /* assuming input after reset */
    parallel_cpu_set_atn(0);
    parallel_cpu_set_ndac(0);
    parallel_cpu_set_nrfd(0);
    parallel_cpu_set_dav(0);
    parallel_cpu_set_eoi(0);
    parallel_cpu_set_bus(0xff);
}

static uint8_t oldpa;

static void pia_set_pa(mc6821_state *ctx)
{
    if (ctx->dataA != oldpa) {
        parallel_cpu_set_bus(ctx->dataA);
        oldpa = ctx->dataA;
    }
}

static void pia_set_pb(mc6821_state *ctx)
{
    uint8_t tmp = ~(ctx->dataB);

    DBG(("IEEEFLASH64: PIA write port B %02x [EOI=%d DAV=%d NRFD=%d NDAC=%d]\n", tmp,
        tmp & PIA_EOI_BIT, tmp & PIA_DAV_BIT, tmp & PIA_NRFD_BIT, tmp & PIA_NDAC_BIT));

    parallel_cpu_set_eoi((uint8_t)(tmp & PIA_EOI_BIT & ctx->ddrB));
    parallel_cpu_set_dav((uint8_t)(tmp & PIA_DAV_BIT & ctx->ddrB));
    parallel_cpu_set_nrfd((uint8_t)(tmp & PIA_NRFD_BIT & ctx->ddrB));
    parallel_cpu_set_ndac((uint8_t)(tmp & PIA_NDAC_BIT & ctx->ddrB));
}

static uint8_t pia_get_pa(mc6821_state *ctx)
{
    uint8_t byte;

    drive_cpu_execute_all(maincpu_clk);

    byte = (parallel_bus & ~ctx->ddrA) | (ctx->dataA & ctx->ddrA);

#ifdef DEBUG
/*    if (debug.ieee) {
        printf("IEEEFlash64: read pia port A %x, parallel_bus=%x, gives %x.\n",
                    ctx->dataA, parallel_bus, (unsigned int)byte);
    } */
#endif

    return byte;
}

static uint8_t pia_get_pb(mc6821_state *ctx)
{
    uint8_t byte;

    drive_cpu_execute_all(maincpu_clk);

    byte = 0xf8;
    if (parallel_ndac) {
        byte &= ~PIA_NDAC_BIT;
    }
    if (parallel_nrfd) {
        byte &= ~PIA_NRFD_BIT;
    }
    if (parallel_dav) {
        byte &= ~PIA_DAV_BIT;
    }
    if (parallel_eoi) {
        byte &= ~PIA_EOI_BIT;
    }

    /* reflect device routing switches 0 = "off" 1 = "on" */
    byte |= ieeeflash64_switch8;
    byte |= ieeeflash64_switch910 << 1;
    byte |= ieeeflash64_switch4 << 2;

    return byte;
}

/* ------------------------------------------------------------------------- */

uint8_t ieeeflash64_romh_read_hirom(uint16_t addr)
{
    /*DBG(("IEEEFlash64(romh_read_hirom): %x\n", addr));*/
    uint8_t value = ieeeflash64_rom[(addr & 0x1fff)];
    /*DBG(("IEEEFlash64(romh_read_hirom): %04x returns %02x\n", addr, value));*/
    return value;
}

int ieeeflash64_romh_phi1_read(uint16_t addr, uint8_t *value)
{
    /*DBG(("IEEEFlash64(romh_phi1_read): %x\n", addr));*/
    return CART_READ_C64MEM;
}

int ieeeflash64_romh_phi2_read(uint16_t addr, uint8_t *value)
{
    /*DBG(("IEEEFlash64(romh_phi2_read): %x\n", addr));*/
    return ieeeflash64_romh_phi1_read(addr, value);
}

int ieeeflash64_peek_mem(uint16_t addr, uint8_t *value)
{
    if (addr >= 0xe000) {
        *value = ieeeflash64_rom[addr & 0x1fff];
        return CART_READ_VALID;
    }
    return CART_READ_THROUGH;
}

/* ------------------------------------------------------------------------- */

static int set_ieeeflash64_enabled(int value, void *param)
{
    /* value = enabled(1)/disabled(0); param = attach/detach(0) vs enable/disable(1) */
    /* i dont really get what should be happening when on attach vs. enable - CAB */
    int val = value ? 1 : 0;

    DBG(("IEEEFlash64: set_enabled: (%p) '%s' %d to %d\n", param, ieeeflash64_filename, ieeeflash64_enabled, val));
    if (ieeeflash64_enabled && !val) {
        /* enabled -> disabled */
        cart_power_off();
#ifdef DEBUGTPI
        if (ieeeflash64_io_list_item == NULL) {
            DBG(("IEEEFlash64: BUG: ieeeflash64_enabled == 1 and ieeeflash64_list_item == NULL ?!\n"));
        }
#endif
        lib_free(ieeeflash64_rom);
        ieeeflash64_rom = NULL;
        export_remove(&export_res);
        io_source_unregister(ieeeflash64_io_list_item);
        ieeeflash64_io_list_item = NULL;
        ieeeflash64_enabled = 0;
        DBG(("IEEEFlash64: set_enabled unregistered\n"));
    } else if (!ieeeflash64_enabled && val) {
        /* disabled -> enabled */
        if (ieeeflash64_rom == NULL) {
            ieeeflash64_rom = lib_malloc(IEEEFLASH64_ROM_SIZE);
        }
        if (param) {
            /* if the param is != NULL, then we should load the default image file */
            if (ieeeflash64_filename) {
                if (*ieeeflash64_filename) {
                    DBG(("IEEEFlash64: attach default image\n"));
                    if (cartridge_attach_image(CARTRIDGE_IEEEFLASH64, ieeeflash64_filename) < 0) {
                        DBG(("IEEEFlash64: set_enabled did not register (attach image failed)\n"));
                        lib_free(ieeeflash64_rom);
                        ieeeflash64_rom = NULL;
                        return -1;
                    }
                    /* ieeeflash64_enabled = 1; */ /* cartridge_attach_image will end up calling set_ieeeflash64_enabled again */
                    DBG(("IEEEFlash64: attach succeeded\n"));
                    return 0;
                }
            }
        } else {
            cart_power_off();
            /* if the param is == NULL, then we should actually set the resource */
            if (export_add(&export_res) < 0) {
                DBG(("IEEEFlash64: set_enabled did not register (export add failed)\n"));
                lib_free(ieeeflash64_rom);
                ieeeflash64_rom = NULL;
                return -1;
            } else {
                DBG(("IEEEFlash64: set_enabled registered\n"));
                ieeeflash64_io_list_item = io_source_register(&ieeeflash64_io1_device);
                ieeeflash64_enabled = 1;
            }
        }
    }

    DBG(("IEEEFlash64: set_enabled done: (%p) '%s' %d : %d\n", ieeeflash64_rom, ieeeflash64_filename, val, ieeeflash64_enabled));
#ifdef DEBUGTPI
    if (ieeeflash64_rom != NULL) {
        DBG(("IEEEFlash64: rom is defined, fffc=%02x%02x\n", ieeeflash64_rom[0x1ffd], ieeeflash64_rom[0x1ffc]));
    }
#endif
    return 0;
}

/* ---------------------------------------------------------------------*/

void ieeeflash64_passthrough_changed(export_t *ex)
{
    ieeeflash64_extexrom = ex->exrom;
    ieeeflash64_extgame  = ex->game;
    DBG(("IEEE Flash 64 passthrough changed exrom: %d game: %d\n", ieeeflash64_extexrom, ieeeflash64_extgame));

    /* cart_set_port_game_slot0(ieeeflash64_extgame); */
    cart_port_config_changed_slot0();
}

int ieeeflash64_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit)
{
    if (addr >= 0xe000) {
        /* DBG(("IEEEFlash64: MMU XLATE %x\n", addr)); */
        *base = ieeeflash64_rom - 0xe000;
        *start = 0xe000;
        *limit = 0xfffd;
        return CART_READ_VALID;
    } else {
        return CART_READ_THROUGH;
    }
}

void ieeeflash64_config_init(export_t *ex)
{
    DBG(("ieeeflash64: ieeeflash64_config_init\n"));

    ieeeflash64_extexrom = (int)ex->exrom;
    ieeeflash64_extgame = (int)ex->game;

    /*
    cart_set_port_exrom_slot0(1);
    cart_set_port_game_slot0(ieeeflash64_extgame);
    */
    /*cart_port_config_changed_slot0();*/
    /* cart_config_changed_slotmain(CMODE_RAM, CMODE_ULTIMAX, CMODE_READ); */
    cart_config_changed_slot0(CMODE_RAM, CMODE_ULTIMAX, CMODE_READ);
#ifdef DEBUGTPI
    if (ieeeflash64_rom != NULL) {
        DBG(("IEEEFlash64: rom is defined, fffc=%02x%02x\n", ieeeflash64_rom[0x1ffd], ieeeflash64_rom[0x1ffc]));
    }
#endif

    /* stop 6821 from calling CA2 during reset */
    ieeeflash64_6821.set_ca2 = NULL;
    ieeeflash64_6821.set_pa = NULL;
    ieeeflash64_6821.set_pb = NULL;
    ieeeflash64_6821.get_pa = NULL;
    ieeeflash64_6821.get_pb = NULL;
    mc6821core_reset(&ieeeflash64_6821);
    ieeeflash64_6821.set_ca2 = pia_set_ca2;
    ieeeflash64_6821.set_pa = pia_set_pa;
    ieeeflash64_6821.set_pb = pia_set_pb;
    ieeeflash64_6821.get_pa = pia_get_pa;
    ieeeflash64_6821.get_pb = pia_get_pb;
    pia_reset();
}

void ieeeflash64_config_setup(uint8_t *rawcart)
{
    DBG(("ieeeflash64: config_setup\n"));
    memcpy(ieeeflash64_rom, rawcart, IEEEFLASH64_ROM_SIZE);
    /* cart_config_changed_slotmain(2, 3, CMODE_READ); */
#ifdef DEBUGTPI
    if (ieeeflash64_rom != NULL) {
        DBG(("IEEEFlash64: rom is defined, fffc=%02x%02x\n", ieeeflash64_rom[0x1ffd], ieeeflash64_rom[0x1ffc]));
    }
#endif
}

static int ieeeflash64_common_attach(void)
{
    DBG(("ieeeflash64: ieeeflash64_common_attach\n"));
    return set_ieeeflash64_enabled(1, NULL);
}

int ieeeflash64_bin_attach(const char *filename, uint8_t *rawcart)
{
    DBG(("ieeeflash64: ieeeflash64_bin_attach\n"));

    if (util_file_load(filename, rawcart, IEEEFLASH64_ROM_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return ieeeflash64_common_attach();
}

int ieeeflash64_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.size != IEEEFLASH64_ROM_SIZE) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    return ieeeflash64_common_attach();
}

void ieeeflash64_detach(void)
{
    set_ieeeflash64_enabled(0, NULL);
}

int ieeeflash64_enable(void)
{
    return set_ieeeflash64_enabled(1, (void*)1);
}

int ieeeflash64_disable(void)
{
    return set_ieeeflash64_enabled(0, (void*)1);
}

void ieeeflash64_reset(void)
{
    /* cart_config_changed_slot0(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ); */
}

/* ---------------------------------------------------------------------*/

/* CARTIEEEFLASH64 snapshot module format:

   type  | name | description
   --------------------------
   ARRAY | ROMH | 8192 BYTES of ROMH data
 */
/* TODO: record more state */
static const char snap_module_name[] = "CARTIEEEFLASH64";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int ieeeflash64_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (SMW_BA(m, ieeeflash64_rom, 0x2000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (mc6821core_snapshot_write_data(&ieeeflash64_6821, m) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int ieeeflash64_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    set_ieeeflash64_enabled(1, NULL);

    if (SMR_BA(m, ieeeflash64_rom, 0x2000) < 0) {
        goto fail;
    }

    if (mc6821core_snapshot_read_data(&ieeeflash64_6821, m) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return ieeeflash64_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
