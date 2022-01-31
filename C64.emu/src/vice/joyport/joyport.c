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

/* #define DEBUG_JOYPORT */

#include "vice.h"

#include <string.h>
#include <ctype.h>

#include "cmdline.h"
#include "joyport.h"
#include "lib.h"
#include "machine.h"
#include "resources.h"
#include "uiapi.h"
#include "util.h"

#ifdef DEBUG_JOYPORT
#define DBG(x) printf x
#else
#define DBG(x)
#endif

static joyport_t joyport_device[JOYPORT_MAX_DEVICES];
static uint16_t joyport_display[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static int joy_port[JOYPORT_MAX_PORTS];
static joyport_port_props_t port_props[JOYPORT_MAX_PORTS];
static int pot_port_mask = 1;

static uint8_t joyport_dig_stored[JOYPORT_MAX_PORTS];

static uint8_t joystick_adapter_id = JOYSTICK_ADAPTER_ID_NONE;
static char *joystick_adapter_name = NULL;
static int joystick_adapter_ports = 0;
static int joystick_adapter_additional_ports = 0;

static int (*joystick_output_check_function)(int port, uint8_t bits) = NULL;

typedef struct resid2text_s {
    int resid;
    const char *text;
} resid2text_t;

static resid2text_t ids[] = {
    { JOYPORT_RES_ID_MOUSE, "host mouse" },
    { JOYPORT_RES_ID_SAMPLER, "host sampler (audio input device)" },
    { -1, NULL }
};

static const char *res2text(int joyport_id)
{
    int i;
    const char *retval = "Unknown joyport resource";

    for (i = 0; ids[i].resid != -1; ++i) {
        if (ids[i].resid == joyport_id) {
            retval = ids[i].text;
        }
    }
    return retval;
}

void set_joyport_pot_mask(int mask)
{
    pot_port_mask = mask;
}

static int joyport_device_is_single_port(int id)
{
    switch (id) {
        case JOYPORT_ID_NONE:
        case JOYPORT_ID_JOYSTICK:
        case JOYPORT_ID_TRAPTHEM_SNESPAD:
        case JOYPORT_ID_BBRTC:
        case JOYPORT_ID_PAPERCLIP64:
        case JOYPORT_ID_SCRIPT64_DONGLE:
        case JOYPORT_ID_VIZAWRITE64_DONGLE:
        case JOYPORT_ID_WAASOFT_DONGLE:
        case JOYPORT_ID_PROTOPAD:
        case JOYPORT_ID_PADDLES:
        case JOYPORT_ID_IO_SIMULATION:
            return 0;
    }
    return 1;
}

/* attach device 'id' to port 'port' */
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
        ui_error("Selected port (%d) is not present on this emulator", port);
        return -1;
    }

    /* check if id is registered */
    if (id != JOYPORT_ID_NONE && !joyport_device[id].name) {
        ui_error("Selected control port device %d is not registered", id);
        return -1;
    }

    /* check if id conflicts with devices on other ports */
    if (joyport_device_is_single_port(id)) {
        for (i = 0; i < JOYPORT_MAX_PORTS; ++i) {
            if (port != i && joy_port[i] == id && joy_port[i] != JOYPORT_ID_MULTIJOY_CONTROL) {
                ui_error("Selected control port device %s on %s is already attached to %s", joyport_device[id].name, port_props[port].name, port_props[i].name);
                return -1;
            }
        }
    }

    /* check if input resource conflicts with device on the other port */
    if (joyport_device_is_single_port(id) && joyport_device[id].resource_id != JOYPORT_RES_ID_NONE) {
        for (i = 0; i < JOYPORT_MAX_PORTS; ++i) {
            if (port != i && joyport_device[id].resource_id == joyport_device[joy_port[i]].resource_id) {
                ui_error("Selected control port device %s on %s uses same host input resource (%s) as the device attached to %s", joyport_device[id].name, port_props[port].name, res2text(joyport_device[id].resource_id), port_props[i].name);
                return -1;
            }
        }
    }

    /* check if device can be connected to this port */
    if (joyport_device_is_single_port(id) && joyport_device[id].is_lp && !port_props[port].has_lp_support) {
        ui_error("Selected control port device %s cannot be attached to %s", joyport_device[id].name, port_props[port].name);
        return -1;
    }

    /* check if device is a joystick adapter and a different joystick adapter is already active */
    if (id != JOYPORT_ID_NONE && joyport_device[id].joystick_adapter_id) {
        if (!joyport_device[joy_port[port]].joystick_adapter_id) {
            /* if the current device in this port is not a joystick adapter
               we need to check if a different joystick adapter is already
               active */
            if (joystick_adapter_get_id()) {
                ui_error("Selected control port device %s is a joystick adapter, but joystick adapter %s is already active.", joyport_device[id].name, joystick_adapter_get_name());
                return -1;
            }
        }
    }

    /* all checks done, now disable the current device and enable the new device */
    if (joyport_device[joy_port[port]].enable) {
        joyport_device[joy_port[port]].enable(port, 0);
        if (joyport_device[joy_port[port]].hook) {
            joystick_set_hook(port, 0, 0);
        }
    }
    if (joyport_device[id].enable) {
        joyport_device[id].enable(port, id);
        if (joyport_device[id].hook) {
            joystick_set_hook(port, 1, joyport_device[id].hook_mask);
        }
    }
    joy_port[port] = id;

    return 0;
}

