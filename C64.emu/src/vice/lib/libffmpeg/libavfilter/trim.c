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

#include <float.h>
#include <math.h>
#include <stdint.h>

#ifdef IDE_COMPILE
#include "ffmpeg-config.h"
#include "ide-config.h"
#else
#include "config.h"
#endif

#include "libavutil/avassert.h"
#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/log.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavutil/samplefmt.h"

#include "audio.h"
#include "avfilter.h"
#include "internal.h"

typedef struct TrimContext {
    const AVClass *class;

    /*
     * AVOptions
     */
    int64_t duration;
    int64_t start_time, end_time;
    int64_t start_frame, end_frame;

    double duration_dbl;
    double start_time_dbl, end_time_dbl;
    /*
     * in the link timebase for video,
     * in 1/samplerate for audio
     */
    int64_t start_pts, end_pts;
    int64_t start_sample, end_sample;

    /*
     * number of video frames that arrived on this filter so far
     */
    int64_t nb_frames;
    /*
     * number of audio samples that arrived on this filter so far
     */
    int64_t nb_samples;
    /*
     * timestamp of the first frame in the output, in the timebase units
     */
    int64_t first_pts;
    /*
     * duration in the timebase units
     */
    int64_t duration_tb;

    int64_t next_pts;

    int eof;
} TrimContext;

static av_cold int init(AVFilterContext *ctx)
{
    TrimContext *s = ctx->priv;

    s->first_pts = AV_NOPTS_VALUE;

    return 0;
}

static int config_input(AVFilterLink *inlink)
{
    AVFilterContext *ctx = inlink->dst;
    TrimContext       *s = ctx->priv;
#ifdef IDE_COMPILE
	AVRational tmp;
	AVRational tb;

	tmp.num = 1;
	tmp.den = inlink->sample_rate;
	tb = (inlink->type == AVMEDIA_TYPE_VIDEO) ?
                     inlink->time_base : tmp;
#else
	AVRational tb = (inlink->type == AVMEDIA_TYPE_VIDEO) ?
                     inlink->time_base : (AVRational){ 1, inlink->sample_rate };
#endif

    if (s->start_time_dbl != DBL_MAX)
        s->start_time = s->start_time_dbl * 1e6;
    if (s->end_time_dbl != DBL_MAX)
        s->end_time = s->end_time_dbl * 1e6;
    if (s->duration_dbl != 0)
        s->duration = s->duration_dbl * 1e6;

    if (s->start_time != INT64_MAX) {
#ifdef IDE_COMPILE
		int64_t start_pts;
        tmp.num = 1;
		tmp.den = AV_TIME_BASE;
		start_pts = av_rescale_q(s->start_time, tmp, tb);
#else
		int64_t start_pts = av_rescale_q(s->start_time, AV_TIME_BASE_Q, tb);
#endif
		if (s->start_pts == AV_NOPTS_VALUE || start_pts < s->start_pts)
            s->start_pts = start_pts;
    }
    if (s->end_time != INT64_MAX) {
#ifdef IDE_COMPILE
		int64_t end_pts;
        tmp.num = 1;
		tmp.den = AV_TIME_BASE;
		end_pts = av_rescale_q(s->end_time, tmp, tb);
#else
		int64_t end_pts = av_rescale_q(s->end_time, AV_TIME_BASE_Q, tb);
#endif
		if (s->end_pts == AV_NOPTS_VALUE || end_pts > s->end_pts)
            s->end_pts = end_pts;
    }
    if (s->duration) {
#ifdef IDE_COMPILE
        tmp.num = 1;
		tmp.den = AV_TIME_BASE;
		s->duration_tb = av_rescale_q(s->duration, tmp, tb);
#else
		s->duration_tb = av_rescale_q(s->duration, AV_TIME_BASE_Q, tb);
#endif
	}
    return 0;
}

static int config_output(AVFilterLink *outlink)
{
    outlink->flags |= FF_LINK_FLAG_REQUEST_LOOP;
    return 0;
}

