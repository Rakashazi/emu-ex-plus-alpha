/*
 * romset.c - Romset file handling.
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "archdep.h"
#include "cmdline.h"
#include "ioutil.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "romset.h"
#include "sysfile.h"
#include "translate.h"
#include "types.h"
#include "util.h"


static log_t romset_log = LOG_DEFAULT;

int romset_resources_init(void)
{
    return 0;
}

void romset_resources_shutdown(void)
{
}

static int option_romsetfile(const char *value, void *extra_param)
{
    return romset_file_load(value);
}

static int option_romsetarchive(const char *value, void *extra_param)
{
    return romset_archive_load(value, 0);
}

static int option_romsetarchiveselect(const char *value, void *extra_param)
{
    return romset_archive_item_select(value);
}

static const cmdline_option_t cmdline_options[] = {
    { "-romsetfile", CALL_FUNCTION, 1,
      option_romsetfile, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_PB_FILE, IDCLS_LOAD_ROMSET_FILE,
      NULL, NULL },
    { "-romsetarchive", CALL_FUNCTION, 1,
      option_romsetarchive, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_PB_FILE, IDCLS_LOAD_ROMSET_ARCHIVE,
      NULL, NULL },
    { "-romsetarchiveselect", CALL_FUNCTION, 1,
      option_romsetarchiveselect, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_ITEM_NUMBER, IDCLS_SELECT_ITEM_FROM_ROMSET_ARCHIVE,
      NULL, NULL },
    { NULL }
};

int romset_cmdline_options_init()
{
    return cmdline_register_options(cmdline_options);
}

const char *prepend_dir_to_path(const char *dir)
{
    const char *saved_path;
    char *new_path;

    resources_get_string("Directory", &saved_path);
    saved_path = lib_stralloc(saved_path);

    if (dir && *dir) {
        new_path = util_concat(dir,
                               ARCHDEP_FINDPATH_SEPARATOR_STRING,
                               saved_path,
                               NULL);
    } else {
        char *current_dir = ioutil_current_dir();
        new_path = util_concat(current_dir,
                               ARCHDEP_FINDPATH_SEPARATOR_STRING,
                               saved_path,
                               NULL);
        lib_free(current_dir);
    }

    resources_set_string("Directory", new_path);
    lib_free(new_path);

    return saved_path;
} 

void restore_path(const char *saved_path)
{
    resources_set_string("Directory", saved_path);
    lib_free(saved_path);
}

int romset_file_load(const char *filename)
{
    FILE *fp;
    int retval, line_num;
    int err = 0;
    char *complete_path, *dir;
    const char *saved_path;

    if (filename == NULL) {
        log_error(romset_log, "ROM set filename is NULL!");
        return -1;
    }

    fp = sysfile_open(filename, &complete_path, MODE_READ_TEXT);

    if (fp == NULL) {
        log_warning(romset_log, "Could not open file '%s' for reading (%s)!",
                    filename, strerror(errno));
        return -1;
    }

    log_message(romset_log, "Loading ROM set from file '%s'", filename);

    /* Prepend dir to search path */
    util_fname_split(complete_path, &dir, NULL);
    saved_path = prepend_dir_to_path(dir);
    lib_free(dir);
    lib_free(complete_path);

    line_num = 0;
    do {
        retval = resources_read_item_from_file(fp);
        switch (retval) {
            case RESERR_TYPE_INVALID:
                log_error(romset_log,
                        "%s: Invalid resource specification at line %d.",
                        filename, line_num);
                err = 1;
                break;
            case RESERR_UNKNOWN_RESOURCE:
                log_warning(romset_log,
                            "%s: Unknown resource specification at line %d.",
                            filename, line_num);
                break;
        }
        line_num++;
    } while (retval != 0);

    /* Restore search path */
    restore_path(saved_path);
    fclose(fp);

    return err;
}

int romset_file_save(const char *filename, const char **resource_list)
{
    FILE *fp;
    char *newname;
    const char *s;

    newname = util_add_extension_const(filename, "vrs");

    fp = fopen(newname, MODE_WRITE_TEXT);

    if (fp == NULL) {
        log_warning(romset_log, "Could not open file '%s' for writing (%s)!",
                    newname, strerror(errno));
        lib_free(newname);
        return -1;
    }

    log_message(romset_log, "Saving ROM set to file '%s'", newname);

    s = *resource_list++;

    while (s != NULL) {
        resources_write_item_to_file(fp, s);
        s = *resource_list++;
    }

    fclose(fp);
    lib_free(newname);

    return 0;
}

char *romset_file_list(const char **resource_list)
{
    char *list;
    const char *s;

    list = lib_stralloc("");
    s = *resource_list++;

    while (s != NULL) {
        char *line;
        line = resources_write_item_to_string(s, ARCHDEP_LINE_DELIMITER);
        if (line != NULL) {
            util_addline_free(&list, line);
        }

        s = *resource_list++;
    }

    return list;
}


