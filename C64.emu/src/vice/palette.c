/*
 * palette.c - Palette handling.
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

#include "vice.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "palette.h"
#include "resources.h"
#include "sysfile.h"
#include "types.h"
#include "util.h"

/* #define DEBUG_PALETTE */

#ifdef DEBUG_PALETTE
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

static log_t palette_log = LOG_ERR;


palette_t *palette_create(unsigned int num_entries, const char *entry_names[])
{
    palette_t *p;
    unsigned int i;

    p = lib_malloc(sizeof(palette_t));

    p->num_entries = num_entries;
    p->entries = lib_calloc(num_entries, sizeof(palette_entry_t));

    if (entry_names != NULL) {
        for (i = 0; i < num_entries; i++) {
            p->entries[i].name = lib_strdup(entry_names[i]);
        }
    }

    return p;
}

void palette_free(palette_t *p)
{
    unsigned int i;

    if (p == NULL) {
        return;
    }

    for (i = 0; i < p->num_entries; i++) {
        lib_free(p->entries[i].name);
    }
    lib_free(p->entries);
    lib_free(p);
}

static int palette_set_entry(palette_t *p, unsigned int number,
                             uint8_t red,
                             uint8_t green,
                             uint8_t blue)
{
    if (p == NULL || number >= p->num_entries) {
        return -1;
    }

    p->entries[number].red = red;
    p->entries[number].green = green;
    p->entries[number].blue = blue;

    return 0;
}

static int palette_copy(palette_t *dest, const palette_t *src)
{
    unsigned int i;

    if (dest->num_entries != src->num_entries) {
        log_error(palette_log,
                  "Number of entries of src and dest palette do not match.");
        return -1;
    }

    for (i = 0; i < src->num_entries; i++) {
        palette_set_entry(dest, i,
                          src->entries[i].red,
                          src->entries[i].green,
                          src->entries[i].blue);
    }

    return 0;
}

static const char *next_nonspace(const char *p)
{
    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }

    return p;
}

static int palette_load_core(FILE *f, const char *file_name,
                             palette_t *tmp_palette,
                             palette_t *palette_return)
{
    char buf[1024];

    unsigned int line_num = 0;
    unsigned int entry_num = 0;

    while (1) {
        int i;
        uint8_t values[3];
        const char *p1;

        int line_len = util_get_line(buf, 1024, f);

        if (line_len < 0) {
            break;
        }

        line_num++;

        if (*buf == '#') {
            continue;
        }

        p1 = next_nonspace(buf);

        if (*p1 == '\0') { /* empty line */
            continue;
        }

        for (i = 0; i < 3; i++) {
            long result;
            char *p2;

            result = strtol(p1, &p2, 16);
            if (p1 == p2) {
                log_error(palette_log, "%s, %u: number expected.",
                          file_name, line_num);
                return -1;
            }
            if (result < 0
                || (i == 3 && result > 0xf)
                || result > 0xff
                || result < 0) {
                log_error(palette_log, "%s, %u: invalid value %lx.",
                          file_name, line_num, (unsigned long)result);
                return -1;
            }
            values[i] = (uint8_t)result;
            p1 = p2;
        }

        p1 = next_nonspace(p1);
        if (*p1 != '\0') {
            log_warning(palette_log,
                      "%s, %u: garbage at end of line.",
                      file_name, line_num);
        }
        if (entry_num >= palette_return->num_entries) {
            log_error(palette_log,
                      "%s: too many entries, %u expected.", file_name,
                      palette_return->num_entries);
            return -1;
        }
        if (palette_set_entry(tmp_palette, entry_num,
                              values[0], values[1], values[2]) < 0) {
            log_error(palette_log, "Failed to set palette entry.");
            return -1;
        }
        entry_num++;
    }

    if (line_num == 0) {
        log_error(palette_log, "Could not read from palette file.");
        return -1;
    }

    if (entry_num < palette_return->num_entries) {
        log_error(palette_log, "%s: too few entries, %u found, %u expected.",
                  file_name, entry_num, palette_return->num_entries);
        return -1;
    }

    if (palette_copy(palette_return, tmp_palette) < 0) {
        log_error(palette_log, "Failed to copy palette.");
        return -1;
    }

    return 0;
}

