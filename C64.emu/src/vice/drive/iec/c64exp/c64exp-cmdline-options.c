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

#include "c64exp-cmdline-options.h"
#include "cmdline.h"
#include "drive.h"
#include "lib.h"
#include "translate.h"

static const cmdline_option_t cmdline_options[] = {
    { "-profdos1571", SET_RESOURCE, 1,
      NULL, NULL, "DriveProfDOS1571Name", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_PROFDOS_1571_ROM_NAME,
      NULL, NULL },
    { "-supercard", SET_RESOURCE, 1,
      NULL, NULL, "DriveSuperCardName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_SUPERCARD_ROM_NAME,
      NULL, NULL },
    { "-stardos", SET_RESOURCE, 1,
      NULL, NULL, "DriveStarDosName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_STARDOS_ROM_NAME,
      NULL, NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t cmd_drive[] = {
    { NULL, SET_RESOURCE, 1,
      NULL, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_TYPE, IDCLS_PAR_CABLE_C64EXP_TYPE,
      NULL, NULL },
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_PROFDOS,
      NULL, NULL },
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_PROFDOS,
      NULL, NULL },
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_SUPERCARD,
      NULL, NULL },
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_SUPERCARD,
      NULL, NULL },
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_STARDOS,
      NULL, NULL },
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_STARDOS,
      NULL, NULL },
    CMDLINE_LIST_END
};

int c64exp_cmdline_options_init(void)
{
    unsigned int dnr, i;

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        cmd_drive[0].name = lib_msprintf("-parallel%i", dnr + 8);
        cmd_drive[0].resource_name
            = lib_msprintf("Drive%iParallelCable", dnr + 8);
        cmd_drive[1].name = lib_msprintf("-drive%iprofdos", dnr + 8);
        cmd_drive[1].resource_name
            = lib_msprintf("Drive%iProfDOS", dnr + 8);
        cmd_drive[2].name = lib_msprintf("+drive%iprofdos", dnr + 8);
        cmd_drive[2].resource_name
            = lib_msprintf("Drive%iProfDOS", dnr + 8);
        cmd_drive[3].name = lib_msprintf("-drive%isupercard", dnr + 8);
        cmd_drive[3].resource_name
            = lib_msprintf("Drive%iSuperCard", dnr + 8);
        cmd_drive[4].name = lib_msprintf("+drive%isupercard", dnr + 8);
        cmd_drive[4].resource_name
            = lib_msprintf("Drive%iSuperCard", dnr + 8);
        cmd_drive[5].name = lib_msprintf("-drive%istardos", dnr + 8);
        cmd_drive[5].resource_name
            = lib_msprintf("Drive%iStarDos", dnr + 8);
        cmd_drive[6].name = lib_msprintf("+drive%istardos", dnr + 8);
        cmd_drive[6].resource_name
            = lib_msprintf("Drive%iStarDos", dnr + 8);

        if (cmdline_register_options(cmd_drive) < 0) {
            return -1;
        }

        for (i = 0; i < 7; i++) {
            lib_free(cmd_drive[i].name);
            lib_free(cmd_drive[i].resource_name);
        }
    }

    return cmdline_register_options(cmdline_options);
}
