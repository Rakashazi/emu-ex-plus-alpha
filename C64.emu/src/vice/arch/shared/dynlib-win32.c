/** \file   dynlib-win32.c
 * \brief   Win32 support for dynamic library loading
 *
 * \author  Christian Vogelgsang <chris@vogelgsang.org>
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

#include <windows.h>

#include "dynlib.h"

static int opencbm_fix_tried = 0;

/*
    HACK HACK:

    The original opencbm.dll (tested with 0.4.99.104) did, for some reason,
    under certain conditions, not find the libusb-1.0.dll that it required
    to work with zoomfloppy/xum1541.
    The problem seems to be this:
    a) on a "virgin" windows system, after "opencbm" was installed as
       documented ("install xum1541"), including answering yes to "install the
       USB driver?", the following files have been copied into the system:
       %windows%\System32\opencbm.dll
       %windows%\System32\opencbm-xum1541.dll
       %programmfiles%\opencbm\opencbm.dll
       %programmfiles%\opencbm\opencbm-xum1541.dll
       %programmfiles%\opencbm\libusb-1.0.dll
    b) When VICE opens opencbm.dll, the "%programmfiles%\opencbm" directory
       is not searched. The result is that it can not find libusb-1.0.dll
    To work around the problem, we look up the uninstall path for "opencbm" in
    the registry, which gives us the path where libusb-1.0.dll can be found,
    and we add this to the dll search path.
 */
void archdep_opencbm_fix_dll_path(void)
{
    HKEY hKey;
    TCHAR OpenCBM[256];
    DWORD OpenCBMlen = 256;
    LONG ret;
    char *dllpath = NULL;
    char *dllpath_end = NULL;

    if (opencbm_fix_tried) {
        return;
    }

    /* open the opencbm registry key */
    ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\OpenCBM"), 0, KEY_QUERY_VALUE, &hKey);

    if (ret == ERROR_SUCCESS) {
        /* get the uninstall string, it contains the path to where opencbm is installed */
        ret = RegQueryValueEx(hKey, TEXT("UninstallString"), NULL, NULL, (LPBYTE)OpenCBM, &OpenCBMlen);
        if (ret == ERROR_SUCCESS && OpenCBMlen <= 256) {
            /* get to the beginning of the path */
            dllpath = strstr(OpenCBM, "/c ");
            if (dllpath) {
                dllpath += 4;

                /* get to the ending of the path */
                dllpath_end = strstr(dllpath, "\\installer\\uninstall.cmd");

                if (dllpath_end) {
                    /* put a NULL ending at the correct location, now dllpath has the correct path */
                    dllpath_end[0] = 0;

                    /* add the path to the dll search path */
                    SetDllDirectory(dllpath);
                }
            }
        }
        RegCloseKey(hKey);
    }
    opencbm_fix_tried = 1;
}

void *vice_dynlib_open(const char *name)
{
    return LoadLibrary(name);
}

void *vice_dynlib_symbol(void *handle,const char *name)
{
    return GetProcAddress((HMODULE)handle, name);
}

char *vice_dynlib_error(void)
{
    return "unknown";
}

int vice_dynlib_close(void *handle)
{
    if (FreeLibrary(handle)) {
        return 0;
    } else {
        return -1;
    }
}
