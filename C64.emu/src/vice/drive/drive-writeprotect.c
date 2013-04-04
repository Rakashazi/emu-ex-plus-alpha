/*
 * drive-writeprotect.c
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

#include "drive-writeprotect.h"
#include "drive.h"
#include "types.h"


BYTE drive_writeprotect_sense(drive_t *dptr)
{
    /* Clear the write protection bit for the time the disk is pulled out on
       detach.  */
    if (dptr->detach_clk != (CLOCK)0) {
        if (*(dptr->clk) - dptr->detach_clk < DRIVE_DETACH_DELAY) {
            return 0x0;
        }
        dptr->detach_clk = (CLOCK)0;
    }
    /* Set the write protection bit for the minimum time until a new disk
       can be inserted.  */
    if (dptr->attach_detach_clk != (CLOCK)0) {
        if (*(dptr->clk) - dptr->attach_detach_clk
            < DRIVE_ATTACH_DETACH_DELAY) {
            return 0x10;
        }
        dptr->attach_detach_clk = (CLOCK)0;
    }
    /* Clear the write protection bit for the time the disk is put in on
       attach.  */
    if (dptr->attach_clk != (CLOCK)0) {
        if (*(dptr->clk) - dptr->attach_clk < DRIVE_ATTACH_DELAY) {
            return 0x0;
        }
        dptr->attach_clk = (CLOCK)0;
    }

    if ((dptr->GCR_image_loaded == 0) && (dptr->P64_image_loaded == 0)) {
        /* No disk in drive, write protection is off. */
        return 0x10;
    } else {
        if (dptr->P64_image_loaded && dptr->p64) {
            if (dptr->p64->WriteProtected) {
                return 0x0;
            }
        }
        return dptr->read_only ? 0x0 : 0x10;
    }
}
