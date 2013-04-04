//  ---------------------------------------------------------------------------
//  This file is part of reSID, a MOS6581 SID emulator engine.
//  Copyright (C) 1999  Dag Lem <resid@nimrod.no>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  ---------------------------------------------------------------------------

#ifndef VICE__SIDDEFS_FP_H__
#define VICE__SIDDEFS_FP_H__

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#ifndef M_LN2
#define M_LN2   0.69314718055994530942
#endif

// Define bool, true, and false for C++ compilers that lack these keywords.
#define RESID_HAVE_BOOL @RESID_HAVE_BOOL@

#if !RESID_HAVE_BOOL
typedef int bool;
const bool true = 1;
const bool false = 0;
#endif

typedef unsigned char reg4;
typedef unsigned char reg8;
typedef unsigned short reg12;
typedef unsigned short reg16;
typedef unsigned int reg24;
typedef unsigned int reg32;

typedef int cycle_count;

enum chip_model { MOS6581FP=1, MOS8580FP };

enum sampling_method { SAMPLE_INTERPOLATE=1, SAMPLE_RESAMPLE_INTERPOLATE };

extern "C"
{
#ifndef __VERSION_CC__
extern const char* resid_version_string;
#else
const char* resid_version_string = VERSION;
#endif
}

// Inlining on/off.
#define RESID_INLINE @RESID_INLINE@

#define RESID_USE_SSE @RESID_USE_SSE@

#if @HAVE_LOGF_PROTOTYPE@
#define HAVE_LOGF_PROTOTYPE
#endif

#if @HAVE_EXPF_PROTOTYPE@
#define HAVE_EXPF_PROTOTYPE
#endif

#endif // not __SIDDEFS_H__
