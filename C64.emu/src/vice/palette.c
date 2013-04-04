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
#include "embedded.h"
#include "lib.h"
#include "log.h"
#include "palette.h"
#include "sysfile.h"
#include "types.h"
#include "util.h"


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
            p->entries[i].name = lib_stralloc(entry_names[i]);
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
                             BYTE red, BYTE green, BYTE blue, BYTE dither)
{
    if (p == NULL || number >= p->num_entries) {
        return -1;
    }

    p->entries[number].red = red;
    p->entries[number].green = green;
    p->entries[number].blue = blue;
    p->entries[number].dither = dither;

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
        palette_set_entry(dest, i, src->entries[i].red, src->entries[i].green,
                          src->entries[i].blue, src->entries[i].dither);
    }

    return 0;
}

static char *next_nonspace(const char *p)
{
    while (*p != '\0' && isspace((int)*p)) {
        p++;
    }

    return (char *)p;
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
        BYTE values[4];
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

        for (i = 0; i < 4; i++) {
            long result;
            const char *p2;

            if (util_string_to_long(p1, &p2, 16, &result) < 0) {
                log_error(palette_log, "%s, %d: number expected.",
                          file_name, line_num);
                return -1;
            }
            if (result < 0
                || (i == 3 && result > 0xf)
                || result > 0xff
                || result < 0) {
                log_error(palette_log, "%s, %d: invalid value %lx.",
                          file_name, line_num, result);
                return -1;
            }
            values[i] = (BYTE)result;
            p1 = p2;
        }

        p1 = next_nonspace(p1);
        if (*p1 != '\0') {
            log_error(palette_log,
                      "%s, %d: garbage at end of line.",
                      file_name, line_num);
            return -1;
        }
        if (entry_num >= palette_return->num_entries) {
            log_error(palette_log,
                      "%s: too many entries, %d expected.", file_name,
                      palette_return->num_entries);
            return -1;
        }
        if (palette_set_entry(tmp_palette, entry_num,
                              values[0], values[1], values[2], values[3]) < 0) {
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
        log_error(palette_log, "%s: too few entries, %d found, %d expected.",
                  file_name, entry_num, palette_return->num_entries);
        return -1;
    }

    if (palette_copy(palette_return, tmp_palette) < 0) {
        log_error(palette_log, "Failed to copy palette.");
        return -1;
    }

    return 0;
}

int palette_load(const char *file_name, palette_t *palette_return)
{
    palette_t *tmp_palette;
    char *complete_path;
    FILE *f;
    int rc;

    if (embedded_palette_load(file_name, palette_return) == 0) {
        return 0;
    }

    f = sysfile_open(file_name, &complete_path, MODE_READ_TEXT);

    if (f == NULL) {
        /* Try to add the extension.  */
        char *tmp = lib_stralloc(file_name);

        util_add_extension(&tmp, "vpl");
        f = sysfile_open(tmp, &complete_path, MODE_READ_TEXT);
        lib_free(tmp);

        if (f == NULL) {
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

int palette_save(const char *file_name, const palette_t *palette)
{
    unsigned int i;
    FILE *f;

    f = fopen(file_name, MODE_WRITE);

    if (f == NULL) {
        return -1;
    }

    fprintf(f, "#\n# VICE Palette file\n#\n");
    fprintf(f, "# Syntax:\n# Red Green Blue Dither\n#\n\n");

    for (i = 0; i < palette->num_entries; i++) {
        fprintf(f, "# %s\n%02X %02X %02X %01X\n\n",
                palette->entries[i].name,
                palette->entries[i].red,
                palette->entries[i].green,
                palette->entries[i].blue,
                palette->entries[i].dither);
    }

    return fclose(f);
}

void palette_init(void)
{
    palette_log = log_open("Palette");
}
