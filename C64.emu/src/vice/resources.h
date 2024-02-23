/*
 * resources.h - Resource handling for VICE.
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

#ifndef VICE_RESOURCES_H
#define VICE_RESOURCES_H

#include <stdio.h>
#include <stdbool.h>

typedef enum resource_type_s {
    RES_INTEGER,
    RES_STRING
} resource_type_t;

/* Flag: Is the resource important for history recording or netplay? */
typedef enum resource_event_relevant_s {
    /* The value can be different on client and server, and may change at any
       time on either side. This is the default for most resources. */
    RES_EVENT_NO,
    /* Must be the same on server and client, when a value changes that event
       is distributed to the other emulator. when client connects to server, the
       server sends the initial values to the client.
       example: MachineVideoStandard */
    RES_EVENT_SAME,
    /* must be the same on server and client, the value can not be changed.
       Server sets the resource to event_strict_value when server is started.
       Client sets the resource to event_strict_value when it connects to the
       server.
       example: InitialWarpMode = 0 */
    RES_EVENT_STRICT
} resource_event_relevant_t;

typedef void *resource_value_t;

typedef int resource_set_func_int_t(int, void *);
typedef int resource_set_func_string_t(const char *, void *);

typedef void resource_callback_func_t(const char *name, void *param);

struct resource_callback_desc_s;
struct event_list_state_s;

struct resource_int_s {
    /* Resource name
     *
     * This cannot be `const`, since some code uses this field to store, and
     * later free, dynamically allocated strings. See for example
     * video-resources.c
     */
    char *name;

    /* Factory default value.  This cannot be 'const' since in some cases the
       value will be determined and set at runtime */
    int factory_value;

    /* Flag: Is the resource important for history recording or netplay? */
    resource_event_relevant_t event_relevant;

    /* Value that is needed for correct history recording and netplay.  */
    resource_value_t *event_strict_value;

    /* Pointer to the value.  This is only used for *reading* it.  To change
       it, use `set_func'.  */
    int *value_ptr;

    /* Function to call to set the value.  */
    resource_set_func_int_t *set_func;

    /* Extra parameter to pass to `set_func'.  */
    void *param;
};
typedef struct resource_int_s resource_int_t;

#define RESOURCE_INT_LIST_END { NULL, 0, (resource_event_relevant_t)0, NULL, NULL, NULL, NULL }

struct resource_string_s {
    /* Resource name
     *
     * This cannot be `const`, since some code uses this field to store, and
     * later free, dynamically allocated strings. See for example
     * video-resources.c
     */
    char *name;

    /* Factory default value.  */
    const char *factory_value;

    /* Is the resource important for history recording or netplay? */
    resource_event_relevant_t event_relevant;

    /* Value that is needed for correct history recording and netplay.  */
    resource_value_t *event_strict_value;

    /* Pointer to the value.  This is only used for *reading* it.  To change
       it, use `set_func'.  */
    char **value_ptr;

    /* Function to call to set the value.  */
    resource_set_func_string_t *set_func;

    /* Extra parameter to pass to `set_func'.  */
    void *param;
};
typedef struct resource_string_s resource_string_t;

#define RESOURCE_STRING_LIST_END { NULL, NULL, (resource_event_relevant_t)0, NULL, NULL, NULL, NULL }

/* do not use -1 here since that is reserved for generic/other errors */
#define RESERR_FILE_NOT_FOUND       -2
#define RESERR_FILE_INVALID         -3
#define RESERR_TYPE_INVALID         -4
#define RESERR_UNKNOWN_RESOURCE     -5
#define RESERR_READ_ERROR           -6
#define RESERR_CANNOT_CREATE_FILE   -7
#define RESERR_CANNOT_REMOVE_BACKUP -8
#define RESERR_WRITE_PROTECTED      -9
#define RESERR_CANNOT_RENAME_FILE   -10

/* ------------------------------------------------------------------------- */

extern char *vice_config_file;

int resources_init(const char *machine);
int resources_register_int(const resource_int_t *r);
int resources_register_string(const resource_string_t *r);
void resources_shutdown(void);
int resources_set_value(const char *name, resource_value_t value);
int resources_set_int(const char *name, int value);
int resources_set_string(const char *name, const char *value);
void resources_set_value_event(void *data, int size);
int resources_set_int_sprintf(const char *name, int value, ...) VICE_ATTR_RESPRINTF;
int resources_set_string_sprintf(const char *name, const char *value, ...) VICE_ATTR_RESPRINTF;
int resources_set_value_string(const char *name, const char *value);
int resources_toggle(const char *name, int *new_value_return);
int resources_touch(const char *name);
bool resources_exists(const char *name);
int resources_get_value(const char *name, void *value_return);
int resources_get_int(const char *name, int *value_return);
int resources_get_string(const char *name, const char **value_return);
int resources_get_int_sprintf(const char *name, int *value_return, ...) VICE_ATTR_RESPRINTF;
int resources_get_string_sprintf(const char *name, const char **value_return, ...) VICE_ATTR_RESPRINTF;
int resources_get_default_value(const char *name, void *value_return);
resource_type_t resources_query_type(const char *name);
int resources_save(const char *fname);

/* load resources from a file, keep existing settings */
int resources_load(const char *fname);

/* restore settings to defaults, then load settings from a file. this is used
   for actual settings (vicerc) */
int resources_reset_and_load(const char *fname);
int resources_dump(const char *fname);
void resources_log_active(void);

int resources_write_item_to_file(FILE *fp, const char *name);
int resources_read_item_from_file(FILE *fp);
char *resources_write_item_to_string(const char *name, const char *delim);

int resources_set_defaults(void);
int resources_set_default_int(const char *name, int value);
int resources_set_default_string(const char *name, char *value);

int resources_set_event_safe(void);
void resources_get_event_safe_list(struct event_list_state_s *list);

/* Register a callback for a resource; use name=NULL to register a callback for all.
   Resource-specific callbacks are always called with a valid resource name as parameter.
   Global callbacks may be called with NULL as resource name if many resources changed. */
int resources_register_callback(const char *name, resource_callback_func_t *callback, void *callback_param);

#endif /* _RESOURCES_H */