#define OFFSET(x) offsetof(TrimContext, x)
#ifdef IDE_COMPILE
#define COMMON_OPTS                                                                                                                                                         \
    { "starti", "Timestamp of the first frame that should be passed", OFFSET(start_time), AV_OPT_TYPE_DURATION, {INT64_MAX}, INT64_MIN, INT64_MAX, FLAGS }, \
    { "endi", "Timestamp of the first frame that should be dropped again", OFFSET(end_time), AV_OPT_TYPE_DURATION, {INT64_MAX}, INT64_MIN, INT64_MAX, FLAGS }, \
    { "start_pts", "Timestamp of the first frame that should be passed", OFFSET(start_pts), AV_OPT_TYPE_INT64, {AV_NOPTS_VALUE}, INT64_MIN, INT64_MAX, FLAGS }, \
    { "end_pts", "Timestamp of the first frame that should be dropped again", OFFSET(end_pts), AV_OPT_TYPE_INT64, {AV_NOPTS_VALUE}, INT64_MIN, INT64_MAX, FLAGS }, \
    { "durationi", "Maximum duration of the output", OFFSET(duration), AV_OPT_TYPE_DURATION, {0}, 0, INT64_MAX, FLAGS },
#else
#define COMMON_OPTS                                                                                                                                                         \
    { "starti",      "Timestamp of the first frame that "                                                                                                        \
        "should be passed",                                              OFFSET(start_time),  AV_OPT_TYPE_DURATION, { .i64 = INT64_MAX },    INT64_MIN, INT64_MAX, FLAGS }, \
    { "endi",        "Timestamp of the first frame that "                                                                                                        \
        "should be dropped again",                                       OFFSET(end_time),    AV_OPT_TYPE_DURATION, { .i64 = INT64_MAX },    INT64_MIN, INT64_MAX, FLAGS }, \
    { "start_pts",   "Timestamp of the first frame that should be "                                                                                                         \
       " passed",                                                        OFFSET(start_pts),   AV_OPT_TYPE_INT64,  { .i64 = AV_NOPTS_VALUE }, INT64_MIN, INT64_MAX, FLAGS }, \
    { "end_pts",     "Timestamp of the first frame that should be "                                                                                                         \
        "dropped again",                                                 OFFSET(end_pts),     AV_OPT_TYPE_INT64,  { .i64 = AV_NOPTS_VALUE }, INT64_MIN, INT64_MAX, FLAGS }, \
    { "durationi",   "Maximum duration of the output",                   OFFSET(duration),    AV_OPT_TYPE_DURATION, { .i64 = 0 },                    0, INT64_MAX, FLAGS },
#endif

#ifdef IDE_COMPILE
#define COMPAT_OPTS \
    { "start", "Timestamp in seconds of the first frame that should be passed", OFFSET(start_time_dbl), AV_OPT_TYPE_DOUBLE, {DBL_MAX}, -DBL_MAX, DBL_MAX, FLAGS }, \
    { "end", "Timestamp in seconds of the first frame that should be dropped again", OFFSET(end_time_dbl), AV_OPT_TYPE_DOUBLE, {DBL_MAX}, -DBL_MAX, DBL_MAX, FLAGS }, \
    { "duration", "Maximum duration of the output in seconds", OFFSET(duration_dbl), AV_OPT_TYPE_DOUBLE, {0}, 0, DBL_MAX, FLAGS },
#else
#define COMPAT_OPTS \
    { "start",       "Timestamp in seconds of the first frame that "                                                                                                        \
        "should be passed",                                              OFFSET(start_time_dbl),AV_OPT_TYPE_DOUBLE, { .dbl = DBL_MAX },       -DBL_MAX, DBL_MAX,     FLAGS }, \
    { "end",         "Timestamp in seconds of the first frame that "                                                                                                        \
        "should be dropped again",                                       OFFSET(end_time_dbl),  AV_OPT_TYPE_DOUBLE, { .dbl = DBL_MAX },       -DBL_MAX, DBL_MAX,     FLAGS }, \
    { "duration",    "Maximum duration of the output in seconds",        OFFSET(duration_dbl),  AV_OPT_TYPE_DOUBLE, { .dbl = 0 },                      0,   DBL_MAX, FLAGS },
