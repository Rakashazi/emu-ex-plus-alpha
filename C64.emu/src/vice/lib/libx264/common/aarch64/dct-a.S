/****************************************************************************
 * dct-a.S: AArch6464 transform and zigzag
 *****************************************************************************
 * Copyright (C) 2009-2014 x264 project
 *
 * Authors: David Conrad <lessen42@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@x264.com.
 *****************************************************************************/

#include "asm.S"

const scan4x4_frame, align=4
.byte    0,1,   8,9,   2,3,   4,5
.byte   10,11, 16,17, 24,25, 18,19
.byte   12,13,  6,7,  14,15, 20,21
.byte   26,27, 28,29, 22,23, 30,31
endconst

// sum = a + (b>>shift)   sub = (a>>shift) - b
.macro SUMSUB_SHR shift sum sub a b t0 t1
    sshr        \t0,  \b, #\shift
    sshr        \t1,  \a, #\shift
    add         \sum, \a, \t0
    sub         \sub, \t1, \b
.endm

// sum = (a>>shift) + b   sub = a - (b>>shift)
.macro SUMSUB_SHR2 shift sum sub a b t0 t1
    sshr        \t0,  \a, #\shift
    sshr        \t1,  \b, #\shift
    add         \sum, \t0, \b
    sub         \sub, \a, \t1
.endm

// a += 1.5*ma   b -= 1.5*mb
.macro SUMSUB_15 a b ma mb t0 t1
    sshr        \t0, \ma, #1
    sshr        \t1, \mb, #1
    add         \t0, \t0, \ma
    add         \t1, \t1, \mb
    add         \a,  \a,  \t0
    sub         \b,  \b,  \t1
.endm


function x264_dct4x4dc_neon, export=1
    ld1        {v0.4h,v1.4h,v2.4h,v3.4h}, [x0]
    movi        v31.4h, #1
    SUMSUB_AB   v4.4h,  v5.4h,  v0.4h,  v1.4h
    SUMSUB_AB   v6.4h,  v7.4h,  v2.4h,  v3.4h
    SUMSUB_AB   v0.4h,  v2.4h,  v4.4h,  v6.4h
    SUMSUB_AB   v3.4h,  v1.4h,  v5.4h,  v7.4h
    transpose   v4.4h,  v6.4h,  v0.4h,  v2.4h
    transpose   v5.4h,  v7.4h,  v1.4h,  v3.4h
    SUMSUB_AB   v0.4h,  v2.4h,  v4.4h,  v6.4h
    SUMSUB_AB   v1.4h,  v3.4h,  v5.4h,  v7.4h
    transpose   v4.2s,  v5.2s,  v0.2s,  v1.2s
    transpose   v6.2s,  v7.2s,  v2.2s,  v3.2s
    add         v16.4h, v4.4h,  v31.4h
    add         v17.4h, v6.4h,  v31.4h
    srhadd      v0.4h,  v4.4h,  v5.4h
    shsub       v1.4h,  v16.4h, v5.4h
    shsub       v2.4h,  v17.4h, v7.4h
    srhadd      v3.4h,  v6.4h,  v7.4h
    st1        {v0.4h,v1.4h,v2.4h,v3.4h}, [x0]
    ret
endfunc

function x264_idct4x4dc_neon, export=1
    ld1        {v0.4h,v1.4h,v2.4h,v3.4h}, [x0]
    SUMSUB_AB   v4.4h,  v5.4h,  v0.4h,  v1.4h
    SUMSUB_AB   v6.4h,  v7.4h,  v2.4h,  v3.4h
    SUMSUB_AB   v0.4h,  v2.4h,  v4.4h,  v6.4h
    SUMSUB_AB   v3.4h,  v1.4h,  v5.4h,  v7.4h
    transpose   v4.4h,  v6.4h,  v0.4h,  v2.4h
    transpose   v5.4h,  v7.4h,  v1.4h,  v3.4h
    SUMSUB_AB   v0.4h,  v2.4h,  v4.4h,  v6.4h
    SUMSUB_AB   v1.4h,  v3.4h,  v5.4h,  v7.4h
    transpose   v4.2s,  v5.2s,  v0.2s,  v1.2s
    transpose   v6.2s,  v7.2s,  v2.2s,  v3.2s
    SUMSUB_AB   v0.4h,  v1.4h,  v4.4h,  v5.4h
    SUMSUB_AB   v3.4h,  v2.4h,  v6.4h,  v7.4h
    st1        {v0.4h,v1.4h,v2.4h,v3.4h}, [x0]
    ret
