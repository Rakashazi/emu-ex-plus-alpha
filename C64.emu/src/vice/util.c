/*! \file util.c \n
 *  \author Ettore Perazzoli, Andreas Boose\n
 *  \brief  Miscellaneous utility functions.
 *
 * util.c - Miscellaneous utility functions.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
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

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "archdep.h"
#include "ioutil.h"
#include "lib.h"
#include "log.h"
#include "util.h"

/* #define DBGUTIL */

#ifdef DBGUTIL
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

/* Malloc a new string whose contents concatenate the arguments until the
   first NULL pointer (max `_CONCAT_MAX_ARGS' arguments).  */
char *util_concat(const char *s, ...)
{
#define _CONCAT_MAX_ARGS 128
    const char *arg;
    char *newp, *ptr;
    int num_args;
    size_t arg_len[_CONCAT_MAX_ARGS], tot_len;
    int i;
    va_list ap;

    arg_len[0] = tot_len = strlen(s);

    va_start(ap, s);
    for (i = 1;
         i < _CONCAT_MAX_ARGS && (arg = va_arg(ap, const char *)) != NULL;
         i++) {
        arg_len[i] = strlen(arg);
        tot_len += arg_len[i];
    }
    num_args = i;
    va_end(ap);

    newp = lib_malloc(tot_len + 1);

    if (arg_len[0] > 0) {
        memcpy(newp, s, arg_len[0]);
    }
    ptr = newp + arg_len[0];

    va_start(ap, s);
    for (i = 1; i < num_args; i++) {
        memcpy(ptr, va_arg(ap, const char *), arg_len[i]);
        ptr += arg_len[i];
    }
    *ptr = '\0';
    va_end(ap);

#ifdef AMIGA_SUPPORT
    /* util_concat is often used to build complete paths, but the AmigaOS paths
     * are a little special as they should look like <device>:<directory>/<file>
     * but VICE doesn't handle this and will add a slash "/" after the colon
     * making VICE fail to open files. I know this isn't the place to fix this,
     * but I'm lazy and don't feel like changing a lot of places right now.
     *     An alternative would be to override the fopen commands to make it
     * possible to make this change as needed when opening the file.
     */

    while ((ptr = strstr(newp, ":/")) != NULL) {
        strcpy(ptr + 1, ptr + 2);
    }

#endif
    DBG(("util_concat %p - %s\n", newp, newp));
    return newp;
}

/* Add a line to a string.  */
void util_addline(char **list, const char *line)
{
    char *tmp;

    tmp = util_concat(*list, line, NULL);
    lib_free(*list);
    *list = tmp;
}

/* Add a line to a string and free the line.  */
void util_addline_free(char **list, char *line)
{
    util_addline(list, line);
    lib_free(line);
}

/* Add the first `src_size' bytes of `src' to the end of `buf', which is a
   malloc'ed block of `max_buf_size' bytes of which only the first `buf_size'
   ones are used.  If the `buf' is not large enough, realloc it.  Return a
   pointer to the new block.  */
BYTE *util_bufcat(BYTE *buf, int *buf_size, size_t *max_buf_size,
                  const BYTE *src, int src_size)
{
#define BUFCAT_GRANULARITY 0x1000
    if (*buf_size + src_size > (int)(*max_buf_size)) {
        BYTE *new_buf;

        *max_buf_size = (((*buf_size + src_size) / BUFCAT_GRANULARITY + 1)
                         * BUFCAT_GRANULARITY);
        new_buf = lib_realloc(buf, *max_buf_size);
        buf = new_buf;
    }

    memcpy(buf + *buf_size, src, src_size);
    *buf_size += src_size;
    return buf;
}

/* Remove spaces from start and end of string `s'.  The string is not
   reallocated even if it becomes smaller.  */
void util_remove_spaces(char *s)
{
    char *p;
    size_t l = strlen(s);

    for (p = s; *p == ' '; p++) {
    }

    l -= (p - s);
    memmove(s, p, l + 1);

    if (l > 0) {
        for (p = s + l - 1; l > 0 && *p == ' '; l--, p--) {
        }
        *(p + 1) = '\0';
    }
}

