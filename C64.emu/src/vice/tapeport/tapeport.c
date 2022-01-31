/*
 * tapeport.c - tapeport handling.
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "cmdline.h"
#include "cp-clockf83.h"
#include "datasette.h"
#include "dtl-basic-dongle.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "sense-dongle.h"
#include "snapshot.h"
#include "tapecart.h"
#include "tapeport.h"
#include "uiapi.h"
#include "util.h"

/* flag indicating if the tapeport exists on the current emulated model */
static int tapeport_active = 1;

/* current tapeport devices */
static int tapeport_current_device[TAPEPORT_MAX_PORTS] = { TAPEPORT_DEVICE_NONE, TAPEPORT_DEVICE_NONE };

static tapeport_device_t tapeport_device[TAPEPORT_MAX_DEVICES] = {0};

static int tapeport_ports = 0;

typedef struct type2text_s {
    int type;
    const char *text;
} type2text_t;

static type2text_t device_type_desc[] = {
    { TAPEPORT_DEVICE_TYPE_NONE, "None" },
    { TAPEPORT_DEVICE_TYPE_TAPE, "Tape" },
    { TAPEPORT_DEVICE_TYPE_STORAGE, "Storage" },
    { TAPEPORT_DEVICE_TYPE_RTC, "Real-time clock" },
#ifdef TAPEPORT_EXPERIMENTAL_DEVICES
    { TAPEPORT_DEVICE_TYPE_HARNESS, "Diagnostic harness" },
#endif
    { TAPEPORT_DEVICE_TYPE_DONGLE, "Dongle" },
    { -1, NULL }
};

static const char *tapeport_type2text(int type)
{
    int i;
    const char *retval = NULL;

    for (i = 0; device_type_desc[i].type != -1; ++i) {
        if (device_type_desc[i].type == type) {
            retval = device_type_desc[i].text;
        }
    }
    return retval;
}

/* ---------------------------------------------------------------------------------------------------------- */

/* register a device to be used in the tapeport system */
int tapeport_device_register(int id, tapeport_device_t *device)
{
    if (id < 1 || id > TAPEPORT_MAX_DEVICES) {
        return -1;
    }

    tapeport_device[id].name = device->name;
    tapeport_device[id].device_type = device->device_type;
    tapeport_device[id].machine_mask = device->machine_mask;
    tapeport_device[id].port_mask = device->port_mask;
    tapeport_device[id].enable = device->enable;
    tapeport_device[id].powerup = device->powerup;
    tapeport_device[id].shutdown = device->shutdown;
    tapeport_device[id].set_motor = device->set_motor;
    tapeport_device[id].toggle_write_bit = device->toggle_write_bit;
    tapeport_device[id].set_sense_out = device->set_sense_out;
    tapeport_device[id].set_read_out = device->set_read_out;
    tapeport_device[id].write_snapshot = device->write_snapshot;
    tapeport_device[id].read_snapshot = device->read_snapshot;

    return 0;
}

/* check if the selected device can be attached to the indicated port */
static int tapeport_check_valid_device(int port, int id)
{
    if (id == TAPEPORT_DEVICE_NONE) {
        return 1;
    }

    /* check the machine mask */
    if ((tapeport_device[id].machine_mask & machine_class) == 0) {
        return 0;
    }

    /* check the port mask */
    if ((tapeport_device[id].port_mask & (1 << port)) == 0) {
        return 0;
    }
    return 1;
}

/* attach device 'id' to a tapeport */
static int tapeport_set_device(int port, int id)
{
    /* 1st some sanity checks */
    if (id < TAPEPORT_DEVICE_NONE || id >= TAPEPORT_MAX_DEVICES) {
        return -1;
    }
    if (port >= TAPEPORT_MAX_PORTS) {
        return -1;
    }

    /* Nothing changes */
    if (id == tapeport_current_device[port]) {
        return 0;
    }

    /* check if id is registered */
    if (id != TAPEPORT_DEVICE_NONE && !tapeport_device[id].name) {
        ui_error("Selected tapeport device %d is not registered", id);
        return -1;
    }

    if (!tapeport_check_valid_device(port, id)) {
        ui_error("Selected tapoport device %d is not valid for port %d", id, port);
        return -1;
    }

    /* all checks done, now disable the current device and enable the new device */
    if (tapeport_device[tapeport_current_device[port]].enable) {
        tapeport_device[tapeport_current_device[port]].enable(port, 0);
    }

    if (tapeport_device[id].enable) {
        tapeport_device[id].enable(port, 1);
    }

    tapeport_current_device[port] = id;

    return 0;
}