int palette_load(const char *file_name, const char *subpath, palette_t *palette_return)
{
    palette_t *tmp_palette;
    char *complete_path;
    FILE *f;
    int rc;

    f = sysfile_open(file_name, subpath, &complete_path, MODE_READ_TEXT);

    if (f == NULL) {
        /* Try to add the extension.  */
        char *tmp = lib_strdup(file_name);

        util_add_extension(&tmp, "vpl");
        f = sysfile_open(tmp, subpath, &complete_path, MODE_READ_TEXT);
        lib_free(tmp);

        if (f == NULL) {
            log_error(palette_log, "Palette not found: `%s'.", file_name);
            return -1;
        }
    }

    log_message(palette_log, "Loading palette `%s'.", complete_path);
    lib_free(complete_path);

    tmp_palette = palette_create(palette_return->num_entries, NULL);

    rc = palette_load_core(f, file_name, tmp_palette, palette_return);

    fclose(f);
    palette_free(tmp_palette);

    return rc;
}

/* FIXME: output NAME and TYPE tags! */
int palette_save(const char *file_name, const palette_t *palette)
{
    unsigned int i;
    FILE *f;

    f = fopen(file_name, MODE_WRITE);

    if (f == NULL) {
        return -1;
    }

    fprintf(f, "#\n# VICE Palette file\n#\n");
    fprintf(f, "# Syntax:\n# Red Green Blue\n#\n\n");

    for (i = 0; i < palette->num_entries; i++) {
        fprintf(f, "# %s\n%02X %02X %02X\n\n",
                palette->entries[i].name,
                palette->entries[i].red,
                palette->entries[i].green,
                palette->entries[i].blue);
    }

    return fclose(f);
}

/******************************************************************************/

/* FIXME: we might want to use a dynamically allocated list instead. meh */
#define MAX_PALETTE_FILES   60

/* +2 because there is one empty element after the valid ones, and then one
   extra unused entry reserved */
static palette_info_t palettelist[MAX_PALETTE_FILES + 2];    /* FIXME */

#define MAX_TAG_LEN     63

static int palette_read_tag(FILE *f, char *tag, char *outbuf, int len)
{
    char buf[MAX_TAG_LEN + 1];

    fseek(f, SEEK_SET, 0);

    DBG(("palette_read_tag %s", tag));
    if (len > (MAX_TAG_LEN - 7)) {
        len = (MAX_TAG_LEN - 7);
    }
    memset(buf, 0, MAX_TAG_LEN + 1);
    memset(outbuf, 0, len);

    while (1) {
        int line_len = util_get_line(buf, len - 1, f);

        if (line_len < 0) {
            break;
        }

        if ((buf[0] == '#') && (buf[1] == ' ') && (buf[6] == ':')) {
            if ((memcmp(&buf[2], tag, 4) == 0)) {
                DBG(("palette_read_tag got tag: %s '%s'", tag, &buf[7]));
                memcpy(outbuf, &buf[7], len);
                return 0;
            }
        }
    }

    DBG(("Could not read tag '%s' from palette file.", tag));
    return -1;
}

/* read extra info from the given filename (in pathname) and write output to
   the given palette_info entry */
