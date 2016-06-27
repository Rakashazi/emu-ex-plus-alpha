/*
 * zfile.c - Transparent handling of compressed files.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *
 * ARCHIVE, ZIPCODE and LYNX supports added by
 *  Teemu Rantanen <tvr@cs.hut.fi>
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

/* This code might be improved a lot...  */

#include "vice.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "archdep.h"
#include "ioutil.h"
#include "lib.h"
#include "log.h"
#include "util.h"
#include "zfile.h"
#include "zipcode.h"


/* ------------------------------------------------------------------------- */

/* #define DEBUG_ZFILE */

#ifdef DEBUG_ZFILE
#define ZDEBUG(a)  log_debug a
#else
#define ZDEBUG(a)
#endif

/* We could add more here...  */
enum compression_type {
    COMPR_NONE,
    COMPR_GZIP,
    COMPR_BZIP,
    COMPR_ARCHIVE,
    COMPR_ZIPCODE,
    COMPR_LYNX,
    COMPR_TZX
};

/* This defines a linked list of all the compressed files that have been
   opened.  */
struct zfile_s {
    char *tmp_name;              /* Name of the temporary file.  */
    char *orig_name;             /* Name of the original file.  */
    int write_mode;              /* Non-zero if the file is open for writing.*/
    FILE *stream;                /* Associated stdio-style stream.  */
    FILE *fd;                    /* Associated file descriptor.  */
    enum compression_type type;  /* Compression algorithm.  */
    struct zfile_s *prev, *next; /* Link to the previous and next nodes.  */
    zfile_action_t action;       /* action on close */
    char *request_string;        /* ui string for action=ZFILE_REQUEST */
};
typedef struct zfile_s zfile_t;

static zfile_t *zfile_list = NULL;

static log_t zlog = LOG_ERR;

/* ------------------------------------------------------------------------- */

static int zinit_done = 0;

static void zfile_list_destroy(void)
{
    zfile_t *p;

    for (p = zfile_list; p != NULL; ) {
        zfile_t *next;

        lib_free(p->orig_name);
        lib_free(p->tmp_name);
        next = p->next;
        lib_free(p);
        p = next;
    }

    zfile_list = NULL;
}

static int zinit(void)
{
    zlog = log_open("ZFile");

    /* Free the `zfile_list' if not empty.  */
    zfile_list_destroy();

    zinit_done = 1;

    return 0;
}

/* Add one zfile to the list.  `orig_name' is automatically expanded to the
   complete path.  */
static void zfile_list_add(const char *tmp_name,
                           const char *orig_name,
                           enum compression_type type,
                           int write_mode,
                           FILE *stream, FILE *fd)
{
    zfile_t *new_zfile = lib_malloc(sizeof(zfile_t));

    /* Make sure we have the complete path of the file.  */
    archdep_expand_path(&new_zfile->orig_name, orig_name);

    /* The new zfile becomes first on the list.  */
    new_zfile->tmp_name = tmp_name ? lib_stralloc(tmp_name) : NULL;
    new_zfile->write_mode = write_mode;
    new_zfile->stream = stream;
    new_zfile->fd = fd;
    new_zfile->type = type;
    new_zfile->action = ZFILE_KEEP;
    new_zfile->request_string = NULL;
    new_zfile->next = zfile_list;
    new_zfile->prev = NULL;
    if (zfile_list != NULL) {
        zfile_list->prev = new_zfile;
    }
    zfile_list = new_zfile;
}

void zfile_shutdown(void)
{
    zfile_list_destroy();
}

/* ------------------------------------------------------------------------ */

/* Uncompression.  */

/* If `name' has a gzip-like extension, try to uncompress it into a temporary
   file using gzip or zlib if available.  If this succeeds, return the name
   of the temporary file; return NULL otherwise.  */
