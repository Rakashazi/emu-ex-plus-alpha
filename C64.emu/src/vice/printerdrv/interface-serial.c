/*
 * interface-serial.c - Serial printer interface.
 *
 * Written by
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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
#include "serial.h"
#include "translate.h"
#include "types.h"

#ifdef HAVE_OPENCBM
static int interface_opencbm_attach(unsigned int prnr);
static int interface_opencbm_detach(unsigned int prnr);
#endif
static int interface_serial_attach(unsigned int prnr);
static int interface_serial_detach(unsigned int prnr);

static log_t interface_serial_log = LOG_ERR;

/* ------------------------------------------------------------------------- */

/*
 * We have internal emulation of devices at numbers 4...6,
 * but we allow to connect real devices in the range 4...7.
 */
#define FIRST_PRINTER_DEVICE_NUMBER     4
#define NUM_PRINTERS                    3       /* 4...6 */
#define NUM_PRINTER_DEVICE_NUMBERS      4       /* 4...7 */

static int printer_enabled[NUM_PRINTER_DEVICE_NUMBERS];

static int set_printer_enabled(int flag, void *param)
{
    unsigned int prnr;

    switch (flag) {
        case PRINTER_DEVICE_NONE:
        case PRINTER_DEVICE_FS:
#ifdef HAVE_OPENCBM
        case PRINTER_DEVICE_REAL:
#endif
            break;
        default:
            return -1;
    }

    prnr = vice_ptr_to_uint(param);

    if (prnr >= NUM_PRINTER_DEVICE_NUMBERS) {
        return -1;
    }

#ifdef HAVE_OPENCBM
    /*
     * Special hack to allow the use of a toggle menu item
     * for device #7.
     */
    if (prnr == 3 && flag != 0) {
        flag = PRINTER_DEVICE_REAL;
    }
#endif /* HAVE_OPENCBM */

    if (prnr < NUM_PRINTERS) {
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
    }

#ifdef HAVE_OPENCBM

    if (printer_enabled[prnr] == PRINTER_DEVICE_REAL
        && flag != PRINTER_DEVICE_REAL) {
        if (interface_opencbm_detach(prnr) < 0) {
            return -1;
        }
    }

    if (flag == PRINTER_DEVICE_REAL
        && printer_enabled[prnr] != PRINTER_DEVICE_REAL) {
        if (interface_opencbm_attach(prnr) < 0) {
            return -1;
        }
    }

#endif /* HAVE_OPENCBM */

    printer_enabled[prnr] = flag;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "Printer4", PRINTER_DEVICE_NONE, RES_EVENT_STRICT, (resource_value_t)0,
      &printer_enabled[0], set_printer_enabled, (void *)0 },
    { "Printer5", PRINTER_DEVICE_NONE, RES_EVENT_STRICT, (resource_value_t)0,
      &printer_enabled[1], set_printer_enabled, (void *)1 },
    { "Printer6", PRINTER_DEVICE_NONE, RES_EVENT_STRICT, (resource_value_t)0,
      &printer_enabled[2], set_printer_enabled, (void *)2 },
    { "Printer7", PRINTER_DEVICE_NONE, RES_EVENT_STRICT, (resource_value_t)0,
      &printer_enabled[3], set_printer_enabled, (void *)3 },
    { NULL }
};

int interface_serial_init_resources(void)
{
    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] = {
    { "-device4", SET_RESOURCE, 1,
      NULL, NULL, "Printer4", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_TYPE, IDCLS_SET_DEVICE_TYPE_4,
      NULL, NULL },
    { "-device5", SET_RESOURCE, 1,
      NULL, NULL, "Printer5", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_TYPE, IDCLS_SET_DEVICE_TYPE_5,
      NULL, NULL },
    { "-device6", SET_RESOURCE, 1,
      NULL, NULL, "Printer6", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_TYPE, IDCLS_SET_DEVICE_TYPE_6,
      NULL, NULL },
    { "-device7", SET_RESOURCE, 1,
      NULL, NULL, "Printer7", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_TYPE, IDCLS_SET_DEVICE_TYPE_7,
      NULL, NULL },
    { NULL }
};