static int palette_read_info(const char *pathname, const char *filename, palette_info_t* list)
{
    int res = 0;
    size_t n = 0;
    char *fullpath;
    const char *chip_guess;
    const char *name_guess;
    char *chip_tag;
    char *name_tag;
    char name[MAX_TAG_LEN + 1];
    char chip[MAX_TAG_LEN + 1];
    FILE *f;

    if (machine_class == VICE_MACHINE_VIC20) {
        chip_guess = "VIC";
    } else if (machine_class == VICE_MACHINE_PLUS4) {
        chip_guess = "TED";
    } else if ((machine_class == VICE_MACHINE_PET) ||
               (machine_class == VICE_MACHINE_CBM6x0)) {
        chip_guess = "Crtc";
    } else {
        chip_guess = "VICII";
    }

    /* old hack for x128 - if the filename starts with "vdc", then
        * assume its a vdc palette, else its a vicii palette */
    if (machine_class == VICE_MACHINE_C128) {
        if (((filename[0] == 'v') || (filename[0] == 'V')) &&
            ((filename[1] == 'd') || (filename[1] == 'D')) &&
            ((filename[2] == 'c') || (filename[2] == 'C'))) {
            chip_guess = "VDC";
        }
    }

    fullpath = util_join_paths(pathname, filename, NULL);

    DBG(("palette_read_info: '%s':'%s' (='%s') chip guess:'%s'",
         pathname, filename, fullpath, chip_guess));

    /* read chip tag from palette file */
    f = fopen(fullpath, "rb");
    palette_read_tag(f, "TYPE", chip, MAX_TAG_LEN);
    if (chip[0] != 0) {
        /* use chip type from .vpl file */
        chip_guess = chip;
    }
    chip_tag = lib_strdup(chip_guess);

    /* read name tag from palette file */
    palette_read_tag(f, "NAME", name, MAX_TAG_LEN);
    if (name[0] != 0) {
        /* use name from .vpl file */
        name_guess = name;
        name_tag = lib_strdup(name_guess);
    } else {
        /* use filename */
        name_guess = filename;
        n = strlen(name_guess);
        name_tag = lib_strdup(name_guess);
        name_tag[n - 4] = 0; /* remove file extension from name */
    }
    fclose(f);

    DBG(("palette_read_info: chip_tag:'%s' name_tag:'%s' filename:'%s'", chip_tag, name_tag, filename));

    list->chip = chip_tag; /* free? */
    list->name = name_tag; /* free? */
    list->file = lib_strdup(filename); /* free? */

    lib_free(fullpath);

    return res;
}

/* FIXME: we might want to make the following two functions more generic and
          not use this global state variable, and then move it to sysfile.c */
static char *sysfile_system_path_ptr;

/* kindof like "opendir" but returns all search pathes one by one */
static char *sysfile_get_system_path_start(void)
{
    sysfile_system_path_ptr = NULL;
    const char *path = get_system_path();
    /*printf("path:%s\n", path);*/
    if (path != NULL) {
        return lib_strdup(path);
    }
    return NULL;
}

/* kindof like "readdir" but returns all search pathes one by one */
static char *sysfile_get_system_path_next(char *syspath)
{
    char *path;
    /* syspath always points to the beginning of the syspath,
       path is NULL for first step or points to beginning of
       last partial path we returned */
    if (sysfile_system_path_ptr == NULL) {
        int n = 0;
        while ((syspath[n] != ARCHDEP_FINDPATH_SEPARATOR_CHAR) &&
               (syspath[n] != 0)) {
            n++;
        }
        path = lib_malloc(n + 1);
        memcpy (path, syspath, n);
        path[n] = 0;
        sysfile_system_path_ptr = syspath + n;
        return path;
    } else {
        int n = 0;
        /* two special cases: either we are at the end of the system path, or we
           are at a seperator char */
        if (*sysfile_system_path_ptr == 0) {
            return NULL;
        } else if (*sysfile_system_path_ptr == ARCHDEP_FINDPATH_SEPARATOR_CHAR) {
            sysfile_system_path_ptr++;
        }
        while ((sysfile_system_path_ptr[n] != ARCHDEP_FINDPATH_SEPARATOR_CHAR) &&
               (sysfile_system_path_ptr[n] != 0)) {
            n++;
        }
        path = lib_malloc(n + 1);
        memcpy (path, sysfile_system_path_ptr, n);
        path[n] = 0;
        sysfile_system_path_ptr = sysfile_system_path_ptr + n;
        return path;
    }
}