typedef struct string_link_s {
    char *name;
    struct string_link_s *next;
} string_link_t;


static int num_romsets = 0;
static int array_size = 0;
static string_link_t *romsets = NULL;
static char *romset_dir;


#define READ_ROM_LINE \
    if ((bptr = fgets(buffer, 256, fp)) != NULL) { \
        line_num++; b = buffer;                    \
        while ((*b == ' ') || (*b == '\t')) {      \
            b++;                                   \
        }                                          \
    }

int romset_archive_load(const char *filename, int autostart)
{
    FILE *fp;
    int line_num;
    char buffer[256];
    string_link_t *autoset = NULL;

    if ((fp = fopen(filename, MODE_READ_TEXT)) == NULL) {
        log_warning(romset_log,
                    "Could not open file '%s' for reading!", filename);
        return -1;
    }

    log_message(romset_log, "Loading ROM set archive from file '%s'", filename);
    lib_free(romset_dir);
    util_fname_split(filename, &romset_dir, NULL);

    line_num = 0;
    while (!feof(fp)) {
        char *b = NULL, *bptr;
        string_link_t *anchor, *item, *last;
        size_t length;
        int entry;

        READ_ROM_LINE;
        if (bptr == NULL) {
            break;
        }
        if ((*b == '\n') || (*b == '#')) {
            continue;
        }
        length = strlen(b);
        for (entry = 0, item = romsets; entry < num_romsets; entry++, item++) {
            if (strncmp(item->name, b, length - 1) == 0) {
                break;
            }
        }
        if (entry >= array_size) {
            array_size += 4;
            romsets = lib_realloc(romsets, array_size * sizeof(string_link_t));
        }
        anchor = romsets + entry;
        if (entry < num_romsets) {
            item = anchor->next;
            while (item != NULL) {
                last = item; item = item->next;
                lib_free(last->name);
                lib_free(last);
            }
        } else {
            anchor->name = lib_malloc(length);
            strncpy(anchor->name, b, length - 1);
            anchor->name[length - 1] = '\0';
        }
        anchor->next = NULL;

        if ((autostart != 0) && (autoset == NULL)) {
            autoset = anchor;
        }

        READ_ROM_LINE
        if ((bptr == NULL) || (*b != '{')) {
            log_warning(romset_log, "Parse error at line %d", line_num);
            fclose(fp);
            return -1;
        }
        last = anchor;
        while (!feof(fp)) {
            READ_ROM_LINE
            if (bptr == NULL) {
                log_warning(romset_log, "Parse error at line %d", line_num);
                fclose(fp);
                return -1;
            }
            if (*b == '}') {
                break;
            }
            length = strlen(b);
            item = lib_malloc(sizeof(string_link_t));
            item->name = lib_malloc(length);
            strncpy(item->name, b, length - 1);
            item->name[length - 1] = '\0';
            item->next = NULL;
            last->next = item;
            last = item;
        }
        if (entry >= num_romsets) {
            num_romsets++;
        }
    }

    fclose(fp);

    if (autoset != 0) {
        romset_archive_item_select(autoset->name);
    }

    return 0;
}

int romset_archive_save(const char *filename)
{
    FILE *fp;
    char *newname;
    char *list;

    newname = util_add_extension_const(filename, "vra");

    /*
     * spiro-20080427:
     * The fopen() is *not* performed with MODE_WRITE_TEXT, but with
     * MODE_WRITE (binary), as the list already contains the
     * platform-specific delimiters! Compare romset_archive_list()
     * below!
     * Otherwise, the .vra file would contain too many delimiters,
     * as it was the case with WinVICE 1.22.
     */

    if ((fp = fopen(newname, MODE_WRITE)) == NULL) {
        log_warning(romset_log,
                    "Could not open file '%s' for writing!", newname);
        lib_free(newname);
        return -1;
    }

    log_message(romset_log, "Saving ROM set archive to file '%s'", newname);

    list = romset_archive_list();
    fputs(list, fp);
    lib_free(list);

    fclose(fp);
    lib_free(newname);

    return 0;
}

char *romset_archive_list(void)
{
    string_link_t *item;
    char *list, *line;
    int i;

    list = lib_stralloc("");

    for (i = 0; i < num_romsets; i++) {
        item = romsets + i;
        line = lib_msprintf("%s" ARCHDEP_LINE_DELIMITER, item->name);
        util_addline_free(&list, line);
        line = lib_msprintf("{" ARCHDEP_LINE_DELIMITER);
        util_addline_free(&list, line);
        while (item->next != NULL) {
            item = item->next;
            line = lib_msprintf("\t%s" ARCHDEP_LINE_DELIMITER, item->name);
            util_addline_free(&list, line);
        }
        line = lib_msprintf("}" ARCHDEP_LINE_DELIMITER);
        util_addline_free(&list, line);
    }

    return list;
}