static char *try_uncompress_with_gzip(const char *name)
{
#ifdef HAVE_ZLIB
    FILE *fddest;
    gzFile fdsrc;
    char *tmp_name = NULL;
    int len;

    if (!archdep_file_is_gzip(name)) {
        return NULL;
    }

    fddest = archdep_mkstemp_fd(&tmp_name, MODE_WRITE);

    if (fddest == NULL) {
        return NULL;
    }

    fdsrc = gzopen(name, MODE_READ);
    if (fdsrc == NULL) {
        fclose(fddest);
        ioutil_remove(tmp_name);
        lib_free(tmp_name);
        return NULL;
    }

    do {
        char buf[256];

        len = gzread(fdsrc, (void *)buf, 256);
        if (len > 0) {
            if (fwrite((void *)buf, 1, (size_t)len, fddest) < len) {
                gzclose(fdsrc);
                fclose(fddest);
                ioutil_remove(tmp_name);
                lib_free(tmp_name);
                return NULL;
            }
        }
    } while (len > 0);

    gzclose(fdsrc);
    fclose(fddest);

    return tmp_name;
#else
    char *tmp_name = NULL;
    int exit_status;
    char *argv[4];

    if (!archdep_file_is_gzip(name)) {
        return NULL;
    }

    /* `exec*()' does not want these to be constant...  */
    argv[0] = lib_stralloc("gzip");
    argv[1] = lib_stralloc("-cd");
    argv[2] = archdep_filename_parameter(name);
    argv[3] = NULL;

    ZDEBUG(("try_uncompress_with_gzip: spawning gzip -cd %s", name));
    exit_status = archdep_spawn("gzip", argv, &tmp_name, NULL);

    lib_free(argv[0]);
    lib_free(argv[1]);
    lib_free(argv[2]);

    if (exit_status == 0) {
        ZDEBUG(("try_uncompress_with_gzip: OK"));
        return tmp_name;
    } else {
        ZDEBUG(("try_uncompress_with_gzip: failed"));
        ioutil_remove(tmp_name);
        lib_free(tmp_name);
        return NULL;
    }
#endif
}

/* If `name' has a bzip-like extension, try to uncompress it into a temporary
   file using bzip.  If this succeeds, return the name of the temporary file;
   return NULL otherwise.  */
static char *try_uncompress_with_bzip(const char *name)
{
    char *tmp_name = NULL;
    size_t l = strlen(name);
    int exit_status;
    char *argv[4];

    /* Check whether the name sounds like a bzipped file by checking the
       extension.  MSDOS and UNIX variants of bzip v2 use the extension
       '.bz2'.  bzip v1 is obsolete.  */
    if (l < 5 || strcasecmp(name + l - 4, ".bz2") != 0) {
        return NULL;
    }

    /* `exec*()' does not want these to be constant...  */
    argv[0] = lib_stralloc("bzip2");
    argv[1] = lib_stralloc("-cd");
    argv[2] = archdep_filename_parameter(name);
    argv[3] = NULL;

    ZDEBUG(("try_uncompress_with_bzip: spawning bzip -cd %s", name));
    exit_status = archdep_spawn("bzip2", argv, &tmp_name, NULL);

    lib_free(argv[0]);
    lib_free(argv[1]);
    lib_free(argv[2]);

    if (exit_status == 0) {
        ZDEBUG(("try_uncompress_with_bzip: OK"));
        return tmp_name;
    } else {
        ZDEBUG(("try_uncompress_with_bzip: failed"));
        ioutil_remove(tmp_name);
        lib_free(tmp_name);
        return NULL;
    }
}

static char *try_uncompress_with_tzx(const char *name)
{
    char *tmp_name = NULL;
    size_t l = strlen(name);
    int exit_status;
    char *argv[4];

    /* Check whether the name sounds like a tzx file. */
    if (l < 4 || strcasecmp(name + l - 4, ".tzx") != 0) {
        return NULL;
    }

    /* `exec*()' does not want these to be constant...  */
    argv[0] = lib_stralloc("64tzxtap");
    argv[1] = archdep_filename_parameter(name);
    argv[2] = NULL;

    ZDEBUG(("try_uncompress_with_tzx: spawning 64tzxtap %s", name));
    exit_status = archdep_spawn("64tzxtap", argv, &tmp_name, NULL);

    lib_free(argv[0]);
    lib_free(argv[1]);

    if (exit_status == 0) {
        ZDEBUG(("try_uncompress_with_tzx: OK"));
        return tmp_name;
    } else {
        ZDEBUG(("try_uncompress_with_tzx: failed"));
        ioutil_remove(tmp_name);
        lib_free(tmp_name);
        return NULL;
    }
}

