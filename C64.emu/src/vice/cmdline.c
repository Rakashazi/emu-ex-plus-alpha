/*
 * cmdline.c - Command-line parsing.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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

/* #define DEBUG_CMDLINE */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "cmdline.h"
#include "lib.h"
#include "resources.h"
#include "translate.h"
#include "types.h"
#include "uicmdline.h"
#include "util.h"

#ifdef DEBUG_CMDLINE
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

static unsigned int num_options, num_allocated_options;
static cmdline_option_ram_t *options;

int cmdline_init(void)
{
    lib_free(options);
    options = NULL;

    num_allocated_options = 100;
    num_options = 0;
    options = lib_malloc(sizeof(cmdline_option_ram_t) * num_allocated_options);

    return 0;
}

static cmdline_option_ram_t *lookup_exact(const char *name)
{
    unsigned int i;

    for (i = 0; i < num_options; i++) {
        if (strcmp(options[i].name, name) == 0) {
            return &options[i];
        }
    }
    return NULL;
}

int cmdline_register_options(const cmdline_option_t *c)
{
    cmdline_option_ram_t *p;

    p = options + num_options;
    for (; c->name != NULL; c++, p++) {
        if (lookup_exact(c->name)) {
            archdep_startup_log_error("CMDLINE: (%d) Duplicated option '%s'.\n", num_options, c->name);
            return -1;
        }

        if (c->use_description_id != USE_DESCRIPTION_ID) {
            if (c->description == NULL) {
                archdep_startup_log_error("CMDLINE: (%d) description id not used and description NULL for '%s'.\n", num_options, c->name);
                return -1;
            }
        }

        /* archdep_startup_log_error("CMDLINE: (%d) registering option '%s'.\n", num_options, c->name); */

        if (num_allocated_options <= num_options) {
            num_allocated_options *= 2;
            options = lib_realloc(options, (sizeof(cmdline_option_ram_t) * num_allocated_options));
            p = options + num_options;
        }

        p->name = lib_stralloc(c->name);
        p->type = c->type;
        p->need_arg = c->need_arg;
        p->set_func = c->set_func;
        p->extra_param = c->extra_param;
        if (c->resource_name != NULL) {
            p->resource_name = lib_stralloc(c->resource_name);
        } else {
            p->resource_name = NULL;
        }
        p->resource_value = c->resource_value;

        p->use_param_name_id = c->use_param_name_id;
        p->use_description_id = c->use_description_id;

        p->param_name = c->param_name;
        p->description = c->description;

        p->param_name_trans = c->param_name_trans;
        p->description_trans = c->description_trans;

        p->combined_string = NULL;

        num_options++;
    }

    return 0;
}

static void cmdline_free(void)
{
    unsigned int i;

    for (i = 0; i < num_options; i++) {
        lib_free((options + i)->name);
        lib_free((options + i)->resource_name);
        if ((options + i)->combined_string) {
            lib_free((options + i)->combined_string);
        }
    }
}

void cmdline_shutdown(void)
{
    cmdline_free();

    lib_free(options);
}

static cmdline_option_ram_t *lookup(const char *name, int *is_ambiguous)
{
    cmdline_option_ram_t *match;
    size_t name_len;
    unsigned int i;

    name_len = strlen(name);

    match = NULL;
    *is_ambiguous = 0;
    for (i = 0; i < num_options; i++) {
        if (strncmp(options[i].name, name, name_len) == 0) {
            if (options[i].name[name_len] == '\0') {
                /* return exact matches immediately */
                *is_ambiguous = 0;
                return &options[i];
            } else if (match != NULL) {
                /* multiple non-exact matches found */
                /* don't exit now, an exact match could be found later */
                *is_ambiguous = 1;
            }
            match = &options[i];
        }
    }

    return match;
}

