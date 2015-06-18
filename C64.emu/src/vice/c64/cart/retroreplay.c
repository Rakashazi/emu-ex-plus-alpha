/*
 * retroreplay.c - Cartridge handling, Retro Replay cart.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  groepaz <groepaz@gmx.net>
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
#include "c64cart.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64export.h"
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "crt.h"
#include "flash040.h"
#include "lib.h"
#include "maincpu.h"
#include "resources.h"
#include "translate.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "retroreplay.h"
#include "reu.h"
#ifdef HAVE_TFE
#include "tfe.h"
#endif
#undef CARTRIDGE_INCLUDE_PRIVATE_API

/*
    Retro Replay (Individual Computers)

    64K rom, 8*8k pages (actually 128K Flash ROM, one of two 64K banks selected by bank jumper)
    32K ram, 4*8k pages

    io1
        - registers at de00/de01
        - cart RAM (if enabled) or cart ROM

    io2
        - cart RAM (if enabled) or cart ROM

    Bank Jumper    Flashtool  Physical

    set            Bank2      Bank 0,0x00000
    not set        Bank1      Bank 1,0x10000

*/

/* #define DEBUGRR */

#ifdef DEBUGRR
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

/* Cart is activated.  */
static int rr_active;
int rr_clockport_enabled;

/* current bank */
static int rr_bank;

/* Only one write access is allowed.  */
static int write_once;

/* RAM bank switching allowed.  */
static int allow_bank;

/* Freeze is disallowed.  */
static int no_freeze;

/* REU compatibility mapping.  */
static int reu_mapping;

static int rr_hw_flashjumper = 0;
static int rr_hw_bankjumper = 0;
static int rr_bios_write = 0;
static int rr_revision = 0;
static int export_ram_at_a000 = 0;

static unsigned int rom_offset = 0x10000;

/* the 29F010 statemachine */
static flash040_context_t *flashrom_state = NULL;

static char *retroreplay_filename = NULL;
static int retroreplay_filetype = 0;

static const char STRING_RETRO_REPLAY[] = CARTRIDGE_NAME_RETRO_REPLAY;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static BYTE retroreplay_io1_read(WORD addr);
static void retroreplay_io1_store(WORD addr, BYTE value);
static BYTE retroreplay_io2_read(WORD addr);
static void retroreplay_io2_store(WORD addr, BYTE value);

static io_source_t retroreplay_io1_device = {
    CARTRIDGE_NAME_RETRO_REPLAY,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0,
    retroreplay_io1_store,
    retroreplay_io1_read,
    NULL, /* TODO: peek */
    NULL, /* TODO: dump */
    CARTRIDGE_RETRO_REPLAY,
    0,
    0
};

static io_source_t retroreplay_io2_device = {
    CARTRIDGE_NAME_RETRO_REPLAY,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    0,
    retroreplay_io2_store,
    retroreplay_io2_read,
    NULL, /* TODO: peek */
    NULL, /* TODO: dump */
    CARTRIDGE_RETRO_REPLAY,
    0,
    0
};

static io_source_list_t *retroreplay_io1_list_item = NULL;
static io_source_list_t *retroreplay_io2_list_item = NULL;

static const c64export_resource_t export_res = {
    CARTRIDGE_NAME_RETRO_REPLAY, 1, 1, &retroreplay_io1_device, &retroreplay_io2_device, CARTRIDGE_RETRO_REPLAY
};

/* ---------------------------------------------------------------------*/

BYTE retroreplay_io1_read(WORD addr)
{
    retroreplay_io1_device.io_source_valid = 0;

/* DBG(("io1 r %04x\n",addr)); */

    if (rr_active) {
        switch (addr & 0xff) {
            /*
                $de00 read or $de01 read:
                    Bit 0: 1=Flashmode active (jumper set)
                    Bit 1: feedback of AllowBank bit
                    Bit 2: 1=Freeze button pressed
                    Bit 3: feedback of banking bit 13
                    Bit 4: feedback of banking bit 14
                    Bit 5: feedback of banking bit 16
                    Bit 6: 1=REU compatible memory map active
                    Bit 7: feedback of banking bit 15
             */
            case 0:
            case 1:
                retroreplay_io1_device.io_source_valid = 1;
                return ((roml_bank & 3) << 3) | ((roml_bank & 4) << 5) | ((roml_bank & 8) << 2) | allow_bank | reu_mapping | rr_hw_flashjumper;
            default:
#ifdef HAVE_TFE
                if (rr_clockport_enabled && tfe_cart_enabled() && tfe_as_rr_net && (addr & 0xff) < 0x10) {
                    return 0;
                }
#endif
                if (reu_mapping) {
                    retroreplay_io1_device.io_source_valid = 1;
                    if (export_ram) {
                        if (allow_bank) {
                            return export_ram0[0x1e00 + (addr & 0xff) + ((roml_bank & 3) << 13)];
                        } else {
                            return export_ram0[0x1e00 + (addr & 0xff)];
                        }
                    }

                    return flash040core_read(flashrom_state, rom_offset + ((addr | 0xde00) & 0x1fff) + (roml_bank << 13));
                }
        }
    }
    return 0;
}