/* is the name zipcode -name? */
static int is_zipcode_name(char *name)
{
    if (name[0] >= '1' && name[0] <= '4' && name[1] == '!') {
        return 1;
    }
    return 0;
}

/* Extensions we know about */
static const char *extensions[] = {
    FSDEV_EXT_SEP_STR "d64",
    FSDEV_EXT_SEP_STR "d67",
    FSDEV_EXT_SEP_STR "d71",
    FSDEV_EXT_SEP_STR "d80",
    FSDEV_EXT_SEP_STR "d81",
    FSDEV_EXT_SEP_STR "d82",
    FSDEV_EXT_SEP_STR "d1m",
    FSDEV_EXT_SEP_STR "d2m",
    FSDEV_EXT_SEP_STR "d4m",
    FSDEV_EXT_SEP_STR "g64",
    FSDEV_EXT_SEP_STR "p64",
    FSDEV_EXT_SEP_STR "g41",
    FSDEV_EXT_SEP_STR "x64",
    FSDEV_EXT_SEP_STR "dsk",
    FSDEV_EXT_SEP_STR "t64",
    FSDEV_EXT_SEP_STR "p00",
    FSDEV_EXT_SEP_STR "prg",
    FSDEV_EXT_SEP_STR "lnx",
    FSDEV_EXT_SEP_STR "tap",
    NULL
};

static int is_valid_extension(char *end, size_t l, int nameoffset)
{
    int i;
    size_t len;

    /* zipcode testing is a special case */
    if (l > nameoffset + 2u && is_zipcode_name(end + nameoffset)) {
        return 1;
    }
    /* others */
    for (i = 0; extensions[i]; i++) {
        len = strlen(extensions[i]);
        if (l < nameoffset + len) {
            continue;
        }
        if (!strcasecmp(extensions[i], end + l - len)) {
            return 1;
        }
    }
    return 0;
}

/* define SIZE_MAX if it does not exist (only in C99) */
#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

/* If `name' has a correct extension, try to list its contents and search for
   the first file with a proper extension; if found, extract it.  If this
   succeeds, return the name of the temporary file; if the archive file is
   valid but `write_mode' is non-zero, return a zero-length string; in all
   the other cases, return NULL.  */
