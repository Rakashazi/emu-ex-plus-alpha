/*
 * resources.c - Resource (setting) handling for VICE.
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

/* This implements simple facilities to handle the resources and command-line
   options.  All the resources for the emulators can be stored in a single
   file, and they are separated by an `emulator identifier', i.e. the machine
   name between brackets (e.g. ``[C64]'').  All the resources are stored in
   the form ``ResourceName=ResourceValue'', and separated by newline
   characters.  Leading and trailing spaces are removed from the
   ResourceValue unless it is put between quotes (").  */

#include "vice.h"

/* #define VICE_DEBUG_RESOURCES */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "archdep.h"
#include "ioutil.h"
#include "lib.h"
#include "log.h"
#include "network.h"
#include "resources.h"
#include "util.h"
#include "vice-event.h"

#ifdef VICE_DEBUG_RESOURCES
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

typedef struct resource_ram_s {
    /* Resource name.  */
    char *name;

    /* Type of resource.  */
    resource_type_t type;

    /* Factory default value.  */
    resource_value_t factory_value;

    /* Is the resource important for history recording or netplay? */
    resource_event_relevant_t event_relevant;

    /* Value that is needed for correct history recording and netplay.  */
    resource_value_t *event_strict_value;

    /* Pointer to the value.  This is only used for *reading* it.  To change
       it, use `set_func'.  */
    resource_value_t *value_ptr;

    /* Function to call to set the integer value.  */
    resource_set_func_int_t *set_func_int;

    /* Function to call to set the string value.  */
    resource_set_func_string_t *set_func_string;

    /* Extra parameter to pass to `set_func'.  */
    void *param;

    /* callback function vector chain */
    struct resource_callback_desc_s *callback;

    /* number of next entry in hash collision list */
    int hash_next;
} resource_ram_t;

/* the type of the callback vector chain */
typedef struct resource_callback_desc_s {
    resource_callback_func_t *func;
    void *param;
    struct resource_callback_desc_s *next;
} resource_callback_desc_t;


static unsigned int num_resources, num_allocated_resources;
static resource_ram_t *resources;
static char *machine_id = NULL;

static void write_resource_item(FILE *f, int num);
static char *string_resource_item(int num, const char *delim);

/* use a hash table with 1024 entries */
static const unsigned int logHashSize = 10;

static int *hashTable = NULL;

static resource_callback_desc_t *resource_modified_callback = NULL;

/* calculate the hash key */
static unsigned int resources_calc_hash_key(const char *name)
{
    unsigned int key, i, shift;

    DBG(("resources_calc_hash_key: '%s'\n", name ? name : "<empty/null>"));

    key = 0; shift = 0;
    for (i = 0; name[i] != '\0'; i++) {
        /* resources are case-insensitive */
        unsigned int sym = (unsigned int)tolower((int)name[i]);

        if (shift >= logHashSize) {
            shift = 0;
        }

        key ^= (sym << shift);
        if (shift + 8 > logHashSize) {
            key ^= (((unsigned int)sym) >> (logHashSize - shift));
        }
        shift++;
    }
    return (key & ((1 << logHashSize) - 1));
}


/* add a new callback function at the head of the vector chain */
static void resources_add_callback(resource_callback_desc_t **where,
                                   resource_callback_func_t *callback,
                                   void *param)
{
    if (callback != NULL) {
        resource_callback_desc_t *cbd;

        cbd = lib_malloc(sizeof(resource_callback_desc_t));
        cbd->func = callback;
        cbd->param = param;
        cbd->next = *where;
        *where = cbd;
    }
}


/* execute a callback vector chain */
static void resources_exec_callback_chain(const resource_callback_desc_t
                                          *callbacks, const char *name)
{
    const resource_callback_desc_t *cbd = callbacks;

    while (cbd != NULL) {
        (*cbd->func)(name, cbd->param);
        cbd = cbd->next;
    }
}


/* issue callbacks for a modified resource */
static void resources_issue_callback(resource_ram_t *res, int global_callback)
{
    if (res->callback != NULL) {
        resources_exec_callback_chain(res->callback, res->name);
    }

    if ((global_callback != 0) && (resource_modified_callback != NULL)) {
        resources_exec_callback_chain(resource_modified_callback, res->name);
    }
}