endfunc

.macro DCT_1D v0 v1 v2 v3 v4 v5 v6 v7
    SUMSUB_AB   \v1, \v6, \v5, \v6
    SUMSUB_AB   \v3, \v7, \v4, \v7
    add         \v0, \v3, \v1
    add         \v4, \v7, \v7
    add         \v5, \v6, \v6
    sub         \v2, \v3, \v1
    add         \v1, \v4, \v6
    sub         \v3, \v7, \v5
.endm

function x264_sub4x4_dct_neon, export=1
    mov         x3, #FENC_STRIDE
    mov         x4, #FDEC_STRIDE
    ld1        {v0.s}[0], [x1], x3
    ld1        {v1.s}[0], [x2], x4
    ld1        {v2.s}[0], [x1], x3
    usubl       v16.8h, v0.8b,  v1.8b
    ld1        {v3.s}[0], [x2], x4
    ld1        {v4.s}[0], [x1], x3
    usubl       v17.8h, v2.8b,  v3.8b
    ld1        {v5.s}[0], [x2], x4
    ld1        {v6.s}[0], [x1], x3
    usubl       v18.8h, v4.8b,  v5.8b
    ld1        {v7.s}[0], [x2], x4
    usubl       v19.8h, v6.8b,  v7.8b

    DCT_1D      v0.4h, v1.4h, v2.4h, v3.4h, v16.4h, v17.4h, v18.4h, v19.4h
    transpose4x4.h v0, v1, v2, v3, v4, v5, v6, v7
    DCT_1D      v4.4h, v5.4h, v6.4h, v7.4h, v0.4h, v1.4h, v2.4h, v3.4h
    st1        {v4.4h,v5.4h,v6.4h,v7.4h}, [x0]
    ret
endfunc

function x264_sub8x4_dct_neon
    ld1        {v0.8b}, [x1], x3
    ld1        {v1.8b}, [x2], x4
    usubl       v16.8h, v0.8b,  v1.8b
    ld1        {v2.8b}, [x1], x3
    ld1        {v3.8b}, [x2], x4
    usubl       v17.8h, v2.8b,  v3.8b
    ld1        {v4.8b}, [x1], x3
    ld1        {v5.8b}, [x2], x4
    usubl       v18.8h, v4.8b,  v5.8b
    ld1        {v6.8b}, [x1], x3
    ld1        {v7.8b}, [x2], x4
    usubl       v19.8h, v6.8b,  v7.8b

    DCT_1D      v0.8h, v1.8h, v2.8h, v3.8h, v16.8h, v17.8h, v18.8h, v19.8h
    transpose4x8.h  v0, v1, v2, v3, v4, v5, v6, v7

    SUMSUB_AB   v16.8h, v19.8h, v0.8h,  v3.8h
    SUMSUB_AB   v17.8h, v18.8h, v1.8h,  v2.8h
    add         v22.8h, v19.8h, v19.8h
    add         v21.8h, v18.8h, v18.8h
    add         v0.8h,  v16.8h, v17.8h
    sub         v1.8h,  v16.8h, v17.8h

    add         v2.8h,  v22.8h, v18.8h
    sub         v3.8h,  v19.8h, v21.8h

    zip1        v4.2d,  v0.2d,  v2.2d
    zip2        v6.2d,  v0.2d,  v2.2d
    zip1        v5.2d,  v1.2d,  v3.2d
    zip2        v7.2d,  v1.2d,  v3.2d

    st1        {v4.8h}, [x0], #16
    st1        {v5.8h}, [x0], #16
    st1        {v6.8h}, [x0], #16
    st1        {v7.8h}, [x0], #16
    ret
