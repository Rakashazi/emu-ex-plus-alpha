/*
 * vic20cart.c - VIC20 Cartridge emulation.
 *
 * Written by
 *  Daniel Kahlin <daniel@kahlin.net>
 *
 * Based on code by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifdef AMIGA_AROS
#define __AROS_OFF_T_DECLARED
#define __AROS_PID_T_DECLARED
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "c64acia.h"
#include "cartridge.h"
#include "cmdline.h"
#include "digimax.h"
#include "ds12c887rtc.h"
#include "finalexpansion.h"
#include "georam.h"
#include "lib.h"
#include "log.h"
#include "mem.h"
#include "vic-fp.h"
#include "megacart.h"
#include "monitor.h"
#include "resources.h"
#include "sfx_soundexpander.h"
#include "sfx_soundsampler.h"
#include "snapshot.h"
#ifdef HAVE_TFE
#define CARTRIDGE_INCLUDE_PRIVATE_API
#define CARTRIDGE_INCLUDE_PUBLIC_API
#include "tfe.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API
#undef CARTRIDGE_INCLUDE_PUBLIC_API
#endif
#include "translate.h"
#include "util.h"
#include "vic20cart.h"
#include "vic20cartmem.h"
#include "vic20mem.h"
#include "vic20-generic.h"
#include "zfile.h"

/* flag for indicating if cartridge is from snapshot, used for
   disabling "set as default" and write back */
int cartridge_is_from_snapshot = 0;

/* actual resources */
static char *cartridge_file = NULL;
static int cartridge_type;
static int vic20cartridge_reset;

/* local shadow of some resources (e.g not yet set as default) */
static int vic20cart_type = CARTRIDGE_NONE;
static char *cartfile = NULL;

static int cartres_flags = 0;

static int cartridge_attach_from_resource(int type, const char *filename);

void reset_try_flags(void)
{
    cartres_flags = 0;
}

int try_cartridge_attach(int c)
{
    cartres_flags ^= c;
    if (cartres_flags) {
        return 0;
    }

    return cartridge_attach_from_resource(vic20cart_type, cartfile);
}

static int set_cartridge_type(int val, void *param)
{
    switch (val) {
        case CARTRIDGE_NONE:
        case CARTRIDGE_VIC20_GENERIC:
        case CARTRIDGE_VIC20_MEGACART:
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
        case CARTRIDGE_VIC20_FP:
        case CARTRIDGE_VIC20_IEEE488:
        case CARTRIDGE_VIC20_SIDCART:
        case CARTRIDGE_VIC20_DETECT:
        case CARTRIDGE_VIC20_4KB_2000:
        case CARTRIDGE_VIC20_8KB_2000:
        case CARTRIDGE_VIC20_4KB_6000:
        case CARTRIDGE_VIC20_8KB_6000:
        case CARTRIDGE_VIC20_4KB_A000:
        case CARTRIDGE_VIC20_8KB_A000:
        case CARTRIDGE_VIC20_4KB_B000:
        case CARTRIDGE_VIC20_8KB_4000:
        case CARTRIDGE_VIC20_4KB_4000:
        case CARTRIDGE_VIC20_16KB_2000:
        case CARTRIDGE_VIC20_16KB_4000:
        case CARTRIDGE_VIC20_16KB_6000:
            break;
        default:
            return -1;
    }

    cartridge_type = val;
    vic20cart_type = cartridge_type;

    return try_cartridge_attach(TRY_RESOURCE_CARTTYPE);
}

static int set_cartridge_file(const char *name, void *param)
{
    util_string_set(&cartridge_file, name);
    util_string_set(&cartfile, name);

    return try_cartridge_attach(TRY_RESOURCE_CARTNAME);
}

static int set_cartridge_reset(int val, void *param)
{
    vic20cartridge_reset = val ? 1 : 0;

    return try_cartridge_attach(TRY_RESOURCE_CARTRESET);
}