static char *try_uncompress_archive(const char *name, int write_mode,
                                    const char *program,
                                    const char *listopts,
                                    const char *extractopts,
                                    const char *extension,
                                    const char *search)
{
    char *tmp_name = NULL;
    size_t l = strlen(name), len, nameoffset;
    int found = 0;
    int exit_status;
    char *argv[8];
    FILE *fd;
    char tmp[1024];

    /* Do we have correct extension?  */
    len = strlen(extension);
    if (l <= len || strcasecmp(name + l - len, extension) != 0) {
        return NULL;
    }

    /* First run listing and search for first recognizeable extension.  */
    argv[0] = lib_stralloc(program);
    argv[1] = lib_stralloc(listopts);
    argv[2] = archdep_filename_parameter(name);
    argv[3] = NULL;

    ZDEBUG(("try_uncompress_archive: spawning `%s %s %s'",
            program, listopts, name));
    exit_status = archdep_spawn(program, argv, &tmp_name, NULL);

    lib_free(argv[0]);
    lib_free(argv[1]);
    lib_free(argv[2]);

    /* No luck?  */
    if (exit_status != 0) {
        ZDEBUG(("try_uncompress_archive: `%s %s' failed.", program, listopts));
        ioutil_remove(tmp_name);
        lib_free(tmp_name);
        return NULL;
    }

    ZDEBUG(("try_uncompress_archive: `%s %s' successful.", program, listopts));

    fd = fopen(tmp_name, MODE_READ);
    if (!fd) {
        ZDEBUG(("try_uncompress_archive: cannot read `%s %s' output.",
                program, tmp_name));
        ioutil_remove(tmp_name);
        lib_free(tmp_name);
        return NULL;
    }

    ZDEBUG(("try_uncompress_archive: searching for the first valid file."));

    /* Search for `search' first (if any) to see the offset where
       filename begins, then search for first recognizeable file.  */
    nameoffset = search ? SIZE_MAX : 0;
    len = search ? strlen(search) : 0;
    while (!feof(fd) && !found) {
        if (fgets(tmp, 1024, fd) == NULL) {
            break;
        }
        l = strlen(tmp);
        while (l > 0) {
            tmp[--l] = 0;
            if ((/* (nameoffset == SIZE_MAX) || */ (nameoffset > 1024)) && l >= len &&
                !strcasecmp(tmp + l - len, search) != 0) {
                nameoffset = l - 4;
            }
            if (/* nameoffset >= 0 && */ nameoffset <= 1024 && is_valid_extension(tmp, l, nameoffset)) {
                ZDEBUG(("try_uncompress_archive: found `%s'.",
                        tmp + nameoffset));
                found = 1;
                break;
            }
        }
    }

    fclose(fd);
    ioutil_remove(tmp_name);
    if (!found) {
        ZDEBUG(("try_uncompress_archive: no valid file found."));
        lib_free(tmp_name);
        return NULL;
    }

    /* This would be a valid ZIP file, but we cannot handle ZIP files in
       write mode.  Return a null temporary file name to report this.  */
    if (write_mode) {
        ZDEBUG(("try_uncompress_archive: cannot open file in write mode."));
        lib_free(tmp_name);
        return "";
    }

    /* And then file inside zip.  If we have a zipcode extract all of them
       to the same file. */
    argv[0] = lib_stralloc(program);
    argv[1] = lib_stralloc(extractopts);
    argv[2] = archdep_filename_parameter(name);
    if (is_zipcode_name(tmp + nameoffset)) {
        argv[3] = lib_stralloc(tmp + nameoffset);
        argv[4] = lib_stralloc(tmp + nameoffset);
        argv[5] = lib_stralloc(tmp + nameoffset);
        argv[6] = lib_stralloc(tmp + nameoffset);
        argv[7] = NULL;
        argv[3][0] = '1';
        argv[4][0] = '2';
        argv[5][0] = '3';
        argv[6][0] = '4';
    } else {
        argv[3] = archdep_quote_parameter(tmp + nameoffset);
        argv[4] = NULL;
    }

    ZDEBUG(("try_uncompress_archive: spawning `%s %s %s %s'.",
            program, extractopts, name, tmp + nameoffset));
    exit_status = archdep_spawn(program, argv, &tmp_name, NULL);

    lib_free(argv[0]);
    lib_free(argv[1]);
    lib_free(argv[2]);
    lib_free(argv[3]);
    if (is_zipcode_name(tmp + nameoffset)) {
        lib_free(argv[4]);
        lib_free(argv[5]);
        lib_free(argv[6]);
    }

    if (exit_status != 0) {
        ZDEBUG(("try_uncompress_archive: `%s %s' failed.",
                program, extractopts));
        ioutil_remove(tmp_name);
        lib_free(tmp_name);
        return NULL;
    }

    ZDEBUG(("try_uncompress_archive: `%s %s' successful.", program, tmp_name));
    return tmp_name;
}

#define C1541_NAME     "c1541"

/* If this file looks like a zipcode, try to extract is using c1541. We have
   to figure this out by reading the contents of the file */
static char *try_uncompress_zipcode(const char *name, int write_mode)
{
    char *tmp_name = NULL;
    int i, count, sector, sectors = 0;
    FILE *fd;
    char tmp[256];
    char *argv[5];
    int exit_status;

    /* The 2nd char has to be '!'?  */
    util_fname_split(name, NULL, &tmp_name);
    if (tmp_name == NULL) {
        return NULL;
    }
    if (strlen(tmp_name) < 3 || tmp_name[1] != '!') {
        lib_free(tmp_name);
        return NULL;
    }
    lib_free(tmp_name);

    /* Can we read this file?  */
    fd = fopen(name, MODE_READ);
    if (fd == NULL) {
        return NULL;
    }
    /* Read first track to see if this is zipcode.  */
    fseek(fd, 4, SEEK_SET);
    for (count = 1; count < 21; count++) {
        i = zipcode_read_sector(fd, 1, &sector, tmp);
        if (i || sector < 0 || sector > 20 || (sectors & (1 << sector))) {
            fclose(fd);
            return NULL;
        }
        sectors |= 1 << sector;
    }
    fclose(fd);

    /* it is a zipcode. We cannot support write_mode */
    if (write_mode) {
        return "";
    }

    /* format image first */
    tmp_name = archdep_tmpnam();

    /* ok, now extract the zipcode */
    argv[0] = lib_stralloc(C1541_NAME);
    argv[1] = lib_stralloc("-zcreate");
    argv[2] = lib_stralloc(tmp_name);
    argv[3] = archdep_filename_parameter(name);
    argv[4] = NULL;

    exit_status = archdep_spawn(C1541_NAME, argv, NULL, NULL);

    lib_free(argv[0]);
    lib_free(argv[1]);
    lib_free(argv[2]);
    lib_free(argv[3]);

    if (exit_status) {
        ioutil_remove(tmp_name);
        lib_free(tmp_name);
        return NULL;
    }
    /* everything ok */
    return tmp_name;
}

