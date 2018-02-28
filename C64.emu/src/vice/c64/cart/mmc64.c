/*
 * mmc64.c - Cartridge handling, MMC64 cart.
 *
 * Written by
 *  Markus Stehr <bastetfurry@ircnet.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *  Groepaz <groepaz@gmx.net>
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

#include "archdep.h"
#include "c64cart.h" /* for export_t */
#define CARTRIDGE_INCLUDE_SLOT0_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOT0_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "clockport.h"
#include "cmdline.h"
#include "crt.h"
#include "export.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "mem.h"
#include "monitor.h"
#include "resources.h"
#include "snapshot.h"
#include "spi-sdcard.h"
#include "translate.h"
#include "types.h"
#include "util.h"

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "mmc64.h"
#include "reu.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API

/* FIXME: test and then remove all old code */
#define USEPASSTHROUGHHACK 1 /* define 1 to use the passthrough hack */

/* #define MMC64DEBUG */

/*
#define LOG_READ_DF10
#define LOG_READ_DF11
#define LOG_READ_DF12
#define LOG_READ_DF13

#define LOG_WRITE_DF10
#define LOG_WRITE_DF11
#define LOG_WRITE_DF12
#define LOG_WRITE_DF13
*/

#ifdef MMC64DEBUG
#define LOG(_x_) log_debug _x_
#else
#define LOG(_x_)
#endif

/* MMC64 enable */
static int mmc64_enabled;

/* MMC64 clockport enable */
int mmc64_clockport_enabled = 1;

/* MMC64 clockport base address */
int mmc64_hw_clockport = 0xde02;

/* Current clockport device */
static int clockport_device_id = CLOCKPORT_DEVICE_NONE;
static clockport_device_t *clockport_device = NULL;

/* MMC64 bios writable */
static int mmc64_bios_write;

/* Bios file name */
static char *mmc64_bios_filename = NULL;

/* Image file name */
static char *mmc64_image_filename = NULL;

/* $DF11 bit 7 unlock flag */
static int mmc64_bit7_unlocked = 0;

/* Unlocking sequences buffer */
static BYTE mmc64_unlocking[2] = { 0, 0 };

/* BIOS changed flag */
static int mmc64_bios_changed = 0;

/* flash jumper flag */
static int mmc64_hw_flashjumper; /* status of the flash jumper */

/* write protect flag */
static int mmc64_hw_writeprotect;

/* Flags for the various control bits */
static BYTE mmc64_active;
static BYTE mmc64_spi_mode;
static BYTE mmc64_extrom;
static BYTE mmc64_flashmode;
static BYTE mmc64_cport;
static BYTE mmc64_speed;
static BYTE mmc64_cardsel;
static BYTE mmc64_biossel;

/* Status Bits */
/* $DF12 (R): MMC64 status register */
#define MMC_FLASHJMP  0x20 /* bit 5: 0 = flash jumper not set, 1 = flash jumper set  */
#define MMC_WRITEPROT 0x10 /* bit 4: 0 = card write enabled, 1 = card write disabled */
#define MMC_CARDPRS   0x08 /* bit 3: 0 = card inserted, 1 = no card inserted         */
#define MMC_EXTEXROM  0x04 /* bit 2: external EXROM line                             */
#define MMC_EXTGAME   0x02 /* bit 1: external GAME line                              */
#define MMC_SPISTAT   0x01 /* bit 0: 0 = SPI ready, 1 = SPI busy                     */

/* Variables of the various status bits */
static BYTE mmc64_extexrom = 0;
static BYTE mmc64_extgame = 0;

static int mmc64_revision;
static int mmc64_sd_type = 0;
static BYTE mmc64_image_file_readonly = 0;

static log_t mmc64_log = LOG_ERR;

static BYTE mmc64_bios[0x2002];
static int mmc64_bios_offset = 0;
static int mmc64_bios_type = 0;

static const char STRING_MMC64[] = CARTRIDGE_NAME_MMC64;

static int mmc64_activate(void);
static int mmc64_deactivate(void);

static char *clockport_device_names = NULL;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static void mmc64_clockport_enable_store(WORD addr, BYTE value);
static BYTE mmc64_clockport_enable_peek(WORD addr);
static void mmc64_io1_store(WORD addr, BYTE value);
static BYTE mmc64_io1_read(WORD addr);
static BYTE mmc64_io1_peek(WORD addr);
static void mmc64_io2_store(WORD addr, BYTE value);
static BYTE mmc64_io2_read(WORD addr);
static BYTE mmc64_io2_peek(WORD addr);
static int mmc64_dump(void);

static BYTE mmc64_clockport_read(WORD io_address);
static BYTE mmc64_clockport_peek(WORD io_address);
static void mmc64_clockport_store(WORD io_address, BYTE byte);

static io_source_t mmc64_io1_clockport_enable_device = {
    CARTRIDGE_NAME_MMC64 " Clockport enable",
    IO_DETACH_RESOURCE,
    "MMC64",
    0xde01, 0xde01, 0x01,
    0,
    mmc64_clockport_enable_store,
    NULL, /* read */
    mmc64_clockport_enable_peek,
    mmc64_dump,
    CARTRIDGE_MMC64,
    0,
    0
};

/* FIXME: register map doesnt show a register at $df21 - is this correct? */
static io_source_t mmc64_io2_clockport_enable_device = {
    CARTRIDGE_NAME_MMC64 " Clockport enable",
    IO_DETACH_RESOURCE,
    "MMC64",
    0xdf21, 0xdf21, 0x01,
    0,
    mmc64_clockport_enable_store,
    NULL, /* read */
    mmc64_clockport_enable_peek,
    mmc64_dump,
    CARTRIDGE_MMC64,
    0,
    0
};

static io_source_t mmc64_io1_clockport_device = {
    CARTRIDGE_NAME_MMC64 " Clockport",
    IO_DETACH_RESOURCE,
    "MMC64ClockPort",
    0xde02, 0xde0f, 0x0f,
    0,
    mmc64_clockport_store,
    mmc64_clockport_read,
    mmc64_clockport_peek,
    mmc64_dump,
    CARTRIDGE_MMC64,
    0,
    0
};

