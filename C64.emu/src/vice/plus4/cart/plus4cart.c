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

#define DEBUGCART
/* #define DEBUGCARTRW */

#include "vice.h"

#include <string.h>

#include "archdep.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "crt.h"
#include "plus4cart.h"
#include "plus4mem.h"
#include "lib.h"
#include "log.h"
#include "util.h"
#include "machine.h"
#include "monitor.h"
#include "resources.h"
#include "snapshot.h"
#include "sysfile.h"

#include "debugcart.h"
#include "jacint1mb.h"
#include "magiccart.h"
#include "multicart.h"
#include "plus4-generic.h"

#ifdef DEBUGCART
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

#ifdef DEBUGCARTRW
#define DBGRW(x)  log_debug x
#else
#define DBGRW(x)
#endif

/*
basic   internal                $8000-$bfff basic
kernal  internal                $c000-$ffff kernal
c0lo    internal                $8000-$bfff function rom low (only Plus4)
c0hi    internal                $c000-$ffff function rom high (only Plus4)
c1lo    exp. port               $8000-$bfff cartridge rom low
c1hi    exp. port               $c000-$ffff cartridge rom high
c2lo    exp. port (or internal) $8000-$bfff reserved / v364 speech software low
c2hi    exp. port (or internal) $c000-$ffff reserved / v364 speech software high

TED controls all banking. The cs0 and cs1 are active when the CPU is
accessing $8000-$bfff and $c000-$ffff respectively, but can be “overridden”
by writing anything to TED registers $3e and $3f. Writing anything to $FF3E
will page in the currently configured ROMs to the upper memory area
($8000..$FFFF), and writing anything to $FF3F will page in RAM to the same
area.

ROM Banking is also explained on page 73 of
http://www.zimmers.net/anonftp/pub/cbm/schematics/computers/plus4/264_Hardware_Spec.pdf
*/

/* global options for the cart system */
static int plus4cartridge_reset; /* (resource) hardreset system after cart was attached/detached */

/* defaults */
static char *cartridge_file = NULL; /* (resource) file name */
static int cartridge_type = CARTRIDGE_NONE; /* (resource) is == CARTRIDGE_CRT (0) if CRT file */

/* actually in use */
static char *cartfile = NULL; /* file name */
static int plus4cart_type = CARTRIDGE_NONE; /* is == CARTRIDGE_CRT (0) if CRT file */
static int crttype = CARTRIDGE_NONE; /* contains CRT ID if plus4cart_type == 0 */

static int mem_cartridge_type = CARTRIDGE_NONE;  /* Type of the cartridge attached. */

/* ---------------------------------------------------------------------*/

static cartridge_info_t cartlist[] = {
    /* standard cartridges with CRT ID = 0 */
/*
    { "Raw 16KiB C1LO",                       CARTRIDGE_PLUS4_GENERIC_C1LO,      CARTRIDGE_GROUP_GENERIC },
    { "Raw 16KiB C1HI",                       CARTRIDGE_PLUS4_GENERIC_C1HI,      CARTRIDGE_GROUP_GENERIC },
    { "Raw 16KiB C2LO",                       CARTRIDGE_PLUS4_GENERIC_C2LO,      CARTRIDGE_GROUP_GENERIC },
    { "Raw 16KiB C2HI",                       CARTRIDGE_PLUS4_GENERIC_C2HI,      CARTRIDGE_GROUP_GENERIC },
*/
    /* all cartridges with a CRT ID > 0, alphabetically sorted */
    { CARTRIDGE_PLUS4_NAME_JACINT1MB,         CARTRIDGE_PLUS4_JACINT1MB,         CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_PLUS4_NAME_MAGIC,             CARTRIDGE_PLUS4_MAGIC,             CARTRIDGE_GROUP_UTIL },
    { CARTRIDGE_PLUS4_NAME_MULTI,             CARTRIDGE_PLUS4_MULTI,             CARTRIDGE_GROUP_UTIL },

    { NULL, 0, 0 }
};

