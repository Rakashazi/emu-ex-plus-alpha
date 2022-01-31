/*
 * cmdline.h - Command-line parsing.
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

#ifndef VICE_CMDLINE_H
#define VICE_CMDLINE_H

/* This describes a command-line option.  */
/* Warning: all the pointers should point to areas that are valid throughout
   the execution.  No reallocation is performed.  */

typedef enum cmdline_option_type { SET_RESOURCE, CALL_FUNCTION }
    cmdline_option_type_t;

#define CMDLINE_ATTRIB_NONE                  0
#define CMDLINE_ATTRIB_NEED_ARGS             (1 << 0)
#define CMDLINE_ATTRIB_NEED_BRACKETS         (1 << 1)
#define CMDLINE_ATTRIB_DYNAMIC_DESCRIPTION   (1 << 2)

typedef struct cmdline_option_s {
    /* Name of command-line option
     *
     * This cannot be `const`, since some code (see drive-cmdline-options.c
     * for example) uses this field to store and later free dynamically
     * allocated strings.
     */
    char *name;

    /* Behavior of this command-line option.  */
    cmdline_option_type_t type;

    /* Command line option flags:
       CMDLINE_ATTRIB_NEED_ARGS:           This option needs an argument.
       CMDLINE_ATTRIB_NEED_BRACKETS:       The brackets need to be added to the text.
       CMDLINE_ATTRIB_DYNAMIC_DESCRIPTION: The description text is a pointer to a function that returns the text.
     */
    int attributes;

    /* Function to call if type is `CALL_FUNCTION'.  */
    int (*set_func)(const char *value, void *extra_param);

    /* Extra parameter to pass to `set_func' if type is `CALL_FUNCTION'.  */
    void *extra_param;

    /* Resource to change if `type' is `SET_RESOURCE'
     *
     * Also cannot be `const`, see comment of `name` field
     */
    char *resource_name;

    /* Value to assign to `resource_name' if `type' is `SET_RESOURCE' and
       `need_arg' is zero.  */
    void *resource_value;

    /* String to display after the option name in the help screen. */
    const char *param_name;

    /* Description string. */
    const char *description;
} cmdline_option_t;

typedef struct cmdline_option_ram_s {
    /* Name of command-line option.  */
    char *name;

    /* Behavior of this command-line option.  */
    cmdline_option_type_t type;

    /* Command line option flags:
       CMDLINE_ATTRIB_NEED_ARGS:           This option needs an argument.
       CMDLINE_ATTRIB_NEED_BRACKETS:       The brackets need to be added to the text.
       CMDLINE_ATTRIB_DYNAMIC_DESCRIPTION: The description text is a pointer to a function that returns the text.
     */
    int attributes;

    /* Function to call if type is `CALL_FUNCTION'.  */
    int (*set_func)(const char *value, void *extra_param);

    /* Extra parameter to pass to `set_func' if type is `CALL_FUNCTION'.  */
    void *extra_param;

    /* Resource to change if `type' is `SET_RESOURCE'.  */
    char *resource_name;

    /* Value to assign to `resource_name' if `type' is `SET_RESOURCE' and
       `need_arg' is zero.  */
    void *resource_value;

    /* String to display after the option name in the help screen. */
    const char *param_name;

    /* Description string. */
    const char *description;
} cmdline_option_ram_t;

extern int cmdline_init(void);

extern int cmdline_register_options(const cmdline_option_t *c);
extern void cmdline_shutdown(void);
extern int cmdline_parse(int *argc, char **argv);
extern void cmdline_show_help(void *userparam);
extern char *cmdline_options_string(void);
extern char *cmdline_options_get_name(int counter);
extern const char *cmdline_options_get_param(int counter);
extern char *cmdline_options_get_description(int counter);
extern int cmdline_get_num_options(void);
extern void cmdline_log_active(void);

#define CMDLINE_LIST_END { NULL, (cmdline_option_type_t)0, 0, NULL, NULL, NULL, NULL, NULL, NULL }

/* union to change pointer types for dynamic translations */
union char_func{
    const char *c;
    char *(*f)(int id);
};

#endif
