/*
 * sid-resources.h - SID resources.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_SID_RESOURCES_H
#define VICE_SID_RESOURCES_H

int sid_resources_init(void);
int sid_common_resources_init(void);

int sid_set_sid2_address(int val, void *param);
int sid_set_sid3_address(int val, void *param);
int sid_set_sid4_address(int val, void *param);
int sid_set_sid5_address(int val, void *param);
int sid_set_sid6_address(int val, void *param);
int sid_set_sid7_address(int val, void *param);
int sid_set_sid8_address(int val, void *param);

extern int sid_stereo;
extern int checking_sid_stereo;
extern unsigned int sid2_address_start;
extern unsigned int sid2_address_end;
extern unsigned int sid3_address_start;
extern unsigned int sid3_address_end;
extern unsigned int sid4_address_start;
extern unsigned int sid4_address_end;
extern unsigned int sid5_address_start;
extern unsigned int sid5_address_end;
extern unsigned int sid6_address_start;
extern unsigned int sid6_address_end;
extern unsigned int sid7_address_start;
extern unsigned int sid7_address_end;
extern unsigned int sid8_address_start;
extern unsigned int sid8_address_end;

#endif