#if 0
/* for debugging (hash collisions, hash chains, ...) */
static void resources_check_hash_table(FILE *f)
{
    int i, entries;

    for (i = 0, entries = 0; i < (1 << logHashSize); i++) {
        if (hashTable[i] >= 0) {
            int next;

            fprintf(f, "%d: %s", i, resources[hashTable[i]].name);
            next = resources[hashTable[i]].hash_next;
            while (next >= 0) {
                fprintf(f, " -> %s", resources[next].name);
                next = resources[next].hash_next;
            }
            fprintf(f, "\n");
            entries++;
        }
    }
    fprintf(f, "NUM %d, ENTIES %d\n", num_resources, entries);
}
#endif

static resource_ram_t *lookup(const char *name)
{
    resource_ram_t *res;
    unsigned int hashkey;

    if (name == NULL) {
        return NULL;
    }
    hashkey = resources_calc_hash_key(name);
    res = (hashTable[hashkey] >= 0) ? resources + hashTable[hashkey] : NULL;
    while (res != NULL) {
        if (strcasecmp(res->name, name) == 0) {
            return res;
        }
        res = (res->hash_next >= 0) ? resources + res->hash_next : NULL;
    }
    return NULL;
}

/* Configuration filename set via -config */
char *vice_config_file = NULL;

/* ------------------------------------------------------------------------- */
/* register an array(!) of integer resources */
int resources_register_int(const resource_int_t *r)
{
    const resource_int_t *sp;
    resource_ram_t *dp;

    DBG(("resources_register_int name:'%s'\n", r->name ? r->name : "<empty/null>"));

    sp = r;
    dp = resources + num_resources;
    while (sp->name != NULL) {
        unsigned int hashkey;

        if (sp->value_ptr == NULL || sp->set_func == NULL) {
            archdep_startup_log_error(
                "Inconsistent resource declaration '%s'.\n", sp->name);
            return -1;
        }

        if (lookup(sp->name)) {
            archdep_startup_log_error(
                "Duplicated resource declaration '%s'.\n", sp->name);
            return -1;
        }

        if (num_allocated_resources <= num_resources) {
            num_allocated_resources *= 2;
            resources = lib_realloc(resources, num_allocated_resources
                                    * sizeof(resource_ram_t));
            dp = resources + num_resources;
        }

        dp->name = lib_stralloc(sp->name);
        dp->type = RES_INTEGER;
        dp->factory_value = uint_to_void_ptr(sp->factory_value);
        dp->value_ptr = (void *)(sp->value_ptr);
        dp->event_relevant = sp->event_relevant;
        dp->event_strict_value = sp->event_strict_value;
        dp->set_func_int = sp->set_func;
        dp->param = sp->param;
        dp->callback = NULL;

        hashkey = resources_calc_hash_key(sp->name);
        dp->hash_next = hashTable[hashkey];
        hashTable[hashkey] = (int)(dp - resources);

        num_resources++, sp++, dp++;
    }

    return 0;
}

int resources_register_string(const resource_string_t *r)
{
    const resource_string_t *sp;
    resource_ram_t *dp;

    DBG(("resources_register_string name:'%s'\n", r->name ? r->name : "<empty/null>"));

    sp = r;
    dp = resources + num_resources;
    while (sp->name != NULL) {
        unsigned int hashkey;

        if (sp->factory_value == NULL
            || sp->value_ptr == NULL || sp->set_func == NULL) {
            archdep_startup_log_error(
                "Inconsistent resource declaration '%s'.\n", sp->name);
            return -1;
        }

        if (lookup(sp->name)) {
            archdep_startup_log_error(
                "Duplicated resource declaration '%s'.\n", sp->name);
            return -1;
        }

        if (num_allocated_resources <= num_resources) {
            num_allocated_resources *= 2;
            resources = lib_realloc(resources, num_allocated_resources
                                    * sizeof(resource_ram_t));
            dp = resources + num_resources;
        }

        dp->name = lib_stralloc(sp->name);
        dp->type = RES_STRING;
        dp->factory_value = (resource_value_t)(sp->factory_value);
        dp->value_ptr = (void *)(sp->value_ptr);
        dp->event_relevant = sp->event_relevant;
        dp->event_strict_value = sp->event_strict_value;
        dp->set_func_string = sp->set_func;
        dp->param = sp->param;
        dp->callback = NULL;

        hashkey = resources_calc_hash_key(sp->name);
        dp->hash_next = hashTable[hashkey];
        hashTable[hashkey] = (int)(dp - resources);

        num_resources++, sp++, dp++;
    }

    return 0;
}


static void resources_free(void)
{
    unsigned int i;

    for (i = 0; i < num_resources; i++) {
        lib_free((resources + i)->name);
    }
}


