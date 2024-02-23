/*
 * driver-select.c - Select a printer driver.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Bas Wassink <b.wassink@ziggo.nl>
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
#include <stdbool.h>
#include <string.h>

#include "cmdline.h"
#include "driver-select.h"
#include "drv-ascii.h"
#include "lib.h"
#include "log.h"
#include "printer.h"
#include "resources.h"
#include "types.h"
#include "util.h"


static log_t driver_select_log = LOG_ERR;

/* Currently used printer driver.  */
static driver_select_t driver_select[NUM_DRIVER_SELECT];

/* Pointer to registered printer driver.  */
static driver_select_list_t *driver_select_list = NULL;

/* Pointer to tail of registered printer driver list */
static driver_select_list_t *driver_select_list_tail = NULL;

#if 0
static const char * const userprinter_names[] = { "ascii", "nl10", "raw", NULL };

static const char * const printer_names[] = { "ascii", "2022", "4023", "8023",
    "mps801", "mps802", "mps803", "nl10", "raw", NULL };

static const char * const plotter_names[] = { "1520", "raw", NULL };
#endif

/** \brief  Check boolean propery of a printer driver
 *
 * Iterate registered printer drivers for drive \a name and call function
 * \a propcheck to test a boolean propery.
 *
 * \param[in]   name        printer driver name
 * \param[in]   propcheck   function to call to check property
 *
 * \return  \c true if \a name is valid and \a propcheck returned \a true,
 *          \c false otherwise
 */
static bool check_property(const char *name,
                           bool (*propcheck)(const driver_select_t *))
{
    const driver_select_list_t *node = driver_select_list;

    while (node != NULL) {
        if ((strcmp(name, node->driver_select.drv_name) == 0)
                && propcheck(&(node->driver_select))) {
            return true;
        }
        node = node->next;
    }
    return false;
}

/** \brief  Define printer driver property check function
 *
 * \param[in]   property    boolean member to test for \c true
 */
#define PROP_CHECK_FUNC(property)                                 \
    static bool check_prop_##property(const driver_select_t *drv) \
    {                                                             \
        return drv->property;                                     \
    }

/* property check functions */
PROP_CHECK_FUNC(printer);
PROP_CHECK_FUNC(plotter);
PROP_CHECK_FUNC(iec);
PROP_CHECK_FUNC(ieee488);
PROP_CHECK_FUNC(userport);
PROP_CHECK_FUNC(text);
PROP_CHECK_FUNC(graphics);


/** \brief  Determine if drive is a printer driver
 *
 * \param[in]   drv_name    driver name
 *
 * \return  \c true if \a drv_name is a printer driver
 */
bool driver_select_is_printer(const char *drv_name)
{
    return check_property(drv_name, check_prop_printer);
}

/** \brief  Determine if driver is a plotter driver
 *
 * \param[in]   drv_name    driver name
 *
 * \return  \c true if \a drv_name is a plotter driver
 */
bool driver_select_is_plotter(const char *drv_name)
{
    return check_property(drv_name, check_prop_plotter);
}

/** \brief  Determine if driver supports an IEC bus-connected device
 *
 * \param[in]   drv_name    driver name
 *
 * \return  \c true if \a drv_name is an IEC driver
 */
bool driver_select_has_iec_bus(const char *drv_name)
{
    return check_property(drv_name, check_prop_iec);
}

/** \brief  Determine if driver supports an IEEE-488 bus-connected device
 *
 * \param[in]   drv_name    driver name
 *
 * \return  \c true if \a drv_name is an IEEE-488 driver
 */
bool driver_select_has_ieee488_bus(const char *drv_name)
{
    return check_property(drv_name, check_prop_ieee488);
}

/** \brief  Determine if driver supports a userport-connected device
 *
 * \param[in]   drv_name    driver name
 *
 * \return  \c true if \a drv_name is a userport driver
 */
bool driver_select_has_userport(const char *drv_name)
{
    return check_property(drv_name, check_prop_userport);
}

/** \brief  Determine if driver supports text output
 *
 * \param[in]   drv_name    driver name
 *
 * \return  \c true if \a drv_name supports text output
 */
bool driver_select_has_text_output(const char *drv_name)
{
    return check_property(drv_name, check_prop_text);
}

/** \brief  Determine if driver supports graphics output
 *
 * \param[in]   drv_name    driver name
 *
 * \return  \c true if \a drv_name supports graphics output
 */
