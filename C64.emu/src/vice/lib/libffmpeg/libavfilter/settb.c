/*
 * Copyright (c) 2010 Stefano Sabatini
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
 * @file
 * Set timebase for the output link.
 */

#include <inttypes.h>
#include <stdio.h>

#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/internal.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavutil/rational.h"
#include "audio.h"
#include "avfilter.h"
#include "internal.h"
#include "video.h"

static const char *const var_names[] = {
    "AVTB",   /* default timebase 1/AV_TIME_BASE */
    "intb",   /* input timebase */
    "sr",     /* sample rate */
    NULL
};

enum var_name {
    VAR_AVTB,
    VAR_INTB,
    VAR_SR,
    VAR_VARS_NB
};

typedef struct SetTBContext {
    const AVClass *class;
    char *tb_expr;
    double var_values[VAR_VARS_NB];
} SetTBContext;

#define OFFSET(x) offsetof(SetTBContext, x)

#ifdef IDE_COMPILE
#define DEFINE_OPTIONS(filt_name, filt_type)                                               \
static const AVOption filt_name##_options[] = {                                            \
    { "expr", "set expression determining the output timebase", OFFSET(tb_expr), AV_OPT_TYPE_STRING, {(intptr_t) "intb"}, 0, 0, \
           AV_OPT_FLAG_##filt_type##_PARAM|AV_OPT_FLAG_FILTERING_PARAM },           \
    { "tb",   "set expression determining the output timebase", OFFSET(tb_expr), AV_OPT_TYPE_STRING, {(intptr_t) "intb"}, 0, 0, \
           AV_OPT_FLAG_##filt_type##_PARAM|AV_OPT_FLAG_FILTERING_PARAM },           \
    { NULL }                                                                               \
}
#else
#define DEFINE_OPTIONS(filt_name, filt_type)                                               \
static const AVOption filt_name##_options[] = {                                            \
    { "expr", "set expression determining the output timebase", OFFSET(tb_expr), AV_OPT_TYPE_STRING, {.str="intb"}, \
           .flags=AV_OPT_FLAG_##filt_type##_PARAM|AV_OPT_FLAG_FILTERING_PARAM },           \
    { "tb",   "set expression determining the output timebase", OFFSET(tb_expr), AV_OPT_TYPE_STRING, {.str="intb"}, \
           .flags=AV_OPT_FLAG_##filt_type##_PARAM|AV_OPT_FLAG_FILTERING_PARAM },           \
    { NULL }                                                                               \
}
#endif

static int config_output_props(AVFilterLink *outlink)
{
    AVFilterContext *ctx = outlink->src;
    SetTBContext *settb = ctx->priv;
    AVFilterLink *inlink = ctx->inputs[0];
    AVRational time_base;
    int ret;
    double res;
#ifdef IDE_COMPILE
	AVRational tmp;

    tmp.num = 1;
    tmp.den = AV_TIME_BASE;
    settb->var_values[VAR_AVTB] = av_q2d(tmp);
#else
    settb->var_values[VAR_AVTB] = av_q2d(AV_TIME_BASE_Q);
#endif

	settb->var_values[VAR_INTB] = av_q2d(inlink->time_base);
    settb->var_values[VAR_SR]   = inlink->sample_rate;

    outlink->w = inlink->w;
    outlink->h = inlink->h;

    if ((ret = av_expr_parse_and_eval(&res, settb->tb_expr, var_names, settb->var_values,
                                      NULL, NULL, NULL, NULL, NULL, 0, NULL)) < 0) {
        av_log(ctx, AV_LOG_ERROR, "Invalid expression '%s' for timebase.\n", settb->tb_expr);
        return ret;
    }
    time_base = av_d2q(res, INT_MAX);
    if (time_base.num <= 0 || time_base.den <= 0) {
        av_log(ctx, AV_LOG_ERROR,
               "Invalid non-positive values for the timebase num:%d or den:%d.\n",
               time_base.num, time_base.den);
        return AVERROR(EINVAL);
    }

    outlink->time_base = time_base;
    av_log(outlink->src, AV_LOG_VERBOSE, "tb:%d/%d -> tb:%d/%d\n",
           inlink ->time_base.num, inlink ->time_base.den,
           outlink->time_base.num, outlink->time_base.den);

    return 0;
}

static int filter_frame(AVFilterLink *inlink, AVFrame *frame)
{
    AVFilterContext *ctx = inlink->dst;
    AVFilterLink *outlink = ctx->outputs[0];

    if (av_cmp_q(inlink->time_base, outlink->time_base)) {
        int64_t orig_pts = frame->pts;
        frame->pts = av_rescale_q(frame->pts, inlink->time_base, outlink->time_base);
        av_log(ctx, AV_LOG_DEBUG, "tb:%d/%d pts:%"PRId64" -> tb:%d/%d pts:%"PRId64"\n",
               inlink ->time_base.num, inlink ->time_base.den, orig_pts,
               outlink->time_base.num, outlink->time_base.den, frame->pts);
    }

    return ff_filter_frame(outlink, frame);
}

#if CONFIG_SETTB_FILTER

DEFINE_OPTIONS(settb, VIDEO);
AVFILTER_DEFINE_CLASS(settb);

static const AVFilterPad avfilter_vf_settb_inputs[] = {
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

static const AVFilterPad avfilter_vf_settb_outputs[] = {
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

AVFilter ff_vf_settb = {
#ifdef IDE_COMPILE
    "settb",
    NULL_IF_CONFIG_SMALL("Set timebase for the video output link."),
    avfilter_vf_settb_inputs,
    avfilter_vf_settb_outputs,
    &settb_class,
    0, 0, 0, 0, 0, sizeof(SetTBContext),
#else
	.name        = "settb",
    .description = NULL_IF_CONFIG_SMALL("Set timebase for the video output link."),
    .priv_size   = sizeof(SetTBContext),
    .priv_class  = &settb_class,
    .inputs      = avfilter_vf_settb_inputs,
    .outputs     = avfilter_vf_settb_outputs,
#endif
};
#endif /* CONFIG_SETTB_FILTER */

#if CONFIG_ASETTB_FILTER

DEFINE_OPTIONS(asettb, AUDIO);
AVFILTER_DEFINE_CLASS(asettb);

static const AVFilterPad avfilter_af_asettb_inputs[] = {
    {
#ifdef IDE_COMPILE
        "default",
        AVMEDIA_TYPE_AUDIO,
        0, 0, 0, 0, 0, 0, 0, filter_frame,
#else
		.name         = "default",
        .type         = AVMEDIA_TYPE_AUDIO,
        .filter_frame = filter_frame,
#endif
	},
    { NULL }
};

static const AVFilterPad avfilter_af_asettb_outputs[] = {
    {
#ifdef IDE_COMPILE
        "default",
        AVMEDIA_TYPE_AUDIO,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, config_output_props,
#else
		.name         = "default",
        .type         = AVMEDIA_TYPE_AUDIO,
        .config_props = config_output_props,
#endif
	},
    { NULL }
};

AVFilter ff_af_asettb = {
#ifdef IDE_COMPILE
    "asettb",
    NULL_IF_CONFIG_SMALL("Set timebase for the audio output link."),
    avfilter_af_asettb_inputs,
    avfilter_af_asettb_outputs,
    &asettb_class,
    0, 0, 0, 0, 0, sizeof(SetTBContext),
#else
	.name        = "asettb",
    .description = NULL_IF_CONFIG_SMALL("Set timebase for the audio output link."),
    .priv_size   = sizeof(SetTBContext),
    .inputs      = avfilter_af_asettb_inputs,
    .outputs     = avfilter_af_asettb_outputs,
    .priv_class  = &asettb_class,
#endif
};
#endif /* CONFIG_ASETTB_FILTER */