cartridge_info_t *cartridge_get_info_list(void)
{
    return &cartlist[0];
}

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
    { "-cartreset", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "CartridgeReset", (void *)1,
      NULL, "Reset machine if a cartridge is attached or detached" },
    { "+cartreset", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "CartridgeReset", (void *)0,
      NULL, "Do not reset machine if a cartridge is attached or detached" },
    /* smart attach */
    { "-cart", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cart_attach_cmdline, (void*)CARTRIDGE_PLUS4_DETECT, NULL, NULL,
      "<Name>", "Smart-attach cartridge image" },
    /* smart-insert CRT */
    { "-cartcrt", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cart_attach_cmdline, (void *)CARTRIDGE_CRT, NULL, NULL,
      "<Name>", "Attach CRT cartridge image" },
    /* seperate cartridge types */
    { "-cartjacint", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cart_attach_cmdline, (void*)CARTRIDGE_PLUS4_JACINT1MB, NULL, NULL,
      "<Name>", "Attach 1MiB " CARTRIDGE_PLUS4_NAME_JACINT1MB " image" },
    { "-cartmagic", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cart_attach_cmdline, (void*)CARTRIDGE_PLUS4_MAGIC, NULL, NULL,
      "<Name>", "Attach 128kiB/256kiB/512kiB/1MiB/2MiB " CARTRIDGE_PLUS4_NAME_MAGIC " image" },
    { "-cartmulti", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cart_attach_cmdline, (void*)CARTRIDGE_PLUS4_MULTI, NULL, NULL,
      "<Name>", "Attach 1MiB/2MiB " CARTRIDGE_PLUS4_NAME_MULTI " image" },
    /* no cartridge */
    { "+cart", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      cart_attach_cmdline, NULL, NULL, NULL,
      NULL, "Disable default cartridge" },
    CMDLINE_LIST_END
};

