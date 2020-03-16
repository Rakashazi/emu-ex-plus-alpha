/*
 * lib.c - Library functions.
 *
 * Written by
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


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "archdep.h"

#ifdef AMIGA_SUPPORT
#ifndef __USE_INLINE__
#define __USE_INLINE__
#endif
#endif

#if defined(AMIGA_SUPPORT) || defined(__VBCC__)
#include <proto/exec.h>
#ifndef AMIGA_SUPPORT
#define AMIGA_SUPPORT
#endif
#endif

#include "types.h"
#include "debug.h"

#define COMPILING_LIB_DOT_C
#include "lib.h"

#if (defined(sun) || defined(__sun)) && !(defined(__SVR4) || defined(__svr4__))
#  ifndef RAND_MAX
#    define RAND_MAX 32767
#  endif
#endif

#ifdef DEBUG
/* enable memory debugging */
# define LIB_DEBUG
/* enable pinpointing of memory leaks, don't forget to enable in lib.h */
# define LIB_DEBUG_PINPOINT
/* warn on free(NULL) */
/* #define LIB_DEBUG_WARN_FREE_NULL */
# if defined(HAVE_EXECINFO_H) && defined(HAVE_BT_SYMBOLS)
#  define LIB_DEBUG_CALLER
#  define DEBUG_BT_MAXDEPTH 16
#  include <execinfo.h>
# endif
#endif


#ifdef LIB_DEBUG
#define LIB_DEBUG_SIZE  0x10000
#define LIB_DEBUG_GUARD 0x1000
#define LIB_DEBUG_TOPMAX 50

static bool lib_debug_initialized = false;

#ifdef LIB_DEBUG_PINPOINT
static const char *lib_debug_filename[LIB_DEBUG_SIZE];
static unsigned int lib_debug_line[LIB_DEBUG_SIZE];
static const char *lib_debug_top_filename[LIB_DEBUG_TOPMAX];
static unsigned int lib_debug_top_line[LIB_DEBUG_TOPMAX];
static const char *lib_debug_pinpoint_filename;
static unsigned int lib_debug_pinpoint_line = 0;
#endif

static void *lib_debug_address[LIB_DEBUG_SIZE];
static unsigned int lib_debug_size[LIB_DEBUG_SIZE];
static unsigned int lib_debug_top_size[LIB_DEBUG_TOPMAX];
static unsigned int lib_debug_current_total = 0;
static unsigned int lib_debug_max_total = 0;
#ifdef LIB_DEBUG_CALLER
static void *lib_debug_bt_caller[LIB_DEBUG_SIZE][DEBUG_BT_MAXDEPTH];
static int lib_debug_bt_numcaller[LIB_DEBUG_SIZE];
#endif

#if LIB_DEBUG_GUARD > 0
static char *lib_debug_guard_base[LIB_DEBUG_SIZE];
static unsigned int lib_debug_guard_size[LIB_DEBUG_SIZE];
#endif

/*----------------------------------------------------------------------------*/

static void lib_debug_init(void)
{
    memset(lib_debug_address, 0, sizeof(lib_debug_address));
#ifdef LIB_DEBUG_CALLER
    memset(lib_debug_bt_caller, 0, sizeof(lib_debug_bt_caller));
    memset(lib_debug_bt_numcaller, 0, sizeof(lib_debug_bt_numcaller));
#endif
#if LIB_DEBUG_GUARD > 0
    memset(lib_debug_guard_base, 0, sizeof(lib_debug_guard_base));
#endif
#ifdef LIB_DEBUG_PINPOINT
    memset(lib_debug_line, 0, sizeof(lib_debug_line));
    memset(lib_debug_top_size, 0, sizeof(lib_debug_top_size));
#endif
    lib_debug_initialized = true;
}

static void lib_debug_add_top(const char *filename, unsigned int line, unsigned int size)
{
    unsigned int index, i;
    for (index = 0; index < LIB_DEBUG_TOPMAX; index++) {
        if (size > lib_debug_top_size[index]) {
#if 1
            for (i = (LIB_DEBUG_TOPMAX - 1); i > index; i--) {
                lib_debug_top_size[i] = lib_debug_top_size[i - 1];
                lib_debug_top_line[i] = lib_debug_top_line[i - 1];
                lib_debug_top_filename[i] = lib_debug_top_filename[i - 1];
            }
#endif
            lib_debug_top_size[index] = size;
            lib_debug_top_line[index] = line;
            lib_debug_top_filename[index] = filename;
            break;
        }
    }
}

