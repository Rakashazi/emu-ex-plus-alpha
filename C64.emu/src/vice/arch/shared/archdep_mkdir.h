/** \file   archdep_mkdir.h
 * \brief   Create a directory - header
 * \author  Bas Wassink <b.wassink@ziggo.nl>
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

#ifndef VICE_ARCHDEP_MKDIR_H
#define VICE_ARCHDEP_MKDIR_H

#define ARCHDEP_MKDIR_RWXU   0700
#define ARCHDEP_MKDIR_RWXUG  0770
#define ARCHDEP_MKDIR_RWXUGO 0777

int archdep_mkdir          (const char *pathname, int mode);
int archdep_mkdir_recursive(const char *pathname, int mode);

#endif

