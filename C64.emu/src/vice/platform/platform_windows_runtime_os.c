/*
 * platform_windows_runtime_os.c - Windows runtime version discovery.
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

/* Tested and confirmed working on:
   - OS/2 / EcomStation using odin32 (x86)
   - Wine (x86)
   - ReactOS 0.2.7 (x86)
   - ReactOS 0.2.9 (x86)
   - ReactOS 0.3.0 (x86)
   - ReactOS 0.3.3 (x86)
   - ReactOS 0.3.4 (x86)
   - ReactOS 0.3.5 (x86)
   - ReactOS 0.3.6 (x86)
   - ReactOS 0.3.7 (x86)
   - ReactOS 0.3.11 (x86)
   - ReactOS 0.3.12 (x86)
   - ReactOS 0.3.13 (x86)
   - ReactOS 0.3.14 (x86)
   - ReactOS 0.3.15 (x86)
   - ReactOS 0.3.16 (x86)
   - ReactOS 0.3.17 (x86)
   - Windows 95 Original (x86)
   - Windows 95A (x86)
   - Windows 95B (x86)
   - Windows 95C (x86)
   - Windows 98 (x86)
   - Windows 98 Secure (x86)
   - Windows 98SE (x86)
   - Windows 98SE Secure (x86)
   - Windows ME (x86)
   - Windows ME Secure (x86)
   - Windows NT 3.50 Server (x86)
   - Windows NT 3.51 Workstation (x86)
   - Windows NT 3.51 Server (x86)
   - Windows NT 4.0 Embedded Workstation (x86)
   - Windows NT 4.0 Workstation (x86)
   - Windows NT 4.0 Embedded Server (x86)
   - Windows NT 4.0 Server (x86)
   - Windows NT 4.0 Terminal Server (x86)
   - Windows NT 4.0 Small Business Server 4.0 (x86)
   - Windows NT 4.0 Small Business Server 4.5 (x86)
   - Windows NT 4.0 Enterprise Server (x86)
   - Windows Neptune (x86)
   - Windows 2000 Pro (x86)
   - Windows 2000 Powered (x86)
   - Windows 2000 Server (x86)
   - Windows 2000 Small Business Server (x86)
   - Windows 2000 Advanced Server (x86)
   - Windows 2000 Datacenter Server (x86)
   - Windows XP PE (x86)
   - Windows XP Embedded (x86)
   - Windows XP FLP (x86)
   - Windows XP Starter (x86)
   - Windows XP Home (x86)
   - Windows XP Pro (x86/x64)
   - Windows XP Tablet PC (x86)
   - Windows XP MCE 2002 (x86)
   - Windows XP MCE 2004 (x86)
   - Windows XP MCE 2005 (x86)
   - Windows XP MCE 2005 R1 (x86)
   - Windows XP MCE 2005 R2 (x86)
   - Windows 2003 Web Server (x86)
   - Windows 2003 Standard Server (x86/x64)
   - Windows 2003 Small Business Server (x86)
   - Windows 2003 Enterprise Server (x86/x64)
   - Windows 2003 Datacenter Server (x86)
   - Windows 2003 Compute Cluster Server (x64)
   - Windows Home Server (x86)
   - Windows 2003 R2 Standard Server (x86/x64)
   - Windows 2003 R2 Small Business Server (x86)
   - Windows 2003 R2 Enterprise Server (x86/x64)
   - Windows 2003 R2 Datacenter Server (x64)
   - Windows Vista Starter (x86)
   - Windows Vista Home Basic (x86/x64)
   - Windows Vista Home Premium (x86/x64)
   - Windows Vista Enterprise (x86)
   - Windows Vista Ultimate (x86)
   - Windows 2008 Foundation Server (x64)
   - Windows 2008 Web Server (x86/x64)
   - Windows 2008 Standard Server (x86/x64)
   - Windows 2008 Enterprise Server (x86/x64)
   - Windows 2008 Datacenter Server (x86/x64)
   - Windows 2008 Basic Storage Server (x64)
   - Windows 2008 Workgroup Storage Server (x64)
   - Windows 2008 Enterprise Storage Server (x64)
   - Windows Thin PC (x86)
   - Windows 7 Embedded POSReady (x86/x64)
   - Windows 7 Embedded Standard (x86)
   - Windows 7 Starter (x86)
   - Windows 7 Home Basic (x86/x64)
   - Windows 7 Home Premium (x86/x64)
   - Windows 7 Pro (x86)
   - Windows 7 Enterprise (x86/x64)
   - Windows 7 Ultimate (x86/x64)
   - Windows 2008 R2 Foundation Server (x64)
   - Windows 2008 R2 Web Server (x64)
   - Windows 2008 R2 Enterprise Server (x64)
   - Windows 2008 R2 Datacenter Server (x64)
   - Windows 2008 R2 Workgroup Storage Server (x64)
   - Windows 2008 R2 Standard Storage Server (x64)
   - Windows 2008 R2 Enterprise Storage Server (x64)
   - Windows 2009 Embedded Standard (x86)
   - Windows 2009 Embedded POSReady (x86)
   - Windows Home Server 2011 (x64)
   - Windows 2011 Standard Multipoint Server (x64)
   - Windows 2012 Standard Server (x64)
   - Windows 2012 Datacenter Server (x64)
   - Windows 8 Embedded Standard (x64)
   - Windows 8 (x86/x64)
   - Windows 8 Pro (x86/x64)
   - Windows 8 Enterprise (x86/x64)
   - Windows 8.1 Home (x64)
   - Windows 8.1 Embedded Industry Pro (x86)
   - Windows 8.1 Embedded Industry Enterprise (x86/x64)
   - Windows 8.1 Pro (x86)
   - Windows 8.1 Enterprise (x86)
   - Windows 2012 R2 Foundation Server (x64)
   - Windows 2012 R2 Standard Server (x64)
   - Windows 2012 R2 Datacenter Server (x64)
   - Windows 10 Home (x64)
   - Windows 10 Education (x86/x64)
   - Windows 10 Pro (x86/x64)
*/

#include "vice.h"

#if defined(__CYGWIN32__) || defined(__CYGWIN__) || defined(WIN32_COMPILE)

#ifndef WINMIPS

#include <windows.h>
#include <stdio.h>

#define VICE_DEBUG_H
#include "lib.h"


/* platform ids */
#ifndef VER_NT_WORKSTATION
#define VER_NT_WORKSTATION       0x00000001
#endif


#ifndef VER_NT_DOMAIN_CONTROLLER
#define VER_NT_DOMAIN_CONTROLLER 0x00000002
#endif

#ifndef VER_NT_SERVER
#define VER_NT_SERVER            0x00000003
#endif


/* platform suites */
#ifndef VER_SUITE_SMALLBUSINESS
#define VER_SUITE_SMALLBUSINESS            0x00000001
#endif

#ifndef VER_SUITE_ENTERPRISE
#define VER_SUITE_ENTERPRISE               0x00000002
#endif

#ifndef VER_SUITE_TERMINAL
#define VER_SUITE_TERMINAL                 0x00000010
#endif

#ifndef VER_SUITE_SMALLBUSINESS_RESTRICTED
#define VER_SUITE_SMALLBUSINESS_RESTRICTED 0x00000020
#endif

#ifndef VER_SUITE_EMBEDDEDNT
#define VER_SUITE_EMBEDDEDNT               0x00000040
#endif

#ifndef VER_SUITE_DATACENTER
#define VER_SUITE_DATACENTER               0x00000080
#endif

#ifndef VER_SUITE_SINGLEUSERTS
#define VER_SUITE_SINGLEUSERTS             0x00000100
#endif

#ifndef VER_SUITE_PERSONAL
#define VER_SUITE_PERSONAL                 0x00000200
#endif

#ifndef VER_SUITE_BLADE
#define VER_SUITE_BLADE                    0x00000400
#endif

#ifndef VER_SUITE_STORAGE_SERVER
#define VER_SUITE_STORAGE_SERVER           0x00002000
#endif

#ifndef VER_SUITE_COMPUTE_SERVER
#define VER_SUITE_COMPUTE_SERVER           0x00004000
#endif

#ifndef VER_SUITE_WH_SERVER
#define VER_SUITE_WH_SERVER                0x00008000
#endif


/* product ids */
#ifndef PRODUCT_UNDEFINED
#define PRODUCT_UNDEFINED                           0x00000000
#endif

#ifndef PRODUCT_ULTIMATE
#define PRODUCT_ULTIMATE                            0x00000001
#endif

#ifndef PRODUCT_HOME_BASIC
#define PRODUCT_HOME_BASIC                          0x00000002
#endif

#ifndef PRODUCT_HOME_PREMIUM
#define PRODUCT_HOME_PREMIUM                        0x00000003
#endif

#ifndef PRODUCT_ENTERPRISE
#define PRODUCT_ENTERPRISE                          0x00000004
#endif

#ifndef PRODUCT_HOME_BASIC_N
#define PRODUCT_HOME_BASIC_N                        0x00000005
#endif

#ifndef PRODUCT_BUSINESS
#define PRODUCT_BUSINESS                            0x00000006
#endif

#ifndef PRODUCT_STANDARD_SERVER
#define PRODUCT_STANDARD_SERVER                     0x00000007
#endif

#ifndef PRODUCT_DATACENTER_SERVER
#define PRODUCT_DATACENTER_SERVER                   0x00000008
#endif

#ifndef PRODUCT_SMALLBUSINESS_SERVER
#define PRODUCT_SMALLBUSINESS_SERVER                0x00000009
#endif

#ifndef PRODUCT_ENTERPRISE_SERVER
#define PRODUCT_ENTERPRISE_SERVER                   0x0000000A
#endif

#ifndef PRODUCT_STARTER
#define PRODUCT_STARTER                             0x0000000B
#endif

#ifndef PRODUCT_DATACENTER_SERVER_CORE
#define PRODUCT_DATACENTER_SERVER_CORE              0x0000000C
#endif

#ifndef PRODUCT_STANDARD_SERVER_CORE
#define PRODUCT_STANDARD_SERVER_CORE                0x0000000D
#endif

#ifndef PRODUCT_ENTERPRISE_SERVER_CORE
#define PRODUCT_ENTERPRISE_SERVER_CORE              0x0000000E
#endif