/** \brief  Shutown resources
 */
void resources_shutdown(void)
{
    resources_free();

    lib_free(resources);
    lib_free(hashTable);
    lib_free(machine_id);
    lib_free(vice_config_file);
}

resource_type_t resources_query_type(const char *name)
{
    resource_ram_t *res;

    if ((res = lookup(name)) != NULL) {
        return res->type;
    } else {
        return (resource_type_t)-1;
    }
}

int resources_write_item_to_file(FILE *fp, const char *name)
{
    resource_ram_t *res = lookup(name);

    if (res != NULL) {
        write_resource_item(fp, (int)(res - resources));
        return 0;
    }
    log_warning(LOG_DEFAULT, "Trying to save unknown resource '%s'", name);

    return -1;
}

char *resources_write_item_to_string(const char *name, const char *delim)
{
    resource_ram_t *res = lookup(name);

    if (res != NULL) {
        return string_resource_item((int)(res - resources), delim);
    }

    log_warning(LOG_DEFAULT, "Trying to save unknown resource '%s'", name);

    return NULL;
}

static void resource_create_event_data(char **event_data, int *data_size,
                                       resource_ram_t *r,
                                       resource_value_t value)
{
    int name_size;
    const char *name = r->name;

    name_size = (int)strlen(name) + 1;

    if (r->type == RES_INTEGER) {
        *data_size = name_size + sizeof(DWORD);
    } else {
        *data_size = name_size + (int)strlen((char *)value) + 1;
    }

    *event_data = lib_malloc(*data_size);
    strcpy(*event_data, name);

    if (r->type == RES_INTEGER) {
        *(DWORD *)(*event_data + name_size) = vice_ptr_to_uint(value);
    } else {
        strcpy(*event_data + name_size, (char *)value);
    }
}

static void resource_record_event(resource_ram_t *r,
                                  resource_value_t value)
{
    char *event_data;
    int data_size;

    resource_create_event_data(&event_data, &data_size, r, value);

    network_event_record(EVENT_RESOURCE, event_data, data_size);

    lib_free(event_data);
}

/* ------------------------------------------------------------------------- */

int resources_init(const char *machine)
{
    unsigned int i;

    machine_id = lib_stralloc(machine);
    num_allocated_resources = 100;
    num_resources = 0;
    resources = lib_malloc(num_allocated_resources * sizeof(resource_ram_t));

    /* hash table maps hash keys to index in resources array rather than
       pointers into the array because the array may be reallocated. */
    hashTable = lib_malloc((1 << logHashSize) * sizeof(int));

    for (i = 0; i < (unsigned int)(1 << logHashSize); i++) {
        hashTable[i] = -1;
    }

    return 0;
}

static int resources_set_value_internal(resource_ram_t *r,
                                        resource_value_t value)
{
    int status = 0;

    switch (r->type) {
        case RES_INTEGER:
            status = (*r->set_func_int)(vice_ptr_to_int(value), r->param);
            break;
        case RES_STRING:
            status = (*r->set_func_string)((const char *)value, r->param);
            break;
    }

    if (status != 0) {
        resources_issue_callback(r, 1);
    }

    return status;
}

int resources_set_value(const char *name, resource_value_t value)
{
    resource_ram_t *r = lookup(name);

    if (r == NULL) {
        log_warning(LOG_DEFAULT,
                    "Trying to assign value to unknown "
                    "resource `%s'.", name);
        return -1;
    }

    if (r->event_relevant == RES_EVENT_STRICT
        && network_get_mode() != NETWORK_IDLE) {
        return -2;
    }

    if (r->event_relevant == RES_EVENT_SAME && network_connected()) {
        resource_record_event(r, value);
        return 0;
    }

    return resources_set_value_internal(r, value);
}

static int resources_set_internal_int(resource_ram_t *r, int value)
{
    int status = 0;

    switch (r->type) {
        case RES_INTEGER:
            status = (*r->set_func_int)(value, r->param);
            break;
        default:
            return -1;
    }

    if (status != 0) {
        resources_issue_callback(r, 1);
    }

    return status;
}

static int resources_set_internal_string(resource_ram_t *r,
                                         const char *value)
{
    int status = 0;

    switch (r->type) {
        case RES_STRING:
            status = (*r->set_func_string)(value, r->param);
            break;
        default:
            return -1;
    }

    if (status != 0) {
        resources_issue_callback(r, 1);
    }

    return status;
}