bool driver_select_has_graphics_output(const char *drv_name)
{
    return check_property(drv_name, check_prop_graphics);
}

/** \brief  Get list of registered printer drivers
 *
 * \return  singly-linked list of registered drivers
 */
const driver_select_list_t *driver_select_get_drivers(void)
{
    return driver_select_list;
}


static int set_printer_driver(const char *name, void *param)
{
    driver_select_list_t *list;
    int                    prnr;

    prnr = vice_ptr_to_int(param);
    list = driver_select_list;
    if (list == NULL) {
        return -1;
    }

    if ((prnr == PRINTER_IEC_4) || (prnr == PRINTER_IEC_5)) {
        if (!driver_select_is_printer(name)) {
            return -1;
        }
    } else if (prnr == PRINTER_IEC_6) {
        if (!driver_select_is_plotter(name)) {
            return -1;
        }
    } else if (prnr == PRINTER_USERPORT) {
        if (!driver_select_has_userport(name)) {
            return -1;
        }
    } else {
        return -1;
    }

    do {
        if (!strcmp(list->driver_select.drv_name, name)) {
            memcpy(&(driver_select[prnr]), &(list->driver_select), sizeof(driver_select_t));
            if(driver_select[prnr].drv_select) {
#ifdef DEBUG_PRINTER
                log_debug(driver_select_log, "driver_select[%d].drv_select != NULL", prnr);
#endif
                driver_select[prnr].drv_select(prnr);
            }
            return 0;
        }
        list = list->next;
    } while (list != NULL);

    return -1;
}

static const resource_string_t resources_string[] = {
    { "Printer4Driver", "mps803", RES_EVENT_NO, NULL,
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
    { "-pr4drv", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Printer4Driver", NULL,
      "<Name>", "Specify name of printer driver for device #4. (ascii/raw/mps801/mps802/mps803/220/4032/8023/nl10)" },
    { "-pr5drv", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Printer5Driver", NULL,
      "<Name>", "Specify name of printer driver for device #5. (ascii/raw/mps801/mps802/mps803/220/4032/8023/nl10)" },
    { "-pr6drv", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Printer6Driver", NULL,
      "<Name>", "Specify name of printer driver for device #6. (1520/raw)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t cmdline_options_userport[] =
{
    { "-pruserdrv", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "PrinterUserportDriver", NULL,
      "<Name>", "Specify name of printer driver for the userport printer. (ascii/nl10/raw)" },
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

void driver_select_register(driver_select_t *drv_select)
{
    driver_select_list_t *node;


    node = lib_malloc(sizeof *node);
    node->driver_select          = *drv_select;
    node->driver_select.drv_name = lib_strdup(drv_select->drv_name);
    node->driver_select.ui_name  = lib_strdup(drv_select->ui_name);
    node->next                   = NULL;

    if (driver_select_list == NULL) {
        driver_select_list = node;
    } else {
        driver_select_list_tail->next = node;
    }
    driver_select_list_tail = node;
}


void driver_select_shutdown(void)
{
    driver_select_list_t *node = driver_select_list;

    while (node != NULL) {
        driver_select_list_t *next = node->next;

        lib_free(node->driver_select.drv_name);
        lib_free(node->driver_select.ui_name);
        lib_free(node);
        node = next;
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

int driver_select_putc(unsigned int prnr, unsigned int secondary, uint8_t b)
{
    return driver_select[prnr].drv_putc(prnr, secondary, b);
}

int driver_select_getc(unsigned int prnr, unsigned int secondary, uint8_t *b)
{
    return driver_select[prnr].drv_getc(prnr, secondary, b);
}

int driver_select_flush(unsigned int prnr, unsigned int secondary)
{
#ifdef DEBUG_PRINTER
    log_message(driver_select_log, "Flush device #%u secondary %u.",
            prnr + 4, secondary);
#endif
    return driver_select[prnr].drv_flush(prnr, secondary);
}

/* called by printer.c:printer_formfeed() */
int driver_select_formfeed(unsigned int prnr)
{
#ifdef DEBUG_PRINTER
    log_message(driver_select_log, "Formfeed device #%u", prnr + 4);
#endif
    if (driver_select[prnr].drv_formfeed)
        return driver_select[prnr].drv_formfeed(prnr);
    else
        return 0;
}
