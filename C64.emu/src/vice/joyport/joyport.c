/*
 * joyport.c - control port handling.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "cmdline.h"
#include "joyport.h"
#include "lib.h"
#include "resources.h"
#include "translate.h"
#include "uiapi.h"
#include "util.h"

static joyport_t joyport_device[JOYPORT_MAX_DEVICES];
static BYTE joyport_display[6] = { 0, 0, 0, 0, 0, 0};

static int joy_port[JOYPORT_MAX_PORTS];
static joyport_port_props_t port_props[JOYPORT_MAX_PORTS];
static int pot_port_mask = 1;

static BYTE joyport_dig_stored[JOYPORT_MAX_PORTS];

typedef struct resid2transid_s {
    int resid;
    int transid;
} resid2transid_t;

static resid2transid_t ids[] = {
    { JOYPORT_RES_ID_MOUSE, IDGS_HOST_MOUSE },
    { JOYPORT_RES_ID_SAMPLER, IDGS_HOST_SAMPLER },
    { -1, -1 }
};

static char *res2text(int id)
{
    int i;
    char *retval = "Unknown joyport resource";

    for (i = 0; ids[i].resid != -1; ++i) {
        if (ids[i].resid == id) {
            retval = translate_text(ids[i].transid);
        }
    }
    return retval;
}

void set_joyport_pot_mask(int mask)
{
    pot_port_mask = mask;
}

static int joyport_set_device(int port, int id)
{
    int i;

    /* 1st some sanity checks */
    if (id < JOYPORT_ID_NONE || id >= JOYPORT_MAX_DEVICES) {
        return -1;
    }
    if (port < 0 || port >= JOYPORT_MAX_PORTS) {
        return -1;
    }

    /* Nothing changes */
    if (id == joy_port[port]) {
        return 0;
    }

    /* check if port is present */
    if (!port_props[port].name) {
        ui_error(translate_text(IDGS_SELECTED_PORT_NOT_PRESENT), port);
        return -1;
    }

    /* check if id is registered */
    if (id != JOYPORT_ID_NONE && !joyport_device[id].name) {
        ui_error(translate_text(IDGS_SELECTED_JOYPORT_DEV_NOT_REG), id);
        return -1;
    }

    /* check if id conflicts with devices on other ports */
    if (id != JOYPORT_ID_NONE && id != JOYPORT_ID_JOYSTICK) {
        for (i = 0; i < JOYPORT_MAX_PORTS; ++i) {
            if (port != i && joy_port[i] == id) {
                ui_error(translate_text(IDGS_SELECTED_JOYPORT_DEV_ALREADY_ATTACHED), joyport_device[id].name, translate_text(port_props[port].trans_name), translate_text(port_props[i].trans_name));
                return -1;
            }
        }
    }

    /* check if input resource conflicts with device on the other port */
    if (id != JOYPORT_ID_NONE && id != JOYPORT_ID_JOYSTICK && joyport_device[id].resource_id != JOYPORT_RES_ID_NONE) {
        for (i = 0; i < JOYPORT_MAX_PORTS; ++i) {
            if (port != i && joyport_device[id].resource_id == joyport_device[joy_port[i]].resource_id) {
                ui_error(translate_text(IDGS_SELECTED_JOYPORT_SAME_INPUT_RES), joyport_device[id].name, translate_text(port_props[port].trans_name), res2text(joyport_device[id].resource_id), translate_text(port_props[i].trans_name));
                return -1;
            }
        }
    }

    /* check if device can be connected to this port */
    if (id != JOYPORT_ID_NONE && id != JOYPORT_ID_JOYSTICK && joyport_device[id].is_lp && !port_props[port].has_lp_support) {
        ui_error(translate_text(IDGS_SELECTED_DEVICE_NOT_THIS_PORT), joyport_device[id].name, translate_text(port_props[port].trans_name));
        return -1;
    }

    /* all checks done, now disable the current device and enable the new device */
    if (joyport_device[joy_port[port]].enable) {
        joyport_device[joy_port[port]].enable(port, 0);
    }
    if (joyport_device[id].enable) {
        joyport_device[id].enable(port, id);
    }
    joy_port[port] = id;

    return 0;
}