int resources_set_int(const char *name, int value)
{
    resource_ram_t *r = lookup(name);

    if (r == NULL) {
        log_warning(LOG_DEFAULT,
                    "Trying to assign value to unknown "
                    "resource `%s'.", name);
        return -1;
    }

    if (r->event_relevant == RES_EVENT_STRICT && network_get_mode() != NETWORK_IDLE) {
        return -2;
    }

    if (r->event_relevant == RES_EVENT_SAME && network_connected()) {
        resource_record_event(r, uint_to_void_ptr(value));
        return 0;
    }

    return resources_set_internal_int(r, value);
}

int resources_set_string(const char *name, const char *value)
{
    resource_ram_t *r = lookup(name);

    if (r == NULL) {
        log_warning(LOG_DEFAULT,
                    "Trying to assign value to unknown "
                    "resource `%s'.", name);
        return -1;
    }

    if (r->event_relevant == RES_EVENT_STRICT && network_get_mode() != NETWORK_IDLE) {
        return -2;
    }

    if (r->event_relevant == RES_EVENT_SAME && network_connected()) {
        resource_record_event(r, (resource_value_t)value);
        return 0;
    }

    return resources_set_internal_string(r, value);
}

void resources_set_value_event(void *data, int size)
{
    char *name;
    char *valueptr;
    resource_ram_t *r;

    name = data;
    valueptr = name + strlen(name) + 1;
    r = lookup(name);
    if (r->type == RES_INTEGER) {
        resources_set_value_internal(r, (resource_value_t) uint_to_void_ptr(*(DWORD*)valueptr));
    } else {
        resources_set_value_internal(r, (resource_value_t)valueptr);
    }
}

int resources_set_int_sprintf(const char *name, int value, ...)
{
    va_list args;
    char *resname;
    int result;

    va_start(args, value);
    resname = lib_mvsprintf(name, args);
    va_end(args);

    result = resources_set_int(resname, value);
    lib_free(resname);

    return result;
}

int resources_set_string_sprintf(const char *name, const char *value, ...)
{
    va_list args;
    char *resname;
    int result;

    va_start(args, value);
    resname = lib_mvsprintf(name, args);
    va_end(args);

    result = resources_set_string(resname, value);
    lib_free(resname);

    return result;
}

int resources_set_value_string(const char *name, const char *value)
{
    resource_ram_t *r = lookup(name);
    int status;

    if (r == NULL) {
        log_warning(LOG_DEFAULT,
                    "Trying to assign value to unknown "
                    "resource `%s'.", name);
        return -1;
    }

    switch (r->type) {
        case RES_INTEGER:
            {
                char *endptr;
                int int_value;

                int_value = (int)strtol(value, &endptr, 0);

                if (*endptr == '\0') {
                    status = (*r->set_func_int)(int_value, r->param);
                } else {
                    status = -1;
                }
            }
            break;
        case RES_STRING:
            status = (*r->set_func_string)(value, r->param);
            break;
        default:
            log_warning(LOG_DEFAULT, "Unknown resource type for `%s'", name);
            status = -1;
            break;
    }

    if (status != 0) {
        resources_issue_callback(r, 1);
    }

    return status;
}

int resources_get_value(const char *name, void *value_return)
{
    resource_ram_t *r = lookup(name);

    if (r == NULL) {
        log_warning(LOG_DEFAULT,
                    "Trying to read value from unknown "
                    "resource `%s'.", name);
        return -1;
    }

    switch (r->type) {
        case RES_INTEGER:
            *(int *)value_return = *(int *)r->value_ptr;
            break;
        case RES_STRING:
            *(char **)value_return = *(char **)r->value_ptr;
            break;
        default:
            log_warning(LOG_DEFAULT, "Unknown resource type for `%s'", name);
            return -1;
    }

    return 0;
}

int resources_get_int(const char *name, int *value_return)
{
    resource_ram_t *r = lookup(name);

    if (r == NULL) {
        log_warning(LOG_DEFAULT,
                    "Trying to read value from unknown "
                    "resource `%s'.", name);
        return -1;
    }

    switch (r->type) {
        case RES_INTEGER:
            *value_return = *(int *)r->value_ptr;
            break;
        default:
            log_warning(LOG_DEFAULT, "Unknown resource type for `%s'", name);
            return -1;
    }

    return 0;
}