static const resource_string_t resources_string[] = {
    { "CartridgeFile", "", RES_EVENT_NO, NULL,
      &cartridge_file, set_cartridge_file, NULL },
    { NULL }
};
static const resource_int_t resources_int[] = {
    { "CartridgeType", CARTRIDGE_NONE,
      RES_EVENT_STRICT, (resource_value_t)CARTRIDGE_NONE,
      &cartridge_type, set_cartridge_type, NULL },
    { "CartridgeReset", 1, RES_EVENT_NO, NULL,
      &vic20cartridge_reset, set_cartridge_reset, NULL },
    { NULL }
};

int cartridge_resources_init(void)
{
    if (resources_register_int(resources_int) < 0
        || resources_register_string(resources_string) < 0
        || generic_resources_init() < 0
        || finalexpansion_resources_init() < 0
        || vic_fp_resources_init() < 0
        || megacart_resources_init() < 0
#ifdef HAVE_TFE
        || tfe_resources_init() < 0
#endif
        || aciacart_resources_init() < 0
        || digimax_resources_init() < 0
        || ds12c887rtc_resources_init() < 0
        || sfx_soundexpander_resources_init() < 0
        || sfx_soundsampler_resources_init() < 0
        || georam_resources_init() < 0) {
        return -1;
    }
    return 0;
}

void cartridge_resources_shutdown(void)
{
    megacart_resources_shutdown();
    finalexpansion_resources_shutdown();
    generic_resources_shutdown();
#ifdef HAVE_TFE
    tfe_resources_shutdown();
#endif
    aciacart_resources_shutdown();
    digimax_resources_shutdown();
    ds12c887rtc_resources_shutdown();
    sfx_soundexpander_resources_shutdown();
    sfx_soundsampler_resources_shutdown();
    georam_resources_shutdown();

    lib_free(cartridge_file);
    lib_free(cartfile);
}

static int detach_cartridge_cmdline(const char *param, void *extra_param)
{
    /*
     * this is called by '+cart' and relies on that command line options
     * are processed after the default cartridge gets attached via
     * resources/.ini.
     */
    cartridge_detach_image(-1);
    return 0;
}

static int attach_cartridge_cmdline(const char *param, void *extra_param)
{
    return cartridge_attach_image(vice_ptr_to_int(extra_param), param);
}

static const cmdline_option_t cmdline_options[] =
{
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
    { "-cart2", CALL_FUNCTION, 1,
      attach_cartridge_cmdline, (void *)CARTRIDGE_VIC20_16KB_2000, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_EXT_ROM_2000_NAME,
      NULL, NULL },
    { "-cart4", CALL_FUNCTION, 1,
      attach_cartridge_cmdline, (void *)CARTRIDGE_VIC20_16KB_4000, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_EXT_ROM_4000_NAME,
      NULL, NULL },
    { "-cart6", CALL_FUNCTION, 1,
      attach_cartridge_cmdline, (void *)CARTRIDGE_VIC20_16KB_6000, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_EXT_ROM_6000_NAME,
      NULL, NULL },
    { "-cartA", CALL_FUNCTION, 1,
      attach_cartridge_cmdline, (void *)CARTRIDGE_VIC20_8KB_A000, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_EXT_ROM_A000_NAME,
      NULL, NULL },
    { "-cartB", CALL_FUNCTION, 1,
      attach_cartridge_cmdline, (void *)CARTRIDGE_VIC20_4KB_B000, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_EXT_ROM_B000_NAME,
      NULL, NULL },
    { "-cartgeneric", CALL_FUNCTION, 1,
      attach_cartridge_cmdline, (void *)CARTRIDGE_VIC20_GENERIC, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_GENERIC_ROM_NAME,
      NULL, NULL },
    { "-cartmega", CALL_FUNCTION, 1,
      attach_cartridge_cmdline, (void *)CARTRIDGE_VIC20_MEGACART, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_MEGA_CART_ROM_NAME,
      NULL, NULL },
    { "-cartfe", CALL_FUNCTION, 1,
      attach_cartridge_cmdline, (void *)CARTRIDGE_VIC20_FINAL_EXPANSION, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_FINAL_EXPANSION_ROM_NAME,
      NULL, NULL },
    { "-cartfp", CALL_FUNCTION, 1,
      attach_cartridge_cmdline, (void *)CARTRIDGE_VIC20_FP, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_VIC_FP_ROM_NAME,
      NULL, NULL },
    { "+cart", CALL_FUNCTION, 0,
      detach_cartridge_cmdline, NULL, NULL, NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_CART,
      NULL, NULL },
    { NULL }
};