static void lib_debug_alloc(void *ptr, size_t size, int level)
{
    unsigned int index;

    if (!lib_debug_initialized) {
        lib_debug_init();
    }

    index = 0;

    while (index < LIB_DEBUG_SIZE && lib_debug_address[index] != NULL) {
        index++;
    }

    if (index == LIB_DEBUG_SIZE) {
        printf("Error: lib_debug_alloc(): Out of debug address slots. (increase LIB_DEBUG_SIZE!)\n");
        return;
    }

#ifdef LIB_DEBUG_CALLER
    lib_debug_bt_numcaller[index] = backtrace(lib_debug_bt_caller[index], DEBUG_BT_MAXDEPTH);
#if 0
    printf("lib_debug_alloc(): Alloc address %p size %i slot %i from %p.\n",
           ptr, size, index, func);
#endif
#endif

    lib_debug_address[index] = ptr;
    lib_debug_size[index] = (unsigned int)size;
#ifdef LIB_DEBUG_PINPOINT
    lib_debug_filename[index] = lib_debug_pinpoint_filename;
    lib_debug_line[index] = lib_debug_pinpoint_line;
    lib_debug_add_top(lib_debug_pinpoint_filename, lib_debug_pinpoint_line, (unsigned int)size);
    lib_debug_pinpoint_line = 0;
#endif
    lib_debug_current_total += (unsigned int)size;
    if (lib_debug_current_total > lib_debug_max_total) {
        lib_debug_max_total = lib_debug_current_total;
    }
}

static void lib_debug_free(void *ptr, unsigned int level, bool fill)
{
    unsigned int index;

    if (ptr == NULL) {
        return;
    }

    index = 0;

    while (index < LIB_DEBUG_SIZE && lib_debug_address[index] != ptr) {
        index++;
    }

    if (index == LIB_DEBUG_SIZE) {
#if 0
        printf("lib_debug_free(): Cannot find debug address!\n");
#endif
        return;
    }

#if 0
    printf("lib_debug_free(): Free address %p size %i slot %i from %p.\n",
           ptr, lib_debug_size[index], index, func);
#endif

    if (fill) {
        memset(ptr, 0xdd, (size_t)lib_debug_size[index]);
    }

    lib_debug_address[index] = NULL;
    lib_debug_current_total -= lib_debug_size[index];
}

/*----------------------------------------------------------------------------*/

#if LIB_DEBUG_GUARD > 0
static void lib_debug_guard_add(char *ptr, unsigned int size)
{
    unsigned int index;

    if (!lib_debug_initialized) {
        lib_debug_init();
    }

    index = 0;

    /* find free slot */
    while (index < LIB_DEBUG_SIZE && lib_debug_guard_base[index] != NULL) {
        index++;
    }

    if (index == LIB_DEBUG_SIZE) {
        printf("Error: lib_debug_guard_add(): Out of debug address slots. (increase LIB_DEBUG_SIZE)\n");
        return;
    }
#if 0
    printf("ADD BASE %p SLOT %d SIZE %d\n", ptr, index, size);
#endif
    lib_debug_guard_base[index] = ptr;
    lib_debug_guard_size[index] = (unsigned int)size;

    memset(ptr, 0x55, LIB_DEBUG_GUARD);
    memset(ptr + LIB_DEBUG_GUARD + size, 0x55, LIB_DEBUG_GUARD);
}

/* called by lib_debug_libc_free, lib_debug_check (at exit) */
static int lib_debug_guard_remove(char *ptr)
{
    unsigned int index;
    unsigned int i;

    index = 0;

    /* find matching slot */
    while (index < LIB_DEBUG_SIZE && lib_debug_guard_base[index] != (ptr - LIB_DEBUG_GUARD)) {
        index++;
    }

    if (index == LIB_DEBUG_SIZE) {
#ifdef LIB_DEBUG_PINPOINT
        printf("%s:%u: ", lib_debug_pinpoint_filename, lib_debug_pinpoint_line);
#endif
        printf("Error: lib_debug_guard_remove(): Cannot find debug address %p! (make sure to use functions from lib.h, do NOT use lib_free on pointers allocated by other functions.)\n",
               (void *)(ptr - LIB_DEBUG_GUARD));
        return 0;
    }

    for (i = 0; i < LIB_DEBUG_GUARD; i++) {
        if ((uint8_t)*(ptr - LIB_DEBUG_GUARD + i) != 0x55) {
#ifdef LIB_DEBUG_PINPOINT
            printf("%s:%u: ", lib_debug_pinpoint_filename, lib_debug_pinpoint_line);
#endif
            printf("Error: Memory corruption in lower part of base %p!\n", (void *)(ptr - LIB_DEBUG_GUARD));
            break;
        }
    }
    for (i = 0; i < LIB_DEBUG_GUARD; i++) {
        if (*(ptr + lib_debug_guard_size[index] + i) != 0x55) {
#ifdef LIB_DEBUG_PINPOINT
            printf("%s:%u: ", lib_debug_pinpoint_filename, lib_debug_pinpoint_line);
#endif
            printf("Error: Memory corruption in higher part of base %p!\n", (void *)(ptr - LIB_DEBUG_GUARD));
            break;
        }
    }
#if 0
    printf("REM BASE %p SLOT %d\n", ptr - LIB_DEBUG_GUARD, index);
#endif
    lib_debug_guard_base[index] = NULL;
    return 1;
}