/* Set a new value to the dynamically allocated string *str.
   Returns `-1' if nothing has to be done.  */
int util_string_set(char **str, const char *new_value)
{
    if (*str == NULL) {
        if (new_value != NULL) {
            *str = lib_stralloc(new_value);
        }
    } else {
        if (new_value == NULL) {
            lib_free(*str);
            *str = NULL;
        } else {
            /* Skip copy if src and dest are already the same.  */
            if (strcmp(*str, new_value) == 0) {
                return -1;
            }

            *str = lib_realloc(*str, strlen(new_value) + 1);
            strcpy(*str, new_value);
        }
    }
    DBG(("util_string_set %p - %s\n", *str, *str));
    return 0;
}

int util_check_null_string(const char *string)
{
    if (string != NULL && *string != '\0') {
        return 0;
    }

    return -1;
}

int util_check_filename_access(const char *filename)
{
    FILE *file = NULL;

    file = fopen(filename, MODE_READ);
    if (file == NULL) {
        file = fopen(filename, MODE_WRITE);
        if (file == NULL) {
            return -1;
        } else {
            fclose(file);
            ioutil_remove(filename);
            return 0;
        }
    } else {
        fclose(file);
        return 0;
    }
}

/* ------------------------------------------------------------------------- */

int util_string_to_long(const char *str, const char **endptr, int base,
                        long *result)
{
    const char *sp, *ep;
    long weight, value;
    long sign;
    char last_letter = 0;       /* Initialize to make compiler happy.  */
    char c;

    if (base > 10) {
        last_letter = 'A' + base - 11;
    }

    c = toupper((int) *str);

    if (!isspace((int)c)
        && !isdigit((int)c)
        && (base <= 10 || c > last_letter || c < 'A')
        && c != '+' && c != '-') {
        return -1;
    }

    if (*str == '+') {
        sign = +1;
        str++;
    } else if (*str == '-') {
        str++;
        sign = -1;
    } else {
        sign = +1;
    }

    for (sp = str; isspace((int)*sp); sp++) {
    }

    for (ep = sp;
         (isdigit((int)*ep)
          || (base > 10
              && toupper((int)*ep) <= last_letter
              && toupper((int)*ep) >= 'A')); ep++) {
    }

    if (ep == sp) {
        return -1;
    }

    if (endptr != NULL) {
        *endptr = (char *)ep;
    }

    ep--;

    for (value = 0, weight = 1; ep >= sp; weight *= base, ep--) {
        if (base > 10 && toupper((int) *ep) >= 'A') {
            value += weight * (toupper((int)*ep) - 'A' + 10);
        } else {
            value += weight * (int)(*ep - '0');
        }
    }

    *result = value * sign;
    return 0;
}

/* Replace every occurrence of `string' in `s' with `replacement' and return
   the result as a malloc'ed string.  */
char *util_subst(const char *s, const char *string, const char *replacement)
{
    int num_occurrences;
    int total_size;
    size_t s_len = strlen(s);
    size_t string_len = strlen(string);
    size_t replacement_len = strlen(replacement);
    const char *sp;
    char *dp;
    char *result;

    /* First, count the occurrences so that we avoid re-allocating every
       time.  */
    for (num_occurrences = 0, sp = s;
         (sp = strstr(sp, string)) != NULL;
         num_occurrences++, sp += string_len) {
    }

    total_size = (int)(s_len - (string_len - replacement_len) * num_occurrences + 1);

    result = lib_malloc(total_size);

    sp = s;
    dp = result;
    do {
        char *f = strstr(sp, string);

        if (f == NULL) {
            break;
        }

        memcpy(dp, sp, f - sp);
        memcpy(dp + (f - sp), replacement, replacement_len);
        dp += (f - sp) + replacement_len;
        s_len -= (f - sp) + string_len;
        sp = f + string_len;
        num_occurrences--;
    } while (num_occurrences != 0);

    memcpy(dp, sp, s_len + 1);

    return result;
}