int cartridge_cmdline_options_init(void)
{
    mon_cart_cmd.cartridge_attach_image = cartridge_attach_image;
    mon_cart_cmd.cartridge_detach_image = cartridge_detach_image;
    mon_cart_cmd.cartridge_trigger_freeze = cartridge_trigger_freeze;
#if 0
    mon_cart_cmd.cartridge_trigger_freeze_nmi_only = cartridge_trigger_freeze_nmi_only;
    mon_cart_cmd.export_dump = plus4export_dump;
#endif
    if (generic_cmdline_options_init() < 0) {
        return -1;
    }

    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

/*
    we have 3 resources for the main cart that may be changed in arbitrary order:

    - cartridge type
    - cartridge file name
    - cartridge change reset behaviour

    the following functions try to deal with this in a hopefully sane way... however,
    do _NOT_ change the used resources from the (G)UI directly. (used the set_default
    function instead)
*/

static int try_cartridge_attach(int type, const char *filename)
{
    if (filename) {
        if (util_file_exists(filename)) {
#if 0
            if (crt_getid(filename) > 0) {
                cartridge_type = CARTRIDGE_CRT; /* resource value modified */
                return cartridge_attach_image(CARTRIDGE_CRT, filename);
            } else
#endif
            if ((type != CARTRIDGE_NONE) && (type != CARTRIDGE_CRT)) {
                cartridge_type = type; /* resource value modified */
                return cartridge_attach_image(type, filename);
            }
        } else {
            DBG(("cartridge_file does not exist: '%s'", filename));
        }
    }

    return 0;
}

static int set_cartridge_type(int val, void *param)
{
    switch (val) {
        case CARTRIDGE_NONE:
        /* case CARTRIDGE_CRT: */

        case CARTRIDGE_PLUS4_JACINT1MB:
        case CARTRIDGE_PLUS4_MAGIC:
        case CARTRIDGE_PLUS4_MULTI:
            break;
        default:
            return -1;
    }

    DBG(("set_cartridge_type: %d", val));
    if (cartridge_type != val) {
        DBG(("cartridge_type changed: %d", val));
        cartridge_type = val;
        return try_cartridge_attach(cartridge_type, cartridge_file);
    }

    return 0;
}

/*
*/
static int set_cartridge_file(const char *name, void *param)
{
    DBG(("set_cartridge_file: '%s'", name));
    if (cartridge_file == NULL) {
        util_string_set(&cartridge_file, ""); /* resource value modified */
    }

    if (!strcmp(cartridge_file, name)) {
        return 0;
    }

    if (name == NULL || !strlen(name)) {
        cartridge_detach_image(-1);
        return 0;
    }

    DBG(("cartridge_file changed: '%s'", name));

    if (util_file_exists(name)) {
        util_string_set(&cartridge_file, name); /* resource value modified */
        return try_cartridge_attach(cartridge_type, cartridge_file);
    } else {
        DBG(("cartridge_file does not exist: '%s'", name));
        cartridge_type = CARTRIDGE_NONE; /* resource value modified */
        util_string_set(&cartridge_file, ""); /* resource value modified */
    }

    return 0;
}

static int set_cartridge_reset(int value, void *param)
{
    int val = value ? 1 : 0;

    DBG(("set_cartridge_reset: %d", val));
    if (plus4cartridge_reset != val) {
        DBG(("plus4cartridge_reset changed: %d", val));
        plus4cartridge_reset = val; /* resource value modified */
    }
    return 0;
}

static const resource_int_t resources_int[] = {
    { "CartridgeReset", 1, RES_EVENT_NO, NULL,
      &plus4cartridge_reset, set_cartridge_reset, NULL },
    { "CartridgeType", CARTRIDGE_NONE,
      RES_EVENT_STRICT, (resource_value_t)CARTRIDGE_NONE,
      &cartridge_type, set_cartridge_type, NULL },
    RESOURCE_INT_LIST_END
};

static const resource_string_t resources_string[] = {
    { "CartridgeFile", "", RES_EVENT_NO, NULL,
      &cartridge_file, set_cartridge_file, NULL },
    RESOURCE_STRING_LIST_END
};

int cartridge_resources_init(void)
{
    /* first the general int resource, so we get the "Cartridge Reset" one */
    if (resources_register_int(resources_int) < 0) {
        return -1;
    }

    if (generic_resources_init() < 0) {
        return -1;
    }

    return resources_register_string(resources_string);
}

void cartridge_resources_shutdown(void)
{
    generic_resources_shutdown();

    lib_free(cartridge_file);
    lib_free(cartfile);
}

/* ---------------------------------------------------------------------*/
/* expansion port memory read/write hooks */
uint8_t plus4cart_c1lo_read(uint16_t addr)
{
    DBGRW(("plus4cart_c1lo_read mem_cartridge_type: %04x addr: %04x", (unsigned)mem_cartridge_type, addr));
    if (CARTRIDGE_PLUS4_IS_GENERIC(mem_cartridge_type)) {
        if ((mem_cartridge_type & CARTRIDGE_PLUS4_GENERIC_TYPE_MASK) & CARTRIDGE_PLUS4_GENERIC_C1LO) {
            return generic_c1lo_read(addr);
        }
    }

    switch (mem_cartridge_type) {
        case CARTRIDGE_PLUS4_GENERIC:
            return generic_c1lo_read(addr);
        case CARTRIDGE_PLUS4_JACINT1MB:
            return jacint1mb_c1lo_read(addr);
        case CARTRIDGE_PLUS4_MAGIC:
            return magiccart_c1lo_read(addr);
        case CARTRIDGE_PLUS4_MULTI:
            return multicart_c1lo_read(addr);
    }
    /* FIXME: when no cartridge is attached, we will probably read open i/o */
    return 0xff;
}

uint8_t plus4cart_c1hi_read(uint16_t addr)
{
    DBGRW(("plus4cart_c1hi_read mem_cartridge_type: %04x addr: %04x", (unsigned)mem_cartridge_type, addr));
    if (CARTRIDGE_PLUS4_IS_GENERIC(mem_cartridge_type)) {
        if ((mem_cartridge_type & CARTRIDGE_PLUS4_GENERIC_TYPE_MASK) & CARTRIDGE_PLUS4_GENERIC_C1HI) {
            return generic_c1hi_read(addr);
        }
    }

    switch (mem_cartridge_type) {
        case CARTRIDGE_PLUS4_GENERIC:
            return generic_c1hi_read(addr);
        case CARTRIDGE_PLUS4_MULTI:
            return multicart_c1hi_read(addr);
    }
    /* FIXME: when no cartridge is attached, we will probably read open i/o */
    return 0xff;
}

void cartridge_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit)
{
    *base = NULL;
    *start = 0;
    *limit = 0;
}

