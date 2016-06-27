/*
 * pet-resources.h
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

#ifndef VICE_PET_RESOURCES_H
#define VICE_PET_RESOURCES_H

/* FIXME: perhaps move this into pets.h ? */

/*
    the following keyboard models exist for the PET:

    - chicklet keyboard (original PET 2001)
    - graphics keyboard with the 20 key numpad (PET 2001-N)

      "Graphics"

    - business keyboard (PET 2001-B)

      For some countries (e.g. Germany) there where some kits for the business
      keyboard including an changed Exxx-EPROM (editor) with other keyboard
      mappings and some changeable keyboard caps with the right characters on it.

      "Business (us)"
      "Business (uk)"
      "Business (de)"
      "Business (jp)"
*/

#define KBD_TYPE_BUSINESS_US    0
#define KBD_TYPE_BUSINESS_UK    1
#define KBD_TYPE_BUSINESS_DE    2
#define KBD_TYPE_BUSINESS_JP    3
#define KBD_TYPE_GRAPHICS_US    4
#define KBD_TYPE_LAST           4
#define KBD_TYPE_NUM            5

#define KBD_TYPE_STR_BUSINESS_US    "buus"
#define KBD_TYPE_STR_BUSINESS_UK    "buuk"
#define KBD_TYPE_STR_BUSINESS_DE    "bude"
#define KBD_TYPE_STR_BUSINESS_JP    "bujp"
#define KBD_TYPE_STR_GRAPHICS_US    "grus"

extern int pet_colour_type;
extern int pet_colour_analog_bg;

extern int pet_resources_init(void);
extern void pet_resources_shutdown(void);

#endif
