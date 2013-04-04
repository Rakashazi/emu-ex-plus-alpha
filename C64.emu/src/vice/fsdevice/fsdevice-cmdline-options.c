/*
 * fsdevice-cmdline-options.c - File system device, command line options.
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
#include <string.h>

#include "archdep.h"
#include "cmdline.h"
#include "fsdevice.h"
#include "ioutil.h"
#include "lib.h"
#include "translate.h"
#include "types.h"

static int cmdline_fsdirectory(const char *param, void *extra_param)
{
    unsigned int unit;
    char *directory;

    unit = vice_ptr_to_uint(extra_param);
    directory = lib_malloc(ioutil_maxpathlen());

    strcpy(directory, param);
    strcat(directory, FSDEV_DIR_SEP_STR);

    fsdevice_set_directory(directory, unit);

    lib_free(directory);

    return 0;
}

static const cmdline_option_t cmdline_options[] = {
    { "-fs8", CALL_FUNCTION, 1,
      cmdline_fsdirectory, (void *)8, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_USE_AS_DIRECTORY_FSDEVICE_8,
      NULL, NULL },
    { "-fs9", CALL_FUNCTION, 1,
      cmdline_fsdirectory, (void *)9, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_USE_AS_DIRECTORY_FSDEVICE_9,
      NULL, NULL },
    { "-fs10", CALL_FUNCTION, 1,
      cmdline_fsdirectory, (void *)10, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_USE_AS_DIRECTORY_FSDEVICE_10,
      NULL, NULL },
    { "-fs11", CALL_FUNCTION, 1,
      cmdline_fsdirectory, (void *)11, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_USE_AS_DIRECTORY_FSDEVICE_11,
      NULL, NULL },
    { NULL }
};

int fsdevice_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