/* If the file looks like a lynx image, try to extract it using c1541. We have
   to figure this out by reading the contsnts of the file */
static char *try_uncompress_lynx(const char *name, int write_mode)
{
    char *tmp_name;
    size_t i;
    int count;
    FILE *fd;
    char tmp[256];
    char *argv[20];
    int exit_status;

    /* can we read this file? */
    fd = fopen(name, MODE_READ);
    if (fd == NULL) {
        return NULL;
    }
    /* is this lynx -image? */
    i = fread(tmp, 1, 2, fd);
    if (i != 2 || tmp[0] != 1 || tmp[1] != 8) {
        fclose(fd);
        return NULL;
    }
    count = 0;
    while (1) {
        i = fread(tmp, 1, 1, fd);
        if (i != 1) {
            fclose(fd);
            return NULL;
        }
        if (tmp[0]) {
            count = 0;
        } else {
            count++;
        }
        if (count == 3) {
            break;
        }
    }
    i = fread(tmp, 1, 1, fd);
    if (i != 1 || tmp[0] != 13) {
        fclose(fd);
        return NULL;
    }
    count = 0;
    while (1) {
        i = fread(&tmp[count], 1, 1, fd);
        if (i != 1 || count == 254) {
            fclose(fd);
            return NULL;
        }
        if (tmp[count++] == 13) {
            break;
        }
    }
    tmp[count] = 0;
    if (!atoi(tmp)) {
        fclose(fd);
        return NULL;
    }
    /* XXX: this is not a full check, but perhaps enough? */

    fclose(fd);

    /* it is a lynx image. We cannot support write_mode */
    if (write_mode) {
        return "";
    }

    tmp_name = archdep_tmpnam();

    /* now create the image */
    argv[0] = lib_stralloc("c1541");
    argv[1] = lib_stralloc("-format");
    argv[2] = lib_stralloc("lynximage,00");
    argv[3] = lib_stralloc("x64");
    argv[4] = lib_stralloc(tmp_name);
    argv[5] = lib_stralloc("-unlynx");
    argv[6] = archdep_filename_parameter(name);
    argv[7] = NULL;

    exit_status = archdep_spawn("c1541", argv, NULL, NULL);

    lib_free(argv[0]);
    lib_free(argv[1]);
    lib_free(argv[2]);
    lib_free(argv[3]);
    lib_free(argv[4]);
    lib_free(argv[5]);
    lib_free(argv[6]);

    if (exit_status) {
        ioutil_remove(tmp_name);
        lib_free(tmp_name);
        return NULL;
    }
    /* everything ok */
    return tmp_name;
}

struct valid_archives_s {
    const char *program;
    const char *listopts;
    const char *extractopts;
    const char *extension;
    const char *search;
};
typedef struct valid_archives_s valid_archives_t;

static const valid_archives_t valid_archives[] = {
#ifndef __MSDOS__
    { "unzip",   "-l",   "-p",    ".zip",    "Name" },
    { "lha",     "lv",   "pq",    ".lzh",    NULL },
    { "lha",     "lv",   "pq",    ".lha",    NULL },
    /* Hmmm.  Did non-gnu tar have a -O -option?  */
    { "gtar",    "-tf",  "-xOf",  ".tar",    NULL },
    { "tar",     "-tf",  "-xOf",  ".tar",    NULL },
    { "gtar",    "-ztf", "-zxOf", ".tar.gz", NULL },
    { "tar",     "-ztf", "-zxOf", ".tar.gz", NULL },
    { "gtar",    "-ztf", "-zxOf", ".tgz",    NULL },
    { "tar",     "-ztf", "-zxOf", ".tgz",    NULL },
    /* this might be overkill, but adding this was sooo easy...  */
    { "zoo",     "lf1q", "xpq",   ".zoo",    NULL },
#else
    { "unzip",   "-l",   "-p",    ".zip",    "Name" },
    { "lha",     "l",    "p",     ".lzh",    "Name" },
#endif
    { NULL }
};