/* detach all devices from all ports */
void joyport_clear_devices(void)
{
    int i;

    for (i = 0; i < JOYPORT_MAX_PORTS; ++i) {
        if (port_props[i].name) {
            joyport_set_device(i, JOYPORT_ID_NONE);
        }
    }
}

void joyport_handle_joystick_hook(int port, uint16_t state)
{
    if (joyport_device[joy_port[port]].hook) {
        joyport_device[joy_port[port]].hook(port, state);
    }
}

/* read the digital lines from port 'port' */
uint8_t read_joyport_dig(int port)
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

/* drive the digital lines that are indicated as active in 'mask' with value 'val' of port 'port' */
void store_joyport_dig(int port, uint8_t val, uint8_t mask)
{
    int id = joy_port[port];
    uint8_t store_val;

    if (id == JOYPORT_ID_NONE) {
        return;
    }

    if (!joyport_device[id].store_digital) {
        return;
    }

    store_val = joyport_dig_stored[port];

    store_val &= (uint8_t)~mask;
    store_val |= val;

    joyport_device[id].store_digital(port, store_val);

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

/* read the X potentiometer value */
uint8_t read_joyport_potx(void)
{
    int id1 = JOYPORT_ID_NONE;
    int id2 = JOYPORT_ID_NONE;
    uint8_t ret1 = 0xff;
    uint8_t ret2 = 0xff;

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
            ret1 = joyport_device[id1].read_potx(pot_port1);
        }
    }

    if (id2 != JOYPORT_ID_NONE) {
        if (joyport_device[id2].read_potx) {
            ret2 = joyport_device[id2].read_potx(pot_port2);
        }
    }

    DBG(("read_joyport_potx id: %d %d ret: %d %d\n", id1, id2, ret1, ret2));

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

