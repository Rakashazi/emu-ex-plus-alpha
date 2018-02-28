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
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "clockport.h"
#include "cmdline.h"
#include "crt.h"
#include "export.h"
#include "flash040.h"
#include "lib.h"
#include "maincpu.h"
#include "monitor.h"
#include "resources.h"
#include "translate.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "vicii-phi1.h"

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "retroreplay.h"
#include "reu.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API

/*
    Retro Replay, Nordic Replay (Individual Computers)

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
static int rr_active = 0;
static int rr_clockport_enabled = 0;

static int rr_enabled = 0;

/* freeze logic state */
static int rr_frozen = 0;

/* current GAME/EXROM mode */
static int rr_cmode = CMODE_8KGAME;

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

/* Current clockport device */
static int clockport_device_id = CLOCKPORT_DEVICE_NONE;
static clockport_device_t *clockport_device = NULL;

static char *clockport_device_names = NULL;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static BYTE retroreplay_io1_read(WORD addr);
static void retroreplay_io1_store(WORD addr, BYTE value);
static BYTE retroreplay_io2_read(WORD addr);
static void retroreplay_io2_store(WORD addr, BYTE value);
static int retroreplay_dump(void);

static BYTE retroreplay_clockport_read(WORD io_address);
static BYTE retroreplay_clockport_peek(WORD io_address);
static void retroreplay_clockport_store(WORD io_address, BYTE byte);

static io_source_t retroreplay_io1_device = {
    CARTRIDGE_NAME_RETRO_REPLAY,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0,
    retroreplay_io1_store,
    retroreplay_io1_read,
    NULL, /* TODO: peek */
    retroreplay_dump,
    CARTRIDGE_RETRO_REPLAY,
    1,
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
    retroreplay_dump,
    CARTRIDGE_RETRO_REPLAY,
    0,
    0
};

static io_source_t retroreplay_clockport_io1_device = {
    CARTRIDGE_NAME_RRNET " on " CARTRIDGE_NAME_RETRO_REPLAY " Clockport",
    IO_DETACH_RESOURCE,
    "RRClockPort",
    0xde02, 0xde0f, 0x0f,
    0,
    retroreplay_clockport_store,
    retroreplay_clockport_read,
    retroreplay_clockport_peek,
    retroreplay_dump,
    CARTRIDGE_RETRO_REPLAY,
    0,
    0
};

static io_source_list_t *retroreplay_io1_list_item = NULL;
static io_source_list_t *retroreplay_io2_list_item = NULL;
static io_source_list_t *retroreplay_clockport_io1_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_RETRO_REPLAY, 1, 1, &retroreplay_io1_device, &retroreplay_io2_device, CARTRIDGE_RETRO_REPLAY
};

/* ---------------------------------------------------------------------*/

static BYTE retroreplay_clockport_read(WORD address)
{
    if (clockport_device) {
        if (rr_clockport_enabled) {
            if (address < 0x02) {
                retroreplay_clockport_io1_device.io_source_valid = 0;
                return 0;
            }
            return clockport_device->read(address, &retroreplay_clockport_io1_device.io_source_valid, clockport_device->device_context);
        }
    }
    return 0;
}

static BYTE retroreplay_clockport_peek(WORD address)
{
    if (clockport_device) {
        if (rr_clockport_enabled) {
            if (address < 0x02) {
                return 0;
            }
            return clockport_device->peek(address, clockport_device->device_context);
        }
    }
    return 0;
}

static void retroreplay_clockport_store(WORD address, BYTE byte)
{
    if (clockport_device) {
        if (rr_clockport_enabled) {
            if (address < 0x02) {
                return;
            }

            clockport_device->store(address, byte, clockport_device->device_context);
        }
    }
}

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
                if (rr_clockport_enabled && (addr & 0xff) < 0x10) {
                    return 0;
                }
                if ((reu_mapping) && (!rr_frozen)) {
                    if (export_ram || ((rr_revision == RR_REV_NORDIC_REPLAY) && export_ram_at_a000)) {
                        retroreplay_io1_device.io_source_valid = 1;
                        if (allow_bank) {
                            return export_ram0[0x1e00 + (addr & 0xff) + ((roml_bank & 3) << 13)];
                        } else {
                            return export_ram0[0x1e00 + (addr & 0xff)];
                        }
                    }
                    /* in ultimax mode only RAM will appear in IO1 */
                    /* if 16K-game mode is active and RAM is not selected , then nothing will
                       be mapped to IO1 */
                    if ((rr_cmode != CMODE_ULTIMAX) && (rr_cmode != CMODE_16KGAME)) {
                        retroreplay_io1_device.io_source_valid = 1;
                        return flash040core_read(flashrom_state, rom_offset + ((addr | 0xde00) & 0x1fff) + (roml_bank << 13));
                    }
                }
        }
    }
    return 0;
}