#endif

#if CONFIG_TRIM_FILTER
static int trim_filter_frame(AVFilterLink *inlink, AVFrame *frame)
{
    AVFilterContext *ctx = inlink->dst;
    TrimContext       *s = ctx->priv;
    int drop;

    /* drop everything if EOF has already been returned */
    if (s->eof) {
        av_frame_free(&frame);
        return 0;
    }

    if (s->start_frame >= 0 || s->start_pts != AV_NOPTS_VALUE) {
        drop = 1;
        if (s->start_frame >= 0 && s->nb_frames >= s->start_frame)
            drop = 0;
        if (s->start_pts != AV_NOPTS_VALUE && frame->pts != AV_NOPTS_VALUE &&
            frame->pts >= s->start_pts)
            drop = 0;
        if (drop)
            goto drop;
    }

    if (s->first_pts == AV_NOPTS_VALUE && frame->pts != AV_NOPTS_VALUE)
        s->first_pts = frame->pts;

    if (s->end_frame != INT64_MAX || s->end_pts != AV_NOPTS_VALUE || s->duration_tb) {
        drop = 1;

        if (s->end_frame != INT64_MAX && s->nb_frames < s->end_frame)
            drop = 0;
        if (s->end_pts != AV_NOPTS_VALUE && frame->pts != AV_NOPTS_VALUE &&
            frame->pts < s->end_pts)
            drop = 0;
        if (s->duration_tb && frame->pts != AV_NOPTS_VALUE &&
            frame->pts - s->first_pts < s->duration_tb)
            drop = 0;

        if (drop) {
            s->eof = inlink->closed = 1;
            goto drop;
        }
    }

    s->nb_frames++;

    return ff_filter_frame(ctx->outputs[0], frame);

drop:
    s->nb_frames++;
    av_frame_free(&frame);
    return 0;
}

#define FLAGS AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_FILTERING_PARAM
static const AVOption trim_options[] = {
    COMMON_OPTS
#ifdef IDE_COMPILE
	{ "start_frame", "Number of the first frame that should be passed to the output", OFFSET(start_frame), AV_OPT_TYPE_INT64, {-1}, -1, INT64_MAX, FLAGS },
    { "end_frame", "Number of the first frame that should be dropped again", OFFSET(end_frame), AV_OPT_TYPE_INT64, {INT64_MAX}, 0, INT64_MAX, FLAGS },
#else
	{ "start_frame", "Number of the first frame that should be passed "
        "to the output",                                                 OFFSET(start_frame), AV_OPT_TYPE_INT64,  { .i64 = -1 },       -1, INT64_MAX, FLAGS },
    { "end_frame",   "Number of the first frame that should be dropped "
        "again",                                                         OFFSET(end_frame),   AV_OPT_TYPE_INT64,  { .i64 = INT64_MAX }, 0, INT64_MAX, FLAGS },
#endif
	COMPAT_OPTS
    { NULL }
};
#undef FLAGS

AVFILTER_DEFINE_CLASS(trim);

static const AVFilterPad trim_inputs[] = {
    {
#ifdef IDE_COMPILE
        "default",
        AVMEDIA_TYPE_VIDEO,
        0, 0, 0, 0, 0, 0, 0, trim_filter_frame,
        0, 0, config_input,
#else
		.name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .filter_frame = trim_filter_frame,
        .config_props = config_input,
#endif
	},
    { NULL }
};

static const AVFilterPad trim_outputs[] = {
    {
#ifdef IDE_COMPILE
        "default",
        AVMEDIA_TYPE_VIDEO,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, config_output,
#else
		.name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .config_props = config_output,
#endif
	},
    { NULL }
};

