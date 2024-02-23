/** \file   fliplist.c
 * \brief   Fliplist handling
 *
 * \author  pottendo <pottendo@gmx.net>
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
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

/* #define DEBUG_FLIPLIST */

#include "vice.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "archdep.h"
#include "attach.h"
#include "charset.h"
#include "cmdline.h"
#include "drive.h"
#include "fliplist.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "util.h"

#ifdef DEBUG_FLIPLIST
#define DBG(x)  log_debug   x
#else
#define DBG(x)
#endif

struct fliplist_s {
    fliplist_t next, prev;
    char *image;
    unsigned int unit;
};

static fliplist_t fliplist[NUM_DISK_UNITS] = {
    (fliplist_t)NULL,
    (fliplist_t)NULL
};

static char *current_image = (char *)NULL;
static unsigned int current_drive;
static fliplist_t iterator;

static const char flip_file_header_old[] = "# Vice fliplist file";
static const char flip_file_header_new[] = "; Vice fliplist file";
static const char flip_file_petscii[9] = { 0x23, 0x50, 0x45, 0x54, 0x53, 0x43, 0x49, 0x49, 0 };

#define buffer_size 1024

static void show_fliplist(unsigned int unit);

static char *fliplist_file_name = NULL;


/** \brief  Keep track of factory value
 *
 * Don't free() a const
 */
static char *fliplist_factory = NULL;


static int set_fliplist_file_name(const char *val, void *param)
{
    if (util_string_set(&fliplist_file_name, val)) {
        return 0;
    }

    fliplist_load_list(FLIPLIST_ALL_UNITS, fliplist_file_name, 0);

    return 0;
}

static resource_string_t resources_string[] = {
    { "FliplistName", NULL, RES_EVENT_NO, NULL,
      &fliplist_file_name, set_fliplist_file_name, NULL },
    RESOURCE_STRING_LIST_END
};

int fliplist_resources_init(void)
{
    fliplist_factory = archdep_default_fliplist_file_name();
    resources_string[0].factory_value = fliplist_factory;

    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return 0;
}

void fliplist_resources_shutdown(void)
{
    int i;

    for (i = 0; i < NUM_DISK_UNITS; i++) {
        fliplist_clear_list(8 + i);
    }

    lib_free(fliplist_file_name);
    lib_free(fliplist_factory);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-flipname", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "FliplistName", NULL,
      "<Name>", "Specify name of the flip list file image" },
    CMDLINE_LIST_END
};

int fliplist_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */
/* interface functions */

void fliplist_shutdown(void)
{
    lib_free(current_image);
    current_image = NULL;
}

void fliplist_set_current(unsigned int unit, const char *filename)
{
    lib_free(current_image);
    current_image = lib_strdup(filename);
    current_drive = unit;
}

char *fliplist_get_head(unsigned int unit)
{
    if (fliplist[unit - 8]) {
        return fliplist[unit - 8]->image;
    }
    return NULL;
}

const char *fliplist_get_next(unsigned int unit)
{
    if (fliplist[unit - 8]) {
        return fliplist[unit - 8]->next->image;
    }
    return NULL;
}

const char *fliplist_get_prev(unsigned int unit)
{
    if (fliplist[unit - 8]) {
        return fliplist[unit - 8]->prev->image;
    }
    return NULL;
}

const char *fliplist_get_image(fliplist_t fl)
{
    return fl->image;
}

unsigned int fliplist_get_unit(fliplist_t fl)
{
    return fl->unit;
}

bool fliplist_add_image(unsigned int unit)
{
    fliplist_t n;

    if (current_image == NULL) {
        return false;
    }
    if (strcmp(current_image, "") == 0) {
        return false;
    }

    n = lib_malloc(sizeof(struct fliplist_s));
    n->image = lib_strdup(current_image);
    unit = n->unit = current_drive;

    log_message(LOG_DEFAULT, "Adding `%s' to fliplist[%u]", n->image, unit);
    if (fliplist[unit - 8]) {
        fliplist[unit - 8]->prev->next = n;
        n->prev = fliplist[unit - 8]->prev;
        fliplist[unit - 8]->prev = n;
        n->next = fliplist[unit - 8];
    } else {
        fliplist[unit - 8] = n;
        n->next = n;
        n->prev = n;
    }
    show_fliplist(unit);
    return true;
}


