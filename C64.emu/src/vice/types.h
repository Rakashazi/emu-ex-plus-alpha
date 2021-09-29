/** \file   types.h
 * \brief   Type definitions for VICE
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_TYPES_H
#define VICE_TYPES_H

#include "vice.h"

#if defined(USE_SDLUI) || defined(USE_SDLUI2)
#  include "vice_sdl.h"
#endif

#include <stdbool.h>

#ifdef HAVE_INTTYPES_H
/* FIXME: Doxygen can't find the C99 header */
#  include <inttypes.h>
#else
#  ifdef HAVE_STDINT_H
#    include <stdint.h>
#  endif
#endif

typedef uint32_t CLOCK;

/* Maximum value of a CLOCK.  */
#undef CLOCK_MAX
#define CLOCK_MAX (~((CLOCK)0))

#ifdef _WIN64
#define vice_ptr_to_int(x) ((int)(long long)(x))
#define vice_ptr_to_uint(x) ((unsigned int)(unsigned long long)(x))
#define int_to_void_ptr(x) ((void *)(long long)(x))
#define uint_to_void_ptr(x) ((void *)(unsigned long long)(x))
#else
#define vice_ptr_to_int(x) ((int)(long)(x))
#define vice_ptr_to_uint(x) ((unsigned int)(unsigned long)(x))
#define int_to_void_ptr(x) ((void *)(long)(x))
#define uint_to_void_ptr(x) ((void *)(unsigned long)(x))
#endif

typedef uint32_t uint32align1 __attribute__((aligned(1)));
typedef uint32_t uint32align2 __attribute__((aligned(2)));

#include <viceCommonAPI.h>

#endif
