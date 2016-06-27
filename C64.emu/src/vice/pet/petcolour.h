/*
 * petcolour.h
 *
 * Written by
 *  Olaf Seibert <rhialto@falu.nl>
 *
 *  This implements the 2-chip colour board for the Universal PET
 *  mainboard (i.e. model 4032) as designed by Steve Gray:
 *  http://www.6502.org/users/sjgray/projects/colourpet/index.html
 *  The hardware is a work-in-progress.
 *
 *  The second version of the ColourPET board is the one that is emulated,
 *  because it is 80 columns compatible.
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

#define COLOUR_MEMORY_START 0x0800

int petcolour_set_type(int val);
void petcolour_init(void);
