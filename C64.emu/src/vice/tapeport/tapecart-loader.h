/*   tapecart - a tape port storage pod for the C64
 *
 *   Copyright (C) 2013-2017  Ingo Korb <ingo@akana.de>
 *   All rights reserved.
 *   Idea by enthusi
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 *   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *   ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 *   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *   SUCH DAMAGE.
 *
 *
 *   loader.asm: Two-bit fastloader for the C64 tape port
 *
 *   Translated to C "macro assembler" by Ingo Korb
 */


#define ASMARG_16BIT(value) (value) & 0xff, ((value) >> 8) & 0xff

#define ASM_and_imm(val)    0x29, (val)
#define ASM_bcc(offset)     0x90, (offset & 0xff)
#define ASM_bit_zp(addr)    0x24, (addr)
#define ASM_bne(offset)     0xd0, (offset & 0xff)
#define ASM_clc             0x18
#define ASM_cli             0x58
#define ASM_cmp_zp(addr)    0xc5, (addr)
#define ASM_cpx_zp(addr)    0xe4, (addr)
#define ASM_dex             0xca
#define ASM_dey             0x88
#define ASM_eor_zp(addr)    0x45, (addr)
#define ASM_inc_zp(addr)    0xe6, (addr)
#define ASM_inx             0xe8
#define ASM_iny             0xc8
#define ASM_jmp_ind(addr)   0x6c, ASMARG_16BIT(addr)
#define ASM_jsr(addr)       0x20, ASMARG_16BIT(addr)
#define ASM_lda_imm(val)    0xa9, (val)
#define ASM_lda_zp(addr)    0xa5, (addr)
#define ASM_ldx_imm(val)    0xa2, (val)
#define ASM_ldx_zp(addr)    0xa6, (addr)
#define ASM_ldy_imm(val)    0xa0, (val)
#define ASM_lsr             0x4a
#define ASM_nop             0xea
#define ASM_ora_abs_x(addr) 0x1d, ASMARG_16BIT(addr)
#define ASM_ora_imm(val)    0x09, (val)
#define ASM_rol_abs(addr)   0x2e, ASMARG_16BIT(addr)
#define ASM_ror_abs(addr)   0x6e, ASMARG_16BIT(addr)
#define ASM_rts             0x60
#define ASM_sec             0x38
#define ASM_sei             0x78
#define ASM_sta_abs_x(addr) 0x99, ASMARG_16BIT(addr)
#define ASM_sta_ind_y(addr) 0x91, (addr)
#define ASM_sta_zp(addr)    0x85, (addr)
#define ASM_stx_zp(addr)    0x86, (addr)
#define ASM_sty_zp(addr)    0x84, (addr)
#define ASM_tax             0xaa


#define LOADER_CALLADDR        0xaa
#define LOADER_ENDADDR         0xac
#define LOADER_LOADADDR        0xae
#define LOADER_INFO_BLOCK_ADDR LOADER_CALLADDR
#define LOADER_INFO_BLOCK_SIZE 6

#define LOADER_SUB_GETBYTE     0x03b2
#define LOADER_NIBBLETAB       0x03e3


/* * = $0351 */
    ASM_ror_abs(0xd011),  /* blank screen */
    ASM_sei,
    ASM_clc,
    ASM_ldy_imm(16),      /* 16 bits */
    ASM_ldx_imm(0),
    ASM_stx_zp(0xc6),     /* clear keyboard buffer */
/*handshake_loop:*/
    ASM_lda_imm(0x27),    /* motor off, write low */
    ASM_rol_abs(0x03f3),  /* read top bit of signature byte (handshk) */
    ASM_bcc(2),           /* skip next instruction */
    ASM_ora_imm(0x08),    /* set write high */
    ASM_sta_zp(0x01),

/* delay loop because motor line is heavily RC-filtered */
    ASM_dex,
    ASM_nop,
    ASM_bne(-4),

    ASM_and_imm(0xdf),    /* motor on, keep write */
    ASM_sta_zp(0x01),