endfunc

function x264_sub8x8_dct_neon, export=1
    mov         x5,  x30
    mov         x3, #FENC_STRIDE
    mov         x4, #FDEC_STRIDE
    bl          x264_sub8x4_dct_neon
    mov         x30, x5
    b           x264_sub8x4_dct_neon
endfunc

function x264_sub16x16_dct_neon, export=1
    mov         x5,  x30
    mov         x3, #FENC_STRIDE
    mov         x4, #FDEC_STRIDE
    bl          x264_sub8x4_dct_neon
    bl          x264_sub8x4_dct_neon
    sub         x1, x1, #8*FENC_STRIDE-8
    sub         x2, x2, #8*FDEC_STRIDE-8
    bl          x264_sub8x4_dct_neon
    bl          x264_sub8x4_dct_neon
    sub         x1, x1, #8
    sub         x2, x2, #8
    bl          x264_sub8x4_dct_neon
    bl          x264_sub8x4_dct_neon
    sub         x1, x1, #8*FENC_STRIDE-8
    sub         x2, x2, #8*FDEC_STRIDE-8
    bl          x264_sub8x4_dct_neon
    mov         x30, x5
    b           x264_sub8x4_dct_neon
endfunc


.macro DCT8_1D type
    SUMSUB_AB   v18.8h, v17.8h, v3.8h,  v4.8h   // s34/d34
    SUMSUB_AB   v19.8h, v16.8h, v2.8h,  v5.8h   // s25/d25
    SUMSUB_AB   v22.8h, v21.8h, v1.8h,  v6.8h   // s16/d16
    SUMSUB_AB   v23.8h, v20.8h, v0.8h,  v7.8h   // s07/d07

    SUMSUB_AB   v24.8h, v26.8h,  v23.8h, v18.8h  // a0/a2
    SUMSUB_AB   v25.8h, v27.8h,  v22.8h, v19.8h  // a1/a3

    SUMSUB_AB   v30.8h, v29.8h,  v20.8h, v17.8h  // a6/a5
    sshr        v23.8h, v21.8h, #1
    sshr        v18.8h, v16.8h, #1
    add         v23.8h, v23.8h, v21.8h
    add         v18.8h, v18.8h, v16.8h
    sub         v30.8h, v30.8h, v23.8h
    sub         v29.8h, v29.8h, v18.8h

    SUMSUB_AB   v28.8h, v31.8h,  v21.8h, v16.8h   // a4/a7
    sshr        v22.8h, v20.8h, #1
    sshr        v19.8h, v17.8h, #1
    add         v22.8h, v22.8h, v20.8h
    add         v19.8h, v19.8h, v17.8h
    add         v22.8h, v28.8h, v22.8h
    add         v31.8h, v31.8h, v19.8h

    SUMSUB_AB      v0.8h,  v4.8h,  v24.8h, v25.8h
    SUMSUB_SHR  2, v1.8h,  v7.8h,  v22.8h, v31.8h, v16.8h, v17.8h
    SUMSUB_SHR  1, v2.8h,  v6.8h,  v26.8h, v27.8h, v18.8h, v19.8h
    SUMSUB_SHR2 2, v3.8h,  v5.8h,  v30.8h, v29.8h, v20.8h, v21.8h
.endm