/* Try to uncompress file `name' using the algorithms we know of.  If this is
   not possible, return `COMPR_NONE'.  Otherwise, uncompress the file into a
   temporary file, return the type of algorithm used and the name of the
   temporary file in `tmp_name'.  If `write_mode' is non-zero and the
   returned `tmp_name' has zero length, then the file cannot be accessed in
   write mode.  */
static enum compression_type try_uncompress(const char *name,
                                            char **tmp_name,
                                            int write_mode)
{
    int i;

    for (i = 0; valid_archives[i].program; i++) {
        if ((*tmp_name = try_uncompress_archive(name, write_mode,
                                                valid_archives[i].program,
                                                valid_archives[i].listopts,
                                                valid_archives[i].extractopts,
                                                valid_archives[i].extension,
                                                valid_archives[i].search))
            != NULL) {
            return COMPR_ARCHIVE;
        }
    }

    /* need this order or .tar.gz is misunderstood */
    if ((*tmp_name = try_uncompress_with_gzip(name)) != NULL) {
        return COMPR_GZIP;
    }

    if ((*tmp_name = try_uncompress_with_bzip(name)) != NULL) {
        return COMPR_BZIP;
    }

    if ((*tmp_name = try_uncompress_zipcode(name, write_mode)) != NULL) {
        return COMPR_ZIPCODE;
    }

    if ((*tmp_name = try_uncompress_lynx(name, write_mode)) != NULL) {
        return COMPR_LYNX;
    }

    if ((*tmp_name = try_uncompress_with_tzx(name)) != NULL) {
        return COMPR_TZX;
    }

    return COMPR_NONE;
}

/* ------------------------------------------------------------------------- */

/* Compression.  */

/* Compress `src' into `dest' using gzip.  */
static int compress_with_gzip(const char *src, const char *dest)
{
#ifdef HAVE_ZLIB
    FILE *fdsrc;
    gzFile fddest;
    size_t len;

    fdsrc = fopen(dest, MODE_READ);
    if (fdsrc == NULL) {
        return -1;
    }

    fddest = gzopen(src, MODE_WRITE "9");
    if (fddest == NULL) {
        fclose(fdsrc);
        return -1;
    }

    do {
        char buf[256];
        len = fread((void *)buf, 256, 1, fdsrc);
        if (len > 0) {
            gzwrite(fddest, (void *)buf, (unsigned int)len);
        }
    } while (len > 0);

    gzclose(fddest);
    fclose(fdsrc);

    archdep_file_set_gzip(dest);

    ZDEBUG(("compress with zlib: OK."));

    return 0;
#else
    static char *argv[4];
    int exit_status;
    char *mdest;

    /* `exec*()' does not want these to be constant...  */
    argv[0] = lib_stralloc("gzip");
    argv[1] = lib_stralloc("-c");
    argv[2] = lib_stralloc(src);
    argv[3] = NULL;

    mdest = lib_stralloc(dest);

    ZDEBUG(("compress_with_gzip: spawning gzip -c %s", src));
    exit_status = archdep_spawn("gzip", argv, &mdest, NULL);

    lib_free(mdest);

    lib_free(argv[0]);
    lib_free(argv[1]);
    lib_free(argv[2]);

    if (exit_status == 0) {
        ZDEBUG(("compress_with_gzip: OK."));
        return 0;
    } else {
        ZDEBUG(("compress_with_gzip: failed."));
        return -1;
    }
#endif
}