static unsigned int lib_debug_guard_size_get(char *ptr)
{
    unsigned int index;

    index = 0;

    while (index < LIB_DEBUG_SIZE && lib_debug_guard_base[index] != (ptr - LIB_DEBUG_GUARD)) {
        index++;
    }

    if (index == LIB_DEBUG_SIZE) {
#ifdef LIB_DEBUG_PINPOINT
        printf("%s:%u: ", lib_debug_pinpoint_filename, lib_debug_pinpoint_line);
#endif
        printf("Error: lib_debug_guard_size(): Cannot find debug address %p!\n",
               (void *)(ptr - LIB_DEBUG_GUARD));
        return 0;
    }

    return lib_debug_guard_size[index];
}
#endif

/*----------------------------------------------------------------------------*/

static void *lib_debug_libc_malloc(size_t size)
{
#if LIB_DEBUG_GUARD > 0
    char *ptr;

    ptr = (char *)malloc(size + 2 * LIB_DEBUG_GUARD);
    lib_debug_guard_add(ptr, (unsigned int)size);

    return (void *)(ptr + LIB_DEBUG_GUARD);
#else
    return malloc(size);
#endif
}

static void *lib_debug_libc_calloc(size_t nmemb, size_t size)
{
#if LIB_DEBUG_GUARD > 0
    char *ptr;

    ptr = (char *)malloc(nmemb * size + 2 * LIB_DEBUG_GUARD);
    lib_debug_guard_add(ptr, (unsigned int)(nmemb * size));

    memset(ptr + LIB_DEBUG_GUARD, 0, nmemb * size);

    return (void *)(ptr + LIB_DEBUG_GUARD);
#else
    return calloc(nmemb, size);
#endif
}

static void lib_debug_libc_free(void *ptr)
{
#if LIB_DEBUG_GUARD > 0
    if (ptr != NULL) {
        if (lib_debug_guard_remove((char *)ptr)) {
            free((char *)ptr - LIB_DEBUG_GUARD);
        } else {
            free(ptr);
        }
    }
#ifdef LIB_DEBUG_WARN_FREE_NULL
    else {
#ifdef LIB_DEBUG_PINPOINT
        printf("%s:%u: ", lib_debug_pinpoint_filename, lib_debug_pinpoint_line);
#endif
        printf("Warning: Pointer passed to lib_debug_libc_free is NULL.\n");
    }
#endif

#else
    free(ptr);
#endif
}

static void *lib_debug_libc_realloc(void *ptr, size_t size)
{
#if LIB_DEBUG_GUARD > 0
    char *new_ptr = NULL;

    if (size > 0) {
        new_ptr = lib_debug_libc_malloc(size);

        if (ptr != NULL) {
            size_t old_size;

            old_size = (size_t)lib_debug_guard_size_get((char *)ptr);

            if (size >= old_size) {
                memcpy(new_ptr, ptr, old_size);
            } else {
                memcpy(new_ptr, ptr, size);
            }
        }
    }

    if (ptr != NULL) {
        lib_debug_libc_free(ptr);
    }

    return (void *)new_ptr;
#else
    return realloc(ptr, size);
#endif
}
#endif

/*----------------------------------------------------------------------------*/

#ifdef LIB_DEBUG
static void printsize(unsigned int size)
{
    if (size > (1024 * 1024)) {
        printf("%uMiB", size / (1024 * 1024));
    } else if (size > (1024)) {
        printf("%uKiB", size / (1024));
    } else {
        printf("%uB", size);
    }
}
#endif

#ifdef LIB_DEBUG
#ifdef LIB_DEBUG_PINPOINT

#define LIB_DEBUG_LEAKLIST_MAX 0x80

static unsigned int lib_debug_leaklist_num = 0;
static const char *lib_debug_leaklist_filename[LIB_DEBUG_LEAKLIST_MAX];
static unsigned int lib_debug_leaklist_line[LIB_DEBUG_LEAKLIST_MAX];
static unsigned int lib_debug_leaklist_size[LIB_DEBUG_LEAKLIST_MAX];
static void *lib_debug_leaklist_address[LIB_DEBUG_LEAKLIST_MAX];
#ifdef LIB_DEBUG_CALLER
static void *lib_debug_leaklist_bt_caller[LIB_DEBUG_LEAKLIST_MAX][DEBUG_BT_MAXDEPTH];
static int lib_debug_leaklist_bt_numcaller[LIB_DEBUG_LEAKLIST_MAX];
#endif