function x264_sub8x8_dct8_neon, export=1
    mov         x3, #FENC_STRIDE
    mov         x4, #FDEC_STRIDE
    ld1        {v16.8b}, [x1], x3
    ld1        {v17.8b}, [x2], x4
    ld1        {v18.8b}, [x1], x3
    ld1        {v19.8b}, [x2], x4
    usubl       v0.8h,  v16.8b, v17.8b
    ld1        {v20.8b}, [x1], x3
    ld1        {v21.8b}, [x2], x4
    usubl       v1.8h,  v18.8b, v19.8b
    ld1        {v22.8b}, [x1], x3
    ld1        {v23.8b}, [x2], x4
    usubl       v2.8h,  v20.8b, v21.8b
    ld1        {v24.8b}, [x1], x3
    ld1        {v25.8b}, [x2], x4
    usubl       v3.8h,  v22.8b, v23.8b
    ld1        {v26.8b}, [x1], x3
    ld1        {v27.8b}, [x2], x4
    usubl       v4.8h,  v24.8b, v25.8b
    ld1        {v28.8b}, [x1], x3
    ld1        {v29.8b}, [x2], x4
    usubl       v5.8h,  v26.8b, v27.8b
    ld1        {v30.8b}, [x1], x3
    ld1        {v31.8b}, [x2], x4
    usubl       v6.8h,  v28.8b, v29.8b
    usubl       v7.8h,  v30.8b, v31.8b

    DCT8_1D row
    transpose8x8.h v0, v1, v2, v3, v4, v5, v6, v7, v30, v31
    DCT8_1D col

    st1        {v0.8h,v1.8h,v2.8h,v3.8h}, [x0], #64
    st1        {v4.8h,v5.8h,v6.8h,v7.8h}, [x0], #64
    ret
endfunc

function x264_sub16x16_dct8_neon, export=1
    mov         x7,  x30
    bl          X(x264_sub8x8_dct8_neon)
    sub         x1,  x1,  #FENC_STRIDE*8 - 8
    sub         x2,  x2,  #FDEC_STRIDE*8 - 8
    bl          X(x264_sub8x8_dct8_neon)
    sub         x1,  x1,  #8
    sub         x2,  x2,  #8
    bl          X(x264_sub8x8_dct8_neon)
    mov         x30, x7
    sub         x1,  x1,  #FENC_STRIDE*8 - 8
    sub         x2,  x2,  #FDEC_STRIDE*8 - 8
    b           X(x264_sub8x8_dct8_neon)
endfunc


// First part of IDCT (minus final SUMSUB_BA)
.macro IDCT_1D d4 d5 d6 d7 d0 d1 d2 d3
    SUMSUB_AB   \d4, \d5, \d0, \d2
    sshr        \d7, \d1, #1
    sshr        \d6, \d3, #1
    sub         \d7, \d7, \d3
    add         \d6, \d6, \d1
.endm

function x264_add4x4_idct_neon, export=1
    mov         x2, #FDEC_STRIDE
    ld1        {v0.4h,v1.4h,v2.4h,v3.4h}, [x1]

    IDCT_1D     v4.4h, v5.4h, v6.4h, v7.4h, v0.4h, v1.4h, v2.4h, v3.4h
    ld1        {v28.s}[0], [x0], x2
    SUMSUB_AB   v0.4h, v2.4h, v4.4h, v6.4h
    SUMSUB_AB   v1.4h, v3.4h, v5.4h, v7.4h

    transpose4x4.h v0, v1, v3, v2, v16, v17, v18, v19

    IDCT_1D     v4.4h, v5.4h, v6.4h, v7.4h, v0.4h, v1.4h, v3.4h, v2.4h
    ld1        {v29.s}[0], [x0], x2
    SUMSUB_AB   v0.4h, v2.4h, v4.4h, v6.4h
    SUMSUB_AB   v1.4h, v3.4h, v5.4h, v7.4h

    srshr       v0.4h,  v0.4h,  #6
    srshr       v1.4h,  v1.4h,  #6
    ld1        {v31.s}[0], [x0], x2
    srshr       v2.4h,  v2.4h,  #6
    srshr       v3.4h,  v3.4h,  #6
    ld1        {v30.s}[0], [x0], x2

    sub         x0,  x0,  x2,  lsl #2
    uaddw       v0.8h,  v0.8h,  v28.8b
    uaddw       v1.8h,  v1.8h,  v29.8b
    uaddw       v2.8h,  v2.8h,  v30.8b
    uaddw       v3.8h,  v3.8h,  v31.8b
    sqxtun      v0.8b,  v0.8h
    sqxtun      v1.8b,  v1.8h
    sqxtun      v2.8b,  v2.8h
    sqxtun      v3.8b,  v3.8h

    st1        {v0.s}[0], [x0], x2
    st1        {v1.s}[0], [x0], x2
    st1        {v3.s}[0], [x0], x2
    st1        {v2.s}[0], [x0], x2
    ret
