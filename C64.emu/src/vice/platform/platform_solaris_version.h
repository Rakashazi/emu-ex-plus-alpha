/*
 * platform_solaris_version.h - Solaris version discovery.
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

#ifndef VICE_PLATFORM_SOLARIS_VERSION_H
#define VICE_PLATFORM_SOLARIS_VERSION_H

/* sys/syscall.h:
   syscall            number SOL1 SOL2 SOL3 SOL4 SOL5 SOL6 SOL7 SOL8 SOL9 SOL10 OSOL SOL11.0 SOL11.1 SOL11.2 SOL11.3
   SYS_p_online       198    ---- XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXXX XXXX XXXXXXX XXXXXXX XXXXXXX XXXXXXX
   SYS_nanosleep      199    ---- ---- XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXXX XXXX XXXXXXX XXXXXXX XXXXXXX XXXXXXX
   SYS_facl           200    ---- ---- ---- XXXX XXXX XXXX XXXX XXXX XXXX XXXXX XXXX XXXXXXX XXXXXXX XXXXXXX XXXXXXX
   SYS_lwp_alarm      212    ---- ---- ---- ---- XXXX XXXX XXXX XXXX XXXX XXXXX XXXX XXXXXXX XXXXXXX XXXXXXX XXXXXXX
   SYS_ntp_adjtime    249    ---- ---- ---- ---- ---- XXXX XXXX XXXX XXXX XXXXX XXXX XXXXXXX XXXXXXX XXXXXXX XXXXXXX
   SYS_lwp_mutex_init 252    ---- ---- ---- ---- ---- ---- XXXX XXXX XXXX XXXXX XXXX XXXXXXX XXXXXXX XXXXXXX XXXXXXX
   SYS_umount2        255    ---- ---- ---- ---- ---- ---- ---- XXXX XXXX XXXXX XXXX XXXXXXX XXXXXXX XXXXXXX XXXXXXX
*/

/* limits.h:
                   SOL8 SOL9 SOL10 OSOL SOL11.0 SOL11.1 SOL11.2 SOL11.3
   ATEXIT_MAX      XXXX ---- ----- ---- ------- ------- ------- -------
   _XOPEN_NAME_MAX ---- ---- XXXXX XXXX XXXXXXX XXXXXXX XXXXXXX XXXXXXX
*/

/* arpa/nameser.h:
             SOL10    OSOL     SOL11.0  SOL11.1  SOL11.2  SOL11.3
   __NAMESER 19991006 20090302 20090302 20090302 20090302 20090302
*/

/* fnmatch.h:
                   OSOL SOL11.0 SOL11.1 SOL11.2 SOL11.3
   FNM_LEADING_DIR ---- XXXXXXX XXXXXXX XXXXXXX XXXXXXX
*/

/* glob.h:
              SOL11.0 SOL11.1 SOL11.2 SOL11.3
   GLOB_LIMIT ------- XXXXXXX XXXXXXX XXXXXXX
*/

/* inet/ip.h:
                    SOL11.1 SOL11.2 SOL11.3
   CONN_PATH_LOOKUP ------- XXXXXXX XXXXXXX
*/

/* netinet/tcp.h:
              SOL11.2 SOL11.3
   TCPOPT_MD5 ------- XXXXXXX
*/

#include <sys/syscall.h>
#include <sys/types.h>

/* test for 'SYS_umount2' in 'sys/syscall.h'
   runs on   : 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 7, 8, 9, 10, osol, 11.0, 11.1, 11.2, 11.3
   passes on : 8, 9, 10, osol, 11.0, 11.1, 11.2, 11.3
   fails on  : 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 7
*/
#ifdef SYS_umount2
#  define SOL8_9_10_OSOL_110_111_112_113
#else
#  define SOL1_2_3_4_5_6_7
#endif

/* test for 'SYS_lwp_mutex_init' in 'sys/syscall.h'
   runs on   : 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 7
   passes on : 7
   fails on  : 2.1, 2.2, 2.3, 2.4, 2.5, 2.6
*/
#ifdef SOL1_2_3_4_5_6_7
#  ifdef SYS_lwp_mutex_init
#    define PLATFORM_OS "Solaris 7"
#  else
#    define SOL1_2_3_4_5_6
#  endif
#endif

/* test for 'SYS_ntp_adjtime' in 'sys/syscall.h'
   runs on   : 2.1, 2.2, 2.3, 2.4, 2.5, 2.6
   passes on : 2.6
   fails on  : 2.1, 2.2, 2.3, 2.4, 2.5
*/
#ifdef SOL1_2_3_4_5_6
#  ifdef SYS_ntp_adjtime
#    define PLATFORM_OS "Solaris 2.6"
#  else
#    define SOL1_2_3_4_5
#  endif
#endif

/* test for 'SYS_lwp_alarm' in 'sys/syscall.h'
   runs on   : 2.1, 2.2, 2.3, 2.4, 2.5
   passes on : 2.5
   fails on  : 2.1, 2.2, 2.3, 2.4
*/
#ifdef SOL1_2_3_4_5
#  ifdef SYS_lwp_alarm
#    define PLATFORM_OS "Solaris 2.5"
#  else
#    define SOL1_2_3_4
#  endif
#endif

