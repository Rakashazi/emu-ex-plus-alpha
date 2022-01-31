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

/* #define VICE_DEBUG_CMDLINE */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "cmdline.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "types.h"
#include "uicmdline.h"
#include "util.h"

#ifdef VICE_DEBUG_CMDLINE
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

static unsigned int num_options, num_allocated_options;
static cmdline_option_ram_t *options;

static char *combined_string = NULL;

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

        if (c->description == NULL) {
            archdep_startup_log_error("CMDLINE: (%d) description id not used and description NULL for '%s'.\n", num_options, c->name);
            return -1;
        }

        /* archdep_startup_log_error("CMDLINE: (%d) registering option '%s'.\n", num_options, c->name); */

        if (num_allocated_options <= num_options) {
            num_allocated_options *= 2;
            options = lib_realloc(options, (sizeof(cmdline_option_ram_t) * num_allocated_options));
            p = options + num_options;
        }

        p->name = lib_strdup(c->name);
        p->type = c->type;
        p->attributes = c->attributes;
        p->set_func = c->set_func;
        p->extra_param = c->extra_param;
        if (c->resource_name != NULL) {
            p->resource_name = lib_strdup(c->resource_name);
        } else {
            p->resource_name = NULL;
        }
        p->resource_value = c->resource_value;

        p->param_name = c->param_name;
        p->description = c->description;

        num_options++;
    }

    return 0;
}

static void cmdline_free(void)
{
    unsigned int i;

    if (combined_string) {
        lib_free(combined_string);
        combined_string = NULL;
    }

    for (i = 0; i < num_options; i++) {
        lib_free((options + i)->name);
        lib_free((options + i)->resource_name);
    }
}