/* ---------------------------------------------------------------------*/
/*
    called by plus4.c:machine_specific_reset (calls XYZ_reset)
*/
void cartridge_reset(void)
{
    /* cart_unset_alarms(); */
    /* cart_reset_memptr(); */
    switch (mem_cartridge_type) {
        case CARTRIDGE_PLUS4_JACINT1MB:
            return jacint1mb_reset();
        case CARTRIDGE_PLUS4_MAGIC:
            return magiccart_reset();
        case CARTRIDGE_PLUS4_MULTI:
            return multicart_reset();
    }
}

/*
    called by plus4.c:machine_specific_powerup (calls XYZ_reset)
*/
void cartridge_powerup(void)
{
}

static void cart_power_off(void)
{
    if (plus4cartridge_reset) {
        /* "Turn off machine before removing cartridge" */
        machine_trigger_reset(MACHINE_RESET_MODE_HARD);
    }
}

/*
    Attach cartridge from snapshot

    Sets static variables related to the "Main Slot".
*/
static void cart_attach_from_snapshot(int type)
{
    plus4cart_type = type;
}


static void plus4cart_detach_cartridges(void)
{
    DBG(("plus4cart_detach_cartridges"));
#if 1
    resources_set_string("c1loName", "");
    resources_set_string("c1hiName", "");
    resources_set_string("c2loName", "");
    resources_set_string("c2hiName", "");
#endif
#if 1
    generic_detach(CARTRIDGE_PLUS4_GENERIC_C1 | CARTRIDGE_PLUS4_GENERIC_C2);
    jacint1mb_detach();
    magiccart_detach();
    multicart_detach();
#endif
    mem_cartridge_type = CARTRIDGE_NONE;

    cart_power_off();
}

void cartridge_detach_image(int type)
{
    DBG(("cartridge_detach_image type %04x", (unsigned)type));
    if (type < 0) {
        plus4cart_detach_cartridges();
    } else {
        if (CARTRIDGE_PLUS4_IS_GENERIC(type)) {
            generic_detach(type);
        } else {
            switch (type) {
                case CARTRIDGE_PLUS4_JACINT1MB:
                    jacint1mb_detach();
                    break;
                case CARTRIDGE_PLUS4_MAGIC:
                    magiccart_detach();
                    break;
                case CARTRIDGE_PLUS4_MULTI:
                    multicart_detach();
                    break;
            }
        }
        cart_power_off();
    }
}

/*
    set currently active cart as default
*/
void cartridge_set_default(void)
{
    int type = CARTRIDGE_NONE;

    if (cartfile != NULL) {
        if (util_file_exists(cartfile)) {
#if 0
            if (crt_getid(cartfile) > 0) {
                type = CARTRIDGE_CRT;
            } else 
#endif
            {
                type = plus4cart_type;
            }
        } else {
            DBG(("cartridge_set_default: file does not exist: '%s'", cartfile ? cartfile : "NULL"));
        }
    } else {
        DBG(("cartridge_set_default: no filename\n"));
    }
    DBG(("cartridge_set_default: id %d '%s'", type, cartfile ? cartfile : "NULL"));

    if (type == CARTRIDGE_NONE) {
        util_string_set(&cartridge_file, ""); /* resource value modified */
    } else {
        util_string_set(&cartridge_file, cartfile); /* resource value modified */
    }
    cartridge_type = type; /* resource value modified */
}

/** \brief  Wipe "default cartidge"
 */
void cartridge_unset_default(void)
{
    util_string_set(&cartridge_file, "");
    cartridge_type = CARTRIDGE_NONE;
}

/* FIXME: this is kinda broken and wrong, remove */
int cartridge_detect(const char *filename)
{
    int type = CARTRIDGE_NONE;
    FILE *fd;
    size_t len;

    fd = fopen(filename, "rb");
    if (fd == NULL) {
        return CARTRIDGE_NONE;
    }
    len = util_file_length(fd);

    if (len == 8192) {
        type = CARTRIDGE_PLUS4_GENERIC_C1LO;
    } else if (len == 16384) {
        type = CARTRIDGE_PLUS4_GENERIC_C1LO;
    } else if (len == 32768) {
        type = CARTRIDGE_PLUS4_GENERIC_C1;
    }

    fclose (fd);

    DBG(("detected cartridge type: %04x", (unsigned int)type));

    return type;
}