static void lib_debug_leaklist_add(unsigned int index)
{
    unsigned int i;
#ifdef LIB_DEBUG_CALLER
    unsigned int j;
#endif
    for (i = 0; i < lib_debug_leaklist_num; i++) {
        if ((lib_debug_line[index] == lib_debug_leaklist_line[i]) &&
            (!strcmp(lib_debug_filename[index],lib_debug_leaklist_filename[i]))) {
            lib_debug_leaklist_size[i] += lib_debug_size[index];
            return;
        }
    }

    if (i < (LIB_DEBUG_LEAKLIST_MAX - 1)) {
        lib_debug_leaklist_num++;
        lib_debug_leaklist_line[i] = lib_debug_line[index];
        lib_debug_leaklist_filename[i] = lib_debug_filename[index];
        lib_debug_leaklist_size[i] = lib_debug_size[index];
        lib_debug_leaklist_address[i] = lib_debug_address[index];
#ifdef LIB_DEBUG_CALLER
        lib_debug_leaklist_bt_numcaller[i] = lib_debug_bt_numcaller[index];
        for (j = 0; j < DEBUG_BT_MAXDEPTH; j++) {
            lib_debug_leaklist_bt_caller[i][j] = lib_debug_bt_caller[index][j];
        }
#endif
    } else {
        printf("Error: lib_debug_leaklist_add(): Out of slots. (increase LIB_DEBUG_LEAKLIST_MAX!)\n");
    }
}
#endif
#endif

void lib_debug_check(void)
{
#ifdef LIB_DEBUG
    unsigned int index, count;
    unsigned int leakbytes;
#ifdef LIB_DEBUG_CALLER
    char **btstring;
    int btidx, spc;
#endif
    count = 0;
    leakbytes = 0;
    lib_debug_leaklist_num = 0;

    for (index = 0; index < LIB_DEBUG_SIZE; index++) {
        if (lib_debug_address[index] != NULL) {
            count++;
#ifdef LIB_DEBUG_PINPOINT
            lib_debug_leaklist_add(index);
#else
            printf("Warning: Memory block allocated here was not free'd (Memory leak with size 0x%x at %p).",
                   lib_debug_size[index], lib_debug_address[index]);
            printf("\n");
#endif
            leakbytes += lib_debug_size[index];
#if LIB_DEBUG_GUARD > 0
            lib_debug_guard_remove((char *)lib_debug_address[index]);
#endif
        }
    }

#ifdef LIB_DEBUG_PINPOINT
    printf("\n");
    for (index = 0; index < lib_debug_leaklist_num; index++) {
        printf("%s:%u: Warning: Memory block(s) allocated here was not "
                "free'd (Memory leak with size 0x%x at %p).",
               lib_debug_leaklist_filename[index], lib_debug_leaklist_line[index],
               lib_debug_leaklist_size[index], lib_debug_leaklist_address[index]);
#ifdef LIB_DEBUG_CALLER
        printf("\ncallstack:\n");
        btstring = backtrace_symbols(lib_debug_leaklist_bt_caller[index], lib_debug_leaklist_bt_numcaller[index]);
        if (btstring == NULL) {
            printf("             lookup failed\n");
        } else {
            for (btidx = 1; btidx < lib_debug_leaklist_bt_numcaller[index]; btidx++) {
                printf("             ");
                for (spc = 0; spc < btidx; spc++) {
                    printf(" ");
                }
                printf("%s\n", btstring[btidx]);
            }
        }
        free(btstring);
#endif
        printf("\n");
    }
#endif

    printf("\nTotal memory leaks: %u", count);
#ifdef LIB_DEBUG_PINPOINT
    printf(" in %u lines", lib_debug_leaklist_num);
#endif
    printf(". Total bytes leaked: 0x%x (", leakbytes);
    printsize(leakbytes);
    printf(").\n\nmax. total memory that was allocated: 0x%x bytes. (", lib_debug_max_total);
    printsize(lib_debug_max_total);
    printf(")\n");

#ifdef LIB_DEBUG_PINPOINT
    printf("\nTop %d largest allocated blocks:\n", LIB_DEBUG_TOPMAX);
    for (index = 0; index < LIB_DEBUG_TOPMAX; index++) {
        if (lib_debug_top_size[index]) {
            printf("%8x bytes (", lib_debug_top_size[index]);
            printsize(lib_debug_top_size[index]);
            printf(") allocated at %s:%u\n",
                    lib_debug_top_filename[index], lib_debug_top_line[index]);
        }
    }
#endif

#endif
}