int resources_get_string(const char *name, const char **value_return)
{
    resource_ram_t *r = lookup(name);

    if (r == NULL) {
        log_warning(LOG_DEFAULT,
                    "Trying to read value from unknown "
                    "resource `%s'.", name);
        return -1;
    }

    switch (r->type) {
        case RES_STRING:
            *value_return = *(const char **)r->value_ptr;
            break;
        default:
            log_warning(LOG_DEFAULT, "Unknown resource type for `%s'", name);
            return -1;
    }

    return 0;
}

int resources_get_int_sprintf(const char *name, int *value_return, ...)
{
    va_list args;
    char *resname;
    int result;

    va_start(args, value_return);
    resname = lib_mvsprintf(name, args);
    va_end(args);

    result = resources_get_int(resname, value_return);
    lib_free(resname);

    return result;
}

int resources_get_string_sprintf(const char *name, const char **value_return,
                                 ...)
{
    va_list args;
    char *resname;
    int result;

    va_start(args, value_return);
    resname = lib_mvsprintf(name, args);
    va_end(args);

    result = resources_get_string(resname, value_return);
    lib_free(resname);

    return result;
}

int resources_set_default_int(const char *name, int value)
{
    resource_ram_t *r = lookup(name);

    if (r == NULL) {
        log_warning(LOG_DEFAULT,
                    "Trying to assign default to unknown "
                    "resource `%s'.", name);
        return -1;
    }

    r->factory_value = uint_to_void_ptr(value);
    return 0;
}

int resources_set_default_string(const char *name, char *value)
{
    resource_ram_t *r = lookup(name);

    if (r == NULL) {
        log_warning(LOG_DEFAULT,
                    "Trying to assign default to unknown "
                    "resource `%s'.", name);
        return -1;
    }
    /* since these pointers are usually static/not allocated, we just
       assign it here and don't free() as one might expect */
    r->factory_value = value;
    return 0;
}

int resources_get_default_value(const char *name, void *value_return)
{
    resource_ram_t *r = lookup(name);

    if (r == NULL) {
        log_warning(LOG_DEFAULT,
                    "Trying to read value from unknown "
                    "resource `%s'.", name);
        return -1;
    }

    switch (r->type) {
        case RES_INTEGER:
            *(int *)value_return = vice_ptr_to_int(r->factory_value);
            break;
        case RES_STRING:
            *(char **)value_return = (char *)(r->factory_value);
            break;
        default:
            log_warning(LOG_DEFAULT, "Unknown resource type for `%s'", name);
            return -1;
    }

    return 0;
}

/* FIXME: make event safe */
int resources_set_defaults(void)
{
    unsigned int i;

    for (i = 0; i < num_resources; i++) {
        switch (resources[i].type) {
            case RES_INTEGER:
                if ((*resources[i].set_func_int)(vice_ptr_to_int(resources[i].factory_value),
                                                 resources[i].param) < 0) {
                    log_verbose("Cannot set resource %s", resources[i].name);
                    return -1;
                }
                break;
            case RES_STRING:
                if ((*resources[i].set_func_string)((const char *)(resources[i].factory_value),
                                                    resources[i].param) < 0) {
                    log_verbose("Cannot set resource %s", resources[i].name);
                    return -1;
                }
                break;
        }

        resources_issue_callback(resources + i, 0);
    }

    if (resource_modified_callback != NULL) {
        resources_exec_callback_chain(resource_modified_callback, NULL);
    }

    return 0;
}

int resources_set_event_safe(void)
{
    unsigned int i;

    for (i = 0; i < num_resources; i++) {
        switch (resources[i].type) {
            case RES_INTEGER:
                if (resources[i].event_relevant == RES_EVENT_STRICT) {
                    if ((*resources[i].set_func_int)(vice_ptr_to_int(resources[i].event_strict_value),
                                                     resources[i].param) < 0) {
                        return -1;
                    }
                }
                break;
            case RES_STRING:
                if (resources[i].event_relevant == RES_EVENT_STRICT) {
                    if ((*resources[i].set_func_string)((const char *)(resources[i].event_strict_value),
                                                        resources[i].param) < 0) {
                        return -1;
                    }
                }
                break;
        }
        resources_issue_callback(resources + i, 0);
    }

    if (resource_modified_callback != NULL) {
        resources_exec_callback_chain(resource_modified_callback, NULL);
    }

    return 0;
}

