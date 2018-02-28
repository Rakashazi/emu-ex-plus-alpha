/*
 * snapshot.c - Implementation of machine snapshot files.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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
#include "lib.h"
#include "ioutil.h"
#include "log.h"
#include "snapshot.h"
#ifdef USE_SVN_REVISION
#include "svnversion.h"
#endif
#include "translate.h"
#include "types.h"
#include "uiapi.h"
#include "version.h"
#include "vsync.h"
#include "zfile.h"

static int snapshot_error = SNAPSHOT_NO_ERROR;
static char *current_module = NULL;
static char read_name[SNAPSHOT_MACHINE_NAME_LEN];
static char *current_machine_name = NULL;
static char *current_filename = NULL;

char snapshot_magic_string[] = "VICE Snapshot File\032";
char snapshot_version_magic_string[] = "VICE Version\032";

#define SNAPSHOT_MAGIC_LEN              19
#define SNAPSHOT_VERSION_MAGIC_LEN      13

struct snapshot_module_s {
    /* File descriptor.  */
    FILE *file;

    /* Flag: are we writing it?  */
    int write_mode;

    /* Size of the module.  */
    DWORD size;

    /* Offset of the module in the file.  */
    long offset;

    /* Offset of the size field in the file.  */
    long size_offset;
};

struct snapshot_s {
    /* File descriptor.  */
    FILE *file;

    /* Offset of the first module.  */
    long first_module_offset;

    /* Flag: are we writing it?  */
    int write_mode;
};

/* ------------------------------------------------------------------------- */

static int snapshot_write_byte(FILE *f, BYTE data)
{
    if (fputc(data, f) == EOF) {
        snapshot_error = SNAPSHOT_WRITE_EOF_ERROR;
        return -1;
    }

    return 0;
}

static int snapshot_write_word(FILE *f, WORD data)
{
    if (snapshot_write_byte(f, (BYTE)(data & 0xff)) < 0
        || snapshot_write_byte(f, (BYTE)(data >> 8)) < 0) {
        return -1;
    }

    return 0;
}

static int snapshot_write_dword(FILE *f, DWORD data)
{
    if (snapshot_write_word(f, (WORD)(data & 0xffff)) < 0
        || snapshot_write_word(f, (WORD)(data >> 16)) < 0) {
        return -1;
    }

    return 0;
}

static int snapshot_write_double(FILE *f, double data)
{
    BYTE *byte_data = (BYTE *)&data;
    int i;

    for (i = 0; i < sizeof(double); i++) {
        if (snapshot_write_byte(f, byte_data[i]) < 0) {
            return -1;
        }
    }
    return 0;
}

static int snapshot_write_padded_string(FILE *f, const char *s, BYTE pad_char,
                                        int len)
{
    int i, found_zero;
    BYTE c;

    for (i = found_zero = 0; i < len; i++) {
        if (!found_zero && s[i] == 0) {
            found_zero = 1;
        }
        c = found_zero ? (BYTE)pad_char : (BYTE) s[i];
        if (snapshot_write_byte(f, c) < 0) {
            return -1;
        }
    }

    return 0;
}

static int snapshot_write_byte_array(FILE *f, const BYTE *data, unsigned int num)
{
    if (num > 0 && fwrite(data, (size_t)num, 1, f) < 1) {
        snapshot_error = SNAPSHOT_WRITE_BYTE_ARRAY_ERROR;
        return -1;
    }

    return 0;
}

static int snapshot_write_word_array(FILE *f, const WORD *data, unsigned int num)
{
    unsigned int i;

    for (i = 0; i < num; i++) {
        if (snapshot_write_word(f, data[i]) < 0) {
            return -1;
        }
    }

    return 0;
}

static int snapshot_write_dword_array(FILE *f, const DWORD *data, unsigned int num)
{
    unsigned int i;

    for (i = 0; i < num; i++) {
        if (snapshot_write_dword(f, data[i]) < 0) {
            return -1;
        }
    }

    return 0;
}


static int snapshot_write_string(FILE *f, const char *s)
{
    size_t len, i;

    len = s ? (strlen(s) + 1) : 0;      /* length includes nullbyte */

    if (snapshot_write_word(f, (WORD)len) < 0) {
        return -1;
    }

    for (i = 0; i < len; i++) {
        if (snapshot_write_byte(f, s[i]) < 0) {
            return -1;
        }
    }

    return (int)(len + sizeof(WORD));
}