#ifndef PRODUCT_ENTERPRISE_SERVER_IA64
#define PRODUCT_ENTERPRISE_SERVER_IA64              0x0000000F
#endif

#ifndef PRODUCT_BUSINESS_N
#define PRODUCT_BUSINESS_N                          0x00000010
#endif

#ifndef PRODUCT_WEB_SERVER
#define PRODUCT_WEB_SERVER                          0x00000011
#endif

#ifndef PRODUCT_CLUSTER_SERVER
#define PRODUCT_CLUSTER_SERVER                      0x00000012
#endif

#ifndef PRODUCT_HOME_SERVER
#define PRODUCT_HOME_SERVER                         0x00000013
#endif

#ifndef PRODUCT_STORAGE_EXPRESS_SERVER
#define PRODUCT_STORAGE_EXPRESS_SERVER              0x00000014
#endif

#ifndef PRODUCT_STORAGE_STANDARD_SERVER
#define PRODUCT_STORAGE_STANDARD_SERVER             0x00000015
#endif

#ifndef PRODUCT_STORAGE_WORKGROUP_SERVER
#define PRODUCT_STORAGE_WORKGROUP_SERVER            0x00000016
#endif

#ifndef PRODUCT_STORAGE_ENTERPRISE_SERVER
#define PRODUCT_STORAGE_ENTERPRISE_SERVER           0x00000017
#endif

#ifndef PRODUCT_SERVER_FOR_SMALLBUSINESS
#define PRODUCT_SERVER_FOR_SMALLBUSINESS            0x00000018
#endif

#ifndef PRODUCT_SMALLBUSINESS_SERVER_PREMIUM
#define PRODUCT_SMALLBUSINESS_SERVER_PREMIUM        0x00000019
#endif

#ifndef PRODUCT_HOME_PREMIUM_N
#define PRODUCT_HOME_PREMIUM_N                      0x0000001A
#endif

#ifndef PRODUCT_ENTERPRISE_N
#define PRODUCT_ENTERPRISE_N                        0x0000001B
#endif

#ifndef PRODUCT_ULTIMATE_N
#define PRODUCT_ULTIMATE_N                          0x0000001C
#endif

#ifndef PRODUCT_WEB_SERVER_CORE
#define PRODUCT_WEB_SERVER_CORE                     0x0000001D
#endif

#ifndef PRODUCT_MEDIUMBUSINESS_SERVER_MANAGEMENT
#define PRODUCT_MEDIUMBUSINESS_SERVER_MANAGEMENT    0x0000001E
#endif

#ifndef PRODUCT_MEDIUMBUSINESS_SERVER_SECURITY
#define PRODUCT_MEDIUMBUSINESS_SERVER_SECURITY      0x0000001F
#endif

#ifndef PRODUCT_MEDIUMBUSINESS_SERVER_MESSAGING
#define PRODUCT_MEDIUMBUSINESS_SERVER_MESSAGING     0x00000020
#endif

#ifndef PRODUCT_SERVER_FOUNDATION
#define PRODUCT_SERVER_FOUNDATION                   0x00000021
#endif

#ifndef PRODUCT_HOME_PREMIUM_SERVER
#define PRODUCT_HOME_PREMIUM_SERVER                 0x00000022
#endif

#ifndef PRODUCT_SERVER_FOR_SMALLBUSINESS_V
#define PRODUCT_SERVER_FOR_SMALLBUSINESS_V          0x00000023
#endif

#ifndef PRODUCT_STANDARD_SERVER_V
#define PRODUCT_STANDARD_SERVER_V                   0x00000024
#endif

#ifndef PRODUCT_DATACENTER_SERVER_V
#define PRODUCT_DATACENTER_SERVER_V                 0x00000025
#endif

#ifndef PRODUCT_ENTERPRISE_SERVER_V
#define PRODUCT_ENTERPRISE_SERVER_V                 0x00000026
#endif

#ifndef PRODUCT_DATACENTER_SERVER_CORE_V
#define PRODUCT_DATACENTER_SERVER_CORE_V            0x00000027
#endif

#ifndef PRODUCT_STANDARD_SERVER_CORE_V
#define PRODUCT_STANDARD_SERVER_CORE_V              0x00000028
#endif

#ifndef PRODUCT_ENTERPRISE_SERVER_CORE_V
#define PRODUCT_ENTERPRISE_SERVER_CORE_V            0x00000029
#endif

#ifndef PRODUCT_HYPERV
#define PRODUCT_HYPERV                              0x0000002A
#endif

#ifndef PRODUCT_STORAGE_EXPRESS_SERVER_CORE
#define PRODUCT_STORAGE_EXPRESS_SERVER_CORE         0x0000002B
#endif

#ifndef PRODUCT_STORAGE_STANDARD_SERVER_CORE
#define PRODUCT_STORAGE_STANDARD_SERVER_CORE        0x0000002C
#endif

#ifndef PRODUCT_STORAGE_WORKGROUP_SERVER_CORE
#define PRODUCT_STORAGE_WORKGROUP_SERVER_CORE       0x0000002D
#endif

#ifndef PRODUCT_STORAGE_ENTERPRISE_SERVER_CORE
#define PRODUCT_STORAGE_ENTERPRISE_SERVER_CORE      0x0000002E
#endif

#ifndef PRODUCT_STARTER_N
#define PRODUCT_STARTER_N                           0x0000002F
#endif

#ifndef PRODUCT_PROFESSIONAL
#define PRODUCT_PROFESSIONAL                        0x00000030
#endif

#ifndef PRODUCT_PROFESSIONAL_N
#define PRODUCT_PROFESSIONAL_N                      0x00000031
#endif

#ifndef PRODUCT_SB_SOLUTION_SERVER
#define PRODUCT_SB_SOLUTION_SERVER                  0x00000032
#endif

#ifndef PRODUCT_SERVER_FOR_SB_SOLUTIONS
#define PRODUCT_SERVER_FOR_SB_SOLUTIONS             0x00000033
#endif

#ifndef PRODUCT_STANDARD_SERVER_SOLUTIONS
#define PRODUCT_STANDARD_SERVER_SOLUTIONS           0x00000034
#endif

#ifndef PRODUCT_STANDARD_SERVER_SOLUTIONS_CORE
#define PRODUCT_STANDARD_SERVER_SOLUTIONS_CORE      0x00000035
#endif

#ifndef PRODUCT_SB_SOLUTION_SERVER_EM
#define PRODUCT_SB_SOLUTION_SERVER_EM               0x00000036
#endif

#ifndef PRODUCT_SERVER_FOR_SB_SOLUTIONS_EM
#define PRODUCT_SERVER_FOR_SB_SOLUTIONS_EM          0x00000037
#endif

#ifndef PRODUCT_SOLUTION_EMBEDDEDSERVER
#define PRODUCT_SOLUTION_EMBEDDEDSERVER             0x00000038
#endif

#ifndef PRODUCT_SOLUTION_EMBEDDEDSERVER_CORE
#define PRODUCT_SOLUTION_EMBEDDEDSERVER_CORE        0x00000039
#endif

#ifndef PRODUCT_PROFESSIONAL_EMBEDDED
#define PRODUCT_PROFESSIONAL_EMBEDDED               0x0000003A
#endif

#ifndef PRODUCT_ESSENTIALBUSINESS_SERVER_MGMT
#define PRODUCT_ESSENTIALBUSINESS_SERVER_MGMT       0x0000003B
#endif

#ifndef PRODUCT_ESSENTIALBUSINESS_SERVER_ADDL
#define PRODUCT_ESSENTIALBUSINESS_SERVER_ADDL       0x0000003C
#endif

#ifndef PRODUCT_ESSENTIALBUSINESS_SERVER_MGMTSVC
#define PRODUCT_ESSENTIALBUSINESS_SERVER_MGMTSVC    0x0000003D
#endif

#ifndef PRODUCT_ESSENTIALBUSINESS_SERVER_ADDLSVC
#define PRODUCT_ESSENTIALBUSINESS_SERVER_ADDLSVC    0x0000003E
#endif

#ifndef PRODUCT_SMALLBUSINESS_SERVER_PREMIUM_CORE
#define PRODUCT_SMALLBUSINESS_SERVER_PREMIUM_CORE   0x0000003F
#endif

#ifndef PRODUCT_CLUSTER_SERVER_V
#define PRODUCT_CLUSTER_SERVER_V                    0x00000040
#endif

#ifndef PRODUCT_EMBEDDED
#define PRODUCT_EMBEDDED                            0x00000041
#endif

#ifndef PRODUCT_STARTER_E
#define PRODUCT_STARTER_E                           0x00000042
#endif

#ifndef PRODUCT_HOME_BASIC_E
#define PRODUCT_HOME_BASIC_E                        0x00000043
#endif

#ifndef PRODUCT_HOME_PREMIUM_E
#define PRODUCT_HOME_PREMIUM_E                      0x00000044
#endif

#ifndef PRODUCT_PROFESSIONAL_E
#define PRODUCT_PROFESSIONAL_E                      0x00000045
#endif

#ifndef PRODUCT_ENTERPRISE_E
#define PRODUCT_ENTERPRISE_E                        0x00000046
#endif

#ifndef PRODUCT_ULTIMATE_E
#define PRODUCT_ULTIMATE_E                          0x00000047
#endif

#ifndef PRODUCT_ENTERPRISE_EVALUATION
#define PRODUCT_ENTERPRISE_EVALUATION               0x00000048
#endif

#ifndef PRODUCT_MULTIPOINT_STANDARD_SERVER
#define PRODUCT_MULTIPOINT_STANDARD_SERVER          0x0000004C
#endif

#ifndef PRODUCT_MULTIPOINT_PREMIUM_SERVER
#define PRODUCT_MULTIPOINT_PREMIUM_SERVER           0x0000004D
#endif

#ifndef PRODUCT_STANDARD_EVALUATION_SERVER
#define PRODUCT_STANDARD_EVALUATION_SERVER          0x0000004F
#endif

#ifndef PRODUCT_DATACENTER_EVALUATION_SERVER
#define PRODUCT_DATACENTER_EVALUATION_SERVER        0x00000050
#endif

#ifndef PRODUCT_ENTERPRISE_N_EVALUATION
#define PRODUCT_ENTERPRISE_N_EVALUATION             0x00000054
#endif

#ifndef PRODUCT_EMBEDDED_AUTOMOTIVE
#define PRODUCT_EMBEDDED_AUTOMOTIVE                 0x00000055
#endif

