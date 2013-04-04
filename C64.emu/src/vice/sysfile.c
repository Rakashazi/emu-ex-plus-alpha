/*
 * sysfile.c - Simple locator for VICE system files.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "cmdline.h"
#include "embedded.h"
#include "findpath.h"
#include "ioutil.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "sysfile.h"
#include "translate.h"
#include "util.h"

/* Resources.  */

static char *default_path = NULL;
static char *system_path = NULL;
static char *expanded_system_path = NULL;

static int set_system_path(const char *val, void *param)
{
    char *tmp_path, *tmp_path_save, *p, *s, *current_dir;

    util_string_set(&system_path, val);

    lib_free(expanded_system_path);
    expanded_system_path = NULL; /* will subsequently be replaced */

    tmp_path_save = util_subst(system_path, "$$", default_path); /* malloc'd */

    current_dir = ioutil_current_dir();

    tmp_path = tmp_path_save; /* tmp_path points into tmp_path_save */
    do {
        p = strstr(tmp_path, ARCHDEP_FINDPATH_SEPARATOR_STRING);

        if (p != NULL) {
            *p = 0;
        }
        if (!archdep_path_is_relative(tmp_path)) {
            /* absolute path */
            if (expanded_system_path == NULL) {
                s = util_concat(tmp_path, NULL); /* concat allocs a new str. */
            } else {
                s = util_concat(expanded_system_path,
                                ARCHDEP_FINDPATH_SEPARATOR_STRING,
                                tmp_path, NULL );
            }
        } else { /* relative path */
            if (expanded_system_path == NULL) {
                s = util_concat(current_dir,
                                FSDEV_DIR_SEP_STR,
                                tmp_path, NULL );
            } else {
                s = util_concat(expanded_system_path,
                                ARCHDEP_FINDPATH_SEPARATOR_STRING,
                                current_dir,
                                FSDEV_DIR_SEP_STR,
                                tmp_path, NULL );
            }
        }
        lib_free(expanded_system_path);
        expanded_system_path = s;

        tmp_path = p + strlen(ARCHDEP_FINDPATH_SEPARATOR_STRING);
    } while (p != NULL);

    lib_free(current_dir);
    lib_free(tmp_path_save);

    return 0;
}

static const resource_string_t resources_string[] = {
    { "Directory", "$$", RES_EVENT_NO, NULL,
      &system_path, set_system_path, NULL },
    { NULL },
};

/* Command-line options.  */

static const cmdline_option_t cmdline_options[] = {
    { "-directory", SET_RESOURCE, 1,
      NULL, NULL, "Directory", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_PATH, IDCLS_DEFINE_SYSTEM_FILES_PATH,
      NULL, NULL },
    { NULL },
};

/* ------------------------------------------------------------------------- */

int sysfile_init(const char *emu_id)
{
    default_path = archdep_default_sysfile_pathlist(emu_id);

    return 0;
}

void sysfile_shutdown(void)
{
    lib_free(default_path);
    lib_free(expanded_system_path);
}

int sysfile_resources_init(void)
{
    return resources_register_string(resources_string);
}

void sysfile_resources_shutdown(void)
{
    lib_free(system_path);
}

int sysfile_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* Locate a system file called `name' by using the search path in
   `Directory', checking that the file can be accesses in mode `mode', and
   return an open stdio stream for that file.  If `complete_path_return' is
   not NULL, `*complete_path_return' points to a malloced string with the
   complete path if the file was found or is NULL if not.  */