void retroreplay_io1_store(WORD addr, BYTE value)
{
    int mode = CMODE_WRITE, cmode;

    DBG(("io1 w %04x %02x\n", addr, value));

    if (rr_active) {
        switch (addr & 0xff) {
            /*
                $de00 write:

                This register is reset to $00 on a hard reset if not in flash mode.

                If in flash mode, it is set to $02 in order to prevent the computer
                from starting the normal cartridge. Flash mode is selected with a jumper.

                Bit 0 controls the GAME line: A 1 asserts the line, a 0 will deassert it.
                Bit 1 controls the EXROM line: A 0 will assert it, a 1 will deassert it.
                Bit 2 Writing a 1 to bit 2 will disable further write accesses to all
                    registers of Retro Replay, and set the memory map of the C-64
                    to standard, as if there is no cartridge installed at all.
                Bit 3 controls bank-address 13 for ROM and RAM banking
                Bit 4 controls bank-address 14 for ROM and RAM banking
                Bit 5 switches between ROM and RAM: 0=ROM, 1=RAM
                Bit 6 must be written once to "1" after a successful freeze in
                    order to set the correct memory map and enable Bits 0 and 1
                    of this register. Otherwise no effect.
                Bit 7 controls bank-address 15 for ROM banking
             */
            case 0:
                rr_bank = ((value >> 3) & 3) | ((value >> 5) & 4); /* bit 3-4, 7 */
                cmode = (value & 3);  /* bit 0-1 */
                if ((rr_revision > 0) && ((value & 0xe7) == 0x22)) {
                    /* Nordic Replay supports additional Nordic Power compatible values */
                    cmode = 1; /* 16k Game */
                    export_ram_at_a000 = 1; /* RAM at a000 enabled */
                } else {
                    /* Action Replay 5 compatible values */
                    export_ram_at_a000 = 0;
                    if (value & 0x40) { /* bit 6 */
                        mode |= CMODE_RELEASE_FREEZE;
                    }
                    if (value & 0x20) { /* bit 5 */
                        mode |= CMODE_EXPORT_RAM;
                    }
                }
#if 0
                if (rr_hw_flashjumper) {
                    /* FIXME: what exactly is really happening ? */
                    if ((value & 3) == 3) {
                        value = 0;
                    } else if ((value & 3) == 1) {
                        value = 0;
                    }
                }
#endif
                cart_config_changed_slotmain(0, (BYTE)(cmode | (rr_bank << CMODE_BANK_SHIFT)), mode);

                if (value & 4) { /* bit 2 */
                    rr_active = 0;
                }
                break;
            /*
                $de01 write: Extended control register.

                    If not in Flash mode, bits 1, 2 and 6 can only be written once.
                    Bit 5 is always set to 0 if not in flash mode.

                    If in Flash mode, the REUcomp bit cannot be set, but the register
                    will not be disabled by the first write.

                    Bit 0: enable accessory connector. See further down.
                    Bit 1: AllowBank  (1 allows banking of RAM in $df00/$de02 area)
                    Bit 2: NoFreeze   (1 disables Freeze function)
                    Bit 3: bank-address 13 for RAM and ROM (mirror of $de00)
                    Bit 4: bank-address 14 for RAM and ROM (mirror of $de00)
                    Bit 5: bank-address 16 for ROM (only in flash mode)
                    Bit 6: REU compatibility bit. 0=standard memory map
                                                1=REU compatible memory map
                    Bit 7: bank-address 15 for ROM (mirror of $de00)
             */
            case 1:
                if (rr_hw_flashjumper) {
                    if (rr_hw_bankjumper) {
                        rr_bank = ((value >> 3) & 3) | ((value >> 5) & 4) | (((value >> 2) & 8) ^ 8);
                    } else {
                        rr_bank = ((value >> 3) & 3) | ((value >> 5) & 4);
                    }
                    cart_romhbank_set_slotmain(rr_bank);
                    cart_romlbank_set_slotmain(rr_bank);
                    allow_bank = value & 2;
                    no_freeze = value & 4;
                    reu_mapping = 0; /* can not be set in flash mode */
                    cart_port_config_changed_slotmain();
                } else {
                    if (write_once == 0) {
                        rr_bank = ((value >> 3) & 3) | ((value >> 5) & 4);
                        cart_romhbank_set_slotmain(rr_bank);
                        cart_romlbank_set_slotmain(rr_bank);
                        allow_bank = value & 2;
                        no_freeze = value & 4;
                        reu_mapping = value & 0x40;
                        if (rr_clockport_enabled != (value & 1)) {
                            rr_clockport_enabled = value & 1;
#ifdef HAVE_TFE
                            tfe_clockport_changed();
#endif
                        }
                        write_once = 1;
                        cart_port_config_changed_slotmain();
                    }
                }
                break;
            default:
#ifdef HAVE_TFE
                if (rr_clockport_enabled && tfe_cart_enabled() && tfe_as_rr_net && (addr & 0xff) < 0x10) {
                    return;
                }
#endif
                if (reu_mapping) {
                    if (export_ram) {
                        if (allow_bank) {
                            export_ram0[0x1e00 + (addr & 0xff) + ((roml_bank & 3) << 13)] = value;
                        } else {
                            export_ram0[0x1e00 + (addr & 0xff)] = value;
                        }
                    }
                }
        }
    }
}

