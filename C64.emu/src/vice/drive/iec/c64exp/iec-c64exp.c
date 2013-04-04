/*
 * iec-c64exp.c - IEC drive C64 expansion specific routines.
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

#include "c64exp-cmdline-options.h"
#include "c64exp-resources.h"
#include "drivetypes.h"
#include "dolphindos3.h"
#include "profdos.h"
#include "supercard.h"


int iec_c64exp_resources_init(void)
{
    return c64exp_resources_init();
}

void iec_c64exp_resources_shutdown(void)
{
    c64exp_resources_shutdown();
}

int iec_c64exp_cmdline_options_init(void)
{
    return c64exp_cmdline_options_init();
}

void iec_c64exp_init(struct drive_context_s *drv)
{
    dd3_init(drv);
    profdos_init(drv);
    supercard_init(drv);
}

void iec_c64exp_reset(struct drive_context_s *drv)
{
    dd3_reset(drv);
    profdos_reset(drv);
    supercard_reset(drv);
}

void iec_c64exp_mem_init(struct drive_context_s *drv, unsigned int type)
{
    dd3_mem_init(drv, type);
    profdos_mem_init(drv, type);
    supercard_mem_init(drv, type);
}