/* ------------------------------------------------------------------------- */

/* Return the length of an open file in bytes.  */
size_t util_file_length(FILE *fd)
{
    size_t off, filesize;

    off = ftell(fd);
    fseek(fd, 0, SEEK_END);
    filesize = ftell(fd);
    fseek(fd, (long)off, SEEK_SET);
    return filesize;
}

/* Load the first `size' bytes of file named `name' into `dest'.  Return 0 on
   success, -1 on failure.  */
int util_file_load(const char *name, BYTE *dest, size_t size,
                   unsigned int load_flag)
{
    FILE *fd;
    size_t i, length, r = 0;
    long start = 0;

    if (util_check_null_string(name)) {
        log_error(LOG_ERR, "No file name given for load_file().");
        return -1;
    }

    fd = fopen(name, MODE_READ);

    if (fd == NULL) {
        return -1;
    }

    length = util_file_length(fd);

    if ((load_flag & UTIL_FILE_LOAD_SKIP_ADDRESS) && (length & 2)) {
        start = 2;
        length -= 2;
    }

    if (length > size) {
        fclose(fd);
        return -1;
    }

    if ((load_flag & UTIL_FILE_LOAD_FILL) == 0 && length != size) {
        fclose(fd);
        return -1;
    }

    for (i = 0; i < size; i += length) {
        fseek(fd, start, SEEK_SET);
        if (i + length > size) {
            break;
        }
        r = fread((void *)&(dest[i]), length, 1, fd);
/* log_debug("READ %i bytes to offset %i result %i.",length,i,r); */
        if (r < 1) {
            break;
        }
    }

    fclose(fd);

    if (r < 1) {
        return -1;
    }

    return 0;
}

/* Write the first `size' bytes of `src' into a newly created file `name'.
   If `name' already exists, it is replaced by the new one.  Returns 0 on
   success, -1 on failure.  */
int util_file_save(const char *name, BYTE *src, int size)
{
    FILE *fd;
    size_t r;

    if (util_check_null_string(name)) {
        log_error(LOG_ERR, "No file name given for save_file().");
        return -1;
    }

    fd = fopen(name, MODE_WRITE);

    if (fd == NULL) {
        return -1;
    }

    r = fwrite((char *)src, size, 1, fd);

    fclose(fd);

    if (r < 1) {
        return -1;
    }

    return 0;
}

/* Input one line from the file descriptor `f'.  FIXME: we need something
   better, like GNU `getline()'.  */
int util_get_line(char *buf, int bufsize, FILE *f)
{
    char *r;
    size_t len;

    r = fgets(buf, bufsize, f);

    if (r == NULL) {
        return -1;
    }

    len = strlen(buf);

    if (len > 0) {
        char *p;

        /* Remove trailing newline characters.  */
        /* Remove both 0x0a and 0x0d characters, this solution makes it */
        /* work on all target platforms: Unixes, Win32, DOS, and even for MAC */
        while ((len > 0) && ((*(buf + len - 1) == 0x0d) || (*(buf + len - 1) == 0x0a))) {
            len--;
        }

        /* Remove useless spaces.  */
        while ((len > 0) && (*(buf + len - 1) == ' ')) {
            len--;
        }
        for (p = buf; *p == ' '; p++, len--) {
        }
        memmove(buf, p, len + 1);
        *(buf + len) = '\0';
    }

    return (int)len;
}

/* Split `path' into a file name and a directory component.  Unlike
   the MS-DOS `fnsplit', the directory does not have a trailing '/'.  */
