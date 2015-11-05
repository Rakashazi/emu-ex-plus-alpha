/*
 * vdc-resources.h - Resources for the MOS 8563 (VDC) emulation.
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

#ifndef VICE_VDC_RESOURCES_H
#define VICE_VDC_RESOURCES_H

/* VDC resources.  */
struct vdc_resources_s {
    int vdc_64kb_expansion; /* Flag: VDC memory size.  */
    int stretchy;           /* additional doubling of y size */
};
typedef struct vdc_resources_s vdc_resources_t;

extern vdc_resources_t vdc_resources;

extern int vdc_resources_init(void);

#endif