#ifndef PRODUCT_EMBEDDED_INDUSTRY_A
#define PRODUCT_EMBEDDED_INDUSTRY_A                 0x00000056
#endif

#ifndef PRODUCT_THINPC
#define PRODUCT_THINPC                              0x00000057
#endif

#ifndef PRODUCT_EMBEDDED_A
#define PRODUCT_EMBEDDED_A                          0x00000058
#endif

#ifndef PRODUCT_EMBEDDED_INDUSTRY
#define PRODUCT_EMBEDDED_INDUSTRY                   0x00000059
#endif

#ifndef PRODUCT_EMBEDDED_E
#define PRODUCT_EMBEDDED_E                          0x0000005A
#endif

#ifndef PRODUCT_EMBEDDED_INDUSTRY_E
#define PRODUCT_EMBEDDED_INDUSTRY_E                 0x0000005B
#endif

#ifndef PRODUCT_EMBEDDED_INDUSTRY_A_E
#define PRODUCT_EMBEDDED_INDUSTRY_A_E               0x0000005C
#endif

#ifndef PRODUCT_STORAGE_WORKGROUP_EVALUATION_SERVER
#define PRODUCT_STORAGE_WORKGROUP_EVALUATION_SERVER 0x0000005F
#endif

#ifndef PRODUCT_STORAGE_STANDARD_EVALUATION_SERVER
#define PRODUCT_STORAGE_STANDARD_EVALUATION_SERVER  0x00000060
#endif

#ifndef PRODUCT_CORE_ARM
#define PRODUCT_CORE_ARM                            0x00000061
#endif

#ifndef PRODUCT_CORE_N
#define PRODUCT_CORE_N                              0x00000062
#endif

#ifndef PRODUCT_CORE_COUNTRYSPECIFIC
#define PRODUCT_CORE_COUNTRYSPECIFIC                0x00000063
#endif

#ifndef PRODUCT_CORE_SINGLELANGUAGE
#define PRODUCT_CORE_SINGLELANGUAGE                 0x00000064
#endif

#ifndef PRODUCT_CORE_LANGUAGESPECIFIC
#define PRODUCT_CORE_LANGUAGESPECIFIC               0x00000064
#endif

#ifndef PRODUCT_CORE
#define PRODUCT_CORE                                0x00000065
#endif

#ifndef PRODUCT_PROFESSIONAL_WMC
#define PRODUCT_PROFESSIONAL_WMC                    0x00000067
#endif

#ifndef PRODUCT_MOBILE_CORE
#define PRODUCT_MOBILE_CORE                         0x00000068
#endif

#ifndef PRODUCT_EMBEDDED_INDUSTRY_EVAL
#define PRODUCT_EMBEDDED_INDUSTRY_EVAL              0x00000069
#endif

#ifndef PRODUCT_EMBEDDED_INDUSTRY_E_EVAL
#define PRODUCT_EMBEDDED_INDUSTRY_E_EVAL            0x0000006A
#endif

#ifndef PRODUCT_EMBEDDED_EVAL
#define PRODUCT_EMBEDDED_EVAL                       0x0000006B
#endif

#ifndef PRODUCT_EMBEDDED_E_EVAL
#define PRODUCT_EMBEDDED_E_EVAL                     0x0000006C
#endif

#ifndef PRODUCT_NANO_SERVER
#define PRODUCT_NANO_SERVER                         0x0000006D
#endif

#ifndef PRODUCT_CLOUD_STORAGE_SERVER
#define PRODUCT_CLOUD_STORAGE_SERVER                0x0000006E
#endif

#ifndef PRODUCT_CORE_CONNECTED
#define PRODUCT_CORE_CONNECTED                      0x0000006F
#endif

#ifndef PRODUCT_PROFESSIONAL_STUDENT
#define PRODUCT_PROFESSIONAL_STUDENT                0x00000070
#endif

#ifndef PRODUCT_CORE_CONNECTED_N
#define PRODUCT_CORE_CONNECTED_N                    0x00000071
#endif

#ifndef PRODUCT_PROFESSIONAL_STUDENT_N
#define PRODUCT_PROFESSIONAL_STUDENT_N              0x00000072
#endif

#ifndef PRODUCT_CORE_CONNECTED_SINGLELANGUAGE
#define PRODUCT_CORE_CONNECTED_SINGLELANGUAGE       0x00000073
#endif

#ifndef PRODUCT_CORE_CONNECTED_COUNTRYSPECIFIC
#define PRODUCT_CORE_CONNECTED_COUNTRYSPECIFIC      0x00000074
#endif

#ifndef PRODUCT_CONNECTED_CAR
#define PRODUCT_CONNECTED_CAR                       0x00000075
#endif

#ifndef PRODUCT_INDUSTRY_HANDHELD
#define PRODUCT_INDUSTRY_HANDHELD                   0x00000076
#endif

#ifndef PRODUCT_PPI_PRO
#define PRODUCT_PPI_PRO                             0x00000077
#endif

#ifndef PRODUCT_ARM64_SERVER
#define PRODUCT_ARM64_SERVER                        0x00000078
#endif

#ifndef PRODUCT_EDUCATION
#define PRODUCT_EDUCATION                           0x00000079
#endif

#ifndef PRODUCT_EDUCATION_N
#define PRODUCT_EDUCATION_N                         0x0000007A
#endif

#ifndef PRODUCT_IOTUAP
#define PRODUCT_IOTUAP                              0x0000007B
#endif

#ifndef PRODUCT_CLOUD_HOST_INFRASTRUCTURE_SERVER
#define PRODUCT_CLOUD_HOST_INFRASTRUCTURE_SERVER    0x0000007C
#endif

#ifndef PRODUCT_ENTERPRISE_S
#define PRODUCT_ENTERPRISE_S                        0x0000007D
#endif

#ifndef PRODUCT_ENTERPRISE_S_N
#define PRODUCT_ENTERPRISE_S_N                      0x0000007E
#endif

#ifndef PRODUCT_PROFESSIONAL_S
#define PRODUCT_PROFESSIONAL_S                      0x0000007F
#endif

#ifndef PRODUCT_PROFESSIONAL_S_N
#define PRODUCT_PROFESSIONAL_S_N                    0x00000080
#endif

#ifndef PRODUCT_ENTERPRISE_S_EVALUATION
#define PRODUCT_ENTERPRISE_S_EVALUATION             0x00000081
#endif

#ifndef PRODUCT_ENTERPRISE_S_N_EVALUATION
#define PRODUCT_ENTERPRISE_S_N_EVALUATION           0x00000082
#endif

/* System metrics */
#ifndef SM_TABLETPC
#define SM_TABLETPC    86
#endif

#ifndef SM_MEDIACENTER
#define SM_MEDIACENTER 87
#endif

#ifndef SM_STARTER
#define SM_STARTER     88
#endif

#ifndef SM_SERVERR2
#define SM_SERVERR2    89
#endif

/* CPU types */
#ifndef PROCESSOR_ARCHITECTURE_MIPS
#define PROCESSOR_ARCHITECTURE_MIPS     1
#endif

#ifndef PROCESSOR_ARCHITECTURE_ALPHA
#define PROCESSOR_ARCHITECTURE_ALPHA    2
#endif

#ifndef PROCESSOR_ARCHITECTURE_PPC
#define PROCESSOR_ARCHITECTURE_PPC      3
#endif

#ifndef PROCESSOR_ARCHITECTURE_IA64
#define PROCESSOR_ARCHITECTURE_IA64     6
#endif

#ifndef PROCESSOR_ARCHITECTURE_ALPHA64
#define PROCESSOR_ARCHITECTURE_ALPHA64  7
#endif

#ifndef PROCESSOR_ARCHITECTURE_AMD64
#define PROCESSOR_ARCHITECTURE_AMD64    9
#endif

#ifndef VER_MINORVERSION
#define VER_MINORVERSION 0x0000001
#endif

#ifndef VER_MAJORVERSION
#define VER_MAJORVERSION 0x0000002
#endif

#ifndef VER_EQUAL
#define VER_EQUAL 1
#endif

#ifdef VER_SET_CONDITION
#undef VER_SET_CONDITION
#endif

#define VER_SET_CONDITION(ConditionMask, TypeBitMask, ComparisonType) \
	((ConditionMask) = ViceVerSetConditionMask((ConditionMask), \
	(TypeBitMask), (ComparisonType)))

/* Bit patterns for system metrics */
#define VICE_SM_SERVERR2        8
#define VICE_SM_MEDIACENTER     4
#define VICE_SM_STARTER         2
#define VICE_SM_TABLETPC        1

#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 0x0100
#endif

#ifndef KEY_WOW64_32KEY
#define KEY_WOW64_32KEY 0x0200
#endif

typedef BOOL (WINAPI *VGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);
typedef void (WINAPI *VGNSI)(LPSYSTEM_INFO);

#if (_MSC_VER >= 1300)
typedef ULONGLONG (WINAPI *VSCM)(ULONGLONG, DWORD, BYTE);
typedef BOOL (WINAPI *VVI)(LPOSVERSIONINFOEX, DWORD, DWORDLONG);
#endif

typedef struct winver_s {
    char *name;
    int platformid;
    int majorver;
    int minorver;
    int realos;
    int producttype;
    int suite;
    int pt6;
    int metrics;
} winver_t;

