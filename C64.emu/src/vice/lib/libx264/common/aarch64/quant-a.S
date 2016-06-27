/****************************************************************************
 * quant.S: arm quantization and level-run
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

.macro QUANT_TWO bias0 bias1 mf0_1 mf2_3 mask
    add         v18.8h, v18.8h, \bias0
    add         v19.8h, v19.8h, \bias1
    umull       v20.4s, v18.4h, \mf0_1\().4h
    umull2      v21.4s, v18.8h, \mf0_1\().8h
    umull       v22.4s, v19.4h, \mf2_3\().4h
    umull2      v23.4s, v19.8h, \mf2_3\().8h
    sshr        v16.8h, v16.8h, #15
    sshr        v17.8h, v17.8h, #15
    shrn        v18.4h, v20.4s, #16
    shrn2       v18.8h, v21.4s, #16
    shrn        v19.4h, v22.4s, #16
    shrn2       v19.8h, v23.4s, #16
    eor         v18.16b, v18.16b, v16.16b
    eor         v19.16b, v19.16b, v17.16b
    sub         v18.8h, v18.8h, v16.8h
    sub         v19.8h, v19.8h, v17.8h
    orr         \mask,  v18.16b, v19.16b
    st1        {v18.8h,v19.8h}, [x0], #32
.endm

.macro QUANT_END d
    fmov        x2,  \d
    mov         w0,  #0
    tst         x2,  x2
    cinc        w0,  w0,  ne
    ret
.endm

// quant_2x2_dc( int16_t dct[4], int mf, int bias )
function x264_quant_2x2_dc_neon, export=1
    ld1        {v0.4h}, [x0]
    dup         v2.4h,  w2
    dup         v1.4h,  w1
    abs         v3.4h,  v0.4h
    add         v3.4h,  v3.4h,  v2.4h
    umull       v3.4s,  v3.4h,  v1.4h
    sshr        v0.4h,  v0.4h,  #15
    shrn        v3.4h,  v3.4s,  #16
    eor         v3.8b,  v3.8b,  v0.8b
    sub         v3.4h,  v3.4h,  v0.4h
    st1        {v3.4h}, [x0]
    QUANT_END   d3
endfunc

// quant_4x4_dc( int16_t dct[16], int mf, int bias )
function x264_quant_4x4_dc_neon, export=1
    ld1        {v16.8h,v17.8h}, [x0]
    abs         v18.8h,  v16.8h
    abs         v19.8h,  v17.8h
    dup         v0.8h,  w2
    dup         v2.8h,  w1
    QUANT_TWO   v0.8h,  v0.8h,  v2,  v2,  v0.16b
    uqxtn       v0.8b,  v0.8h
    QUANT_END   d0
endfunc

// quant_4x4( int16_t dct[16], uint16_t mf[16], uint16_t bias[16] )
function x264_quant_4x4_neon, export=1
    ld1        {v16.8h,v17.8h}, [x0]
    abs         v18.8h,  v16.8h
    abs         v19.8h,  v17.8h
    ld1        {v0.8h,v1.8h}, [x2]
    ld1        {v2.8h,v3.8h}, [x1]
    QUANT_TWO   v0.8h,  v1.8h,  v2,  v3,  v0.16b
    uqxtn       v0.8b,  v0.8h
    QUANT_END   d0
endfunc

// quant_4x4x4( int16_t dct[4][16], uint16_t mf[16], uint16_t bias[16] )
function x264_quant_4x4x4_neon, export=1
    ld1        {v16.8h,v17.8h}, [x0]
    abs         v18.8h, v16.8h
    abs         v19.8h, v17.8h
    ld1        {v0.8h,v1.8h}, [x2]
    ld1        {v2.8h,v3.8h}, [x1]
    QUANT_TWO   v0.8h,  v1.8h,  v2,  v3,  v4.16b
    ld1        {v16.8h,v17.8h}, [x0]
    abs         v18.8h, v16.8h
    abs         v19.8h, v17.8h
    QUANT_TWO   v0.8h,  v1.8h,  v2,  v3,  v5.16b
    ld1        {v16.8h,v17.8h}, [x0]
    abs         v18.8h, v16.8h
    abs         v19.8h, v17.8h
    QUANT_TWO   v0.8h,  v1.8h,  v2,  v3,  v6.16b
    ld1        {v16.8h,v17.8h}, [x0]
    abs         v18.8h, v16.8h
    abs         v19.8h, v17.8h
    QUANT_TWO   v0.8h,  v1.8h,  v2,  v3,  v7.16b
    uqxtn       v4.8b,  v4.8h
    uqxtn       v7.8b,  v7.8h
    uqxtn       v6.8b,  v6.8h
    uqxtn       v5.8b,  v5.8h
    fmov        x7,  d7
    fmov        x6,  d6
    fmov        x5,  d5
    fmov        x4,  d4
    mov         w0,  #0
    tst         x7,  x7
    cinc        w0,  w0,  ne
    lsl         w0,  w0,  #1
    tst         x6,  x6
    cinc        w0,  w0,  ne
    lsl         w0,  w0,  #1
    tst         x5,  x5
    cinc        w0,  w0,  ne
    lsl         w0,  w0,  #1
    tst         x4,  x4
    cinc        w0,  w0,  ne
    ret
endfunc

// quant_8x8( int16_t dct[64], uint16_t mf[64], uint16_t bias[64] )
function x264_quant_8x8_neon, export=1
    ld1        {v16.8h,v17.8h}, [x0]
    abs         v18.8h, v16.8h
    abs         v19.8h, v17.8h
    ld1        {v0.8h,v1.8h}, [x2], #32
    ld1        {v2.8h,v3.8h}, [x1], #32
    QUANT_TWO   v0.8h,  v1.8h,  v2,  v3,  v4.16b
.rept 3
    ld1        {v16.8h,v17.8h}, [x0]
    abs         v18.8h, v16.8h
    abs         v19.8h, v17.8h
    ld1        {v0.8h,v1.8h}, [x2], #32
    ld1        {v2.8h,v3.8h}, [x1], #32
    QUANT_TWO   v0.8h,  v1.8h,  v2,  v3,  v5.16b
    orr         v4.16b, v4.16b, v5.16b
.endr
    uqxtn       v0.8b,  v4.8h
    QUANT_END   d0
endfunc

.macro DEQUANT_START mf_size offset dc=no
    mov         w3,  #0x2b
    mul         w3,  w3,  w2
    lsr         w3,  w3,  #8            // i_qbits = i_qp / 6
    add         w5,  w3,  w3,  lsl #1
    sub         w2,  w2,  w5,  lsl #1   // i_mf = i_qp % 6
    lsl         w2,  w2,  #\mf_size
.ifc \dc,no
    add         x1,  x1,  w2, sxtw      // dequant_mf[i_mf]
.else
    ldr         x1, [x1,  w2, sxtw]     // dequant_mf[i_mf][0][0]
.endif
    subs        w3,  w3,  #\offset      // 6 for 8x8
.endm

// dequant_4x4( int16_t dct[16], int dequant_mf[6][16], int i_qp )
.macro DEQUANT size bits
function x264_dequant_\size\()_neon, export=1
    DEQUANT_START \bits+2, \bits
.ifc \size, 8x8
    mov         w2,  #4
.endif
    b.lt        dequant_\size\()_rshift

    dup         v31.8h, w3
dequant_\size\()_lshift_loop:
.ifc \size, 8x8
    subs        w2,  w2,  #1
.endif
    ld1        {v16.4s}, [x1], #16
    ld1        {v17.4s}, [x1], #16
    sqxtn       v2.4h,  v16.4s
    ld1        {v18.4s}, [x1], #16
    sqxtn2      v2.8h,  v17.4s
    ld1        {v19.4s}, [x1], #16
    sqxtn       v3.4h,  v18.4s
    ld1        {v0.8h,v1.8h}, [x0]
    sqxtn2      v3.8h,  v19.4s
    mul         v0.8h,  v0.8h,  v2.8h
    mul         v1.8h,  v1.8h,  v3.8h
    sshl        v0.8h,  v0.8h,  v31.8h
    sshl        v1.8h,  v1.8h,  v31.8h
    st1        {v0.8h,v1.8h}, [x0], #32
.ifc \size, 8x8
    b.gt        dequant_\size\()_lshift_loop
.endif
    ret

dequant_\size\()_rshift:
    dup         v31.4s, w3
    neg         w3,  w3
    mov         w5,  #1
    sub         w3,  w3,  #1
    lsl         w5,  w5,  w3

.ifc \size, 8x8
dequant_\size\()_rshift_loop:
    subs        w2,  w2,  #1
.endif
    ld1        {v16.4s}, [x1], #16
    ld1        {v17.4s}, [x1], #16
    sqxtn       v2.4h,  v16.4s
    ld1        {v18.4s}, [x1], #16
    dup         v16.4s, w5
    sqxtn2      v2.8h,  v17.4s
    ld1        {v19.4s}, [x1], #16
    dup         v17.4s, w5
    sqxtn       v3.4h,  v18.4s
    ld1        {v0.8h,v1.8h}, [x0]
    dup         v18.4s, w5
    sqxtn2      v3.8h,  v19.4s
    dup         v19.4s, w5

    smlal       v16.4s, v0.4h,  v2.4h
    smlal2      v17.4s, v0.8h,  v2.8h
    smlal       v18.4s, v1.4h,  v3.4h
    smlal2      v19.4s, v1.8h,  v3.8h
    sshl        v16.4s, v16.4s, v31.4s
    sshl        v17.4s, v17.4s, v31.4s
    sshl        v18.4s, v18.4s, v31.4s
    sshl        v19.4s, v19.4s, v31.4s

    sqxtn       v0.4h,  v16.4s
    sqxtn2      v0.8h,  v17.4s
    sqxtn       v1.4h,  v18.4s
    sqxtn2      v1.8h,  v19.4s
    st1        {v0.8h,v1.8h}, [x0], #32
.ifc \size, 8x8
    b.gt        dequant_\size\()_rshift_loop
.endif
    ret
endfunc
.endm

DEQUANT 4x4, 4
DEQUANT 8x8, 6

// dequant_4x4_dc( int16_t dct[16], int dequant_mf[6][16], int i_qp )
function x264_dequant_4x4_dc_neon, export=1
    DEQUANT_START 6, 6, yes
    b.lt        dequant_4x4_dc_rshift

    lsl         w1,  w1,  w3
    dup         v2.8h,  w1
    ld1        {v0.8h,v1.8h},   [x0]

    mul         v0.8h,  v0.8h,  v2.8h
    mul         v1.8h,  v1.8h,  v2.8h
    st1        {v0.8h,v1.8h},   [x0]
    ret

dequant_4x4_dc_rshift:
    dup         v4.8h,  w1
    dup         v3.4s, w3
    neg         w3,  w3
    mov         w5,  #1
    sub         w3,  w3,  #1
    lsl         w5,  w5,  w3

    dup         v16.4s, w5
    dup         v17.4s, w5
    ld1        {v0.8h,v1.8h}, [x0]
    dup         v18.4s, w5
    dup         v19.4s, w5

    smlal       v16.4s, v0.4h,  v4.4h
    smlal2      v17.4s, v0.8h,  v4.8h
    smlal       v18.4s, v1.4h,  v4.4h
    smlal2      v19.4s, v1.8h,  v4.8h
    sshl        v16.4s, v16.4s, v3.4s
    sshl        v17.4s, v17.4s, v3.4s
    sshl        v18.4s, v18.4s, v3.4s
    sshl        v19.4s, v19.4s, v3.4s

    sqxtn       v0.4h,  v16.4s
    sqxtn2      v0.8h,  v17.4s
    sqxtn       v1.4h,  v18.4s
    sqxtn2      v1.8h,  v19.4s
    st1        {v0.8h,v1.8h}, [x0]
    ret
endfunc

// int coeff_last( int16_t *l )
function x264_coeff_last4_aarch64, export=1
    ldr         x2,  [x0]
    mov         w4,  #3
    clz         x0,  x2
    sub         w0,  w4,  w0, lsr #4
    ret
endfunc

function x264_coeff_last8_aarch64, export=1
    ldr         x3,  [x0, #8]
    mov         w4,  #7
    clz         x2,  x3
    cmp         w2,  #64
    b.ne        1f
    ldr         x3,  [x0]
    sub         w4,  w4,  #4
    clz         x2,  x3
1:
    sub         w0,  w4,  w2, lsr #4
    ret
endfunc

.macro COEFF_LAST_1x size
function x264_coeff_last\size\()_neon, export=1
.if \size == 15
    sub         x0,  x0,  #2
.endif
    ld1        {v0.8h,v1.8h}, [x0]
    uqxtn       v0.8b,  v0.8h
    uqxtn2      v0.16b, v1.8h
    cmtst       v0.16b, v0.16b, v0.16b
    shrn        v0.8b,  v0.8h,  #4
    fmov        x1,  d0
    mov         w3,  #\size - 1
    clz         x2,  x1
    sub         w0,  w3,  w2, lsr #2
    ret
endfunc
.endm

COEFF_LAST_1x 15
COEFF_LAST_1x 16

function x264_coeff_last64_neon, export=1
    ld1        {v0.8h,v1.8h,v2.8h,v3.8h}, [x0], 64
    movi        v31.8h,  #8
    movi        v30.8h,  #1
    uqxtn       v0.8b,  v0.8h
    uqxtn2      v0.16b, v1.8h
    ld1        {v4.8h,v5.8h,v6.8h,v7.8h}, [x0], 64
    uqxtn       v1.8b,  v2.8h
    uqxtn2      v1.16b, v3.8h
    uqxtn       v2.8b,  v4.8h
    uqxtn2      v2.16b, v5.8h
    uqxtn       v3.8b,  v6.8h
    uqxtn2      v3.16b, v7.8h

    cmtst       v0.16b, v0.16b, v0.16b
    cmtst       v1.16b, v1.16b, v1.16b
    cmtst       v2.16b, v2.16b, v2.16b
    cmtst       v3.16b, v3.16b, v3.16b

    shrn        v0.8b,  v0.8h,  #4
    shrn2       v0.16b, v1.8h,  #4
    shrn        v1.8b,  v2.8h,  #4
    shrn2       v1.16b, v3.8h,  #4

    clz         v0.4s,  v0.4s
    clz         v1.4s,  v1.4s

    shrn        v0.4h,  v0.4s,  #2
    shrn2       v0.8h,  v1.4s,  #2

    sub         v0.8h,  v31.8h,  v0.8h
    sshl        v0.8h,  v30.8h,  v0.8h
    shrn        v0.8b,  v0.8h,  #1

    fmov        x2,  d0
    mov         w3,  #63
    clz         x2,  x2
    sub         w0,  w3,  w2
    ret
endfunc