void joyport_clear_devices(void)
{
    int i;

    for (i = 0; i < JOYPORT_MAX_PORTS; ++i) {
        if (port_props[i].name) {
            joyport_set_device(i, JOYPORT_ID_NONE);
        }
    }
}

BYTE read_joyport_dig(int port)
{
    int id = joy_port[port];

    if (id == JOYPORT_ID_NONE) {
        return 0xff;
    }

    if (!joyport_device[id].read_digital) {
        return 0xff;
    }
    return joyport_device[id].read_digital(port);
}

void store_joyport_dig(int port, BYTE val, BYTE mask)
{
    int id = joy_port[port];
    BYTE store_val;

    if (id == JOYPORT_ID_NONE) {
        return;
    }

    if (!joyport_device[id].store_digital) {
        return;
    }

    store_val = joyport_dig_stored[port];

    store_val &= (BYTE)~mask;
    store_val |= val;

    joyport_device[id].store_digital(store_val);

    joyport_dig_stored[port] = store_val;
}

static int pot_port1 = -1;
static int pot_port2 = -1;

static void find_pot_ports(void)
{
    int i;

    for (i = 0; i < JOYPORT_MAX_PORTS; ++i) {
        if (port_props[i].has_pot) {
            if (pot_port1 == -1) {
                pot_port1 = i;
            } else {
                pot_port2 = i;
            }
        }
    }
    if (pot_port1 == -1) {
        pot_port1 = -2;
    }
    if (pot_port2 == -1) {
        pot_port2 = -2;
    }
}

BYTE read_joyport_potx(void)
{
    int id1 = JOYPORT_ID_NONE;
    int id2 = JOYPORT_ID_NONE;
    BYTE ret1 = 0xff;
    BYTE ret2 = 0xff;

    /* first find the pot ports if needed */
    if (pot_port1 == -1 || pot_port2 == -1) {
        find_pot_ports();
    }

    if (pot_port_mask == 1 || pot_port_mask == 3) {
        if (pot_port1 != -2) {
            id1 = joy_port[pot_port1];
        }
    }

    if (pot_port_mask == 2 || pot_port_mask == 3) {
        if (pot_port2 != -2) {
            id2 = joy_port[pot_port2];
        }
    }

    if (id1 != JOYPORT_ID_NONE) {
        if (joyport_device[id1].read_potx) {
            ret1 = joyport_device[id1].read_potx();
        }
    }

    if (id2 != JOYPORT_ID_NONE) {
        if (joyport_device[id2].read_potx) {
            ret2 = joyport_device[id2].read_potx();
        }
    }

    switch (pot_port_mask) {
        case 1:
            return ret1;
        case 2:
            return ret2;
        case 3:
            return ret1 & ret2;
        default:
            return 0xff;
    }
}

BYTE read_joyport_poty(void)
{
    int id1 = JOYPORT_ID_NONE;
    int id2 = JOYPORT_ID_NONE;
    BYTE ret1 = 0xff;
    BYTE ret2 = 0xff;

    /* first find the pot ports if needed */
    if (pot_port1 == -1 || pot_port2 == -1) {
        find_pot_ports();
    }

    if (pot_port_mask == 1 || pot_port_mask == 3) {
        if (pot_port1 != -2) {
            id1 = joy_port[pot_port1];
        }
    }

    if (pot_port_mask == 2 || pot_port_mask == 3) {
        if (pot_port2 != -2) {
            id2 = joy_port[pot_port2];
        }
    }

    if (id1 != JOYPORT_ID_NONE) {
        if (joyport_device[id1].read_poty) {
            ret1 = joyport_device[id1].read_poty();
        }
    }

    if (id2 != JOYPORT_ID_NONE) {
        if (joyport_device[id2].read_poty) {
            ret2 = joyport_device[id2].read_poty();
        }
    }

    switch (pot_port_mask) {
        case 1:
            return ret1;
        case 2:
            return ret2;
        case 3:
            return ret1 & ret2;
        default:
            return 0xff;
    }
}

static int pot_present = -1;