/* check if a palette associated with this "file" exists, return number of times */
static int palette_find_name(const char *name)
{
    int found = 0;
    int n = 0;
    while (palettelist[n].file != NULL) {
        if (util_strcasecmp(palettelist[n].file, name) == 0) {
            found++;
        }
        n++;
        /*printf("palette_find_name n:%d:'%s' '%s' found:%d\n", n, palettelist[n].file, name, found);*/
    }
    return found;
}

/* free all memory associated with pointers in the palette info struct/list */
static void palette_list_free(palette_info_t *list)
{
    while (list->file) {
        lib_free(list->file);
        if (list->name) {
            lib_free(list->name);
        }
        if (list->chip) {
            lib_free(list->chip);
        }
        list++;
    }
    memset(palettelist, 0, sizeof(palette_info_t) * MAX_PALETTE_FILES);
}

/*
    High level API call for the UI
    - search in all pathes given by the system path
    - find all valid palette files
      - make sure this behaves exactly like sysfile_open would. ie when two or
        more files with the same basename exist, only the first one should be
        valid.
    - populate the palette_info list
 */
const palette_info_t *palette_get_info_list(void)
{
    int coln = 0;
    char *syspath = sysfile_get_system_path_start();
    char *path;

    palette_list_free(palettelist);

    if (syspath) {
        /*printf("syspath:%s\n", syspath);*/
        while ((path = sysfile_get_system_path_next(syspath))) {
            /*printf("path:%s\n", path);*/
            char *dirname = util_join_paths(path, machine_name, NULL);
            DBG(("palette_get_info_list dirname:'%s'", dirname));

            /* now in the resulting path name, list palette files */
            {
                archdep_dir_t *host_dir;
                const char *fname;
                size_t n;
                host_dir = archdep_opendir(dirname, ARCHDEP_OPENDIR_ALL_FILES);
                if (host_dir) {
                    while ((fname = archdep_readdir(host_dir)) != NULL) {
                        /*printf("fname:%s", fname);*/
                        /* only consider files with .vpl extension */
                        n = strlen(fname);
                        if ((n < 4) ||
                            (util_strcasecmp(&fname[n - 4], ".vpl") != 0)) {
                            /*printf("[skip:filtered]\n");*/
                            continue;
                        }
                        /* check if we already added a file with this name */
                        if (palette_find_name(fname) > 0) {
                            /*printf("[skip-exists]\n");*/
                            continue;
                        }
                        /*printf("[ok coln:%d]\n", coln);*/
                        /* set up the palette info */
                        palette_read_info(dirname, fname, &palettelist[coln]);

                        /* FIXME: limit the list here. we may really want a
                                  dynamically allocated list here, but its not
                                  worth the trouble right now :=P */
                        coln++;
                        if (coln == MAX_PALETTE_FILES) {
                            log_error(palette_log, "too many palette files found (max is %d)\n", MAX_PALETTE_FILES);
                            palettelist[coln].chip = NULL;
                            palettelist[coln].name = NULL;
                            palettelist[coln].file = NULL;
                            archdep_closedir(host_dir);
                            lib_free(dirname);
                            lib_free(path);
                            lib_free(syspath);
                            return &palettelist[0];
                        }
                    }
                    archdep_closedir(host_dir);
                }
            }
            lib_free(dirname);
            lib_free(path);
        }
        lib_free(syspath);
    }
    /* mark end of list */
    palettelist[coln].chip = NULL;
    palettelist[coln].name = NULL;
    palettelist[coln].file = NULL;

    /* TODO: sort list by name(s) */

    return &palettelist[0];
}

/******************************************************************************/

void palette_init(void)
{
    palette_log = log_open("Palette");
}

void palette_shutdown(void)
{
    palette_list_free(palettelist);
}
