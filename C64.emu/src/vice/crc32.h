/** \file   crc32.h
 * \brief  CRC32 checksum implementation
 *
 * \author  Spiro Trikaliotis <Spiro.Trikaliotis@gmx.de>
 * \author  Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_CRC32_H
#define VICE_CRC32_H

#include "types.h"

extern uint32_t crc32_buf(const char *buffer, unsigned int len);
extern uint32_t crc32_file(const char *filename);


void     crc32_to_le(uint8_t *dest, uint32_t crc);
uint32_t crc32_from_le(const uint8_t *src);

#endif