BYTE retroreplay_io2_read(WORD addr)
{
    retroreplay_io2_device.io_source_valid = 0;

    DBG(("io2 r %04x\n", addr));

    if (rr_active) {
        if (!reu_mapping) {
            retroreplay_io2_device.io_source_valid = 1;
            if (export_ram || ((rr_revision > 0) && export_ram_at_a000)) {
                if (allow_bank) {
                    return export_ram0[0x1f00 + (addr & 0xff) + ((roml_bank & 3) << 13)];
                } else {
                    return export_ram0[0x1f00 + (addr & 0xff)];
                }
            }

            return flash040core_read(flashrom_state, rom_offset + ((addr | 0xdf00) & 0x1fff) + (roml_bank << 13));
        }
    }
    return 0;
}

void retroreplay_io2_store(WORD addr, BYTE value)
{
    DBG(("io2 w %04x %02x\n", addr, value));

    if (rr_active) {
        if (!reu_mapping) {
            if (export_ram) {
                if (allow_bank) {
                    export_ram0[0x1f00 + (addr & 0xff) + ((roml_bank & 3) << 13)] = value;
                } else {
                    export_ram0[0x1f00 + (addr & 0xff)] = value;
                }
            }
        }
    }
}

/* ---------------------------------------------------------------------*/

BYTE retroreplay_roml_read(WORD addr)
{
    if (export_ram) {
        return export_ram0[(addr & 0x1fff) + ((roml_bank & 3) << 13)];
    }

    return flash040core_read(flashrom_state, rom_offset + (addr & 0x1fff) + (roml_bank << 13));
}

void retroreplay_roml_store(WORD addr, BYTE value)
{
/*    DBG(("roml w %04x %02x ram:%d flash:%d\n", addr, value, export_ram, rr_hw_flashjumper)); */
    if (export_ram) {
        export_ram0[(addr & 0x1fff) + ((roml_bank & 3) << 13)] = value;
    } else {
        /* writes to flash are completely disabled if the flash jumper is not set */
        if (rr_hw_flashjumper) {
            flash040core_store(flashrom_state, rom_offset + (addr & 0x1fff) + (roml_bank << 13), value);
            if (flashrom_state->flash_state != FLASH040_STATE_READ) {
                maincpu_resync_limits();
            }
        }
    }
}

