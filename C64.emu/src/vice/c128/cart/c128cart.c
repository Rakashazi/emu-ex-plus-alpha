/*
 * c128cart.c -- c128 cartridge memory interface.
 *
 * Written by
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
#include <stdlib.h>
#include <string.h>

#include "c128mem.h"
#include "cmdline.h"
#include "lib.h"
#include "log.h"
#include "viciitypes.h"
#include "vicii-phi1.h"

#include "cartridge.h"
#include "export.h"
#include "monitor.h"

#include "c64cart.h"
#include "c64cartsystem.h"
#include "c128cart.h"
#include "functionrom.h"

#include "generic.h"
#include "comal80.h"
#include "gmod2c128.h"
#include "magicdesk128.h"
#include "partner128.h"
#include "warpspeed128.h"
#include "ltkernal.h"
#include "ramlink.h"

/* #define DBGC128CART */

#ifdef DBGC128CART
#define DBG(x) printf x
#else
#define DBG(x)
#endif

extern c128cartridge_interface_t *c128cartridge; /* lives in c64cart.c */

static c128cartridge_interface_t c128interface;

uint8_t ext_function_rom[EXTERNAL_FUNCTION_ROM_SIZE * EXTERNAL_FUNCTION_ROM_BANKS];

uint8_t ext_function_rom_bank = 0;

/* FIXME: get rid of these */
#define EXT_FUNCTION_NONE   0
#define EXT_FUNCTION_ROM    1

/* FIXME: get rid of this */
/* Flag: Do we enable the external function ROM?  */
static int external_function_rom_enabled = EXT_FUNCTION_NONE;