void fliplist_remove(unsigned int unit, const char *image)
{
    fliplist_t tmp;

    if (fliplist[unit - 8] == NULL) {
        return;
    }
    if (image == (char *) NULL) {
        /* no image given, so remove the head */
        if ((fliplist[unit - 8] == fliplist[unit - 8]->next) &&
            (fliplist[unit - 8] == fliplist[unit - 8]->prev)) {
            /* this is the last entry */
            tmp = fliplist[unit - 8];
            fliplist[unit - 8] = (fliplist_t) NULL;
        } else {
            fliplist[unit - 8]->next->prev = fliplist[unit - 8]->prev;
            fliplist[unit - 8]->prev->next = fliplist[unit - 8]->next;
            tmp = fliplist[unit - 8];
            fliplist[unit - 8] = fliplist[unit - 8]->next;
        }
        log_message(LOG_DEFAULT, "Removing `%s' from fliplist[%u]",
                    tmp->image, unit);
        lib_free(tmp->image);
        lib_free(tmp);
        show_fliplist(unit);
        return;
    } else {
        /* do a lookup and remove it */
        fliplist_t it = fliplist[unit - 8];

        if (strcmp(it->image, image) == 0) {
            /* it's the head */
            fliplist_remove(unit, NULL);
            return;
        }
        it = it->next;
        while ((strcmp(it->image, image) != 0) &&
               (it != fliplist[unit - 8])) {
            it = it->next;
        }

        if (it == fliplist[unit - 8]) {
            log_message(LOG_DEFAULT,
                        "Cannot remove `%s'; not found in fliplist[%u]",
                        it->image, unit);
            return;
        }

        it->next->prev = it->prev;
        it->prev->next = it->next;
        lib_free(it->image);
        lib_free(it);
        show_fliplist(unit);
    }
}


/** \brief  Attach new image from the fliplist
 *
 * \param[in]   unit        drive unit number (8-11)
 * \param[in]   direction   attach either next (>=1) or previous image
 *
 * \return  boolean
 */
bool fliplist_attach_head(unsigned int unit, int direction)
{
    if (fliplist[unit - 8] == NULL) {
        return false;
    }

    if (direction) {
        fliplist[unit - 8] = fliplist[unit - 8]->next;
    } else {
        fliplist[unit - 8] = fliplist[unit - 8]->prev;
    }

    if (file_system_attach_disk(
                fliplist[unit - 8]->unit,
                0,  /* drive */
                fliplist[unit - 8]->image) < 0) {
        /* shouldn't happen, so ignore it */
        return false;   /* handle it anyway */
    }
    return true;
}

fliplist_t fliplist_init_iterate(unsigned int unit)
{
    fliplist_t ret = NULL;

    iterator = fliplist[unit - 8];
    if (iterator) {
        ret = iterator;
        iterator = iterator->next;
    }
    return ret;
}

fliplist_t fliplist_next_iterate(unsigned int unit)
{
    fliplist_t ret = NULL;

    if (iterator) {
        if (iterator != fliplist[unit - 8]) {
            ret = iterator;
            iterator = iterator->next;
        }
    }
    return ret;
}

void fliplist_clear_list(unsigned int unit)
{
    fliplist_t flip = fliplist[unit - 8];

    if (flip != NULL) {
        do {
            fliplist_t tmp = flip->next;

            lib_free(flip->image);
            lib_free(flip);
            flip = tmp;
        }
        while (flip != fliplist[unit - 8]);

        fliplist[unit - 8] = NULL;
    }
}