FILE *sysfile_open(const char *name, char **complete_path_return,
                   const char *open_mode)
{
#ifndef DINGOO_NATIVE
    char *p = NULL;
#endif
    FILE *f;

#ifdef DINGOO_NATIVE
    *complete_path_return = make_absolute_system_path(name);
    f = fopen(*complete_path_return, open_mode);
    if (!f) {
        *complete_path_return = NULL;
    }
    return f;
#else
    if (name == NULL || *name == '\0') {
        log_error(LOG_DEFAULT, "Missing name for system file.");
        return NULL;
    }

    p = findpath(name, expanded_system_path, IOUTIL_ACCESS_R_OK);

    if (p == NULL) {
        if (complete_path_return != NULL) {
            *complete_path_return = NULL;
        }
        return NULL;
    } else {
        f = fopen(p, open_mode);

        if (f == NULL || complete_path_return == NULL) {
            lib_free(p);
            p = NULL;
        }
        if (complete_path_return != NULL) {
            *complete_path_return = p;
        }
        return f;
    }
#endif  /* DINGOO_NATIVE */
}

/* As `sysfile_open', but do not open the file.  Just return 0 if the file is
   found and is readable, or -1 if an error occurs.  */
int sysfile_locate(const char *name, char **complete_path_return)
{
    FILE *f = sysfile_open(name, complete_path_return, MODE_READ);

    if (f != NULL) {
        fclose(f);
        return 0;
    } else {
        return -1;
    }
}

/* ------------------------------------------------------------------------- */

/*
 * If minsize >= 0, and the file is smaller than maxsize, load the data
 * into the end of the memory range.
 * If minsize < 0, load it at the start.
 */
int sysfile_load(const char *name, BYTE *dest, int minsize, int maxsize)
{
    FILE *fp = NULL;
    size_t rsize = 0;
    char *complete_path = NULL;
    int load_at_end;


/*
 * This feature is only active when --enable-embedded is given to the
 * configure script, its main use is to make developing new ports easier
 * and to allow ports for platforms which don't have a filesystem, or a
 * filesystem which is hard/impossible to load data files from.
 *
 * when USE_EMBEDDED is defined this will check if a
 * default system file is loaded, when USE_EMBEDDED
 * is not defined the function is just 0 and will
 * be optimized away.
 */

    if ((rsize = embedded_check_file(name, dest, minsize, maxsize)) != 0) {
        return rsize;
    }

    fp = sysfile_open(name, &complete_path, MODE_READ);

    if (fp == NULL) {
        /* Try to open the file from the current directory. */
        const char working_dir_prefix[3] = {
            '.', FSDEV_DIR_SEP_CHR, '\0'
        };
        char *local_name = NULL;

        local_name = util_concat(working_dir_prefix, name, NULL);
        fp = sysfile_open((const char *)local_name, &complete_path, MODE_READ);
        lib_free(local_name);
        local_name = NULL;

        if (fp == NULL) {
            goto fail;
        }
    }

    log_message(LOG_DEFAULT, "Loading system file `%s'.", complete_path);

    rsize = util_file_length(fp);
    if (minsize < 0) {
        minsize = -minsize;
        load_at_end = 0;
    } else {
        load_at_end = 1;
    }

    if (rsize < ((size_t)minsize)) {
        log_error(LOG_DEFAULT, "ROM %s: short file.", complete_path);
        goto fail;
    }
    if (rsize == ((size_t)maxsize + 2)) {
        log_warning(LOG_DEFAULT,
                    "ROM `%s': two bytes too large - removing assumed "
                    "start address.", complete_path);
        if (fread((char *)dest, 1, 2, fp) < 2) {
            goto fail;
        }
        rsize -= 2;
    }
    if (load_at_end && rsize < ((size_t)maxsize)) {
        dest += maxsize - rsize;
    } else if (rsize > ((size_t)maxsize)) {
        log_warning(LOG_DEFAULT, "ROM `%s': long file, discarding end.",
                    complete_path);
        rsize = maxsize;
    }
    if ((rsize = fread((char *)dest, 1, rsize, fp)) < ((size_t)minsize)) {
        goto fail;
    }

    fclose(fp);
    lib_free(complete_path);
    return (int)rsize;  /* return ok */

fail:
    lib_free(complete_path);
    return -1;
}
