/*
 * cbm2cart.c -- CBM2 cartridge handling.
 *
 * Written by
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

/* in case cartridge_detect() gets uncommented remove the #if 0/#endif */
#if 0
#include "archdep.h"
#endif
#include "cartridge.h"
#include "cmdline.h"
#include "cbm2cart.h"
#include "cbm2mem.h"
#include "cbm2rom.h"
#include "lib.h"
#include "log.h"
#include "util.h"
#include "machine.h"
#include "monitor.h"
#include "resources.h"
#include "sysfile.h"

/* #define DEBUGCART */

#ifdef DEBUGCART
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

/* Expansion port signals. */
export_t export = { 0, 0, 0, 0 };

/* global options for the cart system */
static int cbm2cartridge_reset; /* (resource) hardreset system after cart was attached/detached */

static char *cart_1_name = NULL;
static char *cart_2_name = NULL;
static char *cart_4_name = NULL;
static char *cart_6_name = NULL;

int cart08_ram = 0;
int cart1_ram = 0;
int cart2_ram = 0;
int cart4_ram = 0;
int cart6_ram = 0;
int cartC_ram = 0;

static uint8_t romh_banks[1]; /* dummy */

uint8_t *ultimax_romh_phi1_ptr(uint16_t addr)
{
    return romh_banks;
}

uint8_t *ultimax_romh_phi2_ptr(uint16_t addr)
{
    return romh_banks;
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
#if 0
    /* smart attach */
    { "-cart", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cart_attach_cmdline, (void*)CARTRIDGE_CBM2_DETECT, NULL, NULL,
      "<Name>", "Smart-attach cartridge image" },
#endif
    /* no cartridge */
    { "+cart", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      cart_attach_cmdline, NULL, NULL, NULL,
      NULL, "Disable default cartridge" },
    { "-cart1", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Cart1Name", NULL,
      "<Name>", "Specify name of cartridge ROM image for $1000" },
    { "-cart2", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Cart2Name", NULL,
      "<Name>", "Specify name of cartridge ROM image for $2000-$3fff" },
    { "-cart4", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Cart4Name", NULL,
      "<Name>", "Specify name of cartridge ROM image for $4000-$5fff" },
    { "-cart6", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Cart6Name", NULL,
      "<Name>", "Specify name of cartridge ROM image for $6000-$7fff" },
    CMDLINE_LIST_END
};