void cmdline_shutdown(void)
{
#ifdef VICE_DEBUG_CMDLINE
    unsigned int i;

    for (i = 0; i < num_options; i++) {
        printf("CMDLINE\t%s\t%s\n",
                (options + i)->name,
                (options + i)->type == SET_RESOURCE ? (options + i)->resource_name : "(call-function)");

    }
#endif
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
            if ((p->attributes & CMDLINE_ATTRIB_NEED_ARGS) && i >= *argc - 1) {
                archdep_startup_log_error("Option '%s' requires a parameter.\n",
                                          p->name);
                return -1;
            }
            switch (p->type) {
                case SET_RESOURCE:
                    if (p->attributes & CMDLINE_ATTRIB_NEED_ARGS) {
                        retval = resources_set_value_string(p->resource_name, argv[i + 1]);
                    } else {
                        retval = resources_set_value(p->resource_name, p->resource_value);
                    }
                    break;
                case CALL_FUNCTION:
                    retval = p->set_func((p->attributes & CMDLINE_ATTRIB_NEED_ARGS) ? argv[i + 1] : NULL,
                                         p->extra_param);
                    break;
                default:
                    archdep_startup_log_error("Invalid type for option '%s'.\n",
                                              p->name);
                    return -1;
            }
            if (retval < 0) {
                if (p->attributes & CMDLINE_ATTRIB_NEED_ARGS) {
                    archdep_startup_log_error("Argument '%s' not valid for option `%s'.\n",
                                              argv[i + 1], p->name);
                } else {
                    archdep_startup_log_error("Option '%s' not valid.\n", p->name);
                }
                return -1;
            }

            i += (p->attributes & CMDLINE_ATTRIB_NEED_ARGS) ? 2 : 1;
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


/** \brief  Dump command line options on stdout
 *
 * \param[in]   num_options number of options
 * \param[in]   options     list of options
 * \param[in]   userparm    ignored for some reason
 *
 * XXX: this function and its brethren where all over archdep, while basically
 *      doing the same thing.
 *
 *      On Windows it seems we need to use `x64 -help|more` to make the list
 *      appear on the console. (thanks Greg)
 *
 *      Once this works, we can remove a lot of 'uicmdline.c' files from
 *      various src/arch/$arch directories. -- compyx, 2017-08-12
 */
void cmdline_show_help(void *userparam)
{
    unsigned int i;

    /* AmigaOS used some translation function for this string: */
    printf("\nAvailable command-line options:\n\n");
    for (i = 0; i < num_options; i++) {
        const char *param = cmdline_options_get_param(i);
        if ((options[i].attributes & CMDLINE_ATTRIB_NEED_ARGS) && param != NULL) {
            printf("%s %s\n", options[i].name, param);
        } else {
            puts(options[i].name);
        }
        printf("\t%s\n", cmdline_options_get_description(i));
    }
    putchar('\n');
}


char *cmdline_options_get_name(int counter)
{
    return (char *)options[counter].name;
}

const char *cmdline_options_get_param(int counter)
{
    return options[counter].param_name;
}

char *cmdline_options_get_description(int counter)
{
    if (combined_string != NULL) {
        lib_free(combined_string);
        combined_string = NULL;
    }
    if (options[counter].attributes & CMDLINE_ATTRIB_DYNAMIC_DESCRIPTION) {
        union char_func cf;
        cf.c = options[counter].description;
        combined_string = cf.f(options[counter].attributes >> 8);
    } else {
        combined_string = lib_strdup(options[counter].description);
    }
    return combined_string;
}

char *cmdline_options_string(void)
{
    unsigned int i;
    char *cmdline_string, *new_cmdline_string;
    char *add_to_options1, *add_to_options2, *add_to_options3;

    cmdline_string = lib_strdup("\n");

    for (i = 0; i < num_options; i++) {
        add_to_options1 = lib_msprintf("%s", options[i].name);
        add_to_options3 = lib_msprintf("\n\t%s\n", cmdline_options_get_description(i));
        if ((options[i].attributes & CMDLINE_ATTRIB_NEED_ARGS) && cmdline_options_get_param(i) != NULL) {
            if (options[i].attributes & CMDLINE_ATTRIB_NEED_BRACKETS) {
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

void cmdline_log_active(void)
{
    unsigned int i;
    char *cmdline, *cmd;

    cmdline = lib_strdup("-default");

    for (i = 0; i < num_options; i++) {
        const char *param = cmdline_options_get_param(i);
        const char *resname = options[i].resource_name;
        const char *optname = options[i].name;
        const char *resval_str = NULL;
        char *resval_str_default = NULL;
        int restype = -1;
        int resval_int = -1;
        int resval_int_default = -1;
        if (resname) {
            restype = resources_query_type(resname);
            if (restype == RES_INTEGER) {
                resources_get_int(resname, &resval_int);
                resources_get_default_value(resname, &resval_int_default);
            } else if (restype == RES_STRING) {
                resources_get_string(resname, &resval_str);
                resources_get_default_value(resname, &resval_str_default);
            }
        }
        cmd = NULL;
        if ((options[i].attributes & CMDLINE_ATTRIB_NEED_ARGS) && param != NULL) {
            /* the cmdline option needs a parameter, we assume this is what the resource got assigned */
            if (restype == RES_INTEGER) {
                if (resval_int != resval_int_default) {
                    char tmp[32];
                    sprintf(tmp, "%d", resval_int);
                    cmd = util_concat(optname, " \"", tmp, "\"", NULL);
                }
            } else if (restype == RES_STRING) {
                if ((resval_str != NULL) && (resval_str_default != NULL)) {
                    if (strcmp(resval_str, resval_str_default)) {
                        cmd = util_concat(optname, " \"", resval_str, "\"", NULL);
                    }
                }
            }
        } else {
            /* the options does not need a parameter, the resource value should match the value
               defined in the option itself */
            if (restype == RES_INTEGER) {
                if (resval_int != resval_int_default) {
                    if (resval_int == vice_ptr_to_int(options[i].resource_value)) {
                        cmd = lib_strdup(optname);
                    }
                }
            } else if (restype == RES_STRING) {
                if ((resval_str != NULL) && (resval_str_default != NULL) && (options[i].resource_value != NULL)) {
                    if (strcmp(resval_str, resval_str_default)) {
                        if (!strcmp(resval_str, options[i].resource_value)) {
                            cmd = lib_strdup(optname);
                        }
                    }
                }
            }
        }
        if (cmd) {
            char *p;
            p = cmdline; /* remember old pointer */
            cmdline = util_concat(p, " ", cmd, NULL);
            lib_free(p); /* free old pointer */
            lib_free(cmd); /* free old pointer */
        }
    }
    log_message(LOG_DEFAULT, "\nreconstructed commandline options (might be incomplete):");
    log_message(LOG_DEFAULT, "%s\n", cmdline);
    lib_free(cmdline);
}