static io_source_t mmc64_io2_clockport_device = {
    CARTRIDGE_NAME_MMC64 " Clockport",
    IO_DETACH_RESOURCE,
    "MMC64ClockPort",
    0xdf22, 0xdf2f, 0x0f,
    0,
    mmc64_clockport_store,
    mmc64_clockport_read,
    mmc64_clockport_peek,
    mmc64_dump,
    CARTRIDGE_MMC64,
    0,
    0
};

static io_source_t *mmc64_current_clockport_enable_device = &mmc64_io1_clockport_enable_device;
static io_source_t *mmc64_current_clockport_device = &mmc64_io1_clockport_device;

/* FIXME: register/handle the clockport resource properly */

/* FIXME: unused? */
#if 0
static const export_resource_t export_cp_res = {
    CARTRIDGE_NAME_MMC64 " Clockport", 0, 0, &mmc64_io1_clockport_device, &mmc64_io2_clockport_device, CARTRIDGE_MMC64
};
#endif

static io_source_t mmc64_io2_device = {
    CARTRIDGE_NAME_MMC64,
    IO_DETACH_RESOURCE,
    "MMC64",
    0xdf10, 0xdf13, 0x03,
    0,
    mmc64_io2_store,
    mmc64_io2_read,
    mmc64_io2_peek,
    mmc64_dump,
    CARTRIDGE_MMC64,
    1, /* mask df10-df13 from passthrough */
    0
};

static io_source_t mmc64_io1_device = {
    CARTRIDGE_NAME_MMC64,
    IO_DETACH_RESOURCE,
    "MMC64",
    0xde10, 0xde13, 0x03,
    0,
    mmc64_io1_store,
    mmc64_io1_read,
    mmc64_io1_peek,
    mmc64_dump,
    CARTRIDGE_MMC64,
    0,
    0
};

static io_source_list_t *mmc64_clockport_list_item = NULL;
static io_source_list_t *mmc64_clockport_enable_list_item = NULL;
static io_source_list_t *mmc64_io1_list_item = NULL;
static io_source_list_t *mmc64_io2_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_MMC64, 1, 1, &mmc64_io1_device, &mmc64_io2_device, CARTRIDGE_MMC64
};

/* ---------------------------------------------------------------------*/

int mmc64_cart_enabled(void)
{
    return mmc64_enabled;
}

int mmc64_cart_active(void)
{
    if (mmc64_enabled && !mmc64_active && !mmc64_biossel) {
        return 1;
    }
    return 0;
}

/* Resets the card */
void mmc64_reset(void)
{
    mmc64_active = 0;
    mmc64_spi_mode = 0;
    mmc64_extrom = 0;
    mmc64_flashmode = 0;
    mmc64_cport = 0;
    mmc64_speed = 0;
    mmc64_cardsel = 0;
    mmc64_biossel = mmc64_hw_flashjumper; /* disable bios at reset when flash jumper is set */

#if 0
    mmc64_extexrom = 0x04;
    mmc64_extgame = 0x02;
#endif

    if (mmc64_clockport_enabled != 1) {
        mmc64_clockport_enabled = 1;
        if (mmc64_enabled && clockport_device) {
            clockport_device->reset(clockport_device->device_context);
        }
    }
    if (mmc64_enabled) {
#if USEPASSTHROUGHHACK
        mmc64_io2_device.io_source_prio = 1;
        cart_set_port_exrom_slot0(1);
        cart_port_config_changed_slot0();
#else
        cart_config_changed_slot0(0, 0, CMODE_READ);
#endif
    }
}

static int mmc64_activate(void)
{
    mmc64_bios_changed = 0;
    mmc_open_card_image(mmc64_image_filename, mmc64_hw_writeprotect ^ 1);
    /* mmc64_reset(); */
    return 0;
}

static int mmc64_deactivate(void)
{
    int ret;

    mmc_close_card_image();

    if (mmc64_bios_changed && mmc64_bios_write) {
        if (mmc64_bios_type == CARTRIDGE_FILETYPE_CRT) {
            ret = mmc64_crt_save(mmc64_bios_filename);
        } else {
            ret = mmc64_bin_save(mmc64_bios_filename);
        }
        if (ret <= 0) {
            return 0; /* FIXME */
        }
    }
    return 0;
}

static int set_mmc64_clockport_device(int val, void *param)
{
    if (val == clockport_device_id) {
        return 0;
    }

    if (!mmc64_enabled) {
        clockport_device_id = val;
        return 0;
    }

    if (clockport_device_id != CLOCKPORT_DEVICE_NONE) {
        clockport_device->close(clockport_device);
        clockport_device_id = CLOCKPORT_DEVICE_NONE;
        clockport_device = NULL;
    }

    if (val != CLOCKPORT_DEVICE_NONE) {
        clockport_device = clockport_open_device(val, (char *)STRING_MMC64);
        if (!clockport_device) {
            return -1;
        }
        clockport_device_id = val;
    }
    return 0;
}

static int clockport_activate(void)
{
    if (mmc64_enabled) {
        return 0;
    }

    if (clockport_device_id == CLOCKPORT_DEVICE_NONE) {
        return 0;
    }

    clockport_device = clockport_open_device(clockport_device_id, (char *)STRING_MMC64);
    if (!clockport_device) {
        return -1;
    }
    return 0;
}

static int clockport_deactivate(void)
{
    if (!mmc64_enabled) {
        return 0;
    }

    if (clockport_device_id == CLOCKPORT_DEVICE_NONE) {
        return 0;
    }

    clockport_device->close(clockport_device);
    clockport_device = NULL;

    return 0;
}

