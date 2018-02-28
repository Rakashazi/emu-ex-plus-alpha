
/*! \file vice.h
 *
 *  \brief Main header file for VICE.
 *
 *  \author Ettore Perazzoli <ettore@comm2000.it>
 *  \author Jouko Valta <jopi@stekt.oulu.fi>
 *  \author Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_VICE_H
#define VICE_VICE_H

/* We use <config.h> instead of "config.h" so that a compilation using
   -I. -I$srcdir will use ./config.h rather than $srcdir/config.h
   (which it would do because vice.h was found in $srcdir).  Well,
   actually automake barfs if the source directory was already
   configured, so this should not be an issue anymore.  */

#include <vice-config.h>

/* ------------------------------------------------------------------------- */

/* Portability... */

#if defined(__hpux) || defined(__IBMC__)
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif
#ifndef _INCLUDE_POSIX_SOURCE
#define _INCLUDE_POSIX_SOURCE
#endif
#endif  /* __hpux */

/* currently tested/testing for the following cpu types:
 *
 * cpu        4*u_char fetch   1*u_int32 fetch   define(s)
 * -----      --------------   ---------------   ---------
 * alpha          faster           slower        __alpha__
 * arm (gp2x)     slower (*)       faster (*)    GP2X
 * ppc            slower           faster        __powerpc__ || __ppc__
 * x86            slower           faster        __i386__
 * m68020+        slower           faster        __m680[2346]0__
 * x86_64         slower           faster        __x86_64__ || __amd64__
 *
 * arm           untested         untested       __arm__ && !GP2X
 * bfin          untested         untested       BFIN
 * hppa          untested         untested       ???
 * ia64          untested         untested       __ia64__
 * m88k          untested         untested       ???
 * mips          untested         untested       __mips__
 * s390          untested         untested       __s390__
 * s390x         untested         untested       __s390x__
 * sparc         untested         untested       sparc
 * sparc64       untested         untested       ???
 * vax           untested         untested       __vax__
 */

/* Allow unaligned access for i386+ based platforms */
#ifdef __i386__
#define ALLOW_UNALIGNED_ACCESS
#endif

/* Allow unaligned access for amd64/x86_64 based platforms */
#if defined(__x86_64__) || defined(__amd64__)
#define ALLOW_UNALIGNED_ACCESS
#endif

/* Allow unaligned access for m68020+ based platforms */
#if defined(__m68020__) || defined(__m68030__) || defined(__m68040__) || defined(__m68060__)
#define ALLOW_UNALIGNED_ACCESS
#endif

/* Allow unaligned access for PPC based platforms */
#if defined(__powerpc__) || defined(__ppc__)
#define ALLOW_UNALIGNED_ACCESS
#endif

/* Allow unaligned access for ARMv6 and newer, or ARMv7 and newer when on Android */
#if __ARM_ARCH >= 6 && (!defined __ANDROID__ || __ARM_ARCH >= 7)
#define ALLOW_UNALIGNED_ACCESS
#endif

/* Allow unaligned access for AArch64 */
#if defined(__aarch64__)
#define ALLOW_UNALIGNED_ACCESS
#endif

/* SunOS 4.x specific stuff */
#if defined(sun) || defined(__sun)
#  if !defined(__SVR4) && !defined(__svr4__)
#    include <unistd.h>
typedef int ssize_t;
#  endif
#endif

/* ------------------------------------------------------------------------- */
/* Which OS is using the common keyboard routines?  */
#if !defined(EMUFRAMEWORK_BUILD) && (!defined(__OS2__) || defined(USE_SDLUI) || defined(USE_SDLUI2))
#define COMMON_KBD
#endif

/* Which OS is using those ugly scale source coordinates.  */
#if defined(__MSDOS__)
#define VIDEO_SCALE_SOURCE
#endif

/* ------------------------------------------------------------------------- */

/* Internationalization stuff */
#if defined(ENABLE_NLS) && defined(HAVE_LIBINTL_H)
#    include <libintl.h>
#    define _(String) gettext (String)
#    ifdef gettext_noop
#        define N_(String) gettext_noop (String)
#    else
#        define N_(String) (String)
#    endif
#else
/* Stubs that do something close enough.  */
#    define _(String) (String)
#    define N_(String) (String)
#endif /* ENABLE_NLS */

/* T_() is just an indicator for new common text which needs
   to be added to the translate.* translation tables. */
#define T_(String) (String)

#if defined(WIN32_COMPILE) && (defined(UNICODE) || defined(_UNICODE))
/* enable WinNT Unicode support in VICE. */
#ifndef WIN32_UNICODE_SUPPORT
#define WIN32_UNICODE_SUPPORT
#endif
#endif

#ifdef WIN32_UNICODE_SUPPORT
/* enable WinNT Unicode API calls. */
#ifndef UNICODE
#define UNICODE
#endif

/* enable Unicode support in tchar.h. */
#ifndef _UNICODE
#define _UNICODE
#endif
#endif

#ifdef __OS2__
int yyparse (void);
#undef __GNUC__
#endif

#ifdef MINIXVMD
#undef vfork
#endif

#if (defined(__BEOS__) && defined(WORDS_BIGENDIAN)) || defined(__OS2__) || defined(__WATCOMC__)
#ifndef __cplusplus
#undef inline
#define inline
#endif
#endif

/* interix using c89 doesn't like empty files, this will work around that */
#if defined(_MSC_VER) && defined(__INTERIX)
static int noop;
#endif

#ifdef USE_GCC
#define int64_t_C(c) (c ## ll)
#define uint64_t_C(c) (c ## ull)
#endif

/* sortix does not have rs232 support */
#ifdef __sortix__
#undef HAVE_RS232DEV
#endif

#endif