int cartridge_cmdline_options_init(void)
{
    mon_cart_cmd.cartridge_attach_image = cartridge_attach_image;
    mon_cart_cmd.cartridge_detach_image = cartridge_detach_image;
#if 0
    mon_cart_cmd.cartridge_trigger_freeze = cartridge_trigger_freeze;
    mon_cart_cmd.cartridge_trigger_freeze_nmi_only = cartridge_trigger_freeze_nmi_only;
    mon_cart_cmd.export_dump = cbm2export_dump;
#endif
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

static int set_cart1_rom_name(const char *val, void *param)
{
    if (util_string_set(&cart_1_name, val)) {
        return 0;
    }

    return cbm2rom_load_cart_1(cart_1_name);
}

static int set_cart2_rom_name(const char *val, void *param)
{
    if (util_string_set(&cart_2_name, val)) {
        return 0;
    }

    return cbm2rom_load_cart_2(cart_2_name);
}

static int set_cart4_rom_name(const char *val, void *param)
{
    if (util_string_set(&cart_4_name, val)) {
        return 0;
    }

    return cbm2rom_load_cart_4(cart_4_name);
}

static int set_cart6_rom_name(const char *val, void *param)
{
    if (util_string_set(&cart_6_name, val)) {
        return 0;
    }

    return cbm2rom_load_cart_6(cart_6_name);
    /* only does something after mem_load() */
}

static int set_cart08_ram(int val, void *param)
{
    cart08_ram = val ? 1 : 0;

    mem_initialize_memory_bank(15);
    return 0;
}

static int set_cart1_ram(int val, void *param)
{
    cart1_ram = val ? 1 : 0;

    mem_initialize_memory_bank(15);
    return 0;
}

static int set_cart2_ram(int val, void *param)
{
    cart2_ram = val ? 1 : 0;

    mem_initialize_memory_bank(15);
    return 0;
}

static int set_cart4_ram(int val, void *param)
{
    cart4_ram = val ? 1 : 0;

    mem_initialize_memory_bank(15);
    return 0;
}

static int set_cart6_ram(int val, void *param)
{
    cart6_ram = val ? 1 : 0;

    mem_initialize_memory_bank(15);
    return 0;
}

static int set_cartC_ram(int val, void *param)
{
    cartC_ram = val ? 1 : 0;

    mem_initialize_memory_bank(15);
    return 0;
}

static int set_cartridge_reset(int value, void *param)
{
    int val = value ? 1 : 0;

/*    DBG(("cbm2cartridge_reset: %d", val)); */
    if (cbm2cartridge_reset != val) {
        DBG(("cbm2cartridge_reset changed: %d", val));
        cbm2cartridge_reset = val; /* resource value modified */
    }
    return 0;
}

static const resource_string_t resources_string[] = {
    { "Cart1Name", "", RES_EVENT_NO, NULL,
      &cart_1_name, set_cart1_rom_name, NULL },
    { "Cart2Name", "", RES_EVENT_NO, NULL,
      &cart_2_name, set_cart2_rom_name, NULL },
    { "Cart4Name", "", RES_EVENT_NO, NULL,
      &cart_4_name, set_cart4_rom_name, NULL },
    { "Cart6Name", "", RES_EVENT_NO, NULL,
      &cart_6_name, set_cart6_rom_name, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "CartridgeReset", 1, RES_EVENT_NO, NULL,
      &cbm2cartridge_reset, set_cartridge_reset, NULL },
    { "Ram08", 0, RES_EVENT_NO, NULL,
      &cart08_ram, set_cart08_ram, NULL },
    { "Ram1", 0, RES_EVENT_NO, NULL,
      &cart1_ram, set_cart1_ram, NULL },
    { "Ram2", 0, RES_EVENT_NO, NULL,
      &cart2_ram, set_cart2_ram, NULL },
    { "Ram4", 0, RES_EVENT_NO, NULL,
      &cart4_ram, set_cart4_ram, NULL },
    { "Ram6", 0, RES_EVENT_NO, NULL,
      &cart6_ram, set_cart6_ram, NULL },
    { "RamC", 0, RES_EVENT_NO, NULL,
      &cartC_ram, set_cartC_ram, NULL },
    RESOURCE_INT_LIST_END
};

int cartridge_resources_init(void)
{
    /* first the general int resource, so we get the "Cartridge Reset" one */
    if (resources_register_int(resources_int) < 0) {
        return -1;
    }
    return resources_register_string(resources_string);
}

void cartridge_resources_shutdown(void)
{
    if (cart_1_name != NULL) {
        lib_free(cart_1_name);
    }
    if (cart_2_name != NULL) {
        lib_free(cart_2_name);
    }
    if (cart_4_name != NULL) {
        lib_free(cart_4_name);
    }
    if (cart_6_name != NULL) {
        lib_free(cart_6_name);
    }
}

/* ---------------------------------------------------------------------*/

void cart_power_off(void)
{
    if (cbm2cartridge_reset) {
        /* "Turn off machine before removing cartridge" */
        machine_trigger_reset(MACHINE_RESET_MODE_POWER_CYCLE);
    }
}
/* ---------------------------------------------------------------------*/

void cartridge_detach_image(int type)
{
    if (type < 0) {
        resources_set_string("Cart1Name", NULL);
        resources_set_string("Cart2Name", NULL);
        resources_set_string("Cart4Name", NULL);
        resources_set_string("Cart6Name", NULL);
    } else {
        switch (type) {
            case CARTRIDGE_CBM2_8KB_1000:
                resources_set_string("Cart1Name", NULL);
                break;
            case CARTRIDGE_CBM2_8KB_2000:
                resources_set_string("Cart2Name", NULL);
                break;
            case CARTRIDGE_CBM2_16KB_4000:
                resources_set_string("Cart4Name", NULL);
                break;
            case CARTRIDGE_CBM2_16KB_6000:
                resources_set_string("Cart6Name", NULL);
                break;
        }
        cart_power_off();
    }
}

#if 0
int cartridge_detect(const char *filename)
{
    int type = CARTRIDGE_NONE;
    FILE *fd;
    off_t len;

    fd = fopen(filename, "rb");
    if (fd == NULL) {
        return CARTRIDGE_NONE;
    }
    len = archdep_file_size(fd);

    /* FIXME: add cartridge detection */

    fclose (fd);

    DBG(("detected cartridge type: %04x", type));

    return type;
}
#endif

int cartridge_attach_image(int type, const char *filename)
{
#if 0
    if (type == CARTRIDGE_CBM2_DETECT) {
        type = cartridge_detect(filename);
    }
#endif
    cart_power_off();

    switch (type) {
        case CARTRIDGE_CBM2_8KB_1000:
            return resources_set_string("Cart1Name", filename);
        case CARTRIDGE_CBM2_8KB_2000:
            return resources_set_string("Cart2Name", filename);
        case CARTRIDGE_CBM2_16KB_4000:
            return resources_set_string("Cart4Name", filename);
        case CARTRIDGE_CBM2_16KB_6000:
            return resources_set_string("Cart6Name", filename);
        default:
            log_error(LOG_DEFAULT,
                    "cartridge_attach_image: unsupported type (%04x)",
                    (unsigned int)type);
            break;
    }

    return -1;
}


int cartridge_save_image(int type, const char *filename)
{
    return -1;
}

int cartridge_save_secondary_image(int type, const char *filename)
{
    return -1;
}

int cartridge_flush_image(int type)
{
    return -1;
}

int cartridge_flush_secondary_image(int type)
{
    return -1;
}

int cartridge_can_save_image(int crtid)
{
    return 0;
}

int cartridge_can_flush_image(int crtid)
{
    return 0;
}

int cartridge_can_save_secondary_image(int crtid)
{
    return 0;
}

int cartridge_can_flush_secondary_image(int crtid)
{
    return 0;
}

int cartridge_enable(int crtid)
{
    return -1;
}

int cartridge_disable(int crtid)
{
    return -1;
}

int cartridge_type_enabled(int crtid)
{
    return 0;
}

void cartridge_set_default(void)
{
}

void cartridge_unset_default(void)
{
}

cartridge_info_t *cartridge_get_info_list(void)
{
    return NULL;
}

/* return cartridge type of main slot
   returns 0 (CARTRIDGE_CRT) if crt file */
int cartridge_get_id(int slot)
{
    return CARTRIDGE_NONE;
}

/* FIXME: slot arg is ignored right now.
   this should return a pointer to a filename, or NULL
*/
char *cartridge_get_filename_by_slot(int slot)
{
    return NULL;
}

/* FIXME: slot arg is ignored right now.
   this should return a pointer to a filename, or NULL
*/
char *cartridge_get_secondary_filename_by_slot(int slot)
{
    return NULL;
}

void cartridge_trigger_freeze(void)
{
}