/* XXX: This is public for the C64, with the prototype contained in
 *      src/c64/cart/c64cartsystem.h. There's no such prototype for this, so
 *      I made the function static for now. --compyx
 */
static int cart_bin_attach(int type, const char *filename, uint8_t *rawcart)
{
    if (CARTRIDGE_PLUS4_IS_GENERIC(type)) {
        return generic_bin_attach(type, filename, rawcart);
    }

    switch (type) {
        case CARTRIDGE_PLUS4_JACINT1MB:
            return jacint1mb_bin_attach(filename, rawcart);
        case CARTRIDGE_PLUS4_MAGIC:
            return magiccart_bin_attach(filename, rawcart);
        case CARTRIDGE_PLUS4_MULTI:
            return multicart_bin_attach(filename, rawcart);
    }
    log_error(LOG_DEFAULT,
              "cartridge_bin_attach: unsupported type (%04x)", (unsigned int)type);
    return -1;
}

/*
    called by cartridge_attach_image after cart_crt/bin_attach
    XYZ_config_setup should copy the raw cart image into the
    individual implementations array.
*/
static void cart_attach(int type, uint8_t *rawcart)
{
    /* cart_detach_conflicting(type); */
    if (CARTRIDGE_PLUS4_IS_GENERIC(type)) {
        generic_config_setup(rawcart);
    } else {
        switch (type) {
            case CARTRIDGE_PLUS4_JACINT1MB:
                jacint1mb_config_setup(rawcart);
                break;
            case CARTRIDGE_PLUS4_MAGIC:
                magiccart_config_setup(rawcart);
                break;
            case CARTRIDGE_PLUS4_MULTI:
                multicart_config_setup(rawcart);
                break;
        }
    }
}

/*
    returns -1 on error, else a positive CRT ID

    FIXME: to simplify this function a little bit, all subfunctions should
           also return the respective CRT ID on success
*/
static int crt_attach(const char *filename, uint8_t *rawcart)
{
    crt_header_t header;
    int rc, new_crttype;
    FILE *fd;

    DBG(("crt_attach: %s", filename));

    fd = crt_open(filename, &header);

    if (fd == NULL) {
        return -1;
    }

    new_crttype = header.type;
    if (new_crttype & 0x8000) {
        /* handle our negative test IDs */
        new_crttype -= 0x10000;
    }

    DBG(("crt_attach ID: %d", new_crttype));

/*  cart should always be detached. there is no reason for doing fancy checks
    here, and it will cause problems incase a cart MUST be detached before
    attaching another, or even itself. (eg for initialization reasons)

    most obvious reason: attaching a different ROM (software) for the same
    cartridge (hardware) */

    cartridge_detach_image(new_crttype);

    switch (new_crttype) {
        case CARTRIDGE_CRT:
            rc = generic_crt_attach(fd, rawcart);
            if (rc != CARTRIDGE_NONE) {
                new_crttype = rc;
            }
            break;
        case CARTRIDGE_PLUS4_JACINT1MB:
            rc = jacint1mb_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_PLUS4_MAGIC:
            rc = magiccart_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_PLUS4_MULTI:
            rc = multicart_crt_attach(fd, rawcart);
            break;
        default:
            archdep_startup_log_error("unknown CRT ID: %d", new_crttype);
            rc = -1;
            break;
    }

    fclose(fd);

    if (rc == -1) {
        DBG(("crt_attach error (%d)", rc));
        return -1;
    }
    DBG(("crt_attach return ID: %d", new_crttype));
    return new_crttype;
}

/*
    attach cartridge image

    type == -1  NONE
    type ==  0  CRT format

    returns -1 on error, 0 on success
*/