/* FIXME: resetting the c64 should be handled in the upper layer */
static int set_mmc64_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    LOG(("MMC64: set_enabled: '%s' %d to %d", mmc64_bios_filename, mmc64_enabled, val));
    if (!mmc64_enabled && val) {
        /* activate mmc64 */
        if (param) {
            /* if the param is != NULL, then we should load the default image file */
            LOG(("MMC64: set_enabled(1) '%s'", mmc64_bios_filename));
            if (mmc64_bios_filename) {
                if (*mmc64_bios_filename) {
                    if (cartridge_attach_image(CARTRIDGE_MMC64, mmc64_bios_filename) < 0) {
                        LOG(("MMC64: set_enabled(1) did not register"));
                        return -1;
                    }
                    /* mmc64_enabled = 1; */ /* cartridge_attach_image will end up calling set_mmc64_enabled again */
                    return 0;
                }
            }
        } else {
            LOG(("MMC64: set_enabled(0) '%s'", mmc64_bios_filename));
            cart_power_off();
            /* if the param is == NULL, then we should actually set the resource */
            if (export_add(&export_res) < 0) {
                LOG(("MMC64: set_enabled(0) did not register"));
                return -1;
            } else {
                LOG(("MMC64: set_enabled registered"));

                if (mmc64_activate() < 0) {
                    return -1;
                }
                if (clockport_activate() < 0) {
                    return -1;
                }
                mmc64_enabled = 1;
                cart_set_port_exrom_slot0(1);
                cart_port_config_changed_slot0();
                mmc64_clockport_enable_list_item = io_source_register(mmc64_current_clockport_enable_device);
                mmc64_clockport_list_item = io_source_register(mmc64_current_clockport_device);
                mmc64_io1_list_item = io_source_register(&mmc64_io1_device);
                mmc64_io2_list_item = io_source_register(&mmc64_io2_device);
                mmc64_reset();
            }
        }
    } else if (mmc64_enabled && !val) {
        /* remove mmc64 */
        if (mmc64_deactivate() < 0) {
            return -1;
        }
        clockport_deactivate();
        cart_power_off();
        export_remove(&export_res);
        mmc64_enabled = 0;
        cart_set_port_exrom_slot0(0);
        cart_port_config_changed_slot0();
        io_source_unregister(mmc64_clockport_enable_list_item);
        io_source_unregister(mmc64_clockport_list_item);
        io_source_unregister(mmc64_io1_list_item);
        io_source_unregister(mmc64_io2_list_item);
        mmc64_clockport_enable_list_item = NULL;
        mmc64_clockport_list_item = NULL;
        mmc64_io1_list_item = NULL;
        mmc64_io2_list_item = NULL;
    }
    LOG(("MMC64: set_enabled done: '%s' %d : %d", mmc64_bios_filename, val, mmc64_enabled));
    return 0;
}

static int set_mmc64_readonly(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!mmc64_image_file_readonly) {
        mmc64_hw_writeprotect = val;
        if (!((*mmc64_image_filename) == 0)) {
            return mmc_open_card_image(mmc64_image_filename, mmc64_hw_writeprotect ^ 1);
        }
        return 0;
    } else {
        mmc64_hw_writeprotect = 1;
    }

    if (!((*mmc64_image_filename) == 0)) {
        return mmc_open_card_image(mmc64_image_filename, mmc64_hw_writeprotect ^ 1);
    }

    return -1;
}

static int set_mmc64_flashjumper(int val, void *param)
{
    mmc64_hw_flashjumper = val ? 1 : 0;
    LOG(("MMC64 Flashjumper: %d", mmc64_hw_flashjumper));
    return 0;
}

static int set_mmc64_revision(int val, void *param)
{
    switch (val) {
        case MMC64_REV_A:
        case MMC64_REV_B:
            break;
        default:
            return -1;
    }

    mmc64_revision = val;

    return 0;
}

static int set_mmc64_sd_type(int val, void *param)
{
    switch (val) {
        case MMC64_TYPE_AUTO:
        case MMC64_TYPE_MMC:
        case MMC64_TYPE_SD:
        case MMC64_TYPE_SDHC:
            break;
        default:
            return -1;
    }

    mmc64_sd_type = val;
    mmc_set_card_type((BYTE)val);
    return 0;
}

static int set_mmc64_bios_write(int val, void *param)
{
    mmc64_bios_write = val ? 1 : 0;
    return 0;
}

static int set_mmc64_bios_filename(const char *name, void *param)
{
    int enabled;

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }
    LOG(("MMC64: set_name: %d '%s'", mmc64_enabled, mmc64_bios_filename));

    util_string_set(&mmc64_bios_filename, name);
    resources_get_int("MMC64", &enabled);

    if (set_mmc64_enabled(enabled, (void*)1) < 0) {
        lib_free(mmc64_bios_filename);
        mmc64_bios_filename = NULL;
        LOG(("MMC64: set_name done: %d '%s'", mmc64_enabled, mmc64_bios_filename));
        return -1;
    }
    LOG(("MMC64: set_name done: %d '%s'", mmc64_enabled, mmc64_bios_filename));

    return 0;
}

static int set_mmc64_image_filename(const char *name, void *param)
{
    if (mmc64_image_filename != NULL && name != NULL && strcmp(name, mmc64_image_filename) == 0) {
        return 0;
    }

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    if (mmc64_enabled) {
        mmc64_deactivate();
        util_string_set(&mmc64_image_filename, name);
        mmc64_activate();
    } else {
        util_string_set(&mmc64_image_filename, name);
    }

    return 0;
}

/* ---------------------------------------------------------------------*/

int mmc64_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit)
{
    if (!mmc64_active && !mmc64_biossel) {
        switch (addr & 0xf000) {
            case 0x9000:
            case 0x8000:
                *base = &mmc64_bios[mmc64_bios_offset] - 0x8000;
                *start = 0x8000;
                *limit = 0x9ffd;
                return CART_READ_VALID;
            default:
                break;
        }
    }
    return CART_READ_THROUGH;
}

void mmc64_config_init(export_t *export)
{
    LOG(("MMC64 mmc64_config_init"));

    mmc64_active = 0;
    mmc64_spi_mode = 0;
    mmc64_extrom = 0;
    mmc64_flashmode = 0;
    mmc64_cport = 0;
    mmc64_speed = 0;
    mmc64_cardsel = 0;
    mmc64_biossel = mmc64_hw_flashjumper; /* disable bios at reset when flash jumper is set */
#if 0
    /* for now external exrom and game are constantly   *
    * high until the pass-through port support is made */
    mmc64_extexrom = 0;
    mmc64_extgame = 0;
#endif
    mmc64_extexrom = export->exrom;
    mmc64_extgame = export->game;

    if (mmc64_enabled) {
#if USEPASSTHROUGHHACK
        mmc64_io2_device.io_source_prio = 1;
        cart_config_changed_slot0((BYTE)((0 << 1) | mmc64_extgame), (BYTE)((0 << 1) | mmc64_extgame), CMODE_READ);
#else
        cart_config_changed_slot0(0, 0, CMODE_READ);
#endif
    } else {
        mmc64_io2_device.io_source_prio = 0;
    }
}