/* CAUTION: keep in sync with the list in c64/c64cart.c */
static cartridge_info_t cartlist[] = {
    /* standard cartridges with CRT ID = 0 */
    { CARTRIDGE_C128_NAME_GENERIC,        CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC),        CARTRIDGE_GROUP_GENERIC },
    { CARTRIDGE_NAME_GENERIC_8KB,         CARTRIDGE_GENERIC_8KB,         CARTRIDGE_GROUP_GENERIC },
    { CARTRIDGE_NAME_GENERIC_16KB,        CARTRIDGE_GENERIC_16KB,        CARTRIDGE_GROUP_GENERIC },
    { CARTRIDGE_NAME_ULTIMAX,             CARTRIDGE_ULTIMAX,             CARTRIDGE_GROUP_GENERIC },

    /* all cartridges with a CRT ID > 0, alphabetically sorted */
    { CARTRIDGE_NAME_ACTION_REPLAY,       CARTRIDGE_ACTION_REPLAY,       CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_ACTION_REPLAY2,      CARTRIDGE_ACTION_REPLAY2,      CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_ACTION_REPLAY3,      CARTRIDGE_ACTION_REPLAY3,      CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_ACTION_REPLAY4,      CARTRIDGE_ACTION_REPLAY4,      CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_ATOMIC_POWER,        CARTRIDGE_ATOMIC_POWER,        CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_BISPLUS,             CARTRIDGE_BISPLUS,             CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_BLACKBOX3,           CARTRIDGE_BLACKBOX3,           CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_BLACKBOX4,           CARTRIDGE_BLACKBOX4,           CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_BLACKBOX8,           CARTRIDGE_BLACKBOX8,           CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_BLACKBOX9,           CARTRIDGE_BLACKBOX9,           CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_CAPTURE,             CARTRIDGE_CAPTURE,             CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_COMAL80,             CARTRIDGE_COMAL80,             CARTRIDGE_GROUP_UTIL },

    { CARTRIDGE_C128_NAME_COMAL80,        CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80),             CARTRIDGE_GROUP_UTIL },

    { CARTRIDGE_NAME_DELA_EP256,          CARTRIDGE_DELA_EP256,          CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_DELA_EP64,           CARTRIDGE_DELA_EP64,           CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_DELA_EP7x8,          CARTRIDGE_DELA_EP7x8,          CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_DIASHOW_MAKER,       CARTRIDGE_DIASHOW_MAKER,       CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_DINAMIC,             CARTRIDGE_DINAMIC,             CARTRIDGE_GROUP_GAME },
    { CARTRIDGE_NAME_EASYCALC,            CARTRIDGE_EASYCALC,            CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_EASYFLASH,           CARTRIDGE_EASYFLASH,           CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_EPYX_FASTLOAD,       CARTRIDGE_EPYX_FASTLOAD,       CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_EXOS,                CARTRIDGE_EXOS,                CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_EXPERT,              CARTRIDGE_EXPERT,              CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_FINAL_I,             CARTRIDGE_FINAL_I,             CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_FINAL_III,           CARTRIDGE_FINAL_III,           CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_FINAL_PLUS,          CARTRIDGE_FINAL_PLUS,          CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_FORMEL64,            CARTRIDGE_FORMEL64,            CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_FREEZE_FRAME,        CARTRIDGE_FREEZE_FRAME,        CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_FREEZE_FRAME_MK2,    CARTRIDGE_FREEZE_FRAME_MK2,    CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_FREEZE_MACHINE,      CARTRIDGE_FREEZE_MACHINE,      CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_FUNPLAY,             CARTRIDGE_FUNPLAY,             CARTRIDGE_GROUP_GAME },
    { CARTRIDGE_NAME_GAME_KILLER,         CARTRIDGE_GAME_KILLER,         CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_GMOD2,               CARTRIDGE_GMOD2,               CARTRIDGE_GROUP_GAME },

    { CARTRIDGE_C128_NAME_GMOD2C128,               CARTRIDGE_C128_GMOD2C128,               CARTRIDGE_GROUP_GAME },

    { CARTRIDGE_NAME_GMOD3,               CARTRIDGE_GMOD3,               CARTRIDGE_GROUP_GAME },
    { CARTRIDGE_NAME_GS,                  CARTRIDGE_GS,                  CARTRIDGE_GROUP_GAME },
    { CARTRIDGE_NAME_DREAN,               CARTRIDGE_DREAN,               CARTRIDGE_GROUP_GAME },
    { CARTRIDGE_NAME_IDE64,               CARTRIDGE_IDE64,               CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_IEEE488,             CARTRIDGE_IEEE488,             CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_IEEEFLASH64,         CARTRIDGE_IEEEFLASH64,         CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_KCS_POWER,           CARTRIDGE_KCS_POWER,           CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_KINGSOFT,            CARTRIDGE_KINGSOFT,            CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_LT_KERNAL,           CARTRIDGE_LT_KERNAL,           CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_MACH5,               CARTRIDGE_MACH5,               CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_MAGIC_DESK,          CARTRIDGE_MAGIC_DESK,          CARTRIDGE_GROUP_UTIL },

    { CARTRIDGE_C128_NAME_MAGICDESK128,   CARTRIDGE_C128_MAGICDESK128,   CARTRIDGE_GROUP_UTIL },

    { CARTRIDGE_NAME_MAGIC_FORMEL,        CARTRIDGE_MAGIC_FORMEL,        CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_MAGIC_VOICE,         CARTRIDGE_MAGIC_VOICE,         CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_MAX_BASIC,           CARTRIDGE_MAX_BASIC,           CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_MIKRO_ASSEMBLER,     CARTRIDGE_MIKRO_ASSEMBLER,     CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_MMC64,               CARTRIDGE_MMC64,               CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_MMC_REPLAY,          CARTRIDGE_MMC_REPLAY,          CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_MULTIMAX,            CARTRIDGE_MULTIMAX,            CARTRIDGE_GROUP_GAME },
    { CARTRIDGE_NAME_OCEAN,               CARTRIDGE_OCEAN,               CARTRIDGE_GROUP_GAME },
    { CARTRIDGE_NAME_P64,                 CARTRIDGE_P64,                 CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_PAGEFOX,             CARTRIDGE_PAGEFOX,             CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_PARTNER64,           CARTRIDGE_PARTNER64,           CARTRIDGE_GROUP_UTIL },

    { CARTRIDGE_C128_NAME_PARTNER128,     CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128),             CARTRIDGE_GROUP_UTIL },

    { CARTRIDGE_NAME_RAMLINK,             CARTRIDGE_RAMLINK,             CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_RETRO_REPLAY,        CARTRIDGE_RETRO_REPLAY,        CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_REX,                 CARTRIDGE_REX,                 CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_REX_EP256,           CARTRIDGE_REX_EP256,           CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_REX_RAMFLOPPY,       CARTRIDGE_REX_RAMFLOPPY,       CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_RGCD,                CARTRIDGE_RGCD,                CARTRIDGE_GROUP_GAME },
