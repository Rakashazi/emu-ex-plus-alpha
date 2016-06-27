/*
 * drive-sound.h - Drive sound for 1541 and friends
 *
 * Written by
 *  Kajtar Zsolt <soci@c64.rulez.org>
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

#ifndef VICE_DRIVE_SOUND_H
#define VICE_DRIVE_SOUND_H

#define DRIVE_SOUND_MOTOR_ON 4
#define DRIVE_SOUND_MOTOR_OFF 5

void drive_sound_update(int i, int unit);
void drive_sound_head(int track, int step, int unit);

void drive_sound_stop(void);
void drive_sound_init(void);

#endif