endfunc

function x264_add8x4_idct_neon, export=1
    ld1        {v0.8h,v1.8h}, [x1], #32
    ld1        {v2.8h,v3.8h}, [x1], #32
    transpose   v20.2d, v21.2d, v0.2d, v2.2d
    transpose   v22.2d, v23.2d, v1.2d, v3.2d
    IDCT_1D     v16.8h, v17.8h, v18.8h, v19.8h, v20.8h, v21.8h, v22.8h, v23.8h
    SUMSUB_AB   v0.8h,  v3.8h,  v16.8h, v18.8h
    SUMSUB_AB   v1.8h,  v2.8h,  v17.8h, v19.8h

    transpose4x8.h v0, v1, v2, v3, v4, v5, v6, v7

    IDCT_1D     v16.8h, v17.8h, v18.8h, v19.8h, v0.8h, v1.8h, v2.8h, v3.8h
    SUMSUB_AB   v0.8h,  v3.8h,  v16.8h, v18.8h
    SUMSUB_AB   v1.8h,  v2.8h,  v17.8h, v19.8h

    srshr       v0.8h,  v0.8h,  #6
    ld1        {v28.8b}, [x0], x2
    srshr       v1.8h,  v1.8h,  #6
    ld1        {v29.8b}, [x0], x2
    srshr       v2.8h,  v2.8h,  #6
    ld1        {v30.8b}, [x0], x2
    srshr       v3.8h,  v3.8h,  #6
    ld1        {v31.8b}, [x0], x2

    sub         x0,  x0,  x2,  lsl #2
    uaddw       v0.8h,  v0.8h,  v28.8b
    uaddw       v1.8h,  v1.8h,  v29.8b
    uaddw       v2.8h,  v2.8h,  v30.8b
    uaddw       v3.8h,  v3.8h,  v31.8b

    sqxtun      v0.8b,  v0.8h
    sqxtun      v1.8b,  v1.8h
    st1        {v0.8b}, [x0], x2
    sqxtun      v2.8b,  v2.8h
    st1        {v1.8b}, [x0], x2
    sqxtun      v3.8b,  v3.8h
    st1        {v2.8b}, [x0], x2
    st1        {v3.8b}, [x0], x2
    ret
endfunc

function x264_add8x8_idct_neon, export=1
    mov             x2, #FDEC_STRIDE
    mov             x5,  x30
    bl              X(x264_add8x4_idct_neon)
    mov             x30, x5
    b               X(x264_add8x4_idct_neon)
endfunc

function x264_add16x16_idct_neon, export=1
    mov             x2, #FDEC_STRIDE
    mov             x5,  x30
    bl              X(x264_add8x4_idct_neon)
    bl              X(x264_add8x4_idct_neon)
    sub             x0, x0, #8*FDEC_STRIDE-8
    bl              X(x264_add8x4_idct_neon)
    bl              X(x264_add8x4_idct_neon)
    sub             x0, x0, #8
    bl              X(x264_add8x4_idct_neon)
    bl              X(x264_add8x4_idct_neon)
    sub             x0, x0, #8*FDEC_STRIDE-8
    bl              X(x264_add8x4_idct_neon)
    mov             x30, x5
    b               X(x264_add8x4_idct_neon)
endfunc

.macro IDCT8_1D type
    SUMSUB_AB   v0.8h,  v1.8h,  v16.8h, v20.8h          // a0/a2
.ifc \type, row
    ld1        {v22.8h,v23.8h}, [x1], #32