int cartridge_attach_image(int type, const char *filename)
{
    unsigned char *rawcartdata;  /* raw cartridge data while loading/attaching */
    char *abs_filename;
    int cartid = type; /* FIXME: this will get the crtid */
    int carttype = CARTRIDGE_NONE;
    /* FIXME: we should convert the intermediate type to generic type 0 */

    if (filename == NULL) {
        return -1;
    }

    /* Attaching no cartridge always works. */
    if (type == CARTRIDGE_NONE || *filename == '\0') {
        return 0;
    }

    if (archdep_path_is_relative(filename)) {
        archdep_expand_path(&abs_filename, filename);
    } else {
        abs_filename = lib_strdup(filename);
    }

    DBG(("CART: detach slot main ID: %d", mem_cartridge_type));
    cartridge_detach_image(mem_cartridge_type);

    if (type == CARTRIDGE_PLUS4_DETECT) {
        type = cartridge_detect(filename);
    }

    if (type == CARTRIDGE_CRT) {
        carttype = crt_getid(abs_filename);
        if (carttype == -1) {
            log_message(LOG_DEFAULT, "CART: '%s' is not a valid CRT file.", abs_filename);
            lib_free(abs_filename);
            return -1;
        }
    } else {
        carttype = type;
    }
    DBG(("CART: cartridge_attach_image type: %d ID: %d", type, cartid));

    /* allocate temporary array */
    rawcartdata = lib_malloc(PLUS4CART_IMAGE_LIMIT);

    if (type == CARTRIDGE_CRT) {
        DBG(("CART: attach CRT ID: %d '%s'", carttype, filename));
        cartid = crt_attach(abs_filename, rawcartdata);
        if (cartid == CARTRIDGE_NONE) {
            goto exiterror;
        }
        if (type < 0) {
            DBG(("CART: attach generic CRT ID: %d", type));
        }
    } else {
       DBG(("CART: attach BIN ID: %d '%s'", carttype, filename));
        cartid = carttype;
        if (cart_bin_attach(cartid, abs_filename, rawcartdata) < 0) {
            goto exiterror;
        }
    }

    mem_cartridge_type = cartid;

    cart_attach(cartid, rawcartdata);
    cart_power_off();

    DBG(("CART: set ID: %d type: %d", carttype, type));
    plus4cart_type = type;
    if (type == CARTRIDGE_CRT) {
        crttype = carttype;
    }
    util_string_set(&cartfile, abs_filename);

    DBG(("CART: cartridge_attach_image type: %d ID: %d done.", type, cartid));
    lib_free(rawcartdata);
    log_message(LOG_DEFAULT, "CART: attached '%s' as ID %d.", filename, cartid);
    return 0;

exiterror:
    DBG(("CART: error\n"));
    lib_free(rawcartdata);
    log_message(LOG_DEFAULT, "CART: could not attach '%s'.", filename);
    return -1;
}

/* FIXME: todo */
void cartridge_trigger_freeze(void)
{
    int delay = lib_unsigned_rand(1, (unsigned int)machine_get_cycles_per_frame());
#if 0
    cart_freeze_alarm_time = maincpu_clk + delay;
    alarm_set(cartridge_freeze_alarm, cart_freeze_alarm_time);
#endif
    DBG(("cartridge_trigger_freeze delay %d cycles", delay));
}

/* ------------------------------------------------------------------------- */

/*
    Snapshot reading and writing
*/

/* FIXME: due to the snapshots being generally broken as a while, none of this
          could be tested */

#define PLUS4CART_DUMP_MAX_CARTS  1

#define PLUS4CART_DUMP_VER_MAJOR   0
#define PLUS4CART_DUMP_VER_MINOR   1
#define SNAP_MODULE_NAME  "PLUS4CART"

