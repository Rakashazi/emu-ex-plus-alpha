/*
 * interface-serial.c - Serial printer interface.
 *
 * Written by
 *  André Fachat <a.fachat@physik.tu-chemnitz.de>
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "driver-select.h"
#include "interface-serial.h"
#include "log.h"
#include "machine-bus.h"
#include "printer.h"
#include "resources.h"
#include "translate.h"
#include "types.h"

static int interface_serial_attach(unsigned int prnr);
static int interface_serial_detach(unsigned int prnr);

static log_t interface_serial_log = LOG_ERR;

/* ------------------------------------------------------------------------- */

static int printer_enabled[2];

static int set_printer_enabled(int flag, void *param)
{
    unsigned int prnr;

    if (flag != PRINTER_DEVICE_NONE
        && flag != PRINTER_DEVICE_FS
#ifdef HAVE_OPENCBM
        && flag != PRINTER_DEVICE_REAL
#endif
        ) {
        return -1;
    }

    prnr = vice_ptr_to_uint(param);

    if (printer_enabled[prnr] == PRINTER_DEVICE_FS
        && flag != PRINTER_DEVICE_FS) {
        if (interface_serial_detach(prnr) < 0) {
            return -1;
        }
    }
    if (flag == PRINTER_DEVICE_FS
        && printer_enabled[prnr] != PRINTER_DEVICE_FS) {
        if (interface_serial_attach(prnr) < 0) {
            return -1;
        }
    }

    if (printer_enabled[prnr] == PRINTER_DEVICE_REAL
        && flag != PRINTER_DEVICE_REAL) {
    }

    if (flag == PRINTER_DEVICE_REAL
        && printer_enabled[prnr] != PRINTER_DEVICE_REAL) {
    }

    printer_enabled[prnr] = flag;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "Printer4", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &printer_enabled[0], set_printer_enabled, (void *)0 },
    { "Printer5", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &printer_enabled[1], set_printer_enabled, (void *)1 },
    { NULL }
};

int interface_serial_init_resources(void)
{
    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] = {
    { "-device4", SET_RESOURCE, 1,
      NULL, NULL, "Printer4", (void *)PRINTER_DEVICE_FS,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_TYPE, IDCLS_SET_DEVICE_TYPE_4,
      NULL, NULL },
    { "-device5", SET_RESOURCE, 1,
      NULL, NULL, "Printer5", (void *)PRINTER_DEVICE_FS,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_TYPE, IDCLS_SET_DEVICE_TYPE_5,
      NULL, NULL },
    { NULL }
};

