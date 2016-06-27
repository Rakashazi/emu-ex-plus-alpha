/*
 * portaudio_drv.c - PortAudio audio input driver.
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

#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "portaudio_drv.h"
#include "sampler.h"

#ifdef USE_PORTAUDIO
#include <portaudio.h>

static log_t portaudio_log = LOG_ERR;

static int stream_started = 0;
static PaStream *stream = NULL;

static unsigned int old_frame;
static unsigned int sound_frames_per_sec;
static unsigned int sound_cycles_per_frame;
static unsigned int sound_samples_per_frame;
static unsigned int same_sample = 0;

static int current_channels = 0;

static WORD *stream_buffer = NULL;
static BYTE old_sample = 0x80;

static void portaudio_start_stream(void)
{
    PaStreamParameters inputParameters;
    PaError err = paNoError;

    inputParameters.device = Pa_GetDefaultInputDevice();
    if (inputParameters.device != paNoDevice) {
        inputParameters.channelCount = current_channels;
        inputParameters.sampleFormat = paInt16;
        inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultHighInputLatency ;
        inputParameters.hostApiSpecificStreamInfo = NULL;
        sound_cycles_per_frame = machine_get_cycles_per_frame();
        sound_frames_per_sec = machine_get_cycles_per_second() / sound_cycles_per_frame;
        sound_samples_per_frame = 44100 / sound_frames_per_sec;
        err = Pa_OpenStream(&stream, &inputParameters, NULL, 44100, sound_samples_per_frame, paClipOff, NULL, NULL);
        if (err == paNoError) {
            err = Pa_StartStream(stream);
            if (err == paNoError) {
                stream_started = 1;
                stream_buffer = lib_malloc(sound_samples_per_frame * 2 * current_channels);
                memset(stream_buffer, 0, sound_samples_per_frame * 2 * current_channels);
                old_frame = (maincpu_clk / sound_cycles_per_frame) + 1;
            } else {
                log_error(portaudio_log, "Could not start stream");
            }
        } else {
            log_error(portaudio_log, "Could not open stream");
        }
    } else {
        log_error(portaudio_log, "Could not find a default input device");
    }
}

static void portaudio_stop_stream(void)
{
    Pa_AbortStream(stream);
    Pa_CloseStream(stream);
    stream = NULL;
    if (stream_buffer) {
        lib_free(stream_buffer);
        stream_buffer = NULL;
    }
    stream_started = 0;
}

static void portaudio_start_sampling(int channels)
{
    PaError err = paNoError;

    if (stream_started) {
        log_error(portaudio_log, "Attempted to start portaudio twice");
    } else {

        err = Pa_Initialize();

        if (err == paNoError ) {
            current_channels = channels;
            portaudio_start_stream();
        } else {
            log_error(portaudio_log, "Could not init portaudio");
        }
    }
}

static void portaudio_stop_sampling(void)
{
    portaudio_stop_stream();
    Pa_Terminate();
}

static BYTE portaudio_get_sample(int channel)
{
    unsigned int current_frame;
    unsigned int current_cycle;
    unsigned int frame_diff;
    unsigned int frame_sample;

    if (!stream_buffer) {
        return 0x80;
    }
    current_frame = maincpu_clk / sound_cycles_per_frame;
    current_cycle = maincpu_clk % sound_cycles_per_frame;

    if (current_frame > old_frame) {
        frame_diff = current_frame - old_frame;
        while (frame_diff) {
            --frame_diff;
            ++old_frame;
            if (Pa_GetStreamReadAvailable(stream) >= sound_samples_per_frame) {
                Pa_ReadStream(stream, stream_buffer, sound_samples_per_frame);
                same_sample = 0;
            } else {
                ++same_sample;
                if (same_sample >= sound_samples_per_frame) {
                    same_sample = 0;
                    portaudio_stop_stream();
                    portaudio_start_stream();
                    log_warning(portaudio_log, "Had to restart the stream");
                }
                return old_sample;
            }
        }
    }
    frame_sample = current_cycle * sound_samples_per_frame / sound_cycles_per_frame;

    switch (channel) {
        case SAMPLER_CHANNEL_1:
            old_sample = (BYTE)((stream_buffer[frame_sample * 2] >> 8) + 0x80);
            break;
        case SAMPLER_CHANNEL_2:
            old_sample = (BYTE)((stream_buffer[(frame_sample * 2) + 1] >> 8) + 0x80);
            break;
        case SAMPLER_CHANNEL_DEFAULT:
        default:
            old_sample = (BYTE)((stream_buffer[frame_sample] >> 8) + 0x80);
            break;
    }

    return old_sample;
}

static void portaudio_shutdown(void)
{
    if (stream_started) {
        portaudio_stop_sampling();
    }
}

static sampler_device_t portaudio_device =
{
    "portaudio based hardware audio input",
    portaudio_start_sampling,
    portaudio_stop_sampling,
    portaudio_get_sample,
    portaudio_shutdown,
    NULL, /* no resources */
    NULL, /* no cmdline options */
    NULL  /* no reset */
};


void portaudio_init(void)
{
    portaudio_log = log_open("Sampler PortAudio");

    sampler_device_register(&portaudio_device, SAMPLER_DEVICE_PORTAUDIO);
}
#endif