int joyport_device_register(int id, joyport_t *device)
{
    int i;

    if (id < 1 || id > JOYPORT_MAX_DEVICES) {
        return -1;
    }

    /* check for pot ports if needed */
    if (pot_present == -1) {
        for (i = 0; i < JOYPORT_MAX_PORTS && pot_present == -1; ++i) {
            if (port_props[i].has_pot) {
                pot_present = 1;
            }
        }
        if (pot_present == -1) {
            pot_present = 0;
        }
    }

    /* skip pot devices if no pot is present */
    if ((device->read_potx || device->read_poty) && !pot_present && !device->pot_optional) {
        return 0;
    }

    joyport_device[id].name = device->name;
    joyport_device[id].trans_name = device->trans_name;
    joyport_device[id].resource_id = device->resource_id;
    joyport_device[id].is_lp = device->is_lp;
    joyport_device[id].pot_optional = device->pot_optional;
    joyport_device[id].enable = device->enable;
    joyport_device[id].read_digital = device->read_digital;
    joyport_device[id].store_digital = device->store_digital;
    joyport_device[id].read_potx = device->read_potx;
    joyport_device[id].read_poty = device->read_poty;
    joyport_device[id].write_snapshot = device->write_snapshot;
    joyport_device[id].read_snapshot = device->read_snapshot;
    return 0;
}

int joyport_port_register(int port, joyport_port_props_t *props)
{
    if (port < 0 || port >= JOYPORT_MAX_PORTS) {
        return -1;
    }

    if (!port) {
        memset(port_props, 0, sizeof(port_props));
    }

    port_props[port].name = props->name;
    port_props[port].trans_name = props->trans_name;
    port_props[port].has_pot = props->has_pot;
    port_props[port].has_lp_support = props->has_lp_support;
    port_props[port].active = props->active;

    return 0;
}

static int check_valid_lightpen(int port, int index)
{
    if (!joyport_device[index].is_lp) {
        return 1;
    }
    if (port_props[port].has_lp_support) {
        return 1;
    }
    return 0;
}

static int check_valid_pot(int port, int index)
{
    if (!joyport_device[index].read_potx && !joyport_device[index].read_poty) {
        return 1;
    }
    if (port_props[port].has_pot || joyport_device[index].pot_optional) {
        return 1;
    }
    return 0;
}

joyport_desc_t *joyport_get_valid_devices(int port)
{
    joyport_desc_t *retval = NULL;
    int i;
    int valid = 0;
    int j = 0;

    for (i = 0; i < JOYPORT_MAX_DEVICES; ++i) {
        if (joyport_device[i].name) {
            if (check_valid_lightpen(port, i) && check_valid_pot(port, i)) {
                ++valid;
            }
        }
    }

    retval = lib_malloc(((size_t)valid + 1) * sizeof(joyport_desc_t));
    for (i = 0; i < JOYPORT_MAX_DEVICES; ++i) {
        if (joyport_device[i].name) {
            if (check_valid_lightpen(port, i) && check_valid_pot(port, i)) {
                retval[j].name = joyport_device[i].name;
                retval[j].trans_name = joyport_device[i].trans_name;
                retval[j].id = i;
                ++j;
            }
        }
    }
    retval[j].name = NULL;

    return retval;
}

void joyport_display_joyport(int id, BYTE status)
{
    if (id == JOYPORT_ID_JOY1 || id == JOYPORT_ID_JOY2 || id == JOYPORT_ID_JOY3 || id == JOYPORT_ID_JOY4 || id == JOYPORT_ID_JOY5) {
        if (id == JOYPORT_ID_JOY1 && joy_port[0] == JOYPORT_ID_JOYSTICK) {
            joyport_display[1] = status;
        }
        if (id == JOYPORT_ID_JOY2 && joy_port[1] == JOYPORT_ID_JOYSTICK) {
            joyport_display[2] = status;
        }
        if (id == JOYPORT_ID_JOY3 && joy_port[2] == JOYPORT_ID_JOYSTICK) {
            joyport_display[3] = status;
        }
        if (id == JOYPORT_ID_JOY4 && joy_port[3] == JOYPORT_ID_JOYSTICK) {
            joyport_display[4] = status;
        }
        if (id == JOYPORT_ID_JOY5 && joy_port[4] == JOYPORT_ID_JOYSTICK) {
            joyport_display[5] = status;
        }
    } else {
        if (id != joy_port[0] && id != joy_port[1] && id != joy_port[2] && id != joy_port[3] && id != joy_port[4]) {
            return;
        }

        if (id == joy_port[0]) {
            joyport_display[1] = status;
        }

        if (id == joy_port[1]) {
            joyport_display[2] = status;
        }

        if (id == joy_port[2]) {
            joyport_display[3] = status;
        }

        if (id == joy_port[3]) {
            joyport_display[4] = status;
        }

        if (id == joy_port[4]) {
            joyport_display[5] = status;
        }
    }
    ui_display_joyport(joyport_display);
}