static int tapeport_valid_devices_compare_names(const void* a, const void* b)
{
    const tapeport_desc_t *arg1 = (const tapeport_desc_t*)a;
    const tapeport_desc_t *arg2 = (const tapeport_desc_t*)b;

    if (arg1->device_type != arg2->device_type) {
        if (arg1->device_type < arg2->device_type) {
            return -1;
        } else {
            return 1;
        }
    }

    return strcmp(arg1->name, arg2->name);
}

int tapeport_valid_port(int port)
{
    if (port < 0 || port >= tapeport_ports) {
        return 0;
    }
    return 1;
}

tapeport_desc_t *tapeport_get_valid_devices(int port, int sort)
{
    tapeport_desc_t *retval = NULL;
    int i;
    int j = 0;
    int valid = 0;

    for (i = 0; i < TAPEPORT_MAX_DEVICES; ++i) {
        if (tapeport_device[i].name) {
            if (tapeport_check_valid_device(port, i)) {
               ++valid;
            }
        }
    }

    retval = lib_malloc(((size_t)valid + 1) * sizeof(tapeport_desc_t));

    for (i = 0; i < TAPEPORT_MAX_DEVICES; ++i) {
        if (tapeport_device[i].name) {
            if (tapeport_check_valid_device(port, i)) {
                retval[j].name = tapeport_device[i].name;
                retval[j].id = i;
                retval[j].device_type = tapeport_device[i].device_type;
                ++j;
            }
        }
    }

    retval[j].name = NULL;

    if (sort) {
        qsort(retval, valid, sizeof(tapeport_desc_t), tapeport_valid_devices_compare_names);
    }
    return retval;
}

const char *tapeport_get_device_type_desc(int type)
{
    return tapeport_type2text(type);
}

/* ---------------------------------------------------------------------------------------------------------- */

void tapeport_set_motor(int port, int flag)
{
    /* use new tapeport system if the device has been registered */
    if (tapeport_current_device[port] != TAPEPORT_DEVICE_NONE) {
        if (tapeport_device[tapeport_current_device[port]].name) {
            if (tapeport_device[tapeport_current_device[port]].set_motor) {
                tapeport_device[tapeport_current_device[port]].set_motor(port, flag);
            }
        }
    }
}

void tapeport_toggle_write_bit(int port, int write_bit)
{
    /* use new tapeport system if the device has been registered */
    if (tapeport_current_device[port] != TAPEPORT_DEVICE_NONE) {
        if (tapeport_device[tapeport_current_device[port]].name) {
            if (tapeport_device[tapeport_current_device[port]].toggle_write_bit) {
                tapeport_device[tapeport_current_device[port]].toggle_write_bit(port, write_bit);
            }
        }
    }
}

void tapeport_set_sense_out(int port, int sense)
{
    /* use new tapeport system if the device has been registered */
    if (tapeport_current_device[port] != TAPEPORT_DEVICE_NONE) {
        if (tapeport_device[tapeport_current_device[port]].name) {
            if (tapeport_device[tapeport_current_device[port]].set_sense_out) {
                tapeport_device[tapeport_current_device[port]].set_sense_out(port, sense);
            }
        }
    }
}

void tapeport_powerup(void)
{
    int i;

    if (tapeport_active) {
        for (i = 0; i < TAPEPORT_MAX_PORTS; i++) {
            /* use new tapeport system if the device has been registered */
            if (tapeport_current_device[i] != TAPEPORT_DEVICE_NONE) {
                if (tapeport_device[tapeport_current_device[i]].name) {
                    if (tapeport_device[tapeport_current_device[i]].powerup) {
                        tapeport_device[tapeport_current_device[i]].powerup(i);
                    }
                }
            }
        }
    }
}

void tapeport_trigger_flux_change(unsigned int on, int port)
{
    machine_trigger_flux_change(port, on);
}

void tapeport_set_tape_sense(int sense, int port)
{
    machine_set_tape_sense(port, sense);
}

void tapeport_set_write_in(int val, int port)
{
    machine_set_tape_write_in(port, val);
}

void tapeport_set_motor_in(int val, int port)
{
    machine_set_tape_motor_in(port, val);
}

/* ---------------------------------------------------------------------------------------------------------- */

static int set_tapeport_device(int val, void *param)
{
    int port = vice_ptr_to_int(param);

    return tapeport_set_device(port, val);
}