int fliplist_save_list(unsigned int unit, const char *filename)
{
    int all_units = 0;
    fliplist_t flip;
    FILE *fp = NULL;
    char *fliplist_fullname, *fliplist_path, *fliplist_name;
    char *file_path, *file_name;

    if (filename == NULL || *filename == 0) {
        return -1;
    }

    DBG(("fliplist_save_list unit %u, name: '%s'", unit, filename));

    if (archdep_expand_path(&fliplist_fullname, filename) != 0) {
        return -1;
    }

    DBG(("full name: '%s'", fliplist_fullname));

    /* create the directory where the fliplist should be written first */
    util_fname_split(fliplist_fullname, &fliplist_path, &fliplist_name);
    if ((fliplist_path != NULL) && (*fliplist_path != 0) && (!strcmp(fliplist_path, "."))) {
        archdep_mkdir(fliplist_path, ARCHDEP_MKDIR_RWXU);
    }
    DBG(("path: '%s' name: '%s'", fliplist_path, fliplist_name));

    if (unit == FLIPLIST_ALL_UNITS) {
        all_units = 1;
        unit = 8;
    }

    do {
        flip = fliplist[unit - 8];

        if (flip != NULL) {
            if (!fp) {
                if ((fp = fopen(fliplist_fullname, MODE_WRITE)) == NULL) {
                    lib_free(fliplist_fullname);
                    lib_free(fliplist_path);
                    lib_free(fliplist_name);
                    return -1;
                }
                /* output the header line */
                fprintf(fp, "%s\n", flip_file_header_new);
            }

            /* omit the ";UNIT X" line if this is the only unit */
            if (all_units) {
                fprintf(fp, ";UNIT %u\n", unit);
            }
            /* output the names of the images */
            do {
                util_fname_split(flip->image, &file_path, &file_name);
                DBG(("file path: '%s' name: '%s'", file_path, file_name));
                /* FIXME: we should handle some more special cases:
                   - absolute pathes must not be modified (those are non portable anyway
                   - relative pathes could be converted to a CMD style path so it works with sd2iec
                */
                /* if file- and fliplist path is the same, omit the path in the fliplist */
                if (!strcmp(file_path, fliplist_path)) {
                    fprintf(fp, "%s\n", file_name);
                } else {
                    fprintf(fp, "%s\n", flip->image);
                }
                flip = flip->next;
                lib_free(file_path);
                lib_free(file_name);
            }
            while (flip != fliplist[unit - 8]);
        }
        unit++;
    } while (all_units && ((unit - 8) < NUM_DISK_UNITS));

    if (fp) {
        fclose(fp);
    }
    lib_free(fliplist_fullname);
    lib_free(fliplist_path);
    lib_free(fliplist_name);
    return 0;
}

