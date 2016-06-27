/*
 * audio resampling
 * Copyright (c) 2004-2012 Michael Niedermayer <michaelni@gmx.at>
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
 * audio resampling
 * @author Michael Niedermayer <michaelni@gmx.at>
 */

#include "libavutil/x86/cpu.h"
#include "libswresample/resample.h"

#define RESAMPLE_FUNCS(type, opt) \
int ff_resample_common_##type##_##opt(ResampleContext *c, void *dst, \
                                      const void *src, int sz, int upd); \
int ff_resample_linear_##type##_##opt(ResampleContext *c, void *dst, \
                                      const void *src, int sz, int upd)

RESAMPLE_FUNCS(int16,  mmxext);
RESAMPLE_FUNCS(int16,  sse2);
RESAMPLE_FUNCS(int16,  xop);
RESAMPLE_FUNCS(float,  sse);
RESAMPLE_FUNCS(float,  avx);
RESAMPLE_FUNCS(float,  fma3);
RESAMPLE_FUNCS(float,  fma4);
RESAMPLE_FUNCS(double, sse2);

void swri_resample_dsp_x86_init(ResampleContext *c)
{
    int av_unused mm_flags = av_get_cpu_flags();

    switch(c->format){
    case AV_SAMPLE_FMT_S16P:
#if (ARCH_X86_32 == 1) && (HAVE_MMXEXT_EXTERNAL == 1)
        if (ARCH_X86_32 && EXTERNAL_MMXEXT(mm_flags)) {
            c->dsp.resample = c->linear ? ff_resample_linear_int16_mmxext
                                        : ff_resample_common_int16_mmxext;
        }
#endif
#if (HAVE_SSE2_EXTERNAL == 1)
        if (EXTERNAL_SSE2(mm_flags)) {
            c->dsp.resample = c->linear ? ff_resample_linear_int16_sse2
                                        : ff_resample_common_int16_sse2;
        }
#endif
#if (HAVE_XOP_EXTERNAL == 1)
        if (EXTERNAL_XOP(mm_flags)) {
            c->dsp.resample = c->linear ? ff_resample_linear_int16_xop
                                        : ff_resample_common_int16_xop;
        }
#endif
        break;
    case AV_SAMPLE_FMT_FLTP:
#if (HAVE_SSE_EXTERNAL == 1)
        if (EXTERNAL_SSE(mm_flags)) {
            c->dsp.resample = c->linear ? ff_resample_linear_float_sse
                                        : ff_resample_common_float_sse;
        }
#endif
#if (HAVE_AVX_EXTERNAL == 1)
        if (EXTERNAL_AVX(mm_flags)) {
            c->dsp.resample = c->linear ? ff_resample_linear_float_avx
                                        : ff_resample_common_float_avx;
        }
#endif
#if (HAVE_FMA3_EXTERNAL == 1)
        if (EXTERNAL_FMA3(mm_flags)) {
            c->dsp.resample = c->linear ? ff_resample_linear_float_fma3
                                        : ff_resample_common_float_fma3;
        }
#endif
#if (HAVE_FMA4_EXTERNAL == 1)
        if (EXTERNAL_FMA4(mm_flags)) {
            c->dsp.resample = c->linear ? ff_resample_linear_float_fma4
                                        : ff_resample_common_float_fma4;
        }
#endif
        break;
    case AV_SAMPLE_FMT_DBLP:
#if (HAVE_SSE2_EXTERNAL == 1)
        if (EXTERNAL_SSE2(mm_flags)) {
            c->dsp.resample = c->linear ? ff_resample_linear_double_sse2
                                        : ff_resample_common_double_sse2;
        }
#endif
        break;
    }
}