void resources_get_event_safe_list(event_list_state_t *list)
{
    unsigned int i;
    char *event_data;
    int data_size;

    for (i = 0; i < num_resources; i++) {
        if (resources[i].event_relevant == RES_EVENT_SAME) {
            resource_create_event_data(&event_data, &data_size,
                                       &resources[i],
                                       *(resources[i].value_ptr));
            event_record_in_list(list, EVENT_RESOURCE,
                                 event_data, data_size);
            lib_free(event_data);
        }
    }
    event_record_in_list(list, EVENT_LIST_END, NULL, 0);
}

int resources_toggle(const char *name, int *new_value_return)
{
    resource_ram_t *r = lookup(name);
    int value;

    if (r == NULL) {
        log_warning(LOG_DEFAULT,
                    "Trying to toggle boolean value of unknown "
                    "resource `%s'.", name);
        return -1;
    }

    value = !(*(int *)r->value_ptr);

    if (r->event_relevant == RES_EVENT_STRICT && network_get_mode() != NETWORK_IDLE) {
        return -2;
    }

    if (new_value_return != NULL) {
        *new_value_return = value;
    }

    if (r->event_relevant == RES_EVENT_SAME && network_connected()) {
        resource_record_event(r, uint_to_void_ptr(value));
        return 0;
    }

    return resources_set_internal_int(r, value);
}

int resources_touch(const char *name)
{
    void *tmp;

    if (resources_get_value(name, (resource_value_t *)&tmp) < 0) {
        return -1;
    }

    return resources_set_value(name, (resource_value_t)tmp);
}

/* ------------------------------------------------------------------------- */

/* Check whether `buf' is the emulator ID for the machine we are emulating.  */
static int check_emu_id(const char *buf)
{
    size_t machine_id_len, buf_len;

    buf_len = strlen(buf);
    if (*buf != '[' || *(buf + buf_len - 1) != ']') {
        return 0;
    }

    if (machine_id == NULL) {
        return 1;
    }

    machine_id_len = strlen(machine_id);
    if (machine_id_len != buf_len - 2) {
        return 0;
    }

    if (strncmp(buf + 1, machine_id, machine_id_len) == 0) {
        return 1;
    } else {
        return 0;
    }
}

/* ------------------------------------------------------------------------- */

/* Read one resource line from the file descriptor `f'.
   Returns:
    1 on success,
    0 on EOF or end of emulator section.
   -1 on general error
   RESERR_TYPE_INVALID on parse/type error
   RESERR_UNKNOWN_RESOURCE on unknown resource error
*/
/* FIXME: make event safe */
int resources_read_item_from_file(FILE *f)
{
    char buf[1024];
    char *arg_ptr;
    int line_len, resname_len;
    size_t arg_len;
    resource_ram_t *r;

    line_len = util_get_line(buf, 1024, f);

    if (line_len < 0) {
        return 0;
    }

    /* Ignore empty lines.  */
    if (*buf == '\0') {
        return 1;
    }

    if (*buf == '[') {
        /* End of emulator-specific section.  */
        return 0;
    }

    arg_ptr = strchr(buf, '=');
    if (!arg_ptr) {
        return -1;
    }

    resname_len = (int)(arg_ptr - buf);
    arg_ptr++;
    arg_len = strlen(arg_ptr);

    /* If the value is between quotes, remove them.  */
    if (*arg_ptr == '"' && *(arg_ptr + arg_len - 1) == '"') {
        *(arg_ptr + arg_len - 1) = '\0';
        arg_ptr++;
    }

    *(buf + resname_len) = '\0';

    {
        int result;

        r = lookup(buf);
        if (r == NULL) {
            log_error(LOG_DEFAULT, "Unknown resource `%s'.", buf);
            return RESERR_UNKNOWN_RESOURCE;
        }

        switch (r->type) {
            case RES_INTEGER:
                result = (*r->set_func_int)(atoi(arg_ptr), r->param);
                break;
            case RES_STRING:
                result = (*r->set_func_string)(arg_ptr, r->param);
                break;
            default:
                log_error(LOG_DEFAULT, "Unknown resource type for `%s'.",
                          r->name);
                result = RESERR_TYPE_INVALID;
                break;
        }

        if (result < 0) {
            switch (r->type) {
                case RES_INTEGER:
                case RES_STRING:
                    log_error(LOG_DEFAULT, "Cannot assign value `%s' to resource `%s'.", arg_ptr, r->name);
                    break;
                default:
                    log_error(LOG_DEFAULT, "Cannot assign value to resource `%s'.", r->name);
                    break;
            }
            return -1;
        }

        resources_issue_callback(r, 0);

        return 1;
    }
}

/* Load the resources from file `fname'.  If `fname' is NULL, load them from
   the default resource file.  */