#ifdef HAVE_RAWNET
    { CARTRIDGE_NAME_RRNETMK3,            CARTRIDGE_RRNETMK3,            CARTRIDGE_GROUP_UTIL },
#endif
    { CARTRIDGE_NAME_ROSS,                CARTRIDGE_ROSS,                CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_SDBOX,               CARTRIDGE_SDBOX,               CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_SILVERROCK_128,      CARTRIDGE_SILVERROCK_128,      CARTRIDGE_GROUP_GAME },
    { CARTRIDGE_NAME_SIMONS_BASIC,        CARTRIDGE_SIMONS_BASIC,        CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_SNAPSHOT64,          CARTRIDGE_SNAPSHOT64,          CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_STARDOS,             CARTRIDGE_STARDOS,             CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_STRUCTURED_BASIC,    CARTRIDGE_STRUCTURED_BASIC,    CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_SUPER_EXPLODE_V5,    CARTRIDGE_SUPER_EXPLODE_V5,    CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_SUPER_GAMES,         CARTRIDGE_SUPER_GAMES,         CARTRIDGE_GROUP_GAME },
    { CARTRIDGE_NAME_SUPER_SNAPSHOT,      CARTRIDGE_SUPER_SNAPSHOT,      CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_SUPER_SNAPSHOT_V5,   CARTRIDGE_SUPER_SNAPSHOT_V5,   CARTRIDGE_GROUP_FREEZER },
    { CARTRIDGE_NAME_TURTLE_GRAPHICS_II,  CARTRIDGE_TURTLE_GRAPHICS_II,  CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_WARPSPEED,           CARTRIDGE_WARPSPEED,           CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_C128_NAME_WARPSPEED128,   CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128),        CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_WESTERMANN,          CARTRIDGE_WESTERMANN,          CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_NAME_ZAXXON,              CARTRIDGE_ZAXXON,              CARTRIDGE_GROUP_GAME },
    { CARTRIDGE_NAME_ZIPPCODE48,          CARTRIDGE_ZIPPCODE48,          CARTRIDGE_GROUP_UTIL },

    /* cartridges that have only RAM, these do not have a CRT ID */
    { CARTRIDGE_NAME_DQBB,                CARTRIDGE_DQBB,                CARTRIDGE_GROUP_RAMEX },
    { CARTRIDGE_NAME_GEORAM,              CARTRIDGE_GEORAM,              CARTRIDGE_GROUP_RAMEX },
    { CARTRIDGE_NAME_ISEPIC,              CARTRIDGE_ISEPIC,              CARTRIDGE_GROUP_RAMEX },
    { CARTRIDGE_NAME_RAMCART,             CARTRIDGE_RAMCART,             CARTRIDGE_GROUP_RAMEX },
    { CARTRIDGE_NAME_REU,                 CARTRIDGE_REU,                 CARTRIDGE_GROUP_RAMEX },

    { NULL, 0, 0 }
};

static cartridge_info_t *c128cartridge_get_info_list(void)
{
    return &cartlist[0];
}

static void c128cartridge_config_init(int type)
{
    DBG(("c128cartridge_config_init()\n"));

    switch (type) {
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128):
            c128gmod2_config_init();
            break;
    }
}

/* copy data from rawcart into actually used ROM array(s). (called from c64carthooks:cart_attach()) */
static void c128cartridge_config_setup(int type, uint8_t *rawcart)
{
    DBG(("c128cartridge_config_setup(ptr: 0x%p)\n", (void*)rawcart));

    switch (type) {
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):
            c128generic_config_setup(rawcart);
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):
            c128comal80_config_setup(rawcart);
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128):
            c128gmod2_config_setup(rawcart);
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128):
            magicdesk128_config_setup(rawcart);
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
            partner128_config_setup(rawcart);
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):
            warpspeed128_config_setup(rawcart);
            break;
    }
}

static int c128cartridge_attach_crt(int type, FILE *fd, const char *filename, uint8_t *rawcart)
{
    int res = -1;

    DBG(("c128cartridge_attach_crt type: %d fd: %p ptr:%p\n", type, (void*)fd, (void*)rawcart));

    switch (type) {
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):
            res = c128generic_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):
            res = c128comal80_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128):
            res = c128gmod2_crt_attach(fd, rawcart, filename);
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128):
            res = magicdesk128_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
            res = partner128_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):
            res = warpspeed128_crt_attach(fd, rawcart);
            break;
    }
    if (res != -1) {
        /* FIXME: get rid of this flag */
        external_function_rom_enabled = EXT_FUNCTION_ROM;
    }
    return res;
}