static int snapshot_read_byte(FILE *f, BYTE *b_return)
{
    int c;

    c = fgetc(f);
    if (c == EOF) {
        snapshot_error = SNAPSHOT_READ_EOF_ERROR;
        return -1;
    }
    *b_return = (BYTE)c;
    return 0;
}

static int snapshot_read_word(FILE *f, WORD *w_return)
{
    BYTE lo, hi;

    if (snapshot_read_byte(f, &lo) < 0 || snapshot_read_byte(f, &hi) < 0) {
        return -1;
    }

    *w_return = lo | (hi << 8);
    return 0;
}

static int snapshot_read_dword(FILE *f, DWORD *dw_return)
{
    WORD lo, hi;

    if (snapshot_read_word(f, &lo) < 0 || snapshot_read_word(f, &hi) < 0) {
        return -1;
    }

    *dw_return = lo | (hi << 16);
    return 0;
}

static int snapshot_read_double(FILE *f, double *d_return)
{
    int i;
    int c;
    double val;
    BYTE *byte_val = (BYTE *)&val;

    for (i = 0; i < sizeof(double); i++) {
        c = fgetc(f);
        if (c == EOF) {
            snapshot_error = SNAPSHOT_READ_EOF_ERROR;
            return -1;
        }
        byte_val[i] = (BYTE)c;
    }
    *d_return = val;
    return 0;
}

static int snapshot_read_byte_array(FILE *f, BYTE *b_return, unsigned int num)
{
    if (num > 0 && fread(b_return, (size_t)num, 1, f) < 1) {
        snapshot_error = SNAPSHOT_READ_BYTE_ARRAY_ERROR;
        return -1;
    }

    return 0;
}

static int snapshot_read_word_array(FILE *f, WORD *w_return, unsigned int num)
{
    unsigned int i;

    for (i = 0; i < num; i++) {
        if (snapshot_read_word(f, w_return + i) < 0) {
            return -1;
        }
    }

    return 0;
}

static int snapshot_read_dword_array(FILE *f, DWORD *dw_return, unsigned int num)
{
    unsigned int i;

    for (i = 0; i < num; i++) {
        if (snapshot_read_dword(f, dw_return + i) < 0) {
            return -1;
        }
    }

    return 0;
}