static winver_t windows_versions[] = {
    { NULL, 0,
      0, 0, 0, 0, 0, 0, 0 },     /* place holder for what has been detected */
    { "Windows 95", VER_PLATFORM_WIN32_WINDOWS,
      4, 0, 3, -1, -1, -1, -1 },
    { "Windows 98", VER_PLATFORM_WIN32_WINDOWS,
      4, 10, 4, -1, -1, -1, -1 },
    { "Windows ME", VER_PLATFORM_WIN32_WINDOWS,
      4, 90, 5, -1, -1, -1, -1 },
    { "Windows NT 3.10 Workstation", VER_PLATFORM_WIN32_NT,
      3, 10, 0, VER_NT_WORKSTATION, -1, -1, -1 },
    { "Windows NT 3.10 Advanced Server", VER_PLATFORM_WIN32_NT,
      3, 10, 0, VER_NT_SERVER, -1, -1, -1 },
    { "Windows NT 3.50 Workstation", VER_PLATFORM_WIN32_NT,
      3, 50, 0, VER_NT_WORKSTATION, -1, -1, -1 },
    { "Windows NT 3.50 Server", VER_PLATFORM_WIN32_NT,
      3, 50, 0, VER_NT_SERVER, -1, -1, -1 },
    { "Windows NT 3.51 Workstation", VER_PLATFORM_WIN32_NT,
      3, 51, 0, VER_NT_WORKSTATION, -1, -1, -1 },
    { "Windows NT 3.51 Server", VER_PLATFORM_WIN32_NT,
      3, 51, 0, VER_NT_SERVER, -1, -1, -1 },
    { "Windows NT 4.0 Workstation Embedded", VER_PLATFORM_WIN32_NT,
      4, 0, 1, VER_NT_WORKSTATION, VER_SUITE_EMBEDDEDNT, -1, -1 },
    { "Windows NT 4.0 Workstation", VER_PLATFORM_WIN32_NT,
      4, 0, 1, VER_NT_WORKSTATION, 0, -1, -1 },
    { "Windows NT 4.0", VER_PLATFORM_WIN32_NT,
      4, 0, 1, -1, 0, -1, -1 },
    { "Windows NT 4.0 Server Embedded", VER_PLATFORM_WIN32_NT,
      4, 0, 1, VER_NT_SERVER, VER_SUITE_EMBEDDEDNT, -1, -1 },
    { "Windows NT 4.0 Terminal Server", VER_PLATFORM_WIN32_NT,
      4, 0, 1, VER_NT_SERVER, VER_SUITE_TERMINAL, -1, -1 },
    { "Windows NT 4.0 Enterprise Server", VER_PLATFORM_WIN32_NT,
      4, 0, 1, VER_NT_SERVER, VER_SUITE_ENTERPRISE, -1, -1 },
    { "Windows NT 4.0 Small Business Server 4.0", VER_PLATFORM_WIN32_NT,
      4, 0, 1, VER_NT_SERVER, 0, -1, 1 },
    { "Windows NT 4.0 Small Business Server 4.5", VER_PLATFORM_WIN32_NT,
      4, 0, 1, VER_NT_SERVER, 0, -1, 2 },
    { "Windows NT 4.0 Server", VER_PLATFORM_WIN32_NT,
      4, 0, 1, VER_NT_SERVER, 0, -1, -1 },
    { "Windows Neptune", VER_PLATFORM_WIN32_NT,
      5, 0, 6, VER_NT_WORKSTATION, 0, -1, 1 },
    { "Windows 2000 Professional", VER_PLATFORM_WIN32_NT,
      5, 0, 6, VER_NT_WORKSTATION, 0, -1, -1 },
    { "Windows 2000 Powered", VER_PLATFORM_WIN32_NT,
      5, 0, 6, VER_NT_SERVER, VER_SUITE_BLADE, -1, -1 },
    { "Windows 2000 Datacenter Server", VER_PLATFORM_WIN32_NT,
      5, 0, 6, VER_NT_SERVER, VER_SUITE_DATACENTER, -1, -1 },
    { "Windows 2000 Advanced Server", VER_PLATFORM_WIN32_NT,
      5, 0, 6, VER_NT_SERVER, VER_SUITE_ENTERPRISE, -1, -1 },
    { "Windows 2000 Small Business Server", VER_PLATFORM_WIN32_NT,
      5, 0, 6, VER_NT_SERVER, VER_SUITE_SMALLBUSINESS, -1, -1 },
    { "Windows 2000 Server", VER_PLATFORM_WIN32_NT,
      5, 0, 6, VER_NT_SERVER, 0, -1, -1 },
    { "Windows 2000", VER_PLATFORM_WIN32_NT,
      5, 0, 6, -1, 0, -1, -1 },
    { "ODIN32 (OS/2 / EComStation)", VER_PLATFORM_WIN32_NT,
      5, 0, 0, 0, 0, -1, -1 },
    { "Windows XP Starter", VER_PLATFORM_WIN32_NT,
      5, 1, 8, VER_NT_WORKSTATION, VER_SUITE_PERSONAL | VER_SUITE_SINGLEUSERTS, -1, VICE_SM_STARTER },
    { "Windows XP Home", VER_PLATFORM_WIN32_NT,
      5, 1, 8, VER_NT_WORKSTATION, VER_SUITE_PERSONAL, -1, -1 },
    { "Windows XP Tablet PC", VER_PLATFORM_WIN32_NT,
      5, 1, 8, VER_NT_WORKSTATION, 0, -1, VICE_SM_TABLETPC },
    { "Windows XP Media Center 2002", VER_PLATFORM_WIN32_NT,
      5, 1, 8, VER_NT_WORKSTATION, 0, 0, VICE_SM_MEDIACENTER },
    { "Windows XP Media Center 2004", VER_PLATFORM_WIN32_NT,
      5, 1, 8, VER_NT_WORKSTATION, 0, 2, VICE_SM_MEDIACENTER },
    { "Windows XP Media Center 2005", VER_PLATFORM_WIN32_NT,
      5, 1, 8, VER_NT_WORKSTATION, 0, 3, VICE_SM_MEDIACENTER },
    { "Windows XP Media Center 2005 R1", VER_PLATFORM_WIN32_NT,
      5, 1, 8, VER_NT_WORKSTATION, 0, 4, VICE_SM_MEDIACENTER },
    { "Windows XP Media Center 2005 R2", VER_PLATFORM_WIN32_NT,
      5, 1, 8, VER_NT_WORKSTATION, 0, 5, VICE_SM_MEDIACENTER },
    { "Windows XP Fundamentals for Legacy PCs", VER_PLATFORM_WIN32_NT,
      5, 1, 8, VER_NT_WORKSTATION, VER_SUITE_EMBEDDEDNT | VER_SUITE_SINGLEUSERTS, -1, -1 },
    { "Windows Embedded POSReady 2009", VER_PLATFORM_WIN32_NT,
      5, 1, 8, VER_NT_WORKSTATION, VER_SUITE_EMBEDDEDNT, 2, -1 },
    { "Windows Embedded Standard 2009", VER_PLATFORM_WIN32_NT,
      5, 1, 8, VER_NT_WORKSTATION, VER_SUITE_EMBEDDEDNT, -1, 16 },
    { "Windows XP Embedded", VER_PLATFORM_WIN32_NT,
      5, 1, 8, VER_NT_WORKSTATION, VER_SUITE_EMBEDDEDNT, -1, -1 },
    { "Windows XP Professional", VER_PLATFORM_WIN32_NT,
      5, 1, 8, VER_NT_WORKSTATION, 0, -1, -1 },
    { "Windows XP", VER_PLATFORM_WIN32_NT,
      5, 1, 8, -1, 0, -1, -1 },
    { "Windows Home Server", VER_PLATFORM_WIN32_NT,
      5, 2, 9, VER_NT_SERVER, VER_SUITE_WH_SERVER, -1, -1 },
    { "Windows 2003 R2 Web Server", VER_PLATFORM_WIN32_NT,
      5, 2, 9, VER_NT_SERVER, VER_SUITE_BLADE, -1, VICE_SM_SERVERR2 },
    { "Windows 2003 Web Server", VER_PLATFORM_WIN32_NT,
      5, 2, 9, VER_NT_SERVER, VER_SUITE_BLADE, -1, -1 },
    { "Windows 2003 R2 Datacenter Server", VER_PLATFORM_WIN32_NT,
      5, 2, 9, VER_NT_SERVER, VER_SUITE_DATACENTER, -1, VICE_SM_SERVERR2 },
    { "Windows 2003 Datacenter Server", VER_PLATFORM_WIN32_NT,
      5, 2, 9, VER_NT_SERVER, VER_SUITE_DATACENTER, -1, -1 },
    { "Windows 2003 R2 Enterprise Server", VER_PLATFORM_WIN32_NT,
      5, 2, 9, VER_NT_SERVER, VER_SUITE_ENTERPRISE, -1, VICE_SM_SERVERR2 },
    { "Windows 2003 Enterprise Server", VER_PLATFORM_WIN32_NT,
      5, 2, 9, VER_NT_SERVER, VER_SUITE_ENTERPRISE, -1, -1 },
    { "Windows 2003 R2 Compute Cluster Server", VER_PLATFORM_WIN32_NT,
      5, 2, 9, VER_NT_SERVER, VER_SUITE_COMPUTE_SERVER, -1, VICE_SM_SERVERR2 },
    { "Windows 2003 Compute Cluster Server", VER_PLATFORM_WIN32_NT,
      5, 2, 9, VER_NT_SERVER, VER_SUITE_COMPUTE_SERVER, -1, -1 },
    { "Windows 2003 R2 Small Business Server", VER_PLATFORM_WIN32_NT,
      5, 2, 9, VER_NT_SERVER, VER_SUITE_SMALLBUSINESS, -1, VICE_SM_SERVERR2 },
    { "Windows 2003 Small Business Server", VER_PLATFORM_WIN32_NT,
      5, 2, 9, VER_NT_SERVER, VER_SUITE_SMALLBUSINESS, -1, -1 },
    { "Windows 2003 R2 Storage Server", VER_PLATFORM_WIN32_NT,
      5, 2, 9, VER_NT_SERVER, VER_SUITE_STORAGE_SERVER, -1, VICE_SM_SERVERR2 },
    { "Windows 2003 Storage Server", VER_PLATFORM_WIN32_NT,
      5, 2, 9, VER_NT_SERVER, VER_SUITE_STORAGE_SERVER, -1, -1 },
    { "Windows XP64", VER_PLATFORM_WIN32_NT,
      5, 2, 9, VER_NT_WORKSTATION, 0, -1, -1 },
    { "Windows 2003 R2 Standard Server", VER_PLATFORM_WIN32_NT,
      5, 2, 9, VER_NT_SERVER, 0, -1, VICE_SM_SERVERR2 },
    { "Windows 2003 Standard Server", VER_PLATFORM_WIN32_NT,
      5, 2, 9, VER_NT_SERVER, 0, -1, -1 },
    { "Windows 2003 Server", VER_PLATFORM_WIN32_NT,
      5, 2, 9, -1, 0, -1, -1 },
    { "Windows Vista Starter", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_WORKSTATION, VER_SUITE_PERSONAL, PRODUCT_STARTER, -1 },
    { "Windows Vista Home Basic", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_WORKSTATION, VER_SUITE_PERSONAL, PRODUCT_HOME_BASIC, -1 },
    { "Windows Vista Home Premium", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_WORKSTATION, VER_SUITE_PERSONAL, PRODUCT_HOME_PREMIUM, -1 },
    { "Windows Vista Business", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_WORKSTATION, 0, PRODUCT_BUSINESS, -1 },
    { "Windows Vista Enterprise", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_WORKSTATION, 0, PRODUCT_ENTERPRISE, -1 },
    { "Windows Vista Ultimate", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_WORKSTATION, 0, PRODUCT_ULTIMATE, -1 },
    { "Windows 2008 Enterprise Storage Server", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_SERVER, VER_SUITE_STORAGE_SERVER | VER_SUITE_SINGLEUSERTS | VER_SUITE_TERMINAL, -1, 3 },
    { "Windows 2008 Basic Storage Server", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_SERVER, VER_SUITE_STORAGE_SERVER | VER_SUITE_SINGLEUSERTS | VER_SUITE_TERMINAL, -1, 4 },
    { "Windows 2008 Workgroup Storage Server", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_SERVER, VER_SUITE_STORAGE_SERVER | VER_SUITE_SINGLEUSERTS | VER_SUITE_TERMINAL, -1, -1 },
    { "Windows 2008 Web Server", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_SERVER, VER_SUITE_BLADE, PRODUCT_WEB_SERVER, -1 },
    { "Windows 2008 Enterprise Server", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_SERVER, VER_SUITE_ENTERPRISE, PRODUCT_ENTERPRISE_SERVER, -1 },
    { "Windows 2008 Datacenter Server", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_SERVER, VER_SUITE_DATACENTER, PRODUCT_DATACENTER_SERVER, -1 },
    { "Windows 2008 Standard Server", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_SERVER, 0, PRODUCT_STANDARD_SERVER, -1 },
    { "Windows 2008 Small Business Server", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_SERVER, 0, PRODUCT_SMALLBUSINESS_SERVER, -1 },
    { "Windows 2008 Enterprise Server for IA64 Systems", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_SERVER, 0, PRODUCT_ENTERPRISE_SERVER_IA64, -1 },
    { "Windows 2008 HPC Server", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_SERVER, 0, PRODUCT_CLUSTER_SERVER, -1 },
    { "Windows 2008 Essential Business Server", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_SERVER, 0, PRODUCT_SERVER_FOR_SMALLBUSINESS, -1 },
    { "Windows 2008 Foundation Server", VER_PLATFORM_WIN32_NT,
      6, 0, 10, VER_NT_SERVER, 0, PRODUCT_SERVER_FOUNDATION, -1 },
    { "Windows Thin PC", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_WORKSTATION, VER_SUITE_EMBEDDEDNT, 1, -1 },
    { "Windows 7 Embedded POSReady", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_WORKSTATION, VER_SUITE_EMBEDDEDNT, 2, -1 },
    { "Windows 7 Embedded Standard", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_WORKSTATION, VER_SUITE_EMBEDDEDNT, -1, -1 },
    { "Windows 7 Starter", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_WORKSTATION, 0, PRODUCT_STARTER, -1 },
    { "Windows 7 Home Basic", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_WORKSTATION, 0, PRODUCT_HOME_BASIC, -1 },
    { "Windows 7 Home Premium", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_WORKSTATION, 0, PRODUCT_HOME_PREMIUM, -1 },
    { "Windows 7 Professional", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_WORKSTATION, 0, PRODUCT_PROFESSIONAL, -1 },
    { "Windows 7 Enterprise", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_WORKSTATION, 0, PRODUCT_ENTERPRISE, -1 },
    { "Windows 7 Business", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_WORKSTATION, 0, PRODUCT_BUSINESS, -1 },
    { "Windows 7 Ultimate", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_WORKSTATION, 0, PRODUCT_ULTIMATE, -1 },
    { "Windows 2008 R2 Web Server", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_SERVER, 0, PRODUCT_WEB_SERVER, -1 },
    { "Windows 2008 R2 Workgroup Storage Server", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_SERVER, 0, PRODUCT_STANDARD_SERVER, 1 },
    { "Windows 2008 R2 Standard Storage Server", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_SERVER, 0, PRODUCT_STANDARD_SERVER, 2 },
    { "Windows 2008 R2 Enterprise Storage Server", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_SERVER, 0, PRODUCT_ENTERPRISE_SERVER, 3 },
    { "Windows 2008 R2 Standard Server", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_SERVER, 0, PRODUCT_STANDARD_SERVER, -1 },
    { "Windows 2008 R2 Enterprise Server", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_SERVER, 0, PRODUCT_ENTERPRISE_SERVER, -1 },
    { "Windows 2008 R2 Datacenter Server", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_SERVER, 0, PRODUCT_DATACENTER_SERVER, -1 },
    { "Windows 2008 R2 Enterprise Server for IA64 Systems", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_SERVER, 0, PRODUCT_ENTERPRISE_SERVER_IA64, -1 },
    { "Windows 2008 R2 Foundation Server", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_SERVER, 0, PRODUCT_SERVER_FOUNDATION, -1 },
    { "Windows 2008 R2 HPC Server", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_SERVER, 0, PRODUCT_CLUSTER_SERVER, -1 },
    { "Windows Home Server 2011", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_SERVER, VER_SUITE_WH_SERVER, PRODUCT_HOME_PREMIUM_SERVER, -1 },
    { "Windows 2011 Multipoint Server", VER_PLATFORM_WIN32_NT,
      6, 1, 10, VER_NT_SERVER, 0, PRODUCT_SOLUTION_EMBEDDEDSERVER, -1 },
    { "Windows 8 Embedded Standard", VER_PLATFORM_WIN32_NT,
      6, 2, 10, VER_NT_WORKSTATION, VER_SUITE_EMBEDDEDNT, 4, -1 },
    { "Windows 8", VER_PLATFORM_WIN32_NT,
      6, 2, 10, VER_NT_WORKSTATION, VER_SUITE_PERSONAL | VER_SUITE_SINGLEUSERTS, -1, -1 },
    { "Windows 8 Pro", VER_PLATFORM_WIN32_NT,
      6, 2, 10, VER_NT_WORKSTATION, VER_SUITE_SINGLEUSERTS, PRODUCT_PROFESSIONAL, -1 },
    { "Windows 8 Enterprise", VER_PLATFORM_WIN32_NT,
      6, 2, 10, VER_NT_WORKSTATION, VER_SUITE_SINGLEUSERTS, -1, -1 },
    { "Windows 2012 Standard Server", VER_PLATFORM_WIN32_NT,
      6, 2, 10, VER_NT_SERVER, VER_SUITE_SINGLEUSERTS, PRODUCT_STANDARD_SERVER, -1 },
    { "Windows 2012 Datacenter Server", VER_PLATFORM_WIN32_NT,
      6, 2, 10, VER_NT_SERVER, VER_SUITE_DATACENTER, PRODUCT_DATACENTER_SERVER, -1 },
    { "Windows 2012 Server (Foundation/Essentials)", VER_PLATFORM_WIN32_NT,
      6, 2, 10, VER_NT_SERVER, 0, 0, -1 },
    { "Windows 8.1 Embedded Industry Pro", VER_PLATFORM_WIN32_NT,
      6, 3, 10, VER_NT_WORKSTATION, VER_SUITE_SINGLEUSERTS, PRODUCT_EMBEDDED_INDUSTRY, -1 },
    { "Windows 8.1 Embedded Industry Enterprise", VER_PLATFORM_WIN32_NT,
      6, 3, 10, VER_NT_WORKSTATION, VER_SUITE_SINGLEUSERTS, PRODUCT_EMBEDDED_INDUSTRY_E, -1 },
    { "Windows 8.1 Home", VER_PLATFORM_WIN32_NT,
      6, 3, 10, VER_NT_WORKSTATION, VER_SUITE_PERSONAL | VER_SUITE_SINGLEUSERTS, -1, -1 },
    { "Windows 8.1 Pro", VER_PLATFORM_WIN32_NT,
      6, 3, 10, VER_NT_WORKSTATION, VER_SUITE_SINGLEUSERTS, PRODUCT_PROFESSIONAL, -1 },
    { "Windows 8.1 Enterprise", VER_PLATFORM_WIN32_NT,
      6, 3, 10, VER_NT_WORKSTATION, VER_SUITE_SINGLEUSERTS, PRODUCT_ENTERPRISE, -1 },
    { "Windows 2012 R2 Essentials Server", VER_PLATFORM_WIN32_NT,
      6, 3, 10, VER_NT_SERVER, VER_SUITE_SMALLBUSINESS, PRODUCT_SB_SOLUTION_SERVER, 0 },
    { "Windows 2012 R2 Foundation Server", VER_PLATFORM_WIN32_NT,
      6, 3, 10, VER_NT_SERVER, VER_SUITE_SINGLEUSERTS | VER_SUITE_TERMINAL, PRODUCT_SERVER_FOUNDATION, 0 },
    { "Windows 2012 R2 Standard Server", VER_PLATFORM_WIN32_NT,
      6, 3, 10, VER_NT_SERVER, VER_SUITE_SINGLEUSERTS, PRODUCT_STANDARD_SERVER, 0 },
    { "Windows 2012 R2 Datacenter Server", VER_PLATFORM_WIN32_NT,
      6, 3, 10, VER_NT_SERVER, VER_SUITE_DATACENTER, PRODUCT_DATACENTER_SERVER, 0 },
    { "Windows 2012 R2 Standard Storage Server", VER_PLATFORM_WIN32_NT,
      6, 3, 10, VER_NT_SERVER, VER_SUITE_STORAGE_SERVER | VER_SUITE_SINGLEUSERTS | VER_SUITE_TERMINAL, PRODUCT_STORAGE_STANDARD_SERVER, 0 },
    { "Windows 10 Home", VER_PLATFORM_WIN32_NT,
      10, 0, 10, VER_NT_WORKSTATION, VER_SUITE_SINGLEUSERTS, -1, 1 },
    { "Windows 10 Pro", VER_PLATFORM_WIN32_NT,
      10, 0, 10, VER_NT_WORKSTATION, VER_SUITE_SINGLEUSERTS, -1, 2 },
    { "Windows 10 Education", VER_PLATFORM_WIN32_NT,
      10, 0, 10, VER_NT_WORKSTATION, VER_SUITE_SINGLEUSERTS, -1, 4 },
    { "Windows 10 Enterprise", VER_PLATFORM_WIN32_NT,
      10, 0, 10, VER_NT_WORKSTATION, VER_SUITE_SINGLEUSERTS, -1, 3 },
    { NULL, 0,
      0, 0, 0, 0, 0, 0, 0 }
};