static int c128cartridge_bin_attach(int type, const char *filename, uint8_t *rawcart)
{
    int res = -1;
    DBG(("c128cartridge_bin_attach type: %d name: %s\n", type, filename));
    switch (type) {
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):
            res = c128generic_bin_attach(filename, rawcart);
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):
            res = c128comal80_bin_attach(filename, rawcart);
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128):
            res = c128gmod2_bin_attach(filename, rawcart);
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128):
            res = magicdesk128_bin_attach(filename, rawcart);
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
            res = partner128_bin_attach(filename, rawcart);
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):
            res = warpspeed128_bin_attach(filename, rawcart);
            break;
    }
    if (res != -1) {
        /* FIXME: get rid of this flag */
        external_function_rom_enabled = EXT_FUNCTION_ROM;
    }
    return res;
}

static int c128cartridge_bin_save(int type, const char *filename)
{
    DBG(("c128cartridge_bin_save name: %s\n", filename));
    switch (type) {
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128):
            return c128gmod2_bin_save(filename);
    }
    log_error(LOG_ERR, "Failed saving binary cartridge image for cartridge ID %d.\n", type);
    return -1;
}

static int c128cartridge_save_secondary_image(int type, const char *filename)
{
    DBG(("c128cartridge_save_secondary_image name: %s\n", filename));
    switch (type) {
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128):
            return c128gmod2_eeprom_save(filename);
    }
    log_error(LOG_ERR, "Failed saving secondary image for cartridge ID %d.\n", type);
    return -1;
}

static int c128cartridge_crt_save(int type, const char *filename)
{
    DBG(("c128cartridge_crt_save name: %s\n", filename));
    switch (type) {
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128):
            return c128gmod2_crt_save(filename);
    }
    log_error(LOG_ERR, "Failed saving .crt cartridge image for cartridge ID %d.\n", type);
    return -1;
}

static int c128cartridge_flush_image(int type)
{
    switch (type) {
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128):
            return c128gmod2_flush_image();
    }
    log_error(LOG_ERR, "Failed flushing cartridge image for cartridge ID %d.\n", type);
    return -1;
}

static int c128cartridge_flush_secondary_image(int type)
{
    switch (type) {
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128):
            return c128gmod2_flush_eeprom();
    }
    log_error(LOG_ERR, "Failed flushing secondary for cartridge ID %d.\n", type);
    return -1;
}

static int c128cartridge_can_save_image(int crtid)
{
    switch (crtid) {
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128):
            return 1;
    }
    return 0;
}

static int c128cartridge_can_flush_image(int crtid)
{
    switch (crtid) {
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128):
            return 1;
    }
    return 0;
}

static int c128cartridge_can_save_secondary_image(int crtid)
{
    switch (crtid) {
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128):
            return c128gmod2_can_save_eeprom();
    }
    return 0;
}

static int c128cartridge_can_flush_secondary_image(int crtid)
{
    switch (crtid) {
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128):
            return c128gmod2_can_flush_eeprom();
    }
    return 0;
}

/*
    detach a cartridge.
    - carts that are not "main" cartridges can be disabled individually
    - if type is -1, then all carts will get detached
    - if type is 0, then cart in main slot will get detached

    - carts not in "Main Slot" must make sure their detach hook does not
      fail when it is called and the cart is not actually attached.
*/
static void c128cartridge_detach_image(int type)
{
    DBG(("c128cartridge_detach_image type: %d\n", type));
    if (type == 0) {
        type = cartridge_get_id(0);
        DBG(("c128cartridge_detach_image  got type: %d\n", type));
    }
    switch (type) {
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):
            c128generic_detach();
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):
            c128comal80_detach();
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128):
            c128gmod2_detach();
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128):
            magicdesk128_detach();
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
            partner128_detach();
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):
            warpspeed128_detach();
            break;
        case -1:
            c128generic_detach();
            c128comal80_detach();
            c128gmod2_detach();
            magicdesk128_detach();
            partner128_detach();
            warpspeed128_detach();
            break;
    }
    memset(ext_function_rom, 0xff, EXTERNAL_FUNCTION_ROM_SIZE * EXTERNAL_FUNCTION_ROM_BANKS);
    /* FIXME: get rid of this flag */
    external_function_rom_enabled = EXT_FUNCTION_NONE;
}