/*----------------------------------------------------------------------------*/
/* like malloc, but abort on out of memory. */
#ifdef LIB_DEBUG_PINPOINT
static
#endif
void *lib_malloc(size_t size)
{
#ifdef LIB_DEBUG
    void *ptr = lib_debug_libc_malloc(size);
#else
    void *ptr = malloc(size);
#endif

#ifndef __OS2__
    if (ptr == NULL && size > 0) {
        fprintf(stderr, "error: lib_malloc failed\n");
        archdep_vice_exit(-1);
    }
#endif
#ifdef LIB_DEBUG
    lib_debug_alloc(ptr, size, 3);
#endif

#if 0
    /* clear/fill the block - this should only ever be used for debugging! */
    if (ptr) {
        memset(ptr, 0, size);
    }
#endif
    return ptr;
}

#ifdef AMIGA_SUPPORT
void *lib_AllocVec(unsigned long size, unsigned long attributes)
{
#ifdef LIB_DEBUG
    void *ptr;

    if (attributes & MEMF_CLEAR) {
        ptr = lib_debug_libc_calloc(1, size);
    } else {
        ptr = lib_debug_libc_malloc(size);
    }
#else
    void *ptr = AllocVec(size, attributes);
#endif

#ifndef __OS2__
    if (ptr == NULL && size > 0) {
        fprintf(stderr, "error: lib_AllocVec failed\n");
        archdep_vice_exit(-1);
    }
#endif
#ifdef LIB_DEBUG
    lib_debug_alloc(ptr, size, 1);
#endif

    return ptr;
}

void *lib_AllocMem(unsigned long size, unsigned long attributes)
{
#ifdef LIB_DEBUG
    void *ptr;

    if (attributes & MEMF_CLEAR) {
        ptr = lib_debug_libc_calloc(1, size);
    } else {
        ptr = lib_debug_libc_malloc(size);
    }
#else
    void *ptr = AllocMem(size, attributes);
#endif

#ifndef __OS2__
    if (ptr == NULL && size > 0) {
        fprintf(stderr, "error: lib_AllocMem failed\n");
        archdep_vice_exit(-1);
    }
#endif
#ifdef LIB_DEBUG
    lib_debug_alloc(ptr, size, 1);
#endif

    return ptr;
}
#endif

/* Like calloc, but abort if not enough memory is available.  */
#ifdef LIB_DEBUG_PINPOINT
static
#endif
void *lib_calloc(size_t nmemb, size_t size)
{
#ifdef LIB_DEBUG
    void *ptr = lib_debug_libc_calloc(nmemb, size);
#else
    void *ptr = calloc(nmemb, size);
#endif

#ifndef __OS2__
    if (ptr == NULL && (size * nmemb) > 0) {
        fprintf(stderr, "error: lib_calloc failed\n");
        archdep_vice_exit(-1);
    }
#endif
#ifdef LIB_DEBUG
    lib_debug_alloc(ptr, size * nmemb, 1);
#endif

    return ptr;
}

/* Like realloc, but abort if not enough memory is available.  */
#ifdef LIB_DEBUG_PINPOINT
static
#endif
void *lib_realloc(void *ptr, size_t size)
{
#ifdef LIB_DEBUG
    void *new_ptr = lib_debug_libc_realloc(ptr, size);
#else
    void *new_ptr = realloc(ptr, size);
#endif

#ifndef __OS2__
    if (new_ptr == NULL) {
        fprintf(stderr, "error: lib_realloc failed\n");
        archdep_vice_exit(-1);
    }
#endif
#ifdef LIB_DEBUG
    lib_debug_free(ptr, 1, false);
    lib_debug_alloc(new_ptr, size, 1);
#endif

    return new_ptr;
}


#ifdef LIB_DEBUG_PINPOINT
static
#endif
void lib_free(void *ptr)
{
#ifdef LIB_DEBUG
    lib_debug_free(ptr, 1, true);
#endif

#ifdef LIB_DEBUG
    lib_debug_libc_free(ptr);
#else
    free(ptr);
#endif
}

#ifdef AMIGA_SUPPORT
void lib_FreeVec(void *ptr)
{
#ifdef LIB_DEBUG
    lib_debug_free(ptr, 1, true);
    lib_debug_libc_free(ptr);
#else
    FreeVec(ptr);
#endif
}

void lib_FreeMem(void *ptr, unsigned long size)
{
#ifdef LIB_DEBUG
    lib_debug_free(ptr, 1, true);
    lib_debug_libc_free(ptr);
#else
    FreeMem(ptr, size);
#endif
}
#endif

/*----------------------------------------------------------------------------*/

/* Malloc enough space for `str'; copy `str' into it; then, return its
   address. */