int romset_archive_item_save(const char *filename, const char *romset_name)
{
    int i;

    for (i = 0; i < num_romsets; i++) {
        if (strcmp(romsets[i].name, romset_name) == 0) {
            string_link_t *item;
            FILE *fp;

            if ((fp = fopen(filename, MODE_WRITE_TEXT)) == NULL) {
                log_warning(romset_log,
                            "Could not open file '%s' for writing", filename);
                return -1;
            }
            item = romsets + i;
            fprintf(fp, "%s\n", item->name);
            fprintf(fp, "{\n");
            while (item->next != NULL) {
                item = item->next;
                fprintf(fp, "\t%s\n", item->name);
            }
            fprintf(fp, "}\n");
            fclose(fp);

            return 0;
        }
    }
    return -1;
}

/*
int romset_archive_item_valid(const char *romset_name)
{
    int i;
    string_link_t *item;

    for (i = 0, item = romsets; i < num_romsets; i++, item++) {
        if (strcmp(romset_name, item->name) == 0)
            return 0;
    }
    return -1;
}
*/

int romset_archive_item_select(const char *romset_name)
{
    int i;
    string_link_t *item;

    for (i = 0, item = romsets; i < num_romsets; i++, item++) {
        if (strcmp(romset_name, item->name) == 0) {
            /* Prepend dir to search path */
            const char *saved_path = prepend_dir_to_path(romset_dir);

            while (item->next != NULL) {
                /* FIXME: Apparently there are no boundary checks! */
                char buffer[256];
                char *b, *d;

                item = item->next;
                b = buffer;
                d = item->name;
                while (*d != '\0') {
                    if (*d == '=') {
                        break;
                    } else {
                        *b++ = *d++;
                    }
                }
                *b++ = '\0';
                if (*d == '=') {
                    resource_type_t tp;
                    char *arg;

                    arg = b; d++;
                    while (*d != '\0') {
                        if (*d != '\"') {
                            *b++ = *d;
                        }
                        d++;
                    }
                    *b++ = '\0';
                    tp = resources_query_type(buffer);
                    switch (tp) {
                        case RES_INTEGER:
                            resources_set_int(buffer, atoi(arg));
                            break;
                        case RES_STRING:
                            resources_set_string(buffer, arg);
                            break;
                        default:
                            b = NULL;
                            break;
                    }
                }
            }
            restore_path(saved_path);
            return 0;
        }
    }
    return -1;
}


int romset_archive_item_create(const char *romset_name,
                               const char **resource_list)
{
    int entry;
    string_link_t *anchor, *item, *last;
    const char **res;

    for (entry = 0, item = romsets; entry < num_romsets; entry++, item++) {
        if (strcmp(romset_name, item->name) == 0) {
            break;
        }
    }
    if (entry >= array_size) {
        array_size += 4;
        romsets = lib_realloc(romsets, array_size * sizeof(string_link_t));
    }
    anchor = romsets + entry;
    if (entry < num_romsets) {
        item = anchor->next;
        while (item != NULL) {
            last = item;
            item = item->next;
            lib_free(last->name);
            lib_free(last);
        }
    } else {
        anchor->name = lib_malloc(strlen(romset_name) + 1);
        strcpy(anchor->name, romset_name);
    }
    anchor->next = NULL;

    last = anchor;
    res = resource_list;
    while (*res != NULL) {
        item = lib_malloc(sizeof(string_link_t));
        item->name = resources_write_item_to_string(*res, "");
        item->next = NULL;
        last->next = item;
        last = item;
        res++;
    }

    if (entry >= num_romsets) {
        num_romsets++;
    }

    return 0;
}


int romset_archive_item_delete(const char *romset_name)
{
    string_link_t *item;
    int i;

    for (i = 0, item = romsets; i < num_romsets; i++, item++) {
        if (strcmp(romset_name, item->name) == 0) {
            string_link_t *last;

            lib_free(item->name);
            item = item->next;
            while (item != NULL) {
                last = item;
                item = item->next;
                lib_free(last->name);
                lib_free(last);
            }
            while (i < num_romsets - 1) {
                romsets[i] = romsets[i + 1]; i++;
            }
            num_romsets--;

            return 0;
        }
    }
    return -1;
}


void romset_archive_clear(void)
{
    int i;
    string_link_t *item, *last;

    for (i = 0; i < num_romsets; i++) {
        item = romsets + i;
        lib_free(item->name);
        item = item->next;
        while (item != NULL) {
            last = item;
            item = item->next;
            lib_free(last->name);
            lib_free(last);
        }
    }
    if (romsets != NULL) {
        lib_free(romsets);
        romsets = NULL;
    }

    num_romsets = 0;
    array_size = 0;
    lib_free(romset_dir);
    romset_dir = NULL;
}

int romset_archive_get_number(void)
{
    return num_romsets;
}


char *romset_archive_get_item(int number)
{
    if ((number < 0) || (number >= num_romsets)) {
        return NULL;
    }

    return romsets[number].name;
}

void romset_init(void)
{
    romset_log = log_open("Romset");

    machine_romset_init();
}