int cartridge_cmdline_options_init(void)
{
    mon_cart_cmd.cartridge_attach_image = cartridge_attach_image;
    mon_cart_cmd.cartridge_detach_image = cartridge_detach_image;

    if (cmdline_register_options(cmdline_options) < 0
        || finalexpansion_cmdline_options_init() < 0
        || vic_fp_cmdline_options_init() < 0
        || megacart_cmdline_options_init() < 0
#ifdef HAVE_TFE
        || tfe_cmdline_options_init() < 0
#endif
        || aciacart_cmdline_options_init() < 0
        || digimax_cmdline_options_init() < 0
        || ds12c887rtc_cmdline_options_init() < 0
        || sfx_soundexpander_cmdline_options_init() < 0
        || sfx_soundsampler_cmdline_options_init() < 0
        || georam_cmdline_options_init() < 0) {
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
static int cartridge_attach_from_resource(int type, const char *filename)
{
    if (vic20cart_type == CARTRIDGE_VIC20_GENERIC) {
        /* special case handling for the multiple file generic type */
        return generic_attach_from_resource(vic20cart_type, cartfile);
    }
    return cartridge_attach_image(vic20cart_type, cartfile);
}

int cartridge_attach_image(int type, const char *filename)
{
    int type_orig;
    int generic_multifile = 0;
    int ret = 0;

    /* Attaching no cartridge always works.  */
    if (type == CARTRIDGE_NONE || filename == NULL || *filename == '\0') {
        return 0;
    }

    log_message(LOG_DEFAULT, "Attached cartridge type %d, file=`%s'.", type, filename);

    type_orig = type;
    switch (type_orig) {
        case CARTRIDGE_VIC20_DETECT:
        case CARTRIDGE_VIC20_4KB_2000:
        case CARTRIDGE_VIC20_8KB_2000:
        case CARTRIDGE_VIC20_4KB_6000:
        case CARTRIDGE_VIC20_8KB_6000:
        case CARTRIDGE_VIC20_4KB_A000:
        case CARTRIDGE_VIC20_8KB_A000:
        case CARTRIDGE_VIC20_4KB_B000:
        case CARTRIDGE_VIC20_8KB_4000:
        case CARTRIDGE_VIC20_4KB_4000:
        case CARTRIDGE_VIC20_16KB_2000:
        case CARTRIDGE_VIC20_16KB_4000:
        case CARTRIDGE_VIC20_16KB_6000:
            /*
             * For specific layouts only detach if we were something else than
             * CARTRIDGE_VIC20_GENERIC before.
             * This allows us to add images to a generic type.
             */
            if (vic20cart_type != CARTRIDGE_VIC20_GENERIC) {
                cartridge_detach_image(-1);
            }
            generic_multifile = 1;
            type = CARTRIDGE_VIC20_GENERIC;
            break;
        case CARTRIDGE_VIC20_GENERIC:
            /*
             * this is because the only generic cart that is attachable
             * will be attached as a auto detected multi file cart for now
             * Remove when this changes.
             */
            generic_multifile = 1;
            break;
        default:
            cartridge_detach_image(-1);
    }

    switch (type) {
        case CARTRIDGE_VIC20_GENERIC:
            ret = generic_bin_attach(type_orig, filename);
            break;
        case CARTRIDGE_VIC20_FP:
            ret = vic_fp_bin_attach(filename);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            ret = megacart_bin_attach(filename);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            ret = finalexpansion_bin_attach(filename);
            break;
    }

    vic20cart_type = type;
    if (generic_multifile) {
        util_string_set(&cartfile, NULL);
    } else {
        util_string_set(&cartfile, filename);
    }
    if (ret == 0) {
        cartridge_attach(type, NULL);
    }
    return ret;
}

void cartridge_detach_image(int type)
{
    cartridge_detach(vic20cart_type);
    vic20cart_type = CARTRIDGE_NONE;
    cartridge_is_from_snapshot = 0;
}

void cartridge_set_default(void)
{
    if (cartridge_is_from_snapshot) {
        /* TODO replace with ui_error */
        log_warning(LOG_DEFAULT, "Set as default disabled");
        return;
    }

    set_cartridge_type(vic20cart_type, NULL);
    set_cartridge_file((vic20cart_type == CARTRIDGE_NONE) ? "" : cartfile, NULL);
    /* special case handling for the multiple file generic type */
    generic_set_default();

    /* reset the try flags (we've only called the set function once each) */
    reset_try_flags();
}

const char *cartridge_get_file_name(int addr)
{
    if (vic20cart_type == CARTRIDGE_VIC20_GENERIC) {
        /* special case handling for the multiple file generic type */
        return generic_get_file_name((WORD)addr);
    }

    return cartfile;
}

/* ------------------------------------------------------------------------- */

#define VIC20CART_DUMP_VER_MAJOR   2
#define VIC20CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "VIC20CART"

int vic20cart_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;
    int ret = 0;

    m = snapshot_module_create(s, SNAP_MODULE_NAME, VIC20CART_DUMP_VER_MAJOR, VIC20CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (SMW_DW(m, (DWORD)vic20cart_type) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    switch (vic20cart_type) {
        case CARTRIDGE_VIC20_GENERIC:
            ret = generic_snapshot_write_module(s);
            break;

        case CARTRIDGE_VIC20_FP:
            ret = vic_fp_snapshot_write_module(s);
            break;

        case CARTRIDGE_VIC20_MEGACART:
            ret = megacart_snapshot_write_module(s);
            break;

        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            ret = finalexpansion_snapshot_write_module(s);
            break;

        case CARTRIDGE_NONE:
        default:
            break;
    }

    return ret;
}

int vic20cart_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;
    int new_cart_type, cartridge_reset;
    int ret = 0;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if (vmajor != VIC20CART_DUMP_VER_MAJOR) {
        snapshot_module_close(m);
        return -1;
    }

    if (SMR_DW_INT(m, &new_cart_type) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    /* disable cartridge reset while detaching old cart */
    resources_get_int("CartridgeReset", &cartridge_reset);
    resources_set_int("CartridgeReset", 0);
    cartridge_detach_image(-1);
    resources_set_int("CartridgeReset", cartridge_reset);

    /* disallow "set as default" and write back */
    cartridge_is_from_snapshot = 1;

    vic20cart_type = new_cart_type;
    mem_cartridge_type = new_cart_type;

    switch (vic20cart_type) {
        case CARTRIDGE_VIC20_GENERIC:
            ret = generic_snapshot_read_module(s);
            break;

        case CARTRIDGE_VIC20_FP:
            ret = vic_fp_snapshot_read_module(s);
            break;

        case CARTRIDGE_VIC20_MEGACART:
            ret = megacart_snapshot_read_module(s);
            break;

        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            ret = finalexpansion_snapshot_read_module(s);
            break;

        case CARTRIDGE_NONE:
            /* cart already detached, nothing to do */
            break;

        default:
            /* unknown cart */
            ret = -1;
            break;
    }

    if (ret < 0) {
        vic20cart_type = CARTRIDGE_NONE;
        mem_cartridge_type = CARTRIDGE_NONE;
    }

    return ret;
}
