/*****************************************************************************
 * deblock.S: aarch64 deblocking
 *****************************************************************************
 * Copyright (C) 2009-2014 x264 project
 *
 * Authors: Mans Rullgard <mans@mansr.com>
 *          Janne Grunau <janne-x264@jannau.net>
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

.macro h264_loop_filter_start
    cmp             w2,  #0
    ldr             w6,  [x4]
    ccmp            w3,  #0, #0, ne
    mov             v24.s[0], w6
    and             w6,  w6,  w6,  lsl #16
    b.eq            1f
    ands            w6,  w6,  w6,  lsl #8
    b.ge            2f
1:
    ret
2:
.endm

.macro h264_loop_filter_luma
    dup             v22.16b, w2                     // alpha
    uxtl            v24.8h,  v24.8b
    uabd            v21.16b, v16.16b, v0.16b        // abs(p0 - q0)
    uxtl            v24.4s,  v24.4h
    uabd            v28.16b, v18.16b, v16.16b       // abs(p1 - p0)
    sli             v24.8h,  v24.8h,  #8
    uabd            v30.16b, v2.16b,  v0.16b        // abs(q1 - q0)
    sli             v24.4s,  v24.4s,  #16
    cmhi            v21.16b, v22.16b, v21.16b       // < alpha
    dup             v22.16b, w3                     // beta
    cmlt            v23.16b, v24.16b, #0
    cmhi            v28.16b, v22.16b, v28.16b       // < beta
    cmhi            v30.16b, v22.16b, v30.16b       // < beta
    bic             v21.16b, v21.16b, v23.16b
    uabd            v17.16b, v20.16b, v16.16b       // abs(p2 - p0)
    and             v21.16b, v21.16b, v28.16b
    uabd            v19.16b,  v4.16b,  v0.16b       // abs(q2 - q0)
    cmhi            v17.16b, v22.16b, v17.16b       // < beta
    and             v21.16b, v21.16b, v30.16b
    cmhi            v19.16b, v22.16b, v19.16b       // < beta
    and             v17.16b, v17.16b, v21.16b
    and             v19.16b, v19.16b, v21.16b
    and             v24.16b, v24.16b, v21.16b
    urhadd          v28.16b, v16.16b,  v0.16b
    sub             v21.16b, v24.16b, v17.16b
    uqadd           v23.16b, v18.16b, v24.16b
    uhadd           v20.16b, v20.16b, v28.16b
    sub             v21.16b, v21.16b, v19.16b
    uhadd           v28.16b,  v4.16b, v28.16b
    umin            v23.16b, v23.16b, v20.16b
    uqsub           v22.16b, v18.16b, v24.16b
    uqadd           v4.16b,   v2.16b, v24.16b
    umax            v23.16b, v23.16b, v22.16b
    uqsub           v22.16b,  v2.16b, v24.16b
    umin            v28.16b,  v4.16b, v28.16b
    uxtl            v4.8h,    v0.8b
    umax            v28.16b, v28.16b, v22.16b
    uxtl2           v20.8h,   v0.16b
    usubw           v4.8h,    v4.8h,  v16.8b
    usubw2          v20.8h,  v20.8h,  v16.16b
    shl             v4.8h,    v4.8h,  #2
    shl             v20.8h,  v20.8h,  #2
    uaddw           v4.8h,    v4.8h,  v18.8b
    uaddw2          v20.8h,  v20.8h,  v18.16b
    usubw           v4.8h,    v4.8h,   v2.8b
    usubw2          v20.8h,  v20.8h,   v2.16b
    rshrn           v4.8b,    v4.8h,  #3
    rshrn2          v4.16b,  v20.8h,  #3
    bsl             v17.16b, v23.16b, v18.16b
    bsl             v19.16b, v28.16b,  v2.16b
    neg             v23.16b, v21.16b
    uxtl            v28.8h,  v16.8b
    smin            v4.16b,   v4.16b, v21.16b
    uxtl2           v21.8h,  v16.16b
    smax            v4.16b,   v4.16b, v23.16b
    uxtl            v22.8h,   v0.8b
    uxtl2           v24.8h,   v0.16b
    saddw           v28.8h,  v28.8h,  v4.8b
    saddw2          v21.8h,  v21.8h,  v4.16b
    ssubw           v22.8h,  v22.8h,  v4.8b
    ssubw2          v24.8h,  v24.8h,  v4.16b
    sqxtun          v16.8b,  v28.8h
    sqxtun2         v16.16b, v21.8h
    sqxtun          v0.8b,   v22.8h
    sqxtun2         v0.16b,  v24.8h
.endm

function x264_deblock_v_luma_neon, export=1
    h264_loop_filter_start

    ld1             {v0.16b},  [x0], x1
    ld1             {v2.16b},  [x0], x1
    ld1             {v4.16b},  [x0], x1
    sub             x0,  x0,  x1, lsl #2
    sub             x0,  x0,  x1, lsl #1
    ld1             {v20.16b},  [x0], x1
    ld1             {v18.16b},  [x0], x1
    ld1             {v16.16b},  [x0], x1

    h264_loop_filter_luma

    sub             x0,  x0,  x1, lsl #1
    st1             {v17.16b}, [x0], x1
    st1             {v16.16b}, [x0], x1
    st1             {v0.16b},  [x0], x1
    st1             {v19.16b}, [x0]

    ret
endfunc

function x264_deblock_h_luma_neon, export=1
    h264_loop_filter_start

    sub             x0,  x0,  #4
    ld1             {v6.8b},  [x0], x1
    ld1             {v20.8b}, [x0], x1
    ld1             {v18.8b}, [x0], x1
    ld1             {v16.8b}, [x0], x1
    ld1             {v0.8b},  [x0], x1
    ld1             {v2.8b},  [x0], x1
    ld1             {v4.8b},  [x0], x1
    ld1             {v26.8b}, [x0], x1
    ld1             {v6.d}[1],  [x0], x1
    ld1             {v20.d}[1], [x0], x1
    ld1             {v18.d}[1], [x0], x1
    ld1             {v16.d}[1], [x0], x1
    ld1             {v0.d}[1],  [x0], x1
    ld1             {v2.d}[1],  [x0], x1
    ld1             {v4.d}[1],  [x0], x1
    ld1             {v26.d}[1], [x0], x1

    transpose_8x16.b v6, v20, v18, v16, v0, v2, v4, v26, v21, v23

    h264_loop_filter_luma

    transpose_4x16.b v17, v16, v0, v19, v21, v23, v25, v27

    sub             x0,  x0,  x1, lsl #4
    add             x0,  x0,  #2
    st1             {v17.s}[0],  [x0], x1
    st1             {v16.s}[0], [x0], x1
    st1             {v0.s}[0],  [x0], x1
    st1             {v19.s}[0], [x0], x1
    st1             {v17.s}[1],  [x0], x1
    st1             {v16.s}[1], [x0], x1
    st1             {v0.s}[1],  [x0], x1
    st1             {v19.s}[1], [x0], x1
    st1             {v17.s}[2],  [x0], x1
    st1             {v16.s}[2], [x0], x1
    st1             {v0.s}[2],  [x0], x1
    st1             {v19.s}[2], [x0], x1
    st1             {v17.s}[3],  [x0], x1
    st1             {v16.s}[3], [x0], x1
    st1             {v0.s}[3],  [x0], x1
    st1             {v19.s}[3], [x0], x1

    ret
endfunc

.macro h264_loop_filter_chroma
    dup             v22.16b, w2              // alpha
    uxtl            v24.8h,  v24.8b
    uabd            v26.16b, v16.16b, v0.16b   // abs(p0 - q0)
    uxtl            v4.8h,   v0.8b
    uxtl2           v5.8h,   v0.16b
    uabd            v28.16b, v18.16b, v16.16b  // abs(p1 - p0)
    usubw           v4.8h,   v4.8h,   v16.8b
    usubw2          v5.8h,   v5.8h,   v16.16b
    sli             v24.8h,  v24.8h,  #8
    shl             v4.8h,   v4.8h,   #2
    shl             v5.8h,   v5.8h,   #2
    uabd            v30.16b, v2.16b,  v0.16b   // abs(q1 - q0)
    uxtl            v24.4s,  v24.4h
    uaddw           v4.8h,   v4.8h,   v18.8b
    uaddw2          v5.8h,   v5.8h,   v18.16b
    cmhi            v26.16b, v22.16b, v26.16b  // < alpha
    usubw           v4.8h,   v4.8h,   v2.8b
    usubw2          v5.8h,   v5.8h,   v2.16b
    sli             v24.4s,  v24.4s,  #16
    dup             v22.16b, w3              // beta
    rshrn           v4.8b,   v4.8h,   #3
    rshrn2          v4.16b,  v5.8h,   #3
    cmhi            v28.16b, v22.16b, v28.16b  // < beta
    cmhi            v30.16b, v22.16b, v30.16b  // < beta
    smin            v4.16b,  v4.16b,  v24.16b
    neg             v25.16b, v24.16b
    and             v26.16b, v26.16b, v28.16b
    smax            v4.16b,  v4.16b,  v25.16b
    and             v26.16b, v26.16b, v30.16b
    uxtl            v22.8h,  v0.8b
    uxtl2           v23.8h,  v0.16b
    and             v4.16b,  v4.16b,  v26.16b
    uxtl            v28.8h,  v16.8b
    uxtl2           v29.8h,  v16.16b
    saddw           v28.8h,  v28.8h,  v4.8b
    saddw2          v29.8h,  v29.8h,  v4.16b
    ssubw           v22.8h,  v22.8h,  v4.8b
    ssubw2          v23.8h,  v23.8h,  v4.16b
    sqxtun          v16.8b,  v28.8h
    sqxtun          v0.8b,   v22.8h
    sqxtun2         v16.16b, v29.8h
    sqxtun2         v0.16b,  v23.8h
.endm

function x264_deblock_v_chroma_neon, export=1
    h264_loop_filter_start

    sub             x0,  x0,  x1, lsl #1
    ld1             {v18.16b}, [x0], x1
    ld1             {v16.16b}, [x0], x1
    ld1             {v0.16b},  [x0], x1
    ld1             {v2.16b},  [x0]

    h264_loop_filter_chroma

    sub             x0,  x0,  x1, lsl #1
    st1             {v16.16b}, [x0], x1
    st1             {v0.16b},  [x0], x1

    ret
endfunc

function x264_deblock_h_chroma_neon, export=1
    h264_loop_filter_start

    sub             x0,  x0,  #4
    ld1             {v18.d}[0], [x0], x1
    ld1             {v16.d}[0], [x0], x1
    ld1             {v0.d}[0],  [x0], x1
    ld1             {v2.d}[0],  [x0], x1
    ld1             {v18.d}[1], [x0], x1
    ld1             {v16.d}[1], [x0], x1
    ld1             {v0.d}[1],  [x0], x1
    ld1             {v2.d}[1],  [x0], x1

    transpose4x8.h  v18, v16, v0, v2, v28, v29, v30, v31

    h264_loop_filter_chroma

    transpose4x8.h  v18, v16, v0, v2, v28, v29, v30, v31

    sub             x0,  x0,  x1, lsl #3
    st1             {v18.d}[0], [x0], x1
    st1             {v16.d}[0], [x0], x1
    st1             {v0.d}[0],  [x0], x1
    st1             {v2.d}[0],  [x0], x1
    st1             {v18.d}[1], [x0], x1
    st1             {v16.d}[1], [x0], x1
    st1             {v0.d}[1],  [x0], x1
    st1             {v2.d}[1],  [x0], x1

    ret
endfunc


//static void deblock_strength_c( uint8_t nnz[X264_SCAN8_SIZE],
//                                int8_t ref[2][X264_SCAN8_LUMA_SIZE],
//                                int16_t mv[2][X264_SCAN8_LUMA_SIZE][2],
//                                uint8_t bs[2][8][4], int mvy_limit,
//                                int bframe )
function x264_deblock_strength_neon, export=1
    movi        v4.16b, #0
    lsl         w4,  w4,  #8
    add         x3,  x3,  #32
    sub         w4,  w4,  #(1<<8)-3
    movi        v5.16b, #0
    dup         v6.8h,  w4
    mov         x6,  #-32

bframe:
    // load bytes ref
    add         x2,  x2,  #16
    ld1        {v31.d}[1], [x1], #8
    ld1        {v1.16b}, [x1], #16
    movi        v0.16b,  #0
    ld1        {v2.16b}, [x1], #16
    ext         v3.16b,  v0.16b,  v1.16b,  #15
    ext         v0.16b,  v0.16b,  v2.16b,  #15
    unzip       v21.4s,  v22.4s,  v1.4s,   v2.4s
    unzip       v23.4s,  v20.4s,  v3.4s,   v0.4s
    ext         v21.16b, v31.16b, v22.16b, #12

    eor         v0.16b,  v20.16b, v22.16b
    eor         v1.16b,  v21.16b, v22.16b
    orr         v4.16b,  v4.16b,  v0.16b
    orr         v5.16b,  v5.16b,  v1.16b

    ld1        {v21.8h}, [x2], #16      // mv + 0x10
    ld1        {v19.8h}, [x2], #16      // mv + 0x20
    ld1        {v22.8h}, [x2], #16      // mv + 0x30
    ld1        {v18.8h}, [x2], #16      // mv + 0x40
    ld1        {v23.8h}, [x2], #16      // mv + 0x50
    ext         v19.16b, v19.16b, v22.16b, #12
    ext         v18.16b, v18.16b, v23.16b, #12
    sabd        v0.8h,   v22.8h,  v19.8h
    ld1        {v19.8h}, [x2], #16      // mv + 0x60
    sabd        v1.8h,   v23.8h,  v18.8h
    ld1        {v24.8h}, [x2], #16      // mv + 0x70
    uqxtn       v0.8b,   v0.8h
    ld1        {v18.8h}, [x2], #16      // mv + 0x80
    ld1        {v25.8h}, [x2], #16      // mv + 0x90
    uqxtn2      v0.16b,  v1.8h
    ext         v19.16b, v19.16b, v24.16b, #12
    ext         v18.16b, v18.16b, v25.16b, #12
    sabd        v1.8h,   v24.8h,  v19.8h
    sabd        v2.8h,   v25.8h,  v18.8h
    uqxtn       v1.8b,   v1.8h
    uqxtn2      v1.16b,  v2.8h

    uqsub       v0.16b,  v0.16b,  v6.16b
    uqsub       v1.16b,  v1.16b,  v6.16b
    uqxtn       v0.8b,   v0.8h
    uqxtn2      v0.16b,  v1.8h

    sabd        v1.8h,   v22.8h,  v23.8h
    orr         v4.16b,  v4.16b,  v0.16b

    sabd        v0.8h,   v21.8h,  v22.8h
    sabd        v2.8h,   v23.8h,  v24.8h
    sabd        v3.8h,   v24.8h,  v25.8h
    uqxtn       v0.8b,   v0.8h
    uqxtn2      v0.16b,  v1.8h
    uqxtn       v1.8b,   v2.8h
    uqxtn2      v1.16b,  v3.8h

    uqsub       v0.16b,  v0.16b,  v6.16b
    uqsub       v1.16b,  v1.16b,  v6.16b
    uqxtn       v0.8b,   v0.8h
    uqxtn2      v0.16b,  v1.8h
    subs        w5,  w5,  #1
    orr         v5.16b,  v5.16b,  v0.16b
    b.eq        bframe

    movi        v6.16b, #1
    // load bytes nnz
    ld1        {v31.d}[1], [x0], #8
    ld1        {v1.16b}, [x0], #16
    movi        v0.16b,  #0
    ld1        {v2.16b}, [x0], #16
    ext         v3.16b,  v0.16b,  v1.16b,  #15
    ext         v0.16b,  v0.16b,  v2.16b,  #15
    unzip       v21.4s,  v22.4s,  v1.4s,   v2.4s
    unzip       v23.4s,  v20.4s,  v3.4s,   v0.4s
    ext         v21.16b, v31.16b, v22.16b, #12

    movrel      x7,  transpose_table
    ld1        {v7.16b}, [x7]
    orr         v0.16b,  v20.16b, v22.16b
    orr         v1.16b,  v21.16b, v22.16b
    umin        v0.16b,  v0.16b,  v6.16b
    umin        v1.16b,  v1.16b,  v6.16b
    umin        v4.16b,  v4.16b,  v6.16b        // mv ? 1 : 0
    umin        v5.16b,  v5.16b,  v6.16b
    add         v0.16b,  v0.16b,  v0.16b        // nnz ? 2 : 0
    add         v1.16b,  v1.16b,  v1.16b
    umax        v4.16b,  v4.16b,  v0.16b
    umax        v5.16b,  v5.16b,  v1.16b
    tbl         v6.16b, {v4.16b}, v7.16b
    st1        {v5.16b}, [x3], x6       // bs[1]
    st1        {v6.16b}, [x3]           // bs[0]
    ret
endfunc

const transpose_table
    .byte 0, 4,  8, 12
    .byte 1, 5,  9, 13
    .byte 2, 6, 10, 14
    .byte 3, 7, 11, 15
endconst