void mmc64_passthrough_changed(export_t *export)
{
    mmc64_extexrom = export->exrom;
    mmc64_extgame = export->game;
    LOG(("MMC64 passthrough changed exrom: %d game: %d (mmc64_active: %d)", mmc64_extexrom, mmc64_extgame, mmc64_active));
    if (!mmc64_active) {
        cart_set_port_game_slot0(mmc64_extgame);
        cart_port_config_changed_slot0();
    } else {
        /* MMC64 is completely disabled */
        cart_config_changed_slot0((BYTE)(((mmc64_extexrom ^ 1) << 1) | mmc64_extgame), (BYTE)(((mmc64_extexrom ^ 1) << 1) | mmc64_extgame), CMODE_READ);
    }
}

static void mmc64_clockport_enable_store(WORD addr, BYTE value)
{
    if ((value & 1) != mmc64_clockport_enabled) {
        mmc64_clockport_enabled = value & 1;
    }
}

static BYTE mmc64_clockport_enable_peek(WORD addr)
{
    return mmc64_clockport_enabled;
}

static void mmc64_reg_store(WORD addr, BYTE value, int active)
{
    switch (addr) {
        case 0:
            /*
             * $DF10: MMC SPI transfer register
             *
             * byte written is sent to the card
             */
            if (active) {
#ifdef LOG_WRITE_DF10
                LOG(("MMC64: IO2 ST %04x %02x", addr, value));
#endif
                spi_mmc_data_write(value);
                return;
            }
#ifdef LOG_WRITE_DF10
            else {
                LOG(("MMC64: unhandled IO2 ST %04x %02x", addr, value));
            }
#endif
            break;

        case 1:
            /*
             * $DF11: MMC control register
             *        ------------------------
             *        bit 0:  0 = MMC BIOS enabled, 1 = MMC BIOS disabled                   (R/W)
             *        bit 1:  0 = card selected, 1 = card not selected                      (R/W)
             *        bit 2:  0 = 250khz transfer, 1 = 8mhz transfer                        (R/W)
             *        bit 3:  0 = clock port @ $DE00, 1 = clock port @ $DF20                (R/W)
             *        bit 4:  0 = normal Operation, 1 = flash mode                          (R/W)  (*)
             *        bit 5:  0 = allow external rom when BIOS is disabled , 1 = disable    (R/W)
             *        bit 6:  0 = SPI write trigger mode, 1 = SPI read trigger mode         (R/W)
             *        bit 7:  0 = MMC64 is active, 1 = MMC64 is completely disabled         (R/W)  (**)
             *
             * (*) bit can only be programmed when flash jumper is set
             * (**) bit can only be modified after unlocking
             */
            if (active) {
                mmc64_biossel = (value) & 1; /* bit 0 */
                mmc64_extrom = (value >> 5) & 1;      /* bit 5 */

#ifdef LOG_WRITE_DF11
                LOG(("MMC64: IO2 ST %04x %02x mmc64_biossel %x mmc64_extrom %x", addr, value, mmc64_biossel, mmc64_extrom));
                LOG(("MMC64:                  mmc64_flashmode %d", (((value >> 4)) & 1)));
                LOG(("MMC64:                  mmc64_active %d", (((value >> 7)) & 1)));
#endif

                spi_mmc_card_selected_write((BYTE)(((value >> 1) ^ 1) & 1));   /* bit 1 */
                spi_mmc_enable_8mhz_write((BYTE)(((value >> 2)) & 1)); /* bit 2 */
                mmc64_cport = (((value >> 3)) & 1); /* bit 3 */

                if (mmc64_hw_flashjumper) {    /* this bit can only be changed if the flashjumper is on */
                    mmc64_flashmode = (((value >> 4)) & 1); /* bit 4 */
                }
                spi_mmc_trigger_mode_write((BYTE)(((value >> 6)) & 1));        /* bit 6 */

                mmc64_active = (((value >> 7)) & 1); /* bit 7 */

#if USEPASSTHROUGHHACK
                if (mmc64_active) {
                    /* cart_set_port_exrom_slot0(0); */
                    log_message(mmc64_log, "disabling MMC64 (exrom:%d game:%d) mmc64_active: %d", mmc64_extexrom, mmc64_extgame, mmc64_active);
                    cart_config_changed_slot0((BYTE)(((mmc64_extexrom ^ 1) << 1) | mmc64_extgame), 
                                              (BYTE)(((mmc64_extexrom ^ 1) << 1) | mmc64_extgame), CMODE_READ);
                    mmc64_io2_device.io_source_prio = 0;
                } else {
                    /* this controls the mapping of the MMC64 bios */
                    if (mmc64_biossel) {
                        /* cart_set_port_exrom_slot0(0);
                        cart_set_port_game_slot0(0); */
                        cart_config_changed_slot0(CMODE_RAM, CMODE_RAM, CMODE_READ);
                    } else {
                        /* MMC64 ROM enabled */
                        if (mmc64_flashmode) {
                            /* cart_set_port_exrom_slot0(0);
                            cart_set_port_game_slot0(1); */
                            cart_config_changed_slot0(CMODE_RAM, CMODE_ULTIMAX, CMODE_READ);
                        } else {
                            /* cart_set_port_exrom_slot0(1);
                            cart_set_port_game_slot0(0); */
                            cart_config_changed_slot0(CMODE_RAM, CMODE_8KGAME, CMODE_READ);
                        }
                    }
                    /* cart_port_config_changed_slot0(); */
                    mmc64_io2_device.io_source_prio = 1;
                }
#else
                if (mmc64_active) {
                    log_message(mmc64_log, "disabling MMC64");
                    cart_config_changed_slot0(CMODE_RAM, CMODE_RAM, CMODE_READ);
                } else {
                    if (mmc64_biossel) {
                        cart_config_changed_slot0(CMODE_RAM, CMODE_RAM, CMODE_READ);
                    } else {
                        /* MMC64 ROM enabled */
                        cart_config_changed_slot0(CMODE_RAM, CMODE_8KGAME, CMODE_READ);
                    }
                }
#endif
                if (mmc64_cport) {
                    mmc64_hw_clockport = 0xdf22;
                    mmc64_current_clockport_device = &mmc64_io2_clockport_device;
                    mmc64_current_clockport_enable_device = &mmc64_io2_clockport_enable_device;
                    io_source_unregister(mmc64_clockport_list_item);
                    mmc64_clockport_list_item = io_source_register(mmc64_current_clockport_device);
                    io_source_unregister(mmc64_clockport_enable_list_item);
                    mmc64_clockport_enable_list_item = io_source_register(mmc64_current_clockport_enable_device);
                } else {
                    mmc64_hw_clockport = 0xde02;
                    mmc64_current_clockport_device = &mmc64_io1_clockport_device;
                    io_source_unregister(mmc64_clockport_list_item);
                    mmc64_clockport_list_item = io_source_register(mmc64_current_clockport_device);
                    mmc64_current_clockport_enable_device = &mmc64_io1_clockport_enable_device;
                    io_source_unregister(mmc64_clockport_enable_list_item);
                    mmc64_clockport_enable_list_item = io_source_register(mmc64_current_clockport_enable_device);
                }
                return;
            }
#ifdef LOG_WRITE_DF11
            else {
                LOG(("MMC64: unhandled IO2 ST %04x %02x", addr, value));
            }
#endif
            break;

        case 2:  /* MMC64 status register, read only */
            break;

        case 3:  /* MMC64 identification register, also used for unlocking sequences */
            mmc64_unlocking[0] = mmc64_unlocking[1];
            mmc64_unlocking[1] = value;
            if ((mmc64_unlocking[0] == 0x55) && (mmc64_unlocking[1] == 0xaa)) {
                LOG(("MMC64: bit 7 unlocked"));
                mmc64_bit7_unlocked = 1;    /* unlock bit 7 of $DF11 */
            } else if ((mmc64_unlocking[0] == 0x0a) && (mmc64_unlocking[1] == 0x1c)) {
                LOG(("MMC64: mmc64 reenabled"));
                mmc64_active = 0;
#if USEPASSTHROUGHHACK
                cart_set_port_exrom_slot0(1);
                cart_port_config_changed_slot0();   /* re-enable the MMC64 */
#else
                cart_config_changed_slot0(2, 0, CMODE_READ);
#endif
            }
#ifdef LOG_WRITE_DF11
            else {
                LOG(("MMC64: unhandled IO2 ST %04x %02x", addr, value));
            }
#endif
            break;

        default:      /* Not for us */
            return;
    }
}

