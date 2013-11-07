/*
 * soundalsa.c - Implementation of the ALSA sound device
 *
 * Written by
 *  Dag Lem <resid@nimrod.no>
 *  - based on ALSA /test/pcm.c and various scarce documentation.
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

#define ALSA_PCM_NEW_HW_PARAMS_API

#include "alsa/asoundlib.h"
#include "debug.h"
#include "log.h"
#include "sound.h"

static snd_pcm_t *handle;
static int alsa_bufsize;
static int alsa_fragsize;
static int alsa_channels;
static int alsa_can_pause;

static int alsa_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    int err, dir;
    unsigned int rate, periods;
    snd_pcm_uframes_t period_size;
    snd_pcm_hw_params_t *hwparams;

    if (!param) {
        param = "default";
    }

    snd_pcm_hw_params_alloca(&hwparams);

    if ((err = snd_pcm_open(&handle, param, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        log_message(LOG_DEFAULT, "Playback open error for '%s': %s", param,
                    snd_strerror(err));
        return 1;
    }

    if ((err = snd_pcm_hw_params_any(handle, hwparams)) < 0) {
        log_message(LOG_DEFAULT, "Broken configuration for playback: no configurations available: %s", snd_strerror(err));
        goto fail;
    }

    if ((err = snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        log_message(LOG_DEFAULT, "Access type not available for playback: %s", snd_strerror(err));
        goto fail;
    }

    if ((err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16)) < 0) {
        log_message(LOG_DEFAULT, "Sample format not available for playback: %s", snd_strerror(err));
        goto fail;
    }

    if ((err = snd_pcm_hw_params_set_channels(handle, hwparams, *channels)) < 0) {
        log_message(LOG_DEFAULT, "Channels count (%i) not available for playbacks: %s", *channels, snd_strerror(err));
        goto fail;
    }

    rate = (unsigned int)*speed;
    if ((err = snd_pcm_hw_params_set_rate_near(handle, hwparams, &rate, 0)) < 0) {
        log_message(LOG_DEFAULT, "Rate %iHz not available for playback: %s", *speed, snd_strerror(err));
        goto fail;
    }
    if (rate != (unsigned int)*speed) {
        printf("Rate doesn't match (requested %iHz, got %iHz)", *speed, rate);
        *speed = rate;
    }
    /* calculate requested buffer size */
    alsa_bufsize = (*fragsize) * (*fragnr);

    period_size = *fragsize;
    dir = 0;
    if ((err = snd_pcm_hw_params_set_period_size_near(handle, hwparams, &period_size, &dir)) < 0) {
        log_message(LOG_DEFAULT, "Unable to set period size %li for playback: %s", period_size, snd_strerror(err));
        goto fail;
    }
    *fragsize = period_size;

    /* number of periods according to the buffer size we wanted, nearest val */
    *fragnr = (alsa_bufsize + *fragsize / 2) / *fragsize;

    periods = *fragnr;
    dir = 0;
    if ((err = snd_pcm_hw_params_set_periods_near(handle, hwparams, &periods, &dir)) < 0) {
        log_message(LOG_DEFAULT, "Unable to set periods %i for playback: %s", periods, snd_strerror(err));
        goto fail;
    }
    *fragnr = periods;

    alsa_can_pause = snd_pcm_hw_params_can_pause(hwparams);

    if ((err = snd_pcm_hw_params(handle, hwparams)) < 0) {
        log_message(LOG_DEFAULT, "Unable to set hw params for playback: %s", snd_strerror(err));
        goto fail;
    }

    alsa_bufsize = (*fragsize) * (*fragnr);
    alsa_fragsize = *fragsize;
    alsa_channels = *channels;

    return 0;

fail:
    snd_pcm_close(handle);
    handle = NULL;
    return 1;
}

static int xrun_recovery(snd_pcm_t *handle, int err)
{
    if (err == -EPIPE) {    /* under-run */
        if ((err = snd_pcm_prepare(handle)) < 0) {
            log_message(LOG_DEFAULT, "Can't recover from underrun, prepare failed: %s", snd_strerror(err));
        }
        return 0;
    } else if (err == -ESTRPIPE) {
        while ((err = snd_pcm_resume(handle)) == -EAGAIN) {
            log_message(LOG_DEFAULT, "xrun_recovery: %s", snd_strerror(err));
            sleep(1);       /* wait until the suspend flag is released */
        }
        if (err < 0) {
            if ((err = snd_pcm_prepare(handle)) < 0) {
                log_message(LOG_DEFAULT, "Can't recover from suspend, prepare failed: %s", snd_strerror(err));
            }
        }
        return 0;
    }
    return err;
}

static int alsa_write(SWORD *pbuf, size_t nr)
{
    int err;

    nr /= alsa_channels;

    while (nr > 0) {
        err = snd_pcm_writei(handle, pbuf, nr);
        if (err == -EAGAIN) {
            log_message(LOG_DEFAULT, "Write error: %s", snd_strerror(err));
            continue;
        } else if (err < 0 && (err = xrun_recovery(handle, err)) < 0) {
            log_message(LOG_DEFAULT, "Write error: %s", snd_strerror(err));
            return 1;
        }
        pbuf += err * alsa_channels;
        nr -= err;
    }

    return 0;
}

static int alsa_bufferspace(void)
{
#ifdef HAVE_SND_PCM_AVAIL
    snd_pcm_sframes_t space = snd_pcm_avail(handle);
#else
    snd_pcm_sframes_t space = snd_pcm_avail_update(handle);
#endif
    /* keep alsa values real. Values < 0 mean errors, call to alsa_write
     * will resume. */
    if (space < 0 || space > alsa_bufsize) {
        space = alsa_bufsize;
    }
    return space;
}

static void alsa_close(void)
{
    snd_pcm_close(handle);
    handle = NULL;
    alsa_bufsize = 0;
    alsa_fragsize = 0;
}

static int alsa_suspend(void)
{
    int err;

    if (!alsa_can_pause) {
        return 1;
    }

    if ((err = snd_pcm_pause(handle, 1)) < 0) {
        log_message(LOG_DEFAULT, "Unable to pause playback: %s", snd_strerror(err));
        return 1;
    }

    return 0;
}

static int alsa_resume(void)
{
    int err;

    if (!alsa_can_pause) {
        return 1;
    }

    if ((err = snd_pcm_pause(handle, 0)) < 0) {
        log_message(LOG_DEFAULT, "Unable to resume playback: %s", snd_strerror(err));
        return 1;
    }

    return 0;
}

static sound_device_t alsa_device =
{
    "alsa",
    alsa_init,
    alsa_write,
    NULL,
    NULL,
    alsa_bufferspace,
    alsa_close,
    alsa_suspend,
    alsa_resume,
    1,
    2
};

int sound_init_alsa_device(void)
{
    return sound_register_device(&alsa_device);
}
