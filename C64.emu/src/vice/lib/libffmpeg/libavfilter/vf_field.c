/*
 * Copyright (c) 2003 Rich Felker
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
 * @file
 * field filter, based on libmpcodecs/vf_field.c by Rich Felker
 */

#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "avfilter.h"
#include "internal.h"

enum FieldType { FIELD_TYPE_TOP = 0, FIELD_TYPE_BOTTOM };

typedef struct {
    const AVClass *class;
    enum FieldType type;
    int nb_planes;              ///< number of planes of the current format
} FieldContext;

#define OFFSET(x) offsetof(FieldContext, x)
#define FLAGS AV_OPT_FLAG_FILTERING_PARAM|AV_OPT_FLAG_VIDEO_PARAM

static const AVOption field_options[] = {
#ifdef IDE_COMPILE
	{"type", "set field type (top or bottom)", OFFSET(type), AV_OPT_TYPE_INT, {FIELD_TYPE_TOP}, 0, 1, FLAGS, "field_type" },
    {"top", "select top field", 0, AV_OPT_TYPE_CONST, {FIELD_TYPE_TOP}, INT_MIN, INT_MAX, FLAGS, "field_type"},
    {"bottom", "select bottom field", 0, AV_OPT_TYPE_CONST, {FIELD_TYPE_BOTTOM}, INT_MIN, INT_MAX, FLAGS, "field_type"},
#else
	{"type", "set field type (top or bottom)", OFFSET(type), AV_OPT_TYPE_INT, {.i64=FIELD_TYPE_TOP}, 0, 1, FLAGS, "field_type" },
    {"top",    "select top field",    0, AV_OPT_TYPE_CONST, {.i64=FIELD_TYPE_TOP},    INT_MIN, INT_MAX, FLAGS, "field_type"},
    {"bottom", "select bottom field", 0, AV_OPT_TYPE_CONST, {.i64=FIELD_TYPE_BOTTOM}, INT_MIN, INT_MAX, FLAGS, "field_type"},
#endif
	{NULL}
};

AVFILTER_DEFINE_CLASS(field);

static int config_props_output(AVFilterLink *outlink)
{
    AVFilterContext *ctx = outlink->src;
    FieldContext *field = ctx->priv;
    AVFilterLink *inlink = ctx->inputs[0];

    field->nb_planes = av_pix_fmt_count_planes(outlink->format);

    outlink->w = inlink->w;
    outlink->h = (inlink->h + (field->type == FIELD_TYPE_TOP)) / 2;

    av_log(ctx, AV_LOG_VERBOSE, "w:%d h:%d type:%s -> w:%d h:%d\n",
           inlink->w, inlink->h, field->type == FIELD_TYPE_BOTTOM ? "bottom" : "top",
           outlink->w, outlink->h);
    return 0;
}

static int filter_frame(AVFilterLink *inlink, AVFrame *inpicref)
{
    FieldContext *field = inlink->dst->priv;
    AVFilterLink *outlink = inlink->dst->outputs[0];
    int i;

    inpicref->height = outlink->h;
    inpicref->interlaced_frame = 0;

    for (i = 0; i < field->nb_planes; i++) {
        if (field->type == FIELD_TYPE_BOTTOM)
            inpicref->data[i] = inpicref->data[i] + inpicref->linesize[i];
        inpicref->linesize[i] = 2 * inpicref->linesize[i];
    }
    return ff_filter_frame(outlink, inpicref);
}

static const AVFilterPad field_inputs[] = {
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

static const AVFilterPad field_outputs[] = {
    {
#ifdef IDE_COMPILE
        "default",
        AVMEDIA_TYPE_VIDEO,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, config_props_output,
#else
	.name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .config_props = config_props_output,
#endif
	},
    { NULL }
};

AVFilter ff_vf_field = {
#ifdef IDE_COMPILE
    "field",
    NULL_IF_CONFIG_SMALL("Extract a field from the input video."),
    field_inputs,
    field_outputs,
    &field_class,
    0, 0, 0, 0, 0, sizeof(FieldContext),
#else
	.name        = "field",
    .description = NULL_IF_CONFIG_SMALL("Extract a field from the input video."),
    .priv_size   = sizeof(FieldContext),
    .inputs      = field_inputs,
    .outputs     = field_outputs,
    .priv_class  = &field_class,
#endif
};