void util_fname_split(const char *path, char **directory_return,
                      char **name_return)
{
    const char *p;

    if (path == NULL) {
        if (directory_return != NULL) {
            *directory_return = NULL;
        }
        if (name_return != NULL) {
            *name_return = NULL;
        }
        return;
    }

    p = strrchr(path, FSDEV_DIR_SEP_CHR);

#if (FSDEV_DIR_SEP_CHR == '\\')
    /* Both `/' and `\' are valid.  */
    {
        const char *p1;

        p1 = strrchr(path, '\\');
        if (p == NULL || p < p1) {
            p = p1;
        }
    }
#endif

    if (p == NULL) {
        if (directory_return != NULL) {
            *directory_return = NULL;
        }
        if (name_return != NULL) {
            *name_return = lib_stralloc(path);
        }
        return;
    }

    if (directory_return != NULL) {
        *directory_return = lib_malloc((size_t)(p - path + 1));
        memcpy(*directory_return, path, p - path);
        (*directory_return)[p - path] = '\0';
    }

    if (name_return != NULL) {
        *name_return = lib_stralloc(p + 1);
    }

    return;
}

/* ------------------------------------------------------------------------- */

/*! \brief Read bytes from a position in a file

 \param fd
   file descriptor as obtained by fopen().

 \param buf
   pointer where to from the file

 \param num
   number of bytes to read.

 \param offset
   the offset from start of file

 \return
   0 on success, else -1.

*/

int util_fpread(FILE *fd, void *buf, size_t num, long offset)
{
    if (fseek(fd, offset, SEEK_SET) < 0 || fread(buf, num, 1, fd) < 1) {
        return -1;
    }

    return 0;
}

/*! \brief Write bytes to a position in a file

 \param fd
   file descriptor as obtained by fopen().

 \param buf
   pointer to the data to be written to the file

 \param num
   number of bytes to write.

 \param offset
   the offset from start of file

 \return
   0 on success, else -1.

*/
int util_fpwrite(FILE *fd, const void *buf, size_t num, long offset)
{
    if (fseek(fd, offset, SEEK_SET) < 0 || fwrite(buf, num, 1, fd) < 1) {
        return -1;
    }
    return 0;
}

void util_dword_to_be_buf(BYTE *buf, DWORD data)
{
    buf[3] = (BYTE)(data & 0xff);
    buf[2] = (BYTE)((data >> 8) & 0xff);
    buf[1] = (BYTE)((data >> 16) & 0xff);
    buf[0] = (BYTE)((data >> 24) & 0xff);
}

void util_dword_to_le_buf(BYTE *buf, DWORD data)
{
    buf[0] = (BYTE)(data & 0xff);
    buf[1] = (BYTE)((data >> 8) & 0xff);
    buf[2] = (BYTE)((data >> 16) & 0xff);
    buf[3] = (BYTE)((data >> 24) & 0xff);
}

DWORD util_le_buf_to_dword(BYTE *buf)
{
    DWORD data;

    data = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);

    return data;
}

DWORD util_be_buf_to_dword(BYTE *buf)
{
    DWORD data;

    data = buf[3] | (buf[2] << 8) | (buf[1] << 16) | (buf[0] << 24);

    return data;
}

void util_int_to_be_buf4(BYTE *buf, int data)
{
    util_dword_to_be_buf(buf, (DWORD)data);
}

void util_int_to_le_buf4(BYTE *buf, int data)
{
    util_dword_to_le_buf(buf, (DWORD)data);
}

int util_le_buf4_to_int(BYTE *buf)
{
    return (int)util_le_buf_to_dword(buf);
}

int util_be_buf4_to_int(BYTE *buf)
{
    return (int)util_be_buf_to_dword(buf);
}

void util_word_to_be_buf(BYTE *buf, WORD data)
{
    buf[1] = (BYTE)(data & 0xff);
    buf[0] = (BYTE)((data >> 8) & 0xff);
}

void util_word_to_le_buf(BYTE *buf, WORD data)
{
    buf[0] = (BYTE)(data & 0xff);
    buf[1] = (BYTE)((data >> 8) & 0xff);
}

WORD util_le_buf_to_word(BYTE *buf)
{
    WORD data;

    data = buf[0] | (buf[1] << 8);

    return data;
}

WORD util_be_buf_to_word(BYTE *buf)
{
    WORD data;

    data = buf[1] | (buf[0] << 8);

    return data;
}

/* ------------------------------------------------------------------------- */

