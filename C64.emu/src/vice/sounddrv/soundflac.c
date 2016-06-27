/*
 * soundflac.c - Implementation of the FLAC dump sound device
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

#ifdef USE_FLAC
#include <FLAC/metadata.h>
#include <FLAC/stream_encoder.h>

#include "sound.h"
#include "types.h"
#include "archdep.h"
#include "log.h"

#define PCM_BUFFER_SIZE SOUND_BUFSIZE * 4

static int stereo = 0;
static FLAC__int32 pcm_buffer[PCM_BUFFER_SIZE];
static FLAC__StreamEncoder *encoder = NULL;
static FLAC__StreamMetadata *metadata[2];
static unsigned int samples = 0;

static void progress_callback(const FLAC__StreamEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data)
{
}

static int flac_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    const char *flacname;
    FLAC__bool ok = true;
    FLAC__StreamEncoderInitStatus init_status;
    FLAC__StreamMetadata_VorbisComment_Entry entry;

    flacname = param ? param : "vicesnd.flac";

    samples = 0;

    encoder = FLAC__stream_encoder_new();
    if (!encoder) {
        return 1;
    }

    ok &= FLAC__stream_encoder_set_verify(encoder, true);
    ok &= FLAC__stream_encoder_set_compression_level(encoder, 5);
    ok &= FLAC__stream_encoder_set_channels(encoder, *channels);
    ok &= FLAC__stream_encoder_set_bits_per_sample(encoder, 16);
    ok &= FLAC__stream_encoder_set_sample_rate(encoder, *speed);

    if (ok) {
        if (
            (metadata[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)) == NULL ||
            (metadata[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)) == NULL ||
            !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "CREATOR", "VICE") ||
            !FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, false) ||
            !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "YEAR", "2016") ||
            !FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, /*copy=*/false)
        ) {
            ok = false;
        }

        metadata[1]->length = 1234;

        ok = FLAC__stream_encoder_set_metadata(encoder, metadata, 2);
    }

    if (ok) {
        init_status = FLAC__stream_encoder_init_file(encoder, flacname, progress_callback, NULL);
        if (init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
            ok = false;
        }
    }

    if (!ok) {
        FLAC__stream_encoder_finish(encoder);
        FLAC__metadata_object_delete(metadata[0]);
        FLAC__metadata_object_delete(metadata[1]);
        FLAC__stream_encoder_delete(encoder);
        return 1;
    }

    if (*channels == 2) {
        stereo = 1;
    }

    return 0;
}

static int flac_write(SWORD *pbuf, size_t nr)
{
    FLAC__bool ok;
    unsigned int i;
    unsigned int amount = (stereo == 1) ? nr / 2 : nr;

    for (i = 0; i < nr; ++i) {
        pcm_buffer[i] = (FLAC__int32)(pbuf[i]);
    }

    ok = FLAC__stream_encoder_process_interleaved(encoder, pcm_buffer, amount);

    if (!ok) {
        FLAC__stream_encoder_finish(encoder);
        FLAC__metadata_object_delete(metadata[0]);
        FLAC__metadata_object_delete(metadata[1]);
        FLAC__stream_encoder_delete(encoder);
        return 1;
    }
    samples += amount;
    return 0;
}

static void flac_close(void)
{
    FLAC__stream_encoder_set_total_samples_estimate(encoder, samples);
    FLAC__stream_encoder_finish(encoder);
    FLAC__metadata_object_delete(metadata[0]);
    FLAC__metadata_object_delete(metadata[1]);
    FLAC__stream_encoder_delete(encoder);
}

static sound_device_t flac_device =
{
    "flac",
    flac_init,
    flac_write,
    NULL,
    NULL,
    NULL,
    flac_close,
    NULL,
    NULL,
    0,
    2
};

int sound_init_flac_device(void)
{
    return sound_register_device(&flac_device);
}
#endif
