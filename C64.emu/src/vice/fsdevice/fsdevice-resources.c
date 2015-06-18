/*
 * fsdevice-resources.c - File system device, resources.
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

#include "vice.h"

#include <stdio.h>

#include "archdep.h"
#include "fsdevice-resources.h"
#include "fsdevice.h"
#include "lib.h"
#include "resources.h"
#include "util.h"


int fsdevice_convert_p00_enabled[4];
int fsdevice_save_p00_enabled[4];
int fsdevice_hide_cbm_files_enabled[4];
char *fsdevice_dir[4] = { NULL, NULL, NULL, NULL };


static int set_fsdevice_convert_p00(int val, void *param)
{
    fsdevice_convert_p00_enabled[vice_ptr_to_int(param) - 8] = val ? 1 : 0;

    return 0;
}

static int set_fsdevice_dir(const char *name, void *param)
{
    util_string_set(&fsdevice_dir[vice_ptr_to_int(param) - 8], name ? name : "");
    return 0;
}

static int set_fsdevice_save_p00(int val, void *param)
{
    fsdevice_save_p00_enabled[vice_ptr_to_int(param) - 8] = val ? 1 : 0;

    return 0;
}

static int set_fsdevice_hide_cbm_files(int val, void *param)
{
    if (val && !fsdevice_convert_p00_enabled[vice_ptr_to_int(param) - 8]) {
        return -1;
    }

    fsdevice_hide_cbm_files_enabled[vice_ptr_to_int(param) - 8] = val ? 1 : 0;
    return 0;
}

/* ------------------------------------------------------------------------- */

static const resource_string_t resources_string[] = {
    { "FSDevice8Dir", FSDEVICE_DEFAULT_DIR, RES_EVENT_NO, NULL,
      (void *)&fsdevice_dir[0], set_fsdevice_dir, (void *)8 },
    { "FSDevice9Dir", FSDEVICE_DEFAULT_DIR, RES_EVENT_NO, NULL,
      (void *)&fsdevice_dir[1], set_fsdevice_dir, (void *)9 },
    { "FSDevice10Dir", FSDEVICE_DEFAULT_DIR, RES_EVENT_NO, NULL,
      (void *)&fsdevice_dir[2], set_fsdevice_dir, (void *)10 },
    { "FSDevice11Dir", FSDEVICE_DEFAULT_DIR, RES_EVENT_NO, NULL,
      (void *)&fsdevice_dir[3], set_fsdevice_dir, (void *)11 },
    { NULL }
};

static const resource_int_t resources_int[] = {
    { "FSDevice8ConvertP00", 1, RES_EVENT_NO, NULL,
      &fsdevice_convert_p00_enabled[0],
      set_fsdevice_convert_p00, (void *)8 },
    { "FSDevice9ConvertP00", 1, RES_EVENT_NO, NULL,
      &fsdevice_convert_p00_enabled[1],
      set_fsdevice_convert_p00, (void *)9 },
    { "FSDevice10ConvertP00", 1, RES_EVENT_NO, NULL,
      &fsdevice_convert_p00_enabled[2],
      set_fsdevice_convert_p00, (void *)10 },
    { "FSDevice11ConvertP00", 1, RES_EVENT_NO, NULL,
      &fsdevice_convert_p00_enabled[3],
      set_fsdevice_convert_p00, (void *)11 },
    { "FSDevice8SaveP00", 0, RES_EVENT_NO, NULL,
      &fsdevice_save_p00_enabled[0],
      set_fsdevice_save_p00, (void *)8 },
    { "FSDevice9SaveP00", 0, RES_EVENT_NO, NULL,
      &fsdevice_save_p00_enabled[1],
      set_fsdevice_save_p00, (void *)9 },
    { "FSDevice10SaveP00", 0, RES_EVENT_NO, NULL,
      &fsdevice_save_p00_enabled[2],
      set_fsdevice_save_p00, (void *)10 },
    { "FSDevice11SaveP00", 0, RES_EVENT_NO, NULL,
      &fsdevice_save_p00_enabled[3],
      set_fsdevice_save_p00, (void *)11 },
    { "FSDevice8HideCBMFiles", 0, RES_EVENT_NO, NULL,
      &fsdevice_hide_cbm_files_enabled[0],
      set_fsdevice_hide_cbm_files, (void *)8 },
    { "FSDevice9HideCBMFiles", 0, RES_EVENT_NO, NULL,
      &fsdevice_hide_cbm_files_enabled[1],
      set_fsdevice_hide_cbm_files, (void *)9 },
    { "FSDevice10HideCBMFiles", 0, RES_EVENT_NO, NULL,
      &fsdevice_hide_cbm_files_enabled[2],
      set_fsdevice_hide_cbm_files, (void *)10 },
    { "FSDevice11HideCBMFiles", 0, RES_EVENT_NO, NULL,
      &fsdevice_hide_cbm_files_enabled[3],
      set_fsdevice_hide_cbm_files, (void *)11 },
    { NULL }
};

int fsdevice_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void fsdevice_resources_shutdown(void)
{
    lib_free(fsdevice_dir[0]);
    lib_free(fsdevice_dir[1]);
    lib_free(fsdevice_dir[2]);
    lib_free(fsdevice_dir[3]);
}