static OSVERSIONINFO os_version_info;

/* define our own structure since the windows headers
   don't seem to agree on what OSVERSIONINFOEX should
   be defined as */
typedef struct _VICE_OSVERSIONINFOEX {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    CHAR szCSDVersion[128];
    WORD wServicePackMajor;
    WORD wServicePackMinor;
    WORD wSuiteMask;
    BYTE wProductType;
    BYTE wReserved;
} VICE_OSVERSIONINFOEX;

static VICE_OSVERSIONINFOEX os_version_ex_info;

/* RegOpenKeyEx wrapper for smart access to both 32bit and 64bit registry entries */
static LONG RegOpenKeyEx3264(HKEY hKey, LPCTSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
    LONG retval = 0;

    /* Check 64bit first */
    retval = RegOpenKeyEx(hKey, lpSubKey, ulOptions, samDesired | KEY_WOW64_64KEY, phkResult);

    if (retval == ERROR_SUCCESS) {
        return retval;
    }

    /* Check 32bit second */
    retval = RegOpenKeyEx(hKey, lpSubKey, ulOptions, samDesired | KEY_WOW64_32KEY, phkResult);

    if (retval == ERROR_SUCCESS) {
        return retval;
    }

    /* Fallback to normal open */
    retval = RegOpenKeyEx(hKey, lpSubKey, ulOptions, samDesired, phkResult);

    return retval;
}

