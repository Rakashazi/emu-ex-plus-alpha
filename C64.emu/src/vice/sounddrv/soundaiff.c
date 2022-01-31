/*
 * soundaiff.c - Implementation of the AIFF dump sound device
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

static FILE *aiff_fd = NULL;
static int samples = 0;

static int aiff_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    int i;
    unsigned int check_value;

    /* AIFF header. */
    unsigned char header[54] = "FORMssssAIFFCOMM\0\0\0\022\0cffff\0\020\100rrr\0\0\0\0\0\0SSNDssss\0\0\0\0\0\0\0\0";

    uint32_t sample_rate = (uint32_t)*speed;

    if (sample_rate < 8000 || sample_rate > 48000) {
        return 1;
    }

    aiff_fd = fopen(param ? param : "vicesnd.aiff", MODE_WRITE);
    if (!aiff_fd) {
        return 1;
    }

    samples = 0;

    /* Initialize header. */
    header[21] = (uint8_t)(*channels & 0xff);
    check_value = 2;
    for (i = 0; i < 15; i++) {
        if (sample_rate >= check_value && sample_rate < (check_value * 2)) {
            header[29] = (uint8_t)i;
            header[30] = (uint8_t)((((sample_rate) << (14 - i)) >> 8) & 0xff);
            header[31] = (uint8_t)(((sample_rate) << (14 - i)) & 0xff);
        }
        check_value = check_value * 2;
    }

    return (fwrite(header, 1, 54, aiff_fd) != 54);
}

static int aiff_write(int16_t *pbuf, size_t nr)
{
#ifndef WORDS_BIGENDIAN
    unsigned int i;

    /* Swap bytes on little endian machines. */
    for (i = 0; i < nr; i++) {
        pbuf[i] = (int16_t)((((uint16_t)pbuf[i] & 0xff) << 8) | ((uint16_t)pbuf[i] >> 8));
    }
#endif

    if (nr != fwrite(pbuf, sizeof(int16_t), nr, aiff_fd)) {
        return 1;
    }

    /* Swap the bytes back just in case. */
#ifndef WORDS_BIGENDIAN
    for (i = 0; i < nr; i++) {
        pbuf[i] = (int16_t)((((uint16_t)pbuf[i] & 0xff) << 8) | ((uint16_t)pbuf[i] >> 8));
    }
#endif

    /* Accumulate number of samples. */
    samples += (int)nr;

    return 0;
}


static void aiff_close(void)
{
    int res = -1;
    uint8_t slen[4];
    uint8_t alen[4];
    uint8_t flen[4];

    alen[0] = (uint8_t)((samples >> 24) & 0xff);
    alen[1] = (uint8_t)((samples >> 16) & 0xff);
    alen[2] = (uint8_t)((samples >> 8) & 0xff);
    alen[3] = (uint8_t)(samples & 0xff);

    slen[0] = (uint8_t)((((samples * 2) + 8) >> 24) & 0xff);
    slen[1] = (uint8_t)((((samples * 2) + 8) >> 16) & 0xff);
    slen[2] = (uint8_t)((((samples * 2) + 8) >> 8) & 0xff);
    slen[3] = (uint8_t)(((samples * 2) + 8) & 0xff);

    flen[0] = (uint8_t)((((samples * 2) + 46) >> 24) & 0xff);
    flen[1] = (uint8_t)((((samples * 2) + 46) >> 16) & 0xff);
    flen[2] = (uint8_t)((((samples * 2) + 46) >> 8) & 0xff);
    flen[3] = (uint8_t)(((samples * 2) + 46) & 0xff);

    fseek(aiff_fd, 4, SEEK_SET);
    if (fwrite(flen, 1, 4, aiff_fd) != 4) {
        goto fail;
    }

    fseek(aiff_fd, 22, SEEK_SET);
    if (fwrite(alen, 1, 4, aiff_fd) != 4) {
        goto fail;
    }

    fseek(aiff_fd, 42, SEEK_SET);
    if (fwrite(slen, 1, 4, aiff_fd) != 4) {
        goto fail;
    }
    res = 0;

fail:
    fclose(aiff_fd);
    aiff_fd = NULL;

    if (res < 0) {
        log_debug("ERROR aiff_close failed.");
    }
}

static const sound_device_t aiff_device =
{
    "aiff",
    aiff_init,
    aiff_write,
    NULL,
    NULL,
    NULL,
    aiff_close,
    NULL,
    NULL,
    0,
    2,
    false
};

int sound_init_aiff_device(void)
{
    return sound_register_device(&aiff_device);
}