static void mmc64_io1_store(WORD addr, BYTE value)
{
    if (mmc64_hw_flashjumper) {
        mmc64_reg_store(addr, value, 1);
    }
}

static void mmc64_io2_store(WORD addr, BYTE value)
{
    mmc64_reg_store(addr, value, mmc64_active ^ 1);
}

static BYTE mmc64_io2_read(WORD addr)
{
    BYTE value;

    if (mmc64_active) {
        /* MMC64 is completely disabled */
        mmc64_io2_device.io_source_valid = 0;
        return 0;
    }

    mmc64_io2_device.io_source_valid = 1;

    switch (addr) {
        case 0:
            /*
             * $DF10: MMC SPI transfer register
             *
             * response from the card is read here
             */
            value = spi_mmc_data_read();
#ifdef LOG_READ_DF10
            LOG(("MMC64: IO2 RD %04x %02x", addr, value));
#endif
            return value;

        case 1:
            /*
             * $DF11: MMC control register
             *        ------------------------
             *        bit 0:  0 = MMC BIOS enabled, 1 = MMC BIOS disabled                   (R/W)
             *        bit 1:  0 = card selected, 1 = card not selected                      (R/W)
             *        bit 2:  0 = 250khz transfer, 1 = 8mhz transfer                        (R/W)
             *        bit 3:  0 = clock port @ $DE00, 1 = clock port @ $DF20                (R/W)
             *        bit 4:  0 = normal Operation, 1 = flash mode                          (R/W)  (*)
             *        bit 5:  0 = allow external rom when BIOS is disabled , 1 = disable    (R/W)
             *        bit 6:  0 = SPI write trigger mode, 1 = SPI read trigger mode         (R/W)
             *        bit 7:  0 = MMC64 is active, 1 = MMC64 is completely disabled         (R/W)  (**)
             *
             * (*) bit can only be programmed when flash jumper is set
             * (**) bit can only be modified after unlocking
             */
            value = mmc64_biossel;       /* bit 0 */
            value |= (spi_mmc_card_selected_read() << 1);  /* bit 1 */
            value |= (spi_mmc_enable_8mhz_read() << 2);    /* bit 2 */
            /* bit 3,4 always 0 */
            value |= mmc64_cport << 3;
            value |= mmc64_flashmode << 4;
            value |= (mmc64_extrom << 5); /* bit 5 */
            value |= (spi_mmc_trigger_mode_read() << 6);   /* bit 6 */
            value |= mmc64_active << 7;    /* bit 7 always 0 */
#ifdef LOG_READ_DF11
            LOG(("MMC64: IO2 RD %04x %02x mmc64_biossel %x mmc64_extrom %x", addr, value, mmc64_biossel, mmc64_extrom));
#endif
            return value;

        case 2:
            /*
             * $DF12: MMC status register
             *        -----------------------
             *        bit 0:  0 = SPI ready, 1 = SPI busy                       (R)
             *        bit 1:  feedback of $DE00 bit 0 (GAME)                    (R)
             *        bit 2:  feedback of $DE00 bit 1 (EXROM)                   (R)
             *        bit 3:  0 = card inserted, 1 = no card inserted           (R)
             *        bit 4:  0 = card write enabled, 1 = card write disabled   (R)
             *        bit 5:  0 = flash jumper not set, 1 = flash jumper set    (R)
             *        bit 6:  0
             *        bit 7:  0
             */
            value = mmc64_hw_flashjumper << 5;    /* bit 5 */
            value |= (spi_mmc_busy());     /* bit 0 */
            value |= ((mmc64_extexrom ^ 1) << 1);    /* bit 1 */
            value |= ((mmc64_extgame ^ 1)) << 2;       /* bit 2 */
            value |= (spi_mmc_card_inserted() ^ 1) << 3;   /* bit 3 */
            value |= (spi_mmc_card_write_enabled() ^ 1) << 4;      /* bit 4 */

            /* bit 6,7 not readable */
#ifdef LOG_READ_DF12
            LOG(("MMC64: IO2 RD %04x %02x mmc64_extgame %x mmc64_extexrom %x", addr, value, mmc64_extgame, (mmc64_extexrom ^ 1)));
#endif
            return value;

        /*
         * $DF13 (R/W): MMC64 identification register
         *              -----------------------------
         * (R) #$64 when bit 1 of $DF11 is 0
         *     #$01 when bit 1 of $DF11 is 1 and REV A hardware is used
         *     #$02 when bit 1 of $DF11 is 1 and REV B hardware is used
         */

        case 3:    /* MMC64 identification register */
            if (!mmc64_cardsel) {
                return 0x64;
            } else {
                if (mmc64_revision) {
                    return 2;
                } else {
                    return 1;
                }
            }
            break;
    }

    mmc64_io2_device.io_source_valid = 0;
    return 0;
}