/*
0: NT3, 1: NT4, 2: 95, 3: 95OSR2, 4: 98, 5: ME, 6: 2000, 7: XP, 8: XPSP1, 9: 2003, 10: VISTA+
*/

static int GetRealOS(void)
{
    HMODULE k = GetModuleHandle(TEXT("kernel32.dll"));

    if (GetProcAddress(k, "GetLocaleInfoEx") != NULL) {
        return 10;
    }
    if (GetProcAddress(k, "GetLargePageMinimum") != NULL) {
        return 9;
    }
    if (GetProcAddress(k, "GetDLLDirectory") != NULL) {
        return 8;
    }
    if (GetProcAddress(k, "GetNativeSystemInfo") != NULL) {
        return 7;
    }
    if (GetProcAddress(k, "ReplaceFile") != NULL) {
        return 6;
    }
    if (GetProcAddress(k, "OpenThread") != NULL) {
        return 5;
    }
    if (GetProcAddress(k, "GetThreadPriorityBoost") != NULL) {
        return 1;
    }
    if (GetProcAddress(k, "ConnectNamedPipe") != NULL) {
        return 0;
    }
    if (GetProcAddress(k, "IsDebuggerPresent") != NULL) {
        return 4;
    }
    if (GetProcAddress(k, "GetDiskFreeSpaceEx") != NULL) {
        return 3;
    }
    return 2;
}

static int IsWow64(void)
{
    int bIsWow64 = 0;
    typedef BOOL (APIENTRY *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
    LPFN_ISWOW64PROCESS fnIsWow64Process;
    HMODULE module = GetModuleHandle(TEXT("kernel32"));
    const char funcName[] = "IsWow64Process";

    fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(module, funcName);

    if (NULL != fnIsWow64Process) {
        fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
    }

    return bIsWow64;
}

static int IsReactOS(void)
{
    HKEY hKey;
    LONG ret;

    ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SOFTWARE\\ReactOS", 0, KEY_QUERY_VALUE, &hKey);
    if (ret == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return 1;
    }
    return 0;
}

static void get_ReactOS_ver_string(char **retval)
{
    OSVERSIONINFO RosVersionInfo;
    unsigned RosVersionLen;
    LPTSTR RosVersion;

    RosVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    memset(RosVersionInfo.szCSDVersion, 0, sizeof(RosVersionInfo.szCSDVersion));
    if (GetVersionEx(&RosVersionInfo)) {
        RosVersion = RosVersionInfo.szCSDVersion + strlen(RosVersionInfo.szCSDVersion) + 1;
        RosVersionLen = sizeof(RosVersionInfo.szCSDVersion) / sizeof(RosVersionInfo.szCSDVersion[0]) - (RosVersion - RosVersionInfo.szCSDVersion);
        if (7 <= RosVersionLen && 0 == strncmp(RosVersion, "ReactOS", 7)) {
            sprintf(*retval, "%s", RosVersion);
        } else {
            sprintf(*retval, "ReactOS %s", RosVersion);
        }
    } else {
        sprintf(*retval, "ReactOS");
    }
}

static int IsWine(void)
{
    if (GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "wine_get_unix_file_name") != NULL) {
        return 1;
    }
    return 0;
}

static int IsOdin32(void)
{
    if (GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "RegisterLXExe") != NULL) {
        return 1;
    }
    return 0;
}

static int IsHxDos(void)
{
    if (GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetDKrnl32Version") != NULL) {
        return 1;
    }
    return 0;
}

static char *get_win95_version(void)
{
    HKEY hKey;
    char PT[128];
    DWORD PTlen = 128;
    LONG ret;

    if (!strncmp(os_version_info.szCSDVersion, " A", 2)) {
        return "A";
    }
    if (!strncmp(os_version_info.szCSDVersion, " B", 2)) {
        return "B";
    }
    if (!strncmp(os_version_info.szCSDVersion, " C", 2)) {
        return "C";
    }

    ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion", 0, KEY_QUERY_VALUE, &hKey);
    if (ret != ERROR_SUCCESS) {
        return "";
    }

    ret = RegQueryValueEx(hKey, "SubVersionNumber", NULL, NULL, (LPBYTE)PT, &PTlen);
    if ((ret != ERROR_SUCCESS) || (PTlen > 128)) {
        return "";
    }

    RegCloseKey(hKey);

    if (lstrcmpi("a", PT) == 0) {
        return "A";
    }
    if (lstrcmpi("b", PT) == 0) {
        return "B";
    }
    if (lstrcmpi("c", PT) == 0) {
        return "C";
    }

    return "";
}

static char *get_win98_version(void)
{
    if (!strncmp(os_version_info.szCSDVersion, "A", 1)) {
        return " (Security)";
    }
    if (!strncmp(os_version_info.szCSDVersion, " A", 2)) {
        return "SE";
    }
    if (!strncmp(os_version_info.szCSDVersion, "B", 1)) {
        return "SE (Security)";
    }
    return "";
}


static int optional_mask_compare(int a, int b)
{
    if (b == -1 || (a & b) == b) {
        return 1;
    }
    return 0;
}

static int optional_compare(int a, int b)
{
    if (b < 0 || a == b) {
        return 1;
    }
    return 0;
}

static char windows_version[256];
static int got_os = 0;

/* 0 = error
   1 = Workstation
   2 = Server
   3 = Enterprise
   4 = Terminal Server
   5 = Small Business Server
 */
static int get_product_type_from_reg(void)
{
    HKEY hKey;
    char PT[128];
    DWORD PTlen = 128;
    LONG ret;

    ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\ProductOptions", 0, KEY_QUERY_VALUE, &hKey);
    if (ret != ERROR_SUCCESS) {
        return 0;
    }

    ret = RegQueryValueEx(hKey, "ProductType", NULL, NULL, (LPBYTE)PT, &PTlen);
    if ((ret != ERROR_SUCCESS) || (PTlen > 128)) {
        return 0;
    }

    RegCloseKey(hKey);

    if (lstrcmpi("WINNT", PT) == 0) {
        return 1;
    }

    if (lstrcmpi("SERVERNT", PT) == 0 || lstrcmpi("LANMANNT", PT) == 0) {
        ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\ProductOptions", 0, KEY_QUERY_VALUE, &hKey);
        ret = RegQueryValueEx(hKey, "ProductSuite", NULL, NULL, NULL, NULL);
        if (ret == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Terminal Server", 0, KEY_QUERY_VALUE, &hKey);
            if (ret == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return 4;
            }
            ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\LicenseInfoSuites\\SmallBusiness", 0, KEY_QUERY_VALUE, &hKey);
            if (ret == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return 5;
            }
            return 3;
        }
        return 2;
    }
    return 0;
}

static int get_sp_from_reg(void)
{
    if (!strncmp(os_version_info.szCSDVersion, "Service Pack 1", 14)) {
        return 1;
    }
    if (!strncmp(os_version_info.szCSDVersion, "Service Pack 2", 14)) {
        return 2;
    }
    if (!strncmp(os_version_info.szCSDVersion, "Service Pack 3", 14)) {
        return 3;
    }
    if (!strncmp(os_version_info.szCSDVersion, "Service Pack 4", 14)) {
        return 4;
    }
    if (!strncmp(os_version_info.szCSDVersion, "Service Pack 5", 14)) {
        return 5;
    }
    if (!strncmp(os_version_info.szCSDVersion, "Service Pack 6", 14)) {
        return 6;
    }
    return 0;
}

static int sp_is_nt4_6a(void)
{
    HKEY hKey;
    LONG ret;

    ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009", 0, KEY_QUERY_VALUE, &hKey);
    if (ret == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return 1;
    }
    return 0;
}

static int is_pe_builder(void)
{
    HKEY hKey;
    LONG ret;

    ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\PE Builder", 0, KEY_QUERY_VALUE, &hKey);
    if (ret == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return 1;
    }
    return 0;
}

static int is_flp(void)
{
    HKEY hKey;
    LONG ret;

    ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SYSTEM\\WPA\\Fundamentals", 0, KEY_QUERY_VALUE, &hKey);
    if (ret == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return 1;
    }
    return 0;
}

static int is_cluster(void)
{
    HKEY hKey;
    LONG ret;

    ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Compute Cluster", 0, KEY_QUERY_VALUE, &hKey);
    if (ret == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return 1;
    }
    return 0;
}

static int is_thin_pc(void)
{
    HKEY hKey;
    LONG ret;

    ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\ThinPC", 0, KEY_QUERY_VALUE, &hKey);
    if (ret == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return 1;
    }
    return 0;
}

