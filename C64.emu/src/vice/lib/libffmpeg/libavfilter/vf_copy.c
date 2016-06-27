/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * copy video filter
 */

#include "libavutil/imgutils.h"
#include "libavutil/internal.h"
#include "avfilter.h"
#include "internal.h"
#include "video.h"

static int filter_frame(AVFilterLink *inlink, AVFrame *in)
{
    AVFilterLink *outlink = inlink->dst->outputs[0];
    AVFrame *out = ff_get_video_buffer(outlink, in->width, in->height);

    if (!out) {
        av_frame_free(&in);
        return AVERROR(ENOMEM);
    }
    av_frame_copy_props(out, in);
    av_frame_copy(out, in);
    av_frame_free(&in);
    return ff_filter_frame(outlink, out);
}

static const AVFilterPad avfilter_vf_copy_inputs[] = {
    {
#ifdef IDE_COMPILE
        "default",
        AVMEDIA_TYPE_VIDEO,
        0, 0, 0, 0, 0, 0, 0, filter_frame,
#else
		.name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .filter_frame = filter_frame,
#endif
	},
    { NULL }
};

static const AVFilterPad avfilter_vf_copy_outputs[] = {
    {
#ifdef IDE_COMPILE
        "default",
        AVMEDIA_TYPE_VIDEO,
#else
		.name = "default",
        .type = AVMEDIA_TYPE_VIDEO,
#endif
	},
    { NULL }
};

AVFilter ff_vf_copy = {
#ifdef IDE_COMPILE
    "copy",
    NULL_IF_CONFIG_SMALL("Copy the input video unchanged to the output."),
    avfilter_vf_copy_inputs,
    avfilter_vf_copy_outputs,
#else
	.name        = "copy",
    .description = NULL_IF_CONFIG_SMALL("Copy the input video unchanged to the output."),
    .inputs      = avfilter_vf_copy_inputs,
    .outputs     = avfilter_vf_copy_outputs,
#endif
};
