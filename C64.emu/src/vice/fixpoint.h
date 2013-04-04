/*
 * fixpoint.h - Fixed point routines.
 *
 * Written by
 *  Andreas Dehmel <dehmel@forwiss.tu-muenchen.de>
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

#ifndef VICE_FIXPOINT_H
#define VICE_FIXPOINT_H

#define INLINE_FIXPOINT_FUNCTIONS

#ifdef FIXPOINT_ARITHMETIC
#define FIXPOINT_PREC   12
typedef int vreal_t;
#define REAL_VALUE(x) ((vreal_t)((x) * (1 << FIXPOINT_PREC)))
#define REAL_MULT(x, y) fixpoint_mult(x, y)
#define REAL_TO_INT(x) ((int)((x) >> FIXPOINT_PREC))

/* sound clk specific */
typedef unsigned long soundclk_t;
#define SOUNDCLK_PREC           10
#define SOUNDCLK_CONSTANT(x)    ((soundclk_t)(x * (1 << SOUNDCLK_PREC)))
#define SOUNDCLK_MULT(a, b)     soundclk_mult(a, b)
#define SOUNDCLK_LONG(a)        ((long)((a) >> SOUNDCLK_PREC))
#define SOUNDCLK_LONG_RAW(a)    SOUNDCLK_LONG(a)

/* inline these functions or not? */
#ifdef INLINE_FIXPOINT_FUNCTIONS
#include "fixpoint.c"
#else  /* INLINE_FIXPOINT_FUNCTIONS */
extern vreal_t fixpoint_mult(vreal_t x, vreal_t y);
extern soundclk_t soundclk_mult(soundclk_t a, soundclk_t b);
#endif /* INLINE_FIXPOINT_FUNCTIONS */

#else  /* FIXPOINT_ARITHMETIC */

typedef float vreal_t;
#define REAL_VALUE(x)   (x)
#define REAL_MULT(x, y) (x * y)
#define REAL_TO_INT(x)  ((int)(x))

/* sound clk specific */
typedef double soundclk_t;
#define SOUNDCLK_CONSTANT(x)    ((soundclk_t)(x))
#define SOUNDCLK_MULT(a, b)     (a) * (b)
#define SOUNDCLK_LONG(a)        ((long)BIG_FLOAT_TO_INT(a))
#define SOUNDCLK_LONG_RAW(a)    ((long)(a))

#endif /* FIXPOINT_ARITHMETIC */

/* Some platforms (e.g. RISC OS) throw floating exceptions when attempting
   a float-to-int cast where the floating point value is outside the range of
   an int. Use these macros when casting potentially large FP-values. */

#define BIG_FLOAT_TO_INT(f)     (f)
#define BIG_FLOAT_TO_UINT(f)    (f)

#endif