/* Check for existance of file named `name'.  */
int util_file_exists(const char *name)
{
    FILE *f;

    f = fopen(name, MODE_READ);
    if (f != NULL) {
        fclose(f);
        return 1;
    } else {
        return 0;
    }
}

/* ------------------------------------------------------------------------- */

char *util_find_next_line(const char *pos)
{
    char *p = strchr(pos, '\n');

    return (char *)(p == NULL ? pos : p + 1);
}

char *util_find_prev_line(const char *text, const char *pos)
{
    const char *p;

    if (pos - text <= 2) {
        return (char *) text;
    }

    for (p = pos - 2; p != text; p--) {
        if (*p == '\n') {
            break;
        }
    }

    if (*p == '\n') {
        p++;
    }

    return (char *)p;
}

/* ------------------------------------------------------------------------- */

/* The following are replacements for libc functions that could be missing.  */

#if !defined HAVE_MEMMOVE
void *memmove(void *target, const void *source, unsigned int length)
{
    char *tptr = (char *) target;
    const char *sptr = (const char *) source;

    if (tptr > sptr) {
        tptr += length;
        sptr += length;
        while (length--) {
            *(--tptr) = *(--sptr);
        }
    } else if (tptr < sptr) {
        while (length--) {
            *(tptr++) = *(sptr++);
        }
    }

    return target;
}
#endif /* !defined HAVE_MEMMOVE */

#if !defined HAVE_ATEXIT
static void atexit_support_func(int status, void *arg)
{
    void (*f)(void) = (void (*)(void))arg;

    f();
}

int atexit(void (*function)(void))
{
    return on_exit(atexit_support_func, (void *)function);
}
#endif /* !defined HAVE_ATEXIT */

#if !defined HAVE_STRERROR
char *strerror(int errnum)
{
    static char buffer[100];

    sprintf(buffer, "Error %d", errnum);
    return buffer;
}
#endif /* !defined HAVE_STRERROR */

/* The following `strcasecmp()' and `strncasecmp()' implementations are
   taken from:
   GLIB - Library of useful routines for C programming
   Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh
   MacDonald.
   The source is available from http://www.gtk.org/.  */

#if !defined HAVE_STRCASECMP
int strcasecmp(const char *s1, const char *s2)
{
    int c1, c2;

    if (s1 == NULL || s2 == NULL) {
        return 0;
    }

    while (*s1 && *s2) {
        /* According to A. Cox, some platforms have islower's that don't work
           right on non-uppercase.  */
        c1 = isupper ((unsigned int)*s1) ? tolower ((unsigned int)*s1) : *s1;
        c2 = isupper ((unsigned int)*s2) ? tolower ((unsigned int)*s2) : *s2;
        if (c1 != c2) {
            return (c1 - c2);
        }
        s1++; s2++;
    }

    return (((int)(unsigned char)*s1) - ((int)(unsigned char)*s2));
}
#endif

#if !defined HAVE_STRNCASECMP
int strncasecmp(const char *s1, const char *s2, size_t n)
{
    int c1, c2;

    if (s1 == NULL || s2 == NULL) {
        return 0;
    }

    while (n-- && *s1 && *s2) {
        /* According to A. Cox, some platforms have islower's that don't work
           right on non-uppercase.  */
        c1 = isupper((unsigned int)*s1) ? tolower((unsigned int)*s1) : *s1;
        c2 = isupper((unsigned int)*s2) ? tolower((unsigned int)*s2) : *s2;
        if (c1 != c2) {
            return (c1 - c2);
        }
        s1++;
        s2++;
    }

    if (n) {
        return (((int)(unsigned char)*s1) - ((int)(unsigned char)*s2));
    } else {
        return 0;
    }
}
#endif

#ifndef HAVE_STRTOK_R
char *strtok_r(char *s, const char *delim, char **last)
{
    char *spanp;
    int c, sc;
    char *tok;

    if (s == NULL && (s = *last) == NULL) {
        return (NULL);
    }

cont:
    c = *s++;
    for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
        if (c == sc) {
            goto cont;
        }
    }

    if (c == 0) {
        *last = NULL;
        return (NULL);
    }
    tok = s - 1;

    for (;;) {
        c = *s++;
        spanp = (char *)delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0) {
                    s = NULL;
                } else {
                    s[-1] = 0;
                }
                *last = s;
                return (tok);
            }
        } while (sc != 0);
    }
}
#endif