static int is_embedded_2009(void)
{
    HKEY hKey;
    char PT[128];
    DWORD PTlen = 128;
    LONG ret;

    ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\WindowsEmbedded\\ProductVersion", 0, KEY_QUERY_VALUE, &hKey);
    if (ret != ERROR_SUCCESS) {
        return 0;
    }

    ret = RegQueryValueEx(hKey, "FeaturePackVersion", NULL, NULL, (LPBYTE)PT, &PTlen);
    if ((ret != ERROR_SUCCESS) || (PTlen > 128)) {
        return 0;
    }

    RegCloseKey(hKey);

    if (lstrcmpi("Windows Embedded Standard 2009", PT) == 0) {
        return 1;
    }
    return 0;
}

static int is_posready(void)
{
    HKEY hKey;
    char PT[128];
    DWORD PTlen = 128;
    LONG ret;

    ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\POSReady", 0, KEY_QUERY_VALUE, &hKey);
    if (ret != ERROR_SUCCESS) {
        return 0;
    }

    ret = RegQueryValueEx(hKey, "Version", NULL, NULL, (LPBYTE)PT, &PTlen);
    if ((ret != ERROR_SUCCESS) || (PTlen > 128)) {
        return 0;
    }

    RegCloseKey(hKey);

    if (lstrcmpi("2.0", PT) == 0) {
        return 2;
    }
    return 1;
}

/* 0 = None, 1 = Standard, 2 = Industry Pro, 3 = Industry Enterprise */
static int is_windows8_embedded(void)
{
    HKEY hKey;
    char PT[128];
    DWORD PTlen = 128;
    LONG ret;

    ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE, &hKey);
    if (ret != ERROR_SUCCESS) {
        return 0;
    }

    ret = RegQueryValueEx(hKey, "ProductName", NULL, NULL, (LPBYTE)PT, &PTlen);
    if ((ret != ERROR_SUCCESS) || (PTlen > 128)) {
        return 0;
    }

    RegCloseKey(hKey);

    if (lstrcmpi("Windows Embedded 8 Standard", PT) == 0) {
        return 1;
    }
    return 0;
}

/* 0 = None, 1 = Workgroup, 2 = Standard, 3 = Enterprise, 4 = Basic */
static int is_storage_server(void)
{
    HKEY hKey;
    char PT[128];
    DWORD PTlen = 128;
    LONG ret;

    ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE, &hKey);
    if (ret != ERROR_SUCCESS) {
        return 0;
    }

    ret = RegQueryValueEx(hKey, "ProductName", NULL, NULL, (LPBYTE)PT, &PTlen);
    if ((ret != ERROR_SUCCESS) || (PTlen > 128)) {
        return 0;
    }

    RegCloseKey(hKey);

    if (lstrcmpi("Windows Storage Server 2008 R2 Workgroup", PT) == 0) {
        return 1;
    }
    if (lstrcmpi("Windows Storage Server 2008 R2 Standard", PT) == 0) {
        return 2;
    }
    if (lstrcmpi("Windows Storage Server 2008 R2 Enterprise", PT) == 0) {
        return 3;
    }
    if (lstrcmpi("Windows (R) Storage Server 2008 Enterprise", PT) == 0) {
        return 3;
    }
    if (lstrcmpi("Windows (R) Storage Server 2008 Basic", PT) == 0) {
        return 4;
    }
    return 0;
}

/* Check for windows 8.1 and 10
   0 = 8.0 or below
   1 = 8.1
   2 = 10.0
*/
static int IsWindows8plus(void)
{
    HKEY hKey;
    char PT[128];
    DWORD PTlen = 128;
    LONG ret;

    ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE, &hKey);
    if (ret != ERROR_SUCCESS) {
        return 0;
    }

    ret = RegQueryValueEx(hKey, "CurrentVersion", NULL, NULL, (LPBYTE)PT, &PTlen);
    if ((ret != ERROR_SUCCESS) || (PTlen > 128)) {
        return 0;
    }

    RegCloseKey(hKey);

    if (lstrcmpi("6.3", PT) == 0) {
        return 1;
    }
    if (lstrcmpi("10.0", PT) == 0) {
        return 2;
    }
    return 0;
}

/* Check windows 10 edition
   0 = Unknown
   1 = Home
   2 = Pro
   3 = Enterprise
   4 = Education
*/
static int get_windows_10_edition(void)
{
    HKEY hKey;
    char PT[128];
    DWORD PTlen = 128;
    LONG ret;

    ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE, &hKey);
    if (ret != ERROR_SUCCESS) {
        return 0;
    }

    ret = RegQueryValueEx(hKey, "ProductName", NULL, NULL, (LPBYTE)PT, &PTlen);
    if ((ret != ERROR_SUCCESS) || (PTlen > 128)) {
        return 0;
    }

    RegCloseKey(hKey);

    if (lstrcmpi("Windows 10 Pro", PT) == 0) {
        return 2;
    }
    if (lstrcmpi("Windows 10 Home", PT) == 0) {
        return 1;
    }
    if (lstrcmpi("Windows 10 Enterprise", PT) == 0) {
        return 3;
    }
    if (lstrcmpi("Windows 10 Education", PT) == 0) {
        return 4;
    }

    return 0;
}

/* Get Windows NT 4.0 Small Business Server version:
   0 = None
   1 = 4.0
   2 = 4.5
*/
static int get_sbs_4x(void)
{
    HKEY hKey;
    char PT[128];
    DWORD PTlen = 128;
    LONG ret;
    LONG retval = 0;

    ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\BackOffice", 0, KEY_QUERY_VALUE, &hKey);
    if (ret == ERROR_SUCCESS) {
        ret = RegQueryValueEx(hKey, "SuiteVersion", NULL, NULL, (LPBYTE)PT, &PTlen);
        if ((ret == ERROR_SUCCESS) || (PTlen > 128)) {
            if (lstrcmpi("4.5", PT) == 0) {
                retval = 2;
            }
            if (lstrcmpi("4.0", PT) == 0) {
                retval = 1;
            }
        }
        RegCloseKey(hKey);
    }

    if (!retval) {
        ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Small Business", 0, KEY_QUERY_VALUE, &hKey);
        if (ret != ERROR_SUCCESS) {
            return 0;
        }
        ret = RegQueryValueEx(hKey, "Version", NULL, NULL, (LPBYTE)PT, &PTlen);
        if (ret != ERROR_SUCCESS) {
            return 0;
        }
        RegCloseKey(hKey);
        if (lstrcmpi("4.5", PT) == 0) {
            retval = 2;
        }
        if (lstrcmpi("4.0", PT) == 0) {
            retval = 1;
        }
        if (lstrcmpi("4.0a", PT) == 0) {
            retval = 1;
        }        
    }

    return retval;
}

