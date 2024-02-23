/*
 * lib.h - Library function wrappers, mostly for memory alloc/free tracking.
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

#ifndef VICE_LIB_H
#define VICE_LIB_H

#include "vice.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"

#ifdef DEBUG
/* memory leak pinpointing, don't forget to enable in lib.c */
#define LIB_DEBUG_PINPOINT
#endif

void lib_init(void);

unsigned int lib_unsigned_rand(unsigned int min, unsigned int max);
float lib_float_rand(float min, float max);
double lib_double_rand_unit(void);

void lib_rand_seed(uint64_t seed);
void lib_rand_printseed(void);

char *lib_msprintf(const char *fmt, ...) VICE_ATTR_PRINTF;
char *lib_mvsprintf(const char *fmt, va_list args);

#ifdef LIB_DEBUG_PINPOINT
void *lib_malloc_pinpoint(size_t size, const char *name, unsigned int line);
void *lib_calloc_pinpoint(size_t nmemb, size_t size, const char *name, unsigned int line);
void *lib_realloc_pinpoint(void *p, size_t size, const char *name, unsigned int line);
void lib_free_pinpoint(void *p, const char *name, unsigned int line);

char *lib_strdup_pinpoint(const char *str, const char *name, unsigned int line);

#ifndef COMPILING_LIB_DOT_C

#define lib_malloc(x) lib_malloc_pinpoint(x, __FILE__, __LINE__)
#define lib_free(x) lib_free_pinpoint(x, __FILE__, __LINE__)
#define lib_calloc(x, y) lib_calloc_pinpoint(x, y, __FILE__, __LINE__)
#define lib_realloc(x, y) lib_realloc_pinpoint(x, y, __FILE__, __LINE__)
#define lib_strdup(x) lib_strdup_pinpoint(x, __FILE__, __LINE__)

#endif /* !COMPILING_LIB_DOT_C */

#else
/* !defined LIB_DEBUG_PINPOINT */

void *lib_malloc(size_t size);
void *lib_calloc(size_t nmemb, size_t size);
void *lib_realloc(void *p, size_t size);
void lib_free(void *ptr);

char *lib_strdup(const char *str);

#endif /* LIB_DEBUG_PINPOINT */

char *lib_strdup_trimmed(char *str);

void lib_debug_set_output(int state);

#endif