static void c128cartridge_reset(void)
{
    c128generic_reset();
    c128gmod2_reset();
    c128comal80_reset();
    magicdesk128_reset();
    partner128_reset();
    warpspeed128_reset();
}

static int c128cartridge_freeze_allowed(void)
{
    switch(cartridge_get_id(0)) {
        /*case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):*/
        /*case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):*/
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
        /*case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):*/
        case CARTRIDGE_LT_KERNAL:
            return 1;
    }
    return 0;
}

static void c128cartridge_freeze(void)
{
    switch(cartridge_get_id(0)) {
        /*case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):*/
        /*case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):*/
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
            partner128_freeze();
            break;
        /*case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):*/
        case CARTRIDGE_LT_KERNAL:
            ltkernal_freeze();
            break;
    }
}

static void c128cartridge_powerup(void)
{
    switch(cartridge_get_id(0)) {
        /*case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):*/
        /*case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):*/
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
            partner128_powerup();
            break;
        /*case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):*/
    }
}

void c128cartridge_setup_interface(void)
{
    DBG(("c128cartridge_setup_interface\n"));
    /* assign the function pointers for the interface that is used in the c64 cartridge system */
    c128interface.attach_crt = c128cartridge_attach_crt;
    c128interface.bin_attach = c128cartridge_bin_attach;
    c128interface.bin_save = c128cartridge_bin_save;
    c128interface.save_secondary_image = c128cartridge_save_secondary_image;
    c128interface.crt_save = c128cartridge_crt_save;
    c128interface.flush_image = c128cartridge_flush_image;
    c128interface.flush_secondary_image = c128cartridge_flush_secondary_image;
    c128interface.detach_image = c128cartridge_detach_image;
    c128interface.config_init = c128cartridge_config_init;
    c128interface.config_setup = c128cartridge_config_setup;
    c128interface.get_info_list = c128cartridge_get_info_list;
    c128interface.reset = c128cartridge_reset;
    c128interface.freeze_allowed = c128cartridge_freeze_allowed;
    c128interface.freeze = c128cartridge_freeze;
    c128interface.powerup = c128cartridge_powerup;
    c128interface.can_flush_image = c128cartridge_can_flush_image;
    c128interface.can_save_image = c128cartridge_can_save_image;
    c128interface.can_flush_secondary_image = c128cartridge_can_flush_secondary_image;
    c128interface.can_save_secondary_image = c128cartridge_can_save_secondary_image;
    c128cartridge = &c128interface;
}

int c128cartridge_resources_init(void)
{
    if (c128gmod2_resources_init() < 0) {
        return -1;
    }
    return 0;
}

void c128cartridge_resources_shutdown(void)
{
    c128gmod2_resources_shutdown();
}

static const cmdline_option_t cmdline_options[] =
{
    /* generic C128 cartridges */
    { "-cartfrom", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cart_attach_cmdline, (void *)CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC), NULL, NULL,
      "<Name>", "Attach generic external function ROM cartridge image" },

    /* C128 specific cartridges */
    { "-cartcomal128", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cart_attach_cmdline, (void *)CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80), NULL, NULL,
      "<Name>", "Attach 96k " CARTRIDGE_C128_NAME_COMAL80 " cartridge image" },
    { "-cartgmod128", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cart_attach_cmdline, (void *)CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128), NULL, NULL,
      "<Name>", "Attach 516k " CARTRIDGE_C128_NAME_GMOD2C128 " cartridge image" },
    { "-cartmd128", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cart_attach_cmdline, (void *)CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128), NULL, NULL,
      "<Name>", "Attach 16/32/64/128/256/512k/1M " CARTRIDGE_C128_NAME_MAGICDESK128 " cartridge image" },
    { "-cartpartner128", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cart_attach_cmdline, (void *)CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128), NULL, NULL,
      "<Name>", "Attach 16k " CARTRIDGE_C128_NAME_PARTNER128 " cartridge image" },
    { "-cartws128", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cart_attach_cmdline, (void *)CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128), NULL, NULL,
      "<Name>", "Attach 16k " CARTRIDGE_C128_NAME_WARPSPEED128 " cartridge image" },

    CMDLINE_LIST_END
};