int resources_load(const char *fname)
{
    FILE *f;
    int retval;
    int line_num;
    int err = 0;
    char *default_name = NULL;

    if (fname == NULL) {
        if (vice_config_file == NULL) {
            default_name = archdep_default_resource_file_name();
        } else {
            default_name = lib_stralloc(vice_config_file);
        }
        fname = default_name;
    }

    f = fopen(fname, MODE_READ_TEXT);

    if (f == NULL) {
        lib_free(default_name);
        return RESERR_FILE_NOT_FOUND;
    }

    log_message(LOG_DEFAULT, "Reading configuration file `%s'.", fname);

    /* Find the start of the configuration section for this emulator.  */
    for (line_num = 1;; line_num++) {
        char buf[1024];

        if (util_get_line(buf, 1024, f) < 0) {
            lib_free(default_name);
            fclose(f);
            return RESERR_READ_ERROR;
        }

        if (check_emu_id(buf)) {
            line_num++;
            break;
        }
    }

    do {
        retval = resources_read_item_from_file(f);
        switch (retval) {
            case RESERR_TYPE_INVALID:
                    log_error(LOG_DEFAULT,
                            "%s: Invalid resource specification at line %d.",
                            fname, line_num);
                    err = 1;
                break;
            case RESERR_UNKNOWN_RESOURCE:
                    log_warning(LOG_DEFAULT,
                                "%s: Unknown resource specification at line %d.",
                                fname, line_num);
                break;
        }
        line_num++;
    } while (retval != 0);

    fclose(f);
    lib_free(default_name);

    if (resource_modified_callback != NULL) {
        resources_exec_callback_chain(resource_modified_callback, NULL);
    }

    return err ? RESERR_FILE_INVALID : 0;
}


static char *string_resource_item(int num, const char *delim)
{
    char *line = NULL;
    resource_value_t v;

    switch (resources[num].type) {
        case RES_INTEGER:
            v = (resource_value_t) uint_to_void_ptr(*(int *)resources[num].value_ptr);
            line = lib_msprintf("%s=%d%s", resources[num].name, vice_ptr_to_int(v), delim);
            break;
        case RES_STRING:
            v = *resources[num].value_ptr;
            if ((char *)v != NULL) {
                line = lib_msprintf("%s=\"%s\"%s", resources[num].name, (char *)v,
                                    delim);
            } else {
                line = lib_msprintf("%s=%s", resources[num].name, delim);
            }
            break;
        default:
            log_error(LOG_DEFAULT, "Unknown value type for resource `%s'.",
                      resources[num].name);
            break;
    }
    return line;
}

/* Write the resource specification for resource number `num' to file
   descriptor `f'.  */
static void write_resource_item(FILE *f, int num)
{
    char *line;

    line = string_resource_item(num, "\n");

    if (line != NULL) {
        fputs(line, f);
        lib_free(line);
    }
}

/* check if a resource contains its default value */
static int resource_item_isdefault(int num)
{
    int i1, i2;
    char *s1, *s2;
    resource_value_t v;

    switch (resources[num].type) {
        case RES_INTEGER:
            v = (resource_value_t) uint_to_void_ptr(*(int *)resources[num].value_ptr);
            i1 = vice_ptr_to_int(v);
            i2 = vice_ptr_to_int(resources[num].factory_value);
            if (i1 == i2) {
                return 1;
            }
            DBG(("%s = (int) default: \"%d\" is: \"%d\"\n", resources[num].name, i2, i1));
            break;
        case RES_STRING:
            v = *resources[num].value_ptr;
            s1 = (char *)v == NULL ? "" : (char *)v;
            s2 = (char *)resources[num].factory_value == NULL ? "" : (char *)resources[num].factory_value;
            if (!strcmp(s1, s2)) {
                return 1;
            }
            DBG(("%s = (string) default: \"%s\" is: \"%s\"\n", resources[num].name, s2, s1));
            break;
        default:
            log_error(LOG_DEFAULT, "Unknown value type for resource `%s'.", resources[num].name);
            break;
    }
    return 0;
}

/* Save all the resources into file `fname'.  If `fname' is NULL, save them
   in the default resource file.  Writing the resources does not destroy the
   resources for the other emulators.  */
