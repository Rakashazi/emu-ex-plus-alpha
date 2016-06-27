/*
 * vdc-cmdline-options.c - Command-line options for the MOS 8563 (VDC)
 * emulation.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Markus Brenner <markus@brenner.de>
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
 * */

#include "vice.h"

#include <stdio.h>

#include "cmdline.h"
#include "raster-cmdline-options.h"
#include "resources.h"
#include "translate.h"
#include "vdc-cmdline-options.h"
#include "vdctypes.h"

/* VDC command-line options.  */
static const cmdline_option_t cmdline_options[] =
{
    { "-VDCstretchvertical", SET_RESOURCE, 0,
      NULL, NULL, "VDCStretchVertical", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_STRETCH_VERTICAL,
      NULL, NULL },
    { "+VDCstretchvertical", SET_RESOURCE, 0,
      NULL, NULL, "VDCStretchVertical", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_STRETCH_VERTICAL,
      NULL, NULL },
    { "-VDC16KB", SET_RESOURCE, 0,
      NULL, NULL, "VDC64KB", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_SET_VDC_MEMORY_16KB,
      NULL, NULL },
    { "-VDC64KB", SET_RESOURCE, 0,
      NULL, NULL, "VDC64KB", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_SET_VDC_MEMORY_64KB,
      NULL, NULL },
    { "-VDCRevision", SET_RESOURCE, 1,
      NULL, NULL, "VDCRevision", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NUMBER, IDCLS_SET_VDC_REVISION,
      NULL, NULL },
    CMDLINE_LIST_END
};

int vdc_cmdline_options_init(void)
{
    if (raster_cmdline_options_chip_init("VDC", vdc.video_chip_cap) < 0) {
        return -1;
    }

    return cmdline_register_options(cmdline_options);
}
