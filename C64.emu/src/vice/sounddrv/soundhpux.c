/*
 * soundhpux.c - Implementation of the HPUX sound device
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
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

#include <sys/ioctl.h>
#include <sys/audio.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "sound.h"

static int hpux_fd = -1;

static int hpux_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    int st, tmp, i;

    /* No stereo capability. */
    *channels = 1;

    if (!param) {
        param = "/dev/audio";
    }
    /* open device */
    hpux_fd = open(param, O_WRONLY, 0777);
    if (hpux_fd < 0) {
        return 1;
    }
    /* set 16bit */
    st = ioctl(hpux_fd, AUDIO_SET_DATA_FORMAT, AUDIO_FORMAT_LINEAR16BIT);
    if (st < 0) {
        goto fail;
    }
    /* set speed */
    st = ioctl(hpux_fd, AUDIO_SET_SAMPLE_RATE, *speed);
    if (st < 0) {
        goto fail;
    }
    /* channels */
    st = ioctl(hpux_fd, AUDIO_SET_CHANNELS, 1);
    if (st < 0) {
        goto fail;
    }
    /* should we use the default? */
    st = ioctl(hpux_fd, AUDIO_SET_OUTPUT, AUDIO_OUT_SPEAKER);
    if (st < 0) {
        goto fail;
    }
    /* set buffer size */
    tmp = (*fragsize) * (*fragnr) * sizeof(SWORD);
    st = ioctl(hpux_fd, AUDIO_SET_TXBUFSIZE, tmp);
    if (st < 0) {
        /* XXX: what are valid buffersizes? */
        for (i = 1; i < tmp; i *= 2) {
        }
        tmp = i;
        st = ioctl(hpux_fd, AUDIO_SET_TXBUFSIZE, tmp);
        if (st < 0) {
            goto fail;
        }
        *fragnr = tmp / ((*fragsize) * sizeof(SWORD));
    }
    return 0;
fail:
    close(hpux_fd);
    hpux_fd = -1;
    return 1;
}

static int hpux_write(SWORD *pbuf, size_t nr)
{
    int total, i, now;
    total = nr * sizeof(SWORD);
    for (i = 0; i < total; i += now) {
        now = write(hpux_fd, (char *)pbuf + i, total - i);
        if (now <= 0) {
            return 1;
        }
    }
    return 0;
}

static int hpux_bufferspace(void)
{
    int st;
    struct audio_status ast;
    /* ioctl(fd, AUDIO_GET_STATUS, &ast) yields space in bytes
       in ast.transmit_buffer_count. */
    st = ioctl(hpux_fd, AUDIO_GET_STATUS, &ast);
    if (st < 0) {
        return -1;
    }
    return ast.transmit_buffer_count / sizeof(SWORD);
}

static void hpux_close(void)
{
    close(hpux_fd);
    hpux_fd = -1;
}


static sound_device_t hpux_device =
{
    "hpux",
    hpux_init,
    hpux_write,
    NULL,
    NULL,
    hpux_bufferspace,
    hpux_close,
    NULL,
    NULL,
    1,
    1
};

int sound_init_hpux_device(void)
{
    return sound_register_device(&hpux_device);
}