int retroreplay_roml_no_ultimax_store(WORD addr, BYTE value)
{
/*    DBG(("roml w %04x %02x ram:%d flash:%d\n", addr, value, export_ram, rr_hw_flashjumper)); */
    if (rr_hw_flashjumper) {
        if (export_ram) {
            export_ram0[(addr & 0x1fff) + ((roml_bank & 3) << 13)] = value;
            return 1;
        } else {
            /* writes to flash are completely disabled if the flash jumper is not set */
            flash040core_store(flashrom_state, rom_offset + (addr & 0x1fff) + (roml_bank << 13), value);
            if (flashrom_state->flash_state != FLASH040_STATE_READ) {
                maincpu_resync_limits();
            }
        }
    }
    return 0;
}

BYTE retroreplay_romh_read(WORD addr)
{
    if ((rr_revision > 0) && export_ram_at_a000) {
        return export_ram0[addr & 0x1fff]; /* FIXME: bank ? */
    }
    return flash040core_read(flashrom_state, rom_offset + (addr & 0x1fff) + (roml_bank << 13));
}

void retroreplay_romh_store(WORD addr, BYTE value)
{
    if ((rr_revision > 0) && export_ram_at_a000) {
        export_ram0[addr & 0x1fff] = value; /* FIXME: bank ? */
    }
}

int retroreplay_peek_mem(struct export_s *export, WORD addr, BYTE *value)
{
    if (addr >= 0x8000 && addr <= 0x9fff) {
        *value = retroreplay_roml_read(addr);
        return CART_READ_VALID;
    }
    if (!(((export_t*)export)->exrom) && (((export_t*)export)->game)) {
        if (addr >= 0xe000) {
            *value = retroreplay_romh_read(addr);
            return CART_READ_VALID;
        }
    } else {
        if (addr >= 0xa000 && addr <= 0xbfff) {
            *value = retroreplay_romh_read(addr);
            return CART_READ_VALID;
        }
    }
    return CART_READ_THROUGH;
}

void retroreplay_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit)
{
    switch (addr & 0xe000) {
        case 0xe000:
            if ((rr_revision > 0) && export_ram_at_a000) {
                *base = export_ram0 - 0xe000; /* FIXME: bank ? */
                *start = 0xe000;
                *limit = 0xfffd;
                return;
            }
            if (flashrom_state->flash_state == FLASH040_STATE_READ) {
                *base = flashrom_state->flash_data + rom_offset + (roml_bank << 13) - 0xe000;
                *start = 0xe000;
                *limit = 0xfffd;
                return;
            }
            break;
        case 0xa000:
            if ((rr_revision > 0) && export_ram_at_a000) {
                *base = export_ram0 - 0xa000; /* FIXME: bank ? */
                *start = 0xa000;
                *limit = 0xbffd;
                return;
            }
            if (flashrom_state->flash_state == FLASH040_STATE_READ) {
                *base = flashrom_state->flash_data + rom_offset + (roml_bank << 13) - 0xa000;
                *start = 0xa000;
                *limit = 0xbffd;
                return;
            }
            break;
        case 0x8000:
            if (export_ram) {
                *base = export_ram0 + ((roml_bank & 3) << 13) - 0x8000;
                *start = 0x8000;
                *limit = 0x9ffd;
                return;
            }
            if (flashrom_state->flash_state == FLASH040_STATE_READ) {
                *base = flashrom_state->flash_data + rom_offset + (roml_bank << 13) - 0x8000;
                *start = 0x8000;
                *limit = 0x9ffd;
                return;
            }
            break;
        default:
            break;
    }
    *base = NULL;
    *start = 0;
    *limit = 0;
}

/* ---------------------------------------------------------------------*/

void retroreplay_freeze(void)
{
    /* freeze button is disabled in flash mode */
    if (!rr_hw_flashjumper) {
        rr_active = 1;
        cart_config_changed_slotmain(3, 3, CMODE_READ | CMODE_EXPORT_RAM);
        /* flash040core_reset(flashrom_state); */
    }
}

int retroreplay_freeze_allowed(void)
{
    if (no_freeze) {
        return 0;
    }
    return 1;
}

void retroreplay_config_init(void)
{
    DBG(("retroreplay_config_init flash:%d bank jumper: %d offset: %08x\n", rr_hw_flashjumper, rr_hw_bankjumper, rom_offset));

    rr_active = 1;
    rr_clockport_enabled = 0;
    write_once = 0;
    no_freeze = 0;
    reu_mapping = 0;
    allow_bank = 0;
    export_ram_at_a000 = 0;

    if (rr_hw_flashjumper) {
        cart_config_changed_slotmain(2, 2, CMODE_READ);
    } else {
        cart_config_changed_slotmain(0, 0, CMODE_READ);
    }

    flash040core_reset(flashrom_state);
}