#ifdef LIB_DEBUG_PINPOINT
static
#endif
char *lib_strdup(const char *str)
{
    size_t size;
    char *ptr;

    if (str == NULL) {
#ifdef LIB_DEBUG_PINPOINT
        fprintf(stderr, "%s:%u: ", lib_debug_pinpoint_filename, lib_debug_pinpoint_line);
#endif
        fprintf(stderr, "error: lib_strdup(NULL) not allowed.\n");
        archdep_vice_exit(-1);
    }

    size = strlen(str) + 1;
    ptr = lib_malloc(size);

    memcpy(ptr, str, size);
    return ptr;
}

#if defined(__CYGWIN32__) || defined(__CYGWIN__) || defined(WIN32_COMPILE)

size_t lib_tcstostr(char *str, const char *tcs, size_t len)
{
    strncpy(str, tcs, len);
    str[len - 1] = 0;
    return strlen(str);
}

size_t lib_strtotcs(char *tcs, const char *str, size_t len)
{
    strncpy(tcs, str, len);
    tcs[len - 1] = 0;
    return strlen(tcs);
}

int lib_snprintf(char *str, size_t len, const char *fmt, ...)
{
    va_list args;
    int ret;

    va_start(args, fmt);
#ifdef HAVE_VSNPRINTF
    ret = vsnprintf(str, len, fmt, args);
#else
    /* fake version which ignores len */
    ret = vsprintf(str, fmt, args);
#endif
    va_end(args);

    return ret;
}

#endif /* CYGWIN or WIN32_COMPILE */

#ifdef HAVE_WORKING_VSNPRINTF

/* taken shamelessly from printf(3) man page of the Linux Programmer's Manual */

char *lib_mvsprintf(const char *fmt, va_list args)
{
    /* Guess we need no more than 100 bytes. */
    int n, size = 100;
    char *p, *np;

    if ((p = lib_malloc (size)) == NULL) {
        return NULL;
    }

    while (1) {
        /* Try to print in the allocated space. */
        n = vsnprintf (p, size, fmt, args /* ap */);

        /* If that worked, return the string. */
        if (n > -1 && n < size) {
            return p;
        }

        /* Else try again with more space. */
        if (n > -1) {     /* glibc 2.1 and C99 */
            size = n + 1; /* precisely what is needed */
        } else {          /* glibc 2.0 */
            size *= 2;    /* twice the old size */
        }

        if ((np = lib_realloc (p, size)) == NULL) {
            lib_free(p);
            return NULL;
        } else {
            p = np;
        }
    }
}

#else

/* xmsprintf() is like sprintf() but lib_malloc's the buffer by itself.  */

#define xmvsprintf_is_digit(c) ((c) >= '0' && (c) <= '9')

static int xmvsprintf_skip_atoi(const char **s)
{
    int i = 0;

    while (xmvsprintf_is_digit(**s)) {
        i = i * 10 + *((*s)++) - '0';
    }
    return i;
}

#define ZEROPAD 1               /* pad with zero */
#define SIGN    2               /* unsigned/signed long */
#define PLUS    4               /* show plus */
#define SPACE   8               /* space if plus */
#define LEFT    16              /* left justified */
#define SPECIAL 32              /* 0x */
#define LARGE   64              /* use 'ABCDEF' instead of 'abcdef' */

static inline int xmvsprintf_do_div(long *n, unsigned int base)
{
    int res;

    res = ((unsigned long)*n) % (unsigned)base;
    *n = ((unsigned long)*n) / (unsigned)base;
    return res;
}

static size_t xmvsprintf_strnlen(const char * s, size_t count)
{
    const char *sc;

    for (sc = s; count-- && *sc != '\0'; ++sc) {
        /* nothing */
    }
    return sc - s;
}

static void xmvsprintf_add(char **buf, unsigned int *bufsize,
                           unsigned int *position, char write)
{
    if (*position == *bufsize) {
        *bufsize *= 2;
        *buf = lib_realloc(*buf, *bufsize);
    }
    (*buf)[*position] = write;
    *position += 1;
}