/* test for 'SYS_facl' in 'sys/syscall.h'
   runs on   : 2.1, 2.2, 2.3, 2.4
   passes on : 2.4
   fails on  : 2.1, 2.2, 2.3
*/
#ifdef SOL1_2_3_4
#  ifdef SYS_facl
#    define PLATFORM_OS "Solaris 2.4"
#  else
#    define SOL1_2_3
#  endif
#endif

/* test for 'SYS_nanosleep' in 'sys/syscall.h'
   runs on   : 2.1, 2.2, 2.3
   passes on : 2.3
   fails on  : 2.1, 2.2
*/
#ifdef SOL1_2_3
#  ifdef SYS_nanosleep
#    define PLATFORM_OS "Solaris 2.3"
#  else
#    define SOL1_2
#  endif
#endif

/* test for 'SYS_p_online' in 'sys/syscall.h'
   runs on   : 2.1, 2.2
   passes on : 2.2
   fails on  : 2.1
*/
#ifdef SOL1_2
#  ifdef SYS_p_online
#    define PLATFORM_OS "Solaris 2.2"
#  else
#    define PLATFORM_OS "Solaris 2.1"
#  endif
#endif

/* test for 'ATEXIT_MAX' in 'limits.h'
   runs on   : 8, 9, 10, osol, 11.0, 11.1, 11.2, 11.3
   passes on : 8
   fails on  : 9, 10, osol, 11.0, 11.1, 11.2, 11.3
*/
#ifdef SOL8_9_10_OSOL_110_111_112_113
#  include <limits.h>
#  ifdef ATEXIT_MAX
#    define PLATFORM_OS "Solaris 8"
#  else
#    define SOL9_10_OSOL_110_111_112_113
#  endif
#endif

/* test for '_XOPEN_NAME_MAX' in 'limits.h'
   runs on   : 9, 10, osol, 11.0, 11.1, 11.2, 11.3
   passes on : 10, osol, 11.0, 11.1, 11.2, 11.3
   fails on  : 9
*/
#ifdef SOL9_10_OSOL_110_111_112_113
#  ifdef _XOPEN_NAME_MAX
#    define SOL10_OSOL_110_111_112_113
#  else
#    define PLATFORM_OS "Solaris 9"
#  endif
#endif

/* test for '__NAMESER==19991006' in 'arpa/nameser.h'
   runs on   : 10, osol, 11.0, 11.1, 11.2, 11.3
   passes on : 10
   fails on  : osol, 11.0, 11.1, 11.2, 11.3
*/
#ifdef SOL10_OSOL_110_111_112_113
#  include <arpa/nameser.h>
#  if (__NAMESER==19991006)
#    define PLATFORM_OS "Solaris 10"
#  else
#    define OSOL_110_111_112_113
#  endif
#endif

/* test for 'FNM_LEADING_DIR' in 'fnmatch.h'
   runs on   : osol, 11.0, 11.1, 11.2, 11.3
   passes on : 11.0, 11.1, 11.2, 11.3
   fails on  : osol
*/
#ifdef OSOL_110_111_112_113
#  include <fnmatch.h>
#  ifdef FNM_LEADING_DIR
#    define SOL_110_111_112_113
#  else
#    define PLATFORM_OS "OpenSolaris"
#  endif
#endif

/* test for 'GLOB_LIMIT' in 'glob.h'
   runs on   : 11.0, 11.1, 11.2, 11.3
   passes on : 11.1, 11.2, 11.3
   fails on  : 11.0
*/
#ifdef SOL_110_111_112_113
#  include <glob.h>
#  ifdef GLOB_LIMIT
#    define SOL_111_112_113
#  else
#    define PLATFORM_OS "Solaris 11.0"
#  endif
#endif

/* test for 'CONN_PATH_LOOKUP' in 'inet/ip.h'
   runs on   : 11.1, 11.2, 11.3
   passes on : 11.2, 11.3
   fails on  : 11.1
*/
#ifdef SOL_111_112_113
#  include <inet/ip.h>
#  ifdef CONN_PATH_LOOKUP
#    define SOL_112_113
#  else
#    define PLATFORM_OS "Solaris 11.1"
#  endif
#endif

/* test for 'TCPOPT_MD5' in 'netinet/tcp.h'
   runs on   : 11.2, 11.3
   passes on : 11.3
   fails on  : 11.2
*/
#ifdef SOL_112_113
#  include <netinet/tcp.h>
#  ifdef TCPOPT_MD5
#    define PLATFORM_OS "Solaris 11.3"
#  else
#    define PLATFORM_OS "Solaris 11.2"
#  endif
#endif

#ifndef PLATFORM_OS
#define PLATFORM_OS "Solaris"
#endif

#endif // VICE_PLATFORM_SOLARIS_VERSION_H
