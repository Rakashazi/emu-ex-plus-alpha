/** \file   textfilereader.h
 * \brief   Stack-based text file reader - header
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
 */

#ifndef VICE_HOTKEYS_TEXTFILEREADER_H
#define VICE_HOTKEYS_TEXTFILEREADER_H

#include <stdbool.h>

#include "hotkeystypes.h"

void        textfile_reader_init (textfile_reader_t *reader);
void        textfile_reader_free (textfile_reader_t *reader);
bool        textfile_reader_open (textfile_reader_t *reader, const char *path);
bool        textfile_reader_close(textfile_reader_t *reader);
const char *textfile_reader_read (textfile_reader_t *reader);
long        textfile_reader_linenum (const textfile_reader_t *reader);
const char *textfile_reader_filename(const textfile_reader_t *reader);

#endif
