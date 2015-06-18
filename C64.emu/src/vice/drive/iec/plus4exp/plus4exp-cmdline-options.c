/*
 * c64exp-cmdline-options.c
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

#include "cmdline.h"
#include "drive.h"
#include "lib.h"
#include "plus4exp-cmdline-options.h"
#include "translate.h"

static cmdline_option_t cmd_drive[] = {
    { NULL, SET_RESOURCE, 1,
      NULL, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_TYPE, IDCLS_PAR_CABLE_PLUS4EXP_TYPE,
      NULL, NULL },
    { NULL }
};

int plus4exp_cmdline_options_init(void)
{
    unsigned int dnr, i;

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        cmd_drive[0].name = lib_msprintf("-parallel%i", dnr + 8);
        cmd_drive[0].resource_name
            = lib_msprintf("Drive%iParallelCable", dnr + 8);

        if (cmdline_register_options(cmd_drive) < 0) {
            return -1;
        }

        for (i = 0; i < 1; i++) {
            lib_free((char *)cmd_drive[i].name);
            lib_free((char *)cmd_drive[i].resource_name);
        }
    }

    return 0;
}
