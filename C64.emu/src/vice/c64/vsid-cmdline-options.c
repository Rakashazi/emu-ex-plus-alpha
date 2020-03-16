/** \file   vsid-cmdline-options.c
 * \brief   Handle VSID command line options
 *
 * \author  Andreas Boose <viceteam@t-online.de>
 * \author  Ettore Perazzoli <ettore@comm2000.it>
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c64model.h"
#include "c64rom.h"
#include "c64-cmdline-options.h"
#include "c64-resources.h"
#include "cmdline.h"
#include "log.h"
#include "machine.h"
#include "patchrom.h"
#include "resources.h"
#include "vicii.h"

#include "vsid-cmdline-options.h"


static const cmdline_option_t cmdline_options[] =
{
    { "-hvsc-root", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
        NULL, NULL, "HVSCRoot", NULL,
        "<path>", "Set path to HVSC root directory" },
    CMDLINE_LIST_END
};


/** \brief  Register VSID-specific command line options
 *
 * \return  0 on success, < 0 on failure
 */
int vsid_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