#ifndef HAVE_STRTOUL
unsigned long strtoul(const char *nptr, char **endptr, int base)
{
    const char *s = nptr;
    unsigned long acc;
    int c;
    unsigned long cutoff;
    int neg = 0;
    int any;
    int cutlim;


    do {
        c = *s++;
    } while (isspace(c));

    if (c == '-') {
        neg = 1;
        c = *s++;
    } else if (c == '+') {
        c = *s++;
    }
    if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0) {
        base = c == '0' ? 8 : 10;
    }

    cutoff = (unsigned long)ULONG_MAX / (unsigned long)base;
    cutlim = (unsigned long)ULONG_MAX % (unsigned long)base;

    for (acc = 0, any = 0;; c = *s++) {
        if (isdigit(c)) {
            c -= '0';
        } else if (isalpha(c)) {
            c -= isupper(c) ? 'A' - 10 : 'a' - 10;
        } else {
            break;
        }

        if (c >= base) {
            break;
        }
        if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim)) {
            any = -1;
        } else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }

    if (endptr != 0) {
        *endptr = (char *)(any ? s - 1 : nptr);
    }

    if (any < 0) {
        acc = ULONG_MAX;
        errno = ERANGE;
    } else if (neg) {
        return -acc;
    }

    return (acc);
}
#endif

/* Taken from SDL */
#ifndef HAVE_STRREV
char *strrev(char *string)
{
    size_t len = strlen(string);
    char *a = &string[0];
    char *b = &string[len - 1];
    char c;

    len /= 2;

    while (len--) {
        c = *a;
        *a++ = *b;
        *b-- = c;
    }
    return string;
}
#endif

/* Taken from SDL */
#ifndef HAVE_STRLWR
char *strlwr(char *string)
{
    char *bufp = string;

    while (*bufp) {
        *bufp = tolower((unsigned char)*bufp);
	++bufp;
    }
    return string;
}
#endif

/* Taken from SDL */
#ifndef HAVE_STRLCPY

#define VICE_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define VICE_MAX(x, y) (((x) > (y)) ? (x) : (y))

size_t strlcpy(char *dst, const char *src, size_t maxlen)
{
    size_t srclen = strlen(src);
    size_t len;

    if (maxlen > 0) {
        len = VICE_MIN(srclen, maxlen - 1);
        memcpy(dst, src, len);
        dst[len] = 0;
    }
    return srclen;
}
#endif

#if !defined(HAVE_LTOA) || !defined(HAVE_ULTOA)
static const char ntoa_table[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z'
};
#endif

/* Taken from SDL */
#ifndef HAVE_LTOA
char *ltoa(long value, char *string, int radix)
{
    char *bufp = string;

    if (value < 0) {
        *bufp++ = '-';
        value = -value;
    }
    if (value) {
        while (value > 0) {
            *bufp++ = ntoa_table[value % radix];
            value /= radix;
        }
    } else {
        *bufp++ = '0';
    }
    *bufp = '\0';

    if (*string == '-') {
        strrev(string + 1);
    } else {
        strrev(string);
    }

    return string;
}
#endif

/* Taken from SDL */
#ifndef HAVE_ULTOA
char *ultoa(unsigned long value, char *string, int radix)
{
    char *bufp = string;

    if (value) {
        while (value > 0) {
            *bufp++ = ntoa_table[value % radix];
            value /= radix;
        }
    } else {
        *bufp++ = '0';
    }
    *bufp = 0;

    strrev(string);

    return string;
}
#endif

/* Taken from SDL */
#ifndef HAVE_VSNPRINTF
static size_t PrintLong(char *text, long value, int radix, size_t maxlen)
{
    char num[130];
    size_t size;

    ltoa(value, num, radix);
    size = strlen(num);
    if (size >= maxlen) {
        size = maxlen - 1;
    }
    strlcpy(text, num, size + 1);

    return size;
}