static void c128cartridge_export_dump(void)
{
    export_dump();
    if (external_function_rom_enabled == EXT_FUNCTION_ROM) {
        int roml = 1, romh = 1; /* FIXME: make seperate flags */
        mon_out("C128 mode external function ROML: %s\n", roml ? "enabled" : "disabled");
        mon_out("C128 mode external function ROMH: %s\n", romh ? "enabled" : "disabled");
    }
}

int c128cartridge_cmdline_options_init(void)
{

    mon_cart_cmd.cartridge_attach_image = cartridge_attach_image;
    mon_cart_cmd.cartridge_detach_image = cartridge_detach_image;
    mon_cart_cmd.cartridge_trigger_freeze = cartridge_trigger_freeze;
    mon_cart_cmd.cartridge_trigger_freeze_nmi_only = cartridge_trigger_freeze_nmi_only;
    mon_cart_cmd.export_dump = c128cartridge_export_dump;

    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }
    if (c128gmod2_cmdline_options_init() < 0) {
        return -1;
    }
    return 0;
}

/*****************************************************************************/

/* set the bank for generic function rom access */
void external_function_rom_set_bank(int value)
{
    ext_function_rom_bank = value;
}

/* ROML and ROMH reads at the cartridge port */
uint8_t external_function_rom_read(uint16_t addr)
{
    int type = cart_getid_slot0();
    uint8_t val = vicii_read_phi1();
    /* do slot0 first */
    switch(type) {
        case CARTRIDGE_RAMLINK:
            if (c128ramlink_roml_read(addr, &val)) {
                vicii.last_cpu_val = val;
                return vicii.last_cpu_val;
            }
    }
    /* then do slotmain */
    type = cartridge_get_id(0);
    switch(type) {
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128):
            val = c128gmod2_roml_read(addr);
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):
            val = ext_function_rom[(addr & (EXTERNAL_FUNCTION_ROM_SIZE - 1)) + (ext_function_rom_bank * EXTERNAL_FUNCTION_ROM_SIZE)];
            break;
        case CARTRIDGE_MMC_REPLAY:
            if (mmcreplay_c128_read(addr, &val)) {
                break;
            }
            break;
        case CARTRIDGE_LT_KERNAL:
            if (c128ltkernal_roml_read(addr, &val)) {
                break;
            }
            break;
        default:
            val = vicii_read_phi1();
            break;
    }
    vicii.last_cpu_val = val;
    return vicii.last_cpu_val;
}

/* ROML and ROMH peeks at the cartridge port */
uint8_t external_function_rom_peek(uint16_t addr)
{
    int type = cartridge_get_id(0);
    /* FIXME: What should we return here? is vicii_read_phi1() safe for a peek? */
    uint8_t val = 0;
    switch(type) {
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128):
/* FIXME: gmod2 needs a peek */
/*            val = c128gmod2_roml_peek(addr); */
            val = 0;
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):
            val = ext_function_rom[(addr & (EXTERNAL_FUNCTION_ROM_SIZE - 1)) + (ext_function_rom_bank * EXTERNAL_FUNCTION_ROM_SIZE)];
            break;
        case CARTRIDGE_MMC_REPLAY:
            if (mmcreplay_c128_read(addr, &val)) {
                break;
            }
            break;
        case CARTRIDGE_LT_KERNAL:
            if (c128ltkernal_roml_read(addr, &val)) {
                break;
            }
            break;
        default:
            /* default value above */
            break;
    }
    return val;
}

/* ROML and ROMH stores at the cartridge port */
void external_function_rom_store(uint16_t addr, uint8_t value)
{
    int type = cartridge_get_id(0);
    switch(type) {
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128):
            c128gmod2_roml_store(addr, value);
            break;
        case CARTRIDGE_LT_KERNAL:
            /* LTK doesn't write to internal RAM */
            if (c128ltkernal_roml_store(addr, value)) {
                vicii.last_cpu_val = value;
                return;
            }
            break;
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_COMAL80):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_MAGICDESK128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_PARTNER128):
        case CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_WARPSPEED128):
        default:
            break;
    }
    vicii.last_cpu_val = value;
    ram_store(addr, value);
}

