/** \file   archdep_real_path.h
 * \brief   Normalize path names - header
 * \author  Michael C. Martin <mcmartin@gmail.com>
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

#ifndef VICE_ARCHDEP_REAL_PATH_H
#define VICE_ARCHDEP_REAL_PATH_H

/* These functions convert, or compare, pathnames in their
 * canonicalized forms: aboslute paths, symlinks resolved, and
 * capitalization normalized for case-preserving case-insensitive
 * filesystems like NTFS and AFS. The following caveats apply to all
 * of these functions:
 *
 * - input paths should be in a form fopen() expects.
 * - input paths should be at most PATH_MAX (_MAX_PATH on Windows) long.
 * - output paths should fit PATH_MAX (_MAX_PATH on Windows) characters.
 * - input paths should exist: canonicalizing paths potentially involves
 *   directory traversals and if these fail the whole conversion is
 *   allowed to (but not required to) fail.
 */

char *archdep_real_path(const char *pathname, char *resolved_pathname);
int archdep_real_path_equal(const char *path1, const char *path2);

#endif