int fliplist_load_list(unsigned int unit, const char *filename, int autoattach)
{
    FILE *fp;
    char buffer[buffer_size];
    int all_units = 0, i;
    int listok = 0;
    int ispetscii = 0;
    int firstline = 1;
    int c;
    char *fliplist_fullname, *fliplist_path, *fliplist_name;
    char *buffer_fullname;
    char cwd[ARCHDEP_PATH_MAX];

    if (filename == NULL || *filename == 0 || (fp = fopen(filename, MODE_READ)) == NULL) {
        return -1;
    }

    DBG(("fliplist_load_list unit %u, autoattach: %d, name: '%s'",
         unit, autoattach, filename));

    if (archdep_expand_path(&fliplist_fullname, filename) != 0) {
        return -1;
    }

    DBG(("full name: '%s'", fliplist_fullname));

    util_fname_split(fliplist_fullname, &fliplist_path, &fliplist_name);

    DBG(("path: '%s' name: '%s'", fliplist_path, fliplist_name));

    /* KLUDGES: we need to change the current dir to the fliplist path, else
       the archdep_expand_path below will not work as expected */
    archdep_getcwd(cwd, ARCHDEP_PATH_MAX);
    archdep_chdir(fliplist_path);

    /* remove current fliplist */
    if (unit == FLIPLIST_ALL_UNITS) {
        all_units = 1;
        for (i = 0; i < NUM_DISK_UNITS; i++) {
            fliplist_clear_list(i + 8);
        }
    } else {
        fliplist_clear_list(unit);
    }

    while (!feof(fp)) {
        char *b;

        buffer[0] = '\0';

        /* do not use fgets here, we should deal with all kind of line endings,
           including petscii */
#if 0
        if (fgets(buffer, buffer_size, fp) == NULL) {
            break;
        }
#endif
        b = buffer;
        while (!feof(fp)) {
            c = fgetc(fp);
            if ((c == EOF) || (c == 0x0a) || (c == 0x0d)) {
                break;
            }
            *b++ = c;
        }
        *b = '\0';

        /* remove trailing whitespace (linefeeds etc) */
        b = buffer + strlen(buffer);
        while ((b > buffer) && (isspace((unsigned char)(b[-1])))) {
            b--;
            *b = '\0';
        }

        DBG(("got line: '%s'", buffer));

        if (firstline) {
            firstline = 0;
            /* check the header line VICE creates */
            if ((!strncmp(buffer, flip_file_header_old, strlen(flip_file_header_old))) ||
                (!strncmp(buffer, flip_file_header_new, strlen(flip_file_header_new)))) {
                log_message(LOG_DEFAULT, "File %s recognized as fliplist file created by VICE", filename);
                continue;
            } else {
                log_warning(LOG_DEFAULT, "File %s is not a fliplist file created by VICE, continuing anyway...", filename);
            }
            /* check for the #PETSCI tag (sd2iec) */
            if (!strncmp(buffer, flip_file_petscii, strlen(flip_file_petscii))) {
                DBG(("found #PETSCII tag"));
                ispetscii = 1;
                continue;
            }
        }

        if (ispetscii) {
            DBG(("petscii line: '%s'", buffer));
            charset_petconvstring((uint8_t*)buffer, CONVERT_TO_ASCII);
            DBG(("ascii line: '%s'", buffer));
        }

        /* ";UNIT X" (former "UNIT X") indicates the following files belong to that drive */
        if (!strncmp("UNIT ", buffer, 5) || !strncmp(";UNIT ", buffer, 6)) {
            if (all_units != 0) {
                long unit_long = -1;
                char *unit_buffer = buffer + 5; /* points to the number (old) or the space before it (new) */

                unit_long = strtol(unit_buffer, NULL, 10);
                DBG(("got UNIT '%s' number: %ld", unit_buffer, unit_long));

                if ((unit_long < 8) || (unit_long > 11)) {
                    log_message(LOG_DEFAULT,
                            "Invalid unit number %ld for fliplist\n", unit_long);
                    /* perhaps VICE should properly error out, ie quit? */
                    fclose(fp);
                    archdep_chdir(cwd);
                    lib_free(fliplist_fullname);
                    lib_free(fliplist_path);
                    lib_free(fliplist_name);
                    return -1;
                }

                unit = (unsigned int)unit_long;
            }
            continue;
        }

        /* ; at the start of a line indicates a comment, skip it */
        if (buffer[0] == ';') {
            continue;
        }

        /* skip empty lines, non empty lines contain a filename */
        if (buffer[0] != 0) {
            fliplist_t tmp;

            *b = '\0';

            DBG(("file name: '%s'", buffer));

            if (unit == FLIPLIST_ALL_UNITS) {
                log_message(LOG_DEFAULT, "Fliplist has inconsistent view for unit, assuming 8.\n");
                unit = 8;
            }

            /* FIXME: we should handle some special cases of pathes we can find
                      in .lst files:
               - sd2iec accepts 'cmd' style pathes
               - all kinds of absolute pathes musst not be converted, those are
                 non portable anyway
            */

            /* always use full path for attaching in the fliplist */
            if (archdep_expand_path(&buffer_fullname, buffer) != 0) {
                fclose(fp);
                archdep_chdir(cwd);
                lib_free(fliplist_fullname);
                lib_free(fliplist_path);
                lib_free(fliplist_name);
                return -1;
            }

            DBG(("file full name: '%s'", buffer_fullname));

            if (archdep_access(buffer_fullname, ARCHDEP_ACCESS_R_OK) == 0) {
                tmp = lib_malloc(sizeof(struct fliplist_s));
                tmp->image = lib_strdup(buffer_fullname);
                tmp->unit = unit;

                lib_free(buffer_fullname);

                if (fliplist[unit - 8] == NULL) {
                    /* the first entry in the list */
                    fliplist[unit - 8] = tmp;
                    tmp->prev = tmp;
                    tmp->next = tmp;
                } else {
                    /* add next entry at the bottom of the list */
                    fliplist[unit - 8]->prev->next = tmp;
                    tmp->prev = fliplist[unit - 8]->prev;
                    fliplist[unit - 8]->prev = tmp;
                    tmp->next = fliplist[unit - 8];
                }
                listok = 1;
            } else {
                log_error(LOG_DEFAULT, "File '%s' could not be added to flip list", buffer_fullname);
            }
        }
    }

    fclose(fp);
    archdep_chdir(cwd);
    lib_free(fliplist_fullname);
    lib_free(fliplist_path);
    lib_free(fliplist_name);

    if (listok) {
        current_drive = unit;

        if (all_units) {
            for (i = 0; i < NUM_DISK_UNITS; i++) {
                show_fliplist(i + 8);
            }
        } else {
            show_fliplist(unit);
        }

        if (autoattach) {
            fliplist_attach_head(unit, 1);
        }

        return 0;
    }

    return -1;
}

/* ------------------------------------------------------------------------- */

static void show_fliplist(unsigned int unit)
{
    fliplist_t it = fliplist[unit - 8];

    log_message(LOG_DEFAULT, "Fliplist[%u] contains:", unit);

    if (it) {
        do {
            log_message(LOG_DEFAULT,
                        "\tUnit %u %s (n: %s, p:%s)", it->unit, it->image,
                        it->next->image, it->prev->image);
            it = it->next;
        } while (it != fliplist[unit - 8]);
    } else {
        log_message(LOG_DEFAULT, "\tnothing");
    }
}
