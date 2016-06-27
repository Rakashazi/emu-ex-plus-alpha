/*
 * soundvorbis.c - Implementation of the ogg/vorbis dump sound device
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

#ifdef USE_VORBIS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <vorbis/vorbisenc.h>

#include "sound.h"
#include "types.h"
#include "archdep.h"
#include "log.h"

static int stereo = 0;
static FILE *vorbis_fd = NULL;
static vorbis_dsp_state vd;
static vorbis_block vb;
static ogg_stream_state os;
static ogg_packet op;
static vorbis_info vi;
static vorbis_comment vc;
static ogg_page og;

static int vorbis_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;
    int eos = 0;
    int ret;
    int result;

    vorbis_fd = fopen(param ? param : "vicesnd.ogg", MODE_WRITE);
    if (!vorbis_fd) {
        return 1;
    }

    if (*channels == 2) {
        stereo = 1;
    }

    vorbis_info_init(&vi);
    ret = vorbis_encode_init_vbr(&vi, *channels, *speed, 0.1);
    if (ret) {
        vorbis_info_clear(&vi);
        return -1;
    }

    vorbis_comment_init(&vc);
    vorbis_comment_add_tag(&vc, "ENCODER", "VICE");

    vorbis_analysis_init(&vd, &vi);
    vorbis_block_init(&vd, &vb);

    srand(time(NULL));
    ogg_stream_init(&os, rand());

    vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
    ogg_stream_packetin(&os, &header);
    ogg_stream_packetin(&os, &header_comm);
    ogg_stream_packetin(&os, &header_code);

    while (!eos) {
        result = ogg_stream_flush(&os, &og);
        if (!result) {
            break;
        }
        fwrite(og.header, 1, og.header_len, vorbis_fd);
        fwrite(og.body, 1, og.body_len, vorbis_fd);
    }

    return 0;
}

static int vorbis_write(SWORD *pbuf, size_t nr)
{
    float **buffer;
    unsigned int i;
    unsigned int amount = (stereo) ? nr / 2 : nr;
    int result;
    int eos = 0;

    buffer = vorbis_analysis_buffer(&vd, amount);
    for (i = 0; i < amount; i++) {
        if (stereo == 1) {
            buffer[0][i]= pbuf[i * 2] / 32768.f;
            buffer[1][i]= pbuf[(i * 2) + 1] / 32768.f;
        } else {
            buffer[0][i]= pbuf[i] / 32768.f;
        }
    }

    vorbis_analysis_wrote(&vd, i);

    while (vorbis_analysis_blockout(&vd, &vb) == 1) {
        vorbis_analysis(&vb, NULL);
        vorbis_bitrate_addblock(&vb);
        while (vorbis_bitrate_flushpacket(&vd, &op)) {
            ogg_stream_packetin(&os, &op);
            while(!eos) {
                result = ogg_stream_pageout(&os, &og);
                if (!result) {
                    break;
                }
                fwrite(og.header, 1, og.header_len, vorbis_fd);
                fwrite(og.body, 1, og.body_len, vorbis_fd);
                if (ogg_page_eos(&og)) {
                    eos = 1;
                }
            }
        }
    }
    return 0;
}

static void vorbis_close(void)
{
    ogg_stream_clear(&os);
    vorbis_block_clear(&vb);
    vorbis_dsp_clear(&vd);
    vorbis_comment_clear(&vc);
    vorbis_info_clear(&vi);

    fclose(vorbis_fd);
    vorbis_fd = NULL;
}

static sound_device_t vorbis_device =
{
    "ogg",
    vorbis_init,
    vorbis_write,
    NULL,
    NULL,
    NULL,
    vorbis_close,
    NULL,
    NULL,
    0,
    2
};

int sound_init_vorbis_device(void)
{
    return sound_register_device(&vorbis_device);
}
#endif