static BYTE mmc64_io1_read(WORD addr)
{
    return mmc64_io2_read(addr);
}

static BYTE mmc64_io2_peek(WORD addr)
{
    BYTE value = 0;

    switch (addr) {
        case 0:
            /* $DF10: MMC SPI transfer register */
            return 0; /* FIXME */

        case 1:
            /* $DF11: MMC control register */
            value = mmc64_biossel;       /* bit 0 */
            value |= (spi_mmc_card_selected_read() << 1);  /* bit 1 */
            value |= (spi_mmc_enable_8mhz_read() << 2);    /* bit 2 */
            /* bit 3,4 always 0 */
            value |= mmc64_cport << 3;
            value |= mmc64_flashmode << 4;
            value |= (mmc64_extrom << 5); /* bit 5 */
            value |= (spi_mmc_trigger_mode_read() << 6);   /* bit 6 */
            value |= mmc64_active << 7;    /* bit 7 always 0 */
            return value;

        case 2:
            /* $DF12: MMC status register */
            value = mmc64_hw_flashjumper << 5;    /* bit 5 */
            value |= (spi_mmc_busy());     /* bit 0 */
            value |= ((mmc64_extexrom ^ 1) << 1);    /* bit 1 */
            value |= ((mmc64_extgame) ^ 1) << 2;       /* bit 2 */
            value |= (spi_mmc_card_inserted() ^ 1) << 3;   /* bit 3 */
            value |= (spi_mmc_card_write_enabled() ^ 1) << 4;      /* bit 4 */

            /* bit 6,7 not readable */
            return value;

        case 3:
            /* $DF13 (R/W): MMC64 identification register */
            if (!mmc64_cardsel) {
                return 0x64;
            } else {
                if (mmc64_revision) {
                    return 2;
                } else {
                    return 1;
                }
            }
            break;
    }
    return 0;
}

static BYTE mmc64_io1_peek(WORD addr)
{
    return mmc64_io2_peek(addr);
}

/* ---------------------------------------------------------------------*/

static BYTE mmc64_clockport_read(WORD address)
{
    if (clockport_device) {
        if (address < 0x02) {
            mmc64_current_clockport_device->io_source_valid = 0;
            return 0;
        }
        return clockport_device->read(address, &mmc64_current_clockport_device->io_source_valid, clockport_device->device_context);
    }
    return 0;
}

static BYTE mmc64_clockport_peek(WORD address)
{
    if (clockport_device) {
        if (address < 0x02) {
            return 0;
        }
        return clockport_device->peek(address, clockport_device->device_context);
    }
    return 0;
}

static void mmc64_clockport_store(WORD address, BYTE byte)
{
    if (clockport_device) {
        if (address < 0x02) {
            return;
        }

        clockport_device->store(address, byte, clockport_device->device_context);
    }
}

/* ---------------------------------------------------------------------*/

static int mmc64_dump(void)
{
    mon_out("Clockport is %s.\n", mmc64_clockport_enabled ? "enabled" : "disabled");
    mon_out("Clockport mapped to $%04x.\n", mmc64_hw_clockport);
    mon_out("Clockport device %s\n", clockport_device_id_to_name(clockport_device_id));

    return 0;
}

/* ---------------------------------------------------------------------*/

int mmc64_roml_read(WORD addr, BYTE *value)
{
#if USEPASSTHROUGHHACK
    if (!mmc64_active && !mmc64_biossel) {
        *value = mmc64_bios[(addr & 0x1fff) + mmc64_bios_offset];
        return CART_READ_VALID;
    }
    return CART_READ_THROUGH;
#else
    if (!mmc64_active && !mmc64_biossel) {
        return mmc64_bios[(addr & 0x1fff) + mmc64_bios_offset];
    }
    return mem_ram[addr];
#endif
}

int mmc64_peek_mem(WORD addr, BYTE *value)
{
    if ((addr >= 0x8000) && (addr <= 0x9fff)) {
        if (!mmc64_active && !mmc64_biossel) {
            *value = mmc64_bios[(addr & 0x1fff) + mmc64_bios_offset];
            return CART_READ_VALID;
        }
    }
    return CART_READ_THROUGH;
}

void mmc64_roml_store(WORD addr, BYTE byte)
{
    /* if (addr == 0x8000) LOG(("roml w %04x %02x active: %d == 0 bios: %d == 0 flashjumper: %d == 1 flashmode: %d == 1\n", addr, byte, mmc64_active, mmc64_biossel, mmc64_hw_flashjumper, mmc64_flashmode)); */
    if (!mmc64_active && !mmc64_biossel && mmc64_hw_flashjumper && mmc64_flashmode) {
        LOG(("MMC64 Flash w %04x %02x", addr, byte));
        if (mmc64_bios[(addr & 0x1fff) + mmc64_bios_offset] != byte) {
            mmc64_bios[(addr & 0x1fff) + mmc64_bios_offset] = byte;
            mmc64_bios_changed = 1;
            return;
        }
    }
    /* mem_ram[addr] = byte; */
}

/* ---------------------------------------------------------------------*/

