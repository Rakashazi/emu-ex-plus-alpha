/*
 * petmodel.h - PET model detection and setting.
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#ifndef VICE_PETMODEL_H
#define VICE_PETMODEL_H

#include "types.h"

#define PETMODEL_2001 0
#define PETMODEL_3008 1
#define PETMODEL_3016 2
#define PETMODEL_3032 3
#define PETMODEL_3032B 4
#define PETMODEL_4016 5
#define PETMODEL_4032 6
#define PETMODEL_4032B 7
#define PETMODEL_8032 8
#define PETMODEL_8096 9
#define PETMODEL_8296 10
#define PETMODEL_SUPERPET 11

#define PETMODEL_NUM 12
#define PETMODEL_UNKNOWN 99

extern int petmodel_get(void);
extern void petmodel_set(int model);

#endif
