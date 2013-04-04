/*
 * platform_compiler.h - compiler discovery macros.
 *
 * Written by
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

/* Compilers supported:
 *
 * compiler       | support
 * ------------------------------------------------------
 * clang          | yes
 * comeau c++     | yes
 * compaq/dec     | yes, but wrong version format for now
 * dignus systems | yes
 * EKOPath        | yes
 * gcc            | yes
 * green hill     | yes
 * hp uc          | yes
 * intel cc       | yes
 * llvm           | yes
 * metrowerks     | yes
 * MIPSpro        | yes
 * RealView C     | yes
 * SAS/C          | yes
 * Sun Studio     | yes
 * Tiny C         | yes
 * xLC            | yes
 */

#ifndef VICE_PLATFORM_COMPILER_H
#define VICE_PLATFORM_COMPILER_H

#undef XQUOTE
#undef QUOTE
#define QUOTE(x) XQUOTE(x)
#define XQUOTE(x) #x

/* clang discovery */
#if !defined(PLATFORM_COMPILER) && defined(__clang__)
#define PLATFORM_COMPILER "clang " QUOTE(__clang_major__) "." QUOTE(__clang_minor__) "." QUOTE(__clang_patchlevel__)
#endif

/* GCC discovery */
#if !defined(PLATFORM_COMPILER) && defined(__GNUC__)
#  if (__GNUC__>2)
#    define PLATFORM_COMPILER "GCC-" QUOTE(__GNUC__) "." QUOTE(__GNUC_MINOR__) "." QUOTE(__GNUC_PATCHLEVEL__)
#  else
#    define PLATFORM_COMPILER "GCC-" QUOTE(__GNUC__) "." QUOTE(__GNUC_MINOR__)
#  endif
#endif

/* llvm discovery */
#if !defined(PLATFORM_COMPILER) && defined(__APPLE__) && defined(llvm)
#define PLATFORM_COMPILER  "llvm"
#endif

/* xLC discovery */
#if !defined(PLATFORM_COMPILER) && defined( _AIX) && defined(__TOS_AIX__)
#define PLATFORM_COMPILER "xLC"
#endif

/* HP UPC discovery */
#if !defined(PLATFORM_COMPILER) && defined(_hpux)
#define PLATFORM_COMPILER "HP UPC"
#endif

/* Comeau compiler discovery */
#if !defined(PLATFORM_COMPILER) && defined(__COMO__)
#define PLATFORM_COMPILER "Comeau c++ " QUOTE(__COMO_VERSION__)
#define PLATFORM_COMPILER_NAME "Comeau c++"
#define PLATFORM_COMPILER_VERSION __COMO_VERSION__
#define PLATFORM_COMPILER_MAJOR_MASK 100
#define PLATFORM_COMPILER_MINOR_MASK 1
#endif

/* Intel compiler discovery */
#if !defined(PLATFORM_COMPILER) && defined(__INTEL_COMPILER)
#define PLATFORM_COMPILER "Intel Compiler " QUOTE(__INTEL_COMPILER)
#define PLATFORM_COMPILER_NAME "Intel Compiler"
#define PLATFORM_COMPILER_VERSION __INTEL_COMPILER
#define PLATFORM_COMPILER_MAJOR_MASK 100
#define PLATFORM_COMPILER_MINOR_MASK 10
#define PLATFORM_COMPILER_PATCHLEVEL_MASK 1
#endif

/* compaq/dec compiler discovery */
#if !defined(PLATFORM_COMPILER) && defined(__DECC)
#define PLATFORM_COMPILER "Compaq/DEC compiler " QUOTE(__DECC_VER)
#endif

/* Dignus Systems compiler discovery */
#if !defined(PLATFORM_COMPILER) && defined(__SYSC__)
#define PLATFORM_COMPILER "Dignus Systems compiler " QUOTE(__SYSC_VER__)
#define PLATFORM_COMPILER_NAME "Dignus Systems compiler"
#define PLATFORM_COMPILER_VERSION __SYSC_VER__
#define PLATFORM_COMPILER_MAJOR_MASK 10000
#define PLATFORM_COMPILER_MINOR_MASK 100
#define PLATFORM_COMPILER_PATCHLEVEL_MASK 1
#endif

/* EKOPath compiler discovery */
#if !defined(PLATFORM_COMPILER) && defined(__PATHCC__)
#define PLATFORM_COMPILER "EKOPath compiler " QUOTE(__PATHCC__) "." QUOTE(__PATHCC_MINOR__) "." QUOTE(__PATHCC_PATCHLEVEL__)
#endif

