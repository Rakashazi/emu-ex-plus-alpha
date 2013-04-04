/*
 * soundiff.c - Implementation of the AmigaOS IFF/8SVX dump sound device
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

static FILE *iff_fd = NULL;
static int samples = 0;
static int stereo = 0;

static int iff_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    /* IFF/8SVX header. */
    BYTE mono_header[48] = "FORMssss8SVXVHDR\0\0\0\024oooo\0\0\0\0\0\0\0\0rr\001\0\0\001\0\000BODYssss";
    BYTE stereo_header[60] = "FORMssss8SVXVHDR\0\0\0\024oooo\0\0\0\0\0\0\0\0rr\001\0\0\001\0\0CHAN\0\0\0\004\0\0\0\006BODYssss";

    WORD sample_rate = (WORD)*speed;

    iff_fd = fopen(param ? param : "vicesnd.iff", MODE_WRITE);
    if (!iff_fd) {
        return 1;
    }

    /* Reset number of samples. */
    samples = 0;

    /* Initialize header. */
    if (*channels == 2) {
        stereo = 1;
        stereo_header[32] = (BYTE)((sample_rate >> 8) & 0xff);
        stereo_header[33] = (BYTE)(sample_rate & 0xff);
        if (fwrite(stereo_header, 1, 60, iff_fd) != 60) {
            fclose(iff_fd);
            return 1;
        }
    } else {
        stereo = 0;
        mono_header[32] = (BYTE)((sample_rate >> 8) & 0xff);
        mono_header[33] = (BYTE)(sample_rate & 0xff);
        if (fwrite(mono_header, 1, 48, iff_fd) != 48) {
            fclose(iff_fd);
            return 1;
        }
    }
    return 0;
}

static int iff_write(SWORD *pbuf, size_t nr)
{
    BYTE sample[1];
    unsigned int i;

    for (i = 0; i < nr; i++) {
        sample[0] = (BYTE)((WORD)pbuf[i] >> 8);
        if (fwrite(sample, 1, 1, iff_fd) != 1) {
            return 1;
        }
    }

    /* Accumulate number of samples. */
    samples += (int)nr;

    return 0;
}

static void iff_close(void)
{
    int res = -1;
    BYTE blen[4];
    BYTE slen[4];
    BYTE flen[4];

    blen[0] = (BYTE)((samples >> 24) & 0xff);
    blen[1] = (BYTE)((samples >> 16) & 0xff);
    blen[2] = (BYTE)((samples >> 8) & 0xff);
    blen[3] = (BYTE)(samples & 0xff);

    if (stereo == 1) {
        slen[0] = (BYTE)((samples >> 25) & 0xff);
        slen[1] = (BYTE)((samples >> 17) & 0xff);
        slen[2] = (BYTE)((samples >> 9) & 0xff);
        slen[3] = (BYTE)((samples >> 1) & 0xff);

        flen[0] = (BYTE)(((samples + 52) >> 24) & 0xff);
        flen[1] = (BYTE)(((samples + 52) >> 16) & 0xff);
        flen[2] = (BYTE)(((samples + 52) >> 8) & 0xff);
        flen[3] = (BYTE)((samples + 52) & 0xff);
    } else {
        slen[0] = blen[0];
        slen[1] = blen[1];
        slen[2] = blen[2];
        slen[3] = blen[3];

        flen[0] = (BYTE)(((samples + 40) >> 24) & 0xff);
        flen[1] = (BYTE)(((samples + 40) >> 16) & 0xff);
        flen[2] = (BYTE)(((samples + 40) >> 8) & 0xff);
        flen[3] = (BYTE)((samples + 40) & 0xff);
    }
    fseek(iff_fd, 4, SEEK_SET);
    if (fwrite(flen, 1, 4, iff_fd) != 4) {
        goto fail;
    }
    fseek(iff_fd, 20, SEEK_SET);
    if (fwrite(slen, 1, 4, iff_fd) != 4) {
        goto fail;
    }

    if (stereo == 1) {
        fseek(iff_fd, 56, SEEK_SET);
    } else {
        fseek(iff_fd, 44, SEEK_SET);
    }
    if (fwrite(blen, 1, 4, iff_fd) != 4) {
        goto fail;
    }
    res = 0;

fail:
    fclose(iff_fd);
    iff_fd = NULL;

    if (res < 0) {
        log_debug("ERROR iff_close failed.");
    }
}

static sound_device_t iff_device =
{
    "iff",
    iff_init,
    iff_write,
    NULL,
    NULL,
    NULL,
    iff_close,
    NULL,
    NULL,
    0,
    2
};

int sound_init_iff_device(void)
{
    return sound_register_device(&iff_device);
}
