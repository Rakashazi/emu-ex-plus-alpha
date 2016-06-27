/*
 * sounduss.c - Implementation of the Linux/FreeBSD sound device
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

/* on FreeBSD SDL sound is to be used, even though
   the machine/soundcard.h file is present it is not
   compatible with this implementation of a uss
   sound driver. For BSDI this uss sound driver should
   not be used either. */

#if !defined(__FreeBSD__) && !defined(__bsdi__)

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "debug.h"
#include "log.h"
#include "sound.h"

#ifdef USE_OSS

static int uss_fd = -1;
static int uss_8bit = 0;
static int uss_bufsize = 0;
static int uss_fragsize = 0;
static int uss_channels;

/* For conversion from mono to stereo. */
#define BUFSIZE 32768
static int uss_duplicate = 0;
static SWORD buffer[2 * BUFSIZE];

static int uss_bufferspace(void);

static int uss_init(const char *param, int *speed,
                    int *fragsize, int *fragnr, int *channels)
{
    int st, tmp, orig;

    if (!param) {
        struct stat buf;
        if (!stat("/dev/dsp", &buf)) {
            param = "/dev/dsp";
        } else if (!stat("/dev/sound/dsp", &buf)) {
            param = "/dev/sound/dsp";
        }
    }

    if (!param) {
        log_message(LOG_DEFAULT, "Did not find any uss device");
        return 1;
    }

    uss_fd = open(param, O_WRONLY, 0777);

    if (uss_fd < 0) {
        log_message(LOG_DEFAULT, "Cannot open '%s' for writing", param);
        return 1;
    }

    /* samplesize 16 bits */
#ifdef WORDS_BIGENDIAN
    orig = tmp = AFMT_S16_BE;
#else
    orig = tmp = AFMT_S16_LE;
#endif
    st = ioctl(uss_fd, SNDCTL_DSP_SETFMT, &tmp);

    if (st < 0 || orig != tmp || getenv("USS8BIT")) {
        /* samplesize 8 bits */
        orig = tmp = AFMT_U8;
        st = ioctl(uss_fd, SNDCTL_DSP_SETFMT, &tmp);
        if (st < 0 || orig != tmp) {
            log_message(LOG_DEFAULT, "SNDCTL_DSP_SETFMT failed");
            goto fail;
        }
        log_message(LOG_DEFAULT, "Playing 8bit sample");
        uss_8bit = 1;
    }

    tmp = *channels;
    st = ioctl(uss_fd, SNDCTL_DSP_CHANNELS, &tmp);
    if (st < 0 || tmp != *channels) {
        /* Intel ICH and ICH0 only support 2 channels */
        if (*channels == 1 && tmp == 2) {
            uss_duplicate = 1;
        } else {
            log_message(LOG_DEFAULT, "SNDCTL_DSP_CHANNELS failed");
            /* no stereo */
            tmp = *channels = 1;
            st = ioctl(uss_fd, SNDCTL_DSP_CHANNELS, &tmp);
            if (st < 0 || tmp != *channels) {
                goto fail;
            }
        }
    }

    /* speed */
    tmp = *speed;
    st = ioctl(uss_fd, SNDCTL_DSP_SPEED, &tmp);
    if (st < 0 || tmp <= 0) {
        log_message(LOG_DEFAULT, "SNDCTL_DSP_SPEED failed");
        goto fail;
    }
    *speed = tmp;

    /* fragments */
    for (tmp = 1; 1 << tmp < *fragsize; tmp++) {
    }
    orig = tmp = tmp + (*fragnr << 16) + !uss_8bit;
    st = ioctl(uss_fd, SNDCTL_DSP_SETFRAGMENT, &tmp);

    if (st < 0 || (tmp ^ orig) & 0xffff) {
        log_message(LOG_DEFAULT, "SNDCTL_DSP_SETFRAGMENT failed");
        goto fail;
    }

    if (tmp != orig) {
        if (tmp >> 16 > *fragnr) {
            log_message(LOG_DEFAULT,
                        "SNDCTL_DSP_SETFRAGMENT: too many fragments");
            goto fail;
        }
        *fragnr = tmp >> 16;
        if (*fragnr < 3) {
            log_message(LOG_DEFAULT,
                        "SNDCTL_DSP_SETFRAGMENT: too few fragments");
            goto fail;
        }
    }

    uss_bufsize = (*fragsize) * (*fragnr);
    uss_fragsize = *fragsize;
    uss_channels = *channels;

    return 0;

fail:
    close(uss_fd);
    uss_fd = -1;
    uss_duplicate = 0;
    uss_8bit = 0;
    uss_bufsize = 0;
    uss_fragsize = 0;
    return 1;
}

static int uss_write(SWORD *pbuf, size_t nr)
{
    int i, now;
    size_t total;

    if (uss_duplicate) {
        for (i = 0; (size_t)i < nr; i++) {
            buffer[i * 2] = pbuf[i];
            buffer[i * 2 + 1] = pbuf[i];
        }
        pbuf = buffer;
        nr *= 2;
    }

    if (uss_8bit) {
        for (i = 0; (size_t)i < nr; i++) {
            ((char *)buffer)[i] = pbuf[i] / 256 + 128;
        }
        pbuf = buffer;
        total = nr;
    } else {
        total = nr * sizeof(SWORD);
    }

    for (i = 0; (size_t)i < total; i += now) {
        now = write(uss_fd, (char *)pbuf + i, total - i);
        if (now <= 0) {
            if (now < 0) {
                perror("uss_write");
            }
            return 1;
        }
    }
    return 0;
}

static int uss_bufferspace(void)
{
    audio_buf_info info;
    int st;

    /* ioctl(uss_fd, SNDCTL_DSP_GETOSPACE, &info) yields space in bytes
       in info.bytes. */
    st = ioctl(uss_fd, SNDCTL_DSP_GETOSPACE, &info);
    if (st < 0) {
        log_message(LOG_DEFAULT, "SNDCTL_DSP_GETOSPACE failed");
        return -1;
    }

    /* linux/soundcard.h says crazy things about the info.bytes metric, such
     * as that the value could be more than fragment * fragsize. OSS 4.0 manual
     * also says that GETOSPACE fragsize doesn't actually correspond to
     * hardware buffer sizes due to resampling and format conversions. This
     * probably also taints info.bytes to uselessness. */
    return info.fragments * uss_fragsize / (uss_8bit ? 1 : 2);
}

static void uss_close(void)
{
    close(uss_fd);
    uss_fd = -1;
    uss_duplicate = 0;
    uss_8bit = 0;
    uss_bufsize = 0;
    uss_fragsize = 0;
}

static int uss_suspend(void)
{
    int st;
    st = ioctl(uss_fd, SNDCTL_DSP_POST, NULL);
    if (st < 0) {
        log_message(LOG_DEFAULT, "SNDCTL_DSP_POST failed");
        return 1;
    }
    return 0;
}

static sound_device_t uss_device =
{
    "uss",
    uss_init,
    uss_write,
    NULL,
    NULL,
    uss_bufferspace,
    uss_close,
    uss_suspend,
    NULL,
    1,
    2           /* FIXME */
};

int sound_init_uss_device(void)
{
    return sound_register_device(&uss_device);
}
#endif

#endif