void retroreplay_io1_store(WORD addr, BYTE value)
{
    int mode = CMODE_WRITE;

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

                if (rr_hw_flashjumper) {
                    /* HACK: what really happens in flash mode is that the bits
                             in de00 _only_ affect the mapping in 8000-9fff range,
                             all other areas remain untouched as if no cartridge
                             is active. that means for flashing it will actually
                             use ultimax mode (both bits set in de00) to get the
                             write accesses for the ROML area.
                             another quirk is that in nordic power mode, writes
                             to a000-bfff area will also go to cartridge ram, reads
                             will return c64 ram though.
                     */
                    if ((value & 3) != 2) {
                        /* in flash mode always use 8k game, unless cart is disabled */
                        value &= ~3;
                    }
                }

                rr_bank = ((value >> 3) & 3) | ((value >> 5) & 4); /* bit 3-4, 7 */
                rr_cmode = (value & 3);  /* bit 0-1 */
                if ((rr_revision == RR_REV_NORDIC_REPLAY) && ((value & 0x67) == 0x22)) {
                    /* Nordic Replay supports additional Nordic Power compatible values */
                    rr_cmode = CMODE_16KGAME;
                    export_ram_at_a000 = 1; /* RAM at a000 enabled */
                } else {
                    /* Action Replay 5 compatible values */
                    export_ram_at_a000 = 0;
                    if (value & 0x40) { /* bit 6 */
                        mode |= CMODE_RELEASE_FREEZE;
                        rr_frozen = 0;
                    }
                    if (value & 0x20) { /* bit 5 */
                        mode |= CMODE_EXPORT_RAM;
                    }
                }
                /* after freezing writing to bit 0 and 1 has no effect until freeze
                   was acknowledged by setting bit 6 */
                if (rr_frozen) {
                    rr_cmode = CMODE_ULTIMAX;
                }
                cart_config_changed_slotmain(CMODE_8KGAME, (BYTE)(rr_cmode | (rr_bank << CMODE_BANK_SHIFT)), mode);

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
                        allow_bank = value & 2;
                        no_freeze = value & 4;
                        reu_mapping = value & 0x40;
                        write_once = 1;
                    }
                    /* bits 0 and 3,4,5,7 (bank) are not write once */
                    rr_bank = ((value >> 3) & 3) | ((value >> 5) & 4);
                    cart_romhbank_set_slotmain(rr_bank);
                    cart_romlbank_set_slotmain(rr_bank);
                    cart_port_config_changed_slotmain();
                    if (rr_clockport_enabled != (value & 1)) {
                        rr_clockport_enabled = value & 1;
                    }
                }
                break;
            default:
                if (rr_clockport_enabled && (addr & 0xff) < 0x10) {
                    return;
                }
                if ((reu_mapping) && (!rr_frozen)) {
                    if (export_ram || ((rr_revision == RR_REV_NORDIC_REPLAY) && export_ram_at_a000)) {
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
        if ((!reu_mapping) && (!rr_frozen)) {
            if (export_ram || ((rr_revision == RR_REV_NORDIC_REPLAY) && export_ram_at_a000)) {
                retroreplay_io2_device.io_source_valid = 1;
                if (allow_bank) {
                    return export_ram0[0x1f00 + (addr & 0xff) + ((roml_bank & 3) << 13)];
                } else {
                    return export_ram0[0x1f00 + (addr & 0xff)];
                }
            }
            /* in ultimax mode only RAM will appear in IO2 */
            /* if 16K-game mode is active and RAM is not selected , then nothing will
               be mapped to IO2 */
            if ((rr_cmode != CMODE_ULTIMAX) && (rr_cmode != CMODE_16KGAME)) {
                retroreplay_io2_device.io_source_valid = 1;
                return flash040core_read(flashrom_state, rom_offset + ((addr | 0xdf00) & 0x1fff) + (roml_bank << 13));
            }
        }
    }
    return 0;
}