static size_t PrintUnsignedLong(char *text, unsigned long value, int radix, size_t maxlen)
{
    char num[130];
    size_t size;

    ultoa(value, num, radix);
    size = strlen(num);
    if (size >= maxlen) {
        size = maxlen - 1;
    }
    strlcpy(text, num, size + 1);

    return size;
}

static size_t PrintFloat(char *text, double arg, size_t maxlen)
{
    char *textstart = text;
    const double precision = 0.00000001;
    size_t len;
    unsigned long value;
    int mult;

    if (arg) {
        if (arg < 0) {
            *text++ = '-';
            --maxlen;
            arg = -arg;
        }
        value = (unsigned long)arg;
        len = PrintUnsignedLong(text, value, 10, maxlen);
        text += len;
        maxlen -= len;
        arg -= value;
        if (arg > precision && maxlen) {
            mult = 10;
            *text++ = '.';
            while ((arg > precision) && maxlen) {
                value = (unsigned long)(arg * mult);
                len = PrintUnsignedLong(text, value, 10, maxlen);
                text += len;
                maxlen -= len;
                arg -= (double)value / mult;
                mult *= 10;
            }
        }
    } else {
        *text++ = '0';
    }
    return (text - textstart);
}

static size_t PrintString(char *text, const char *string, size_t maxlen)
{
    char *textstart = text;

    while (*string && maxlen--) {
        *text++ = *string++;
    }
    return (text - textstart);
}

int vsnprintf(char *text, size_t maxlen, const char *fmt, va_list ap)
{
    char *textstart = text;
    int done;
    size_t len;
    int do_lowercase;
    int radix;

    enum {
        DO_INT,
        DO_LONG,
        DO_LONGLONG
    } inttype;


    if (maxlen <= 0) {
        return 0;
    }
    --maxlen;
    while (*fmt && maxlen) {
        if (*fmt == '%') {
            done = 0;
            len = 0;
            do_lowercase = 0;
            radix = 10;
            inttype = DO_INT;

            ++fmt;
            while (*fmt == '.' || (*fmt >= '0' && *fmt <= '9')) {
                ++fmt;
            }
            while (!done) {
                switch (*fmt) {
                    case '%':
                        *text = '%';
                        len = 1;
                        done = 1;
                        break;
                    case 'c':
                        *text = (char)va_arg(ap, int);
                        len = 1;
                        done = 1;
                        break;
                    case 'h':
                        break;
                    case 'l':
                        if (inttype < DO_LONGLONG) {
                            ++inttype;
                        }
                        break;
                    case 'I':
                        if (strncmp(fmt, "I64", 3) == 0) {
                            fmt += 2;
                            inttype = DO_LONGLONG;
                        }
                        break;
                    case 'i':
                    case 'd':
                        switch (inttype) {
                            case DO_INT:
                                len = PrintLong(text, (long)va_arg(ap, int), radix, maxlen);
                                break;
                            case DO_LONG:
                            case DO_LONGLONG:
                                len = PrintLong(text, va_arg(ap, long), radix, maxlen);
                                break;
                        }
                        done = 1;
                        break;
                    case 'p':
                    case 'x':
                        do_lowercase = 1;
                    case 'X':
                        if (radix == 10) {
                            radix = 16;
                        }
                        if (*fmt == 'p') {
                            inttype = DO_LONG;
                        }
                    case 'o':
                        if (radix == 10) {
                            radix = 8;
                        }
                    case 'u':
                        switch (inttype) {
                            case DO_INT:
                                len = PrintUnsignedLong(text, (unsigned long)va_arg(ap, unsigned int), radix, maxlen);
                                break;
                            case DO_LONG:
                            case DO_LONGLONG:
                                len = PrintUnsignedLong(text, va_arg(ap, unsigned long), radix, maxlen);
                                break;
                        }
                        if (do_lowercase) {
                            strlwr(text);
                        }
                        done = 1;
                        break;
                    case 'f':
                        len = PrintFloat(text, va_arg(ap, double), maxlen);
                        done = 1;
                        break;
                    case 's':
                        len = PrintString(text, va_arg(ap, char*), maxlen);
                        done = 1;
                        break;
                    default:
                        done = 1;
                        break;
                }
                ++fmt;
            }
            text += len;
            maxlen -= len;
        } else {
            *text++ = *fmt++;
            --maxlen;
        }
    }
    *text = 0;

    return (text - textstart);
}
#endif

