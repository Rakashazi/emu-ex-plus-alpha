/*
 * soundvoc.c - Implementation of the Creative Voice VOC dump sound device
 *
 * Written by
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

#include "vice.h"

#include <stdio.h>

#include "sound.h"
#include "types.h"
#include "archdep.h"
#include "log.h"

#define VOC_MAX 0x6fc00c   /* taken from sound conversion program */

static FILE *voc_fd = NULL;
static int samples = 0;
static int block_start = 0;
static int extra_block = 0;

static int voc_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    /* VOC header. */
    BYTE header[26] = "Creative Voice File\032\032\0\024\001\037\021";
    BYTE block_header[16] = "\011sssrrrr\026c\004\0\0\0\0\0";
    DWORD sample_rate = *speed;

    voc_fd = fopen(param ? param : "vicesnd.voc", MODE_WRITE);
    if (!voc_fd) {
        return 1;
    }

    samples = 0;
    extra_block = 0;

    if (fwrite(header, 1, 26, voc_fd) != 26) {
        fclose(voc_fd);
        return 1;
    }

    block_start = ftell(voc_fd);

    /* Initialize header. */
    block_header[9] = (BYTE)(*channels & 0xff);
    block_header[4] = (BYTE)(sample_rate & 0xff);
    block_header[5] = (BYTE)((sample_rate >> 8) & 0xff);
    block_header[6] = (BYTE)((sample_rate >> 16) & 0xff);
    block_header[7] = (BYTE)((sample_rate >> 24) & 0xff);

    return (fwrite(block_header, 1, 16, voc_fd) != 16);
}

static int voc_write(SWORD *pbuf, size_t nr)
{
    BYTE rlen[3];

    /* VOC block header. */
    BYTE extra_block_header[] = "\002sss";

    #ifdef WORDS_BIGENDIAN
    unsigned int i;

    /* Swap bytes on big endian machines. */
    for (i = 0; i < nr; i++) {
        pbuf[i] = (((WORD)pbuf[i] & 0xff) << 8) | ((WORD)pbuf[i] >> 8);
    }
    #endif

    if ((samples + (nr * 2)) >= (VOC_MAX - 12)) {
        if (extra_block == 0) {
            rlen[0] = (BYTE)(((samples * 2) + 12) & 0xff);
            rlen[1] = (BYTE)((((samples * 2) + 12) >> 8) & 0xff);
            rlen[2] = (BYTE)((((samples * 2) + 12) >> 16) & 0xff);
            fseek(voc_fd, block_start + 1, SEEK_SET);
            if (fwrite(rlen, 1, 3, voc_fd) != 3) {
                return 1;
            }
            fseek(voc_fd, 0, SEEK_END);
            block_start = ftell(voc_fd);
            if (fwrite(extra_block_header, 1, 4, voc_fd) != 4) {
                return 1;
            }
            samples = 0;
            extra_block++;
        } else {
            rlen[0] = (BYTE)((samples * 2) & 0xff);
            rlen[1] = (BYTE)(((samples * 2) >> 8) & 0xff);
            rlen[2] = (BYTE)(((samples * 2) >> 16) & 0xff);
            fseek(voc_fd, block_start + 1, SEEK_SET);
            if (fwrite(rlen, 1, 3, voc_fd) != 3) {
                return 1;
            }
            fseek(voc_fd, 0, SEEK_END);
            block_start = ftell(voc_fd);
            if (fwrite(extra_block_header, 1, 4, voc_fd) != 4) {
                return 1;
            }
            samples = 0;
        }
    }

    if (nr != fwrite(pbuf, sizeof(SWORD), nr, voc_fd)) {
        return 1;
    }

    /* Swap the bytes back just in case. */
    #ifdef WORDS_BIGENDIAN
    for (i = 0; i < nr; i++) {
        pbuf[i] = (((WORD)pbuf[i] & 0xff) << 8) | ((WORD)pbuf[i] >> 8);
    }
    #endif

    /* Accumulate number of samples. */
    samples += (int)nr;

    return 0;
}

static void voc_close(void)
{
    int res = -1;
    BYTE rlen[3];

    rlen[0] = (BYTE)((samples * 2) & 0xff);
    rlen[1] = (BYTE)(((samples * 2) >> 8) & 0xff);
    rlen[2] = (BYTE)(((samples * 2) >> 16) & 0xff);
    fseek(voc_fd, block_start + 1, SEEK_SET);
    if (fwrite(rlen, 1, 3, voc_fd) == 3) {
        res = 0;
    }
    fclose(voc_fd);
    voc_fd = NULL;

    if (res < 0) {
        log_debug("ERROR voc_close failed.");
    }
}

static sound_device_t voc_device =
{
    "voc",
    voc_init,
    voc_write,
    NULL,
    NULL,
    NULL,
    voc_close,
    NULL,
    NULL,
    0,
    2
};

int sound_init_voc_device(void)
{
    return sound_register_device(&voc_device);
}
