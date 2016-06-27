/*
 * charset.h - Character set conversions.
 *
 * Written by
 *  Jouko Valta <jopi@stekt.oulu.fi>
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

#ifndef VICE_CHARSET_H
#define VICE_CHARSET_H

#include "types.h"

#define a2p(c) charset_petconvstring(c, 0)
#define p2a(c) charset_petconvstring(c, 1)

extern BYTE *charset_petconvstring(BYTE *c, int dir);
extern BYTE charset_p_toascii(BYTE c, int cs);
extern BYTE charset_p_topetcii(BYTE c);

extern BYTE charset_screencode_to_petcii(BYTE code);

extern BYTE charset_petcii_to_screencode(BYTE code,
                                         unsigned int reverse_mode);
extern void charset_petcii_to_screencode_line(const BYTE *line, BYTE **buf,
                                              unsigned int *len);

extern char * charset_hexstring_to_byte( char * source, char * destination );
extern char *charset_replace_hexcodes(char * source);

#endif
