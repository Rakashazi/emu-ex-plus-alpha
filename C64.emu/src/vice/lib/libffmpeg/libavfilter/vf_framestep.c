/*
 * Copyright (c) 2012 Stefano Sabatini
 *
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
 * @file framestep filter, inspired on libmpcodecs/vf_framestep.c by
 * Daniele Fornighieri <guru AT digitalfantasy it>.
 */

#include "libavutil/opt.h"
#include "avfilter.h"
#include "internal.h"
#include "video.h"

typedef struct NullContext {
    const AVClass *class;
    int frame_step;
} FrameStepContext;

#define OFFSET(x) offsetof(FrameStepContext, x)
#define FLAGS AV_OPT_FLAG_FILTERING_PARAM|AV_OPT_FLAG_VIDEO_PARAM

static const AVOption framestep_options[] = {
#ifdef IDE_COMPILE
	{ "step", "set frame step", OFFSET(frame_step), AV_OPT_TYPE_INT, {1}, 1, INT_MAX, FLAGS},
#else
	{ "step", "set frame step",  OFFSET(frame_step), AV_OPT_TYPE_INT, {.i64=1}, 1, INT_MAX, FLAGS},
#endif
	{ NULL },
};

AVFILTER_DEFINE_CLASS(framestep);

static int config_output_props(AVFilterLink *outlink)
{
    AVFilterContext *ctx = outlink->src;
    FrameStepContext *framestep = ctx->priv;
    AVFilterLink *inlink = ctx->inputs[0];
#ifdef IDE_COMPILE
    AVRational tmp;
#endif

    outlink->flags |= FF_LINK_FLAG_REQUEST_LOOP;
#ifdef IDE_COMPILE
	tmp.num = framestep->frame_step;
	tmp.den = 1;
	outlink->frame_rate = av_div_q(inlink->frame_rate, tmp);
#else
	outlink->frame_rate =
        av_div_q(inlink->frame_rate, (AVRational){framestep->frame_step, 1});
#endif

    av_log(ctx, AV_LOG_VERBOSE, "step:%d frame_rate:%d/%d(%f) -> frame_rate:%d/%d(%f)\n",
           framestep->frame_step,
           inlink->frame_rate.num, inlink->frame_rate.den, av_q2d(inlink->frame_rate),
           outlink->frame_rate.num, outlink->frame_rate.den, av_q2d(outlink->frame_rate));
    return 0;
}

static int filter_frame(AVFilterLink *inlink, AVFrame *ref)
{
    FrameStepContext *framestep = inlink->dst->priv;

    if (!(inlink->frame_count % framestep->frame_step)) {
        return ff_filter_frame(inlink->dst->outputs[0], ref);
    } else {
        av_frame_free(&ref);
        return 0;
    }
}

static const AVFilterPad framestep_inputs[] = {
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

static const AVFilterPad framestep_outputs[] = {
    {
#ifdef IDE_COMPILE
        "default",
        AVMEDIA_TYPE_VIDEO,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, config_output_props,
#else
		.name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .config_props = config_output_props,
#endif
	},
    { NULL }
};

AVFilter ff_vf_framestep = {
#ifdef IDE_COMPILE
    "framestep",
    NULL_IF_CONFIG_SMALL("Select one frame every N frames."),
    framestep_inputs,
    framestep_outputs,
    &framestep_class,
    AVFILTER_FLAG_SUPPORT_TIMELINE_GENERIC,
    0, 0, 0, 0, sizeof(FrameStepContext),
#else
	.name        = "framestep",
    .description = NULL_IF_CONFIG_SMALL("Select one frame every N frames."),
    .priv_size   = sizeof(FrameStepContext),
    .priv_class  = &framestep_class,
    .inputs      = framestep_inputs,
    .outputs     = framestep_outputs,
    .flags       = AVFILTER_FLAG_SUPPORT_TIMELINE_GENERIC,
#endif
};