static int snapshot_read_string(FILE *f, char **s)
{
    int i, len;
    WORD w;
    char *p = NULL;

    /* first free the previous string */
    lib_free(*s);
    *s = NULL;      /* don't leave a bogus pointer */

    if (snapshot_read_word(f, &w) < 0) {
        return -1;
    }

    len = (int)w;

    if (len) {
        p = lib_malloc(len);
        *s = p;

        for (i = 0; i < len; i++) {
            if (snapshot_read_byte(f, (BYTE *)(p + i)) < 0) {
                p[0] = 0;
                return -1;
            }
        }
        p[len - 1] = 0;   /* just to be save */
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

int snapshot_module_write_byte(snapshot_module_t *m, BYTE b)
{
    if (snapshot_write_byte(m->file, b) < 0) {
        return -1;
    }

    m->size++;
    return 0;
}

int snapshot_module_write_word(snapshot_module_t *m, WORD w)
{
    if (snapshot_write_word(m->file, w) < 0) {
        return -1;
    }

    m->size += 2;
    return 0;
}

int snapshot_module_write_dword(snapshot_module_t *m, DWORD dw)
{
    if (snapshot_write_dword(m->file, dw) < 0) {
        return -1;
    }

    m->size += 4;
    return 0;
}

int snapshot_module_write_double(snapshot_module_t *m, double db)
{
    if (snapshot_write_double(m->file, db) < 0) {
        return -1;
    }

    m->size += 8;
    return 0;
}

int snapshot_module_write_padded_string(snapshot_module_t *m, const char *s, BYTE pad_char, int len)
{
    if (snapshot_write_padded_string(m->file, s, (BYTE)pad_char, len) < 0) {
        return -1;
    }

    m->size += len;
    return 0;
}

int snapshot_module_write_byte_array(snapshot_module_t *m, const BYTE *b, unsigned int num)
{
    if (snapshot_write_byte_array(m->file, b, num) < 0) {
        return -1;
    }

    m->size += num;
    return 0;
}

int snapshot_module_write_word_array(snapshot_module_t *m, const WORD *w, unsigned int num)
{
    if (snapshot_write_word_array(m->file, w, num) < 0) {
        return -1;
    }

    m->size += num * sizeof(WORD);
    return 0;
}

int snapshot_module_write_dword_array(snapshot_module_t *m, const DWORD *dw, unsigned int num)
{
    if (snapshot_write_dword_array(m->file, dw, num) < 0) {
        return -1;
    }

    m->size += num * sizeof(DWORD);
    return 0;
}

int snapshot_module_write_string(snapshot_module_t *m, const char *s)
{
    int len;
    len = snapshot_write_string(m->file, s);
    if (len < 0) {
        snapshot_error = SNAPSHOT_ILLEGAL_STRING_LENGTH_ERROR;
        return -1;
    }

    m->size += len;
    return 0;
}

/* ------------------------------------------------------------------------- */

int snapshot_module_read_byte(snapshot_module_t *m, BYTE *b_return)
{
    if (ftell(m->file) + sizeof(BYTE) > m->offset + m->size) {
        snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
        return -1;
    }

    return snapshot_read_byte(m->file, b_return);
}

int snapshot_module_read_word(snapshot_module_t *m, WORD *w_return)
{
    if (ftell(m->file) + sizeof(WORD) > m->offset + m->size) {
        snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
        return -1;
    }

    return snapshot_read_word(m->file, w_return);
}

int snapshot_module_read_dword(snapshot_module_t *m, DWORD *dw_return)
{
    if (ftell(m->file) + sizeof(DWORD) > m->offset + m->size) {
        snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
        return -1;
    }

    return snapshot_read_dword(m->file, dw_return);
}

int snapshot_module_read_double(snapshot_module_t *m, double *db_return)
{
    if (ftell(m->file) + sizeof(double) > m->offset + m->size) {
        snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
        return -1;
    }

    return snapshot_read_double(m->file, db_return);
}

int snapshot_module_read_byte_array(snapshot_module_t *m, BYTE *b_return, unsigned int num)
{
    if ((long)(ftell(m->file) + num) > (long)(m->offset + m->size)) {
        snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
        return -1;
    }

    return snapshot_read_byte_array(m->file, b_return, num);
}

int snapshot_module_read_word_array(snapshot_module_t *m, WORD *w_return, unsigned int num)
{
    if ((long)(ftell(m->file) + num * sizeof(WORD)) > (long)(m->offset + m->size)) {
        snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
        return -1;
    }

    return snapshot_read_word_array(m->file, w_return, num);
}

int snapshot_module_read_dword_array(snapshot_module_t *m, DWORD *dw_return, unsigned int num)
{
    if ((long)(ftell(m->file) + num * sizeof(DWORD)) > (long)(m->offset + m->size)) {
        snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
        return -1;
    }

    return snapshot_read_dword_array(m->file, dw_return, num);
}

int snapshot_module_read_string(snapshot_module_t *m, char **charp_return)
{
    if (ftell(m->file) + sizeof(WORD) > m->offset + m->size) {
        snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
        return -1;
    }

    return snapshot_read_string(m->file, charp_return);
}

int snapshot_module_read_byte_into_int(snapshot_module_t *m, int *value_return)
{
    BYTE b;

    if (snapshot_module_read_byte(m, &b) < 0) {
        return -1;
    }
    *value_return = (int)b;
    return 0;
}

int snapshot_module_read_word_into_int(snapshot_module_t *m, int *value_return)
{
    WORD b;

    if (snapshot_module_read_word(m, &b) < 0) {
        return -1;
    }
    *value_return = (int)b;
    return 0;
}

int snapshot_module_read_dword_into_ulong(snapshot_module_t *m, unsigned long *value_return)
{
    DWORD b;

    if (snapshot_module_read_dword(m, &b) < 0) {
        return -1;
    }
    *value_return = (unsigned long)b;
    return 0;
}

int snapshot_module_read_dword_into_int(snapshot_module_t *m, int *value_return)
{
    DWORD b;

    if (snapshot_module_read_dword(m, &b) < 0) {
        return -1;
    }
    *value_return = (int)b;
    return 0;
}

int snapshot_module_read_dword_into_uint(snapshot_module_t *m, unsigned int *value_return)
{
    DWORD b;

    if (snapshot_module_read_dword(m, &b) < 0) {
        return -1;
    }
    *value_return = (unsigned int)b;
    return 0;
}

/* ------------------------------------------------------------------------- */

snapshot_module_t *snapshot_module_create(snapshot_t *s, const char *name, BYTE major_version, BYTE minor_version)
{
    snapshot_module_t *m;

    current_module = (char *)name;

    m = lib_malloc(sizeof(snapshot_module_t));
    m->file = s->file;
    m->offset = ftell(s->file);
    if (m->offset == -1) {
        snapshot_error = SNAPSHOT_ILLEGAL_OFFSET_ERROR;
        lib_free(m);
        return NULL;
    }
    m->write_mode = 1;

    if (snapshot_write_padded_string(s->file, name, (BYTE)0, SNAPSHOT_MODULE_NAME_LEN) < 0
        || snapshot_write_byte(s->file, major_version) < 0
        || snapshot_write_byte(s->file, minor_version) < 0
        || snapshot_write_dword(s->file, 0) < 0) {
        return NULL;
    }

    m->size = ftell(s->file) - m->offset;
    m->size_offset = ftell(s->file) - sizeof(DWORD);

    return m;
}

snapshot_module_t *snapshot_module_open(snapshot_t *s, const char *name, BYTE *major_version_return, BYTE *minor_version_return)
{
    snapshot_module_t *m;
    char n[SNAPSHOT_MODULE_NAME_LEN];
    unsigned int name_len = (unsigned int)strlen(name);

    current_module = (char *)name;

    if (fseek(s->file, s->first_module_offset, SEEK_SET) < 0) {
        snapshot_error = SNAPSHOT_FIRST_MODULE_NOT_FOUND_ERROR;
        return NULL;
    }

    m = lib_malloc(sizeof(snapshot_module_t));
    m->file = s->file;
    m->write_mode = 0;

    m->offset = s->first_module_offset;

    /* Search for the module name.  This is quite inefficient, but I don't
       think we care.  */
    while (1) {
        if (snapshot_read_byte_array(s->file, (BYTE *)n,
                                     SNAPSHOT_MODULE_NAME_LEN) < 0
            || snapshot_read_byte(s->file, major_version_return) < 0
            || snapshot_read_byte(s->file, minor_version_return) < 0
            || snapshot_read_dword(s->file, &m->size)) {
            snapshot_error = SNAPSHOT_MODULE_HEADER_READ_ERROR;
            goto fail;
        }

        /* Found?  */
        if (memcmp(n, name, name_len) == 0
            && (name_len == SNAPSHOT_MODULE_NAME_LEN || n[name_len] == 0)) {
            break;
        }

        m->offset += m->size;
        if (fseek(s->file, m->offset, SEEK_SET) < 0) {
            snapshot_error = SNAPSHOT_MODULE_NOT_FOUND_ERROR;
            goto fail;
        }
    }

    m->size_offset = ftell(s->file) - sizeof(DWORD);

    return m;

fail:
    fseek(s->file, s->first_module_offset, SEEK_SET);
    lib_free(m);
    return NULL;
}

int snapshot_module_close(snapshot_module_t *m)
{
    /* Backpatch module size if writing.  */
    if (m->write_mode
        && (fseek(m->file, m->size_offset, SEEK_SET) < 0
            || snapshot_write_dword(m->file, m->size) < 0)) {
        snapshot_error = SNAPSHOT_MODULE_CLOSE_ERROR;
        return -1;
    }

    /* Skip module.  */
    if (fseek(m->file, m->offset + m->size, SEEK_SET) < 0) {
        snapshot_error = SNAPSHOT_MODULE_SKIP_ERROR;
        return -1;
    }

    lib_free(m);
    return 0;
}

/* ------------------------------------------------------------------------- */

snapshot_t *snapshot_create(const char *filename, BYTE major_version, BYTE minor_version, const char *snapshot_machine_name)
{
    FILE *f;
    snapshot_t *s;
    unsigned char viceversion[4] = { VERSION_RC_NUMBER };

    current_filename = (char *)filename;

    f = fopen(filename, MODE_WRITE);
    if (f == NULL) {
        snapshot_error = SNAPSHOT_CANNOT_CREATE_SNAPSHOT_ERROR;
        return NULL;
    }

    /* Magic string.  */
    if (snapshot_write_padded_string(f, snapshot_magic_string, (BYTE)0, SNAPSHOT_MAGIC_LEN) < 0) {
        snapshot_error = SNAPSHOT_CANNOT_WRITE_MAGIC_STRING_ERROR;
        goto fail;
    }

    /* Version number.  */
    if (snapshot_write_byte(f, major_version) < 0
        || snapshot_write_byte(f, minor_version) < 0) {
        snapshot_error = SNAPSHOT_CANNOT_WRITE_VERSION_ERROR;
        goto fail;
    }

    /* Machine.  */
    if (snapshot_write_padded_string(f, snapshot_machine_name, (BYTE)0, SNAPSHOT_MACHINE_NAME_LEN) < 0) {
        snapshot_error = SNAPSHOT_CANNOT_WRITE_MACHINE_NAME_ERROR;
        goto fail;
    }

    /* VICE version and revision */
    if (snapshot_write_padded_string(f, snapshot_version_magic_string, (BYTE)0, SNAPSHOT_VERSION_MAGIC_LEN) < 0) {
        snapshot_error = SNAPSHOT_CANNOT_WRITE_MAGIC_STRING_ERROR;
        goto fail;
    }

    if (snapshot_write_byte(f, viceversion[0]) < 0
        || snapshot_write_byte(f, viceversion[1]) < 0
        || snapshot_write_byte(f, viceversion[2]) < 0
        || snapshot_write_byte(f, viceversion[3]) < 0
#ifdef USE_SVN_REVISION
        || snapshot_write_dword(f, VICE_SVN_REV_NUMBER) < 0) {
#else
        || snapshot_write_dword(f, 0) < 0) {
#endif
        snapshot_error = SNAPSHOT_CANNOT_WRITE_VERSION_ERROR;
        goto fail;
    }

    s = lib_malloc(sizeof(snapshot_t));
    s->file = f;
    s->first_module_offset = ftell(f);
    s->write_mode = 1;

    return s;

fail:
    fclose(f);
    ioutil_remove(filename);
    return NULL;
}

/* informal only, used by the error message created below */
static unsigned char snapshot_viceversion[4];
static DWORD snapshot_vicerevision;

snapshot_t *snapshot_open(const char *filename, BYTE *major_version_return, BYTE *minor_version_return, const char *snapshot_machine_name)
{
    FILE *f;
    char magic[SNAPSHOT_MAGIC_LEN];
    snapshot_t *s = NULL;
    int machine_name_len;
    size_t offs;

    current_machine_name = (char *)snapshot_machine_name;
    current_filename = (char *)filename;
    current_module = NULL;

    f = zfile_fopen(filename, MODE_READ);
    if (f == NULL) {
        snapshot_error = SNAPSHOT_CANNOT_OPEN_FOR_READ_ERROR;
        return NULL;
    }

    /* Magic string.  */
    if (snapshot_read_byte_array(f, (BYTE *)magic, SNAPSHOT_MAGIC_LEN) < 0
        || memcmp(magic, snapshot_magic_string, SNAPSHOT_MAGIC_LEN) != 0) {
        snapshot_error = SNAPSHOT_MAGIC_STRING_MISMATCH_ERROR;
        goto fail;
    }

    /* Version number.  */
    if (snapshot_read_byte(f, major_version_return) < 0
        || snapshot_read_byte(f, minor_version_return) < 0) {
        snapshot_error = SNAPSHOT_CANNOT_READ_VERSION_ERROR;
        goto fail;
    }

    /* Machine.  */
    if (snapshot_read_byte_array(f, (BYTE *)read_name, SNAPSHOT_MACHINE_NAME_LEN) < 0) {
        snapshot_error = SNAPSHOT_CANNOT_READ_MACHINE_NAME_ERROR;
        goto fail;
    }

    /* Check machine name.  */
    machine_name_len = (int)strlen(snapshot_machine_name);
    if (memcmp(read_name, snapshot_machine_name, machine_name_len) != 0
        || (machine_name_len != SNAPSHOT_MODULE_NAME_LEN
            && read_name[machine_name_len] != 0)) {
        snapshot_error = SNAPSHOT_MACHINE_MISMATCH_ERROR;
        goto fail;
    }

    /* VICE version and revision */
    memset(snapshot_viceversion, 0, 4);
    snapshot_vicerevision = 0;
    offs = ftell(f);

    if (snapshot_read_byte_array(f, (BYTE *)magic, SNAPSHOT_VERSION_MAGIC_LEN) < 0
        || memcmp(magic, snapshot_version_magic_string, SNAPSHOT_VERSION_MAGIC_LEN) != 0) {
        /* old snapshots do not contain VICE version */
        fseek(f, offs, SEEK_SET);
        log_warning(LOG_DEFAULT, "attempting to load pre 2.4.30 snapshot");
    } else {
        /* actually read the version */
        if (snapshot_read_byte(f, &snapshot_viceversion[0]) < 0
            || snapshot_read_byte(f, &snapshot_viceversion[1]) < 0
            || snapshot_read_byte(f, &snapshot_viceversion[2]) < 0
            || snapshot_read_byte(f, &snapshot_viceversion[3]) < 0
            || snapshot_read_dword(f, &snapshot_vicerevision) < 0) {
            snapshot_error = SNAPSHOT_CANNOT_READ_VERSION_ERROR;
            goto fail;
        }
    }

    s = lib_malloc(sizeof(snapshot_t));
    s->file = f;
    s->first_module_offset = ftell(f);
    s->write_mode = 0;

    vsync_suspend_speed_eval();
    return s;

fail:
    fclose(f);
    return NULL;
}

int snapshot_close(snapshot_t *s)
{
    int retval;

    if (!s->write_mode) {
        if (zfile_fclose(s->file) == EOF) {
            snapshot_error = SNAPSHOT_READ_CLOSE_EOF_ERROR;
            retval = -1;
        } else {
            retval = 0;
        }
    } else {
        if (fclose(s->file) == EOF) {
            snapshot_error = SNAPSHOT_WRITE_CLOSE_EOF_ERROR;
            retval = -1;
        } else {
            retval = 0;
        }
    }

    lib_free(s);
    return retval;
}

static void display_error_with_vice_version(char *text, char *filename)
{
    char *vmessage = lib_malloc(0x100);
    char *message = lib_malloc(0x100 + strlen(text));
    if ((snapshot_viceversion[0] == 0) && (snapshot_viceversion[1] == 0)) {
        /* generic message for the case when no version is present in the snapshot */
        strcpy(vmessage, translate_text(IDGS_SNAPSHOT_OLD_VICE_VERSION));
    } else {
        sprintf(vmessage, translate_text(IDGS_SNAPSHOT_VICE_VERSION),
                snapshot_viceversion[0], snapshot_viceversion[1], snapshot_viceversion[2]);
        if (snapshot_vicerevision != 0) {
            sprintf(message, " (r%d)", (int)snapshot_vicerevision);
            strcat(vmessage, message);
        }
    }
    sprintf(message, "%s\n\n%s.", text, vmessage);
    ui_error(message, filename);
    lib_free(message);
    lib_free(vmessage);
}

void snapshot_display_error(void)
{
    switch (snapshot_error) {
        default:
        case SNAPSHOT_NO_ERROR:
            break;
        case SNAPSHOT_WRITE_EOF_ERROR:
            if (current_module) {
                ui_error(translate_text(IDGS_EOF_WRITING_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            } else {
                ui_error(translate_text(IDGS_EOF_WRITING_SNAPSHOT_S), current_filename);
            }
            break;
        case SNAPSHOT_WRITE_BYTE_ARRAY_ERROR:
            if (current_module) {
                ui_error(translate_text(IDGS_ERROR_WRITING_ARRAY_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            } else {
                ui_error(translate_text(IDGS_ERROR_WRITING_ARRAY_SNAPSHOT_S), current_filename);
            }
            break;
        case SNAPSHOT_READ_EOF_ERROR:
            if (current_module) {
                ui_error(translate_text(IDGS_EOF_READING_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            } else {
                ui_error(translate_text(IDGS_EOF_READING_SNAPSHOT_S), current_filename);
            }
            break;
        case SNAPSHOT_READ_BYTE_ARRAY_ERROR:
            if (current_module) {
                ui_error(translate_text(IDGS_ERROR_READING_ARRAY_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            } else {
                ui_error(translate_text(IDGS_ERROR_READING_ARRAY_SNAPSHOT_S), current_filename);
            }
            break;
        case SNAPSHOT_ILLEGAL_STRING_LENGTH_ERROR:
            if (current_module) {
                ui_error(translate_text(IDGS_ERROR_WRITING_STRING_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            } else {
                ui_error(translate_text(IDGS_ERROR_WRITING_STRING_SNAPSHOT_S), current_filename);
            }
            break;
        case SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR:
            if (current_module) {
                ui_error(translate_text(IDGS_OUT_OF_BOUNDS_READING_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            } else {
                ui_error(translate_text(IDGS_OUT_OF_BOUNDS_READING_SNAPSHOT_S), current_filename);
            }
            break;
        case SNAPSHOT_ILLEGAL_OFFSET_ERROR:
            ui_error(translate_text(IDGS_ILLEGAL_OFFSET_CREATE_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            break;
        case SNAPSHOT_FIRST_MODULE_NOT_FOUND_ERROR:
            ui_error(translate_text(IDGS_CANNOT_FIND_1ST_MODULE_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_MODULE_HEADER_READ_ERROR:
            ui_error(translate_text(IDGS_ERROR_MODULE_HEADER_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_MODULE_NOT_FOUND_ERROR:
            ui_error(translate_text(IDGS_CANNOT_FIND_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            break;
        case SNAPSHOT_MODULE_CLOSE_ERROR:
            ui_error(translate_text(IDGS_ERROR_CLOSING_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            break;
        case SNAPSHOT_MODULE_SKIP_ERROR:
            ui_error(translate_text(IDGS_ERROR_SKIPPING_MODULE_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_CANNOT_CREATE_SNAPSHOT_ERROR:
            ui_error(translate_text(IDGS_CANNOT_CREATE_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_CANNOT_WRITE_MAGIC_STRING_ERROR:
            ui_error(translate_text(IDGS_CANNOT_WRITE_MAGIC_STRING_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_CANNOT_WRITE_VERSION_ERROR:
            ui_error(translate_text(IDGS_CANNOT_WRITE_VERSION_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_CANNOT_WRITE_MACHINE_NAME_ERROR:
            ui_error(translate_text(IDGS_CANNOT_WRITE_MACHINE_NAME_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_CANNOT_OPEN_FOR_READ_ERROR:
            ui_error(translate_text(IDGS_CANNOT_OPEN_SNAPSHOT_S_READING), current_filename);
            break;
        case SNAPSHOT_MAGIC_STRING_MISMATCH_ERROR:
            ui_error(translate_text(IDGS_MAGIC_STRING_MISMATCH_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_CANNOT_READ_VERSION_ERROR:
            ui_error(translate_text(IDGS_CANNOT_READ_VERSION_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_CANNOT_READ_MACHINE_NAME_ERROR:
            ui_error(translate_text(IDGS_CANNOT_READ_MACHINE_NAME_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_MACHINE_MISMATCH_ERROR:
            ui_error(translate_text(IDGS_WRONG_MACHINE_TYPE_SNAPSHOT_S), current_filename, read_name, current_machine_name);
            break;
        case SNAPSHOT_READ_CLOSE_EOF_ERROR:
        case SNAPSHOT_WRITE_CLOSE_EOF_ERROR:
            ui_error(translate_text(IDGS_EOF_CLOSING_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_MODULE_HIGHER_VERSION:
            display_error_with_vice_version(translate_text(IDGS_SNAPSHOT_HIGHER_VERSION), current_filename);
            break;
        case SNAPSHOT_MODULE_INCOMPATIBLE:
            display_error_with_vice_version(translate_text(IDGS_INCOMPATIBLE_SNAPSHOT), current_filename);
            break;
    }
}

void snapshot_set_error(int error)
{
    snapshot_error = error;
}

int snapshot_version_at_least(BYTE major_version, BYTE minor_version, BYTE major_version_required, BYTE minor_version_required)
{
    if (major_version != major_version_required) {
        return 0;
    }

    if (minor_version >= minor_version_required) {
        return 1;
    }

    return 0;
}
