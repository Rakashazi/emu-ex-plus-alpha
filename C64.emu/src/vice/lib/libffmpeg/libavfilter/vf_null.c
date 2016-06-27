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
 * null video filter
 */

#include "libavutil/internal.h"
#include "avfilter.h"
#include "internal.h"
#include "video.h"

static const AVFilterPad avfilter_vf_null_inputs[] = {
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

static const AVFilterPad avfilter_vf_null_outputs[] = {
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

AVFilter ff_vf_null = {
#ifdef IDE_COMPILE
    "null",
    NULL_IF_CONFIG_SMALL("Pass the source unchanged to the output."),
    avfilter_vf_null_inputs,
    avfilter_vf_null_outputs,
#else
	.name        = "null",
    .description = NULL_IF_CONFIG_SMALL("Pass the source unchanged to the output."),
    .inputs      = avfilter_vf_null_inputs,
    .outputs     = avfilter_vf_null_outputs,
#endif
};
