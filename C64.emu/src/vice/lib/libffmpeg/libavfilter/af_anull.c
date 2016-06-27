/*
 * Copyright (c) 2010 S.N. Hemanth Meenakshisundaram <smeenaks@ucsd.edu>
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
 * null audio filter
 */

#include "audio.h"
#include "avfilter.h"
#include "internal.h"
#include "libavutil/internal.h"

static const AVFilterPad avfilter_af_anull_inputs[] = {
    {
#ifdef IDE_COMPILE
        "default",
        AVMEDIA_TYPE_AUDIO,
#else
		.name = "default",
        .type = AVMEDIA_TYPE_AUDIO,
#endif
	},
    { NULL }
};

static const AVFilterPad avfilter_af_anull_outputs[] = {
    {
#ifdef IDE_COMPILE
        "default",
        AVMEDIA_TYPE_AUDIO,
#else
		.name = "default",
        .type = AVMEDIA_TYPE_AUDIO,
#endif
	},
    { NULL }
};

AVFilter ff_af_anull = {
#ifdef IDE_COMPILE
    "anull",
    NULL_IF_CONFIG_SMALL("Pass the source unchanged to the output."),
    avfilter_af_anull_inputs,
    avfilter_af_anull_outputs,
    0, 0, 0, 0, 0, ff_query_formats_all,
#else
	.name          = "anull",
    .description   = NULL_IF_CONFIG_SMALL("Pass the source unchanged to the output."),
    .query_formats = ff_query_formats_all,
    .inputs        = avfilter_af_anull_inputs,
    .outputs       = avfilter_af_anull_outputs,
#endif
};
