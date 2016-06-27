/*
 * lib.h - Library functions.
 *
 * Written by
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

extern void lib_init_rand(void);
extern unsigned int lib_unsigned_rand(unsigned int min, unsigned int max);
extern float lib_float_rand(float min, float max);

extern char *lib_msprintf(const char *fmt, ...);
extern char *lib_mvsprintf(const char *fmt, va_list args);

extern void lib_debug_check(void);

#ifdef LIB_DEBUG_PINPOINT
extern void *lib_malloc_pinpoint(size_t size, const char *name, unsigned int line);
extern void *lib_calloc_pinpoint(size_t nmemb, size_t size, const char *name, unsigned int line);
extern void *lib_realloc_pinpoint(void *p, size_t size, const char *name, unsigned int line);
extern void lib_free_pinpoint(const void *p, const char *name, unsigned int line);

extern char *lib_stralloc_pinpoint(const char *str, const char *name, unsigned int line);

#define lib_malloc(x) lib_malloc_pinpoint(x, __FILE__, __LINE__)
#define lib_free(x) lib_free_pinpoint(x, __FILE__, __LINE__)
#define lib_calloc(x, y) lib_calloc_pinpoint(x, y, __FILE__, __LINE__)
#define lib_realloc(x, y) lib_realloc_pinpoint(x, y, __FILE__, __LINE__)
#define lib_stralloc(x) lib_stralloc_pinpoint(x, __FILE__, __LINE__)

#if defined(AMIGA_SUPPORT) || defined(__VBCC__)
extern void *lib_AllocVec_pinpoint(unsigned long size, unsigned long attributes, char *name, unsigned int line);
extern void lib_FreeVec_pinpoint(void *ptr, char *name, unsigned int line);
extern void *lib_AllocMem_pinpoint(unsigned long size, unsigned long attributes, char *name, unsigned int line);
extern void lib_FreeMem_pinpoint(void *ptr, unsigned long size, char *name, unsigned int line);

#define lib_AllocVec(x, y) lib_AllocVec_pinpoint(x, y, __FILE__, __LINE__)
#define lib_FreeVec(x) lib_FreeVec_pinpoint(x, __FILE__, __LINE__)
#define lib_AllocMem(x, y) lib_AllocMem_pinpoint(x, y, __FILE__, __LINE__)
#define lib_FreeMem(x, y) lib_FreeMem_pinpoint(x, y, __FILE__, __LINE__)
#endif

#else

extern void *lib_malloc(size_t size);
extern void *lib_calloc(size_t nmemb, size_t size);
extern void *lib_realloc(void *p, size_t size);
extern void lib_free(const void *ptr);

extern char *lib_stralloc(const char *str);

#if defined(AMIGA_SUPPORT) || defined(__VBCC__)
extern void *lib_AllocVec(unsigned long size, unsigned long attributes);
extern void lib_FreeVec(void *ptr);
extern void *lib_AllocMem(unsigned long size, unsigned long attributes);
extern void lib_FreeMem(void *ptr, unsigned long size);
#endif

#endif

#endif