int interface_serial_init_cmdline_options(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

static unsigned int inuse[2];

static int open_pr(unsigned int prnr, const BYTE *name, unsigned int length,
                   unsigned int secondary)
{
    if (inuse[prnr]) {
        log_error(interface_serial_log,
                  "Open printer #%i while still open - ignoring.", prnr + 4);
        return 0;
    }

    if (driver_select_open(prnr, secondary) < 0) {
        log_error(interface_serial_log,
                  "Couldn't open device #%i.", prnr + 4);
        return -1;
    }

    inuse[prnr] = 1;

    return 0;
}

static int read_pr(unsigned int prnr, BYTE *byte, unsigned int secondary)
{
    return 0x80;
}

static int write_pr(unsigned int prnr, BYTE byte, unsigned int secondary)
{
    int err;

    if (!inuse[prnr]) {
        /* oh, well, we just assume an implicit open - "OPEN 1,4"
           just does not leave any trace on the serial bus */
        log_message(interface_serial_log,
                    "Auto-opening printer #%i.", prnr + 4);

        err = open_pr(prnr, NULL, 0, secondary);

        if (err < 0) {
            return err;
        }
    }

    return driver_select_putc(prnr, secondary, (BYTE)byte);
}

static int close_pr(unsigned int prnr, unsigned int secondary)
{
    if (!inuse[prnr]) {
        log_error(interface_serial_log,
                  "Close printer #%i while being closed - ignoring.",
                  prnr + 4);
        return 0;
    }

    driver_select_close(prnr, secondary);
    inuse[prnr] = 0;

    return 0;
}


static void flush_pr(unsigned int prnr, unsigned int secondary)
{
    if (!inuse[prnr]) {
        log_error(interface_serial_log,
                  "Flush printer #%i while being closed - ignoring.",
                  prnr + 4);
        return;
    }

    driver_select_flush(prnr, secondary);
}

/* ------------------------------------------------------------------------- */

static int open_pr4(struct vdrive_s *var, const BYTE *name, unsigned int length,
                    unsigned int secondary,
                    struct cbmdos_cmd_parse_s *cmd_parse_ext)
{
    return open_pr(0, name, length, secondary);
}

static int read_pr4(struct vdrive_s *var, BYTE *byte, unsigned int secondary)
{
    return read_pr(0, byte, secondary);
}

static int write_pr4(struct vdrive_s *var, BYTE byte, unsigned int secondary)
{
    return write_pr(0, byte, secondary);
}

static int close_pr4(struct vdrive_s *var, unsigned int secondary)
{
    return close_pr(0, secondary);
}

static void flush_pr4(struct vdrive_s *var, unsigned int secondary)
{
    flush_pr(0, secondary);
}

static int open_pr5(struct vdrive_s *var, const BYTE *name, unsigned int length,
                    unsigned int secondary,
                    struct cbmdos_cmd_parse_s *cmd_parse_ext)
{
    return open_pr(1, name, length, secondary);
}

static int read_pr5(struct vdrive_s *var, BYTE *byte, unsigned int secondary)
{
    return read_pr(1, byte, secondary);
}

static int write_pr5(struct vdrive_s *var, BYTE byte, unsigned int secondary)
{
    return write_pr(1, byte, secondary);
}

static int close_pr5(struct vdrive_s *var, unsigned int secondary)
{
    return close_pr(1, secondary);
}

static void flush_pr5(struct vdrive_s *var, unsigned int secondary)
{
    flush_pr(1, secondary);
}

/* ------------------------------------------------------------------------- */

int interface_serial_close(unsigned int unit)
{
    if (unit == 4) {
        close_pr(0, 0);
    }
    if (unit == 5) {
        close_pr(1, 0);
    }
    return 0;
}

int interface_serial_late_init(void)
{
    if (printer_enabled[0]) {
        if (interface_serial_attach(0) < 0) {
            return -1;
        }
    }
    if (printer_enabled[1]) {
        if (interface_serial_attach(1) < 0) {
            return -1;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

static int interface_serial_attach(unsigned int prnr)
{
    int err;

    inuse[prnr] = 0;

    switch (prnr) {
        case 0:
            err = machine_bus_device_attach(4, "Printer #4 device", read_pr4,
                                            write_pr4, open_pr4, close_pr4,
                                            flush_pr4, NULL);
            break;
        case 1:
            err = machine_bus_device_attach(5, "Printer #5 device", read_pr5,
                                            write_pr5, open_pr5, close_pr5,
                                            flush_pr5, NULL);
            break;
        default:
            err = -1;
    }

    if (err) {
        log_error(interface_serial_log,
                  "Cannot attach serial printer #%i.", prnr + 4);
        return -1;
    }

    return 0;
}

static int interface_serial_detach(unsigned int prnr)
{
    if (inuse[prnr]) {
        flush_pr(prnr, 0);
        close_pr(prnr, 0);
    }

    machine_bus_device_detach(prnr + 4);

    return 0;
}

/* ------------------------------------------------------------------------- */

void interface_serial_init(void)
{
    interface_serial_log = log_open("Serial Interface");
}

void interface_serial_shutdown(void)
{
    interface_serial_detach(0);
    interface_serial_detach(1);
}