void retroreplay_reset(void)
{
    DBG(("retroreplay_reset flash:%d bank jumper: %d offset: %08x\n", rr_hw_flashjumper, rr_hw_bankjumper, rom_offset));
    rr_active = 1;

    if (rr_hw_flashjumper) {
        cart_config_changed_slotmain(2, 2, CMODE_READ);
    } else {
        cart_config_changed_slotmain(0, 0, CMODE_READ);
    }

    /* on the real hardware pressing reset would NOT reset the flash statemachine,
       only a powercycle would help. we do it here anyway :)
    */
    flash040core_reset(flashrom_state);
}

void retroreplay_config_setup(BYTE *rawcart)
{
    DBG(("retroreplay_config_setup bank jumper: %d offset: %08x\n", rr_hw_bankjumper, rom_offset));

    if (rr_hw_flashjumper) {
        cart_config_changed_slotmain(2, 2, CMODE_READ);
    } else {
        cart_config_changed_slotmain(0, 0, CMODE_READ);
    }

    flashrom_state = lib_malloc(sizeof(flash040_context_t));
    flash040core_init(flashrom_state, maincpu_alarm_context, FLASH040_TYPE_010, roml_banks);
    /* the logical bank 0 is the physical bank 1 */
    memcpy(flashrom_state->flash_data, &rawcart[0x10000], 0x10000);
    memcpy(&flashrom_state->flash_data[0x10000], rawcart, 0x10000);
}

int retroreplay_cart_enabled(void)
{
    return rr_active;
}

/* ---------------------------------------------------------------------*/

static int set_rr_revision(int val, void *param)
{
    rr_revision = val ? 1 : 0;

    if (rr_active) {
        maincpu_resync_limits();
    }
    return 0;
}

static int set_rr_flashjumper(int val, void *param)
{
    rr_hw_flashjumper = val ? 1 : 0;
    DBG(("set_rr_flashjumper: %d\n", rr_hw_flashjumper));
    return 0;
}

/*
 "If the bank-select jumper is not set, you only have access to the upper 64K of the Flash"
*/

static int set_rr_bankjumper(int val, void *param)
{
    /* if the jumper is set, physical bank 0 is selected */
    rr_hw_bankjumper = val ? 1 : 0;

    if (rr_hw_bankjumper) {
        rom_offset = 0x0;
    } else {
        rom_offset = 0x10000;
    }
    if (rr_active) {
        maincpu_resync_limits();
    }
    DBG(("bank jumper: %d offset: %08x\n", rr_hw_bankjumper, rom_offset));
    return 0;
}

static int set_rr_bios_write(int val, void *param)
{
    rr_bios_write = val ? 1 : 0;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "RRFlashJumper", 0, RES_EVENT_NO, NULL,
      &rr_hw_flashjumper, set_rr_flashjumper, NULL },
    { "RRBankJumper", 0, RES_EVENT_NO, NULL,
      &rr_hw_bankjumper, set_rr_bankjumper, NULL },
    { "RRBiosWrite", 0, RES_EVENT_NO, NULL,
      &rr_bios_write, set_rr_bios_write, NULL },
    { "RRrevision", RR_REV_RETRO_REPLAY, RES_EVENT_NO, NULL,
      &rr_revision, set_rr_revision, NULL },
    { NULL }
};

int retroreplay_resources_init(void)
{
    return resources_register_int(resources_int);
}

void retroreplay_resources_shutdown(void)
{
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-rrbioswrite", SET_RESOURCE, 0,
      NULL, NULL, "RRBiosWrite", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_SAVE_RR_ROM_AT_EXIT,
      NULL, NULL },
    { "+rrbioswrite", SET_RESOURCE, 0,
      NULL, NULL, "RRBiosWrite", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_SAVE_RR_ROM_AT_EXIT,
      NULL, NULL },
    { "-rrbankjumper", SET_RESOURCE, 0,
      NULL, NULL, "RRBankJumper", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_SET_RR_BANK_JUMPER,
      NULL, NULL },
    { "+rrbankjumper", SET_RESOURCE, 0,
      NULL, NULL, "RRBankJumper", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_UNSET_RR_BANK_JUMPER,
      NULL, NULL },
    { "-rrflashjumper", SET_RESOURCE, 0,
      NULL, NULL, "RRFlashJumper", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_SET_RR_FLASH_JUMPER,
      NULL, NULL },
    { "+rrflashjumper", SET_RESOURCE, 0,
      NULL, NULL, "RRFlashJumper", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_UNSET_RR_FLASH_JUMPER,
      NULL, NULL },
    { "-rrrev", SET_RESOURCE, 1,
      NULL, NULL, "RRrevision", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_REVISION, IDCLS_RR_REVISION,
      NULL, NULL },
    { NULL }
};