static void xmvsprintf_number(char **buf, unsigned int *bufsize,
                              unsigned int *position, long num, int base,
                              int size, int precision, int type)
{
    char c, sign, tmp[66];
    const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    int i;

    if (type & LARGE) {
        digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    }
    if (type & LEFT) {
        type &= ~ZEROPAD;
    }
    if (base < 2 || base > 36) {
        return;
    }
    c = (type & ZEROPAD) ? '0' : ' ';
    sign = 0;
    if (type & SIGN) {
        if (num < 0) {
            sign = '-';
            num = -num;
            size--;
        } else if (type & PLUS) {
            sign = '+';
            size--;
        } else if (type & SPACE) {
            sign = ' ';
            size--;
        }
    }
    if (type & SPECIAL) {
        if (base == 16) {
            size -= 2;
        } else if (base == 8) {
            size--;
        }
    }

    i = 0;
    if (num == 0) {
        tmp[i++] = '0';
    } else {
        while (num != 0) {
            tmp[i++] = digits[xmvsprintf_do_div(&num, base)];
        }
    }
    if (i > precision) {
        precision = i;
    }
    size -= precision;
    if (!(type & (ZEROPAD + LEFT))) {
        while (size-- > 0) {
            xmvsprintf_add(buf, bufsize, position, ' ');
        }
    }
    if (sign) {
        xmvsprintf_add(buf, bufsize, position, sign);
    }
    if (type & SPECIAL) {
        if (base == 8) {
            xmvsprintf_add(buf, bufsize, position, '0');
        } else if (base == 16) {
            xmvsprintf_add(buf, bufsize, position, '0');
            xmvsprintf_add(buf, bufsize, position, digits[33]);
        }
    }
    if (!(type & LEFT)) {
        while (size-- > 0) {
            xmvsprintf_add(buf, bufsize, position, c);
        }
    }
    while (i < precision--) {
        xmvsprintf_add(buf, bufsize, position, '0');
    }
    while (i-- > 0) {
        xmvsprintf_add(buf, bufsize, position, tmp[i]);
    }
    while (size-- > 0) {
        xmvsprintf_add(buf, bufsize, position, ' ');
    }
}

char *lib_mvsprintf(const char *fmt, va_list args)
{
    char *buf;
    unsigned int position, bufsize;

    size_t len;
    int i, base;
    unsigned long num;
    const char *s;

    int flags;        /* flags to number() */
    int field_width;  /* width of output field */
    int precision;    /* min. # of digits for integers; max
                         number of chars for from string */
    int qualifier;    /* 'h', 'l', or 'L' for integer fields */

    /* Setup the initial buffer.  */
    buf = lib_malloc(10);
    position = 0;
    bufsize = 10;

    for (; *fmt; ++fmt) {
        if (*fmt != '%') {
            xmvsprintf_add(&buf, &bufsize, &position, *fmt);
            continue;
        }

        /* process flags */
        flags = 0;
repeat:
        ++fmt;  /* this also skips first '%' */
        switch (*fmt) {
            case '-':
                flags |= LEFT;
                goto repeat;
            case '+':
                flags |= PLUS;
                goto repeat;
            case ' ':
                flags |= SPACE;
                goto repeat;
            case '#':
                flags |= SPECIAL;
                goto repeat;
            case '0':
                flags |= ZEROPAD;
                goto repeat;
        }

        /* get field width */
        field_width = -1;
        if (xmvsprintf_is_digit(*fmt)) {
            field_width = xmvsprintf_skip_atoi(&fmt);
        } else if (*fmt == '*') {
            ++fmt;
            /* it's the next argument */
            field_width = va_arg(args, int);
            if (field_width < 0) {
                field_width = -field_width;
                flags |= LEFT;
            }
        }

        /* get the precision */
        precision = -1;
        if (*fmt == '.') {
            ++fmt;
            if (xmvsprintf_is_digit(*fmt)) {
                precision = xmvsprintf_skip_atoi(&fmt);
            } else if (*fmt == '*') {
                ++fmt;
                /* it's the next argument */
                precision = va_arg(args, int);
            }
            if (precision < 0) {
                precision = 0;
            }
        }

        /* get the conversion qualifier */
        qualifier = -1;
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
            qualifier = *fmt;
            ++fmt;
        }

        /* default base */
        base = 10;

        switch (*fmt) {
            case 'c':
                if (!(flags & LEFT)) {
                    while (--field_width > 0) {
                        xmvsprintf_add(&buf, &bufsize, &position, ' ');
                    }
                }
                xmvsprintf_add(&buf, &bufsize, &position, (unsigned char) va_arg(args, int));
                while (--field_width > 0) {
                    xmvsprintf_add(&buf, &bufsize, &position, ' ');
                }
                continue;

            case 's':
                s = va_arg(args, char *);
                if (!s) {
                    s = "<NULL>";
                }

                len = xmvsprintf_strnlen(s, precision);

                if (!(flags & LEFT)) {
                    while (field_width > 0 && len < field_width--) {
                        xmvsprintf_add(&buf, &bufsize, &position, ' ');
                    }
                }
                for (i = 0; i < len; ++i) {
                    xmvsprintf_add(&buf, &bufsize, &position, *s++);
                }
                while (field_width > 0 && len < field_width--) {
                    xmvsprintf_add(&buf, &bufsize, &position, ' ');
                }
                continue;

            case 'p':
                if (field_width == -1) {
                    field_width = 2 * sizeof(void *);
                    flags |= ZEROPAD;
                }
                xmvsprintf_number(&buf, &bufsize, &position,
                                  vice_ptr_to_uint(va_arg(args, void *)), 16,
                                  field_width, precision, flags);
                continue;

            case '%':
                xmvsprintf_add(&buf, &bufsize, &position, '%');
                continue;

            /* integer number formats - set up the flags and "break" */
            case 'o':
                base = 8;
                break;
            case 'X':
                flags |= LARGE;
                /* FALLTHRU */ /* to lowercase hex */
            case 'x':
                base = 16;
                break;
            case 'd':
            case 'i':
                flags |= SIGN;
                /* FALLTHRU */ /* to unsigned dec */
            case 'u':
                break;

            default:
                xmvsprintf_add(&buf, &bufsize, &position, '%');
                if (*fmt) {
                    xmvsprintf_add(&buf, &bufsize, &position, *fmt);
                } else {
                    --fmt;
                }
                continue;
        }
        if (qualifier == 'l') {
            num = va_arg(args, unsigned long);
        } else if (qualifier == 'h') {
            num = (unsigned short) va_arg(args, int);
            if (flags & SIGN) {
                num = (short) num;
            }
        } else if (flags & SIGN) {
            num = va_arg(args, int);
        } else {
            num = va_arg(args, unsigned int);
        }
        xmvsprintf_number(&buf, &bufsize, &position, num, base, field_width,
                          precision, flags);
    }
    xmvsprintf_add(&buf, &bufsize, &position, '\0');

    /* Trim buffer to final size.  */
    buf = lib_realloc(buf, strlen(buf) + 1);

    return buf;
}
#endif /* #ifdef HAVE_WORKING_VSNPRINTF */