int cartridge_snapshot_write_modules(struct snapshot_s *s)
{
    snapshot_module_t *m;

    uint8_t i;
    uint8_t number_of_carts = 0;
    int cart_ids[PLUS4CART_DUMP_MAX_CARTS];

    memset(cart_ids, 0, sizeof(cart_ids));

    if (mem_cartridge_type != CARTRIDGE_NONE) {
        cart_ids[number_of_carts++] = mem_cartridge_type;
    }

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               PLUS4CART_DUMP_VER_MAJOR, PLUS4CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, number_of_carts) < 0) {
        goto fail;
    }

    /* Not much to do if no carts present */
    if (number_of_carts == 0) {
        return snapshot_module_close(m);
    }

    /* Save "global" cartridge things */
    if (0
        || SMW_DW(m, (uint32_t)mem_cartridge_type) < 0
        /* || SMW_DW(m, (uint32_t)cart_freeze_alarm_time) < 0 */
        /* || SMW_DW(m, (uint32_t)cart_nmi_alarm_time) < 0 */
        ) {
        goto fail;
    }

    /* Save cart IDs */
    for (i = 0; i < number_of_carts; i++) {
        if (SMW_DW(m, (uint32_t)cart_ids[i]) < 0) {
            goto fail;
        }
    }

    /* Main module done */
    snapshot_module_close(m);
    m = NULL;

    /* Save individual cart data */
    for (i = 0; i < number_of_carts; i++) {
        switch (cart_ids[i]) {

            case CARTRIDGE_PLUS4_GENERIC:
                if (generic_snapshot_write_module(s) < 0) {
                    return -1;
                }
                break;

            case CARTRIDGE_PLUS4_JACINT1MB:
                if (jacint1mb_snapshot_write_module(s) < 0) {
                    return -1;
                }
                break;
            case CARTRIDGE_PLUS4_MAGIC:
                if (magiccart_snapshot_write_module(s) < 0) {
                    return -1;
                }
                break;
            case CARTRIDGE_PLUS4_MULTI:
                if (multicart_snapshot_write_module(s) < 0) {
                    return -1;
                }
                break;

            default:
                /* If the cart cannot be saved, we obviously can't load it either.
                   Returning an error at this point is better than failing at later. */
                DBG(("CART snapshot save: cart %i handler missing", cart_ids[i]));
                return -1;
        }
    }

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}

int cartridge_snapshot_read_modules(struct snapshot_s *s)
{
    snapshot_module_t *m;
    uint8_t vmajor, vminor;

    uint8_t i;
    uint8_t number_of_carts;
    int cart_ids[PLUS4CART_DUMP_MAX_CARTS];
    int local_cartridge_reset;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    if ((vmajor != PLUS4CART_DUMP_VER_MAJOR) || (vminor != PLUS4CART_DUMP_VER_MINOR)) {
        goto fail;
    }

    /* disable cartridge reset while detaching old cart */
    resources_get_int("CartridgeReset", &local_cartridge_reset);
    resources_set_int("CartridgeReset", 0);
    cartridge_detach_image(-1);
    resources_set_int("CartridgeReset", local_cartridge_reset);

    if (SMR_B(m, &number_of_carts) < 0) {
        goto fail;
    }

    /* Not much to do if no carts in snapshot */
    if (number_of_carts == 0) {
        return snapshot_module_close(m);
    }

    if (number_of_carts > PLUS4CART_DUMP_MAX_CARTS) {
        DBG(("CART snapshot read: carts %i > max %i", number_of_carts, PLUS4CART_DUMP_MAX_CARTS));
        goto fail;
    }

    /* Read "global" cartridge things */
    if (0
        || SMR_DW_INT(m, &mem_cartridge_type) < 0
        /* || SMR_DW(m, &cart_freeze_alarm_time) < 0 */
        /* || SMR_DW(m, &cart_nmi_alarm_time) < 0 */
        ) {
        goto fail;
    }

    /* cart ID */
    for (i = 0; i < number_of_carts; i++) {
        if (SMR_DW_INT(m, &cart_ids[i]) < 0) {
            goto fail;
        }
    }

    /* Main module done */
    snapshot_module_close(m);
    m = NULL;

    /* Read individual cart data */
    for (i = 0; i < number_of_carts; i++) {
        switch (cart_ids[i]) {

            case CARTRIDGE_PLUS4_GENERIC:
                if (generic_snapshot_read_module(s) < 0) {
                    goto fail2;
                }
                break;

            case CARTRIDGE_PLUS4_JACINT1MB:
                if (jacint1mb_snapshot_read_module(s) < 0) {
                    goto fail2;
                }
                break;
            case CARTRIDGE_PLUS4_MAGIC:
                if (magiccart_snapshot_read_module(s) < 0) {
                    goto fail2;
                }
                break;
            case CARTRIDGE_PLUS4_MULTI:
                if (multicart_snapshot_read_module(s) < 0) {
                    goto fail2;
                }
                break;

            default:
                DBG(("CART snapshot read: cart %i handler missing", cart_ids[i]));
                goto fail2;
        }
    }

    cart_attach_from_snapshot(cart_ids[i]);

    /* set up config */
    /* machine_update_memory_ptrs(); */

    /* restore alarms */
    /* cart_undump_alarms(); */

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
fail2:
    mem_cartridge_type = CARTRIDGE_NONE; /* Failed to load cartridge! */
    return -1;
}
