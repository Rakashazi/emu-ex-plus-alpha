/*
 * driver-select.c - Select a printer driver.
 *
 * Written by
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

/* #define DEBUG_PRINTER */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "cmdline.h"
#include "driver-select.h"
#include "drv-ascii.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "translate.h"
#include "types.h"
#include "util.h"

static log_t driver_select_log = LOG_ERR;

struct driver_select_list_s {
    driver_select_t driver_select;
    struct driver_select_list_s *next;
};
typedef struct driver_select_list_s driver_select_list_t;

/* Currently used printer driver.  */
static driver_select_t driver_select[NUM_DRIVER_SELECT];

/* Pointer to registered printer driver.  */
static driver_select_list_t *driver_select_list = NULL;

static char *userprinter_names[] = { "ascii", "nl10", "raw", NULL };

static char *printer_names[] = { "ascii", "mps803", "nl10", "raw", NULL };

static char *plotter_names[] = { "1520", "raw", NULL };

static int userprinter_name_is_valid(const char *name)
{
    int i = 0;

    while (userprinter_names[i]) {
        if (!strcmp(userprinter_names[i], name)) {
            return 1;
        }
        i++;
    }
    return 0;
}

static int printer_name_is_valid(const char *name)
{
    int i = 0;

    while (printer_names[i]) {
        if (!strcmp(printer_names[i], name)) {
            return 1;
        }
        i++;
    }
    return 0;
}

static int plotter_name_is_valid(const char *name)
{
    int i = 0;

    while (plotter_names[i]) {
        if (!strcmp(plotter_names[i], name)) {
            return 1;
        }
        i++;
    }
    return 0;
}

static int set_printer_driver(const char *name, void *param)
{
    driver_select_list_t *list;
    int prnr = vice_ptr_to_int(param);

    if (prnr == 2) {
        if (!plotter_name_is_valid(name)) {
            return -1;
        }
    } else if (prnr == 3) {
        if (!userprinter_name_is_valid(name)) {
            return -1;
        }
    } else {
        if (!printer_name_is_valid(name)) {
            return -1;
        }
    }

    list = driver_select_list;

    if (list == NULL) {
        return -1;
    }

    do {
        if (!strcmp(list->driver_select.drv_name, name)) {
            driver_select[prnr] = list->driver_select;
            return 0;
        }
        list = list->next;
    } while (list != NULL);

    return -1;
}

static const resource_string_t resources_string[] = {
    { "Printer4Driver", "ascii", RES_EVENT_NO, NULL,
      (char **)&driver_select[0].drv_name, set_printer_driver, (void *)0 },
    { "Printer5Driver", "ascii", RES_EVENT_NO, NULL,
      (char **)&driver_select[1].drv_name, set_printer_driver, (void *)1 },
    { "Printer6Driver", "1520", RES_EVENT_NO, NULL,
      (char **)&driver_select[2].drv_name, set_printer_driver, (void *)2 },
    RESOURCE_STRING_LIST_END
};

static const resource_string_t resources_string_userport[] = {
    { "PrinterUserportDriver", "ascii", RES_EVENT_NO, NULL,
      (char **)&driver_select[3].drv_name, set_printer_driver, (void *)3 },
    RESOURCE_STRING_LIST_END
};

int driver_select_init_resources(void)
{
    return resources_register_string(resources_string);
}

int driver_select_userport_init_resources(void)
{
    return resources_register_string(resources_string_userport);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-pr4drv", SET_RESOURCE, 1,
      NULL, NULL, "Printer4Driver", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_PRT_DRIVER_4_NAME,
      NULL, NULL },
    { "-pr5drv", SET_RESOURCE, 1,
      NULL, NULL, "Printer5Driver", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_PRT_DRIVER_5_NAME,
      NULL, NULL },
    { "-pr6drv", SET_RESOURCE, 1,
      NULL, NULL, "Printer6Driver", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_PRT_DRIVER_6_NAME,
      NULL, NULL },
    CMDLINE_LIST_END
};

static const cmdline_option_t cmdline_options_userport[] =
{
    { "-pruserdrv", SET_RESOURCE, 1,
      NULL, NULL, "PrinterUserportDriver", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_PRT_DRIVER_USR_NAME,
      NULL, NULL },
    CMDLINE_LIST_END
};

int driver_select_init_cmdline_options(void)
{
    return cmdline_register_options(cmdline_options);
}

int driver_select_userport_init_cmdline_options(void)
{
    return cmdline_register_options(cmdline_options_userport);
}

void driver_select_init(void)
{
    driver_select_log = log_open("Driver Select");
}

/* ------------------------------------------------------------------------- */

void driver_select_register(driver_select_t *driver_select)
{
    driver_select_list_t *list, *prev;

    prev = driver_select_list;
    while (prev != NULL && prev->next != NULL) {
        prev = prev->next;
    }

    list = lib_malloc(sizeof(driver_select_list_t));
    memcpy(&(list->driver_select), driver_select, sizeof(driver_select_t));
    list->next = NULL;

    if (driver_select_list != NULL) {
        prev->next = list;
    } else {
        driver_select_list = list;
    }
}

void driver_select_shutdown(void)
{
    driver_select_list_t *list, *next;

    list = driver_select_list;

    while (list != NULL) {
        next = list->next;
        lib_free(list);
        list = next;
    }
}

/* ------------------------------------------------------------------------- */

int driver_select_open(unsigned int prnr, unsigned int secondary)
{
#ifdef DEBUG_PRINTER
    log_message(driver_select_log, "Open device #%i secondary %i.", prnr + 4, secondary);
#endif
    return driver_select[prnr].drv_open(prnr, secondary);
}

void driver_select_close(unsigned int prnr, unsigned int secondary)
{
#ifdef DEBUG_PRINTER
    log_message(driver_select_log, "Close device #%i secondary %i.", prnr + 4, secondary);
#endif
    driver_select[prnr].drv_close(prnr, secondary);
}

int driver_select_putc(unsigned int prnr, unsigned int secondary, BYTE b)
{
    return driver_select[prnr].drv_putc(prnr, secondary, b);
}

int driver_select_getc(unsigned int prnr, unsigned int secondary, BYTE *b)
{
    return driver_select[prnr].drv_getc(prnr, secondary, b);
}

int driver_select_flush(unsigned int prnr, unsigned int secondary)
{
#ifdef DEBUG_PRINTER
    log_message(driver_select_log, "Flush device #%i secondary %i.", prnr + 4, secondary);
#endif
    return driver_select[prnr].drv_flush(prnr, secondary);
}

/* called by printer.c:printer_formfeed() */
int driver_select_formfeed(unsigned int prnr)
{
#ifdef DEBUG_PRINTER
    log_message(driver_select_log, "Formfeed device #%i", prnr + 4);
#endif
    return driver_select[prnr].drv_formfeed(prnr);
}