AVFilter ff_vf_trim = {
#ifdef IDE_COMPILE
    "trim",
    NULL_IF_CONFIG_SMALL("Pick one continuous section from the input, drop the rest."),
    trim_inputs,
    trim_outputs,
    &trim_class,
    0, init,
    0, 0, 0, sizeof(TrimContext),
#else
	.name        = "trim",
    .description = NULL_IF_CONFIG_SMALL("Pick one continuous section from the input, drop the rest."),
    .init        = init,
    .priv_size   = sizeof(TrimContext),
    .priv_class  = &trim_class,
    .inputs      = trim_inputs,
    .outputs     = trim_outputs,
#endif
};
#endif // CONFIG_TRIM_FILTER

#if CONFIG_ATRIM_FILTER
static int atrim_filter_frame(AVFilterLink *inlink, AVFrame *frame)
{
    AVFilterContext *ctx = inlink->dst;
    TrimContext       *s = ctx->priv;
    int64_t start_sample, end_sample;
    int64_t pts;
    int drop;

    /* drop everything if EOF has already been returned */
    if (s->eof) {
        av_frame_free(&frame);
        return 0;
    }

    if (frame->pts != AV_NOPTS_VALUE) {
#ifdef IDE_COMPILE
        AVRational tmp;

		tmp.num = 1;
		tmp.den = inlink->sample_rate;
		pts = av_rescale_q(frame->pts, inlink->time_base, tmp);
#else
		pts = av_rescale_q(frame->pts, inlink->time_base,
                           (AVRational){ 1, inlink->sample_rate });
#endif
	} else
        pts = s->next_pts;
    s->next_pts = pts + frame->nb_samples;

    /* check if at least a part of the frame is after the start time */
    if (s->start_sample < 0 && s->start_pts == AV_NOPTS_VALUE) {
        start_sample = 0;
    } else {
        drop = 1;
        start_sample = frame->nb_samples;

        if (s->start_sample >= 0 &&
            s->nb_samples + frame->nb_samples > s->start_sample) {
            drop         = 0;
            start_sample = FFMIN(start_sample, s->start_sample - s->nb_samples);
        }

        if (s->start_pts != AV_NOPTS_VALUE && pts != AV_NOPTS_VALUE &&
            pts + frame->nb_samples > s->start_pts) {
            drop = 0;
            start_sample = FFMIN(start_sample, s->start_pts - pts);
        }

        if (drop)
            goto drop;
    }

    if (s->first_pts == AV_NOPTS_VALUE)
        s->first_pts = pts + start_sample;

    /* check if at least a part of the frame is before the end time */
    if (s->end_sample == INT64_MAX && s->end_pts == AV_NOPTS_VALUE && !s->duration_tb) {
        end_sample = frame->nb_samples;
    } else {
        drop       = 1;
        end_sample = 0;

        if (s->end_sample != INT64_MAX &&
            s->nb_samples < s->end_sample) {
            drop       = 0;
            end_sample = FFMAX(end_sample, s->end_sample - s->nb_samples);
        }

        if (s->end_pts != AV_NOPTS_VALUE && pts != AV_NOPTS_VALUE &&
            pts < s->end_pts) {
            drop       = 0;
            end_sample = FFMAX(end_sample, s->end_pts - pts);
        }

        if (s->duration_tb && pts - s->first_pts < s->duration_tb) {
            drop       = 0;
            end_sample = FFMAX(end_sample, s->first_pts + s->duration_tb - pts);
        }

        if (drop) {
            s->eof = inlink->closed = 1;
            goto drop;
        }
    }

    s->nb_samples += frame->nb_samples;
    start_sample   = FFMAX(0, start_sample);
    end_sample     = FFMIN(frame->nb_samples, end_sample);
    av_assert0(start_sample < end_sample || (start_sample == end_sample && !frame->nb_samples));

    if (start_sample) {
        AVFrame *out = ff_get_audio_buffer(ctx->outputs[0], end_sample - start_sample);
        if (!out) {
            av_frame_free(&frame);
            return AVERROR(ENOMEM);
        }

        av_frame_copy_props(out, frame);
        av_samples_copy(out->extended_data, frame->extended_data, 0, start_sample,
                        out->nb_samples, inlink->channels,
                        frame->format);
        if (out->pts != AV_NOPTS_VALUE) {
#ifdef IDE_COMPILE
			AVRational tmp;

			tmp.num = 1;
			tmp.den = out->sample_rate;
			out->pts += av_rescale_q(start_sample, tmp,
                                     inlink->time_base);
#else
			out->pts += av_rescale_q(start_sample, (AVRational){ 1, out->sample_rate },
                                     inlink->time_base);
#endif
		}
        av_frame_free(&frame);
        frame = out;
    } else
        frame->nb_samples = end_sample;

    return ff_filter_frame(ctx->outputs[0], frame);

drop:
    s->nb_samples += frame->nb_samples;
    av_frame_free(&frame);
    return 0;
}

