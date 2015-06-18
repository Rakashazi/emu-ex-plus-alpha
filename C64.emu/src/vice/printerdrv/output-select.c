/*
 * output-select.c - Select an output driver.
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

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "cmdline.h"
#include "lib.h"
#include "output-select.h"
#include "resources.h"
#include "translate.h"
#include "types.h"
#include "util.h"

struct output_select_list_s {
    output_select_t output_select;
    struct output_select_list_s *next;
};
typedef struct output_select_list_s output_select_list_t;

/* Currently used output device.  */
static output_select_t output_select[NUM_OUTPUT_SELECT];

/* Pointer to registered printer driver.  */
static output_select_list_t *output_select_list = NULL;


static int set_output_device(const char *name, void *param)
{
    output_select_list_t *list;
    int prnr;

    list = output_select_list;

    if (list == NULL) {
        return -1;
    }

    prnr = vice_ptr_to_int(param);

    do {
        if (!strcmp(list->output_select.output_name, name)) {
            output_select[prnr] = list->output_select;
            return 0;
        }
        list = list->next;
    } while (list != NULL);

    return -1;
}

static const resource_string_t resources_string[] = {
    { "Printer4Output", "text", RES_EVENT_NO, NULL,
      (char **)&output_select[0].output_name, set_output_device, (void *)0 },
    { "Printer5Output", "text", RES_EVENT_NO, NULL,
      (char **)&output_select[1].output_name, set_output_device, (void *)1 },
    { "Printer6Output", "text", RES_EVENT_NO, NULL,
      (char **)&output_select[2].output_name, set_output_device, (void *)2 },
    { NULL }
};

static const resource_string_t resources_string_userport[] = {
    { "PrinterUserportOutput", "text", RES_EVENT_NO, NULL,
      (char **)&output_select[3].output_name, set_output_device, (void *)3 },
    { NULL }
};

int output_select_init_resources(void)
{
    return resources_register_string(resources_string);
}

int output_select_userport_init_resources(void)
{
    return resources_register_string(resources_string_userport);
}

static cmdline_option_t cmdline_options[] =
{
    { "-pr4output", SET_RESOURCE, 1,
      NULL, NULL, "Printer4Output", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_COMBO,
      IDCLS_P_NAME, IDCLS_SPECIFY_OUTPUT_DEVICE_4_NAME,
      NULL, NULL },
    { "-pr5output", SET_RESOURCE, 1,
      NULL, NULL, "Printer5Output", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_COMBO,
      IDCLS_P_NAME, IDCLS_SPECIFY_OUTPUT_DEVICE_5_NAME,
      NULL, NULL },
    { "-pr6output", SET_RESOURCE, 1,
      NULL, NULL, "Printer6Output", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_COMBO,
      IDCLS_P_NAME, IDCLS_SPECIFY_OUTPUT_DEVICE_6_NAME,
      NULL, NULL },
    { NULL }
};

static cmdline_option_t cmdline_options_userport[] =
{
    { "-pruseroutput", SET_RESOURCE, 1,
      NULL, NULL, "PrinterUserportOutput", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_COMBO,
      IDCLS_P_NAME, IDCLS_SPECIFY_OUTPUT_DEVICE_USR_NAME,
      NULL, NULL },
    { NULL }
};

static char *printer_output_names = NULL;

static void build_printer_output_names(void)
{
    output_select_list_t *list;
    char *tmp1, *tmp2;

    list = output_select_list;

    if (list != NULL) {
        tmp1 = util_concat(". (", list->output_select.output_name, NULL);
        list = list->next;
        while (list) {
            tmp2 = util_concat(tmp1, ", ", list->output_select.output_name, NULL);
            lib_free(tmp1);
            tmp1 = tmp2;
            list = list->next;
        }
        printer_output_names = util_concat(tmp1, ")", NULL);
        lib_free(tmp1);
    }
}

int output_select_init_cmdline_options(void)
{
    int i;

    if (!printer_output_names) {
        build_printer_output_names();
    }
    if (!printer_output_names) {
        return -1;
    }
    for (i = 0; i < 3; i++) {
        cmdline_options[i].description = printer_output_names;
    }

    return cmdline_register_options(cmdline_options);
}

int output_select_userport_init_cmdline_options(void)
{
    if (!printer_output_names) {
        build_printer_output_names();
    }
    if (!printer_output_names) {
        return -1;
    }
    cmdline_options_userport[0].description = printer_output_names;

    return cmdline_register_options(cmdline_options_userport);
}

void output_select_shutdown(void)
{
    output_select_list_t *list, *next;

    list = output_select_list;

    while (list != NULL) {
        next = list->next;
        lib_free(list);
        list = next;
    }
    if (printer_output_names) {
        lib_free(printer_output_names);
        printer_output_names = NULL;
    }
}

/* ------------------------------------------------------------------------- */

void output_select_register(output_select_t *output_select)
{
    output_select_list_t *list, *prev;

    prev = output_select_list;
    while (prev != NULL && prev->next != NULL) {
        prev = prev->next;
    }

    list = lib_malloc(sizeof(output_select_list_t));
    memcpy(&(list->output_select), output_select, sizeof(output_select_t));
    list->next = NULL;

    if (output_select_list != NULL) {
        prev->next = list;
    } else {
        output_select_list = list;
    }
}

/* ------------------------------------------------------------------------- */

int output_select_open(unsigned int prnr,
                       struct output_parameter_s *output_parameter)
{
    return output_select[prnr].output_open(prnr, output_parameter);
}

void output_select_close(unsigned int prnr)
{
    output_select[prnr].output_close(prnr);
}

int output_select_putc(unsigned int prnr, BYTE b)
{
    return output_select[prnr].output_putc(prnr, b);
}

int output_select_getc(unsigned int prnr, BYTE *b)
{
    return output_select[prnr].output_getc(prnr, b);
}

int output_select_flush(unsigned int prnr)
{
    return output_select[prnr].output_flush(prnr);
}
