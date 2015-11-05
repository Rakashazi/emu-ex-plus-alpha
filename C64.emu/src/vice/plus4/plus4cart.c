/*
 * plus4cart.c -- Plus4 cartridge handling.
 *
 * Written by
 *  Tibor Biczo <crown@axelero.hu>
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

#include <string.h>

#include "cartridge.h"
#include "cmdline.h"
#include "plus4cart.h"
#include "plus4mem.h"
#include "log.h"
#include "util.h"
#include "machine.h"
#include "monitor.h"
#include "resources.h"
#include "sysfile.h"
#include "translate.h"

/* #define DEBUGCART */

#ifdef DEBUGCART
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

/* global options for the cart system */
static int plus4cartridge_reset; /* (resource) hardreset system after cart was attached/detached */

/* ---------------------------------------------------------------------*/

static int cart_attach_cmdline(const char *param, void *extra_param)
{
    int type = vice_ptr_to_int(extra_param);

    /* NULL param is used for +cart */
    if (!param) {
        cartridge_detach_image(-1);
        return 0;
    }
    return cartridge_attach_image(type, param);
}

static const cmdline_option_t cmdline_options[] =
{
    /* hardreset on cartridge change */
    { "-cartreset", SET_RESOURCE, 0,
      NULL, NULL, "CartridgeReset", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_CART_ATTACH_DETACH_RESET,
      NULL, NULL },
    { "+cartreset", SET_RESOURCE, 0,
      NULL, NULL, "CartridgeReset", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_CART_ATTACH_DETACH_NO_RESET,
      NULL, NULL },
    /* smart attach */
    { "-cart", CALL_FUNCTION, 1,
      cart_attach_cmdline, (void*)CARTRIDGE_PLUS4_DETECT, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SMART_ATTACH_CART,
      NULL, NULL },
    /* no cartridge */
    { "+cart", CALL_FUNCTION, 0,
      cart_attach_cmdline, NULL, NULL, NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_CART,
      NULL, NULL },
    { NULL }
};