/* Green Hill C/C++ compiler discovery */
#if !defined(PLATFORM_COMPILER) && defined(__ghs__)
#define PLATFORM_COMPILER "Green Hill C/C++ " QUOTE(__GHS_VERSION_NUMBER__)
#define PLATFORM_COMPILER_NAME "Green Hill C/C++"
#define PLATFORM_COMPILER_VERSION __GHS_VERSION_NUMBER__
#define PLATFORM_COMPILER_MAJOR_MASK 100
#define PLATFORM_COMPILER_MINOR_MASK 10
#define PLATFORM_COMPILER_PATCHLEVEL_MASK 1
#endif

/* MetroWerks compiler discovery */
#if !defined(PLATFORM_COMPILER) && defined(__MWERKS__)
#if (__MWERKS__>1)
#define PLATFORM_COMPILER "MetroWerks " QUOTE(__MWERKS__)
#define PLATFORM_COMPILER_NAME "MetroWerks"
#define PLATFORM_COMPILER_VERSION __MWERKS__
#define PLATFORM_COMPILER_MAJOR_MASK 0x1000
#define PLATFORM_COMPILER_MINOR_MASK 0x100
#else
#define PLATFORM_COMPILER "MetroWerks"
#endif
#endif

/* MIPSpro compiler discovery */
#if !defined(PLATFORM_COMPILER) && (defined(__sgi) || defined(sgi))
#if defined(_COMPILER_VERSION) || defined(_SGI_COMPILER_VERSION)
#ifdef _COMPILER_VERSION
#define PLATFORM_COMPILER "MIPSpro " QUOTE(_COMPILER_VERSION)
#define PLATFORM_COMPILER_VERSION _COMPILER_VERSION
#else
#define PLATFORM_COMPILER "MIPSpro" QUOTE(_SGI_COMPILER_VERSION)
#define PLATFORM_COMPILER_VERSION _SGI_COMPILER_VERSION
#endif
#define PLATFORM_COMPILER_NAME "MIPSpro"
#define PLATFORM_COMPILER_MAJOR_MASK 100
#define PLATFORM_COMPILER_MINOR_MASK 10
#define PLATFORM_COMPILER_PATCHLEVEL_MASK 1
#else
#define PLATFORM_COMPILER "MIPSpro"
#endif
#endif

/* RealView C compiler discovery */
#if !defined(PLATFORM_COMPILER) && defined(__CC_ARM)
#define PLATFORM_COMPILER "RealView C " QUOTE(__ARMCC_VERSION)
#define PLATFORM_COMPILER_NAME "RealView C"
#define PLATFORM_COMPILER_VERSION __ARMCC_VERSION
#define PLATFORM_COMPILER_MAJOR_MASK 100000
#define PLATFORM_COMPILER_MINOR_MASK 10000
#define PLATFORM_COMPILER_PATCHLEVEL_MASK 1000
#endif

/* SAS/C compiler discovery */
#if !defined(PLATFORM_COMPILER) && defined(__SASC)
#ifdef __VERSION__
#define PLATFORM_COMPILER "SAS/C " QUOTE(__VERSION__) "." QUOTE(__REVISION__)
#else
#define PLATFORM_COMPILER "SASC " QUOTE(__SASC__)
#define PLATFORM_COMPILER_NAME "SAS/C"
#define PLATFORM_COMPILER_VERSION ___SASC__
#define PLATFORM_COMPILER_MAJOR_MASK 100
#define PLATFORM_COMPILER_MINOR_MASK 1
#endif
#endif

/* Sun Studio compiler discovery */
#if !defined(PLATFORM_COMPILER) && defined(__SUNPRO_C)
#define PLATFORM_COMPILER "Sun Studio Compiler " QUOTE(__SUNPRO_C)
#define PLATFORM_COMPILER_NAME "Sun Studio Compiler"
#define PLATFORM_COMPILER_VERSION __SUNPRO_C
#define PLATFORM_COMPILER_MAJOR_MASK 0x100
#define PLATFORM_COMPILER_MINOR_MASK 0x10
#define PLATFORM_COMPILER_PATCHLEVEL_MASK 1
#endif

/* Tiny C compiler discovery */
#if !defined(PLATFORM_COMPILER) && defined(__TINYC__)
#define PLATFORM_COMPILER "Tiny C"
#endif

#endif