static const resource_int_t resources_int_port1[] = {
    { "TapePort1Device", TAPEPORT_DEVICE_DATASETTE, RES_EVENT_NO, NULL,
      &tapeport_current_device[TAPEPORT_PORT_1], set_tapeport_device, (void *)TAPEPORT_PORT_1 },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_int_port2[] = {
    { "TapePort2Device", TAPEPORT_DEVICE_DATASETTE, RES_EVENT_NO, NULL,
      &tapeport_current_device[TAPEPORT_PORT_2], set_tapeport_device, (void *)TAPEPORT_PORT_2 },
    RESOURCE_INT_LIST_END
};

static int tapeport_device_resources_init(int amount)
{
    datasette_resources_init(amount);

    if (tapertc_resources_init(amount) < 0) {
        return -1;
    }

    if (sense_dongle_resources_init(amount) < 0) {
        return -1;
    }

    /* Only use tapecart device and dtl basic dongle on c64/c128 */
    if (machine_class == VICE_MACHINE_C64 || machine_class == VICE_MACHINE_C128 || machine_class == VICE_MACHINE_C64SC) {
        if (dtlbasic_dongle_resources_init(amount) < 0) {
            return -1;
        }

        if (tapecart_resources_init(amount) < 0) {
            return -1;
        }
    }

#ifdef TAPEPORT_EXPERIMENTAL_DEVICES
    if (tape_diag_586220_harness_resources_init(amount) < 0) {
        return -1;
    }
#endif

    return 0;
}

int tapeport_resources_init(int amount)
{
    memset(tapeport_device, 0, sizeof(tapeport_device));
    tapeport_device[0].name = "None";
    tapeport_ports = amount;

    if (tapeport_ports >= 1) {
        if (resources_register_int(resources_int_port1) < 0) {
            return -1;
        }
    }

    if (tapeport_ports >= 2) {
        if (resources_register_int(resources_int_port2) < 0) {
            return -1;
        }
    }

    return tapeport_device_resources_init(amount);
}

void tapeport_resources_shutdown(void)
{
    int i;

    if (tapeport_active) {
        for (i = 0; i < TAPEPORT_MAX_DEVICES; i++) {
            if (tapeport_device[i].name) {
                if (tapeport_device[i].shutdown) {
                    tapeport_device[i].shutdown();
                }
            }
        }
    }
}

/* ------------------------------------------------------------------------- */

struct tapeport_opt_s {
    const char *name;
    int id;
};

static const struct tapeport_opt_s id_match[] = {
    { "none",           TAPEPORT_DEVICE_NONE },
    { "datasette",      TAPEPORT_DEVICE_DATASETTE },
    { "casette",        TAPEPORT_DEVICE_DATASETTE },
    { "tape",           TAPEPORT_DEVICE_DATASETTE },
    { "tapecart",       TAPEPORT_DEVICE_TAPECART },
#ifdef TAPEPORT_EXPERIMENTAL_DEVICES
    { "harness",        TAPEPORT_DEVICE_TAPE_DIAG_586220_HARNESS },
#endif
    { "rtc",            TAPEPORT_DEVICE_CP_CLOCK_F83 },
    { "sensedongle",    TAPEPORT_DEVICE_SENSE_DONGLE },
    { "tapedongle",     TAPEPORT_DEVICE_SENSE_DONGLE },
    { "playdongle",     TAPEPORT_DEVICE_SENSE_DONGLE },
    { "dtl",            TAPEPORT_DEVICE_DTL_BASIC_DONGLE },
    { "dtldongle",      TAPEPORT_DEVICE_DTL_BASIC_DONGLE },
    { "dtlbasic",       TAPEPORT_DEVICE_DTL_BASIC_DONGLE },
    { "dtlbasicdongle", TAPEPORT_DEVICE_DTL_BASIC_DONGLE },
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

static int set_tapeport_cmdline_device(const char *param, void *extra_param)
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

    return set_tapeport_device(temp, int_to_void_ptr(port));
}

/* ------------------------------------------------------------------------- */

static char *build_tapeport_string(int port)
{
    int i = 0;
    char *tmp1;
    char *tmp2;
    char number[4];
    tapeport_desc_t *devices = tapeport_get_valid_devices(port, 0);

    tmp1 = lib_msprintf("Set Tapeport %d device (0: None", port);

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
    { "-tapeport1device", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS | CMDLINE_ATTRIB_DYNAMIC_DESCRIPTION,
      set_tapeport_cmdline_device, (void *)TAPEPORT_PORT_1, NULL, NULL,
      "Device", NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t cmdline_options_port2[] =
{
    { "-tapeport2device", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS | CMDLINE_ATTRIB_DYNAMIC_DESCRIPTION,
      set_tapeport_cmdline_device, (void *)TAPEPORT_PORT_2, NULL, NULL,
      "Device", NULL },
    CMDLINE_LIST_END
};

static int tapeport_devices_cmdline_options_init(void)
{
    if (tapertc_cmdline_options_init() < 0) {
        return -1;
    }

    /* Only use tapecart device on c64/c128 */
    if (machine_class == VICE_MACHINE_C64 || machine_class == VICE_MACHINE_C128 || machine_class == VICE_MACHINE_C64SC) {
        if (tapecart_cmdline_options_init() < 0) {
            return -1;
        }
    }

    return datasette_cmdline_options_init();
}

int tapeport_cmdline_options_init(void)
{
    union char_func cf;

    if (tapeport_ports >= 1) {
        cf.f = build_tapeport_string;
        cmdline_options_port1[0].description = cf.c;
        cmdline_options_port1[0].attributes |= (TAPEPORT_PORT_1 << 8);
        if (cmdline_register_options(cmdline_options_port1) < 0) {
            return -1;
        }
    }

    if (tapeport_ports >= 2) {
        cf.f = build_tapeport_string;
        cmdline_options_port2[0].description = cf.c;
        cmdline_options_port2[0].attributes |= (TAPEPORT_PORT_2 << 8);
        if (cmdline_register_options(cmdline_options_port2) < 0) {
            return -1;
        }
    }

    return tapeport_devices_cmdline_options_init();
}

void tapeport_enable(int val)
{
    tapeport_active = val ? 1 : 0;
}

/* ---------------------------------------------------------------------------------------------------------- */

/* TAPEPORT snapshot module format:

   type  | name               | description
   ----------------------------------------
   BYTE  | active             | tapeport active flag
   BYTE  | id1                | device id port 1
   BYTE  | id2                | device id port 2
 */

#define DUMP_VER_MAJOR   1
#define DUMP_VER_MINOR   0
static const char snap_module_name[] = "TAPEPORT";

int tapeport_snapshot_write_module(snapshot_t *s, int write_image)
{
    snapshot_module_t *m;
    int i;

    m = snapshot_module_create(s, snap_module_name, DUMP_VER_MAJOR, DUMP_VER_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    /* save tapeport active and current device id of port 1 */
    if (0
        || SMW_B(m, (uint8_t)tapeport_active) < 0
        || SMW_B(m, (uint8_t)tapeport_current_device[TAPEPORT_PORT_1]) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (tapeport_ports >= 2) {
        if (0
            || SMW_B(m, (uint8_t)tapeport_current_device[TAPEPORT_PORT_2]) < 0) {
            snapshot_module_close(m);
            return -1;
        }
    }

    snapshot_module_close(m);

    /* save seperate tapeport device modules */
    for (i = 0; i < tapeport_ports; i++) {
        switch (tapeport_current_device[i]) {
            case TAPEPORT_DEVICE_NONE:
                break;
            default:
                if (tapeport_device[tapeport_current_device[i]].write_snapshot) {
                    if (tapeport_device[tapeport_current_device[i]].write_snapshot(i, s, write_image) < 0) {
                        return -1;
                    }
                }
                break;
        }
    }

    return 0;
}

int tapeport_snapshot_read_module(struct snapshot_s *s)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;
    int tmp_tapeport_device[2];
    int i;

    m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);
    if (m == NULL) {
        return -1;
    }

    if (!snapshot_version_is_equal(major_version, minor_version, DUMP_VER_MAJOR, DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    /* load tapeport active and current device id for port 1 */
    if (0
        || SMR_B_INT(m, &tapeport_active) < 0
        || SMR_B_INT(m, &tmp_tapeport_device[TAPEPORT_PORT_1]) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (tapeport_ports >= 2) {
        if (0
            || SMR_B_INT(m, &tmp_tapeport_device[TAPEPORT_PORT_2]) < 0) {
            snapshot_module_close(m);
            return -1;
        }
    }

    snapshot_module_close(m);

    /* enable devices */
    tapeport_set_device(TAPEPORT_PORT_1, tmp_tapeport_device[TAPEPORT_PORT_1]);

    if (tapeport_ports >= 2) {
        tapeport_set_device(TAPEPORT_PORT_2, tmp_tapeport_device[TAPEPORT_PORT_2]);
    }

    /* load device snapshots */
    for (i = 0; i < tapeport_ports; i++) {
        switch (tapeport_current_device[i]) {
            case TAPEPORT_DEVICE_NONE:
                break;
            default:
                if (tapeport_device[tapeport_current_device[i]].read_snapshot) {
                    if (tapeport_device[tapeport_current_device[i]].read_snapshot(i, s) < 0) {
                        return -1;
                    }
                }
                break;
        }
    }

    return 0;
}