int cartridge_cmdline_options_init(void)
{
    mon_cart_cmd.cartridge_attach_image = cartridge_attach_image;
    mon_cart_cmd.cartridge_detach_image = cartridge_detach_image;
#if 0
    mon_cart_cmd.cartridge_trigger_freeze = cartridge_trigger_freeze;
    mon_cart_cmd.cartridge_trigger_freeze_nmi_only = cartridge_trigger_freeze_nmi_only;
    mon_cart_cmd.export_dump = plus4export_dump;
#endif
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

static int set_cartridge_reset(int value, void *param)
{
    int val = value ? 1 : 0;

/*    DBG(("plus4cartridge_reset: %d", val)); */
    if (plus4cartridge_reset != val) {
        DBG(("plus4cartridge_reset changed: %d", val));
        plus4cartridge_reset = val; /* resource value modified */
    }
    return 0;
}

static const resource_int_t resources_int[] = {
    { "CartridgeReset", 1, RES_EVENT_NO, NULL,
      &plus4cartridge_reset, set_cartridge_reset, NULL },
    { NULL }
};

int cartridge_resources_init(void)
{
    /* first the general int resource, so we get the "Cartridge Reset" one */
    if (resources_register_int(resources_int) < 0) {
        return -1;
    }
    return 0;
}

void cartridge_resources_shutdown(void)
{
}

/* ---------------------------------------------------------------------*/

void cart_power_off(void)
{
    if (plus4cartridge_reset) {
        /* "Turn off machine before removing cartridge" */
        machine_trigger_reset(MACHINE_RESET_MODE_HARD);
    }
}
/* ---------------------------------------------------------------------*/

extern int plus4_rom_loaded;

int plus4cart_load_func_lo(const char *rom_name)
{
    if (!plus4_rom_loaded) {
        return 0;
    }

    /* Load 3plus1 low ROM.  */
    if (*rom_name != 0) {
        if (sysfile_load(rom_name, extromlo1, PLUS4_CART16K_SIZE, PLUS4_CART16K_SIZE) < 0) {
            log_error(LOG_ERR,
                      "Couldn't load 3plus1 low ROM `%s'.",
                      rom_name);
            return -1;
        }
    } else {
        memset(extromlo1, 0, PLUS4_CART16K_SIZE);
    }
    return 0;
}

int plus4cart_load_func_hi(const char *rom_name)
{
    if (!plus4_rom_loaded) {
        return 0;
    }

    /* Load 3plus1 high ROM.  */
    if (*rom_name != 0) {
        if (sysfile_load(rom_name, extromhi1, PLUS4_CART16K_SIZE, PLUS4_CART16K_SIZE) < 0) {
            log_error(LOG_ERR,
                      "Couldn't load 3plus1 high ROM `%s'.",
                      rom_name);
            return -1;
        }
    } else {
        memset(extromhi1, 0, PLUS4_CART16K_SIZE);
    }
    return 0;
}

/*
    FIXME: make these 4 non public and replace by cartridge_attach_image() in UIs (win32, amigaos)
*/
int plus4cart_load_c1lo(const char *rom_name)
{
    if (!plus4_rom_loaded) {
        return 0;
    }

    /* Load c1 low ROM.  */
    if (*rom_name != 0) {
        if (sysfile_load(rom_name, extromlo2, PLUS4_CART16K_SIZE, PLUS4_CART16K_SIZE) < 0) {
            log_error(LOG_ERR,
                      "Couldn't load cartridge 1 low ROM `%s'.",
                      rom_name);
            return -1;
        }
    } else {
        DBG(("clearing c1lo"));
        memset(extromlo2, 0, PLUS4_CART16K_SIZE);
    }
    return 0;
}

int plus4cart_load_c1hi(const char *rom_name)
{
    if (!plus4_rom_loaded) {
        return 0;
    }

    /* Load c1 high ROM.  */
    if (*rom_name != 0) {
        if (sysfile_load(rom_name, extromhi2, PLUS4_CART16K_SIZE, PLUS4_CART16K_SIZE) < 0) {
            log_error(LOG_ERR,
                      "Couldn't load cartridge 1 high ROM `%s'.",
                      rom_name);
            return -1;
        }
    } else {
        DBG(("clearing c1hi"));
        memset(extromhi2, 0, PLUS4_CART16K_SIZE);
    }
    return 0;
}

int plus4cart_load_c2lo(const char *rom_name)
{
    if (!plus4_rom_loaded) {
        return 0;
    }

    /* Load c2 low ROM.  */
    if (*rom_name != 0) {
        if (sysfile_load(rom_name, extromlo3, PLUS4_CART16K_SIZE, PLUS4_CART16K_SIZE) < 0) {
            log_error(LOG_ERR,
                      "Couldn't load cartridge 2 low ROM `%s'.",
                      rom_name);
            return -1;
        }
    } else {
        memset(extromlo3, 0, PLUS4_CART16K_SIZE);
    }
    return 0;
}

int plus4cart_load_c2hi(const char *rom_name)
{
    if (!plus4_rom_loaded) {
        return 0;
    }

    /* Load c2 high ROM.  */
    if (*rom_name != 0) {
        if (sysfile_load(rom_name, extromhi3, PLUS4_CART16K_SIZE, PLUS4_CART16K_SIZE) < 0) {
            log_error(LOG_ERR,
                      "Couldn't load cartridge 2 high ROM `%s'.",
                      rom_name);
            return -1;
        }
    } else {
        memset(extromhi3, 0, PLUS4_CART16K_SIZE);
    }
    return 0;
}

/*
    FIXME: remove in UIs (replace by cartridge_detach_image(-1): Amiga, SDL, Win32
*/
void plus4cart_detach_cartridges(void)
{
    resources_set_string("c1loName", "");
    resources_set_string("c1hiName", "");
    resources_set_string("c2loName", "");
    resources_set_string("c2hiName", "");
    memset(extromlo2, 0, PLUS4_CART16K_SIZE);
    memset(extromhi2, 0, PLUS4_CART16K_SIZE);
    memset(extromlo3, 0, PLUS4_CART16K_SIZE);
    memset(extromhi3, 0, PLUS4_CART16K_SIZE);

    cart_power_off();
}

void cartridge_detach_image(int type)
{
    if (type < 0) {
        plus4cart_detach_cartridges();
    } else {
        switch (type) {
            case CARTRIDGE_PLUS4_16KB_C1LO:
                resources_set_string("c1loName", "");
                break;
            case CARTRIDGE_PLUS4_16KB_C1HI:
                resources_set_string("c1hiName", "");
                break;
            case CARTRIDGE_PLUS4_16KB_C2LO:
                resources_set_string("c2loName", "");
                break;
            case CARTRIDGE_PLUS4_16KB_C2HI:
                resources_set_string("c2hiName", "");
                break;
        }
        cart_power_off();
    }
}

int cartridge_detect(const char *filename)
{
    int type = CARTRIDGE_NONE;
    FILE *fd;
    size_t len;
    unsigned char b[0x100];

    fd = fopen(filename, "rb");
    if (fd == NULL) {
        return CARTRIDGE_NONE;
    }
    len = util_file_length(fd);

    /* there are so few carts that this little thing actually works :) */
    if (len == 8192) {
        type = CARTRIDGE_PLUS4_16KB_C1LO;
    } else if (len == 16384) {
        type = CARTRIDGE_PLUS4_16KB_C1LO;
        fseek(fd, 10, SEEK_SET);
        if (fread(b, 1, 0x100, fd) < 0x100) {
            fclose(fd);
            return CARTRIDGE_NONE;
        }
        /* Octasoft Basic v7 */
        if (!strncmp("SYS1546: BASIC V7.0 ON KEY F2", (const char*)b, 29)) {
            type = CARTRIDGE_PLUS4_16KB_C2LO;
        }
    } else if (len == 32768) {
        type = CARTRIDGE_PLUS4_32KB_C1;
    } else if (len == 49152) {
        type = CARTRIDGE_PLUS4_NEWROM;
    }

    fclose (fd);

    DBG(("detected cartridge type: %04x", type));

    return type;
}

static int cart_load_generic(int type, const char *filename)
{
    FILE *fd;
    unsigned char *blocks[6] = {
        extromlo1, extromhi1, extromlo2, extromhi2, extromlo3, extromhi3
    };
    int i;

    fd = fopen(filename, "rb");
    if (fd == NULL) {
        return -1;
    }
    for (i = 0; i < 6; i++) {
        if (type & 1) {
            memset(blocks[i], 0, PLUS4_CART16K_SIZE);
            log_debug("loading block %d", i);
            if (fread(blocks[i], 1, PLUS4_CART16K_SIZE, fd) < PLUS4_CART16K_SIZE) {
                break;
            }
        }
        type >>= 1;
    }
    fclose (fd);
    return 0;
}

int cartridge_attach_image(int type, const char *filename)
{
    if (type == CARTRIDGE_PLUS4_DETECT) {
        type = cartridge_detect(filename);
    }

    cart_power_off();

    switch (type) {
#if 0
        case CARTRIDGE_PLUS4_16KB_C1LO:
            return plus4cart_load_c1lo(filename);
        case CARTRIDGE_PLUS4_16KB_C1HI:
            return plus4cart_load_c1hi(filename);
        case CARTRIDGE_PLUS4_16KB_C2LO:
            return plus4cart_load_c2lo(filename);
        case CARTRIDGE_PLUS4_16KB_C2HI:
            return plus4cart_load_c2hi(filename);
#endif
        default:
            if ((type & 0xff00) == CARTRIDGE_PLUS4_DETECT) {
                return cart_load_generic(type, filename);
            } else {
                log_error(LOG_DEFAULT, "cartridge_attach_image: unsupported type (%04x)", type);
            }
            break;
    }

    return -1;
}