.endif
    SUMSUB_SHR  1, v2.8h,  v3.8h,  v18.8h, v22.8h, v16.8h, v20.8h   // a6/a4
    SUMSUB_AB   v16.8h, v18.8h, v21.8h, v19.8h
    SUMSUB_15   v16.8h, v18.8h, v17.8h, v23.8h, v20.8h, v22.8h      // a7/a1
    SUMSUB_AB   v22.8h, v23.8h, v23.8h, v17.8h
    SUMSUB_15   v23.8h, v22.8h, v21.8h, v19.8h, v20.8h, v17.8h      // a5/a3

    SUMSUB_SHR  2, v21.8h, v22.8h, v22.8h, v23.8h, v19.8h, v17.8h   // b3/b5
    SUMSUB_SHR2 2, v20.8h, v23.8h, v16.8h, v18.8h, v19.8h, v17.8h   // b1/b7

    SUMSUB_AB   v18.8h, v2.8h,  v0.8h,  v2.8h           // b0/b6
    SUMSUB_AB   v19.8h, v3.8h,  v1.8h,  v3.8h           // b2/b4

    SUMSUB_AB   v16.8h, v23.8h, v18.8h, v23.8h
    SUMSUB_AB   v17.8h, v22.8h, v19.8h, v22.8h
    SUMSUB_AB   v18.8h, v21.8h, v3.8h,  v21.8h
    SUMSUB_AB   v19.8h, v20.8h, v2.8h,  v20.8h
.endm

function x264_add8x8_idct8_neon, export=1
    mov         x2,  #FDEC_STRIDE
    ld1        {v16.8h,v17.8h}, [x1], #32
    ld1        {v18.8h,v19.8h}, [x1], #32
    ld1        {v20.8h,v21.8h}, [x1], #32

    IDCT8_1D    row

    transpose8x8.h v16, v17, v18, v19, v20, v21, v22, v23, v30, v31

    IDCT8_1D    col

    ld1        {v0.8b}, [x0], x2
    srshr       v16.8h, v16.8h, #6
    ld1        {v1.8b}, [x0], x2
    srshr       v17.8h, v17.8h, #6
    ld1        {v2.8b}, [x0], x2
    srshr       v18.8h, v18.8h, #6
    ld1        {v3.8b}, [x0], x2
    srshr       v19.8h, v19.8h, #6
    ld1        {v4.8b}, [x0], x2
    srshr       v20.8h, v20.8h, #6
    ld1        {v5.8b}, [x0], x2
    srshr       v21.8h, v21.8h, #6
    ld1        {v6.8b}, [x0], x2
    srshr       v22.8h, v22.8h, #6
    ld1        {v7.8b}, [x0], x2
    srshr       v23.8h, v23.8h, #6
    sub         x0,  x0,  x2,  lsl #3

    uaddw       v16.8h, v16.8h, v0.8b
    uaddw       v17.8h, v17.8h, v1.8b
    uaddw       v18.8h, v18.8h, v2.8b
    sqxtun      v0.8b,  v16.8h
    sqxtun      v1.8b,  v17.8h
    sqxtun      v2.8b,  v18.8h
    uaddw       v19.8h, v19.8h, v3.8b
    st1        {v0.8b}, [x0], x2
    uaddw       v20.8h, v20.8h, v4.8b
    st1        {v1.8b}, [x0], x2
    uaddw       v21.8h, v21.8h, v5.8b
    st1        {v2.8b}, [x0], x2
    sqxtun      v3.8b,  v19.8h
    sqxtun      v4.8b,  v20.8h
    uaddw       v22.8h, v22.8h, v6.8b
    uaddw       v23.8h, v23.8h, v7.8b
    st1        {v3.8b}, [x0], x2
    sqxtun      v5.8b,  v21.8h
    st1        {v4.8b}, [x0], x2
    sqxtun      v6.8b,  v22.8h
    sqxtun      v7.8b,  v23.8h
    st1        {v5.8b}, [x0], x2
    st1        {v6.8b}, [x0], x2
    st1        {v7.8b}, [x0], x2
    ret
endfunc

function x264_add16x16_idct8_neon, export=1
    mov             x7,  x30
    bl              X(x264_add8x8_idct8_neon)
    sub             x0,  x0,  #8*FDEC_STRIDE-8
    bl              X(x264_add8x8_idct8_neon)
    sub             x0,  x0,  #8
    bl              X(x264_add8x8_idct8_neon)
    sub             x0,  x0,  #8*FDEC_STRIDE-8
    mov             x30, x7
    b               X(x264_add8x8_idct8_neon)