int cmdline_parse(int *argc, char **argv)
{
    int i = 1;
    unsigned j;

    DBG(("cmdline_parse (argc:%d)\n", *argc));
    while ((i < *argc) && (argv[i] != NULL)) {
        DBG(("%d:%s\n", i, argv[i]));
        if ((argv[i][0] == '-') || (argv[i][0] == '+')) {
            int is_ambiguous, retval;
            cmdline_option_ram_t *p;

            if (argv[i][1] == '\0') {
                archdep_startup_log_error("Invalid option '%s'.\n", argv[i]);
                return -1;
            }

            if (argv[i][1] == '-') {
                /* `--' delimits the end of the option list.  */
                if (argv[i][2] == '\0') {
                    i++;
                    break;
                }
                /* This is a kludge to allow --long options */
                for (j = 0; j < strlen(argv[i]); j++) {
                    argv[i][j] = argv[i][j + 1];
                }
            }

            p = lookup(argv[i], &is_ambiguous);
            if (p == NULL) {
                archdep_startup_log_error("Unknown option '%s'.\n", argv[i]);
                return -1;
            }

            if (is_ambiguous) {
                archdep_startup_log_error("Option '%s' is ambiguous.\n",
                                          argv[i]);
                return -1;
            }
            if (p->need_arg && i >= *argc - 1) {
                archdep_startup_log_error("Option '%s' requires a parameter.\n",
                                          p->name);
                return -1;
            }
            switch (p->type) {
                case SET_RESOURCE:
                    if (p->need_arg) {
                        retval = resources_set_value_string(p->resource_name, argv[i + 1]);
                    } else {
                        retval = resources_set_value(p->resource_name, p->resource_value);
                    }
                    break;
                case CALL_FUNCTION:
                    retval = p->set_func(p->need_arg ? argv[i + 1] : NULL,
                                         p->extra_param);
                    break;
                default:
                    archdep_startup_log_error("Invalid type for option '%s'.\n",
                                              p->name);
                    return -1;
            }
            if (retval < 0) {
                if (p->need_arg) {
                    archdep_startup_log_error("Argument '%s' not valid for option `%s'.\n",
                                              argv[i + 1], p->name);
                } else {
                    archdep_startup_log_error("Option '%s' not valid.\n", p->name);
                }
                return -1;
            }

            i += p->need_arg ? 2 : 1;
        } else {
            break;
        }
    }

    /* Remove all of the parsed options. */
    DBG(("i:%d argc:%d\n", i, *argc));
    j = 1;
    while (1) {
        argv[j] = argv[i];
        if ((argv[i] == NULL) || (i >= *argc)) {
            break;
        }
        DBG(("%u <- %d:%s\n", j, i, argv[i]));
        j++;
        i++;
    }
    *argc = (int)j;
    DBG(("new argc:%u\n", j));

    return 0;
}

void cmdline_show_help(void *userparam)
{
    ui_cmdline_show_help(num_options, options, userparam);
}

char *cmdline_options_get_name(int counter)
{
    return (char *)_(options[counter].name);
}

char *cmdline_options_get_param(int counter)
{
    if (options[counter].use_param_name_id == USE_PARAM_ID) {
        return translate_text(options[counter].param_name_trans);
    } else {
        return (char *)_(options[counter].param_name);
    }
}

char *cmdline_options_get_description(int counter)
{
    if (options[counter].use_description_id == USE_DESCRIPTION_ID) {
        return translate_text(options[counter].description_trans);
    } else if (options[counter].use_description_id == USE_DESCRIPTION_COMBO) {
        if (options[counter].combined_string) {
            lib_free(options[counter].combined_string);
        }
        options[counter].combined_string = util_concat(translate_text(options[counter].description_trans), options[counter].description, NULL);
        return options[counter].combined_string;
    } else {
        return (char *)_(options[counter].description);
    }
}

char *cmdline_options_string(void)
{
    unsigned int i;
    char *cmdline_string, *new_cmdline_string;
    char *add_to_options1, *add_to_options2, *add_to_options3;

    cmdline_string = lib_stralloc("\n");

    for (i = 0; i < num_options; i++) {
        add_to_options1 = lib_msprintf("%s", options[i].name);
        add_to_options3 = lib_msprintf("\n\t%s\n", cmdline_options_get_description(i));
        if (options[i].need_arg && cmdline_options_get_param(i) != NULL) {
            if (options[i].need_arg == -1) {
                add_to_options2 = lib_msprintf(" <%s>", cmdline_options_get_param(i));
            } else {
                add_to_options2 = lib_msprintf(" %s", cmdline_options_get_param(i));
            }
            new_cmdline_string = util_concat(cmdline_string, add_to_options1,
                                             add_to_options2, add_to_options3,
                                             NULL);
            lib_free(add_to_options2);
        } else {
            new_cmdline_string = util_concat(cmdline_string, add_to_options1,
                                             add_to_options3, NULL);
        }
        lib_free(add_to_options1);
        lib_free(add_to_options3);

        lib_free(cmdline_string);

        cmdline_string = new_cmdline_string;
    }
    return cmdline_string;
}

int cmdline_get_num_options(void)
{
    return num_options;
}