void retroreplay_io2_store(WORD addr, BYTE value)
{
    DBG(("io2 w %04x %02x\n", addr, value));

    if (rr_active) {
        if ((!reu_mapping) && (!rr_frozen)) {
            if (export_ram || ((rr_revision == RR_REV_NORDIC_REPLAY) && export_ram_at_a000)) {
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
    /* in frozen state nothing is mapped to ROML */
    if (rr_frozen) {
        /* FIXME: what should be returned? C64 RAM or open bus? */
        return vicii_read_phi1();
    }
    if (export_ram) {
        return export_ram0[(addr & 0x1fff) + ((roml_bank & 3) << 13)];
    }
    /* on Nordic Replay when RAM is selected for ROMH in "nordic power mode"
       the ROM is mapped to ROML */
    if ((rr_revision == RR_REV_NORDIC_REPLAY) && export_ram_at_a000) {
        return flash040core_read(flashrom_state, rom_offset + (addr & 0x1fff) + (roml_bank << 13));
    }
    /* if 16K-game mode is active and RAM is not selected , then nothing will
       be mapped to ROML */
    if (rr_cmode == CMODE_16KGAME) {
        /* FIXME: what should be returned? C64 RAM or open bus? */
        return vicii_read_phi1();
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
    } else {
        /* Nordic Replay has RAM always writeable */
        if ((rr_revision == RR_REV_NORDIC_REPLAY) && export_ram) {
            export_ram0[(addr & 0x1fff) + ((roml_bank & 3) << 13)] = value;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------*/

BYTE retroreplay_a000_bfff_read(WORD addr)
{
    if ((rr_revision == RR_REV_NORDIC_REPLAY) && export_ram_at_a000) {
        if (rr_frozen) {
            if (allow_bank) {
                return export_ram0[(addr & 0x1fff) + ((roml_bank & 3) << 13)];
            } else {
                return export_ram0[addr & 0x1fff];
            }
        }
    }
    /* FIXME: is this correct? */
    return vicii_read_phi1();
}

void retroreplay_a000_bfff_store(WORD addr, BYTE value)
{
    if ((rr_revision == RR_REV_NORDIC_REPLAY) && export_ram_at_a000) {
        if (rr_frozen) {
            if (allow_bank) {
                export_ram0[(addr & 0x1fff) + ((roml_bank & 3) << 13)] = value;
            } else {
                export_ram0[addr & 0x1fff] = value;
            }
        }
    }
}

/* ---------------------------------------------------------------------*/

BYTE retroreplay_romh_read(WORD addr)
{
    if ((rr_revision == RR_REV_NORDIC_REPLAY) && export_ram_at_a000) {
        if (rr_frozen) {
            if (allow_bank) {
                return flash040core_read(flashrom_state, rom_offset + (addr & 0x1fff) + (roml_bank << 13));
            } else {
                /* if the "allow bank" bit is not set, and RAM is selected, then
                bot 0 and bit 1 of the bank nr are inactive for selecting the ROMH bank */
                return flash040core_read(flashrom_state, rom_offset + (addr & 0x1fff) + ((roml_bank & ~3) << 13));
            }
        }
        if (allow_bank) {
            return export_ram0[(addr & 0x1fff) + ((roml_bank & 3) << 13)];
        } else {
            return export_ram0[addr & 0x1fff];
        }
    }
    if ((allow_bank) || (!export_ram)) {
        return flash040core_read(flashrom_state, rom_offset + (addr & 0x1fff) + (roml_bank << 13));
    }
    /* if the "allow bank" bit is not set, and RAM is selected, then
       bot 0 and bit 1 of the bank nr are inactive for selecting the ROMH bank */
    return flash040core_read(flashrom_state, rom_offset + (addr & 0x1fff) + ((roml_bank & ~3) << 13));
}

void retroreplay_romh_store(WORD addr, BYTE value)
{
    if ((rr_revision == RR_REV_NORDIC_REPLAY) && export_ram_at_a000) {
        if (!rr_frozen) {
            if (allow_bank) {
                export_ram0[(addr & 0x1fff) + ((roml_bank & 3) << 13)] = value;
            } else {
                export_ram0[addr & 0x1fff] = value;
            }
        }
    }
}

int retroreplay_peek_mem(export_t *export, WORD addr, BYTE *value)
{
    if (addr >= 0x8000 && addr <= 0x9fff) {
        *value = retroreplay_roml_read(addr);
        return CART_READ_VALID;
    }
    if (!(export->exrom) && (export->game)) {
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
#if 0
    if (flashrom_state && flashrom_state->flash_data) {
        switch (addr & 0xe000) {
            case 0xe000:
                if ((rr_revision == RR_REV_NORDIC_REPLAY) && export_ram_at_a000) {
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
                if ((rr_revision == RR_REV_NORDIC_REPLAY) && export_ram_at_a000) {
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
    }
#endif
    *base = NULL;
    *start = 0;
    *limit = 0;
}

static int retroreplay_dump(void)
{
    /* FIXME: incomplete */
    mon_out("Retro Replay registers are %s.\n", rr_active ? "enabled" : "disabled");
    mon_out("Clockport is %s.\n", rr_clockport_enabled ? "enabled" : "disabled");
    if (rr_clockport_enabled) {
        mon_out("Clockport device: %s.\n", clockport_device_id_to_name(clockport_device_id));
    }
    mon_out("Freeze status: %s.\n", rr_frozen ? "frozen" : "released");

    mon_out("EXROM line: %s, GAME line: %s, Mode: %s\n",
            (rr_cmode & 2) ? "high" : "low",
            (rr_cmode & 1) ? "low" : "high",
            cart_config_string((BYTE)(rr_cmode & 3)));
    mon_out("ROM bank: %d\n", (rr_bank));
    /* FIXME: take system RAM and cart mode(s) into account here */
    /* FIXME: this is very inaccurate */
    mon_out("$8000-$9FFF: %s\n", (export_ram) ? "RAM" : "ROM");
    mon_out("$A000-$BFFF: %s\n", (export_ram_at_a000) ? "RAM" : "ROM");
    mon_out("$DF00-$DFFF: %s\n", (export_ram || export_ram_at_a000) ? "RAM" : "ROM");

    return 0;
}

/* ---------------------------------------------------------------------*/

void retroreplay_freeze(void)
{
    /* freeze button is disabled in flash mode */
    if (!rr_hw_flashjumper) {
        rr_active = 1;
        rr_frozen = 1;
        rr_cmode = CMODE_ULTIMAX;
        cart_config_changed_slotmain((BYTE)rr_cmode, (BYTE)rr_cmode, CMODE_READ);
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
    rr_frozen = 0;
    rr_clockport_enabled = 0;
    write_once = 0;
    no_freeze = 0;
    reu_mapping = 0;
    allow_bank = 0;
    export_ram_at_a000 = 0;

    if (rr_hw_flashjumper) {
        rr_cmode = CMODE_RAM;
    } else {
        rr_cmode = CMODE_8KGAME;
    }
    cart_config_changed_slotmain((BYTE)rr_cmode, (BYTE)rr_cmode, CMODE_READ);

    flash040core_reset(flashrom_state);
}

void retroreplay_reset(void)
{
    DBG(("retroreplay_reset flash:%d bank jumper: %d offset: %08x\n", rr_hw_flashjumper, rr_hw_bankjumper, rom_offset));
    rr_active = 1;
    rr_frozen = 0;

    if (rr_hw_flashjumper) {
        rr_cmode = CMODE_RAM;
    } else {
        rr_cmode = CMODE_8KGAME;
    }
    cart_config_changed_slotmain((BYTE)rr_cmode, (BYTE)rr_cmode, CMODE_READ);

    /* on the real hardware pressing reset would NOT reset the flash statemachine,
       only a powercycle would help. we do it here anyway :)
    */
    flash040core_reset(flashrom_state);
    if (rr_enabled && clockport_device) {
        clockport_device->reset(clockport_device->device_context);
    }
}

void retroreplay_config_setup(BYTE *rawcart)
{
    DBG(("retroreplay_config_setup bank jumper: %d offset: %08x\n", rr_hw_bankjumper, rom_offset));

    if (rr_hw_flashjumper) {
        rr_cmode = CMODE_RAM;
    } else {
        rr_cmode = CMODE_8KGAME;
    }
    cart_config_changed_slotmain((BYTE)rr_cmode, (BYTE)rr_cmode, CMODE_READ);

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

static int set_rr_clockport_device(int val, void *param)
{
    if (val == clockport_device_id) {
        return 0;
    }

    if (!rr_enabled) {
        clockport_device_id = val;
        return 0;
    }

    if (clockport_device_id != CLOCKPORT_DEVICE_NONE) {
        clockport_device->close(clockport_device);
        clockport_device_id = CLOCKPORT_DEVICE_NONE;
        clockport_device = NULL;
    }

    if (val != CLOCKPORT_DEVICE_NONE) {
        clockport_device = clockport_open_device(val, (char *)STRING_RETRO_REPLAY);
        if (!clockport_device) {
            return -1;
        }
        clockport_device_id = val;
    }
    return 0;
}

static int clockport_activate(void)
{
    if (rr_enabled) {
        return 0;
    }

    if (clockport_device_id == CLOCKPORT_DEVICE_NONE) {
        return 0;
    }

    clockport_device = clockport_open_device(clockport_device_id, (char *)STRING_RETRO_REPLAY);
    if (!clockport_device) {
        return -1;
    }
    return 0;
}

static int clockport_deactivate(void)
{
    if (!rr_enabled) {
        return 0;
    }

    if (clockport_device_id == CLOCKPORT_DEVICE_NONE) {
        return 0;
    }

    clockport_device->close(clockport_device);
    clockport_device = NULL;

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
    { "RRClockPort", 0, RES_EVENT_NO, NULL,
      &clockport_device_id, set_rr_clockport_device, NULL },
    RESOURCE_INT_LIST_END
};

int retroreplay_resources_init(void)
{
    return resources_register_int(resources_int);
}

void retroreplay_resources_shutdown(void)
{
    lib_free(clockport_device_names);
    clockport_device_names = NULL;
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
    CMDLINE_LIST_END
};

static cmdline_option_t clockport_cmdline_options[] =
{
    { "-rrclockportdevice", SET_RESOURCE, 1,
      NULL, NULL, "RRClockPort", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_COMBO,
      IDCLS_P_DEVICE, IDCLS_CLOCKPORT_DEVICE,
      NULL, NULL },
    CMDLINE_LIST_END
};

int retroreplay_cmdline_options_init(void)
{
    int i;
    char *tmp;
    char number[10];

    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    sprintf(number, "%d", clockport_supported_devices[0].id);

    clockport_device_names = util_concat(". (", number, ": ", clockport_supported_devices[0].name, NULL);

    for (i = 1; clockport_supported_devices[i].name; ++i) {
        tmp = clockport_device_names;
        sprintf(number, "%d", clockport_supported_devices[i].id);
        clockport_device_names = util_concat(tmp, ", ", number, ": ", clockport_supported_devices[i].name, NULL);
        lib_free(tmp);
    }
    tmp = clockport_device_names;
    clockport_device_names = util_concat(tmp, ")", NULL);
    lib_free(tmp);
    clockport_cmdline_options[0].description = clockport_device_names;

    return cmdline_register_options(clockport_cmdline_options);
}

static int retroreplay_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    retroreplay_io1_list_item = io_source_register(&retroreplay_io1_device);
    retroreplay_io2_list_item = io_source_register(&retroreplay_io2_device);
    retroreplay_clockport_io1_list_item = io_source_register(&retroreplay_clockport_io1_device);

    if (clockport_activate() < 0) {
        return -1;
    }

    rr_enabled = 1;

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
    export_remove(&export_res);
    clockport_deactivate();
    io_source_unregister(retroreplay_io1_list_item);
    io_source_unregister(retroreplay_io2_list_item);
    io_source_unregister(retroreplay_clockport_io1_list_item);
    retroreplay_io1_list_item = NULL;
    retroreplay_io2_list_item = NULL;
    retroreplay_clockport_io1_list_item = NULL;
    rr_enabled = 0;
}

/* ---------------------------------------------------------------------*/

/* CARTRR snapshot module format:

   type  | name              | version | description
   -------------------------------------------------
   BYTE  | revision          |   0.1+  | RR revision
   BYTE  | active            |   0.0+  | cartridge active flag
   BYTE  | frozen            |   0.2+  | frozen flag
   BYTE  | cmode             |   0.3   | cmode
   BYTE  | clockport enabled |   0.0+  | clockport enabled flag
   BYTE  | bank              |   0.0+  | current bank
   BYTE  | write once        |   0.0+  | write once flag
   BYTE  | allow bank        |   0.0+  | allow bank flag
   BYTE  | no freeze         |   0.0+  | no freeze flag
   BYTE  | REU mapping       |   0.0+  | REU mapping flag
   BYTE  | RAM at a000       |   0.1+  | RAM at $A000 flag
   BYTE  | flash jumper      |   0.0+  | flash jumper state
   BYTE  | bank jumper       |   0.0+  | bank jumper state
   BYTE  | ROM offset        |   0.0+  | ROM offset
   ARRAY | ROML              |   0.0+  | 131072 BYTES of ROML data
   ARRAY | RAM               |   0.0+  | 32768 BYTES of RAM data
 */

static char snap_module_name[] = "CARTRR";
static char flash_snap_module_name[] = "FLASH040RR";
#define SNAP_MAJOR   0
#define SNAP_MINOR   3

int retroreplay_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (BYTE)rr_revision) < 0
        || SMW_B(m, (BYTE)rr_active) < 0
        || SMW_B(m, (BYTE)rr_frozen) < 0
        || SMW_B(m, (BYTE)rr_cmode) < 0
        || SMW_B(m, (BYTE)rr_clockport_enabled) < 0
        || SMW_B(m, (BYTE)rr_bank) < 0
        || SMW_B(m, (BYTE)write_once) < 0
        || SMW_B(m, (BYTE)allow_bank) < 0
        || SMW_B(m, (BYTE)no_freeze) < 0
        || SMW_B(m, (BYTE)reu_mapping) < 0
        || SMW_B(m, (BYTE)export_ram_at_a000) < 0
        || SMW_B(m, (BYTE)rr_hw_flashjumper) < 0
        || SMW_B(m, (BYTE)rr_hw_bankjumper) < 0
        || SMW_DW(m, (DWORD)rom_offset) < 0
        || SMW_BA(m, roml_banks, 0x20000) < 0
        || SMW_BA(m, export_ram0, 0x8000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return flash040core_snapshot_write_module(s, flashrom_state, flash_snap_module_name);
}

int retroreplay_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;
    DWORD temp_rom_offset;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (vmajor > SNAP_MAJOR || vminor > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    /* new in 0.1 */
    if (SNAPVAL(vmajor, vminor, 0, 1)) {
        if (SMR_B_INT(m, &rr_revision) < 0) {
            goto fail;
        }
    } else {
        rr_revision = 0;
    }

    if (SMR_B_INT(m, &rr_active) < 0) {
        goto fail;
    }

    /* new in 0.2 */
    if (SNAPVAL(vmajor, vminor, 0, 2)) {
        if (SMR_B_INT(m, &rr_frozen) < 0) {
            goto fail;
        }
    } else {
        rr_frozen = 0;
    }

    /* new in 0.3 */
    if (SNAPVAL(vmajor, vminor, 0, 3)) {
        if (SMR_B_INT(m, &rr_cmode) < 0) {
            goto fail;
        }
    } else {
        rr_cmode = CMODE_8KGAME;
    }

    if (0
        || SMR_B_INT(m, &rr_clockport_enabled) < 0
        || SMR_B_INT(m, &rr_bank) < 0
        || SMR_B_INT(m, &write_once) < 0
        || SMR_B_INT(m, &allow_bank) < 0
        || SMR_B_INT(m, &no_freeze) < 0
        || SMR_B_INT(m, &reu_mapping) < 0) {
        goto fail;
    }

    /* new in 0.1 */
    if (SNAPVAL(vmajor, vminor, 0, 1)) {
        if (SMR_B_INT(m, &export_ram_at_a000) < 0) {
            goto fail;
        }
    } else {
        export_ram_at_a000 = 0;
    }

    if (0
        || SMR_B_INT(m, &rr_hw_flashjumper) < 0
        || SMR_B_INT(m, &rr_hw_bankjumper) < 0
        || SMR_DW(m, &temp_rom_offset) < 0
        || SMR_BA(m, roml_banks, 0x20000) < 0
        || SMR_BA(m, export_ram0, 0x8000) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    rom_offset = temp_rom_offset;

    flashrom_state = lib_malloc(sizeof(flash040_context_t));

    flash040core_init(flashrom_state, maincpu_alarm_context, FLASH040_TYPE_010, roml_banks);

    if (flash040core_snapshot_read_module(s, flashrom_state, flash_snap_module_name) < 0) {
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

fail:
    snapshot_module_close(m);
    return -1;
}