void external_function_top_shared_store(uint16_t addr, uint8_t value)
{
    vicii.last_cpu_val = value;
    top_shared_store(addr, value);
}

/* basic hi replacement reads at the cartridge port */
uint8_t c128cartridge_basic_hi_read(uint16_t addr, uint8_t *value)
{
    int type = cartridge_get_id(0);
    uint8_t ret = 0;
    /* return 1 if the read was successful */
    switch(type) {
        case CARTRIDGE_LT_KERNAL:
            ret = c128ltkernal_basic_hi_read(addr, value);
            break;
        default:
            break;
    }
    return ret;
}

/* basic hi replacement store at the cartridge port */
uint8_t c128cartridge_basic_hi_store(uint16_t addr, uint8_t value)
{
    int type = cartridge_get_id(0);
    uint8_t ret = 0;
    /* return 1 if the write was successful */
    switch(type) {
        case CARTRIDGE_LT_KERNAL:
            ret = c128ltkernal_basic_hi_store(addr, value);
            break;
        default:
            break;
    }
    return ret;
}

/* kernal replacement reads at the cartridge port */
uint8_t c128cartridge_hi_read(uint16_t addr, uint8_t *value)
{
    int type = cart_getid_slot0();
    uint8_t ret = 0;
    /* return 1 if the read was successful */
    switch(type) {
        case CARTRIDGE_RAMLINK:
            ret = c128ramlink_hi_read(addr, value);
            break;
        default:
            break;
    }
    if (ret) {
        return ret;
    }
    type = cartridge_get_id(0);
    switch(type) {
        case CARTRIDGE_LT_KERNAL:
            ret = c128ltkernal_hi_read(addr, value);
            break;
        default:
            break;
    }
    return ret;
}

/* kernal replacement store at the cartridge port */
uint8_t c128cartridge_hi_store(uint16_t addr, uint8_t value)
{
    int type = cartridge_get_id(0);
    uint8_t ret = 0;
    /* return 1 if the write was successful */
    switch(type) {
        case CARTRIDGE_LT_KERNAL:
            ret = c128ltkernal_hi_store(addr, value);
            break;
        default:
            break;
    }
    return ret;
}

/* memory replacement reads at the cartridge port */
uint8_t c128cartridge_ram_read(uint16_t addr, uint8_t *value)
{
    int type = cartridge_get_id(0);
    uint8_t ret = 0;
    /* return 1 if the read was successful */
    switch(type) {
        case CARTRIDGE_LT_KERNAL:
            ret = c128ltkernal_ram_read(addr, value);
            break;
        default:
            break;
    }
    return ret;
}

/* kernal replacement store at the cartridge port */
uint8_t c128cartridge_ram_store(uint16_t addr, uint8_t value)
{
    int type = cartridge_get_id(0);
    uint8_t ret = 0;
    /* return 1 if the write was successful */
    switch(type) {
        case CARTRIDGE_LT_KERNAL:
            ret = c128ltkernal_ram_store(addr, value);
            break;
        default:
            break;
    }
    return ret;
}

/* mmu translation: return 0 if no translation applied, leave it for the tables */
int c128cartridge_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit, int mem_config)
{
    int type = cart_getid_slot0();
#if 0
    /* disable all the mmu translation stuff for testing */
    return 0;
#endif
    switch(type) {
        case CARTRIDGE_RAMLINK:
            if (c128ramlink_mmu_translate(addr, base, start, limit, mem_config)) {
                return 1;
            }
            break;
        default:
            break;
    }
    type = cartridge_get_id(0);
    switch(type) {
        case CARTRIDGE_LT_KERNAL:
            return c128ltkernal_mmu_translate(addr, base, start, limit, mem_config);
            break;
        default:
            break;
    }
    return 0;
}

/* notify cartridge of mode change */
void c128cartridge_switch_mode(int mode)
{
    int type = cart_getid_slot0();
    switch(type) {
        case CARTRIDGE_RAMLINK:
            c128ramlink_switch_mode(mode);
            break;
        default:
            break;
    }

    type = cartridge_get_id(0);
    switch(type) {
        case CARTRIDGE_LT_KERNAL:
            c128ltkernal_switch_mode(mode);
            break;
        case CARTRIDGE_MMC_REPLAY:
            mmcreplay_c128_switch_mode(mode);
            break;
        default:
            break;
    }

    return;
}