endfunc

function x264_add8x8_idct_dc_neon, export=1
    mov         x2,  #FDEC_STRIDE
    ld1        {v16.4h}, [x1]
    ld1        {v0.8b}, [x0], x2
    srshr       v16.4h, v16.4h, #6
    ld1        {v1.8b}, [x0], x2
    dup         v20.8h, v16.h[0]
    dup         v21.8h, v16.h[1]
    ld1        {v2.8b}, [x0], x2
    dup         v22.8h, v16.h[2]
    dup         v23.8h, v16.h[3]
    ld1        {v3.8b}, [x0], x2
    trn1        v20.2d, v20.2d,  v21.2d
    ld1        {v4.8b}, [x0], x2
    trn1        v21.2d, v22.2d,  v23.2d
    ld1        {v5.8b}, [x0], x2
    neg         v22.8h, v20.8h
    ld1        {v6.8b}, [x0], x2
    neg         v23.8h, v21.8h
    ld1        {v7.8b}, [x0], x2

    sub         x0,  x0,  #8*FDEC_STRIDE

    sqxtun      v20.8b,  v20.8h
    sqxtun      v21.8b,  v21.8h
    sqxtun      v22.8b,  v22.8h
    sqxtun      v23.8b,  v23.8h

    uqadd       v0.8b,  v0.8b,  v20.8b
    uqadd       v1.8b,  v1.8b,  v20.8b
    uqadd       v2.8b,  v2.8b,  v20.8b
    uqadd       v3.8b,  v3.8b,  v20.8b
    uqadd       v4.8b,  v4.8b,  v21.8b
    uqadd       v5.8b,  v5.8b,  v21.8b
    uqadd       v6.8b,  v6.8b,  v21.8b
    uqadd       v7.8b,  v7.8b,  v21.8b
    uqsub       v0.8b,  v0.8b,  v22.8b
    uqsub       v1.8b,  v1.8b,  v22.8b
    uqsub       v2.8b,  v2.8b,  v22.8b
    uqsub       v3.8b,  v3.8b,  v22.8b
    uqsub       v4.8b,  v4.8b,  v23.8b
    uqsub       v5.8b,  v5.8b,  v23.8b
    uqsub       v6.8b,  v6.8b,  v23.8b
    uqsub       v7.8b,  v7.8b,  v23.8b

    st1        {v0.8b}, [x0], x2
    st1        {v1.8b}, [x0], x2
    st1        {v2.8b}, [x0], x2
    st1        {v3.8b}, [x0], x2
    st1        {v4.8b}, [x0], x2
    st1        {v5.8b}, [x0], x2
    st1        {v6.8b}, [x0], x2
    st1        {v7.8b}, [x0], x2
    ret
endfunc

.macro ADD16x4_IDCT_DC dc
    ld1         {v4.16b}, [x0], x3
    dup         v24.8h,  \dc[0]
    dup         v25.8h,  \dc[1]
    ld1         {v5.16b}, [x0], x3
    dup         v26.8h,  \dc[2]
    dup         v27.8h,  \dc[3]
    ld1         {v6.16b}, [x0], x3
    trn1        v24.2d,  v24.2d,  v25.2d
    ld1         {v7.16b}, [x0], x3
    trn1        v25.2d,  v26.2d,  v27.2d
    neg         v26.8h,  v24.8h
    neg         v27.8h,  v25.8h

    sqxtun      v20.8b,  v24.8h
    sqxtun      v21.8b,  v26.8h
    sqxtun2     v20.16b, v25.8h
    sqxtun2     v21.16b, v27.8h

    uqadd        v4.16b, v4.16b, v20.16b
    uqadd        v5.16b, v5.16b, v20.16b
    uqadd        v6.16b, v6.16b, v20.16b
    uqadd        v7.16b, v7.16b, v20.16b

    uqsub        v4.16b, v4.16b, v21.16b
    uqsub        v5.16b, v5.16b, v21.16b
    uqsub        v6.16b, v6.16b, v21.16b
    st1         {v4.16b}, [x2], x3
    uqsub        v7.16b, v7.16b, v21.16b
    st1         {v5.16b}, [x2], x3
    st1         {v6.16b}, [x2], x3
    st1         {v7.16b}, [x2], x3