/* another delay loop, turning motor on is faster */
    ASM_dex,
    ASM_bne(-3),

    ASM_dey,
    ASM_bne(-25),         /* to handshake_loop until all bits are sent */

    ASM_lda_imm(0x30),    /* motor off, write low, all RAM */
    ASM_sta_zp(0x01),

/* read info block */
    ASM_ldy_imm(256 - LOADER_INFO_BLOCK_SIZE),
/* [037b] info_loop:*/
    ASM_jsr(LOADER_SUB_GETBYTE),
    ASM_sta_abs_x(LOADER_INFO_BLOCK_ADDR - (256 - LOADER_INFO_BLOCK_SIZE)),
    ASM_iny,
    ASM_bne(-9),          /* to info_loop */

/* [0384] data_loop:*/
    ASM_jsr(LOADER_SUB_GETBYTE),
    ASM_sta_ind_y(LOADER_LOADADDR),
    ASM_inc_zp(LOADER_LOADADDR),
    ASM_bne(2),           /* skip high-inc if no overflow */
    ASM_inc_zp(LOADER_LOADADDR + 1),

/* check if at end of file */
    ASM_lda_zp(LOADER_LOADADDR),
    ASM_cmp_zp(LOADER_ENDADDR),
    ASM_bne(-17),         /* to data_loop */

    ASM_ldx_zp(LOADER_LOADADDR + 1),
    ASM_cpx_zp(LOADER_ENDADDR + 1),
    ASM_bne(-23),         /* to data_loop */

/* load complete, fix system state and start */
    ASM_ldy_imm(0x37),    /* turn on ROMs */
    ASM_sty_zp(0x01),

    ASM_sec,
    ASM_rol_abs(0xd011),  /* un-blank screen */

    ASM_stx_zp(0x2e),     /* update BASIC pointers */
    ASM_stx_zp(0x30),
    ASM_sta_zp(0x2d),
    ASM_sta_zp(0x2f),

    ASM_cli,
    ASM_jsr(0xe453),      /* restore vectors */
    ASM_jmp_ind(LOADER_CALLADDR),

/* [03b2] getbyte -- returns byte in A, preserves Y */

    /* wait until tapecart is ready (sense low) */
    ASM_lda_imm(0x10),
    ASM_bit_zp(0x01),
    ASM_bne(-4),

    /* signal ready-ness */
    ASM_ldx_imm(0x38),
    ASM_lda_imm(0x27),
    ASM_stx_zp(0x01),     /* set write high */
    ASM_sta_zp(0x00),     /* 3 - switch write to input */
    ASM_nop,              /* 2 - delay */

    /* receive byte */
    ASM_lda_zp(0x01),     /* 3 - read bits 5+4 */
    ASM_and_imm(0x18),    /* 2 - mask */
    ASM_lsr,              /* 2 - shift down */
    ASM_lsr,              /* 2 */
    ASM_eor_zp(0x01),     /* 3 - read bits 7+6 */
    ASM_lsr,              /* 2 */
    ASM_and_imm(0x0f),    /* 2 - mask */
    ASM_tax,              /* 2 - remember value */

    ASM_lda_zp(0x01),     /* 3 - read bits 1+0 */
    ASM_and_imm(0x18),    /* 2 - mask */
    ASM_lsr,              /* 2 - shift down */
    ASM_lsr,              /* 2 */
    ASM_eor_zp(0x01),     /* 3 - read bits 3+2 */
    ASM_lsr,              /* 2 */
    ASM_and_imm(0x0f),    /* 2 - mask */
    ASM_ora_abs_x(LOADER_NIBBLETAB), /* 4 - add upper nibble */

    ASM_ldx_imm(0x2f),    /* 2 - switch write to output */
    ASM_stx_zp(0x00),     /* 3 */
    ASM_inx,              /* set write low again */
    ASM_stx_zp(0x01),

    ASM_rts,

/*nibbletab:*/
    0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
    0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0,

/*handshk:*/
    0xca                  /* handshake value seed */