int joyport_get_port_trans_name(int port)
{
    return port_props[port].trans_name;
}

char *joyport_get_port_name(int port)
{
    return port_props[port].name;
}

/* ------------------------------------------------------------------------- */

static int set_joyport_device(int val, void *param)
{
    int port = vice_ptr_to_int(param);

    return joyport_set_device(port, val);
}

static const resource_int_t resources_int_port1[] = {
    { "JoyPort1Device", JOYPORT_ID_JOYSTICK, RES_EVENT_NO, NULL,
      &joy_port[JOYPORT_1], set_joyport_device, (void *)JOYPORT_1 },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_int_port2[] = {
    { "JoyPort2Device", JOYPORT_ID_JOYSTICK, RES_EVENT_NO, NULL,
      &joy_port[JOYPORT_2], set_joyport_device, (void *)JOYPORT_2 },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_int_port3[] = {
    { "JoyPort3Device", JOYPORT_ID_JOYSTICK, RES_EVENT_NO, NULL,
      &joy_port[JOYPORT_3], set_joyport_device, (void *)JOYPORT_3 },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_int_port4[] = {
    { "JoyPort4Device", JOYPORT_ID_JOYSTICK, RES_EVENT_NO, NULL,
      &joy_port[JOYPORT_4], set_joyport_device, (void *)JOYPORT_4 },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_int_port5[] = {
    { "JoyPort5Device", JOYPORT_ID_JOYSTICK, RES_EVENT_NO, NULL,
      &joy_port[JOYPORT_5], set_joyport_device, (void *)JOYPORT_5 },
    RESOURCE_INT_LIST_END
};

int joyport_resources_init(void)
{
    int i;

    memset(joyport_device, 0, sizeof(joyport_device));
    joyport_device[0].name = "None";
    joyport_device[0].trans_name = IDGS_NONE;
    joyport_device[0].is_lp = JOYPORT_IS_NOT_LIGHTPEN;
    for (i = 0; i < JOYPORT_MAX_PORTS; ++i) {
        joy_port[i] = JOYPORT_ID_NONE;
    }

    if (port_props[JOYPORT_5].name) {
        if (resources_register_int(resources_int_port5) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_4].name) {
        if (resources_register_int(resources_int_port4) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_3].name) {
        if (resources_register_int(resources_int_port3) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_2].name) {
        if (resources_register_int(resources_int_port2) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_1].name) {
        if (resources_register_int(resources_int_port1) < 0) {
            return -1;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

struct joyport_opt_s {
    const char *name;
    int id;
};

static struct joyport_opt_s id_match[] = {
    { "0",               JOYPORT_ID_NONE },
    { "none",            JOYPORT_ID_NONE },
    { "1",               JOYPORT_ID_JOYSTICK },
    { "joy",             JOYPORT_ID_JOYSTICK },
    { "joystick",        JOYPORT_ID_JOYSTICK },
    { "2",               JOYPORT_ID_PADDLES },
    { "paddles",         JOYPORT_ID_PADDLES },
    { "3",               JOYPORT_ID_MOUSE_1351 },
    { "1351",            JOYPORT_ID_MOUSE_1351 },
    { "1351mouse",       JOYPORT_ID_MOUSE_1351 },
    { "4",               JOYPORT_ID_MOUSE_NEOS },
    { "neos",            JOYPORT_ID_MOUSE_NEOS },
    { "neosmouse",       JOYPORT_ID_MOUSE_NEOS },
    { "5",               JOYPORT_ID_MOUSE_AMIGA },
    { "amiga",           JOYPORT_ID_MOUSE_AMIGA },
    { "amigamouse",      JOYPORT_ID_MOUSE_AMIGA },
    { "6",               JOYPORT_ID_MOUSE_CX22 },
    { "cx22",            JOYPORT_ID_MOUSE_CX22 },
    { "cx22mouse",       JOYPORT_ID_MOUSE_CX22 },
    { "7",               JOYPORT_ID_MOUSE_ST },
    { "st",              JOYPORT_ID_MOUSE_ST },
    { "atarist",         JOYPORT_ID_MOUSE_ST },
    { "stmouse",         JOYPORT_ID_MOUSE_ST },
    { "ataristmouse",    JOYPORT_ID_MOUSE_ST },
    { "8",               JOYPORT_ID_MOUSE_SMART },
    { "smart",           JOYPORT_ID_MOUSE_SMART },
    { "smartmouse",      JOYPORT_ID_MOUSE_SMART },
    { "9",               JOYPORT_ID_MOUSE_MICROMYS },
    { "micromys",        JOYPORT_ID_MOUSE_MICROMYS },
    { "micromysmouse",   JOYPORT_ID_MOUSE_MICROMYS },
    { "10",              JOYPORT_ID_KOALAPAD },
    { "koalapad",        JOYPORT_ID_KOALAPAD },
    { "11",              JOYPORT_ID_LIGHTPEN_U },
    { "lpup",            JOYPORT_ID_LIGHTPEN_U },
    { "lightpenup",      JOYPORT_ID_LIGHTPEN_U },
    { "12",              JOYPORT_ID_LIGHTPEN_L },
    { "lpleft",          JOYPORT_ID_LIGHTPEN_L },
    { "lightpenleft",    JOYPORT_ID_LIGHTPEN_L },
    { "13",              JOYPORT_ID_LIGHTPEN_DATEL },
    { "lpdatel",         JOYPORT_ID_LIGHTPEN_DATEL },
    { "lightpendatel",   JOYPORT_ID_LIGHTPEN_DATEL },
    { "datellightpen",   JOYPORT_ID_LIGHTPEN_DATEL },
    { "14",              JOYPORT_ID_LIGHTGUN_Y },
    { "magnum",          JOYPORT_ID_LIGHTGUN_Y },
    { "15",              JOYPORT_ID_LIGHTGUN_L },
    { "stack",           JOYPORT_ID_LIGHTGUN_L },
    { "slr",             JOYPORT_ID_LIGHTGUN_L },
    { "16",              JOYPORT_ID_LIGHTPEN_INKWELL },
    { "lpinkwell",       JOYPORT_ID_LIGHTPEN_INKWELL },
    { "lightpeninkwell", JOYPORT_ID_LIGHTPEN_INKWELL },
    { "inkwelllightpen", JOYPORT_ID_LIGHTPEN_INKWELL },
    { "17",              JOYPORT_ID_SAMPLER_2BIT },
    { "2bitsampler",     JOYPORT_ID_SAMPLER_2BIT },
    { "18",              JOYPORT_ID_SAMPLER_4BIT },
    { "4bitsampler",     JOYPORT_ID_SAMPLER_4BIT },
    { "19",              JOYPORT_ID_BBRTC },
    { "bbrtc",           JOYPORT_ID_BBRTC },
    { "20",              JOYPORT_ID_PAPERCLIP64 },
    { "paperclip64",     JOYPORT_ID_PAPERCLIP64 },
    { "paperclip",       JOYPORT_ID_PAPERCLIP64 },
    { "pc64",            JOYPORT_ID_PAPERCLIP64 },
    { NULL, -1 }
};

static int set_joyport_cmdline_device(const char *param, void *extra_param)
{
    int temp = -1;
    int i = 0;
    int port = vice_ptr_to_int(extra_param);

    if (!param) {
        return -1;
    }

    do {
        if (strcmp(id_match[i].name, param) == 0) {
            temp = id_match[i].id;
        }
        i++;
    } while ((temp == -1) && (id_match[i].name != NULL));

    if (temp == -1) {
        return -1;
    }

    return set_joyport_device(temp, int_to_void_ptr(port));
}

/* ------------------------------------------------------------------------- */

static char *build_joyport_string(int port)
{
    int i = 0;
    char *tmp1;
    char *tmp2;
    char number[4];
    joyport_desc_t *devices = joyport_get_valid_devices(port);

    tmp1 = lib_msprintf(translate_text(IDGS_SET_JOYPORT_S_DEVICE), translate_text(port_props[port].trans_name));

    for (i = 1; devices[i].name; ++i) {
        sprintf(number, "%d", devices[i].id);
        tmp2 = util_concat(tmp1, ", ", number, ": ", translate_text(devices[i].trans_name), NULL);
        lib_free(tmp1);
        tmp1 = tmp2;
    }
    tmp2 = util_concat(tmp1, ")", NULL);
    lib_free(tmp1);
    lib_free(devices);
    return tmp2;
}

static cmdline_option_t cmdline_options_port1[] =
{
    { "-controlport1device", CALL_FUNCTION, 1,
      set_joyport_cmdline_device, (void *)JOYPORT_1, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_DYN,
      IDGS_DEVICE, JOYPORT_1,
      NULL, NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t cmdline_options_port2[] =
{
    { "-controlport2device", CALL_FUNCTION, 1,
      set_joyport_cmdline_device, (void *)JOYPORT_2, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_DYN,
      IDGS_DEVICE, JOYPORT_2,
      NULL, NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t cmdline_options_port3[] =
{
    { "-controlport3device", CALL_FUNCTION, 1,
      set_joyport_cmdline_device, (void *)JOYPORT_3, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_DYN,
      IDGS_DEVICE, JOYPORT_3,
      NULL, NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t cmdline_options_port4[] =
{
    { "-controlport4device", CALL_FUNCTION, 1,
      set_joyport_cmdline_device, (void *)JOYPORT_4, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_DYN,
      IDGS_DEVICE, JOYPORT_4,
      NULL, NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t cmdline_options_port5[] =
{
    { "-controlport5device", CALL_FUNCTION, 1,
      set_joyport_cmdline_device, (void *)JOYPORT_5, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_DYN,
      IDGS_DEVICE, JOYPORT_5,
      NULL, NULL },
    CMDLINE_LIST_END
};

int joyport_cmdline_options_init(void)
{
    union char_func cf;

    if (port_props[JOYPORT_1].name) {
        cf.f = build_joyport_string;
        cmdline_options_port1[0].description = cf.c;
        if (cmdline_register_options(cmdline_options_port1) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_2].name) {
        cf.f = build_joyport_string;
        cmdline_options_port2[0].description = cf.c;
        if (cmdline_register_options(cmdline_options_port2) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_3].name) {
        cf.f = build_joyport_string;
        cmdline_options_port3[0].description = cf.c;
        if (cmdline_register_options(cmdline_options_port3) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_4].name) {
        cf.f = build_joyport_string;
        cmdline_options_port4[0].description = cf.c;
        if (cmdline_register_options(cmdline_options_port4) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_5].name) {
        cf.f = build_joyport_string;
        cmdline_options_port5[0].description = cf.c;
        if (cmdline_register_options(cmdline_options_port5) < 0) {
            return -1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

#define DUMP_VER_MAJOR   0
#define DUMP_VER_MINOR   0

int joyport_snapshot_write_module(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;
    char snapshot_name[16];

    sprintf(snapshot_name, "JOYPORT%d", port);

    m = snapshot_module_create(s, snapshot_name, DUMP_VER_MAJOR, DUMP_VER_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    /* save device id */
    if (SMW_B(m, (BYTE)joy_port[port]) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    /* save seperate joyport device module */
    switch (joy_port[port]) {
        case JOYPORT_ID_NONE:
            break;
        default:
            if (joyport_device[joy_port[port]].write_snapshot) {
                if (joyport_device[joy_port[port]].write_snapshot(s, port) < 0) {
                    return -1;
                }
            }
            break;
    }

    return 0;
}

int joyport_snapshot_read_module(struct snapshot_s *s, int port)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    int temp_joy_port;
    char snapshot_name[16];

    sprintf(snapshot_name, "JOYPORT%d", port);

    m = snapshot_module_open(s, snapshot_name, &major_version, &minor_version);
    if (m == NULL) {
        return -1;
    }

    if (major_version != DUMP_VER_MAJOR || minor_version != DUMP_VER_MINOR) {
        snapshot_module_close(m);
        return -1;
    }

    /* load device id */
    if (SMR_B_INT(m, &temp_joy_port) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    /* enable device */
    joyport_set_device(port, temp_joy_port);

    /* load device snapshot */
    switch (joy_port[port]) {
        case JOYPORT_ID_NONE:
            break;
        default:
            if (joyport_device[joy_port[port]].read_snapshot) {
                if (joyport_device[joy_port[port]].read_snapshot(s, port) < 0) {
                    return -1;
                }
            }
            break;
    }

    return 0;
}
