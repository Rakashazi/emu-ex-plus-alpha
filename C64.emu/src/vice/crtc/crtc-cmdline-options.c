/*
 * crtc-cmdline-options.c - A line-based CRTC emulation (under construction).
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#include "crtc-cmdline-options.h"
#include "crtctypes.h"
#include "cmdline.h"
#include "raster-cmdline-options.h"
#include "resources.h"
#include "translate.h"

/* CRTC command-line options.  */
static const cmdline_option_t cmdline_options[] =
{
    { "-CRTCstretchvertical", SET_RESOURCE, 0,
      NULL, NULL, "CrtcStretchVertical", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_STRETCH_VERTICAL,
      NULL, NULL },
    { "+CRTCstretchvertical", SET_RESOURCE, 0,
      NULL, NULL, "CrtcStretchVertical", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_STRETCH_VERTICAL,
      NULL, NULL },
    CMDLINE_LIST_END
};

int crtc_cmdline_options_init(void)
{
    if (raster_cmdline_options_chip_init("CRTC", crtc.video_chip_cap) < 0) {
        return -1;
    }

    return cmdline_register_options(cmdline_options);
}