int retroreplay_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

static int retroreplay_common_attach(void)
{
    if (c64export_add(&export_res) < 0) {
        return -1;
    }

    retroreplay_io1_list_item = io_source_register(&retroreplay_io1_device);
    retroreplay_io2_list_item = io_source_register(&retroreplay_io2_device);

    return 0;
}

int retroreplay_bin_attach(const char *filename, BYTE *rawcart)
{
    int len = 0;
    FILE *fd;

    retroreplay_filetype = 0;
    retroreplay_filename = NULL;

    fd = fopen(filename, MODE_READ);
    if (fd == NULL) {
        return -1;
    }
    len = util_file_length(fd);
    fclose(fd);

    memset(rawcart, 0xff, 0x20000);

    /* we accept 32k, 64k and full 128k images */
    switch (len) {
        case 0x8000: /* 32K */
            if (util_file_load(filename, rawcart, 0x8000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
                return -1;
            }
            break;
        case 0x10000: /* 64K */
            if (util_file_load(filename, rawcart, 0x10000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
                return -1;
            }
            break;
        case 0x20000: /* 128K */
            if (util_file_load(filename, rawcart, 0x20000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
                return -1;
            }
            break;
        default:
            return -1;
    }
    retroreplay_filetype = CARTRIDGE_FILETYPE_BIN;
    retroreplay_filename = lib_stralloc(filename);
    return retroreplay_common_attach();
}

/*
    a CRT may contain up to 16 8k chunks. 32K, 64K and 128K total are accepted.
    - 32K and 64K files will always get loaded into logical bank 0
*/
int retroreplay_crt_attach(FILE *fd, BYTE *rawcart, const char *filename)
{
    crt_chip_header_t chip;
    int i;

    memset(rawcart, 0xff, 0x20000);

    retroreplay_filetype = 0;
    retroreplay_filename = NULL;

    for (i = 0; i <= 15; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.bank > 15 || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }

    if ((i != 4) && (i != 8) && (i != 16)) {
        return -1;
    }

    retroreplay_filetype = CARTRIDGE_FILETYPE_CRT;
    retroreplay_filename = lib_stralloc(filename);

    return retroreplay_common_attach();
}

/*
    saving will create either 64k or 128k files, depending on the state
    of the flash chip.
*/

static int checkempty(int bank)
{
    int i;
    bank *= 0x10000;
    for (i = 0; i < 0x10000; i++) {
        if (roml_banks[i + bank] != 0xff) {
            return 0;
        }
    }
    return 1;
}

int retroreplay_bin_save(const char *filename)
{
    FILE *fd;

    if (filename == NULL) {
        return -1;
    }

    fd = fopen(filename, MODE_WRITE);

    if (fd == NULL) {
        return -1;
    }

    if (!checkempty(1)) {
        if (fwrite(&roml_banks[0x10000], 1, 0x10000, fd) != 0x10000) {
            fclose(fd);
            return -1;
        }
    }

    if (!checkempty(0)) {
        if (fwrite(&roml_banks[0x00000], 1, 0x10000, fd) != 0x10000) {
            fclose(fd);
            return -1;
        }
    }

    fclose(fd);

    return 0;
}

int retroreplay_crt_save(const char *filename)
{
    FILE *fd;
    crt_chip_header_t chip;
    BYTE *data;
    int i;

    fd = crt_create(filename, CARTRIDGE_RETRO_REPLAY, 1, 0, STRING_RETRO_REPLAY);

    if (fd == NULL) {
        return -1;
    }

    chip.type = 2;
    chip.size = 0x2000;
    chip.start = 0x8000;

    if (!checkempty(1)) {
        data = &roml_banks[0x10000];

        for (i = 0; i < 8; i++) {
            chip.bank = i; /* bank */

            if (crt_write_chip(data, &chip, fd)) {
                fclose(fd);
                return -1;
            }
            data += 0x2000;
        }
    }

    if (!checkempty(0)) {
        data = &roml_banks[0x00000];

        for (i = 0; i < 8; i++) {
            chip.bank = 8 + i; /* bank */

            if (crt_write_chip(data, &chip, fd)) {
                fclose(fd);
                return -1;
            }
            data += 0x2000;
        }
    }
    fclose(fd);
    return 0;
}

int retroreplay_flush_image(void)
{
    if (retroreplay_filetype == CARTRIDGE_FILETYPE_BIN) {
        return retroreplay_bin_save(retroreplay_filename);
    } else if (retroreplay_filetype == CARTRIDGE_FILETYPE_CRT) {
        return retroreplay_crt_save(retroreplay_filename);
    }
    return -1;
}

void retroreplay_detach(void)
{
    if (rr_bios_write && flashrom_state->flash_dirty) {
        retroreplay_flush_image();
    }

    flash040core_shutdown(flashrom_state);
    lib_free(flashrom_state);
    flashrom_state = NULL;
    lib_free(retroreplay_filename);
    retroreplay_filename = NULL;
    c64export_remove(&export_res);
    io_source_unregister(retroreplay_io1_list_item);
    io_source_unregister(retroreplay_io2_list_item);
    retroreplay_io1_list_item = NULL;
    retroreplay_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   1
#define SNAP_MODULE_NAME  "CARTRR"
#define FLASH_SNAP_MODULE_NAME  "FLASH040RR"

int retroreplay_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)rr_revision) < 0)
        || (SMW_B(m, (BYTE)rr_active) < 0)
        || (SMW_B(m, (BYTE)rr_clockport_enabled) < 0)
        || (SMW_B(m, (BYTE)rr_bank) < 0)
        || (SMW_B(m, (BYTE)write_once) < 0)
        || (SMW_B(m, (BYTE)allow_bank) < 0)
        || (SMW_B(m, (BYTE)no_freeze) < 0)
        || (SMW_B(m, (BYTE)reu_mapping) < 0)
        || (SMW_B(m, (BYTE)export_ram_at_a000) < 0)
        || (SMW_B(m, (BYTE)rr_hw_flashjumper) < 0)
        || (SMW_B(m, (BYTE)rr_hw_bankjumper) < 0)
        || (SMW_DW(m, (DWORD)rom_offset) < 0)
        || (SMW_BA(m, roml_banks, 0x20000) < 0)
        || (SMW_BA(m, export_ram0, 0x8000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (0
        || (flash040core_snapshot_write_module(s, flashrom_state, FLASH_SNAP_MODULE_NAME) < 0)) {
        return -1;
    }

    return 0;
}

int retroreplay_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;
    DWORD temp_rom_offset;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CART_DUMP_VER_MAJOR) || (vminor != CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    if (0
        || (SMR_B_INT(m, &rr_revision) < 0)
        || (SMR_B_INT(m, &rr_active) < 0)
        || (SMR_B_INT(m, &rr_clockport_enabled) < 0)
        || (SMR_B_INT(m, &rr_bank) < 0)
        || (SMR_B_INT(m, &write_once) < 0)
        || (SMR_B_INT(m, &allow_bank) < 0)
        || (SMR_B_INT(m, &no_freeze) < 0)
        || (SMR_B_INT(m, &reu_mapping) < 0)
        || (SMR_B_INT(m, &export_ram_at_a000) < 0)
        || (SMR_B_INT(m, &rr_hw_flashjumper) < 0)
        || (SMR_B_INT(m, &rr_hw_bankjumper) < 0)
        || (SMR_DW(m, &temp_rom_offset) < 0)
        || (SMR_BA(m, roml_banks, 0x20000) < 0)
        || (SMR_BA(m, export_ram0, 0x8000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    rom_offset = temp_rom_offset;

    flashrom_state = lib_malloc(sizeof(flash040_context_t));

    flash040core_init(flashrom_state, maincpu_alarm_context, FLASH040_TYPE_010, roml_banks);

    if (0
        || (flash040core_snapshot_read_module(s, flashrom_state, FLASH_SNAP_MODULE_NAME) < 0)) {
        flash040core_shutdown(flashrom_state);
        lib_free(flashrom_state);
        flashrom_state = NULL;
        return -1;
    }

    retroreplay_common_attach();

    /* set filetype to none */
    retroreplay_filename = NULL;
    retroreplay_filetype = 0;

    return 0;
}