/* Compress `src' into `dest' using bzip.  */
static int compress_with_bzip(const char *src, const char *dest)
{
    static char *argv[4];
    int exit_status;
    char *mdest;

    /* `exec*()' does not want these to be constant...  */
    argv[0] = lib_stralloc("bzip2");
    argv[1] = lib_stralloc("-c");
    argv[2] = lib_stralloc(src);
    argv[3] = NULL;

    mdest = lib_stralloc(dest);

    ZDEBUG(("compress_with_bzip: spawning bzip -c %s", src));
    exit_status = archdep_spawn("bzip2", argv, &mdest, NULL);

    lib_free(mdest);

    lib_free(argv[0]);
    lib_free(argv[1]);
    lib_free(argv[2]);

    if (exit_status == 0) {
        ZDEBUG(("compress_with_bzip: OK."));
        return 0;
    } else {
        ZDEBUG(("compress_with_bzip: failed."));
        return -1;
    }
}

/* Compress `src' into `dest' using algorithm `type'.  */
static int zfile_compress(const char *src, const char *dest,
                          enum compression_type type)
{
    char *dest_backup_name;
    int retval;

    /* This shouldn't happen */
    if (type == COMPR_ARCHIVE) {
        log_error(zlog, "compress: trying to compress archive-file.");
        return -1;
    }

    /* This shouldn't happen */
    if (type == COMPR_ZIPCODE) {
        log_error(zlog, "compress: trying to compress zipcode-file.");
        return -1;
    }

    /* This shouldn't happen */
    if (type == COMPR_LYNX) {
        log_error(zlog, "compress: trying to compress lynx-file.");
        return -1;
    }

    /* This shouldn't happen */
    if (type == COMPR_TZX) {
        log_error(zlog, "compress: trying to compress tzx-file.");
        return -1;
    }

    /* Check whether `compression_type' is a known one.  */
    if (type != COMPR_GZIP && type != COMPR_BZIP) {
        log_error(zlog, "compress: unknown compression type");
        return -1;
    }

    /* If we have no write permissions for `dest', give up.  */
    if (ioutil_access(dest, IOUTIL_ACCESS_W_OK) < 0) {
        ZDEBUG(("compress: no write permissions for `%s'",
                dest));
        return -1;
    }

    if (ioutil_access(dest, IOUTIL_ACCESS_R_OK) < 0) {
        ZDEBUG(("compress: no read permissions for `%s'", dest));
        dest_backup_name = NULL;
    } else {
        /* If `dest' exists, make a backup first.  */
        dest_backup_name = archdep_make_backup_filename(dest);
        if (dest_backup_name != NULL) {
            ZDEBUG(("compress: making backup %s... ", dest_backup_name));
        }
        if (dest_backup_name != NULL
            && ioutil_rename(dest, dest_backup_name) < 0) {
            ZDEBUG(("failed."));
            log_error(zlog, "Could not make pre-compression backup.");
            return -1;
        } else {
            ZDEBUG(("OK."));
        }
    }

    switch (type) {
        case COMPR_GZIP:
            retval = compress_with_gzip(src, dest);
            break;
        case COMPR_BZIP:
            retval = compress_with_bzip(src, dest);
            break;
        default:
            retval = -1;
    }

    if (retval == -1) {
        /* Compression failed: restore original file.  */
        if (dest_backup_name != NULL
            && ioutil_rename(dest_backup_name, dest) < 0) {
            log_error(zlog,
                      "Could not restore backup file after failed compression.");
        }
    } else {
        /* Compression succeeded: remove backup file.  */
        if (dest_backup_name != NULL
            && ioutil_remove(dest_backup_name) < 0) {
            log_error(zlog, "Warning: could not remove backup file.");
            /* Do not return an error anyway (no data is lost).  */
        }
    }

    if (dest_backup_name) {
        lib_free(dest_backup_name);
    }
    return retval;
}

/* ------------------------------------------------------------------------ */

/* Here we have the actual fopen and fclose wrappers.

   These functions work exactly like the standard library versions, but
   handle compression and decompression automatically.  When a file is
   opened, we check whether it looks like a compressed file of some kind.
   If so, we uncompress it and then actually open the uncompressed version.
   When a file that was opened for writing is closed, we re-compress the
   uncompressed version and update the original file.  */