/* read the Y potentiometer value */
uint8_t read_joyport_poty(void)
{
    int id1 = JOYPORT_ID_NONE;
    int id2 = JOYPORT_ID_NONE;
    uint8_t ret1 = 0xff;
    uint8_t ret2 = 0xff;

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
            ret1 = joyport_device[id1].read_poty(pot_port1);
        }
    }

    if (id2 != JOYPORT_ID_NONE) {
        if (joyport_device[id2].read_poty) {
            ret2 = joyport_device[id2].read_poty(pot_port2);
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

/* powerup the device, called on hard reset only */
void joyport_powerup(void)
{
    int i;

    for (i = 0; i < JOYPORT_MAX_PORTS; i++) {
        if (joy_port[i] != JOYPORT_ID_NONE) {
            if (joyport_device[joy_port[i]].powerup) {
                joyport_device[joy_port[i]].powerup(i);
            }
        }
    }
}

static int pot_present = -1;

/* register a device to be used in the control port system if possible */
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
    joyport_device[id].resource_id = device->resource_id;
    joyport_device[id].is_lp = device->is_lp;
    joyport_device[id].pot_optional = device->pot_optional;
    joyport_device[id].joystick_adapter_id = device->joystick_adapter_id;
    joyport_device[id].device_type = device->device_type;
    joyport_device[id].output_bits = device->output_bits;
    joyport_device[id].enable = device->enable;
    joyport_device[id].read_digital = device->read_digital;
    joyport_device[id].store_digital = device->store_digital;
    joyport_device[id].read_potx = device->read_potx;
    joyport_device[id].read_poty = device->read_poty;
    joyport_device[id].powerup = device->powerup;
    joyport_device[id].write_snapshot = device->write_snapshot;
    joyport_device[id].read_snapshot = device->read_snapshot;
    joyport_device[id].hook = device->hook;
    joyport_device[id].hook_mask = device->hook_mask;
    return 0;
}

/* register a port to be used in the control port system */
int joyport_port_register(int port, joyport_port_props_t *props)
{
    if (port < 0 || port >= JOYPORT_MAX_PORTS) {
        return -1;
    }

    if (!port) {
        memset(port_props, 0, sizeof(port_props));
    }

    port_props[port].name = props->name;
    port_props[port].has_pot = props->has_pot;
    port_props[port].has_lp_support = props->has_lp_support;
    port_props[port].has_adapter_support = props->has_adapter_support;
    port_props[port].has_output_support = props->has_output_support;
    port_props[port].active = props->active;

    return 0;
}

int joyport_port_has_pot(int port)
{
    return port_props[port].has_pot;
}

static int joystick_adapter_is_snes_adapter(int id)
{
    switch (id) {
        case JOYSTICK_ADAPTER_ID_NINJA_SNES:
        case JOYSTICK_ADAPTER_ID_USERPORT_PETSCII_SNES:
        case JOYSTICK_ADAPTER_ID_USERPORT_SUPERPAD64:
            return 1;
    }
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

static int check_valid_adapter(int port, int index)
{
    if (!joyport_device[index].joystick_adapter_id) {
        if (index != JOYPORT_ID_MULTIJOY_CONTROL) {
            return 1;
        }
    }
    if (port_props[port].has_adapter_support) {
        return 1;
    }
    return 0;
}

static int check_valid_snes_adapter(int port, int index)
{
    if (port <= JOYPORT_2) {
        return 1;
    }
    if (!joystick_adapter_is_snes_adapter(joystick_adapter_id)) {
        return 1;
    }
    if (!index) {
        return 1;
    }
    if (index == JOYPORT_ID_JOYSTICK) {
        return 1;
    }
    return 0;
}

static int check_valid_output_bits(int port, int index)
{
    if (!joyport_device[index].output_bits) {
        return 1;
    }
    if (!port_props[port].has_output_support) {
        return 0;
    }
    if (port <= JOYPORT_2) {
        return 1;
    }
    if (!joystick_adapter_id) {
        return 1;
    }
    if (!joystick_output_check_function) {
        return 0;
    }
    return joystick_output_check_function(port, joyport_device[index].output_bits);
}

static int check_valid_dongle(int port, int index)
{
    /* Currently only c64 dongles are supported,
       this needs to be extended when other machine dongles are supported. */

    if (joyport_device[index].device_type != JOYPORT_DEVICE_C64_DONGLE) {
        return 1;
    }
    if (port >= JOYPORT_3) {
        return 0;
    }
    switch (machine_class) {
        case VICE_MACHINE_C64:
        case VICE_MACHINE_C128:
        case VICE_MACHINE_C64DTV:
        case VICE_MACHINE_C64SC:
        case VICE_MACHINE_SCPU64:
            return 1;
    }
    return 0;
}

static int check_valid_io_sim(int port, int index)
{
    if (joyport_device[index].device_type != JOYPORT_DEVICE_IO_SIMULATION) {
        return 1;
    }
    /* check for plus4 port 6 */
    if (port == JOYPORT_6 && machine_class == VICE_MACHINE_PLUS4) {
        return 1;
    }
    if (port == JOYPORT_1 || port == JOYPORT_2) {
        return 1;
    }
    return 0;
}

static int joyport_valid_devices_compare_names(const void* a, const void* b)
{
    const joyport_desc_t *arg1 = (const joyport_desc_t*)a;
    const joyport_desc_t *arg2 = (const joyport_desc_t*)b;

    if (arg1->device_type != arg2->device_type) {
        if (arg1->device_type < arg2->device_type) {
            return -1;
        } else {
            return 1;
        }
    }

    return strcmp(arg1->name, arg2->name);
}

static int joyport_check_valid_devices(int port, int index)
{
    if (!check_valid_lightpen(port, index)) {
        return 0;
    }
    if (!check_valid_pot(port, index)) {
        return 0;
    }
    if (!check_valid_adapter(port, index)) {
        return 0;
    }
    if (!check_valid_snes_adapter(port, index)) {
        return 0;
    }
    if (!check_valid_output_bits(port, index)) {
        return 0;
    }
    if (!check_valid_dongle(port, index)) {
        return 0;
    }
    if (!check_valid_io_sim(port, index)) {
        return 0;
    }
    return 1;
}

static char *joyport_get_joystick_device_name(int port)
{
    if (port == JOYPORT_1 || port == JOYPORT_2) {
        return "Joystick";
    }
    switch (joystick_adapter_get_id()) {
        case JOYSTICK_ADAPTER_ID_NONE:
        case JOYSTICK_ADAPTER_ID_GENERIC_USERPORT:
        case JOYSTICK_ADAPTER_ID_SPACEBALLS:
        case JOYSTICK_ADAPTER_ID_MULTIJOY:
        case JOYSTICK_ADAPTER_ID_INCEPTION:
            return "Joystick";
        case JOYSTICK_ADAPTER_ID_NINJA_SNES:
        case JOYSTICK_ADAPTER_ID_USERPORT_PETSCII_SNES:
        case JOYSTICK_ADAPTER_ID_USERPORT_SUPERPAD64:
            return "SNES Pad";
    }
    return "Unknown joystick";
}

joyport_desc_t *joyport_get_valid_devices(int port, int sort)
{
    joyport_desc_t *retval = NULL;
    int i;
    int valid = 0;
    int j = 0;

    for (i = 0; i < JOYPORT_MAX_DEVICES; ++i) {
        if (joyport_device[i].name) {
            if (joyport_check_valid_devices(port, i)) {
                ++valid;
            }
        }
    }

    retval = lib_malloc(((size_t)valid + 1) * sizeof(joyport_desc_t));
    for (i = 0; i < JOYPORT_MAX_DEVICES; ++i) {
        if (joyport_device[i].name) {
            if (joyport_check_valid_devices(port, i)) {
                if (i == JOYPORT_ID_JOYSTICK) {
                    retval[j].name = joyport_get_joystick_device_name(port);
                } else {
                    retval[j].name = joyport_device[i].name;
                }
                retval[j].id = i;
                retval[j].device_type = joyport_device[i].device_type;
                ++j;
            }
        }
    }
    retval[j].name = NULL;

    if (sort) {
        qsort(retval, valid, sizeof(joyport_desc_t), joyport_valid_devices_compare_names);
    }

    return retval;
}

static int joyport_valid_joyport_display(int id)
{
    switch (id) {
        case JOYPORT_ID_JOY1:
        case JOYPORT_ID_JOY2:
        case JOYPORT_ID_JOY3:
        case JOYPORT_ID_JOY4:
        case JOYPORT_ID_JOY5:
        case JOYPORT_ID_JOY6:
        case JOYPORT_ID_JOY7:
        case JOYPORT_ID_JOY8:
        case JOYPORT_ID_JOY9:
        case JOYPORT_ID_JOY10:
            return 1;
    }
    return 0;
}

void joyport_display_joyport(int id, uint16_t status)
{
    int i;
    int all = 1;

    if (joyport_valid_joyport_display(id)) {
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
        if (id == JOYPORT_ID_JOY6 && joy_port[5] == JOYPORT_ID_JOYSTICK) {
            joyport_display[6] = status;
        }
        if (id == JOYPORT_ID_JOY7 && joy_port[6] == JOYPORT_ID_JOYSTICK) {
            joyport_display[7] = status;
        }
        if (id == JOYPORT_ID_JOY8 && joy_port[7] == JOYPORT_ID_JOYSTICK) {
            joyport_display[8] = status;
        }
        if (id == JOYPORT_ID_JOY9 && joy_port[8] == JOYPORT_ID_JOYSTICK) {
            joyport_display[9] = status;
        }
        if (id == JOYPORT_ID_JOY10 && joy_port[9] == JOYPORT_ID_JOYSTICK) {
            joyport_display[10] = status;
        }
    } else {
        for (i = 0; i < 10; i++) {
            if (id == joy_port[i]) {
                all = 0;
            }
        }

        if (!all) {
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

        if (id == joy_port[5]) {
            joyport_display[6] = status;
        }

        if (id == joy_port[6]) {
            joyport_display[7] = status;
        }

        if (id == joy_port[7]) {
            joyport_display[8] = status;
        }

        if (id == joy_port[8]) {
            joyport_display[9] = status;
        }

        if (id == joy_port[9]) {
            joyport_display[10] = status;
        }
    }
    ui_display_joyport(joyport_display);
}

char *joyport_get_port_name(int port)
{
    return port_props[port].name;
}

/* ------------------------------------------------------------------------- */

char *joystick_adapter_get_name(void)
{
    return joystick_adapter_name;
}

uint8_t joystick_adapter_get_id(void)
{
    return joystick_adapter_id;
}

int joystick_adapter_is_snes(void)
{
    return joystick_adapter_is_snes_adapter(joystick_adapter_get_id());
}

/* returns 1 on success */
uint8_t joystick_adapter_activate(uint8_t id, char *name)
{
    int i;

    if (joystick_adapter_id) {
        if (id == joystick_adapter_id) {
            joystick_adapter_name = name;
            return 1;
        } else {
            ui_error("Joystick adapter %s already active", joystick_adapter_name);
            return 0;
        }
    }

    joystick_adapter_id = id;
    joystick_adapter_name = name;

    /* if the joystick adapter is a SNES adapter, make sure the devices on ports 3-10 are 'none' or 'joystick' only */
    if (joystick_adapter_is_snes_adapter(id)) {
        for (i = JOYPORT_3; i < JOYPORT_MAX_PORTS; i++) {
            if (joy_port[i] != JOYPORT_ID_NONE && joy_port[i] != JOYPORT_ID_JOYSTICK) {
                /* set device to joystick if it was not 'none' or 'joystick' before */
                joyport_set_device(i, JOYPORT_ID_JOYSTICK);
            }
        }
    }
    return 1;
}

void joystick_adapter_deactivate(void)
{
    int i;

    joystick_adapter_id = JOYSTICK_ADAPTER_ID_NONE;
    joystick_adapter_name = NULL;
    joystick_adapter_ports = 0;
    joystick_output_check_function = NULL;

    /* deactivate all extra joy ports */
    for (i = JOYPORT_3; i < JOYPORT_MAX_PORTS; i++) {
        port_props[i].active = 0;
    }

    /* turn plus4 sidcard joy back on if it was still on */
    if (joystick_adapter_additional_ports) {
        port_props[JOYPORT_PLUS4_SIDCART].active = 1;
    }
}

void joystick_adapter_set_ports(int ports)
{
    int i;

    joystick_adapter_ports = ports;

    /* activate the extra joy ports */
    for (i = 0; i < ports; i++) {
        port_props[JOYPORT_3 + i].active = 1;
    }
}

int joystick_adapter_get_ports(void)
{
    return joystick_adapter_ports + joystick_adapter_additional_ports;
}

void joystick_adapter_set_add_ports(int ports)
{
    joystick_adapter_additional_ports = ports;

    port_props[JOYPORT_PLUS4_SIDCART].active = ports;
}


void joystick_adapter_set_output_check_function(int (*function)(int port, uint8_t bits))
{
    joystick_output_check_function = function;
}

/* ------------------------------------------------------------------------- */

static joyport_mapping_t joystick_mapping[JOYPORT_MAX_PORTS] = { 0 };
static int joystick_mapped[JOYPORT_MAX_PORTS] = { 0 };

int joyport_port_is_active(int port)
{
    return port_props[port].active;
}

void joyport_set_mapping(joyport_mapping_t *map, int port)
{
    joystick_mapping[port].name = map->name;
    joystick_mapping[port].pin0 = map->pin0;
    joystick_mapping[port].pin1 = map->pin1;
    joystick_mapping[port].pin2 = map->pin2;
    joystick_mapping[port].pin3 = map->pin3;
    joystick_mapping[port].pin4 = map->pin4;
    joystick_mapping[port].pin5 = map->pin5;
    joystick_mapping[port].pin6 = map->pin6;
    joystick_mapping[port].pin7 = map->pin7;
    joystick_mapping[port].pin8 = map->pin8;
    joystick_mapping[port].pin9 = map->pin9;
    joystick_mapping[port].pin10 = map->pin10;
    joystick_mapping[port].pin11 = map->pin11;
    joystick_mapping[port].pot1 = map->pot1;
    joystick_mapping[port].pot2 = map->pot2;
    joystick_mapped[port] = 1;
}

void joyport_clear_mapping(int port)
{
    joystick_mapping[port].name = NULL;
    joystick_mapping[port].pin0 = NULL;
    joystick_mapping[port].pin1 = NULL;
    joystick_mapping[port].pin2 = NULL;
    joystick_mapping[port].pin3 = NULL;
    joystick_mapping[port].pin4 = NULL;
    joystick_mapping[port].pin5 = NULL;
    joystick_mapping[port].pin6 = NULL;
    joystick_mapping[port].pin7 = NULL;
    joystick_mapping[port].pin8 = NULL;
    joystick_mapping[port].pin9 = NULL;
    joystick_mapping[port].pin10 = NULL;
    joystick_mapping[port].pin11 = NULL;
    joystick_mapping[port].pot1 = NULL;
    joystick_mapping[port].pot2 = NULL;
    joystick_mapped[port] = 0;
}

int joyport_has_mapping(int port)
{
    if (joyport_port_is_active(port)) {
        return joystick_mapped[port];
    }
    return 0;
}

static joyport_map_t pinmap[JOYPORT_MAX_PINS + 1] = { 0 };
static joyport_map_t potmap[JOYPORT_MAX_POTS + 1] = { 0 };
static joyport_map_desc_t joyport_map = { 0 };

joyport_map_desc_t *joyport_get_mapping(int port)
{
    int i = 0;
    int j = 0;
    int pots_used = 0;

    if (!joystick_mapped[port]) {
        return NULL;
    }

    /* build joystick mapping description */
    joyport_map.name = joystick_mapping[port].name;

    if (joystick_mapping[port].pin0 != NULL) {
        pinmap[i].name = joystick_mapping[port].pin0;
        pinmap[i].pin = j;
        i++;
    }
    j++;

    if (joystick_mapping[port].pin1 != NULL) {
        pinmap[i].name = joystick_mapping[port].pin1;
        pinmap[i].pin = j;
        i++;
    }
    j++;

    if (joystick_mapping[port].pin2 != NULL) {
        pinmap[i].name = joystick_mapping[port].pin2;
        pinmap[i].pin = j;
        i++;
    }
    j++;

    if (joystick_mapping[port].pin3 != NULL) {
        pinmap[i].name = joystick_mapping[port].pin3;
        pinmap[i].pin = j;
        i++;
    }
    j++;

    if (joystick_mapping[port].pin4 != NULL) {
        pinmap[i].name = joystick_mapping[port].pin4;
        pinmap[i].pin = j;
        i++;
    }
    j++;

    if (joystick_mapping[port].pin5 != NULL) {
        pinmap[i].name = joystick_mapping[port].pin5;
        pinmap[i].pin = j;
        i++;
    }
    j++;

    if (joystick_mapping[port].pin6 != NULL) {
        pinmap[i].name = joystick_mapping[port].pin6;
        pinmap[i].pin = j;
        i++;
    }
    j++;

    if (joystick_mapping[port].pin7 != NULL) {
        pinmap[i].name = joystick_mapping[port].pin7;
        pinmap[i].pin = j;
        i++;
    }
    j++;

    if (joystick_mapping[port].pin8 != NULL) {
        pinmap[i].name = joystick_mapping[port].pin8;
        pinmap[i].pin = j;
        i++;
    }
    j++;

    if (joystick_mapping[port].pin9 != NULL) {
        pinmap[i].name = joystick_mapping[port].pin9;
        pinmap[i].pin = j;
        i++;
    }
    j++;

    if (joystick_mapping[port].pin10 != NULL) {
        pinmap[i].name = joystick_mapping[port].pin10;
        pinmap[i].pin = j;
        i++;
    }
    j++;

    if (joystick_mapping[port].pin11 != NULL) {
        pinmap[i].name = joystick_mapping[port].pin11;
        pinmap[i].pin = j;
        i++;
    }

    pinmap[i].name = NULL;
    pinmap[i].pin = 0;

    i = 0;
    j = 0;

    if (joystick_mapping[port].pot1 != NULL) {
        potmap[i].name = joystick_mapping[port].pot1;
        potmap[i].pin = j;
        pots_used++;
        i++;
    }
    j++;

    if (joystick_mapping[port].pot2 != NULL) {
        potmap[i].name = joystick_mapping[port].pot2;
        potmap[i].pin = j;
        i++;
        pots_used++;
    }

    potmap[i].name = NULL;
    potmap[i].pin = 0;

    joyport_map.pinmap = pinmap;
    if (pots_used) {
        joyport_map.potmap = potmap;
    } else {
        joyport_map.potmap = NULL;
    }
    return &joyport_map;
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

static const resource_int_t resources_int_port6[] = {
    { "JoyPort6Device", JOYPORT_ID_JOYSTICK, RES_EVENT_NO, NULL,
      &joy_port[JOYPORT_6], set_joyport_device, (void *)JOYPORT_6 },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_int_port7[] = {
    { "JoyPort7Device", JOYPORT_ID_JOYSTICK, RES_EVENT_NO, NULL,
      &joy_port[JOYPORT_7], set_joyport_device, (void *)JOYPORT_7 },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_int_port8[] = {
    { "JoyPort8Device", JOYPORT_ID_JOYSTICK, RES_EVENT_NO, NULL,
      &joy_port[JOYPORT_8], set_joyport_device, (void *)JOYPORT_8 },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_int_port9[] = {
    { "JoyPort9Device", JOYPORT_ID_JOYSTICK, RES_EVENT_NO, NULL,
      &joy_port[JOYPORT_9], set_joyport_device, (void *)JOYPORT_9 },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_int_port10[] = {
    { "JoyPort10Device", JOYPORT_ID_JOYSTICK, RES_EVENT_NO, NULL,
      &joy_port[JOYPORT_10], set_joyport_device, (void *)JOYPORT_10 },
    RESOURCE_INT_LIST_END
};

int joyport_resources_init(void)
{
    int i;

    memset(joyport_device, 0, sizeof(joyport_device));
    joyport_device[0].name = "None";
    joyport_device[0].is_lp = JOYPORT_IS_NOT_LIGHTPEN;
    joyport_device[0].joystick_adapter_id = JOYSTICK_ADAPTER_ID_NONE;
    for (i = 0; i < JOYPORT_MAX_PORTS; ++i) {
        joy_port[i] = JOYPORT_ID_NONE;
    }

    if (port_props[JOYPORT_10].name) {
        if (resources_register_int(resources_int_port10) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_9].name) {
        if (resources_register_int(resources_int_port9) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_8].name) {
        if (resources_register_int(resources_int_port8) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_7].name) {
        if (resources_register_int(resources_int_port7) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_6].name) {
        if (resources_register_int(resources_int_port6) < 0) {
            return -1;
        }
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

static const struct joyport_opt_s id_match[] = {
    { "none",            JOYPORT_ID_NONE },
    { "joy",             JOYPORT_ID_JOYSTICK },
    { "joystick",        JOYPORT_ID_JOYSTICK },
    { "paddles",         JOYPORT_ID_PADDLES },
    { "1351",            JOYPORT_ID_MOUSE_1351 },
    { "1351mouse",       JOYPORT_ID_MOUSE_1351 },
    { "neos",            JOYPORT_ID_MOUSE_NEOS },
    { "neosmouse",       JOYPORT_ID_MOUSE_NEOS },
    { "amiga",           JOYPORT_ID_MOUSE_AMIGA },
    { "amigamouse",      JOYPORT_ID_MOUSE_AMIGA },
    { "cx22",            JOYPORT_ID_MOUSE_CX22 },
    { "cx22mouse",       JOYPORT_ID_MOUSE_CX22 },
    { "st",              JOYPORT_ID_MOUSE_ST },
    { "atarist",         JOYPORT_ID_MOUSE_ST },
    { "stmouse",         JOYPORT_ID_MOUSE_ST },
    { "ataristmouse",    JOYPORT_ID_MOUSE_ST },
    { "smart",           JOYPORT_ID_MOUSE_SMART },
    { "smartmouse",      JOYPORT_ID_MOUSE_SMART },
    { "micromys",        JOYPORT_ID_MOUSE_MICROMYS },
    { "micromysmouse",   JOYPORT_ID_MOUSE_MICROMYS },
    { "koala",           JOYPORT_ID_KOALAPAD },
    { "koalapad",        JOYPORT_ID_KOALAPAD },
    { "lpup",            JOYPORT_ID_LIGHTPEN_U },
    { "lightpenup",      JOYPORT_ID_LIGHTPEN_U },
    { "lpleft",          JOYPORT_ID_LIGHTPEN_L },
    { "lightpenleft",    JOYPORT_ID_LIGHTPEN_L },
    { "lpdatel",         JOYPORT_ID_LIGHTPEN_DATEL },
    { "lightpendatel",   JOYPORT_ID_LIGHTPEN_DATEL },
    { "datellightpen",   JOYPORT_ID_LIGHTPEN_DATEL },
    { "magnum",          JOYPORT_ID_LIGHTGUN_Y },
    { "stack",           JOYPORT_ID_LIGHTGUN_L },
    { "slr",             JOYPORT_ID_LIGHTGUN_L },
    { "lpinkwell",       JOYPORT_ID_LIGHTPEN_INKWELL },
    { "lightpeninkwell", JOYPORT_ID_LIGHTPEN_INKWELL },
    { "inkwelllightpen", JOYPORT_ID_LIGHTPEN_INKWELL },
#ifdef JOYPORT_EXPERIMENTAL_DEVICES
    { "gunstick",         JOYPORT_ID_LIGHTGUN_GUNSTICK },
#endif
    { "2bitsampler",      JOYPORT_ID_SAMPLER_2BIT },
    { "4bitsampler",      JOYPORT_ID_SAMPLER_4BIT },
    { "bbrtc",            JOYPORT_ID_BBRTC },
    { "paperclip64",      JOYPORT_ID_PAPERCLIP64 },
    { "paperclip",        JOYPORT_ID_PAPERCLIP64 },
    { "pc64",             JOYPORT_ID_PAPERCLIP64 },
    { "coplin",           JOYPORT_ID_COPLIN_KEYPAD },
    { "coplinkp",         JOYPORT_ID_COPLIN_KEYPAD },
    { "coplinkeypad",     JOYPORT_ID_COPLIN_KEYPAD },
    { "cx85",             JOYPORT_ID_CX85_KEYPAD },
    { "cx85kp",           JOYPORT_ID_CX85_KEYPAD },
    { "cx85keypad",       JOYPORT_ID_CX85_KEYPAD },
    { "rushware",         JOYPORT_ID_RUSHWARE_KEYPAD },
    { "rushwarekp",       JOYPORT_ID_RUSHWARE_KEYPAD },
    { "rushwarekeypad",   JOYPORT_ID_RUSHWARE_KEYPAD },
    { "cx21",             JOYPORT_ID_CX21_KEYPAD },
    { "cx21kp",           JOYPORT_ID_CX21_KEYPAD },
    { "cx21keypad",       JOYPORT_ID_CX21_KEYPAD },
    { "script64",         JOYPORT_ID_SCRIPT64_DONGLE },
    { "script64dongle",   JOYPORT_ID_SCRIPT64_DONGLE },
    { "vizawrite64",       JOYPORT_ID_VIZAWRITE64_DONGLE },
    { "vizawrite64dongle", JOYPORT_ID_VIZAWRITE64_DONGLE },
    { "waasoft",           JOYPORT_ID_WAASOFT_DONGLE },
    { "waasoftdongle",     JOYPORT_ID_WAASOFT_DONGLE },
    { "trapthem",          JOYPORT_ID_TRAPTHEM_SNESPAD },
    { "trapthemsnes",      JOYPORT_ID_TRAPTHEM_SNESPAD },
    { "trapthemsnespad",   JOYPORT_ID_TRAPTHEM_SNESPAD },
    { "ninja",             JOYPORT_ID_NINJA_SNESPAD },
    { "ninjasnes",         JOYPORT_ID_NINJA_SNESPAD },
    { "ninjasnespad",      JOYPORT_ID_NINJA_SNESPAD },
    { "spaceballs",        JOYPORT_ID_SPACEBALLS },
    { "inception",         JOYPORT_ID_INCEPTION },
    { "multijoy",          JOYPORT_ID_MULTIJOY_JOYSTICKS },
    { "protopad",          JOYPORT_ID_PROTOPAD },
    { "io",                JOYPORT_ID_IO_SIMULATION },
    { "iosim",             JOYPORT_ID_IO_SIMULATION },
    { "iosimulation",      JOYPORT_ID_IO_SIMULATION },
    { "mfjoy",             JOYPORT_ID_MF_JOYSTICK },
    { "mfjoystick",        JOYPORT_ID_MF_JOYSTICK },
    { NULL, -1 }
};

static int is_a_number(const char *str)
{
    size_t i;
    size_t len = strlen(str);

    for (i = 0; i < len; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

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
        if (!is_a_number(param)) {
            return -1;
        }
        temp = atoi(param);
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
    joyport_desc_t *devices = joyport_get_valid_devices(port, 0);

    tmp1 = lib_msprintf("Set %s device (0: None", port_props[port].name);

    for (i = 1; devices[i].name; ++i) {
        sprintf(number, "%d", devices[i].id);
        tmp2 = util_concat(tmp1, ", ", number, ": ", devices[i].name, NULL);
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
    { "-controlport1device", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS | CMDLINE_ATTRIB_DYNAMIC_DESCRIPTION,
      set_joyport_cmdline_device, (void *)JOYPORT_1, NULL, NULL,
      "Device", NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t cmdline_options_port2[] =
{
    { "-controlport2device", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS | CMDLINE_ATTRIB_DYNAMIC_DESCRIPTION,
      set_joyport_cmdline_device, (void *)JOYPORT_2, NULL, NULL,
      "Device", NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t cmdline_options_port3[] =
{
    { "-controlport3device", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS | CMDLINE_ATTRIB_DYNAMIC_DESCRIPTION,
      set_joyport_cmdline_device, (void *)JOYPORT_3, NULL, NULL,
      "Device", NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t cmdline_options_port4[] =
{
    { "-controlport4device", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS | CMDLINE_ATTRIB_DYNAMIC_DESCRIPTION,
      set_joyport_cmdline_device, (void *)JOYPORT_4, NULL, NULL,
      "Device", NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t cmdline_options_port5[] =
{
    { "-controlport5device", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS | CMDLINE_ATTRIB_DYNAMIC_DESCRIPTION,
      set_joyport_cmdline_device, (void *)JOYPORT_5, NULL, NULL,
      "Device", NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t cmdline_options_port6[] =
{
    { "-controlport6device", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS | CMDLINE_ATTRIB_DYNAMIC_DESCRIPTION,
      set_joyport_cmdline_device, (void *)JOYPORT_6, NULL, NULL,
      "Device", NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t cmdline_options_port7[] =
{
    { "-controlport7device", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS | CMDLINE_ATTRIB_DYNAMIC_DESCRIPTION,
      set_joyport_cmdline_device, (void *)JOYPORT_7, NULL, NULL,
      "Device", NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t cmdline_options_port8[] =
{
    { "-controlport8device", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS | CMDLINE_ATTRIB_DYNAMIC_DESCRIPTION,
      set_joyport_cmdline_device, (void *)JOYPORT_8, NULL, NULL,
      "Device", NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t cmdline_options_port9[] =
{
    { "-controlport9device", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS | CMDLINE_ATTRIB_DYNAMIC_DESCRIPTION,
      set_joyport_cmdline_device, (void *)JOYPORT_9, NULL, NULL,
      "Device", NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t cmdline_options_port10[] =
{
    { "-controlport10device", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS | CMDLINE_ATTRIB_DYNAMIC_DESCRIPTION,
      set_joyport_cmdline_device, (void *)JOYPORT_10, NULL, NULL,
      "Device", NULL },
    CMDLINE_LIST_END
};

int joyport_cmdline_options_init(void)
{
    union char_func cf;

    if (port_props[JOYPORT_1].name) {
        cf.f = build_joyport_string;
        cmdline_options_port1[0].description = cf.c;
        cmdline_options_port1[0].attributes |= (JOYPORT_1 << 8);
        if (cmdline_register_options(cmdline_options_port1) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_2].name) {
        cf.f = build_joyport_string;
        cmdline_options_port2[0].description = cf.c;
        cmdline_options_port2[0].attributes |= (JOYPORT_2 << 8);
        if (cmdline_register_options(cmdline_options_port2) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_3].name) {
        cf.f = build_joyport_string;
        cmdline_options_port3[0].description = cf.c;
        cmdline_options_port3[0].attributes |= (JOYPORT_3 << 8);
        if (cmdline_register_options(cmdline_options_port3) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_4].name) {
        cf.f = build_joyport_string;
        cmdline_options_port4[0].description = cf.c;
        cmdline_options_port4[0].attributes |= (JOYPORT_4 << 8);
        if (cmdline_register_options(cmdline_options_port4) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_5].name) {
        cf.f = build_joyport_string;
        cmdline_options_port5[0].description = cf.c;
        cmdline_options_port5[0].attributes |= (JOYPORT_5 << 8);
        if (cmdline_register_options(cmdline_options_port5) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_6].name) {
        cf.f = build_joyport_string;
        cmdline_options_port6[0].description = cf.c;
        cmdline_options_port6[0].attributes |= (JOYPORT_6 << 8);
        if (cmdline_register_options(cmdline_options_port6) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_7].name) {
        cf.f = build_joyport_string;
        cmdline_options_port7[0].description = cf.c;
        cmdline_options_port7[0].attributes |= (JOYPORT_7 << 8);
        if (cmdline_register_options(cmdline_options_port7) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_8].name) {
        cf.f = build_joyport_string;
        cmdline_options_port8[0].description = cf.c;
        cmdline_options_port8[0].attributes |= (JOYPORT_8 << 8);
        if (cmdline_register_options(cmdline_options_port8) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_9].name) {
        cf.f = build_joyport_string;
        cmdline_options_port9[0].description = cf.c;
        cmdline_options_port9[0].attributes |= (JOYPORT_9 << 8);
        if (cmdline_register_options(cmdline_options_port9) < 0) {
            return -1;
        }
    }

    if (port_props[JOYPORT_10].name) {
        cf.f = build_joyport_string;
        cmdline_options_port10[0].description = cf.c;
        cmdline_options_port10[0].attributes |= (JOYPORT_10 << 8);
        if (cmdline_register_options(cmdline_options_port10) < 0) {
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
    if (SMW_B(m, (uint8_t)joy_port[port]) < 0) {
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
    uint8_t major_version, minor_version;
    snapshot_module_t *m;
    int temp_joy_port;
    char snapshot_name[16];

    sprintf(snapshot_name, "JOYPORT%d", port);

    m = snapshot_module_open(s, snapshot_name, &major_version, &minor_version);
    if (m == NULL) {
        return -1;
    }

    if (!snapshot_version_is_equal(major_version, minor_version, DUMP_VER_MAJOR, DUMP_VER_MINOR)) {
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