.endm

function x264_add16x16_idct_dc_neon, export=1
    mov         x2,  x0
    mov         x3,  #FDEC_STRIDE

    ld1        {v0.4h,v1.4h,v2.4h,v3.4h}, [x1]
    srshr       v0.4h,  v0.4h,  #6
    srshr       v1.4h,  v1.4h,  #6

    ADD16x4_IDCT_DC v0.h
    srshr       v2.4h,  v2.4h,  #6
    ADD16x4_IDCT_DC v1.h
    srshr       v3.4h,  v3.4h,  #6
    ADD16x4_IDCT_DC v2.h
    ADD16x4_IDCT_DC v3.h
    ret
endfunc

function x264_sub8x8_dct_dc_neon, export=1
    mov             x3,  #FENC_STRIDE
    mov             x4,  #FDEC_STRIDE
    ld1        {v16.8b}, [x1], x3
    ld1        {v17.8b}, [x2], x4
    usubl       v16.8h,  v16.8b, v17.8b
    ld1        {v18.8b}, [x1], x3
    ld1        {v19.8b}, [x2], x4
    usubl       v17.8h,  v18.8b, v19.8b
    ld1        {v20.8b}, [x1], x3
    ld1        {v21.8b}, [x2], x4
    usubl       v18.8h, v20.8b, v21.8b
    ld1        {v22.8b}, [x1], x3
    add         v0.8h,  v16.8h, v17.8h
    ld1        {v23.8b}, [x2], x4
    usubl       v19.8h, v22.8b, v23.8b
    ld1        {v24.8b}, [x1], x3
    add         v0.8h,  v0.8h,  v18.8h
    ld1        {v25.8b}, [x2], x4
    usubl       v20.8h, v24.8b, v25.8b
    ld1        {v26.8b}, [x1], x3
    add         v0.8h,  v0.8h,  v19.8h
    ld1        {v27.8b}, [x2], x4
    usubl       v21.8h, v26.8b, v27.8b
    ld1        {v28.8b}, [x1], x3
    ld1        {v29.8b}, [x2], x4
    usubl       v22.8h, v28.8b, v29.8b
    ld1        {v30.8b}, [x1], x3
    add         v1.8h,  v20.8h, v21.8h
    ld1        {v31.8b}, [x2], x4
    usubl       v23.8h, v30.8b, v31.8b
    add         v1.8h,  v1.8h,  v22.8h
    add         v1.8h,  v1.8h,  v23.8h

    transpose   v2.2d,  v3.2d,  v0.2d,  v1.2d

    add         v0.8h,  v2.8h,  v3.8h
    sub         v1.8h,  v2.8h,  v3.8h

    transpose   v2.2d,  v3.2d,  v0.2d,  v1.2d

    add         v0.8h,  v2.8h,  v3.8h
    sub         v1.8h,  v2.8h,  v3.8h

    transpose   v2.2d,  v3.2d,  v0.2d,  v1.2d

    addp        v0.8h,  v2.8h,  v3.8h
    addp        v0.8h,  v0.8h,  v0.8h

    st1        {v0.4h}, [x0]
    ret
endfunc

function x264_zigzag_scan_4x4_frame_neon, export=1
    movrel      x2, scan4x4_frame
    ld1        {v0.16b,v1.16b}, [x1]
    ld1        {v16.16b,v17.16b}, [x2]
    tbl         v2.16b, {v0.16b,v1.16b}, v16.16b
    tbl         v3.16b, {v0.16b,v1.16b}, v17.16b
    st1        {v2.16b,v3.16b},   [x0]
    ret
endfunc