/* `fopen()' wrapper.  */
FILE *zfile_fopen(const char *name, const char *mode)
{
    char *tmp_name;
    FILE *stream;
    enum compression_type type;
    int write_mode = 0;

    if (!zinit_done) {
        zinit();
    }

    if (name == NULL || name[0] == 0) {
        return NULL;
    }

    /* Do we want to write to this file?  */
    if ((strchr(mode, 'w') != NULL) || (strchr(mode, '+') != NULL)) {
        write_mode = 1;
    }

    /* Check for write permissions.  */
    if (write_mode && ioutil_access(name, IOUTIL_ACCESS_W_OK) < 0) {
        return NULL;
    }

    type = try_uncompress(name, &tmp_name, write_mode);
    if (type == COMPR_NONE) {
        stream = fopen(name, mode);
        if (stream == NULL) {
            return NULL;
        }
        zfile_list_add(NULL, name, type, write_mode, stream, NULL);
        return stream;
    } else if (*tmp_name == '\0') {
        errno = EACCES;
        return NULL;
    }

    /* Open the uncompressed version of the file.  */
    stream = fopen(tmp_name, mode);
    if (stream == NULL) {
        return NULL;
    }

    zfile_list_add(tmp_name, name, type, write_mode, stream, NULL);

    /* now we don't need the archdep_tmpnam allocation any more */
    lib_free(tmp_name);

    return stream;
}

/* Handle close-action of a file.  `ptr' points to the zfile to close.  */
static int handle_close_action(zfile_t *ptr)
{
    if (ptr == NULL || ptr->orig_name == NULL) {
        return -1;
    }

    switch (ptr->action) {
        case ZFILE_KEEP:
            break;
        case ZFILE_REQUEST:
        /*
          ui_zfile_close_request(ptr->orig_name, ptr->request_string);
          break;
        */
        case ZFILE_DEL:
            if (ioutil_remove(ptr->orig_name) < 0) {
                log_error(zlog, "Cannot unlink `%s': %s",
                          ptr->orig_name, strerror(errno));
            }
            break;
    }
    return 0;
}

/* Handle close of a (compressed file). `ptr' points to the zfile to close.  */
static int handle_close(zfile_t *ptr)
{
    ZDEBUG(("handle_close: closing `%s' (`%s'), write_mode = %d",
            ptr->tmp_name ? ptr->tmp_name : "(null)",
            ptr->orig_name, ptr->write_mode));

    if (ptr->tmp_name) {
        /* Recompress into the original file.  */
        if (ptr->orig_name
            && ptr->write_mode
            && zfile_compress(ptr->tmp_name, ptr->orig_name, ptr->type)) {
            return -1;
        }

        /* Remove temporary file.  */
        if (ioutil_remove(ptr->tmp_name) < 0) {
            log_error(zlog, "Cannot unlink `%s': %s", ptr->tmp_name, strerror(errno));
        }
    }

    handle_close_action(ptr);

    /* Remove item from list.  */
    if (ptr->prev != NULL) {
        ptr->prev->next = ptr->next;
    } else {
        zfile_list = ptr->next;
    }

    if (ptr->next != NULL) {
        ptr->next->prev = ptr->prev;
    }

    if (ptr->orig_name) {
        lib_free(ptr->orig_name);
    }
    if (ptr->tmp_name) {
        lib_free(ptr->tmp_name);
    }
    if (ptr->request_string) {
        lib_free(ptr->request_string);
    }

    lib_free(ptr);

    return 0;
}

/* `fclose()' wrapper.  */
int zfile_fclose(FILE *stream)
{
    zfile_t *ptr;

    if (!zinit_done) {
        errno = EBADF;
        return -1;
    }

    /* Search for the matching file in the list.  */
    for (ptr = zfile_list; ptr != NULL; ptr = ptr->next) {
        if (ptr->stream == stream) {
            /* Close temporary file.  */
            if (fclose(stream) == -1) {
                return -1;
            }
            if (handle_close(ptr) < 0) {
                errno = EBADF;
                return -1;
            }

            return 0;
        }
    }

    return fclose(stream);
}

int zfile_close_action(const char *filename, zfile_action_t action,
                       const char *request_str)
{
    char *fullname = NULL;
    zfile_t *p = zfile_list;

    archdep_expand_path(&fullname, filename);

    while (p != NULL) {
        if (p->orig_name && !strcmp(p->orig_name, fullname)) {
            p->action = action;
            p->request_string = request_str ? lib_stralloc(request_str) : NULL;
            lib_free(fullname);
            return 0;
        }
        p = p->next;
    }

    lib_free(fullname);
    return -1;
}