char *lib_msprintf(const char *fmt, ...)
{
    va_list args;
    char *buf;

    va_start(args, fmt);
    buf = lib_mvsprintf(fmt, args);
    va_end(args);

    return buf;
}

/*----------------------------------------------------------------------------*/

#ifdef LIB_DEBUG_PINPOINT
void *lib_malloc_pinpoint(size_t size, const char *name, unsigned int line)
{
    lib_debug_pinpoint_filename = name;
    lib_debug_pinpoint_line = line;
    return lib_malloc(size);
}

void lib_free_pinpoint(void *p, const char *name, unsigned int line)
{
    lib_debug_pinpoint_filename = name;
    lib_debug_pinpoint_line = line;
    lib_free(p);
}

void *lib_calloc_pinpoint(size_t nmemb, size_t size, const char *name, unsigned int line)
{
    lib_debug_pinpoint_filename = name;
    lib_debug_pinpoint_line = line;
    return lib_calloc(nmemb, size);
}

void *lib_realloc_pinpoint(void *p, size_t size, const char *name, unsigned int line)
{
    lib_debug_pinpoint_filename = name;
    lib_debug_pinpoint_line = line;
    return lib_realloc(p, size);
}

char *lib_strdup_pinpoint(const char *str, const char *name, unsigned int line)
{
    lib_debug_pinpoint_filename = name;
    lib_debug_pinpoint_line = line;
    return lib_strdup(str);
}

#ifdef AMIGA_SUPPORT
void *lib_AllocVec_pinpoint(unsigned long size, unsigned long attributes, char *name, unsigned int line)
{
    lib_debug_pinpoint_filename = name;
    lib_debug_pinpoint_line = line;
    return lib_AllocVec(size, attributes);
}

void lib_FreeVec_pinpoint(void *ptr, char *name, unsigned int line)
{
    lib_debug_pinpoint_filename = name;
    lib_debug_pinpoint_line = line;
    return lib_FreeVec(ptr);
}

void *lib_AllocMem_pinpoint(unsigned long size, unsigned long attributes, char *name, unsigned int line)
{
    lib_debug_pinpoint_filename = name;
    lib_debug_pinpoint_line = line;
    return lib_AllocMem(size, attributes);
}

void lib_FreeMem_pinpoint(void *ptr, unsigned long size, char *name, unsigned int line)
{
    lib_debug_pinpoint_filename = name;
    lib_debug_pinpoint_line = line;
    return lib_FreeMem(ptr, size);
}
#endif

/*----------------------------------------------------------------------------*/

#endif

/*
    encapsulated random routines to generate random numbers within a given range.

    see http://c-faq.com/lib/randrange.html
*/

/* set random seed for rand() from current time, so things like random startup
   delay are actually random, ie different on each startup, at all. */
void lib_init_rand(void)
{
    srand((unsigned int)time(NULL));
}

unsigned int lib_unsigned_rand(unsigned int min, unsigned int max)
{
    return min + (rand() / ((RAND_MAX / (max - min + 1)) + 1));
}

float lib_float_rand(float min, float max)
{
    return min + ((float)rand() / (((float)RAND_MAX / (max - min + 1.0f)) + 1.0f));
}