/* Taken from SDL */
#ifndef HAVE_SNPRINTF
int snprintf(char *text, size_t maxlen, const char *fmt, ...)
{
    va_list ap;
    int retval;

    va_start(ap, fmt);
    retval = vsnprintf(text, maxlen, fmt, ap);
    va_end(ap);

    return retval;
}
#endif

/* 
------------------------------------------------------------------------- */

/* util_add_extension() add the extension if not already there.
   If the extension is added `name' is realloced. */

void util_add_extension(char **name, const char *extension)
{
    size_t name_len, ext_len;

    if (extension == NULL || *name == NULL) {
        return;
    }

    name_len = strlen(*name);
    ext_len = strlen(extension);

    if (ext_len == 0) {
        return;
    }

    if ((name_len > ext_len + 1)
        && (strcasecmp(&((*name)[name_len - ext_len]), extension) == 0)) {
        return;
    }

    *name = lib_realloc(*name, name_len + ext_len + 2);
    (*name)[name_len] = FSDEV_EXT_SEP_CHR;
    memcpy(&((*name)[name_len + 1]), extension, ext_len + 1);
}

/* Like util_add_extension() but a const filename is passed.  */
char *util_add_extension_const(const char *filename, const char *extension)
{
    char *ext_filename;

    ext_filename = lib_stralloc(filename);

    util_add_extension(&ext_filename, extension);

    return ext_filename;
}

/* like util_add_extension(), but using a var[MAXPATH] type string
   without using realloc if extension is not present. */

void util_add_extension_maxpath(char *name, const char *extension, unsigned int maxpath)
{
    size_t name_len, ext_len;

    if (extension == NULL || name == NULL) {
        return;
    }

    name_len = strlen(name);
    ext_len = strlen(extension);

    if (ext_len == 0) {
        return;
    }

    if (name_len + ext_len > maxpath) {
        return;
    }

    if ((name_len > ext_len + 1)
        && (strcasecmp(&((name)[name_len - ext_len]), extension) == 0)) {
        return;
    }

    sprintf(name, "%s%c%s", name, FSDEV_EXT_SEP_CHR, extension);
}

char *util_get_extension(char *filename)
{
    char *s;

    if (filename == NULL) {
        return NULL;
    }

    s = strrchr(filename, FSDEV_EXT_SEP_CHR);
    if (s) {
        return s + 1;
    } else {
        return NULL;
    }
}

/* char to char tolower function, still uses tolower,
   but it keeps the ugly casting to avoid warnings
   out of the main sources. */
char util_tolower(char c)
{
    return (char)tolower((int)c);
}

/* char to char toupper function, still uses toupper,
   but it keeps the ugly casting to avoid warnings
   out of the main sources. */
char util_toupper(char c)
{
    return (char)toupper((int)c);
}

/* generate a list in the form "%X/%X/.../%X" */
char *util_gen_hex_address_list(int start, int stop, int step)
{
    char *temp1, *temp2;
    char *temp3 = NULL;
    int i = start;

    temp1 = lib_stralloc("");
    while (i < stop) {
        temp2 = lib_msprintf("0x%X", i);
        temp3 = util_concat(temp1, temp2, NULL);
        lib_free(temp1);
        lib_free(temp2);
        temp1 = temp3;
        if (i + step < stop) {
            temp3 = util_concat(temp1, "/", NULL);
            lib_free(temp1);
            temp1 = temp3;
        }
        i += step;
    }
    return temp3;
}