/* Get MCE version:
   0 = Unknown
   1 = 2002
   2 = 2004
   3 = 2005
   4 = 2005 R1
   5 = 2005 R2
*/
static int get_mce_version(void)
{
    HKEY hKey;
    char PT[128];
    DWORD PTlen = 128;
    LONG ret;

    ret = RegOpenKeyEx3264(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Media Center", 0, KEY_QUERY_VALUE, &hKey);
    if (ret != ERROR_SUCCESS) {
        return 0;
    }

    ret = RegQueryValueEx(hKey, "Ident", NULL, NULL, (LPBYTE)PT, &PTlen);
    if ((ret != ERROR_SUCCESS) || (PTlen > 128)) {
        return 0;
    }

    RegCloseKey(hKey);

    if (lstrcmpi("2.7", PT) == 0) {
        return 2;
    }
    if (lstrcmpi("2.8", PT) == 0) {
        return 2;
    }
    if (lstrcmpi("3.0", PT) == 0) {
        return 3;
    }
    if (lstrcmpi("3.1", PT) == 0) {
        return 4;
    }
    if (lstrcmpi("4.0", PT) == 0) {
        return 5;
    }
    if (lstrcmpi("5.0", PT) == 0) {
        return 0;
    }
    if (lstrcmpi("5.1", PT) == 0) {
        return 0;
    }
    if (lstrcmpi("6.0", PT) == 0) {
        return 0;
    }
    return 1;
}

char *platform_get_windows_runtime_os(void)
{
    int found = 0;
    int i;
    VGPI ViceGetProductInfo;
    VGNSI ViceGetNativeSystemInfo;
    SYSTEM_INFO systeminfo;
    DWORD PT;
    int sp;
    int exinfo_valid = 0;
    char *reactos_ver = NULL;

    if (!got_os) {
        ZeroMemory(&os_version_info, sizeof(os_version_info));
        os_version_info.dwOSVersionInfoSize = sizeof(os_version_info);

        ZeroMemory(&os_version_ex_info, sizeof(os_version_ex_info));
        os_version_ex_info.dwOSVersionInfoSize = sizeof(os_version_ex_info);

        GetVersionEx(&os_version_info);

        windows_versions[0].platformid = (DWORD)os_version_info.dwPlatformId;
        windows_versions[0].majorver = (DWORD)os_version_info.dwMajorVersion;
        windows_versions[0].minorver = (DWORD)os_version_info.dwMinorVersion;
        windows_versions[0].realos = GetRealOS();

        /* check for windows 8.1 when windows 8 was found */
        if (windows_versions[0].majorver == 6 && windows_versions[0].minorver == 2) {
            if (IsWindows8plus() == 1) {
                windows_versions[0].minorver = (DWORD)3;
            } else if (IsWindows8plus() == 2) {
                windows_versions[0].majorver = (DWORD)10;
                windows_versions[0].minorver = (DWORD)0;
            }
        }

        if (windows_versions[0].platformid == VER_PLATFORM_WIN32_NT) {
            if (GetVersionEx((LPOSVERSIONINFOA)&os_version_ex_info)) {
                if (os_version_ex_info.wProductType == VER_NT_DOMAIN_CONTROLLER) {
                    windows_versions[0].producttype = (BYTE)VER_NT_SERVER;
                } else {
                    windows_versions[0].producttype = (BYTE)os_version_ex_info.wProductType;
                }
                windows_versions[0].suite = (WORD)os_version_ex_info.wSuiteMask;
                exinfo_valid = 1;
            } else {
                switch (get_product_type_from_reg()) {
                    case 0:
                    default:
                        windows_versions[0].producttype = 0;
                        windows_versions[0].suite = 0;
                        break;
                    case 1:
                        windows_versions[0].producttype = VER_NT_WORKSTATION;
                        windows_versions[0].suite = 0;
                        break;
                    case 2:
                        windows_versions[0].producttype = VER_NT_SERVER;
                        windows_versions[0].suite = 0;
                        break;
                    case 3:
                        windows_versions[0].producttype = VER_NT_SERVER;
                        windows_versions[0].suite = VER_SUITE_ENTERPRISE;
                        break;
                    case 4:
                        windows_versions[0].producttype = VER_NT_SERVER;
                        windows_versions[0].suite = VER_SUITE_TERMINAL;
                        break;
                    case 5:
                        windows_versions[0].producttype = VER_NT_SERVER;
                        windows_versions[0].suite = VER_SUITE_SMALLBUSINESS;
                        break;
                }
            }
            if (windows_versions[0].majorver == 5) {
                windows_versions[0].pt6 = is_posready();
            } else {
                if (windows_versions[0].majorver >= 6) {
                    if ((windows_versions[0].suite & VER_SUITE_EMBEDDEDNT) == VER_SUITE_EMBEDDEDNT) {
                        windows_versions[0].pt6 = is_thin_pc() | (is_posready() << 1) | is_windows8_embedded() << 2;
                    } else {
                        ViceGetProductInfo = (VGPI)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetProductInfo");
                        ViceGetProductInfo(os_version_ex_info.dwMajorVersion, os_version_ex_info.dwMinorVersion, 0, 0, &PT);
                        windows_versions[0].pt6 = PT;
                    }
                } else {
                    windows_versions[0].pt6 = -1;
                }
            }
        } else {
            windows_versions[0].producttype = -1;
            windows_versions[0].suite = -1;
        }

/* Metrics: 0000RMST
   R: Windows Server 2003 R2			SM_SERVERR2
   M: Windows XP Media Center Edition		SM_MEDIACENTER
   S: Windows XP Starter Edition		SM_STARTER
   T: Windows XP Tablet PC Edition		SM_TABLETPC
*/
        windows_versions[0].metrics = 0;
        if (GetSystemMetrics(SM_TABLETPC)) {
            windows_versions[0].metrics |= 1;
        }
        if (GetSystemMetrics(SM_STARTER)) {
            windows_versions[0].metrics |= 2;
        }
        if (GetSystemMetrics(SM_MEDIACENTER)) {
            windows_versions[0].metrics |= 4;
        }
        if (GetSystemMetrics(SM_SERVERR2)) {
            windows_versions[0].metrics |= 8;
        }

        if (windows_versions[0].metrics & 4 && windows_versions[0].majorver == 5 && windows_versions[0].minorver == 1) {
            windows_versions[0].pt6 = get_mce_version();
        }

        if (windows_versions[0].suite == (VER_SUITE_EMBEDDEDNT | VER_SUITE_SINGLEUSERTS)) {
            if (is_flp() == 0) {
                windows_versions[0].suite = VER_SUITE_EMBEDDEDNT;
            }
            if (is_embedded_2009()) {
                windows_versions[0].metrics |= 16;
            }
        }

        if (is_pe_builder() && windows_versions[0].majorver == 5 && windows_versions[0].minorver == 2) {
            windows_versions[0].producttype = VER_NT_SERVER;
        }

        if (is_cluster()) {
            windows_versions[0].suite |= VER_SUITE_COMPUTE_SERVER;
        }

        i = is_storage_server();
        if (i) {
            windows_versions[0].metrics = i;
        }

        if (windows_versions[0].majorver == 10 || (windows_versions[0].majorver == 6 && windows_versions[0].minorver == 3)) {
            i = get_windows_10_edition();
            if (i) {
                windows_versions[0].majorver = 10;
                windows_versions[0].minorver = 0;
                windows_versions[0].metrics = i;
            }
        }

        if (windows_versions[0].majorver == 4 && windows_versions[0].minorver == 0 && windows_versions[0].producttype == VER_NT_SERVER) {
            i = get_sbs_4x();
            if (i) {
                windows_versions[0].metrics = i;
            }
        }

        if (windows_versions[0].majorver == 5 && windows_versions[0].minorver == 0 && os_version_info.dwBuildNumber == 5111) {
            windows_versions[0].metrics = 1;
        }

#ifdef DEBUG_PLATFORM
        printf("current parameters: (%d %d %d %d %d %d %d %d)\n",
                    windows_versions[0].platformid,
                    windows_versions[0].majorver,
                    windows_versions[0].minorver,
                    windows_versions[0].realos,
                    windows_versions[0].producttype,
                    windows_versions[0].suite,
                    windows_versions[0].pt6,
                    windows_versions[0].metrics);
#endif



        for (i = 1; found == 0 && windows_versions[i].name != NULL; i++) {
#ifdef DEBUG_PLATFORM
        printf("%s (%d %d %d %d %d %d)\n", windows_versions[i].name,
                    windows_versions[i].platformid,
                    windows_versions[i].majorver,
                    windows_versions[i].minorver,
                    windows_versions[i].realos,
                    windows_versions[i].producttype,
                    windows_versions[i].suite);
#endif
            if (windows_versions[0].platformid == windows_versions[i].platformid) {
#ifdef DEBUG_PLATFORM
                printf("same platformid\n");
#endif
                if (windows_versions[0].majorver == windows_versions[i].majorver) {
#ifdef DEBUG_PLATFORM
                    printf("same majorver\n");
#endif
                    if (windows_versions[0].minorver == windows_versions[i].minorver) {
#ifdef DEBUG_PLATFORM
                        printf("same minorver\n");
#endif
                        if (windows_versions[0].realos > windows_versions[i].realos) {
#ifdef DEBUG_PLATFORM
                            printf("realos bigger\n");
#endif
                            windows_versions[0].producttype = -1;
                            windows_versions[0].suite = 0;
                            windows_versions[0].pt6 = 0;
                            windows_versions[0].metrics = 0;
                        }
                        if (windows_versions[0].producttype == windows_versions[i].producttype) {
#ifdef DEBUG_PLATFORM
                            printf("same producttype\n");
#endif
                            if (optional_mask_compare(windows_versions[0].suite, windows_versions[i].suite)) {
#ifdef DEBUG_PLATFORM
                                printf("same suite (mask)\n");
                                printf("pt6 of 0 = %d, pt6 of i = %d\n", windows_versions[0].pt6, windows_versions[i].pt6);
#endif
                                if (optional_compare(windows_versions[0].pt6, windows_versions[i].pt6)) {
#ifdef DEBUG_PLATFORM
                                    printf("same pt6\n");
#endif
                                    if (optional_mask_compare(windows_versions[0].metrics, windows_versions[i].metrics)) {
#ifdef DEBUG_PLATFORM
                                        printf("same metric (mask)\n");
#endif
                                        found = 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        if (found) {
            sprintf(windows_version, "%s", windows_versions[i - 1].name);
            if (windows_versions[0].platformid == VER_PLATFORM_WIN32_WINDOWS) {
                if (windows_versions[0].minorver == 0) {
                    sprintf(windows_version, "%s%s", windows_version, get_win95_version());
                }
                if (windows_versions[0].minorver == 10 || windows_versions[0].minorver == 90) {
                    sprintf(windows_version, "%s%s", windows_version, get_win98_version());
                }
            } else {
                if (exinfo_valid) {
                    sp = os_version_ex_info.wServicePackMajor;
                } else {
                    sp = get_sp_from_reg();
                }
                if (sp) {
                    if (sp == 6) {
                        if (sp_is_nt4_6a()) {
                            sprintf(windows_version, "%s SP6A", windows_version);
                        } else {
                            sprintf(windows_version, "%s SP6", windows_version);
                        }
                    } else {
                        sprintf(windows_version, "%s SP%d", windows_version, sp);
                    }
                }
            }
            if (is_pe_builder()) {
                sprintf(windows_version, "%s (PE)", windows_version);
            }
            if (windows_versions[0].realos > windows_versions[i - 1].realos) {
                sprintf(windows_version, "%s (compatibility mode)", windows_version);
            }
            if (IsWow64()) {
                ViceGetNativeSystemInfo = (VGNSI)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");
                ViceGetNativeSystemInfo(&systeminfo);
                if (systeminfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
                    sprintf(windows_version, "%s (WOW64 X64)", windows_version);
                } else {
                    sprintf(windows_version, "%s (WOW64 IA64)", windows_version);
                }
            } else {
                if (windows_versions[0].majorver >= 5) {
                    GetSystemInfo(&systeminfo);
                    if (systeminfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) {
                        sprintf(windows_version, "%s (64bit IA64)", windows_version);
                    } else if (systeminfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
                        sprintf(windows_version, "%s (64bit X64)", windows_version);
                    } else if (systeminfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_MIPS) {
                        sprintf(windows_version, "%s (32bit MIPS)", windows_version);
                    } else if (systeminfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA) {
                        sprintf(windows_version, "%s (32bit AXP)", windows_version);
                    } else if (systeminfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA64) {
                        sprintf(windows_version, "%s (32bit AXP64)", windows_version);
                    } else if (systeminfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_PPC) {
                        sprintf(windows_version, "%s (32bit PPC)", windows_version);
                    } else {
                        sprintf(windows_version, "%s (32bit X86)", windows_version);
                    }
                }
            }
            if (IsReactOS()) {
                reactos_ver = windows_version;
                get_ReactOS_ver_string(&reactos_ver);
            }
            if (IsWine()) {
                sprintf(windows_version, "%s (Wine)", windows_version);
            }
            if (IsOdin32()) {
                sprintf(windows_version, "%s (Odin32)", windows_version);
            }
            if (IsHxDos()) {
                sprintf(windows_version, "%s (HXDOS)", windows_version);
            }
        } else {
            sprintf(windows_version, "%s\nplatformid: %d\nmajorver: %d\nminorver: %d\nrealos: %d\nproducttype: %d\nsuite: %d\npt6: %d\nmetrics: %d",
                    "Unknown Windows version",
                    windows_versions[0].platformid,
                    windows_versions[0].majorver,
                    windows_versions[0].minorver,
                    windows_versions[0].realos,
                    windows_versions[0].producttype,
                    windows_versions[0].suite,
                    windows_versions[0].pt6,
                    windows_versions[0].metrics);
        }
        got_os = 1;
    }
    return windows_version;
}
#endif
#endif