int interface_serial_init_cmdline_options(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

/*
 * Which secondary addresses are in use for each printer
 * (stored as bits in each int).
 */
static unsigned int inuse_secadr[NUM_PRINTER_DEVICE_NUMBERS];

/*
 * Not all bytes that are sent under ATTENTION on the bus are translated
 * into driver calls here (such as OPEN SECONDARY ADDRESS, or UNLISTEN).
 * And not all actions from BASIC are translated into bus activity (such
 * as OPEN 1,4,0 (without file name)).
 * The result is that we don't properly know if we're open or not.
 * Opens can be implicit, but closes are not.
 */

static int open_pr(unsigned int prnr, const BYTE *name, unsigned int length,
                   unsigned int secondary)
{
    int mask = 1 << secondary;

    if (prnr >= NUM_PRINTERS) {
        return -1;
    }

    /* Check for first open and do special call if so. */
    if (inuse_secadr[prnr] == 0) {
        if (driver_select_open(prnr, DRIVER_FIRST_OPEN) < 0) {
            log_error(interface_serial_log,
                      "Couldn't initialize device #%i.",
                    prnr + FIRST_PRINTER_DEVICE_NUMBER);
            return -1;
        }
    }

    if (inuse_secadr[prnr] & mask) {
        log_error(interface_serial_log,
                  "Open printer #%i,%i while already open - ignoring.",
                prnr + FIRST_PRINTER_DEVICE_NUMBER, secondary);
        return 0;
    }

    if (driver_select_open(prnr, secondary) < 0) {
        log_error(interface_serial_log,
                  "Couldn't open device #%i,%i.",
                prnr + FIRST_PRINTER_DEVICE_NUMBER, secondary);
        return -1;
    }

    inuse_secadr[prnr] |= mask;

    return 0;
}

static int read_pr(unsigned int prnr, BYTE *byte, unsigned int secondary)
{
    return 0x80;
}

static int write_pr(unsigned int prnr, BYTE byte, unsigned int secondary)
{
    int err;
    int mask = 1 << secondary;

    if (!(inuse_secadr[prnr] & mask)) {
        /* oh, well, we just assume an implicit open - "OPEN 1,4"
           just does not leave any trace on the serial bus */
        log_message(interface_serial_log,
                    "Auto-opening printer #%i,%i.",
                    prnr + FIRST_PRINTER_DEVICE_NUMBER, secondary);

        err = open_pr(prnr, NULL, 0, secondary);

        if (err < 0) {
            return err;
        }
    }

    return driver_select_putc(prnr, secondary, byte);
}

static int close_pr(unsigned int prnr, unsigned int secondary)
{
    int mask = 1 << secondary;

    if (!(inuse_secadr[prnr] & mask)) {
        log_error(interface_serial_log,
                  "Close printer #%i,%i while closed - ignoring.",
                  prnr + FIRST_PRINTER_DEVICE_NUMBER, secondary);
        return 0;
    }

    driver_select_close(prnr, secondary);

    inuse_secadr[prnr] &= ~mask;

    /* Check for last close and do special call if so. */
    if (inuse_secadr[prnr] == 0) {
        driver_select_close(prnr, DRIVER_LAST_CLOSE);
    }

    return 0;
}


static void flush_pr(unsigned int prnr, unsigned int secondary)
{
    int mask = 1 << secondary;

    if (!(inuse_secadr[prnr] & mask)) {
        log_error(interface_serial_log,
                  "Flush printer #%i,%i while closed - ignoring.",
                  prnr + FIRST_PRINTER_DEVICE_NUMBER, secondary);
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

static int open_pr6(struct vdrive_s *var, const BYTE *name, unsigned int length,
                    unsigned int secondary,
                    struct cbmdos_cmd_parse_s *cmd_parse_ext)
{
    return open_pr(2, name, length, secondary);
}

static int read_pr6(struct vdrive_s *var, BYTE *byte, unsigned int secondary)
{
    return read_pr(2, byte, secondary);
}

static int write_pr6(struct vdrive_s *var, BYTE byte, unsigned int secondary)
{
    return write_pr(2, byte, secondary);
}

static int close_pr6(struct vdrive_s *var, unsigned int secondary)
{
    return close_pr(2, secondary);
}

static void flush_pr6(struct vdrive_s *var, unsigned int secondary)
{
    flush_pr(2, secondary);
}

/* ------------------------------------------------------------------------- */

int interface_serial_close(unsigned int unit)
{
    if (unit >= FIRST_PRINTER_DEVICE_NUMBER &&
        unit <  FIRST_PRINTER_DEVICE_NUMBER + NUM_PRINTERS) {
        close_pr(unit - FIRST_PRINTER_DEVICE_NUMBER, 0);
    }

    return 0;
}

/*
 * Re-initialize the settings from the resources or command line,
 * which may have been reset in the mean time...
 */
int interface_serial_late_init(void)
{
    int i;

    for (i = 0; i < NUM_PRINTER_DEVICE_NUMBERS; i++) {
        if (printer_enabled[i] == PRINTER_DEVICE_FS) {
            if (interface_serial_attach(i) < 0) {
                return -1;
            }
        }
#ifdef HAVE_OPENCBM
        else if (printer_enabled[i] == PRINTER_DEVICE_REAL) {
            if (interface_opencbm_attach(i) < 0) {
                return -1;
            }
        }
#endif /* HAVE_OPENCBM */
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
#if defined(HAVE_OPENCBM)

static int interface_opencbm_attach(unsigned int prnr)
{
    int devnr = FIRST_PRINTER_DEVICE_NUMBER + prnr;
    serial_t *p;

    /*log_message(interface_serial_log, "calling interface_opencbm_attach %d", prnr);*/

    serial_device_type_set(SERIAL_DEVICE_REAL, devnr);
    p = serial_device_get(devnr);
    p->inuse = 1;

    inuse_secadr[prnr] = 0;

    return 0;
}

static int interface_opencbm_detach(unsigned int prnr)
{
    int devnr = FIRST_PRINTER_DEVICE_NUMBER + prnr;
    serial_t *p;

    /*log_message(interface_serial_log, "calling interface_opencbm_detach %d", prnr);*/

    serial_device_type_set(SERIAL_DEVICE_NONE, devnr);
    p = serial_device_get(devnr);
    p->inuse = 0;

    return interface_serial_detach(prnr);
}

#endif /* HAVE_OPENCBM */

/* ------------------------------------------------------------------------- */

static int interface_serial_attach(unsigned int prnr)
{
    int err;

    inuse_secadr[prnr] = 0;

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
        case 2:
            err = machine_bus_device_attach(6, "Printer #6 device", read_pr6,
                                            write_pr6, open_pr6, close_pr6,
                                            flush_pr6, NULL);
            break;
        default:
            err = -1;
    }

    if (err) {
        log_error(interface_serial_log,
                  "Cannot attach serial printer #%i.", prnr + FIRST_PRINTER_DEVICE_NUMBER);
        return -1;
    }
    serial_device_type_set(SERIAL_DEVICE_FS, FIRST_PRINTER_DEVICE_NUMBER + prnr);

    return 0;
}

static int interface_serial_detach(unsigned int prnr)
{
    if (prnr < NUM_PRINTERS && inuse_secadr[prnr]) {
        int i;
        for (i = 0; i < 8; i++) {
            if (inuse_secadr[prnr] & (1 << i)) {
                flush_pr(prnr, i);
                close_pr(prnr, i);
            }
        }
    }

    machine_bus_device_detach(prnr + FIRST_PRINTER_DEVICE_NUMBER);

    return 0;
}

/* ------------------------------------------------------------------------- */

void interface_serial_init(void)
{
    interface_serial_log = log_open("Serial Interface");
}

/* called by printer.serial.c:printer_serial_shutdown() */
void interface_serial_shutdown(void)
{
    int i;

    for (i = 0; i < NUM_PRINTER_DEVICE_NUMBERS; i++) {
        interface_serial_detach(i);
    }
}