#define FLAGS AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_FILTERING_PARAM
static const AVOption atrim_options[] = {
    COMMON_OPTS
#ifdef IDE_COMPILE
	{ "start_sample", "Number of the first audio sample that should be passed to the output", OFFSET(start_sample), AV_OPT_TYPE_INT64, {-1}, -1, INT64_MAX, FLAGS },
    { "end_sample", "Number of the first audio sample that should be dropped again", OFFSET(end_sample), AV_OPT_TYPE_INT64, {INT64_MAX}, 0, INT64_MAX, FLAGS },
#else
	{ "start_sample", "Number of the first audio sample that should be "
        "passed to the output",                                          OFFSET(start_sample), AV_OPT_TYPE_INT64,  { .i64 = -1 },       -1, INT64_MAX, FLAGS },
    { "end_sample",   "Number of the first audio sample that should be "
        "dropped again",                                                 OFFSET(end_sample),   AV_OPT_TYPE_INT64,  { .i64 = INT64_MAX }, 0, INT64_MAX, FLAGS },
#endif
	COMPAT_OPTS
    { NULL }
};
#undef FLAGS

AVFILTER_DEFINE_CLASS(atrim);

static const AVFilterPad atrim_inputs[] = {
    {
#ifdef IDE_COMPILE
        "default",
        AVMEDIA_TYPE_AUDIO,
        0, 0, 0, 0, 0, 0, 0, atrim_filter_frame,
        0, 0, config_input,
#else
		.name         = "default",
        .type         = AVMEDIA_TYPE_AUDIO,
        .filter_frame = atrim_filter_frame,
        .config_props = config_input,
#endif
	},
    { NULL }
};

static const AVFilterPad atrim_outputs[] = {
    {
#ifdef IDE_COMPILE
        "default",
        AVMEDIA_TYPE_AUDIO,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, config_output,
#else
		.name         = "default",
        .type         = AVMEDIA_TYPE_AUDIO,
        .config_props = config_output,
#endif
	},
    { NULL }
};

AVFilter ff_af_atrim = {
#ifdef IDE_COMPILE
    "atrim",
    NULL_IF_CONFIG_SMALL("Pick one continuous section from the input, drop the rest."),
    atrim_inputs,
    atrim_outputs,
    &atrim_class,
    0, init,
    0, 0, 0, sizeof(TrimContext),
#else
	.name        = "atrim",
    .description = NULL_IF_CONFIG_SMALL("Pick one continuous section from the input, drop the rest."),
    .init        = init,
    .priv_size   = sizeof(TrimContext),
    .priv_class  = &atrim_class,
    .inputs      = atrim_inputs,
    .outputs     = atrim_outputs,
#endif
};
#endif // CONFIG_ATRIM_FILTER
