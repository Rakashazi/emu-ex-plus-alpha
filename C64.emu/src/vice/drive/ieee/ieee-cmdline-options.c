/*
 * ieee-cmdline-options.c
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

#include "drive.h"
#include "lib.h"
#include "cmdline.h"
#include "ieee-cmdline-options.h"

static const cmdline_option_t cmdline_options[] =
{
    { "-dos2031", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DosName2031", NULL,
      "<Name>", "Specify name of 2031 DOS ROM image" },
    { "-dos2040", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DosName2040", NULL,
      "<Name>", "Specify name of 2040 DOS ROM image" },
    { "-dos3040", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DosName3040", NULL,
      "<Name>", "Specify name of 3040 DOS ROM image" },
    { "-dos4040", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DosName4040", NULL,
      "<Name>", "Specify name of 4040 DOS ROM image" },
    { "-dos1001", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DosName1001", NULL,
      "<Name>", "Specify name of 1001/8050/8250 DOS ROM image" },
    { "-dos9000", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DosName9000", NULL,
      "<Name>", "Specify name of D9090/D9060 DOS ROM image" },
    CMDLINE_LIST_END
};

int ieee_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
