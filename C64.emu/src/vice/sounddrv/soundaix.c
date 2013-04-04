/*
 * soundaix.c - Implementation of the AIX sound device
 *
 * Written by
 *  Chris Sharp <sharpc@hursley.ibm.com>
 *
 * Integration with the other sound code
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

/* XXX: includes? */

#include "vice.h"

#include "lib.h"
#include "sound.h"

#include <UMS/UMSAudioDevice.h>
#include <UMS/UMSBAUDDevice.h>

/* XXX: AIX: can these be made static and use aix_ -prefix on these? */
UMSAudioDeviceMClass audio_device_class;
UMSAudioDevice_ReturnCode rc;
UMSBAUDDevice audio_device;
Environment *ev;
UMSAudioTypes_Buffer buffer;
UMSAudioDeviceMClass_ErrorCode audio_device_class_error;
char* error_string;
char* audio_formats_alias;
char* audio_inputs_alias;
char* audio_outputs_alias;
char* obyte_order;
long out_rate;
long left_gain, right_gain;


static int aix_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    int st, tmp, i;

    /* No stereo capability. */
    *channels = 1;

    /* open device */
    ev = somGetGlobalEnvironment();
    audio_device = UMSBAUDDeviceNew();
    rc = UMSAudioDevice_open(audio_device, ev, "/dev/paud0", "PLAY", UMSAudioDevice_BlockingIO);
    if (audio_device == NULL) {
        fprintf(errfile, "can't create audio device object\nError: %s\n",
                error_string);
        return 1;
    }

    rc = UMSAudioDevice_set_volume(audio_device, ev, 100);
    rc = UMSAudioDevice_set_balance(audio_device, ev, 0);

    rc = UMSAudioDevice_set_time_format(audio_device, ev, UMSAudioTypes_Msecs);

    lib_free(obyte_order);
    obyte_order = NULL;

    rc = UMSAudioDevice_set_byte_order(audio_device, ev, "LSB");

    /* set 16bit */
    rc = UMSAudioDevice_set_bits_per_sample(audio_device, ev, 16);
    rc = UMSAudioDevice_set_audio_format_type(audio_device, ev, "PCM");
    rc = UMSAudioDevice_set_number_format(audio_device, ev, "TWOS_COMPLEMENT");

    /* set speed */
    rc = UMSAudioDevice_set_sample_rate(audio_device, ev, *speed, &out_rate);

    /* channels */
    rc = UMSAudioDevice_set_number_of_channels(audio_device, ev, 1);

    /* should we use the default? */
    left_gain = right_gain = 100;
    rc = UMSAudioDevice_enable_output(audio_device, ev, "LINE_OUT", &left_gain, &right_gain);

    /* set buffer size */
    tmp = (*fragsize) * (*fragnr) * sizeof(SWORD);
    buffer._maximum = tmp;
    buffer._buffer = lib_malloc(tmp);
    buffer._length = 0;


    rc = UMSAudioDevice_initialize(audio_device, ev);
    rc = UMSAudioDevice_start(audio_device, ev);

    return 0;
#if 0
    /* XXX: AIX: everything should check rc, this isn't used now */
fail:
    UMSAudioDevice_stop(audio_device, ev);
    UMSAudioDevice_close(audio_device, ev);
    _somFree(audio_device);
    lib_free(buffer._buffer);
    audio_device = NULL;

    return 1;
#endif
}

static int aix_write(SWORD *pbuf, size_t nr)
{
    int total, i, now;
    long samples_written;

    total = nr * sizeof(SWORD);
    buffer._length = total;
    memcpy(buffer._buffer, pbuf, total);
    rc = UMSAudioDevice_write(audio_device, ev, &buffer, total, &samples_written);
    return 0;
}

static int aix_bufferspace(void)
{
    int i = -1;
    /* UMSAudioDevice_write_buff_remain returns space in bytes. */
    rc = UMSAudioDevice_write_buff_remain(audio_device, ev, &i);
    if (i < 0) {
        return -1;
    }
    /* fprintf(errfile,"Audio Buffer remains: %d\n blocks",i); */
    return i / sizeof(SWORD);
}

static void aix_close(void)
{
    rc = UMSAudioDevice_play_remaining_data(audio_device, ev, TRUE);
    UMSAudioDevice_stop(audio_device, ev);
    UMSAudioDevice_close(audio_device, ev);
    _somFree(audio_device);
    lib_free(buffer._buffer);
    audio_device = NULL;
}


static sound_device_t aix_device =
{
    "aix",
    aix_init,
    aix_write,
    NULL,
    NULL,
    aix_bufferspace,
    aix_close,
    NULL,
    NULL,
    1,
    1
};

int sound_init_aix_device(void)
{
    return sound_register_device(&aix_device);
}
