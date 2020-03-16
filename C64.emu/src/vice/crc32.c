/** \file   crc32.c
 *  \brief  CRC32 checksum implementation
 *
 * \author  Spiro Trikaliotis <Spiro.Trikaliotis@gmx.de>
 * \author  Andreas Boose <viceteam@t-online.de>
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

#include "vice.h"

#include <stdio.h>

#include "archdep.h"
#include "crc32.h"
#include "lib.h"
#include "util.h"


/** \brief  CRC32 polynomial
 */
#define CRC32_POLY  0xedb88320

/** \brief  Table of constants
 */
static uint32_t crc32_table[256];


/** \brief  Flag indicating if crc32_table has been initialized
 */
static int crc32_is_initialized = 0;


/** \brief  Calculate CRC32 checksum of \a len bytes of \a buffer
 *
 * \param[in]   buffer  buffer
 * \param[in]   len     length of \a buffer
 *
 * \return  CRC32 checksum
 *
 * \todo    Perhaps change \a buffer into uint8_t and \a len into size_t?
 */
uint32_t crc32_buf(const char *buffer, unsigned int len)
{
    int i, j;
    uint32_t crc, c;
    const char *p;

    if (!crc32_is_initialized) {
        for (i = 0; i < 256; i++) {
            c = (uint32_t)i;
            for (j = 0; j < 8; j++) {
                c = c & 1 ? CRC32_POLY ^ (c >> 1) : c >> 1;
            }
            crc32_table[i] = c;
        }
        crc32_is_initialized = 1;
    }

    crc = 0xffffffff;
    for (p = buffer; len > 0; ++p, --len) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ *p) & 0xff];
    }

    return ~crc;
}


/** \brief  Calculate CRC32 checksum of file \a filename
 *
 * \param[in]   filename    path to file
 *
 * \return  CRC32 checksum
 */
uint32_t crc32_file(const char *filename)
{
    FILE *fd;
    char *buffer;
    unsigned int len;
    uint32_t crc = 0;

    if (util_check_null_string(filename) < 0) {
        return 0;
    }

    fd = fopen(filename, MODE_READ);

    if (fd == NULL) {
        return 0;
    }

    len = (unsigned int)util_file_length(fd);

    buffer = lib_malloc(len);

    if (fread(buffer, len, 1, fd) == 1) {
        crc = crc32_buf(buffer, len);
    }

    fclose(fd);
    lib_free(buffer);

    return crc;
}


/*
 * These functions are designed to handle event data in an
 * architecture-independent way, always reading/storing a crc32 value in little
 * endian format and then translating it to the architectures endianness.
 */

/** \brief  Turn \a crc into a little endian value
 *
 * \param[in]   crc     32-bit architecture-independent unsigned int
 * \param[out]  dest    destination of little endian value (4 bytes)
 */
void crc32_to_le(uint8_t *dest, uint32_t crc)
{
    dest[0] = crc & 0xff;
    dest[1] = (crc >> 8) & 0xff;
    dest[2] = (crc >> 16) & 0xff;
    dest[3] = (crc >> 24) & 0xff;
}


/** \brief  Read 32-bit unsigned from little endian data
 *
 * \param[in]   src data (4 bytes representing a 32-bit little endian value)
 *
 * \return  32-bit architecture-dependent unsigned int
 */
uint32_t crc32_from_le(const uint8_t *src)
{
    return src[0] + (src[1] << 8) + (src[2] << 16) + (src[3] << 24);
}
