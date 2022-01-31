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
#include <stddef.h>

/* for charset_petconvstring, charset_petconv_stralloc */
#define CONVERT_TO_PETSCII                  0
#define CONVERT_TO_ASCII                    1
#define CONVERT_TO_ASCII_WITH_CTRLCODES     2
#define CONVERT_TO_UTF8                     3

/* for charset_p_toascii */
#define CONVERT_WITHOUT_CTRLCODES           0
#define CONVERT_WITH_CTRLCODES              1

/* TODO:    Fix these functions to use size_t for lenght, not int
 */

extern uint8_t *charset_petconvstring(uint8_t *c, int mode);
extern uint8_t charset_p_toascii(uint8_t c, int mode);
extern uint8_t charset_p_topetcii(uint8_t c);

extern uint8_t charset_screencode_to_petcii(uint8_t code);

extern uint8_t charset_petcii_to_screencode(uint8_t code,
                                         unsigned int reverse_mode);
extern void charset_petcii_to_screencode_line(const uint8_t *line, uint8_t **buf,
                                              unsigned int *len);

extern int charset_petscii_to_ucs(uint8_t c);
extern int charset_ucs_to_utf8(uint8_t *out, int code, size_t len);

extern uint8_t *charset_petconv_stralloc(uint8_t *in, int mode);

extern char *charset_hexstring_to_byte(char *source, char *destination);
extern char *charset_replace_hexcodes(char *source);

#endif