static const resource_string_t resources_string[] = {
    { "MMC64BIOSfilename", "", RES_EVENT_NO, NULL,
      &mmc64_bios_filename, set_mmc64_bios_filename, NULL },
    { "MMC64imagefilename", "", RES_EVENT_NO, NULL,
      &mmc64_image_filename, set_mmc64_image_filename, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "MMC64", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &mmc64_enabled, set_mmc64_enabled, (void *)1 },
    { "MMC64_RO", 0, RES_EVENT_NO, NULL,
      &mmc64_hw_writeprotect, set_mmc64_readonly, NULL },
    { "MMC64_flashjumper", 0, RES_EVENT_NO, NULL,
      &mmc64_hw_flashjumper, set_mmc64_flashjumper, NULL },
    { "MMC64_revision", MMC64_REV_A, RES_EVENT_NO, NULL,
      &mmc64_revision, set_mmc64_revision, NULL },
    { "MMC64_bios_write", 0, RES_EVENT_NO, NULL,
      &mmc64_bios_write, set_mmc64_bios_write, NULL },
    { "MMC64_sd_type", MMC64_TYPE_AUTO, RES_EVENT_NO, NULL,
      &mmc64_sd_type, set_mmc64_sd_type, NULL },
    { "MMC64ClockPort", 0, RES_EVENT_NO, NULL,
      &clockport_device_id, set_mmc64_clockport_device, NULL },
    RESOURCE_INT_LIST_END
};

int mmc64_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void mmc64_resources_shutdown(void)
{
    lib_free(mmc64_bios_filename);
    lib_free(mmc64_image_filename);
    mmc64_bios_filename = NULL;
    mmc64_image_filename = NULL;
    lib_free(clockport_device_names);
    clockport_device_names = NULL;
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-mmc64", SET_RESOURCE, 0,
      NULL, NULL, "MMC64", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_MMC64,
      NULL, NULL },
    { "+mmc64", SET_RESOURCE, 0,
      NULL, NULL, "MMC64", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_MMC64,
      NULL, NULL },
    { "-mmc64bios", SET_RESOURCE, 1,
      NULL, NULL, "MMC64BIOSfilename", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_MMC64_BIOS_NAME,
      NULL, NULL },
    { "-mmc64image", SET_RESOURCE, 1,
      NULL, NULL, "MMC64imagefilename", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_MMC64_IMAGE_NAME,
      NULL, NULL },
    { "-mmc64readonly", SET_RESOURCE, 0,
      NULL, NULL, "MMC64_RO", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_MMC64_READONLY,
      NULL, NULL },
    { "-mmc64readwrite", SET_RESOURCE, 0,
      NULL, NULL, "MMC64_RO", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_MMC64_READWRITE,
      NULL, NULL },
    { "-mmc64bioswrite", SET_RESOURCE, 0,
      NULL, NULL, "MMC64_bios_write", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_MMC64_BIOS_WRITE,
      NULL, NULL },
    { "-mmc64biosreadonly", SET_RESOURCE, 0,
      NULL, NULL, "MMC64_bios_write", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_MMC64_BIOS_READ_ONLY,
      NULL, NULL },
    { "-mmc64flash", SET_RESOURCE, 0,
      NULL, NULL, "MMC64_flashjumper", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_MMC64_SET_FLASH_JUMPER,
      NULL, NULL },
    { "+mmc64flash", SET_RESOURCE, 0,
      NULL, NULL, "MMC64_flashjumper", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_MMC64_UNSET_FLASH_JUMPER,
      NULL, NULL },
    { "-mmc64rev", SET_RESOURCE, 1,
      NULL, NULL, "MMC64_revision", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_REVISION, IDCLS_SPECIFY_MMC64_REVISION,
      NULL, NULL },
    { "-mmc64sdtype", SET_RESOURCE, 1,
      NULL, NULL, "MMC64_sd_type", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_TYPE, IDCLS_SPECIFY_MMC64_SD_TYPE,
      NULL, NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t clockport_cmdline_options[] =
{
    { "-mmc64clockportdevice", SET_RESOURCE, 1,
      NULL, NULL, "MMC64ClockPort", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_COMBO,
      IDCLS_P_DEVICE, IDCLS_CLOCKPORT_DEVICE,
      NULL, NULL },
    CMDLINE_LIST_END
};

int mmc64_cmdline_options_init(void)
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

/* ------------------------------------------------------------------------- */

const char *mmc64_get_file_name(void)
{
    return mmc64_bios_filename;
}

void mmc64_init(void)
{
    mmc64_log = log_open("MMC64");
}

void mmc64_config_setup(BYTE *rawcart)
{
    memcpy(mmc64_bios, rawcart, 0x2000 + mmc64_bios_offset);
}

static int mmc64_common_attach(void)
{
    return set_mmc64_enabled(1, NULL);
}

int mmc64_bin_save(const char *filename)
{
    FILE *fd;
    int ret;

    if (filename == NULL) {
        return -1;
    }

    fd = fopen(filename, MODE_WRITE);
    if (fd == NULL) {
        return -1;
    }

    ret = fwrite(mmc64_bios, 1, 0x2000 + mmc64_bios_offset, fd);
    fclose(fd);
    if (ret != 0x2000 + mmc64_bios_offset) {
        return -1;
    }
    mmc64_bios_changed = 0;
    return 0;
}

int mmc64_crt_save(const char *filename)
{
    FILE *fd;
    crt_chip_header_t chip;

    fd = crt_create(filename, CARTRIDGE_MMC64, 1, 0, STRING_MMC64);

    if (fd == NULL) {
        return -1;
    }

    chip.type = 2;
    chip.size = 0x2000;
    chip.start = 0x8000;
    chip.bank = 0;

    if (crt_write_chip(mmc64_bios, &chip, fd)) {
        fclose(fd);
        return -1;
    }

    fclose(fd);
    return 0;
}

int mmc64_bin_attach(const char *filename, BYTE *rawcart)
{
    int amount_read = 0;
    FILE *fd;

    fd = fopen(filename, MODE_READ);
    if (!fd) {
        return -1;
    }

    amount_read = (int)fread(rawcart, 1, 0x2002, fd);
    fclose(fd);

    if (amount_read != 0x2000 && amount_read != 0x2002) {
        return -1;
    }

    mmc64_bios_offset = amount_read & 3;
    mmc64_bios_type = CARTRIDGE_FILETYPE_BIN;
    return mmc64_common_attach();
}

int mmc64_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.bank > 1 || chip.size != 0x2000) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    mmc64_bios_offset = 0;
    mmc64_bios_type = CARTRIDGE_FILETYPE_CRT;
    return mmc64_common_attach();
}

