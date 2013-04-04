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


/* Names of currently used printer driver.  To be removed.  */
static char *output_device[] = { NULL, NULL, NULL };

/* Currently used output device.  */
static output_select_t output_select[3];

/* Pointer to registered printer driver.  */
static output_select_list_t *output_select_list = NULL;


static int set_output_device(const char *name, void *param)
{
    output_select_list_t *list;

    list = output_select_list;

    if (list == NULL) {
        return -1;
    }

    do {
        if (!strcmp(list->output_select.output_name, name)) {
            util_string_set(&output_device[vice_ptr_to_int(param)], name);
            memcpy(&output_select[vice_ptr_to_int(param)], &(list->output_select),
                   sizeof(output_select_t));
            return 0;
        }
        list = list->next;
    } while (list != NULL);

    return -1;
}

static const resource_string_t resources_string[] = {
    { "Printer4Output", "text", RES_EVENT_NO, NULL,
      &output_device[0], set_output_device, (void *)0 },
    { "Printer5Output", "text", RES_EVENT_NO, NULL,
      &output_device[1], set_output_device, (void *)1 },
    { "PrinterUserportOutput", "text", RES_EVENT_NO, NULL,
      &output_device[2], set_output_device, (void *)2 },
    { NULL }
};

int output_select_init_resources(void)
{
    return resources_register_string(resources_string);
}

void output_select_shutdown_resources(void)
{
    lib_free(output_device[0]);
    lib_free(output_device[1]);
    lib_free(output_device[2]);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-pr4output", SET_RESOURCE, 1,
      NULL, NULL, "Printer4Output", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_OUTPUT_DEVICE_4_NAME,
      NULL, NULL },
    { "-pr5output", SET_RESOURCE, 1,
      NULL, NULL, "Printer5Output", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_OUTPUT_DEVICE_5_NAME,
      NULL, NULL },
    { "-pruseroutput", SET_RESOURCE, 1,
      NULL, NULL, "PrinterUserportOutput", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_OUTPUT_DEVICE_USR_NAME,
      NULL, NULL },
    { NULL }
};

int output_select_init_cmdline_options(void)
{
    return cmdline_register_options(cmdline_options);
}


void output_select_init(void)
{
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