int resources_save(const char *fname)
{
    char *backup_name = NULL;
    FILE *in_file = NULL, *out_file;
    unsigned int i;
    char *default_name = NULL;

    /* get name for config file */
    if (fname == NULL) {
        if (vice_config_file == NULL) {
            /* get default filename. this also creates the .vice directory if not present */
            default_name = archdep_default_save_resource_file_name();
        } else {
            default_name = lib_stralloc(vice_config_file);
        }
        fname = default_name;
    }

    /* make a backup of an existing config, open it */
    if (util_file_exists(fname) != 0) {
        /* try to open it */
        if (ioutil_access(fname, IOUTIL_ACCESS_W_OK) != 0) {
            lib_free(default_name);
            return RESERR_WRITE_PROTECTED;
        }
        /* get backup name */
        backup_name = archdep_make_backup_filename(fname);
        /* if backup exists, remove it */
        if (util_file_exists(backup_name) != 0) {
            if (ioutil_access(backup_name, IOUTIL_ACCESS_W_OK) != 0) {
                lib_free(backup_name);
                lib_free(default_name);
                return RESERR_WRITE_PROTECTED;
            }
            if (ioutil_remove(backup_name) != 0) {
                lib_free(backup_name);
                lib_free(default_name);
                return RESERR_CANNOT_REMOVE_BACKUP;
            }
        }
        /* move existing config to backup */
        if (ioutil_rename(fname, backup_name) != 0) {
            lib_free(backup_name);
            lib_free(default_name);
            return RESERR_CANNOT_RENAME_FILE;
        }
        /* open the old config */
        in_file = fopen(backup_name, MODE_READ_TEXT);
        if (!in_file) {
            lib_free(backup_name);
            return RESERR_READ_ERROR;
        }
    }

    log_message(LOG_DEFAULT, "Writing configuration file `%s'.", fname);

    out_file = fopen(fname, MODE_WRITE_TEXT);

    if (!out_file) {
        if (in_file != NULL) {
            fclose(in_file);
        }
        lib_free(backup_name);
        lib_free(default_name);
        return RESERR_CANNOT_CREATE_FILE;
    }

    setbuf(out_file, NULL);

    /* Copy the configuration for the other emulators.  */
    if (in_file != NULL) {
        while (1) {
            char buf[1024];

            if (util_get_line(buf, 1024, in_file) < 0) {
                break;
            }

            if (check_emu_id(buf)) {
                break;
            }

            fprintf(out_file, "%s\n", buf);
        }
    }

    /* Write our current configuration.  */
    fprintf(out_file, "[%s]\n", machine_id);
    for (i = 0; i < num_resources; i++) {
        /* only dump into the file what is different to the default config */
        if (!resource_item_isdefault(i)) {
            write_resource_item(out_file, i);
        }
    }
    fprintf(out_file, "\n");

    if (in_file != NULL) {
        char buf[1024];

        /* Skip the old configuration for this emulator.  */
        while (1) {
            if (util_get_line(buf, 1024, in_file) < 0) {
                break;
            }

            /* Check if another emulation section starts.  */
            if (*buf == '[') {
                fprintf(out_file, "%s\n", buf);
                break;
            }
        }

        if (!feof(in_file)) {
            /* Copy the configuration for the other emulators.  */
            while (util_get_line(buf, 1024, in_file) >= 0) {
                fprintf(out_file, "%s\n", buf);
            }
        }
        fclose(in_file);
        /* remove the backup */
        ioutil_remove(backup_name);
    }

    fclose(out_file);
    lib_free(backup_name);
    lib_free(default_name);
    return 0;
}

/* dump ALL resources of the current machine into a file */
int resources_dump(const char *fname)
{
    FILE *out_file;
    unsigned int i;

    log_message(LOG_DEFAULT, "Dumping %d resources to file `%s'.", num_resources, fname);

    out_file = fopen(fname, MODE_WRITE_TEXT);
    if (!out_file) {
        return RESERR_CANNOT_CREATE_FILE;
    }

    setbuf(out_file, NULL);

    /* Write our current configuration.  */
    fprintf(out_file, "[%s]\n", machine_id);
    for (i = 0; i < num_resources; i++) {
        write_resource_item(out_file, i);
    }
    fprintf(out_file, "\n");

    fclose(out_file);
    return 0;
}

int resources_register_callback(const char *name,
                                resource_callback_func_t *callback,
                                void *callback_param)
{
    if (name == NULL) {
        resources_add_callback(&resource_modified_callback, callback,
                               callback_param);
        return 0;
    } else {
        resource_ram_t *res = lookup(name);

        if (res != NULL) {
            resources_add_callback(&(res->callback), callback, callback_param);
            return 0;
        }
    }
    return -1;
}