int mmc64_flush_image(void)
{
    if (mmc64_bios_type == CARTRIDGE_FILETYPE_BIN) {
        return mmc64_bin_save(mmc64_bios_filename);
    } else if (mmc64_bios_type == CARTRIDGE_FILETYPE_CRT) {
        return mmc64_crt_save(mmc64_bios_filename);
    }
    return -1;
}

void mmc64_detach(void)
{
    set_mmc64_enabled(0, NULL);
}

int mmc64_enable(void)
{
    return set_mmc64_enabled(1, (void*)1);
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

/* CARTMMC64 snapshot module format:

   type  | name              | description
   ---------------------------------------
   BYTE  | clockport enable  | clockport enable flag
   WORD  | clockport address | clockport address
   BYTE  | bios write        | bios writable flag
   BYTE  | bit 7 unlocked    | bit 7 unlocked flag
   ARRAY | unlocking         | 2 BYTES of unlocking data
   BYTE  | bios changed      | bios changed flag
   BYTE  | flash jumper      | flash jumper state
   BYTE  | write protect     | write protect flag
   BYTE  | active            | cartridge active flag
   BYTE  | SPI mode          | SPI mode
   BYTE  | extrom            | extrom flag
   BYTE  | flash mode        | flash mode
   BYTE  | cport             | cport
   BYTE  | speed             | speed
   BYTE  | cardsel           | cardsel
   BYTE  | biossel           | biossel
   BYTE  | extexrom          | EXROM line state
   BYTE  | extgame           | GAME line state
   BYTE  | revision          | MMC64 revision
   BYTE  | SD type           | SD type
   BYTE  | image readonly    | image read-only flag
   ARRAY | BIOS              | 8194 BYTES of BIOS data
   BYTE  | BIOS offset       | BIOS offset
   BYTE  | BIOS type         | BIOS type
 */

static char snap_module_name[] = "CARTMMC64";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int mmc64_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (BYTE)mmc64_clockport_enabled) < 0
        || SMW_W(m, (WORD)mmc64_hw_clockport) < 0
        || SMW_B(m, (BYTE)mmc64_bios_write) < 0
        || SMW_B(m, (BYTE)mmc64_bit7_unlocked) < 0
        || SMW_B(m, mmc64_unlocking[0]) < 0
        || SMW_B(m, mmc64_unlocking[1]) < 0
        || SMW_B(m, (BYTE)mmc64_bios_changed) < 0
        || SMW_B(m, (BYTE)mmc64_hw_flashjumper) < 0
        || SMW_B(m, (BYTE)mmc64_hw_writeprotect) < 0
        || SMW_B(m, mmc64_active) < 0
        || SMW_B(m, mmc64_spi_mode) < 0
        || SMW_B(m, mmc64_extrom) < 0
        || SMW_B(m, mmc64_flashmode) < 0
        || SMW_B(m, mmc64_cport) < 0
        || SMW_B(m, mmc64_speed) < 0
        || SMW_B(m, mmc64_cardsel) < 0
        || SMW_B(m, mmc64_biossel) < 0
        || SMW_B(m, mmc64_extexrom) < 0
        || SMW_B(m, mmc64_extgame) < 0
        || SMW_B(m, (BYTE)mmc64_revision) < 0
        || SMW_B(m, (BYTE)mmc64_sd_type) < 0
        || SMW_B(m, mmc64_image_file_readonly) < 0
        || SMW_BA(m, mmc64_bios, 0x2002) < 0
        || SMW_B(m, (BYTE)mmc64_bios_offset) < 0
        || SMW_B(m, (BYTE)mmc64_bios_type) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int mmc64_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (vmajor > SNAP_MAJOR || vminor > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || SMR_B_INT(m, &mmc64_clockport_enabled) < 0
        || SMR_W_INT(m, &mmc64_hw_clockport) < 0
        || SMR_B_INT(m, &mmc64_bios_write) < 0
        || SMR_B_INT(m, &mmc64_bit7_unlocked) < 0
        || SMR_B(m, &mmc64_unlocking[0]) < 0
        || SMR_B(m, &mmc64_unlocking[1]) < 0
        || SMR_B_INT(m, &mmc64_bios_changed) < 0
        || SMR_B_INT(m, &mmc64_hw_flashjumper) < 0
        || SMR_B_INT(m, &mmc64_hw_writeprotect) < 0
        || SMR_B(m, &mmc64_active) < 0
        || SMR_B(m, &mmc64_spi_mode) < 0
        || SMR_B(m, &mmc64_extrom) < 0
        || SMR_B(m, &mmc64_flashmode) < 0
        || SMR_B(m, &mmc64_cport) < 0
        || SMR_B(m, &mmc64_speed) < 0
        || SMR_B(m, &mmc64_cardsel) < 0
        || SMR_B(m, &mmc64_biossel) < 0
        || SMR_B(m, &mmc64_extexrom) < 0
        || SMR_B(m, &mmc64_extgame) < 0
        || SMR_B_INT(m, &mmc64_revision) < 0
        || SMR_B_INT(m, &mmc64_sd_type) < 0
        || SMR_B(m, &mmc64_image_file_readonly) < 0
        || SMR_BA(m, mmc64_bios, 0x2002) < 0
        || SMR_B_INT(m, &mmc64_bios_offset) < 0
        || SMR_B_INT(m, &mmc64_bios_type) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    mmc64_enabled = 1;

    /* FIXME: ugly code duplication to avoid cart_config_changed calls */
    mmc64_io1_list_item = io_source_register(&mmc64_io1_device);
    mmc64_io2_list_item = io_source_register(&mmc64_io2_device);

    if (mmc64_clockport_enabled) {
        if (mmc64_hw_clockport == 0xde02) {
            mmc64_current_clockport_device = &mmc64_io1_clockport_device;
        } else {
            mmc64_current_clockport_device = &mmc64_io2_clockport_device;
        }
        mmc64_clockport_list_item = io_source_register(mmc64_current_clockport_device);
    }

    if (export_add(&export_res) < 0) {
        io_source_unregister(mmc64_io1_list_item);
        io_source_unregister(mmc64_io2_list_item);
        io_source_unregister(mmc64_clockport_list_item);
        mmc64_io1_list_item = NULL;
        mmc64_io2_list_item = NULL;
        mmc64_clockport_list_item = NULL;
        mmc64_enabled = 0;
        return -1;
    }

    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
