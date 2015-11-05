/*
 * Copyright 2001 by Arto Salmi and Joze Fabcic
 * Copyright 2006, 2007 by Brian Dominy <brian@oddchange.com>
 *
 * This file is part of GCC6809.
 * This file is part of VICE.
 *
 * VICE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * VICE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with VICE; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "vice.h"

#include <stdarg.h>

#include "6809.h"
#include "alarm.h"
#include "h6809regs.h"
#include "interrupt.h"
#include "monitor.h"
#include "petmem.h"
#include "snapshot.h"
#include "machine.h"

#define CLK maincpu_clk
#define CPU_INT_STATUS maincpu_int_status
#define ALARM_CONTEXT maincpu_alarm_context

/* ------------------------------------------------------------------------- */
/* Hook for additional delay.  */

#ifndef CPU_DELAY_CLK
#define CPU_DELAY_CLK
#endif

#ifndef CPU_REFRESH_CLK
#define CPU_REFRESH_CLK
#endif

/*
 * If the 6809 is reset, return from its main loop.
 * We come back to the petcpu.c DMA_ON_RESET macro which again may
 * re-select the 6809, if the CPU switch is set for that.
 */
#ifndef DMA_ON_RESET
#define DMA_ON_RESET    return
#endif

#ifndef DMA_FUNC
#define DMA_FUNC
#endif

#define CALLER  e_comp_space
#define LAST_OPCODE_ADDR iPC
#define GLOBAL_REGS     h6809_regs

h6809_regs_t h6809_regs;

#ifdef H6309
#define CLK_ADD(a, b) CLK += (!H6309_NATIVE_MODE()) ? a : b
#else
#define CLK_ADD(a, b) CLK += a
#endif

/* Export the local version of the registers.  */
#define EXPORT_REGISTERS()             \
    do {                               \
        GLOBAL_REGS.reg_x = get_x();   \
        GLOBAL_REGS.reg_y = get_y();   \
        GLOBAL_REGS.reg_u = get_u();   \
        GLOBAL_REGS.reg_s = get_s();   \
        GLOBAL_REGS.reg_pc = get_pc(); \
        GLOBAL_REGS.reg_dp = get_dp(); \
        GLOBAL_REGS.reg_cc = get_cc(); \
        GLOBAL_REGS.reg_a = get_a();   \
        GLOBAL_REGS.reg_b = get_b();   \
    } while (0)

/* Import the public version of the registers.  */
#define IMPORT_REGISTERS()          \
    do {                            \
        set_x(GLOBAL_REGS.reg_x);   \
        set_y(GLOBAL_REGS.reg_y);   \
        set_u(GLOBAL_REGS.reg_u);   \
        set_s(GLOBAL_REGS.reg_s);   \
        set_pc(GLOBAL_REGS.reg_pc); \
        set_dp(GLOBAL_REGS.reg_dp); \
        set_cc(GLOBAL_REGS.reg_cc); \
        set_a(GLOBAL_REGS.reg_a);   \
        set_b(GLOBAL_REGS.reg_b);   \
    } while (0)
#define JUMP(pc)

#define DO_INTERRUPT(int_kind)                                             \
  do {                                                                     \
        BYTE ik = (int_kind);                                              \
        if (ik & (IK_TRAP | IK_RESET)) {                                   \
            if (ik & IK_TRAP) {                                            \
                EXPORT_REGISTERS();                                        \
                interrupt_do_trap(CPU_INT_STATUS, (WORD)PC);               \
                IMPORT_REGISTERS();                                        \
                if (CPU_INT_STATUS->global_pending_int & IK_RESET) {       \
                    ik |= IK_RESET;                                        \
                }                                                          \
            }                                                              \
            if (ik & IK_RESET) {                                           \
                interrupt_ack_reset(CPU_INT_STATUS);                       \
                cpu6809_reset();                                           \
                DMA_ON_RESET;                                              \
            }                                                              \
        }                                                                  \
        if (ik & (IK_MONITOR | IK_DMA)) {                                  \
            if (ik & IK_MONITOR) {                                         \
                if (monitor_force_import(CALLER)) {                        \
                    IMPORT_REGISTERS();                                    \
                }                                                          \
                if (monitor_mask[CALLER]) {                                \
                    EXPORT_REGISTERS();                                    \
                }                                                          \
                if (monitor_mask[CALLER] & (MI_STEP)) {                    \
                    monitor_check_icount((WORD)PC);                        \
                    IMPORT_REGISTERS();                                    \
                }                                                          \
                if (monitor_mask[CALLER] & (MI_BREAK)) {                   \
                    if (monitor_check_breakpoints(CALLER, (WORD)PC)) {     \
                        monitor_startup(CALLER);                           \
                        IMPORT_REGISTERS();                                \
                    }                                                      \
                }                                                          \
                if (monitor_mask[CALLER] & (MI_WATCH)) {                   \
                    monitor_check_watchpoints(LAST_OPCODE_ADDR, (WORD)PC); \
                    IMPORT_REGISTERS();                                    \
                }                                                          \
            }                                                              \
            if (ik & IK_DMA) {                                             \
                EXPORT_REGISTERS();                                        \
                DMA_FUNC;                                                  \
                interrupt_ack_dma(CPU_INT_STATUS);                         \
                IMPORT_REGISTERS();                                        \
                JUMP(PC);                                                  \
            }                                                              \
        }                                                                  \
        if (ik & IK_NMI) {                                                 \
            request_nmi(0);                                                \
        } else if (ik & IK_IRQ) {                                          \
            req_irq(0);                                                    \
        }                                                                  \
} while (0)

#define JAM(INSTR)                                               \
    do {                                                         \
        unsigned int tmp;                                        \
                                                                 \
        EXPORT_REGISTERS();                                      \
        tmp = machine_jam("   6809: " INSTR " at $%04X   ", PC); \
        switch (tmp) {                                           \
            case JAM_RESET:                                      \
                DO_INTERRUPT(IK_RESET);                          \
                break;                                           \
            case JAM_HARD_RESET:                                 \
                mem_powerup();                                   \
                DO_INTERRUPT(IK_RESET);                          \
                break;                                           \
            case JAM_MONITOR:                                    \
                monitor_startup(e_comp_space);                   \
                IMPORT_REGISTERS();                              \
                break;                                           \
            default:                                             \
                CLK++;                                           \
        }                                                        \
    } while (0)

#ifdef LAST_OPCODE_ADDR
#define SET_LAST_ADDR(x) LAST_OPCODE_ADDR = (x)
#else
#error "please define LAST_OPCODE_ADDR"
#endif

union regs {
    DWORD reg_l;
    WORD reg_s[2];
    BYTE reg_c[4];
} regs6309;

#define Q regs6309.reg_l
#ifndef WORDS_BIGENDIAN
#define W regs6309.reg_s[0]
#define D regs6309.reg_s[1]
#define F regs6309.reg_c[0]
#define E regs6309.reg_c[1]
#define B regs6309.reg_c[2]
#define A regs6309.reg_c[3]
#else
#define W regs6309.reg_s[1]
#define D regs6309.reg_s[0]
#define F regs6309.reg_c[3]
#define E regs6309.reg_c[2]
#define B regs6309.reg_c[1]
#define A regs6309.reg_c[0]
#endif

static WORD X, Y, S, U, DP, PC, iPC;
static BYTE EFI;
static DWORD H, N, Z, OV, C;

#ifdef H6309
static BYTE MD;
static WORD V;

#define MD_NATIVE        0x01   /* if 1, execute in 6309 mode */
#define MD_FIRQ_LIKE_IRQ 0x02   /* if 1, FIRQ acts like IRQ */
#define MD_ILL           0x40   /* illegal instruction */
#define MD_DBZ           0x80   /* divide by zero */

#define H6309_NATIVE_MODE() (MD & 1)
#endif /* H6309 */

/* #define FULL6809 */

static WORD ea = 0;
static unsigned int irqs_pending = 0;
static unsigned int firqs_pending = 0;
static unsigned int cc_changed = 0;

static WORD *index_regs[4] = { &X, &Y, &U, &S };

extern void nmi(void);
extern void irq(void);
extern void firq(void);

void request_nmi(unsigned int source)
{
    /* If the interrupt is not masked, generate
     * IRQ immediately.  Else, mark it pending and
     * we'll check it later when the flags change.
     */
    nmi();
}

void req_irq(unsigned int source)
{
    /* If the interrupt is not masked, generate
     * IRQ immediately.  Else, mark it pending and
     * we'll check it later when the flags change.
     */
    irqs_pending |= (1 << source);
    if (!(EFI & I_FLAG)) {
        irq();
    }
}

void release_irq(unsigned int source)
{
    irqs_pending &= ~(1 << source);
}


void request_firq (unsigned int source)
{
    /* If the interrupt is not masked, generate
     * IRQ immediately.  Else, mark it pending and
     * we'll check it later when the flags change.
     */
    firqs_pending |= (1 << source);
    if (!(EFI & F_FLAG)) {
        firq();
    }
}

void release_firq(unsigned int source)
{
    firqs_pending &= ~(1 << source);
}

void sim_error(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    fprintf(stderr, "m6809-run: (at PC=%04X) ", iPC);
    vfprintf(stderr, format, ap);
    va_end(ap);
}

static inline BYTE imm_byte(void)
{
    BYTE val = read8(PC);

    PC++;
    return val;
}

static inline WORD imm_word(void)
{
    WORD val = read16(PC);

    PC += 2;
    return val;
}

#ifdef H6309
static inline DWORD imm_dword(void)
{
    DWORD val = read32(PC);

    PC += 4;
    return val;
}
#endif

static void WRMEM(WORD addr, BYTE data)
{
    write8(addr, data);
    CLK++;
}

static void WRMEM16(WORD addr, WORD data)
{
    write8(addr, (BYTE)(data >> 8));
    CLK++;
    write8((WORD)(addr + 1), (BYTE)(data & 0xff));
    CLK++;
}

#ifdef H6309
static void WRMEM32(WORD addr, DWORD data)
{
    write8(addr, (BYTE)(data >> 24));
    CLK++;
    write8((WORD)(addr + 1), (BYTE)((data >> 16) & 0xff));
    CLK++;
    write8((WORD)(addr + 2), (BYTE)((data >> 8) & 0xff));
    CLK++;
    write8((WORD)(addr + 3), (BYTE)(data & 0xff));
    CLK++;
}
#endif

static BYTE RDMEM(WORD addr)
{
    BYTE val = read8(addr);

    CLK++;
    return val;
}

static WORD RDMEM16(WORD addr)
{
    WORD val = read8(addr) << 8;

    CLK++;
    val |= read8((WORD)(addr + 1));
    CLK++;
    return val;
}

#ifdef H6309
static DWORD RDMEM32(WORD addr)
{
    DWORD val = read8(addr) << 24;

    val |= read8((WORD)(addr + 1)) << 16;
    CLK++;
    val |= read8((WORD)(addr + 2)) << 8;
    CLK++;
    val |= read8((WORD)(addr + 3));
    CLK++;

    return val;
}
#endif

static void write_stack(WORD addr, BYTE data)
{
    write8(addr, data);
    CLK++;
}

static void write_stack16(WORD addr, WORD data)
{
    write8((WORD)(addr + 1), (BYTE)(data & 0xff));
    CLK++;
    write8(addr, (BYTE)(data >> 8));
    CLK++;
}

static BYTE read_stack(WORD addr)
{
    BYTE val = read8(addr);

    CLK++;
    return val;
}

static WORD read_stack16(WORD addr)
{
    WORD val = read8(addr) << 8;

    CLK++;
    val |= read8((WORD)(addr + 1));
    return val;
}

static void direct(void)
{
    ea = RDMEM(PC) | DP;
    PC++;
}

static void indexed(void)
{
    BYTE post = imm_byte();
    WORD *R = index_regs[(post >> 5) & 0x3];

    if (post & 0x80) {
        switch (post & 0x1f) {
            case 0x00:  /* ,R+ */
                ea = *R;
                *R += 1;
                CLK_ADD(2, 1);
                break;
            case 0x01:  /* ,R++ */
                ea = *R;
                *R += 2;
                CLK_ADD(3, 2);
                break;
            case 0x02:  /* ,-R */
                *R -= 1;
                ea = *R;
                CLK_ADD(2, 1);
                break;
            case 0x03:  /* ,--R */
                *R -= 2;
                ea = *R;
                CLK_ADD(3, 2);
                break;
            case 0x04:  /* ,R */
                ea = *R;
                break;
            case 0x05:  /* B,R */
                ea = *R + (INT8)B;
                CLK++;
                break;
            case 0x06:  /* A,R */
                ea = *R + (INT8)A;
                CLK++;
                break;
#ifdef FULL6809
            case 0x07:  /* ,R (UNDOC) */
                ea = *R;
                break;
#endif
#ifdef H6309
            case 0x07:  /* E,R */
                ea = *R + (INT8)E;
                CLK++;
                break;
#endif
            case 0x08:  /* 8bit,R */
                ea = *R + (INT8)imm_byte();
                break;
            case 0x09:  /* 16bit,R */
                ea = *R + imm_word();
                CLK_ADD(2, 1);
                break;
#ifdef FULL6809
            case 0x0a:  /* ,PC | 0xff (UNDOC) */
                ea = PC | 0xff;
                break;
#endif
#ifdef H6309
            case 0x0a:  /* F,R */
                ea = *R + (INT8)F;
                CLK++;
                break;
#endif
            case 0x0b:  /* D,R */
                ea = *R + D;
                CLK_ADD(4, 2);
                break;
            case 0x0c:  /* 8bit,PC */
                ea = (INT8)imm_byte();
                ea += PC;
                break;
            case 0x0d:  /* 16bit,PC */
                ea = imm_word();
                ea += PC;
                CLK_ADD(3, 1);
                break;
#ifdef H6309
            case 0x0e:  /* W,R */
                ea = *R + W;
                CLK_ADD(4, 1);
                break;
#endif
#ifdef FULL6809
            case 0x0f:  /* ,extended (UNDOC) */
                ea = imm_word();
                break;
#endif
#ifdef H6309
            case 0x0f:  /* relative to W */
                switch (post & 0x60) {
                    case 0x00:  /* ,W */
                        ea = W;
                        break;
                    case 0x20:  /* 16bit,W */
                        ea = imm_word();
                        ea += W;
                        CLK_ADD(3, 0);
                        break;
                    case 0x40:  /* ,W++ */
                        ea = W;
                        W += 2;
                        CLK_ADD(3, 1);
                        break;
                    case 0x60:  /* ,--W */
                        W -= 2;
                        ea = W;
                        CLK_ADD(3, 1);
                        break;
                }
                break;
#endif
#ifdef FULL6809
            case 0x10:  /* [,R+] (UNDOC) */
                ea = *R;
                *R += 1;
                CLK++;
                ea = RDMEM16(ea);
                break;
#endif
#ifdef H6309
            case 0x10:  /* indirect relative to W */
                switch (post & 0x60) {
                    case 0x00:  /* [,W] */
                        ea = W;
                        ea = RDMEM16(ea);
                        break;
                    case 0x20:  /* [16bit,W] */
                        ea = imm_word();
                        ea += W;
                        CLK++;
                        ea = RDMEM16(ea);
                        break;
                    case 0x40:  /* [,W++] */
                        ea = W;
                        W += 2;
                        CLK++;
                        ea = RDMEM16(ea);
                        break;
                    case 0x60:  /* [,--W] */
                        W -= 2;
                        ea = W;
                        CLK++;
                        ea = RDMEM16(ea);
                        break;
                }
                break;
#endif
            case 0x11:  /* [,R++] */
                ea = *R;
                *R += 2;
                CLK += 4;
                ea = RDMEM16(ea);
                break;
#if defined(FULL6809) || defined(H6309)
            case 0x12:  /* [,-R] (UNDOC) */
                *R -= 1;
                ea = *R;
                CLK += 3;
                ea = RDMEM16(ea);
                break;
#endif
            case 0x13:  /* [,--R] */
                *R -= 2;
                ea = *R;
                CLK += 4;
                ea = RDMEM16(ea);
                break;
            case 0x14:  /* [,R] */
                ea = *R;
                CLK++;
                ea = RDMEM16(ea);
                break;
            case 0x15:  /* [B,R] */
                ea = *R + (INT8)B;
                CLK += 2;
                ea = RDMEM16(ea);
                break;
            case 0x16:  /* [A,R] */
                ea = *R + (INT8)A;
                CLK += 2;
                ea = RDMEM16(ea);
                break;
#ifdef FULL6809
            case 0x17:  /* [,R] (UNDOC) */
                ea = *R;
                CLK++;
                ea = RDMEM16(ea);
                break;
#endif
#ifdef H6309
            case 0x17:  /* [E,R] */
                ea = *R + (INT8)E;
                ea = RDMEM16(ea);
                break;
#endif
            case 0x18:  /* [8bit,R] */
                ea = *R + (INT8)imm_byte();
                CLK++;
                ea = RDMEM16(ea);
                break;
            case 0x19:  /* [16bit,R] */
                ea = *R + imm_word();
                CLK += 3;
                ea = RDMEM16(ea);
                break;
#ifdef FULL6809
            case 0x1a:  /* [,PC | 0xff] (UNDOC) */
                ea = PC | 0xff;
                CLK++;
                ea = RDMEM16(ea);
                break;
#endif
#ifdef H6309
            case 0x1a:  /* [F,R] */
                ea = *R + (INT8)F;
                ea = RDMEM16(ea);
                break;
#endif
            case 0x1b:  /* [D,R] */
                ea = *R + D;
                CLK += 2;
                ea = RDMEM16(ea);
                break;
            case 0x1c:  /* [8bit,PC] */
                ea = (INT8)imm_byte();
                ea += PC;
                CLK++;
                ea = RDMEM16(ea);
                break;
            case 0x1d:  /* [16bit,PC] */
                ea = imm_word();
                ea += PC;
                CLK += 4;
                ea = RDMEM16(ea);
                break;
#ifdef H6309
            case 0x1e:  /* [W,R] */
                ea = *R + W;
                CLK += 2;
                ea = RDMEM16(ea);
                break;
#endif
            case 0x1f:  /* [16bit] */
                ea = imm_word();
                CLK++;
                ea = RDMEM16(ea);
                break;
            default:
                ea = 0;
                sim_error("invalid index post $%02X\n", post);
                break;
        }
    } else {
        if (post & 0x10) {
            ea = *R + (post | 0xfff0);
        } else {
            ea = *R + (post & 0x000f);
        }
        CLK++;
    }
}

static void extended(void)
{
    ea = RDMEM16(PC);
    PC += 2;
}

/* external register functions */

static BYTE get_a(void)
{
    return A;
}

static BYTE get_b(void)
{
    return B;
}

static BYTE get_dp(void)
{
    return DP >> 8;
}

static WORD get_x(void)
{
    return X;
}

static WORD get_y(void)
{
    return Y;
}

static WORD get_s(void)
{
    return S;
}

static WORD get_u(void)
{
    return U;
}

static WORD get_pc(void)
{
    return PC;
}

#if 0
/* unused ? */
static WORD get_d(void)
{
    return D;
}

static BYTE get_flags(void)
{
    return EFI;
}
#endif

#ifdef H6309
static BYTE get_e(void)
{
    return E;
}

static BYTE get_f(void)
{
    return F;
}

static WORD get_w(void)
{
    return W;
}

static DWORD get_q(void)
{
    return Q;
}

static WORD get_v(void)
{
    return V;
}

static BYTE get_md(void)
{
    return (MD & (MD_ILL | MD_DBZ));
}
#endif

static void set_a(BYTE val)
{
    A = val;
}

static void set_b(BYTE val)
{
    B = val;
}

static void set_dp(BYTE val)
{
    DP = val << 8;
}

static void set_x(WORD val)
{
    X = val;
}

static void set_y(WORD val)
{
    Y = val;
}

static void set_s(WORD val)
{
    S = val;
}

static void set_u(WORD val)
{
    U = val;
}

static void set_pc(WORD val)
{
    PC = val;
}

#if 0
/* unused ? */
static void set_d(WORD val)
{
    D = val;
}
#endif

#ifdef H6309
static void set_e(BYTE val)
{
    E = val;
}

static void set_f(BYTE val)
{
    F = val;
}

static void set_w(WORD val)
{
    W = val;
}

static void set_q(DWORD val)
{
    Q = val;
}

static void set_v(WORD val)
{
    V = val;
}

static void set_md(BYTE val)
{
    MD = (MD & (MD_ILL | MD_DBZ)) | (val & (MD_NATIVE | MD_FIRQ_LIKE_IRQ));
}
#endif


/* handle condition code register */

static BYTE get_cc(void)
{
    BYTE res = EFI & (E_FLAG | F_FLAG | I_FLAG);

    if (H & 0x10) {
        res |= H_FLAG;
    }
    if (N & 0x80) {
        res |= N_FLAG;
    }
    if (Z == 0) {
        res |= Z_FLAG;
    }
    if (OV & 0x80) {
        res |= V_FLAG;
    }
    if (C != 0) {
        res |= C_FLAG;
    }
    return res;
}

static void set_cc(BYTE arg)
{
    EFI = arg & (E_FLAG | F_FLAG | I_FLAG);
    H = (arg & H_FLAG ? 0x10 : 0);
    N = (arg & N_FLAG ? 0x80 : 0);
    Z = (~arg) & Z_FLAG;
    OV = (arg & V_FLAG ? 0x80 : 0);
    C = arg & C_FLAG;
    cc_changed = 1;
}

static int cc_modified(void)
{
    int interrupt_taken = 0;

    /* Check for pending interrupts */
    if (firqs_pending && !(EFI & F_FLAG)) {
        firq();
        interrupt_taken = 1;
    } else if (irqs_pending && !(EFI & I_FLAG)) {
        irq();
        interrupt_taken = 1;
    }
    cc_changed = 0;

    return interrupt_taken;
}

/* Undocumented: When the 6809 transfers an 8bit register
   to a 16bit register the LSB is filled with the 8bit
   register value and the MSB is filled with 0xff.
 */

static WORD get_reg(BYTE nro)
{
    WORD val = 0xffff;

    switch (nro) {
        case 0:
            val = D;
            break;
        case 1:
            val = X;
            break;
        case 2:
            val = Y;
            break;
        case 3:
            val = U;
            break;
        case 4:
            val = S;
            break;
        case 5:
            val = PC;
            break;
#ifdef H6309
        case 6:
            val = W;
            break;
        case 7:
            val = V;
            break;
#endif
        case 8:
            val = A & 0xffff;
            break;
        case 9:
            val = B & 0xffff;
            break;
        case 10:
            val = get_cc() & 0xffff;
            break;
        case 11:
            val = (DP >> 8) & 0xffff;
            break;
#ifdef H6309
        case 14:
            val = E & 0xffff;
            break;
        case 15:
            val = F & 0xffff;
            break;
#endif
    }
    return val;
}

static void set_reg(BYTE nro, WORD val)
{
    switch (nro) {
        case 0:
            D = val;
            break;
        case 1:
            X = val;
            break;
        case 2:
            Y = val;
            break;
        case 3:
            U = val;
            break;
        case 4:
            S = val;
            break;
        case 5:
            PC = val;
            break;
#ifdef H6309
        case 6:
            W = val;
            break;
        case 7:
            V = val;
            break;
#endif
        case 8:
            A = val & 0xff;
            break;
        case 9:
            B = val & 0xff;
            break;
        case 10:
            set_cc((BYTE)(val & 0xff));
            break;
        case 11:
            DP = (val & 0xff) << 8;
            break;
#ifdef H6309
        case 14:
            E = val & 0xff;
            break;
        case 15:
            F = val & 0xff;
            break;
#endif
    }
}

/* 8-Bit Accumulator and Memory Instructions */

static BYTE adc(BYTE arg, BYTE val, int CLK6809, int CLK6309)
{
    WORD res = arg + val + (C != 0);

    C = (res >> 1) & 0x80;
    N = Z = res &= 0xff;
    OV = H = arg ^ val ^ res ^ C;

    CLK_ADD(CLK6809, CLK6309);

    return (BYTE)res;
}

static BYTE add(BYTE arg, BYTE val, int CLK6809, int CLK6309)
{
    WORD res = arg + val;

    C = (res >> 1) & 0x80;
    N = Z = res &= 0xff;
    OV = H = arg ^ val ^ res ^ C;

    CLK_ADD(CLK6809, CLK6309);

    return (BYTE)res;
}

static BYTE and(BYTE arg, BYTE val, int CLK6809, int CLK6309)
{
    BYTE res = arg & val;

    N = Z = res;
    OV = 0;

    CLK_ADD(CLK6809, CLK6309);

    return res;
}

static BYTE asl(BYTE arg, int CLK6809, int CLK6309)  /* same as lsl */
{
    WORD res = arg << 1;

    C = res & 0x100;
    N = Z = res &= 0xff;
    OV = arg ^ res;
    CLK_ADD(CLK6809, CLK6309);

    return (BYTE)res;
}

static BYTE asr(BYTE arg, int CLK6809, int CLK6309)
{
    WORD res = (INT8)arg;

    C = res & 1;
    N = Z = res = (res >> 1) & 0xff;
    CLK_ADD(CLK6809, CLK6309);

    return (BYTE)res;
}

static void bit(BYTE arg, BYTE val, int CLK6809, int CLK6309)
{
    N = Z = arg & val;
    OV = 0;

    CLK_ADD(CLK6809, CLK6309);
}

static BYTE clr(BYTE arg, int CLK6809, int CLK6309)
{
    C = N = Z = OV = 0;
    CLK_ADD(CLK6809, CLK6309);

    return 0;
}

static void cmp(BYTE arg, BYTE val, int CLK6809, int CLK6309)
{
    WORD res = arg - val;

    C = res & 0x100;
    N = Z = res &= 0xff;
    OV = (arg ^ val) & (arg ^ res);

    CLK_ADD(CLK6809, CLK6309);
}

static BYTE com(BYTE arg, int CLK6809, int CLK6309)
{
    BYTE res = ~arg;

    N = Z = res;
    OV = 0;
    C = 1;
    CLK_ADD(CLK6809, CLK6309);

    return res;
}

static void daa(void)
{
    WORD res = A;
    BYTE msn = res & 0xf0;
    BYTE lsn = res & 0x0f;

    if (lsn > 0x09 || (H & 0x10)) {
        res += 0x06;
    }
    if ((msn > 0x90) || (C != 0) || (msn > 0x80 && lsn > 0x09)) {
        res += 0x60;
    }

    C |= (res & 0x100);
    N = Z = res &= 0xff;
    A = (BYTE)res;
    /* OV = 0;     V is undefined but some sources clear it anyway */
    /* Another remark: http://members.optushome.com.au/jekent/Spartan3/index.html
     * 5. DAA (Decimal Adjust Accumulator) should set the Negative (N) and Zero
     * Flags. It will also affect the Overflow (V) flag although the operation
     * is undefined in the M6809 Programming Reference Manual. It's anyone's
     * guess what DAA does to V although I found Exclusive ORing Bit 7 of the
     * original ACCA value with B7 of the Decimal Adjusted value and Exclusive
     * ORing that with the pre Decimal Adjust Carry input resulting in
     * something approximating what you find on an EF68A09P.
     */

    CLK_ADD(1, 0);
}

static BYTE dec(BYTE arg, int CLK6809, int CLK6309)
{
    BYTE res = arg - 1;

    N = Z = res;
    OV = arg & ~res;

    CLK_ADD(CLK6809, CLK6309);

    return res;
}

static BYTE eor(BYTE arg, BYTE val, int CLK6809, int CLK6309)
{
    BYTE res = arg ^ val;

    N = Z = res;
    OV = 0;

    CLK_ADD(CLK6809, CLK6309);

    return res;
}

static void exg(void)
{
    WORD tmp1 = 0xff;
    WORD tmp2 = 0xff;
    BYTE post = imm_byte();

    if (((post ^ (post << 4)) & 0x80) == 0) {
        tmp1 = get_reg((BYTE)(post >> 4));
        tmp2 = get_reg((BYTE)(post & 15));
    }

    set_reg((BYTE)(post & 15), tmp1);
    set_reg((BYTE)(post >> 4), tmp2);

    CLK_ADD(6, 3);
}

static BYTE inc(BYTE arg, int CLK6809, int CLK6309)
{
    BYTE res = arg + 1;

    N = Z = res;
    OV = ~arg & res;

    CLK_ADD(CLK6809, CLK6309);

    return res;
}

static BYTE ld(BYTE arg, int CLK6809, int CLK6309)
{
    N = Z = arg;
    OV = 0;

    CLK_ADD(CLK6809, CLK6309);

    return arg;
}

static BYTE lsr(BYTE arg, int CLK6809, int CLK6309)
{
    BYTE res = arg >> 1;

    N = 0;
    Z = res;
    C = arg & 1;

    CLK_ADD(CLK6809, CLK6309);

    return res;
}

static void mul(void)
{
    WORD res = A * B;

    Z = D = res;
    C = res & 0x80;
    CLK_ADD(10, 9);
}

static BYTE neg(BYTE arg, int CLK6809, int CLK6309)
{
    BYTE res = ~arg + 1;

    C = N = Z = res;
    OV = res & arg;

    CLK_ADD(CLK6809, CLK6309);

    return res;
}

static BYTE or(BYTE arg, BYTE val, int CLK6809, int CLK6309)
{
    BYTE res = arg | val;

    N = Z = res;
    OV = 0;

    CLK_ADD(CLK6809, CLK6309);

    return res;
}

static BYTE rol(BYTE arg, int CLK6809, int CLK6309)
{
    WORD res = (arg << 1) + (C != 0);

    C = res & 0x100;
    N = Z = res &= 0xff;
    OV = arg ^ res;

    CLK_ADD(CLK6809, CLK6309);

    return (BYTE)res;
}

static BYTE ror(BYTE arg, int CLK6809, int CLK6309)
{
    BYTE res = (arg >> 1) | ((C != 0) << 7);

    C = arg & 1;
    N = Z = res;

    CLK_ADD(CLK6809, CLK6309);

    return res;
}

static BYTE sbc(BYTE arg, BYTE val, int CLK6809, int CLK6309)
{
    WORD res = arg - val - (C != 0);

    C = res & 0x100;
    N = Z = res &= 0xff;
    OV = (arg ^ val) & (arg ^ res);

    CLK_ADD(CLK6809, CLK6309);

    return (BYTE)res;
}

static void st(BYTE arg, int CLK6809, int CLK6309)
{
    N = Z = arg;
    OV = 0;

    WRMEM(ea, arg);

    CLK_ADD(CLK6809, CLK6309);
}

static BYTE sub(BYTE arg, BYTE val, int CLK6809, int CLK6309)
{
    WORD res = arg - val;

    C = res & 0x100;
    N = Z = res &= 0xff;
    OV = (arg ^ val) & (arg ^ res);

    CLK_ADD(CLK6809, CLK6309);

    return (BYTE)res;
}

static void tst(BYTE arg, int CLK6809, int CLK6309)
{
    N = Z = arg;
    OV = 0;

    CLK_ADD(CLK6809, CLK6309);
}

static void tfr(void)
{
    WORD tmp1 = 0xff;
    BYTE post = imm_byte();

    if (((post ^ (post << 4)) & 0x80) == 0) {
        tmp1 = get_reg ((BYTE)(post >> 4));
    }

    set_reg((BYTE)(post & 15), tmp1);

    CLK_ADD(4, 2);
}

/* 16-Bit Accumulator Instructions */

static void abx(void)
{
    X += B;
    CLK_ADD(2, 0);
}

static WORD add16(WORD arg, WORD val, int CLK6809, int CLK6309)
{
    DWORD res = arg + val;

    C = (res >> 1) & 0x8000;
    Z = res &= 0xffff;
    N = res >> 8;
    OV = ((arg ^ res) & (val ^ res)) >> 8;

    CLK_ADD(CLK6809, CLK6309);

    return (WORD)res;
}

static void cmp16(WORD arg, WORD val, int CLK6809, int CLK6309)
{
    DWORD res = arg - val;

    C = res & 0x10000;
    Z = res &= 0xffff;
    N = res >> 8;
    OV = ((arg ^ val) & (arg ^ res)) >> 8;

    CLK_ADD(CLK6809, CLK6309);
}

static WORD ld16(WORD arg, int CLK6809, int CLK6309)
{
    Z = arg;
    N = arg >> 8;
    OV = 0;

    CLK_ADD(CLK6809, CLK6309);

    return arg;
}

static void sex(void)
{
    D = (INT8)B;

    Z = D;
    N = D >> 8;

    CLK_ADD(1, 0);
}

static void st16(WORD arg, int CLK6809, int CLK6309)
{
    Z = arg;
    N = arg >> 8;
    OV = 0;
    WRMEM16(ea, arg);

    CLK_ADD(CLK6809, CLK6309);
}

static WORD sub16(WORD arg, WORD val, int CLK6809, int CLK6309)
{
    DWORD res = arg - val;

    C = res & 0x10000;
    Z = res &= 0xffff;
    N = res >> 8;
    OV = ((arg ^ val) & (arg ^ res)) >> 8;

    CLK_ADD(CLK6809, CLK6309);

    return (WORD)res;
}

/* stack instructions */

static void pshs(void)
{
    BYTE post = imm_byte();

    CLK_ADD(3, 2);

    if (post & 0x80) {
        S -= 2;
        write_stack16(S, PC);
    }
    if (post & 0x40) {
        S -= 2;
        write_stack16(S, U);
    }
    if (post & 0x20) {
        S -= 2;
        write_stack16(S, Y);
    }
    if (post & 0x10) {
        S -= 2;
        write_stack16(S, X);
    }
    if (post & 0x08) {
        S--;
        write_stack(S, (BYTE)(DP >> 8));
    }
    if (post & 0x04) {
        S--;
        write_stack(S, B);
    }
    if (post & 0x02) {
        S--;
        write_stack(S, A);
    }
    if (post & 0x01) {
        S--;
        write_stack(S, get_cc());
    }
}

static void pshu(void)
{
    BYTE post = imm_byte();

    CLK_ADD(3, 2);

    if (post & 0x80) {
        U -= 2;
        write_stack16(U, PC);
    }
    if (post & 0x40) {
        U -= 2;
        write_stack16(U, S);
    }
    if (post & 0x20) {
        U -= 2;
        write_stack16(U, Y);
    }
    if (post & 0x10) {
        U -= 2;
        write_stack16(U, X);
    }
    if (post & 0x08) {
        U--;
        write_stack(U, (BYTE)(DP >> 8));
    }
    if (post & 0x04) {
        U--;
        write_stack(U, B);
    }
    if (post & 0x02) {
        U--;
        write_stack(U, A);
    }
    if (post & 0x01) {
        U--;
        write_stack(U, get_cc());
    }
}

static void puls(void)
{
    BYTE post = imm_byte();

    CLK_ADD(3, 2);

    if (post & 0x01) {
        set_cc(read_stack(S++));
    }
    if (post & 0x02) {
        A = read_stack(S++);
    }
    if (post & 0x04) {
        B = read_stack(S++);
    }
    if (post & 0x08) {
        DP = read_stack(S++) << 8;
    }
    if (post & 0x10) {
        X = read_stack16(S);
        S += 2;
    }
    if (post & 0x20) {
        Y = read_stack16(S);
        S += 2;
    }
    if (post & 0x40) {
        U = read_stack16(S);
        S += 2;
    }
    if (post & 0x80) {
        PC = read_stack16(S);
        S += 2;
    }
}

static void pulu(void)
{
    BYTE post = imm_byte();

    CLK_ADD(3, 2);

    if (post & 0x01) {
        set_cc(read_stack(U++));
    }
    if (post & 0x02) {
        A = read_stack(U++);
    }
    if (post & 0x04) {
        B = read_stack(U++);
    }
    if (post & 0x08) {
        DP = read_stack(U++) << 8;
    }
    if (post & 0x10) {
        X = read_stack16(U);
        U += 2;
    }
    if (post & 0x20) {
        Y = read_stack16(U);
        U += 2;
    }
    if (post & 0x40) {
        S = read_stack16(U);
        U += 2;
    }
    if (post & 0x80) {
        PC = read_stack16(U);
        U += 2;
    }
}

/* Miscellaneous Instructions */

static void jsr(void)
{
    S -= 2;
    write_stack16(S, PC);
    PC = ea;

    CLK_ADD(3, 2);
}

static void rti(void)
{
    CLK += 3;
    set_cc(read_stack(S));
    S++;

    if ((EFI & E_FLAG) != 0) {
        A = read_stack(S++);
        B = read_stack(S++);
#ifdef H6309
        if (H6309_NATIVE_MODE()) {
            E = read_stack(S++);
            F = read_stack(S++);
        }
#endif
        DP = read_stack(S++) << 8;
        X = read_stack16(S);
        S += 2;
        Y = read_stack16(S);
        S += 2;
        U = read_stack16(S);
        S += 2;
    }
    PC = read_stack16(S);
    S += 2;
}

static void rts(void)
{
    CLK_ADD(2, 1);

    PC = read_stack16(S);
    S += 2;
}

void nmi(void)
{
    EFI |= E_FLAG;
    S -= 2;
    write_stack16(S, PC);
    S -= 2;
    write_stack16(S, U);
    S -= 2;
    write_stack16(S, Y);
    S -= 2;
    write_stack16(S--, X);
    write_stack(S--, (BYTE)(DP >> 8));
#ifdef H6309
    if (H6309_NATIVE_MODE()) {
        write_stack(S--, F);
        write_stack(S--, E);
    }
#endif
    write_stack(S--, B);
    write_stack(S--, A);
    write_stack(S, get_cc());
    EFI |= I_FLAG;

    PC = read16(0xfffc);
}

void irq(void)
{
    EFI |= E_FLAG;
    S -= 2;
    write_stack16(S, PC);
    S -= 2;
    write_stack16(S, U);
    S -= 2;
    write_stack16(S, Y);
    S -= 2;
    write_stack16(S--, X);
    write_stack(S--, (BYTE)(DP >> 8));
#ifdef H6309
    if (H6309_NATIVE_MODE()) {
        write_stack(S--, F);
        write_stack(S--, E);
    }
#endif
    write_stack(S--, B);
    write_stack(S--, A);
    write_stack(S, get_cc());
    EFI |= I_FLAG;

    PC = read16(0xfff8);
    irqs_pending = 0;
}

void firq(void)
{
    EFI &= ~E_FLAG;
    S -= 2;
    write_stack16(S--, PC);
#ifdef H6309
    if (MD & MD_FIRQ_LIKE_IRQ) {
        S -= 2;
        write_stack16(S, U);
        S -= 2;
        write_stack16(S, Y);
        S -= 2;
        write_stack16(S--, X);
        write_stack(S--, (BYTE)(DP >> 8));
        if (H6309_NATIVE_MODE()) {
            write_stack(S--, F);
            write_stack(S--, E);
        }
        write_stack(S--, B);
        write_stack(S--, A);
    }
#endif
    write_stack(S, get_cc());

    EFI |= (I_FLAG | F_FLAG);

    PC = read16(0xfff6);
    firqs_pending = 0;
}

void swi(void)
{
    CLK += 6;
    EFI |= E_FLAG;
    S -= 2;
    write_stack16(S, PC);
    S -= 2;
    write_stack16(S, U);
    S -= 2;
    write_stack16(S, Y);
    S -= 2;
    write_stack16(S--, X);
    write_stack(S--, (BYTE)(DP >> 8));
#ifdef H6309
    if (H6309_NATIVE_MODE()) {
        write_stack(S--, F);
        write_stack(S--, E);
    }
#endif
    write_stack(S--, B);
    write_stack(S--, A);
    write_stack(S, get_cc());
    EFI |= (I_FLAG | F_FLAG);

    PC = read16(0xfffa);
}

void swi2(void)
{
    CLK += 6;
    EFI |= E_FLAG;
    S -= 2;
    write_stack16(S, PC);
    S -= 2;
    write_stack16(S, U);
    S -= 2;
    write_stack16(S, Y);
    S -= 2;
    write_stack16(S--, X);
    write_stack(S--, (BYTE)(DP >> 8));
#ifdef H6309
    if (H6309_NATIVE_MODE()) {
        write_stack(S--, F);
        write_stack(S--, E);
    }
#endif
    write_stack(S--, B);
    write_stack(S--, A);
    write_stack(S, get_cc());

    PC = read16(0xfff4);
}

void swi3(void)
{
    CLK += 6;
    EFI |= E_FLAG;
    S -= 2;
    write_stack16(S, PC);
    S -= 2;
    write_stack16(S, U);
    S -= 2;
    write_stack16(S, Y);
    S -= 2;
    write_stack16(S--, X);
    write_stack(S--, (BYTE)(DP >> 8));
#ifdef H6309
    if (H6309_NATIVE_MODE()) {
        write_stack(S--, F);
        write_stack(S--, E);
    }
#endif
    write_stack(S--, B);
    write_stack(S--, A);
    write_stack(S, get_cc());

    PC = read16(0xfff2);
}

#ifdef H6309
void opcode_trap(void)
{
    CLK += 6;
    EFI |= E_FLAG;
    MD &= MD_ILL;
    S -= 2;
    write_stack16(S, PC);
    S -= 2;
    write_stack16(S, U);
    S -= 2;
    write_stack16(S, Y);
    S -= 2;
    write_stack16(S--, X);
    write_stack(S--, (BYTE)(DP >> 8));
    if (H6309_NATIVE_MODE()) {
        write_stack(S--, F);
        write_stack(S--, E);
    }
    write_stack(S--, B);
    write_stack(S--, A);
    write_stack(S, get_cc());

    PC = read16(0xfff0);
}

void div0_trap(void)
{
    CLK += 6;
    EFI |= E_FLAG;
    MD &= MD_DBZ;
    S -= 2;
    write_stack16(S, PC);
    S -= 2;
    write_stack16(S, U);
    S -= 2;
    write_stack16(S, Y);
    S -= 2;
    write_stack16(S--, X);
    write_stack(S--, (BYTE)(DP >> 8));
    if (H6309_NATIVE_MODE()) {
        write_stack(S--, F);
        write_stack(S--, E);
    }
    write_stack(S--, B);
    write_stack(S--, A);
    write_stack(S, get_cc());

    PC = read16(0xfff0);
}
#endif

void cwai(struct interrupt_cpu_status_s *maincpu_int_status, alarm_context_t *maincpu_alarm_context)
{
    BYTE tmp = imm_byte();
    int taken;

    /* JAM("CWAI"); */
    set_cc((BYTE)(get_cc() & tmp));
    CLK++;
    taken = cc_modified();  /* may trigger interrupt immediately if pending */
#define TIME    14          /* saving registers takes at least this time */

    /*
     * Simulate waiting. This isn't cycle exact, since the real
     * instruction would start with saving all CPU registers before
     * waiting for the interrupt. To sort-of end up at the same total
     * time, deduct TIME from the target time to wait for.
     * Only advance the time, do not set it backward of course.
     * The code probably isn't the best way to wait for an IRQ.
     * Nevertheless, for Super-OS/9 this seems to do.
     *
     * NOTE: it seems that CWAI always takes the interrupt even if it
     * would otherwise be masked. That is not accounted for, since
     * the normal interrupt processing (incl. masking) is used.
     */
    while (!taken) {
        int pending = maincpu_int_status->global_pending_int & (IK_IRQ | IK_IRQPEND);
        if (pending) {
            break;
        } else {
            CLOCK newclock = alarm_context_next_pending_clk(maincpu_alarm_context) - TIME;
            if (newclock > CLK) {
                CLK = newclock;
            }
            alarm_context_dispatch(maincpu_alarm_context, CLK);
            taken = irqs_pending /*| firqs_pending*/;
        }
    }
}
#undef TIME

/* FIXME: cycle count */
void sync(void)
{
    CLK += 4;
    /*
     * NOTE: SYNC may or may not take the interrupt, depending on
     * whether it is masked. This version never takes the interrupt,
     * even if unmasked.
     */
    if (superpet_sync()) {
        JAM("SYNC");
    }
}

static void orcc(void)
{
    BYTE tmp = imm_byte();

    set_cc((BYTE)(get_cc() | tmp));

    CLK_ADD(1, 0);
}

static void andcc(void)
{
    BYTE tmp = imm_byte();

    set_cc((BYTE)(get_cc() & tmp));
    CLK++;
}

/* Branch Instructions */

#define cond_HI() ((Z != 0) && (C == 0))
#define cond_LS() ((Z == 0) || (C != 0))
#define cond_HS() (C == 0)
#define cond_LO() (C != 0)
#define cond_NE() (Z != 0)
#define cond_EQ() (Z == 0)
#define cond_VC() ((OV & 0x80) == 0)
#define cond_VS() ((OV & 0x80) != 0)
#define cond_PL() ((N & 0x80) == 0)
#define cond_MI() ((N & 0x80) != 0)
#define cond_GE() (((N ^ OV) & 0x80) == 0)
#define cond_LT() (((N ^ OV) & 0x80) != 0)
#define cond_GT() ((((N ^ OV) & 0x80) == 0) && (Z != 0))
#define cond_LE() ((((N ^ OV) & 0x80) != 0) || (Z == 0))

static void bra(void)
{
    INT8 tmp = (INT8)imm_byte();
    PC += tmp;
    CLK++;
}

static void branch(unsigned cond)
{
    if (cond) {
        bra();
    } else {
        imm_byte();
        CLK++;
    }
}

static void long_bra(void)
{
    PC += imm_word();

    CLK_ADD(2, 1);
}

static void long_branch(unsigned cond)
{
    if (cond) {
        PC += imm_word();
        CLK_ADD(1, 2);
    } else {
        PC += 2;
        CLK_ADD(3, 4);
    }
}

static void long_bsr(void)
{
    WORD tmp = imm_word();

    ea = PC + tmp;
    S -= 2;
    write_stack16(S, PC);
    PC = ea;

    CLK_ADD(4, 1);
}

static void bsr(void)
{
    INT8 tmp = (INT8)imm_byte();

    ea = PC + tmp;
    S -= 2;
    write_stack16(S, PC);
    CLK_ADD(3, 2);
    PC = ea;
}

/* Undocumented 6809 specific code */
#ifdef FULL6809

/* FIXME: cycle count */
void hcf(void)
{
    sim_error("HCF - not supported yet!\n");
    JAM("HCF");
}

void ccrs(void)
{
    DWORD tmp_c = (OV != 0);
    DWORD tmp_h = ((EFI & I_FLAG) != 0);

    set_cc(0);
    C = tmp_c;
    H = tmp_h << 4;
    CLK += 2;
}

void scc(BYTE arg)
{
    N = 0x80;
    Z = OV = 0;
}

void st_imm(WORD arg)
{
    WRMEM(PC++, (BYTE)(arg & 0xff));
    N = 0x80;
    Z = OV = 0;
}

void swires(void)
{
    CLK += 6;

    S -= 2;
    write_stack16(S, PC);
    S -= 2;
    write_stack16(S, U);
    S -= 2;
    write_stack16(S, Y);
    S -= 2;
    write_stack16(S--, X);
    write_stack(S--, (BYTE)(DP >> 8));
    write_stack(S--, B);
    write_stack(S--, A);
    write_stack(S, get_cc());
    EFI |= (I_FLAG | F_FLAG);

    PC = read16(0xfffe);
}
#endif

/* 6309 specific code */
#ifdef H6309
static BYTE tim(BYTE val, int CLK6309)
{
    OV = 0;
    N = Z = val;

    CLK += CLK6309;

    return val;
}

static void pshsw(void)
{
    CLK += 2;

    S -= 2;
    write_stack16(S, W);
}

static void pshuw(void)
{
    CLK += 2;

    U -= 2;
    write_stack16(U, W);
}

static void pulsw(void)
{
    CLK += 2;

    W = read_stack16(S);
    S += 2;
}

static void puluw(void)
{
    CLK += 2;

    W = read_stack16(U);
    U += 2;
}

static WORD neg16(WORD arg, int CLK6809, int CLK6309)
{
    WORD res = ~arg + 1;

    C = Z = res;
    N = res >> 8;
    OV = (res & arg) >> 8;

    CLK_ADD(CLK6809, CLK6309);

    return res;
}

static WORD com16(WORD arg, int CLK6809, int CLK6309)
{
    WORD res = ~arg;

    Z = res;
    N = res >> 8;
    OV = 0;
    C = 1;
    CLK_ADD(CLK6809, CLK6309);

    return res;
}

static WORD lsr16(WORD arg, int CLK6809, int CLK6309)
{
    WORD res = arg >> 1;

    N = 0;
    Z = res;
    C = arg & 1;

    CLK_ADD(CLK6809, CLK6309);

    return res;
}

static WORD ror16(WORD arg)
{
    WORD res = (arg >> 1) | ((C != 0) << 15);

    C = arg & 1;
    Z = res;
    N = res >> 8;

    CLK_ADD(1, 0);

    return res;
}

static WORD asr16(WORD arg, int CLK6809, int CLK6309)
{
    DWORD res = (SWORD)arg;

    C = res & 1;
    Z = res = (res >> 1) & 0xffff;
    N = res >> 8;
    CLK_ADD(CLK6809, CLK6309);

    return (WORD)res;
}

static WORD asl16(WORD arg, int CLK6809, int CLK6309)           /* same as lsl16 */
{
    DWORD res = arg << 1;

    C = res & 0x10000;
    Z = res &= 0xffff;
    N = res >> 8;
    OV = (arg ^ res) >> 8;
    CLK_ADD(CLK6809, CLK6309);

    return (WORD)res;
}

static WORD rol16(WORD arg)
{
    DWORD res = (arg << 1) + (C != 0);

    C = res & 0x10000;
    Z = res &= 0xffff;
    N = res >> 8;
    OV = (arg ^ res) >> 8;

    CLK_ADD(1, 0);

    return (WORD)res;
}

static WORD dec16(WORD arg, int CLK6809, int CLK6309)
{
    WORD res = arg - 1;

    Z = res;
    N = res >> 8;
    OV = (arg & ~res) >> 8;

    CLK_ADD(CLK6809, CLK6309);

    return res;
}

static WORD inc16(WORD arg, int CLK6809, int CLK6309)
{
    WORD res = arg + 1;

    Z = res;
    N = res >> 8;
    OV = (~arg & res) >> 8;

    CLK_ADD(CLK6809, CLK6309);

    return res;
}

static void tst16(WORD arg)
{
    Z = arg;
    N = arg >> 8;
    OV = 0;

    CLK_ADD(1, 0);
}

static WORD clr16(WORD arg, int CLK6809, int CLK6309)
{
    C = N = Z = OV = 0;
    CLK_ADD(CLK6809, CLK6309);

    return 0;
}

static WORD sbc16(WORD arg, WORD val, int CLK6809, int CLK6309)
{
    DWORD res = arg - val - (C != 0);

    C = res & 0x10000;
    Z = res &= 0xffff;
    N = res >> 8;
    OV = ((arg ^ val) & (arg ^ res)) >> 8;

    CLK_ADD(CLK6809, CLK6309);

    return (WORD)res;
}

static WORD and16(WORD arg, WORD val, int CLK6809, int CLK6309)
{
    WORD res = arg & val;

    Z = res;
    N = res >> 8;
    OV = 0;

    CLK_ADD(CLK6809, CLK6309);

    return res;
}

static void bit16(WORD arg, WORD val, int CLK6809, int CLK6309)
{
    WORD res = arg & val;

    Z = res;
    N = res >> 8;
    OV = 0;

    CLK_ADD(CLK6809, CLK6309);
}

static WORD eor16(WORD arg, WORD val, int CLK6809, int CLK6309)
{
    WORD res = arg ^ val;

    Z = res;
    N = res >> 8;
    OV = 0;

    CLK_ADD(CLK6809, CLK6309);

    return res;
}

static WORD adc16(WORD arg, WORD val, int CLK6809, int CLK6309)
{
    DWORD res = arg + val + (C != 0);

    C = (res >> 1) & 0x8000;
    Z = res &= 0xffff;
    N = res >> 8;
    OV = H = (arg ^ val ^ res ^ C) >> 8;

    CLK_ADD(CLK6809, CLK6309);

    return (WORD)res;
}

static WORD or16(WORD arg, WORD val, int CLK6809, int CLK6309)
{
    WORD res = arg | val;

    Z = res;
    N = res >> 8;
    OV = 0;

    CLK_ADD(CLK6809, CLK6309);

    return res;
}

static BYTE get_breg(BYTE rnr)
{
    switch (rnr) {
        case 0:
            return get_cc();
        case 1:
            return A;
        case 2:
            return B;
    }
    return 0;
}

static void set_breg(BYTE rnr, BYTE arg)
{
    switch (rnr) {
        case 0:
            set_cc(arg);
            break;
        case 1:
            A = arg;
            break;
        case 2:
            B = arg;
            break;
    }
}

static BYTE band(BYTE rnr, BYTE arg)
{
    BYTE rr = get_breg(rnr);
    BYTE tmp = arg;
    BYTE sbit = (rnr >> 3) & 7;
    BYTE dbit = rnr & 7;
    BYTE stmp = (rr & (1 << sbit)) ? 1 : 0;
    BYTE dtmp = (tmp & (1 << dbit)) ? 1 : 0;
    BYTE atmp = stmp & dtmp;

    tmp = (tmp & ~(1 << dbit)) | (atmp << dbit);

    OV = 0;
    N = Z = tmp;

    CLK_ADD(1, 0);

    return tmp;
}

static BYTE beor(BYTE rnr, BYTE arg)
{
    BYTE rr = get_breg(rnr);
    BYTE tmp = arg;
    BYTE sbit = (rnr >> 3) & 7;
    BYTE dbit = rnr & 7;
    BYTE stmp = (rr & (1 << sbit)) ? 1 : 0;
    BYTE dtmp = (tmp & (1 << dbit)) ? 1 : 0;
    BYTE atmp = stmp ^ dtmp;

    tmp = (tmp & ~(1 << dbit)) | (atmp << dbit);

    OV = 0;
    N = Z = tmp;

    CLK_ADD(1, 0);

    return tmp;
}

static BYTE biand(BYTE rnr, BYTE arg)
{
    BYTE rr = get_breg(rnr);
    BYTE tmp = arg;
    BYTE sbit = (rnr >> 3) & 7;
    BYTE dbit = rnr & 7;
    BYTE stmp = (rr & (1 << sbit)) ? 1 : 0;
    BYTE dtmp = (tmp & (1 << dbit)) ? 1 : 0;
    BYTE atmp = !(stmp & dtmp);

    tmp = (tmp & ~(1 << dbit)) | (atmp << dbit);

    OV = 0;
    N = Z = tmp;

    CLK_ADD(1, 0);

    return tmp;
}

static BYTE bieor(BYTE rnr, BYTE arg)
{
    BYTE rr = get_breg(rnr);
    BYTE tmp = arg;
    BYTE sbit = (rnr >> 3) & 7;
    BYTE dbit = rnr & 7;
    BYTE stmp = (rr & (1 << sbit)) ? 1 : 0;
    BYTE dtmp = (tmp & (1 << dbit)) ? 1 : 0;
    BYTE atmp = !(stmp ^ dtmp);

    tmp = (tmp & ~(1 << dbit)) | (atmp << dbit);

    OV = 0;
    N = Z = tmp;

    CLK_ADD(1, 0);

    return tmp;
}

static BYTE bior(BYTE rnr, BYTE arg)
{
    BYTE rr = get_breg(rnr);
    BYTE tmp = arg;
    BYTE sbit = (rnr >> 3) & 7;
    BYTE dbit = rnr & 7;
    BYTE stmp = (rr & (1 << sbit)) ? 1 : 0;
    BYTE dtmp = (tmp & (1 << dbit)) ? 1 : 0;
    BYTE atmp = !(stmp | dtmp);

    tmp = (tmp & ~(1 << dbit)) | (atmp << dbit);

    OV = 0;
    N = Z = tmp;

    CLK_ADD(1, 0);

    return tmp;
}

static BYTE bor(BYTE rnr, BYTE arg)
{
    BYTE rr = get_breg(rnr);
    BYTE tmp = arg;
    BYTE sbit = (rnr >> 3) & 7;
    BYTE dbit = rnr & 7;
    BYTE stmp = (rr & (1 << sbit)) ? 1 : 0;
    BYTE dtmp = (tmp & (1 << dbit)) ? 1 : 0;
    BYTE atmp = stmp | dtmp;

    tmp = (tmp & ~(1 << dbit)) | (atmp << dbit);

    OV = 0;
    N = Z = tmp;

    CLK_ADD(1, 0);

    return tmp;
}

static void ldbt(BYTE rnr, BYTE arg)
{
    BYTE tmp = arg;
    BYTE sbit = (rnr >> 3) & 7;
    BYTE dbit = rnr & 7;
    BYTE stmp = (tmp & (1 << sbit)) ? 1 : 0;

    tmp = (tmp & ~(1 << dbit)) | (stmp << dbit);

    OV = 0;
    N = Z = tmp;

    CLK_ADD(1, 0);

    set_breg(rnr, tmp);
}

static BYTE stbt(BYTE rnr, BYTE arg)
{
    BYTE rr = get_breg(rnr);
    BYTE tmp = arg;
    BYTE sbit = (rnr >> 3) & 7;
    BYTE dbit = rnr & 7;
    BYTE stmp = (rr & (1 << sbit)) ? 1 : 0;

    tmp = (tmp & ~(1 << dbit)) | (stmp << dbit);

    OV = 0;
    N = Z = tmp;

    CLK_ADD(3, 2);

    return tmp;
}

/* reg type: 0=illegal, 1=byte, 2=word */
static int tfm_reg_type(BYTE rnr)
{
    switch (rnr) {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:
        case 0x4:
            return 2;
        case 0x8:
        case 0x9:
        case 0xe:
        case 0xf:
            return 1;
    }
    return 0;
}

static WORD tfm_get_reg(BYTE rnr)
{
    switch (rnr) {
        case 0x0:
            return D;
        case 0x1:
            return X;
        case 0x2:
            return Y;
        case 0x3:
            return U;
        case 0x4:
            return S;
        case 0x8:
            return (WORD)A;
        case 0x9:
            return (WORD)B;
        case 0xe:
            return (WORD)E;
        case 0xf:
            return (WORD)F;
    }
    return 0;
}

static void tfm_set_reg(BYTE rnr, WORD val)
{
    switch (rnr) {
        case 0x0:
            D = val;
            break;
        case 0x1:
            X = val;
            break;
        case 0x2:
            Y = val;
            break;
        case 0x3:
            U = val;
            break;
        case 0x4:
            S = val;
            break;
        case 0x8:
            A = (BYTE)val;
            break;
        case 0x9:
            B = (BYTE)val;
            break;
        case 0xe:
            E = (BYTE)val;
            break;
        case 0xf:
            F = (BYTE)val;
            break;
    }
}

static void tfmpp(BYTE rnr)
{
    BYTE val;
    BYTE r0_nr = rnr >> 4;
    BYTE r1_nr = rnr & 7;
    WORD r0 = tfm_get_reg(r0_nr);
    WORD r1 = tfm_get_reg(r1_nr);
    int r0_type = tfm_reg_type(r0_nr);
    int r1_type = tfm_reg_type(r1_nr);

    if (r0_type && r1_type) {
        while (W) {
            val = RDMEM(r0++);
            WRMEM(r1++, val);
            if (r0_type == 1) {
                r0 &= 0xff;
            }
            if (r1_type == 1) {
                r1 &= 0xff;
            }
            W--;
            CLK++;
        }
        tfm_set_reg(r0_nr, r0);
        tfm_set_reg(r1_nr, r1);
        CLK += 6;
    } else {
        opcode_trap();
    }
}

static void tfmmm(BYTE rnr)
{
    BYTE val;
    BYTE r0_nr = rnr >> 4;
    BYTE r1_nr = rnr & 7;
    WORD r0 = tfm_get_reg(r0_nr);
    WORD r1 = tfm_get_reg(r1_nr);
    int r0_type = tfm_reg_type(r0_nr);
    int r1_type = tfm_reg_type(r1_nr);

    if (r0_type && r1_type) {
        while (W) {
            val = RDMEM(r0--);
            WRMEM(r1--, val);
            if (r0_type == 1) {
                r0 &= 0xff;
            }
            if (r1_type == 1) {
                r1 &= 0xff;
            }
            W--;
            CLK++;
        }
        tfm_set_reg(r0_nr, r0);
        tfm_set_reg(r1_nr, r1);
        CLK += 6;
    } else {
        opcode_trap();
    }
}

static void tfmpc(BYTE rnr)
{
    BYTE val;
    BYTE r0_nr = rnr >> 4;
    BYTE r1_nr = rnr & 7;
    WORD r0 = tfm_get_reg(r0_nr);
    WORD r1 = tfm_get_reg(r1_nr);
    int r0_type = tfm_reg_type(r0_nr);
    int r1_type = tfm_reg_type(r1_nr);

    if (r0_type && r1_type) {
        while (W) {
            val = RDMEM(r0--);
            WRMEM(r1, val);
            if (r0_type == 1) {
                r0 &= 0xff;
            }
            W--;
            CLK++;
        }
        tfm_set_reg(r0_nr, r0);
        CLK += 6;
    } else {
        opcode_trap();
    }
}

static void tfmcp(BYTE rnr)
{
    BYTE val;
    BYTE r0_nr = rnr >> 4;
    BYTE r1_nr = rnr & 7;
    WORD r0 = tfm_get_reg(r0_nr);
    WORD r1 = tfm_get_reg(r1_nr);
    int r0_type = tfm_reg_type(r0_nr);
    int r1_type = tfm_reg_type(r1_nr);

    if (r0_type && r1_type) {
        while (W) {
            val = RDMEM(r0);
            WRMEM(r1--, val);
            if (r1_type == 1) {
                r1 &= 0xff;
            }
            W--;
            CLK++;
        }
        tfm_set_reg(r1_nr, r1);
        CLK += 6;
    } else {
        opcode_trap();
    }
}

static void divd(BYTE m, int CLK6809, int CLK6309)
{
    SWORD val, bak;

    if (m) {
        bak = (SWORD)D;
        val = bak / (INT8)m;
        A = bak % (INT8)m;
        B = val & 0xff;
        Z = N = B;
        C = B & 1;
        if (bak < 0) {
            Z = 0x80;
        }
        OV = 0;
        if ((val > 127) || (val < -128)) {
            OV = 0x80;
            if ((val > 255) || (val < -256)) {
                N = (WORD)(bak) >> 8;
                Z = bak;
                D = abs(bak);
            }
        }
    } else {
        div0_trap();
    }

    CLK_ADD(CLK6809, CLK6309);
}

static void divq(WORD m, int CLK6809, int CLK6309)
{
    SDWORD val, bak;

    if (m) {
        bak = (SDWORD)Q;
        val = bak / (SWORD)m;
        D = bak % (SWORD)m;
        W = val & 0xffff;
        N = W >> 8;
        Z = W;
        C = W & 1;
        if (bak < 0) {
            N = 0x80;
        }
        OV = 0;
        if ((val > 32768) || (val < -32767)) {
            OV = 0x80;
            if ((val > 65536) || (val < -65535)) {
                N = (DWORD)(bak) >> 24;
                Z = bak;
                Q = abs(bak);
            }
        }
    } else {
        div0_trap();
    }

    CLK_ADD(CLK6809, CLK6309);
}

static void muld(WORD m, int CLK6809, int CLK6309)
{
    Q = D * m;
    C = OV = 0;
    N = D >> 8;
    Z = D;

    CLK_ADD(CLK6809, CLK6309);
}

static void sexw(void)
{
    W = (INT8)F;

    Z = W;
    N = W >> 8;

    CLK += 3;
}

static DWORD ld32(DWORD arg, int CLK6809, int CLK6309)
{
    Z = arg;
    N = arg >> 24;
    OV = 0;

    CLK_ADD(CLK6809, CLK6309);

    return arg;
}

static void st32(DWORD arg, int CLK6809, int CLK6309)
{
    Z = arg;
    N = arg >> 24;
    OV = 0;
    WRMEM32(ea, arg);

    CLK_ADD(CLK6809, CLK6309);
}

#endif

/* Execute 6809 code for a certain number of cycles. */
void h6809_mainloop (struct interrupt_cpu_status_s *maincpu_int_status, alarm_context_t *maincpu_alarm_context)
{
    WORD opcode;
    BYTE fetch;
#ifdef H6309
    BYTE post_byte;
#endif

    do {
#ifndef CYCLE_EXACT_ALARM
        while (CLK >= alarm_context_next_pending_clk(ALARM_CONTEXT)) {
            alarm_context_dispatch(ALARM_CONTEXT, CLK);
            CPU_DELAY_CLK
        }
#endif

        {
            enum cpu_int pending_interrupt;

            if (!(CPU_INT_STATUS->global_pending_int & IK_IRQ)
                && (CPU_INT_STATUS->global_pending_int & IK_IRQPEND)
                && CPU_INT_STATUS->irq_pending_clk <= CLK) {
                interrupt_ack_irq(CPU_INT_STATUS);
                release_irq(0);
            }

            pending_interrupt = CPU_INT_STATUS->global_pending_int;
            if (pending_interrupt != IK_NONE) {
                DO_INTERRUPT(pending_interrupt);
                if (!(CPU_INT_STATUS->global_pending_int & IK_IRQ)
                    && CPU_INT_STATUS->global_pending_int & IK_IRQPEND) {
                    CPU_INT_STATUS->global_pending_int &= ~IK_IRQPEND;
                }
                CPU_DELAY_CLK
#ifndef CYCLE_EXACT_ALARM
                while (CLK >= alarm_context_next_pending_clk(ALARM_CONTEXT)) {
                    alarm_context_dispatch(ALARM_CONTEXT, CLK);
                    CPU_DELAY_CLK
                }
#endif
            }
        }

        SET_LAST_ADDR(PC);
        fetch = imm_byte();

        if (fetch == 0x10 || fetch == 0x11) {
            opcode = fetch << 8;
            fetch = imm_byte();
            while (fetch == 0x10 || fetch == 0x11) {
                fetch = imm_byte();
            }
            opcode |= fetch;
        } else {
            opcode = fetch;
        }

        switch (opcode) {
            case 0x003a:        /* ABX */
#ifdef FULL6809
            case 0x103a:        /* ABX (UNDOC) */
            case 0x113a:        /* ABX (UNDOC) */
#endif
                abx();
                break;

            case 0x0089:        /* ADCA immediate */
#ifdef FULL6809
            case 0x1089:        /* ADCA immediate (UNDOC) */
            case 0x1189:        /* ADCA immediate (UNDOC) */
#endif
                A = adc(A, imm_byte(), 0, 0);
                break;

            case 0x0099:        /* ADCA direct */
#ifdef FULL6809
            case 0x1099:        /* ADCA direct (UNDOC) */
            case 0x1199:        /* ADCA direct (UNDOC) */
#endif
                direct();
                A = adc(A, RDMEM(ea), 1, 0);
                break;

            case 0x00b9:        /* ADCA extended */
#ifdef FULL6809
            case 0x10b9:        /* ADCA extended (UNDOC) */
            case 0x11b9:        /* ADCA extended (UNDOC) */
#endif
                extended();
                A = adc(A, RDMEM(ea), 1, 0);
                break;

            case 0x00a9:        /* ADCA indexed */
#ifdef FULL6809
            case 0x10a9:        /* ADCA indexed (UNDOC) */
            case 0x11a9:        /* ADCA indexed (UNDOC) */
#endif
                indexed();
                A = adc(A, RDMEM(ea), 1, 1);
                break;

            case 0x00c9:        /* ADCB immediate */
#ifdef FULL6809
            case 0x10c9:        /* ADCB immediate (UNDOC) */
            case 0x11c9:        /* ADCB immediate (UNDOC) */
#endif
                B = adc(B, imm_byte(), 0, 0);
                break;

            case 0x00d9:        /* ADCB direct */
#ifdef FULL6809
            case 0x10d9:        /* ADCB direct (UNDOC) */
            case 0x11d9:        /* ADCB direct (UNDOC) */
#endif
                direct();
                B = adc(B, RDMEM(ea), 1, 0);
                break;

            case 0x00f9:        /* ADCB extended */
#ifdef FULL6809
            case 0x10f9:        /* ADCB extended (UNDOC) */
            case 0x11f9:        /* ADCB extended (UNDOC) */
#endif
                extended();
                B = adc(B, RDMEM(ea), 1, 0);
                break;

            case 0x00e9:        /* ADCB indexed */
#ifdef FULL6809
            case 0x10e9:        /* ADCB indexed (UNDOC) */
            case 0x11e9:        /* ADCB indexed (UNDOC) */
#endif
                indexed();
                B = adc(B, RDMEM(ea), 1, 1);
                break;

#ifdef H6309
            case 0x1089:        /* ADCD immediate */
                D = adc16(D, imm_word(), 1, 0);
                break;

            case 0x1099:        /* ADCD direct */
                direct();
                D = adc16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x10b9:        /* ADCD extended */
                extended();
                D = adc16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x10a9:        /* ADCD indexed */
                indexed();
                D = adc16(D, RDMEM16(ea), 2, 1);
                break;

            case 0x1031:        /* ADCR post */
                post_byte = imm_byte();
                set_reg((BYTE)(post_byte & 0x0f), adc16(get_reg((BYTE)(post_byte >> 4)), get_reg((BYTE)(post_byte & 0x0f)), 1, 1));
                break;
#endif

            case 0x008b:        /* ADDA immediate */
#ifdef FULL6809
            case 0x108b:        /* ADDA immediate (UNDOC) */
            case 0x118b:        /* ADDA immediate (UNDOC) */
#endif
                A = add(A, imm_byte(), 0, 0);
                break;

            case 0x009b:        /* ADDA direct */
#ifdef FULL6809
            case 0x109b:        /* ADDA direct (UNDOC) */
            case 0x119b:        /* ADDA direct (UNDOC) */
#endif
                direct();
                A = add(A, RDMEM(ea), 1, 0);
                break;

            case 0x00bb:        /* ADDA extended */
#ifdef FULL6809
            case 0x10bb:        /* ADDA extended (UNDOC) */
            case 0x11bb:        /* ADDA extended (UNDOC) */
#endif
                extended();
                A = add(A, RDMEM(ea), 1, 0);
                break;

            case 0x00ab:        /* ADDA indexed */
#ifdef FULL6809
            case 0x10ab:        /* ADDA indexed (UNDOC) */
            case 0x11ab:        /* ADDA indexed (UNDOC) */
#endif
                indexed();
                A = add(A, RDMEM(ea), 1, 1);
                break;

            case 0x00cb:        /* ADDB immediate */
#ifdef FULL6809
            case 0x10cb:        /* ADDB immediate (UNDOC) */
            case 0x11cb:        /* ADDB immediate (UNDOC) */
#endif
                B = add(B, imm_byte(), 0, 0);
                break;

            case 0x00db:        /* ADDB direct */
#ifdef FULL6809
            case 0x10db:        /* ADDB direct (UNDOC) */
            case 0x11db:        /* ADDB direct (UNDOC) */
#endif
                direct();
                B = add(B, RDMEM(ea), 1, 0);
                break;

            case 0x00fb:        /* ADDB extended */
#ifdef FULL6809
            case 0x10fb:        /* ADDB extended (UNDOC) */
            case 0x11fb:        /* ADDB extended (UNDOC) */
#endif
                extended();
                B = add(B, RDMEM(ea), 1, 0);
                break;

            case 0x00eb:        /* ADDB indexed */
#ifdef FULL6809
            case 0x10eb:        /* ADDB indexed (UNDOC) */
            case 0x11eb:        /* ADDB indexed (UNDOC) */
#endif
                indexed();
                B = add(B, RDMEM(ea), 1, 1);
                break;

            case 0x00c3:        /* ADDD immediate */
#ifdef FULL6809
            case 0x10c3:        /* ADDD immediate (UNDOC) */
            case 0x11c3:        /* ADDD immediate (UNDOC) */
#endif
                D = add16(D, imm_word(), 1, 0);
                break;

            case 0x00d3:        /* ADDD direct */
#ifdef FULL6809
            case 0x10d3:        /* ADDD direct (UNDOC) */
            case 0x11d3:        /* ADDD direct (UNDOC) */
#endif
                direct();
                D = add16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x00f3:        /* ADDD extended */
#ifdef FULL6809
            case 0x10f3:        /* ADDD extended (UNDOC) */
            case 0x11f3:        /* ADDD extended (UNDOC) */
#endif
                extended();
                D = add16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x00e3:        /* ADDD indexed */
#ifdef FULL6809
            case 0x10e3:        /* ADDD indexed (UNDOC) */
            case 0x11e3:        /* ADDD indexed (UNDOC) */
#endif
                indexed();
                D = add16(D, RDMEM16(ea), 2, 1);
                break;

#ifdef H6309
            case 0x118b:        /* ADDE immediate */
                E = add(E, imm_byte(), 0, 0);
                break;

            case 0x119b:        /* ADDE direct */
                direct();
                E = add(E, RDMEM(ea), 1, 0);
                break;

            case 0x11bb:        /* ADDE extended */
                extended();
                E = add(E, RDMEM(ea), 1, 0);
                break;

            case 0x11ab:        /* ADDE indexed */
                indexed();
                E = add(E, RDMEM(ea), 2, 2);
                break;

            case 0x11cb:        /* ADDF immediate */
                F = add(F, imm_byte(), 0, 0);
                break;

            case 0x11db:        /* ADDF direct */
                direct();
                F = add(F, RDMEM(ea), 1, 0);
                break;

            case 0x11fb:        /* ADDF extended */
                extended();
                F = add(F, RDMEM(ea), 1, 0);
                break;

            case 0x11eb:        /* ADDF indexed */
                indexed();
                F = add(F, RDMEM(ea), 2, 2);
                break;

            case 0x1030:        /* ADDR post */
                post_byte = imm_byte();
                set_reg((BYTE)(post_byte & 0x0f), add16(get_reg((BYTE)(post_byte >> 4)), get_reg((BYTE)(post_byte & 0x0f)), 1, 1));
                break;

            case 0x108b:        /* ADDW immediate */
                W = add16(W, imm_word(), 1, 0);
                break;

            case 0x109b:        /* ADDW direct */
                direct();
                W = add16(W, RDMEM16(ea), 2, 0);
                break;

            case 0x10bb:        /* ADDW extended */
                extended();
                W = add16(W, RDMEM16(ea), 2, 0);
                break;

            case 0x10ab:        /* ADDW indexed */
                indexed();
                W = add16(W, RDMEM16(ea), 2, 1);
                break;

            case 0x0002:        /* AIM post, direct */
                post_byte = imm_byte();
                direct();
                WRMEM(ea, and(RDMEM(ea), post_byte, 1, 1));
                break;

            case 0x0072:        /* AIM post, extended */
                post_byte = imm_byte();
                extended();
                WRMEM(ea, and(RDMEM(ea), post_byte, 1, 1));
                break;

            case 0x0062:        /* AIM post, indexed */
                post_byte = imm_byte();
                indexed();
                WRMEM(ea, and(RDMEM(ea), post_byte, 2, 2));
                break;
#endif

            case 0x0084:        /* ANDA immediate */
#ifdef FULL6809
            case 0x1084:        /* ANDA immediate (UNDOC) */
            case 0x1184:        /* ANDA immediate (UNDOC) */
#endif
                A = and(A, imm_byte(), 0, 0);
                break;

            case 0x0094:        /* ANDA direct */
#ifdef FULL6809
            case 0x1094:        /* ANDA direct (UNDOC) */
            case 0x1194:        /* ANDA direct (UNDOC) */
#endif
                direct();
                A = and(A, RDMEM(ea), 1, 0);
                break;

            case 0x00b4:        /* ANDA extended */
#ifdef FULL6809
            case 0x10b4:        /* ANDA extended (UNDOC) */
            case 0x11b4:        /* ANDA extended (UNDOC) */
#endif
                extended();
                A = and(A, RDMEM(ea), 1, 0);
                break;

            case 0x00a4:        /* ANDA indexed */
#ifdef FULL6809
            case 0x10a4:        /* ANDA indexed (UNDOC) */
            case 0x11a4:        /* ANDA indexed (UNDOC) */
#endif
                indexed();
                A = and(A, RDMEM(ea), 1, 1);
                break;

            case 0x00c4:        /* ANDB immediate */
#ifdef FULL6809
            case 0x10c4:        /* ANDB immediate (UNDOC) */
            case 0x11c4:        /* ANDB immediate (UNDOC) */
#endif
                B = and(B, imm_byte(), 0, 0);
                break;

            case 0x00d4:        /* ANDB direct */
#ifdef FULL6809
            case 0x10d4:        /* ANDB direct (UNDOC) */
            case 0x11d4:        /* ANDB direct (UNDOC) */
#endif
                direct();
                B = and(B, RDMEM(ea), 1, 0);
                break;

            case 0x00f4:        /* ANDB extended */
#ifdef FULL6809
            case 0x10f4:        /* ANDB extended (UNDOC) */
            case 0x11f4:        /* ANDB extended (UNDOC) */
#endif
                extended();
                B = and(B, RDMEM(ea), 1, 0);
                break;

            case 0x00e4:        /* ANDB indexed */
#ifdef FULL6809
            case 0x10e4:        /* ANDB indexed (UNDOC) */
            case 0x11e4:        /* ANDB indexed (UNDOC) */
#endif
                indexed();
                B = and(B, RDMEM(ea), 1, 1);
                break;

            case 0x001c:        /* ANDCC immediate */
#ifdef FULL6809
            case 0x101c:        /* ANDCC immediate (UNDOC) */
            case 0x111c:        /* ANDCC immediate (UNDOC) */
#endif
                andcc();
                break;

#ifdef FULL6809
            case 0x0038:        /* ANDCC immediate (+1 extra cycle) (UNDOC) */
            case 0x1038:        /* ANDCC immediate (+1 extra cycle) (UNDOC) */
            case 0x1138:        /* ANDCC immediate (+1 extra cycle) (UNDOC) */
                andcc();
                CLK++;
                break;
#endif

#ifdef H6309
            case 0x1084:        /* ANDD immediate */
                D = and16(D, imm_word(), 1, 0);
                break;

            case 0x1094:        /* ANDD direct */
                direct();
                D = and16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x10b4:        /* ANDD extended */
                extended();
                D = and16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x10a4:        /* ANDD indexed */
                indexed();
                D = and16(D, RDMEM16(ea), 2, 1);
                break;

            case 0x1034:        /* ANDR post */
                post_byte = imm_byte();
                set_reg((BYTE)(post_byte & 0x0f), and16(get_reg((BYTE)(post_byte >> 4)), get_reg((BYTE)(post_byte & 0x0f)), 1, 1));
                break;
#endif

            case 0x0048:        /* ASLA/LSLA */
#ifdef FULL6809
            case 0x1048:        /* ASLA/LSLA (UNDOC) */
            case 0x1148:        /* ASLA/LSLA (UNDOC) */
#endif
                A = asl(A, 1, 0);
                break;

            case 0x0058:        /* ASLB/LSLB */
#ifdef FULL6809
            case 0x1058:        /* ASLB/LSLB (UNDOC) */
            case 0x1158:        /* ASLB/LSLB (UNDOC) */
#endif
                B = asl(B, 1, 0);
                break;

#ifdef H6309
            case 0x1048:        /* ASLD/LSLD */
                D = asl16(D, 1, 0);
                break;
#endif

            case 0x0008:        /* ASL/LSL direct */
#ifdef FULL6809
            case 0x1008:        /* ASL/LSL direct (UNDOC) */
            case 0x1108:        /* ASL/LSL direct (UNDOC) */
#endif
                direct();
                WRMEM(ea, asl(RDMEM(ea), 2, 1));
                break;

            case 0x0078:        /* ASL/LSL extended */
#ifdef FULL6809
            case 0x1078:        /* ASL/LSL extended (UNDOC) */
            case 0x1178:        /* ASL/LSL extended (UNDOC) */
#endif
                extended();
                WRMEM(ea, asl(RDMEM(ea), 2, 1));
                break;

            case 0x0068:        /* ASL/LSL indexed */
#ifdef FULL6809
            case 0x1068:        /* ASL/LSL indexed (UNDOC) */
            case 0x1168:        /* ASL/LSL indexed (UNDOC) */
#endif
                indexed();
                WRMEM(ea, asl(RDMEM(ea), 2, 2));
                break;

            case 0x0047:        /* ASRA */
#ifdef FULL6809
            case 0x1047:        /* ASRA (UNDOC) */
            case 0x1147:        /* ASRA (UNDOC) */
#endif
                A = asr(A, 1, 0);
                break;

            case 0x0057:        /* ASRB */
#ifdef FULL6809
            case 0x1057:        /* ASRB (UNDOC) */
            case 0x1157:        /* ASRB (UNDOC) */
#endif
                B = asr(B, 1, 0);
                break;

#ifdef H6309
            case 0x1047:        /* ASRD */
                D = asr16(D, 1, 0);
                break;
#endif

            case 0x0007:        /* ASR direct */
#ifdef FULL6809
            case 0x1007:        /* ASR direct (UNDOC) */
            case 0x1107:        /* ASR direct (UNDOC) */
#endif
                direct();
                WRMEM(ea, asr(RDMEM(ea), 2, 1));
                break;

            case 0x0077:        /* ASR extended */
#ifdef FULL6809
            case 0x1077:        /* ASR extended (UNDOC) */
            case 0x1177:        /* ASR extended (UNDOC) */
#endif
                extended();
                WRMEM(ea, asr(RDMEM(ea), 2, 1));
                break;

            case 0x0067:        /* ASR indexed */
#ifdef FULL6809
            case 0x1067:        /* ASR indexed (UNDOC) */
            case 0x1167:        /* ASR indexed (UNDOC) */
#endif
                indexed();
                WRMEM(ea, asr(RDMEM(ea), 2, 2));
                break;

#ifdef H6309
            case 0x1130:        /* BAND post, direct */
                post_byte = imm_byte();
                direct();
                WRMEM(ea, band(post_byte, RDMEM(ea)));
                break;
#endif

#ifdef H6309
            case 0x1134:        /* BEOR post,direct */
                post_byte = imm_byte();
                direct();
                WRMEM(ea, beor(post_byte, RDMEM(ea)));
                break;
#endif

            case 0x0027:        /* BEQ */
#ifdef FULL6809
            case 0x1127:        /* BEQ (UNDOC) */
#endif
                branch(cond_EQ());
                break;

            case 0x002c:        /* BGE */
#ifdef FULL6809
            case 0x112c:        /* BGE (UNDOC) */
#endif
                branch(cond_GE());
                break;

            case 0x002e:        /* BGT */
#ifdef FULL6809
            case 0x112e:        /* BGT (UNDOC) */
#endif
                branch(cond_GT());
                break;

            case 0x0022:        /* BHI */
#ifdef FULL6809
            case 0x1122:        /* BHI (UNDOC) */
#endif
                branch(cond_HI());
                break;

            case 0x0024:        /* BHS */
#ifdef FULL6809
            case 0x1124:        /* BHS (UNDOC) */
#endif
                branch(cond_HS());
                break;

#ifdef H6309
            case 0x1131:        /* BIAND post, direct */
                post_byte = imm_byte();
                direct();
                WRMEM(ea, biand(post_byte, RDMEM(ea)));
                break;

            case 0x1135:        /* BIEOR post, direct */
                post_byte = imm_byte();
                direct();
                WRMEM(ea, bieor(post_byte, RDMEM(ea)));
                break;

            case 0x1133:        /* BIOR post, direct */
                post_byte = imm_byte();
                direct();
                WRMEM(ea, bior(post_byte, RDMEM(ea)));
                break;
#endif

            case 0x0085:        /* BITA immediate */
#ifdef FULL6809
            case 0x1085:        /* BITA immediate (UNDOC) */
            case 0x1185:        /* BITA immediate (UNDOC) */
#endif
                bit(A, imm_byte(), 0, 0);
                break;

            case 0x0095:        /* BITA direct */
#ifdef FULL6809
            case 0x1095:        /* BITA direct (UNDOC) */
            case 0x1195:        /* BITA direct (UNDOC) */
#endif
                direct();
                bit(A, RDMEM(ea), 1, 0);
                break;

            case 0x00b5:        /* BITA extended */
#ifdef FULL6809
            case 0x10b5:        /* BITA extended (UNDOC) */
            case 0x11b5:        /* BITA extended (UNDOC) */
#endif
                extended();
                bit(A, RDMEM(ea), 1, 0);
                break;

            case 0x00a5:        /* BITA indexed */
#ifdef FULL6809
            case 0x10a5:        /* BITA indexed (UNDOC) */
            case 0x11a5:        /* BITA indexed (UNDOC) */
#endif
                indexed();
                bit(A, RDMEM(ea), 1, 1);
                break;

            case 0x00c5:        /* BITB immediate */
#ifdef FULL6809
            case 0x10c5:        /* BITB immediate (UNDOC) */
            case 0x11c5:        /* BITB immediate (UNDOC) */
#endif
                bit(B, imm_byte(), 0, 0);
                break;

            case 0x00d5:        /* BITB direct */
#ifdef FULL6809
            case 0x10d5:        /* BITB direct (UNDOC) */
            case 0x11d5:        /* BITB direct (UNDOC) */
#endif
                direct();
                bit(B, RDMEM(ea), 1, 0);
                break;

            case 0x00f5:        /* BITB extended */
#ifdef FULL6809
            case 0x10f5:        /* BITB extended (UNDOC) */
            case 0x11f5:        /* BITB extended (UNDOC) */
#endif
                extended();
                bit(B, RDMEM(ea), 1, 0);
                break;

            case 0x00e5:        /* BITB indexed */
#ifdef FULL6809
            case 0x10e5:        /* BITB indexed (UNDOC) */
            case 0x11e5:        /* BITB indexed (UNDOC) */
#endif
                indexed();
                bit(B, RDMEM(ea), 1, 1);
                break;

#ifdef H6309
            case 0x1085:        /* BITD immediate */
                bit16(D, imm_word(), 1, 0);
                break;

            case 0x1095:        /* BITD direct */
                direct();
                bit16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x10b5:        /* BITD extended */
                extended();
                bit16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x10a5:        /* BITD indexed */
                indexed();
                bit16(D, RDMEM16(ea), 2, 1);
                break;

            case 0x113c:        /* BITMD immediate */
                bit(get_md(), imm_byte(), 1, 1);
                break;
#endif

            case 0x002f:        /* BLE */
#ifdef FULL6809
            case 0x112f:        /* BLE (UNDOC) */
#endif
                branch(cond_LE());
                break;

            case 0x0025:        /* BLO */
#ifdef FULL6809
            case 0x1125:        /* BLO (UNDOC) */
#endif
                branch(cond_LO());
                break;

            case 0x0023:        /* BLS */
#ifdef FULL6809
            case 0x1123:        /* BLS (UNDOC) */
#endif
                branch(cond_LS());
                break;

            case 0x002d:        /* BLT */
#ifdef FULL6809
            case 0x112d:        /* BLT (UNDOC) */
#endif
                branch(cond_LT());
                break;

            case 0x002b:        /* BMI */
#ifdef FULL6809
            case 0x112b:        /* BMI (UNDOC) */
#endif
                branch(cond_MI());
                break;

            case 0x0026:        /* BNE */
#ifdef FULL6809
            case 0x1126:        /* BNE (UNDOC) */
#endif
                branch(cond_NE());
                break;

#ifdef H6309
            case 0x1132:        /* BOR post,direct */
                post_byte = imm_byte();
                direct();
                WRMEM(ea, bor(post_byte, RDMEM(ea)));
                break;
#endif

            case 0x002a:        /* BPL */
#ifdef FULL6809
            case 0x112a:        /* BPL (UNDOC) */
#endif
                branch(cond_PL());
                break;

            case 0x0020:        /* BRA */
#ifdef FULL6809
            case 0x1120:        /* BRA (UNDOC) */
#endif
                bra();
                break;

            case 0x0021:        /* BRN */
#ifdef FULL6809
            case 0x1121:        /* BRN (UNDOC) */
#endif
                imm_byte();
                CLK++;
                break;

            case 0x008d:        /* BSR */
#ifdef FULL6809
            case 0x108d:        /* BSR (UNDOC) */
            case 0x118d:        /* BSR (UNDOC) */
#endif
                bsr();
                break;

            case 0x0028:        /* BVC */
#ifdef FULL6809
            case 0x1128:        /* BVC (UNDOC) */
#endif
                branch(cond_VC());
                break;

            case 0x0029:        /* BVS */
#ifdef FULL6809
            case 0x1129:        /* BVS (UNDOC) */
#endif
                branch(cond_VS());
                break;

#ifdef FULL6809
            case 0x0018:        /* CCRS (UNDOC) */
            case 0x1018:        /* CCRS (UNDOC) */
            case 0x1118:        /* CCRS (UNDOC) */
                ccrs();
                break;
#endif

            case 0x004f:        /* CLRA */
#ifdef FULL6809
            case 0x004e:        /* CLRA (UNDOC) */
            case 0x104e:        /* CLRA (UNDOC) */
            case 0x104f:        /* CLRA (UNDOC) */
            case 0x114e:        /* CLRA (UNDOC) */
            case 0x114f:        /* CLRA (UNDOC) */
#endif
                A = clr(A, 1, 0);
                break;

            case 0x005f:        /* CLRB */
#ifdef FULL6809
            case 0x005e:        /* CLRB (UNDOC) */
            case 0x105e:        /* CLRB (UNDOC) */
            case 0x105f:        /* CLRB (UNDOC) */
            case 0x115e:        /* CLRB (UNDOC) */
            case 0x115f:        /* CLRB (UNDOC) */
#endif
                B = clr(B, 1, 0);
                break;

#ifdef H6309
            case 0x104f:        /* CLRD */
                D = clr16(D, 1, 0);
                break;

            case 0x114f:        /* CLRE */
                E = clr(E, 1, 0);
                break;

            case 0x115f:        /* CLRF */
                F = clr(F, 1, 0);
                break;

            case 0x105f:        /* CLRW */
                W = clr16(W, 1, 0);
                break;
#endif

            case 0x000f:        /* CLR direct */
#ifdef FULL6809
            case 0x100f:        /* CLR direct (UNDOC) */
            case 0x110f:        /* CLR direct (UNDOC) */
#endif
                direct();
                WRMEM(ea, clr(RDMEM(ea), 2, 1));
                break;

            case 0x007f:        /* CLR extended */
#ifdef FULL6809
            case 0x107f:        /* CLR extended (UNDOC) */
            case 0x117f:        /* CLR extended (UNDOC) */
#endif
                extended();
                WRMEM(ea, clr(RDMEM(ea), 2, 1));
                break;

            case 0x006f:        /* CLR indexed */
#ifdef FULL6809
            case 0x106f:        /* CLR indexed (UNDOC) */
            case 0x116f:        /* CLR indexed (UNDOC) */
#endif
                indexed();
                WRMEM(ea, clr(RDMEM(ea), 2, 2));
                break;

            case 0x0081:        /* CMPA immediate */
#ifdef FULL6809
            case 0x1081:        /* CMPA immediate (UNDOC) */
            case 0x1181:        /* CMPA immediate (UNDOC) */
#endif
                cmp(A, imm_byte(), 0, 0);
                break;

            case 0x0091:        /* CMPA direct */
#ifdef FULL6809
            case 0x1091:        /* CMPA direct (UNDOC) */
            case 0x1191:        /* CMPA direct (UNDOC) */
#endif
                direct();
                cmp(A, RDMEM(ea), 1, 0);
                break;

            case 0x00b1:        /* CMPA extended */
#ifdef FULL6809
            case 0x10b1:        /* CMPA extended (UNDOC) */
            case 0x11b1:        /* CMPA extended (UNDOC) */
#endif
                extended();
                cmp(A, RDMEM(ea), 1, 0);
                break;

            case 0x00a1:        /* CMPA indexed */
#ifdef FULL6809
            case 0x10a1:        /* CMPA indexed (UNDOC) */
            case 0x11a1:        /* CMPA indexed (UNDOC) */
#endif
                indexed();
                cmp(A, RDMEM(ea), 1, 1);
                break;

            case 0x00c1:        /* CMPB immediate */
#ifdef FULL6809
            case 0x10c1:        /* CMPB immediate (UNDOC) */
            case 0x11c1:        /* CMPB immediate (UNDOC) */
#endif
                cmp(B, imm_byte(), 0, 0);
                break;

            case 0x00d1:        /* CMPB direct */
#ifdef FULL6809
            case 0x10d1:        /* CMPB direct (UNDOC) */
            case 0x11d1:        /* CMPB direct (UNDOC) */
#endif
                direct();
                cmp(B, RDMEM(ea), 1, 0);
                break;

            case 0x00f1:        /* CMPB extended */
#ifdef FULL6809
            case 0x10f1:        /* CMPB extended (UNDOC) */
            case 0x11f1:        /* CMPB extended (UNDOC) */
#endif
                extended();
                cmp(B, RDMEM(ea), 1, 0);
                break;

            case 0x00e1:        /* CMPB indexed */
#ifdef FULL6809
            case 0x10e1:        /* CMPB indexed (UNDOC) */
            case 0x11e1:        /* CMPB indexed (UNDOC) */
#endif
                indexed();
                cmp(B, RDMEM(ea), 1, 1);
                break;

            case 0x1083:        /* CMPD immediate */
                cmp16(D, imm_word(), 1, 0);
                break;

            case 0x1093:        /* CMPD direct */
                direct();
                cmp16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x10b3:        /* CMPD extended */
                extended();
                cmp16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x10a3:        /* CMPD indexed */
                indexed();
                cmp16(D, RDMEM16(ea), 2, 1);
                break;

#ifdef H6309
            case 0x1181:        /* CMPE immediate */
                cmp(E, imm_byte(), 0, 0);
                break;

            case 0x1191:        /* CMPE direct */
                direct();
                cmp(E, RDMEM(ea), 1, 0);
                break;

            case 0x11b1:        /* CMPE extended */
                extended();
                cmp(E, RDMEM(ea), 1, 0);
                break;

            case 0x11a1:        /* CMPE indexed */
                indexed();
                cmp(E, RDMEM(ea), 1, 1);
                break;

            case 0x11c1:        /* CMPF immediate */
                cmp(F, imm_byte(), 0, 0);
                break;

            case 0x11d1:        /* CMPF direct */
                direct();
                cmp(F, RDMEM(ea), 1, 0);
                break;

            case 0x11f1:        /* CMPF extended */
                extended();
                cmp(F, RDMEM(ea), 1, 0);
                break;

            case 0x11e1:        /* CMPF indexed */
                indexed();
                cmp(F, RDMEM(ea), 1, 1);
                break;

            case 0x1037:        /* CMPR R,R */
                post_byte = imm_byte();
                cmp16(get_reg((BYTE)(post_byte >> 4)), get_reg((BYTE)(post_byte & 0x0f)), 1, 1);
                break;
#endif

            case 0x118c:        /* CMPS immediate */
                cmp16(S, imm_word(), 1, 0);
                break;

            case 0x119c:        /* CMPS direct */
                direct();
                cmp16(S, RDMEM16(ea), 2, 0);
                break;

            case 0x11bc:        /* CMPS extended */
                extended();
                cmp16(S, RDMEM16(ea), 2, 0);
                break;

            case 0x11ac:        /* CMPS indexed */
                indexed();
                cmp16(S, RDMEM16(ea), 2, 1);
                break;

            case 0x1183:        /* CMPU immediate */
                cmp16(U, imm_word(), 1, 0);
                break;

            case 0x1193:        /* CMPU direct */
                direct();
                cmp16(U, RDMEM16(ea), 2, 0);
                break;

            case 0x11b3:        /* CMPU extended */
                extended();
                cmp16(U, RDMEM16(ea), 2, 0);
                break;

            case 0x11a3:        /* CMPU indexed */
                indexed();
                cmp16(U, RDMEM16(ea), 2, 1);
                break;

#ifdef H6309
            case 0x1081:        /* CMPW immediate */
                cmp16(W, imm_word(), 1, 0);
                break;

            case 0x1091:        /* CMPW direct */
                direct();
                cmp16(W, RDMEM16(ea), 2, 0);
                break;

            case 0x10b1:        /* CMPW extended */
                extended();
                cmp16(W, RDMEM16(ea), 2, 0);
                break;

            case 0x10a1:        /* CMPW indexed */
                indexed();
                cmp16(W, RDMEM16(ea), 2, 1);
                break;
#endif

            case 0x008c:        /* CMPX immediate */
                cmp16(X, imm_word(), 1, 0);
                break;

            case 0x009c:        /* CMPX direct */
                direct();
                cmp16(X, RDMEM16(ea), 2, 0);
                break;

            case 0x00bc:        /* CMPX extended */
                extended();
                cmp16(X, RDMEM16(ea), 2, 0);
                break;

            case 0x00ac:        /* CMPX indexed */
                indexed();
                cmp16(X, RDMEM16(ea), 2, 1);
                break;

            case 0x108c:        /* CMPY immediate */
                cmp16(Y, imm_word(), 1, 0);
                break;

            case 0x109c:        /* CMPY direct */
                direct();
                cmp16(Y, RDMEM16(ea), 2, 0);
                break;

            case 0x10bc:        /* CMPY extended */
                extended();
                cmp16(Y, RDMEM16(ea), 2, 0);
                break;

            case 0x10ac:        /* CMPY indexed */
                indexed();
                cmp16(Y, RDMEM16(ea), 2, 1);
                break;

            case 0x0043:        /* COMA */
#ifdef FULL6809
            case 0x1043:        /* COMA (UNDOC) */
            case 0x1143:        /* COMA (UNDOC) */
#endif
                A = com(A, 1, 0);
                break;

            case 0x0042:        /* COMA/NEGA (UNDOC) */
#ifdef FULL6809
            case 0x1042:        /* COMA/NEGA (UNDOC) */
            case 0x1142:        /* COMA/NEGA (UNDOC) */
                if (C) {
                    A = com(A, 1, 0);
                } else {
                    A = neg(A, 1, 0);
                }
                break;
#endif

            case 0x0053:        /* COMB */
#ifdef FULL6809
            case 0x1053:        /* COMB (UNDOC) */
            case 0x1153:        /* COMB (UNDOC) */
#endif
                B = com(B, 1, 0);
                break;

#ifdef FULL6809
            case 0x0052:        /* COMB/NEGB (UNDOC) */
            case 0x1052:        /* COMB/NEGB (UNDOC) */
            case 0x1152:        /* COMB/NEGB (UNDOC) */
                if (C) {
                    B = com(B, 1, 0);
                } else {
                    B = neg(B, 1, 0);
                }
                break;
#endif

#ifdef H6309
            case 0x1043:        /* COMD */
                D = com16(D, 1, 0);
                break;
#endif

#ifdef H6309
            case 0x1143:        /* COME */
                E = com(E, 1, 0);
                break;

            case 0x1153:        /* COMF */
                F = com(F, 1, 0);
                break;

            case 0x1053:        /* COMW */
                W = com16(W, 1, 0);
                break;
#endif

            case 0x0003:        /* COM direct */
#ifdef H6309
            case 0x1003:        /* COM direct (UNDOC) */
            case 0x1103:        /* COM direct (UNDOC) */
#endif
                direct();
                WRMEM(ea, com(RDMEM(ea), 2, 1));
                break;

#ifdef FULL6809
            case 0x0002:        /* COM/NEG direct (UNDOC) */
            case 0x1002:        /* COM/NEG direct (UNDOC) */
            case 0x1102:        /* COM/NEG direct (UNDOC) */
                direct();
                if (C) {
                    WRMEM(ea, com(RDMEM(ea), 2, 1));
                } else {
                    WRMEM(ea, neg(RDMEM(ea), 2, 1));
                }
                break;
#endif

            case 0x0073:        /* COM extended */
#ifdef FULL6809
            case 0x1073:        /* COM extended (UNDOC) */
            case 0x1173:        /* COM extended (UNDOC) */
#endif
                extended();
                WRMEM(ea, com(RDMEM(ea), 2, 1));
                break;

#ifdef FULL6809
            case 0x0072:        /* COM/NEG extended (UNDOC) */
            case 0x1072:        /* COM/NEG extended (UNDOC) */
            case 0x1172:        /* COM/NEG extended (UNDOC) */
                extended();
                if (C) {
                    WRMEM(ea, com(RDMEM(ea), 2, 1));
                } else {
                    WRMEM(ea, neg(RDMEM(ea), 2, 1));
                }
                break;
#endif

            case 0x0063:        /* COM indexed */
#ifdef FULL6809
            case 0x1063:        /* COM indexed (UNDOC) */
            case 0x1163:        /* COM indexed (UNDOC) */
#endif
                indexed();
                WRMEM(ea, com(RDMEM(ea), 1, 1));
                break;

#ifdef FULL6809
            case 0x0062:        /* COM/NEG indexed (UNDOC) */
            case 0x1062:        /* COM/NEG indexed (UNDOC) */
            case 0x1162:        /* COM/NEG indexed (UNDOC) */
                indexed();
                if (C) {
                    WRMEM(ea, com(RDMEM(ea), 1, 1));
                } else {
                    WRMEM(ea, neg(RDMEM(ea), 1, 1));
                }
                break;
#endif

            case 0x003c:        /* CWAI */
#ifdef FULL6809
            case 0x103c:        /* CWAI (UNDOC) */
            case 0x113c:        /* CWAI (UNDOC) */
#endif
                cwai(maincpu_int_status, maincpu_alarm_context);
                break;

            case 0x0019:        /* DAA */
#ifdef FULL6809
            case 0x1019:        /* DAA (UNDOC) */
            case 0x1119:        /* DAA (UNDOC) */
#endif
                daa();
                break;

            case 0x004a:        /* DECA */
#ifdef FULL6809
            case 0x004b:        /* DECA (UNDOC) */
            case 0x104a:        /* DECA (UNDOC) */
            case 0x104b:        /* DECA (UNDOC) */
            case 0x114a:        /* DECA (UNDOC) */
            case 0x114b:        /* DECA (UNDOC) */
#endif
                A = dec(A, 1, 0);
                break;

            case 0x005a:        /* DECB */
#ifdef FULL6809
            case 0x005b:        /* DECB (UNDOC) */
            case 0x105a:        /* DECB (UNDOC) */
            case 0x105b:        /* DECB (UNDOC) */
            case 0x115a:        /* DECB (UNDOC) */
            case 0x115b:        /* DECB (UNDOC) */
#endif
                B = dec(B, 1, 0);
                break;

#ifdef H6309
            case 0x104a:        /* DECD */
                D = dec16(D, 1, 0);
                break;

            case 0x114a:        /* DECE */
                E = dec(E, 1, 0);
                break;

            case 0x115a:        /* DECF */
                F = dec(F, 1, 0);
                break;

            case 0x105a:        /* DECW */
                W = dec16(W, 1, 0);
                break;
#endif

            case 0x000a:        /* DEC direct */
#ifdef FULL6809
            case 0x000b:        /* DEC direct (UNDOC) */
            case 0x100a:        /* DEC direct (UNDOC) */
            case 0x100b:        /* DEC direct (UNDOC) */
            case 0x110a:        /* DEC direct (UNDOC) */
            case 0x110b:        /* DEC direct (UNDOC) */
#endif
                direct();
                WRMEM(ea, dec(RDMEM(ea), 2, 1));
                break;

            case 0x007a:        /* DEC extended */
#ifdef FULL6809
            case 0x007b:        /* DEC extended (UNDOC) */
            case 0x107a:        /* DEC extended (UNDOC) */
            case 0x107b:        /* DEC extended (UNDOC) */
            case 0x117a:        /* DEC extended (UNDOC) */
            case 0x117b:        /* DEC extended (UNDOC) */
#endif
                extended();
                WRMEM(ea, dec(RDMEM(ea), 2, 1));
                break;

            case 0x006a:        /* DEC indexed */
#ifdef FULL6809
            case 0x006b:        /* DEC indexed (UNDOC) */
            case 0x106a:        /* DEC indexed (UNDOC) */
            case 0x106b:        /* DEC indexed (UNDOC) */
            case 0x116a:        /* DEC indexed (UNDOC) */
            case 0x116b:        /* DEC indexed (UNDOC) */
#endif
                indexed();
                WRMEM(ea, dec(RDMEM(ea), 2, 2));
                break;

#ifdef H6309
            case 0x118d:        /* DIVD immediate */
                divd(imm_byte(), 22, 22);
                break;

            case 0x119d:        /* DIVD direct */
                direct();
                divd(RDMEM(ea), 23, 22);
                break;

            case 0x11bd:        /* DIVD extended */
                extended();
                divd(RDMEM(ea), 23, 22);
                break;

            case 0x11ad:        /* DIVD indexed */
                indexed();
                divd(RDMEM(ea), 23, 23);
                break;

            case 0x118e: /* DIVQ immediate */
                divq(imm_word(), 30, 30);
                break;

            case 0x119e:        /* DIVQ direct */
                direct();
                divq(RDMEM16(ea), 31, 30);
                break;

            case 0x11be:        /* DIVQ extended */
                extended();
                divq(RDMEM16(ea), 31, 30);
                break;

            case 0x11ae:        /* DIVQ indexed */
                indexed();
                divq(RDMEM16(ea), 31, 31);
                break;

            case 0x0005:        /* EIM post, direct */
                post_byte = imm_byte();
                direct();
                WRMEM(ea, eor(RDMEM(ea), post_byte, 1, 1));
                break;

            case 0x0075:        /* EIM extended */
                post_byte = imm_byte();
                extended();
                WRMEM(ea, eor(RDMEM(ea), post_byte, 1, 1));
                break;

            case 0x0065:        /* EIM indexed */
                post_byte = imm_byte();
                indexed();
                WRMEM(ea, eor(RDMEM(ea), post_byte, 2, 2));
                break;
#endif

            case 0x0088:        /* EORA immediate */
#ifdef FULL6809
            case 0x1088:        /* EORA immediate (UNDOC) */
            case 0x1188:        /* EORA immediate (UNDOC) */
#endif
                A = eor(A, imm_byte(), 0, 0);
                break;

            case 0x0098:        /* EORA direct */
#ifdef FULL6809
            case 0x1098:        /* EORA direct (UNDOC) */
            case 0x1198:        /* EORA direct (UNDOC) */
#endif
                direct();
                A = eor(A, RDMEM(ea), 1, 0);
                break;

            case 0x00b8:        /* EORA extended */
#ifdef FULL6809
            case 0x10b8:        /* EORA extended (UNDOC) */
            case 0x11b8:        /* EORA extended (UNDOC) */
#endif
                extended();
                A = eor(A, RDMEM(ea), 1, 0);
                break;

            case 0x00a8:        /* EORA indexed */
#ifdef FULL6809
            case 0x10a8:        /* EORA indexed (UNDOC) */
            case 0x11a8:        /* EORA indexed (UNDOC) */
#endif
                indexed();
                A = eor(A, RDMEM(ea), 1, 1);
                break;

            case 0x00c8:        /* EORB immediate */
#ifdef FULL6809
            case 0x10c8:        /* EORB immediate (UNDOC) */
            case 0x11c8:        /* EORB immediate (UNDOC) */
#endif
                B = eor(B, imm_byte(), 0, 0);
                break;

            case 0x00d8:        /* EORB direct */
#ifdef FULL6809
            case 0x10d8:        /* EORB direct (UNDOC) */
            case 0x11d8:        /* EORB direct (UNDOC) */
#endif
                direct();
                B = eor(B, RDMEM(ea), 1, 0);
                break;

            case 0x00f8:        /* EORB extended */
#ifdef FULL6809
            case 0x10f8:        /* EORB extended (UNDOC) */
            case 0x11f8:        /* EORB extended (UNDOC) */
#endif
                extended();
                B = eor(B, RDMEM(ea), 1, 0);
                break;

            case 0x00e8:        /* EORB indexed */
#ifdef FULL6809
            case 0x10e8:        /* EORB indexed (UNDOC) */
            case 0x11e8:        /* EORB indexed (UNDOC) */
#endif
                indexed();
                B = eor(B, RDMEM(ea), 1, 1);
                break;

#ifdef H6309
            case 0x1088:        /* EORD immediate */
                D = eor16(D, imm_word(), 1, 0);
                break;

            case 0x1098:        /* EORD direct */
                direct();
                D = eor16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x10b8:        /* EORD extended */
                extended();
                D = eor16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x10a8:        /* EORD indexed */
                indexed();
                D = eor16(D, RDMEM16(ea), 2, 1);
                break;
#endif

#ifdef H6309
            case 0x1036: /* EORR post */
                post_byte = imm_byte();
                set_reg((BYTE)(post_byte & 0x0f), eor16(get_reg((BYTE)(post_byte >> 4)), get_reg((BYTE)(post_byte & 0x0f)), 1, 1));
                break;
#endif

            case 0x001e:        /* EXG post */
#ifdef FULL6809
            case 0x101e:        /* EXG post (UNDOC) */
            case 0x111e:        /* EXG post (UNDOC) */
#endif
                exg();
                break;

#ifdef FULL6809
            case 0x0014:        /* HCF (UNDOC) */
            case 0x0015:        /* HCF (UNDOC) */
            case 0x00cd:        /* HCF (UNDOC) */
            case 0x1014:        /* HCF (UNDOC) */
            case 0x1015:        /* HCF (UNDOC) */
            case 0x10cd:        /* HCF (UNDOC) */
            case 0x1114:        /* HCF (UNDOC) */
            case 0x1115:        /* HCF (UNDOC) */
            case 0x11cd:        /* HCF (UNDOC) */
                hcf();
                break;
#endif

            case 0x004c:        /* INCA */
#ifdef FULL6809
            case 0x104c:        /* INCA (UNDOC) */
            case 0x114c:        /* INCA (UNDOC) */
#endif
                A = inc(A, 1, 0);
                break;

            case 0x005c:        /* INCB */
#ifdef FULL6809
            case 0x105c:        /* INCB (UNDOC) */
            case 0x115c:        /* INCB (UNDOC) */
#endif
                B = inc(B, 1, 0);
                break;

#ifdef H6309
            case 0x104c:        /* INCD */
                D = inc16(D, 1, 0);
                break;

            case 0x114c:        /* INCE */
                E = inc(E, 1, 0);
                break;

            case 0x115c:        /* INCF */
                F = inc(F, 1, 0);
                break;

            case 0x105c:        /* INCW */
                W = inc16(W, 1, 0);
                break;
#endif

            case 0x000c:        /* INC direct */
#ifdef FULL6809
            case 0x100c:        /* INC direct (UNDOC) */
            case 0x110c:        /* INC direct (UNDOC) */
#endif
                direct();
                WRMEM(ea, inc(RDMEM(ea), 2, 1));
                break;

            case 0x007c:        /* INC extended */
#ifdef FULL6809
            case 0x107c:        /* INC extended (UNDOC) */
            case 0x117c:        /* INC extended (UNDOC) */
#endif
                extended();
                WRMEM(ea, inc(RDMEM(ea), 2, 1));
                break;

            case 0x006c:        /* INC indexed */
#ifdef FULL6809
            case 0x106c:        /* INC indexed (UNDOC) */
            case 0x116c:        /* INC indexed (UNDOC) */
#endif
                indexed();
                WRMEM(ea, inc(RDMEM(ea), 2, 2));
                break;

            case 0x000e:        /* JMP direct */
#ifdef FULL6809
            case 0x100e:        /* JMP direct (UNDOC) */
            case 0x110e:        /* JMP direct (UNDOC) */
#endif
                direct();
                CLK_ADD(1, 0);
                PC = ea;
                break;

            case 0x007e:        /* JMP extended */
#ifdef FULL6809
            case 0x107e:        /* JMP extended (UNDOC) */
            case 0x117e:        /* JMP extended (UNDOC) */
#endif
                extended();
                CLK_ADD(1, 0);
                PC = ea;
                break;

            case 0x006e:        /* JMP indexed */
#ifdef FULL6809
            case 0x106e:        /* JMP indexed (UNDOC) */
            case 0x116e:        /* JMP indexed (UNDOC) */
#endif
                indexed();
                CLK++;
                PC = ea;
                break;

            case 0x009d:        /* JSR direct */
#ifdef FULL6809
            case 0x109d:        /* JSR direct (UNDOC) */
            case 0x119d:        /* JSR direct (UNDOC) */
#endif
                direct();
                jsr();
                break;

            case 0x00bd:        /* JSR extended */
#ifdef FULL6809
            case 0x10bd:        /* JSR extended (UNDOC) */
            case 0x11bd:        /* JSR extended (UNDOC) */
#endif
                extended();
                jsr();
                break;

            case 0x00ad:        /* JSR indexed */
#ifdef FULL6809
            case 0x10ad:        /* JSR indexed (UNDOC) */
            case 0x11ad:        /* JSR indexed (UNDOC) */
#endif
                indexed();
                jsr();
                break;

            case 0x1027:        /* LBEQ */
                long_branch(cond_EQ());
                break;

            case 0x102c:        /* LBGE */
                long_branch(cond_GE());
                break;

            case 0x102e:        /* LBGT */
                long_branch(cond_GT());
                break;

            case 0x1022:        /* LBHI */
                long_branch(cond_HI());
                break;

            case 0x1024:        /* LBHS */
                long_branch(cond_HS());
                break;

            case 0x102f:        /* LBLE */
                long_branch(cond_LE());
                break;

            case 0x1025:        /* LBLO */
                long_branch(cond_LO());
                break;

            case 0x1023:        /* LBLS */
                long_branch(cond_LS());
                break;

            case 0x102d:        /* LBLT */
                long_branch(cond_LT());
                break;

            case 0x102b:        /* LBMI */
                long_branch(cond_MI());
                break;

            case 0x1026:        /* LBNE */
                long_branch(cond_NE());
                break;

            case 0x102a:        /* LBPL */
                long_branch(cond_PL());
                break;

            case 0x0016:        /* LBRA */
#ifdef FULL6809
            case 0x1016:        /* LBRA (UNDOC) */
            case 0x1020:        /* LBRA (UNDOC) */
            case 0x1116:        /* LBRA (UNDOC) */
#endif
                long_bra();
                break;

            case 0x1021:        /* LBRN */
                PC += 2;
                CLK_ADD(3, 4);
                break;

            case 0x0017:        /* LBSR */
#ifdef FULL6809
            case 0x1017:        /* LBSR (UNDOC) */
            case 0x1117:        /* LBSR (UNDOC) */
#endif
                long_bsr();
                break;

            case 0x1028:        /* LBVC */
                long_branch(cond_VC());
                break;

            case 0x1029:        /* LBVS */
                long_branch(cond_VS());
                break;

            case 0x0086:        /* LDA immediate */
#ifdef FULL6809
            case 0x1086:        /* LDA immediate (UNDOC) */
            case 0x1186:        /* LDA immediate (UNDOC) */
#endif
                A = ld(imm_byte(), 0, 0);
                break;

            case 0x0096:        /* LDA direct */
#ifdef FULL6809
            case 0x1096:        /* LDA direct (UNDOC) */
            case 0x1196:        /* LDA direct (UNDOC) */
#endif
                direct();
                A = ld(RDMEM(ea), 1, 0);
                break;

            case 0x00b6:        /* LDA extended */
#ifdef FULL6809
            case 0x10b6:        /* LDA extended (UNDOC) */
            case 0x11b6:        /* LDA extended (UNDOC) */
#endif
                extended();
                A = ld(RDMEM(ea), 1, 0);
                break;

            case 0x00a6:        /* LDA indexed */
#ifdef FULL6809
            case 0x10a6:        /* LDA indexed (UNDOC) */
            case 0x11a6:        /* LDA indexed (UNDOC) */
#endif
                indexed();
                A = ld(RDMEM(ea), 1, 1);
                break;

            case 0x00c6:        /* LDB immediate */
#ifdef FULL6809
            case 0x10c6:        /* LDB immediate (UNDOC) */
            case 0x11c6:        /* LDB immediate (UNDOC) */
#endif
                B = ld(imm_byte(), 0, 0);
                break;

            case 0x00d6:        /* LDB direct */
#ifdef FULL6809
            case 0x10d6:        /* LDB direct (UNDOC) */
            case 0x11d6:        /* LDB direct (UNDOC) */
#endif
                direct();
                B = ld(RDMEM(ea), 1, 0);
                break;

            case 0x00f6:        /* LDB extended */
#ifdef FULL6809
            case 0x10f6:        /* LDB extended (UNDOC) */
            case 0x11f6:        /* LDB extended (UNDOC) */
#endif
                extended();
                B = ld(RDMEM(ea), 1, 0);
                break;

            case 0x00e6:        /* LDB indexed */
#ifdef FULL6809
            case 0x10e6:        /* LDB indexed (UNDOC) */
            case 0x11e6:        /* LDB indexed (UNDOC) */
#endif
                indexed();
                B = ld(RDMEM(ea), 1, 1);
                break;

#ifdef H6309
            case 0x1136:        /* LDBT post,direct */
                post_byte = imm_byte();
                direct();
                ldbt(post_byte, RDMEM(ea));
                break;
#endif

            case 0x00cc:        /* LDD immediate */
#ifdef FULL6809
            case 0x10cc:        /* LDD immediate (UNDOC) */
            case 0x11cc:        /* LDD immediate (UNDOC) */
#endif
                D = ld16(imm_word(), 0, 0);
                break;

            case 0x00dc:        /* LDD direct */
#ifdef FULL6809
            case 0x10dc:        /* LDD direct (UNDOC) */
            case 0x11dc:        /* LDD direct (UNDOC) */
#endif
                direct();
                D = ld16(RDMEM16(ea), 1, 0);
                break;

            case 0x00fc:        /* LDD extended */
#ifdef FULL6809
            case 0x10fc:        /* LDD extended (UNDOC) */
            case 0x11fc:        /* LDD extended (UNDOC) */
#endif
                extended();
                D = ld16(RDMEM16(ea), 1, 0);
                break;

            case 0x00ec:        /* LDD indexed */
#ifdef FULL6809
            case 0x10ec:        /* LDD indexed (UNDOC) */
            case 0x11ec:        /* LDD indexed (UNDOC) */
#endif
                indexed();
                D = ld16(RDMEM16(ea), 1, 1);
                break;

#ifdef H6309
            case 0x1186:        /* LDE immediate */
                E = ld(imm_byte(), 0, 0);
                break;

            case 0x1196:        /* LDE direct */
                direct();
                E = ld(RDMEM(ea), 1, 0);
                break;

            case 0x11b6:        /* LDE extended */
                extended();
                E = ld(RDMEM(ea), 1, 0);
                break;

            case 0x11a6:        /* LDE indexed */
                indexed();
                E = ld(RDMEM(ea), 1, 1);
                break;

            case 0x11c6:        /* LDF immediate */
                F = ld(imm_byte(), 0, 0);
                break;

            case 0x11d6:        /* LDF direct */
                direct();
                F = ld(RDMEM(ea), 1, 0);
                break;

            case 0x11f6:        /* LDF extended */
                extended();
                F = ld(RDMEM(ea), 1, 0);
                break;

            case 0x11e6:        /* LDF indexed */
                indexed();
                F = ld(RDMEM(ea), 1, 1);
                break;

            case 0x113d:        /* LDMD immediate */
                set_md(ld(imm_byte(), 2, 2));
                break;

            case 0x00cd:        /* LDQ immediate */
                Q = ld32(imm_dword(), 0, 0);
                break;

            case 0x10dc:        /* LDQ direct */
                direct();
                Q = ld32(RDMEM32(ea), 1, 0);
                break;

            case 0x10fc:        /* LDQ extended */
                extended();
                Q = ld32(RDMEM32(ea), 1, 0);
                break;

            case 0x10ec:        /* LDQ indexed */
                indexed();
                Q = ld32(RDMEM32(ea), 1, 1);
                break;
#endif

            case 0x10ce:        /* LDS immediate */
                S = ld16(imm_word(), 0, 0);
                break;

            case 0x10de:        /* LDS direct */
                direct();
                S = ld16(RDMEM16(ea), 1, 0);
                break;

            case 0x10fe:        /* LDS extended */
                extended();
                S = ld16(RDMEM16(ea), 1, 0);
                break;

            case 0x10ee:        /* LDS indexed */
                indexed();
                S = ld16(RDMEM16(ea), 1, 1);
                break;

            case 0x00ce:        /* LDU immediate */
#ifdef FULL6809
            case 0x11ce:        /* LDU immediate (UNDOC) */
#endif
                U = ld16(imm_word(), 0, 0);
                break;

            case 0x00de:        /* LDU direct */
#ifdef FULL6809
            case 0x11de:        /* LDU direct (UNDOC) */
#endif
                direct();
                U = ld16(RDMEM16(ea), 1, 0);
                break;

            case 0x00fe:        /* LDU extended */
#ifdef FULL6809
            case 0x11fe:        /* LDU extended (UNDOC) */
#endif
                extended();
                U = ld16(RDMEM16(ea), 1, 0);
                break;

            case 0x00ee:        /* LDU indexed */
#ifdef FULL6809
            case 0x11ee:        /* LDU indexed (UNDOC) */
#endif
                indexed();
                U = ld16(RDMEM16(ea), 1, 1);
                break;

#ifdef H6309
            case 0x1086:        /* LDW immediate */
                W = ld16(imm_word(), 0, 0);
                break;

            case 0x1096:        /* LDW direct */
                direct();
                W = ld16(RDMEM16(ea), 1, 0);
                break;

            case 0x10b6:        /* LDW extended */
                extended();
                W = ld16(RDMEM16(ea), 1, 0);
                break;

            case 0x10a6:        /* LDW indexed */
                indexed();
                W = ld16(RDMEM16(ea), 1, 1);
                break;
#endif

            case 0x008e:        /* LDX immediate */
#ifdef FULL6809
            case 0x118e:        /* LDX immediate (UNDOC) */
#endif
                X = ld16(imm_word(), 0, 0);
                break;

            case 0x009e:        /* LDX direct */
#ifdef FULL6809
            case 0x119e:        /* LDX direct (UNDOC) */
#endif
                direct();
                X = ld16(RDMEM16(ea), 1, 0);
                break;

            case 0x00be:        /* LDX extended */
#ifdef FULL6809
            case 0x11be:        /* LDX extended (UNDOC) */
#endif
                extended();
                X = ld16(RDMEM16(ea), 1, 0);
                break;

            case 0x00ae:        /* LDX indexed */
#ifdef FULL6809
            case 0x11ae:        /* LDX indexed (UNDOC) */
#endif
                indexed();
                X = ld16(RDMEM16(ea), 1, 1);
                break;

            case 0x108e:        /* LDY immediate */
                Y = ld16(imm_word(), 0, 0);
                break;

            case 0x109e:        /* LDY direct */
                direct();
                Y = ld16(RDMEM16(ea), 1, 0);
                break;

            case 0x10be:        /* LDY extended */
                extended();
                Y = ld16(RDMEM16(ea), 1, 0);
                break;

            case 0x10ae:        /* LDY indexed */
                indexed();
                Y = ld16(RDMEM16(ea), 1, 0);
                break;

            case 0x0032:        /* LEAS indexed */
#ifdef FULL6809
            case 0x1032:        /* LEAS indexed (UNDOC) */
            case 0x1132:        /* LEAS indexed (UNDOC) */
#endif
                indexed();
                S = ea;
                CLK += 2;
                break;

            case 0x0033:        /* LEAU indexed */
#ifdef FULL6809
            case 0x1033:        /* LEAU indexed (UNDOC) */
            case 0x1133:        /* LEAU indexed (UNDOC) */
#endif
                indexed();
                U = ea;
                CLK += 2;
                break;

            case 0x0030:        /* LEAX indexed */
#ifdef FULL6809
            case 0x1030:        /* LEAX indexed (UNDOC) */
            case 0x1130:        /* LEAX indexed (UNDOC) */
#endif
                indexed();
                Z = X = ea;
                CLK += 2;
                break;

            case 0x0031:        /* LEAY indexed */
#ifdef FULL6809
            case 0x1031:        /* LEAY indexed (UNDOC) */
            case 0x1131:        /* LEAY indexed (UNDOC) */
#endif
                indexed();
                Z = Y = ea;
                CLK += 2;
                break;

            case 0x0044:        /* LSRA */
#ifdef FULL6809
            case 0x0045:        /* LSRA (UNDOC) */
            case 0x1044:        /* LSRA (UNDOC) */
            case 0x1045:        /* LSRA (UNDOC) */
            case 0x1144:        /* LSRA (UNDOC) */
            case 0x1145:        /* LSRA (UNDOC) */
#endif
                A = lsr(A, 1, 0);
                break;

            case 0x0054:        /* LSRB */
#ifdef FULL6809
            case 0x0055:        /* LSRB (UNDOC) */
            case 0x1054:        /* LSRB (UNDOC) */
            case 0x1055:        /* LSRB (UNDOC) */
            case 0x1154:        /* LSRB (UNDOC) */
            case 0x1155:        /* LSRB (UNDOC) */
#endif
                B = lsr(B, 1, 0);
                break;

#ifdef H6309
            case 0x1044:        /* LSRD */
                D = lsr16(D, 1, 0);
                break;

            case 0x1054:        /* LSRW */
                W = lsr16(W, 1, 0);
                break;
#endif

            case 0x0004:        /* LSR direct */
#ifdef FULL6809
            case 0x0005:        /* LSR direct (UNDOC) */
            case 0x1004:        /* LSR direct (UNDOC) */
            case 0x1005:        /* LSR direct (UNDOC) */
            case 0x1104:        /* LSR direct (UNDOC) */
            case 0x1105:        /* LSR direct (UNDOC) */
#endif
                direct();
                WRMEM(ea, lsr(RDMEM(ea), 2, 1));
                break;

            case 0x0074:        /* LSR extended */
#ifdef FULL6809
            case 0x0075:        /* LSR extended (UNDOC) */
            case 0x1074:        /* LSR extended (UNDOC) */
            case 0x1075:        /* LSR extended (UNDOC) */
            case 0x1174:        /* LSR extended (UNDOC) */
            case 0x1175:        /* LSR extended (UNDOC) */
#endif
                extended();
                WRMEM(ea, lsr(RDMEM(ea), 2, 1));
                break;

            case 0x0064:        /* LSR indexed */
#ifdef FULL6809
            case 0x0065:        /* LSR indexed (UNDOC) */
            case 0x1064:        /* LSR indexed (UNDOC) */
            case 0x1065:        /* LSR indexed (UNDOC) */
            case 0x1164:        /* LSR indexed (UNDOC) */
            case 0x1165:        /* LSR indexed (UNDOC) */
#endif
                indexed();
                WRMEM(ea, lsr(RDMEM(ea), 2, 2));
                break;

            case 0x003d:        /* MUL */
#ifdef FULL6809
            case 0x103d:        /* MUL (UNDOC) */
            case 0x113d:        /* MUL (UNDOC) */
#endif
                mul();
                break;

#ifdef H6309
            case 0x118f: /* MULD immediate */
                muld(imm_word(), 24, 24);
                break;

            case 0x119f:        /* MULD direct */
                direct();
                muld(RDMEM16(ea), 25, 24);
                break;

            case 0x11bf:        /* MULD extended */
                extended();
                muld(RDMEM16(ea), 25, 24);
                break;

            case 0x11af:        /* MULD indexed */
                indexed();
                muld(RDMEM16(ea), 25, 25);
                break;
#endif

            case 0x0040:        /* NEGA */
#ifdef FULL6809
            case 0x0041:        /* NEGA (UNDOC) */
            case 0x1040:        /* NEGA (UNDOC) */
            case 0x1041:        /* NEGA (UNDOC) */
            case 0x1140:        /* NEGA (UNDOC) */
            case 0x1141:        /* NEGA (UNDOC) */
#endif
                A = neg(A, 1, 0);
                break;

            case 0x0050:        /* NEGB */
#ifdef FULL6809
            case 0x0051:        /* NEGB (UNDOC) */
            case 0x1050:        /* NEGB (UNDOC) */
            case 0x1051:        /* NEGB (UNDOC) */
            case 0x1150:        /* NEGB (UNDOC) */
            case 0x1151:        /* NEGB (UNDOC) */
#endif
                B = neg(B, 1, 0);
                break;

#ifdef H6309
            case 0x1040:        /* NEGD */
                D = neg16(D, 1, 0);
                break;
#endif

            case 0x0000:        /* NEG direct */
#ifdef FULL6809
            case 0x0001:        /* NEG direct (UNDOC) */
            case 0x1000:        /* NEG direct (UNDOC) */
            case 0x1001:        /* NEG direct (UNDOC) */
            case 0x1100:        /* NEG direct (UNDOC) */
            case 0x1101:        /* NEG direct (UNDOC) */
#endif
                direct();
                WRMEM(ea, neg(RDMEM(ea), 2, 1));
                break;

            case 0x0070:        /* NEG extended */
#ifdef FULL6809
            case 0x0071:        /* NEG extended (UNDOC) */
            case 0x1070:        /* NEG extended (UNDOC) */
            case 0x1071:        /* NEG extended (UNDOC) */
            case 0x1170:        /* NEG extended (UNDOC) */
            case 0x1171:        /* NEG extended (UNDOC) */
#endif
                extended();
                WRMEM(ea, neg(RDMEM(ea), 2, 1));
                break;

            case 0x0060:        /* NEG indexed */
#ifdef FULL6809
            case 0x0061:        /* NEG indexed (UNDOC) */
            case 0x1060:        /* NEG indexed (UNDOC) */
            case 0x1061:        /* NEG indexed (UNDOC) */
            case 0x1160:        /* NEG indexed (UNDOC) */
            case 0x1161:        /* NEG indexed (UNDOC) */
#endif
                indexed();
                WRMEM(ea, neg(RDMEM(ea), 2, 2));
                break;

            case 0x0012:        /* NOP */
#ifdef FULL6809
            case 0x001b:        /* NOP (UNDOC) */
            case 0x1012:        /* NOP (UNDOC) */
            case 0x101b:        /* NOP (UNDOC) */
            case 0x1112:        /* NOP (UNDOC) */
            case 0x111b:        /* NOP (UNDOC) */
#endif
                CLK_ADD(1, 0);
                break;

#ifdef H6309
            case 0x0001:        /* OIM post, direct */
                post_byte = imm_byte();
                direct();
                WRMEM(ea, or(RDMEM(ea), post_byte, 1, 1));
                break;

            case 0x0071:        /* OIM extended */
                post_byte = imm_byte();
                extended();
                WRMEM(ea, or(RDMEM(ea), post_byte, 1, 1));
                break;

            case 0x0061:        /* OIM indexed */
                post_byte = imm_byte();
                indexed();
                WRMEM(ea, or(RDMEM(ea), post_byte, 2, 2));
                break;
#endif

            case 0x008a:        /* ORA immediate */
#ifdef FULL6809
            case 0x108a:        /* ORA immediate (UNDOC) */
            case 0x118a:        /* ORA immediate (UNDOC) */
#endif
                A = or(A, imm_byte(), 0, 0);
                break;

            case 0x009a:        /* ORA direct */
#ifdef FULL6809
            case 0x109a:        /* ORA direct (UNDOC) */
            case 0x119a:        /* ORA direct (UNDOC) */
#endif
                direct();
                A = or(A, RDMEM(ea), 1, 0);
                break;

            case 0x00ba:        /* ORA extended */
#ifdef FULL6809
            case 0x10ba:        /* ORA extended (UNDOC) */
            case 0x11ba:        /* ORA extended (UNDOC) */
#endif
                extended();
                A = or(A, RDMEM(ea), 1, 0);
                break;

            case 0x00aa:        /* ORA indexed */
#ifdef FULL6809
            case 0x10aa:        /* ORA indexed (UNDOC) */
            case 0x11aa:        /* ORA indexed (UNDOC) */
#endif
                indexed();
                A = or(A, RDMEM(ea), 1, 1);
                break;

            case 0x00ca:        /* ORB immediate */
#ifdef FULL6809
            case 0x10ca:        /* ORB immediate (UNDOC) */
            case 0x11ca:        /* ORB immediate (UNDOC) */
#endif
                B = or(B, imm_byte(), 0, 0);
                break;

            case 0x00da:        /* ORB direct */
#ifdef FULL6809
            case 0x10da:        /* ORB direct (UNDOC) */
            case 0x11da:        /* ORB direct (UNDOC) */
#endif
                direct();
                B = or(B, RDMEM(ea), 1, 0);
                break;

            case 0x00fa:        /* ORB extended */
#ifdef FULL6809
            case 0x10fa:        /* ORB extended (UNDOC) */
            case 0x11fa:        /* ORB extended (UNDOC) */
#endif
                extended();
                B = or(B, RDMEM(ea), 1, 0);
                break;

            case 0x00ea:        /* ORB indexed */
#ifdef FULL6809
            case 0x10ea:        /* ORB indexed (UNDOC) */
            case 0x11ea:        /* ORB indexed (UNDOC) */
#endif
                indexed();
                B = or(B, RDMEM(ea), 1, 1);
                break;

            case 0x001a:        /* ORCC immediate */
#ifdef FULL6809
            case 0x101a:        /* ORCC immediate (UNDOC) */
            case 0x111a:        /* ORCC immediate (UNDOC) */
#endif
                orcc();
                break;

#ifdef H6309
            case 0x108a:        /* ORD immediate */
                D = or16(D, imm_word(), 1, 0);
                break;

            case 0x109a:        /* ORD direct */
                direct();
                D = or16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x10ba:        /* ORD extended */
                extended();
                D = or16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x10aa:        /* ORD indexed */
                indexed();
                D = or16(D, RDMEM16(ea), 2, 1);
                break;

            case 0x1035:        /* ORR post */
                post_byte = imm_byte();
                set_reg((BYTE)(post_byte & 0x0f), or16(get_reg((BYTE)(post_byte >> 4)), get_reg((BYTE)(post_byte & 0x0f)), 1, 1));
                break;
#endif

            case 0x0034:        /* PSHS post */
#ifdef FULL6809
            case 0x1034:        /* PSHS post (UNDOC) */
            case 0x1134:        /* PSHS post (UNDOC) */
#endif
                pshs();
                break;

#ifdef H6309
            case 0x1038:        /* PSHSW */
                pshsw();
                break;
#endif

            case 0x0036:        /* PSHU post */
#ifdef FULL6809
            case 0x1036:        /* PSHU post (UNDOC) */
            case 0x1136:        /* PSHU post (UNDOC) */
#endif
                pshu();
                break;

#ifdef H6309
            case 0x103a:        /* PSHUW */
                pshuw();
                break;
#endif

            case 0x0035:        /* PULS post */
#ifdef FULL6809
            case 0x1035:        /* PULS post (UNDOC) */
            case 0x1135:        /* PULS post (UNDOC) */
#endif
                puls();
                break;

#ifdef H6309
            case 0x1039:        /* PULSW */
                pulsw();
                break;
#endif

            case 0x0037:        /* PULU post */
#ifdef FULL6809
            case 0x1037:        /* PULU post (UNDOC) */
            case 0x1137:        /* PULU post (UNDOC) */
#endif
                pulu();
                break;

#ifdef H6309
            case 0x103b:        /* PULUW */
                puluw();
                break;
#endif

            case 0x0049:        /* ROLA */
#ifdef FULL6809
            case 0x1049:        /* ROLA (UNDOC) */
            case 0x1149:        /* ROLA (UNDOC) */
#endif
                A = rol(A, 1, 0);
                break;

            case 0x0059:        /* ROLB */
#ifdef FULL6809
            case 0x1059:        /* ROLB (UNDOC) */
            case 0x1159:        /* ROLB (UNDOC) */
#endif
                B = rol(B, 1, 0);
                break;

#ifdef H6309
            case 0x1049:        /* ROLD */
                D = rol16(D);
                break;

            case 0x1059:        /* ROLW */
                W = rol16(W);
                break;
#endif

            case 0x0009:        /* ROL direct */
#ifdef FULL6809
            case 0x1009:        /* ROL direct (UNDOC) */
            case 0x1109:        /* ROL direct (UNDOC) */
#endif
                direct();
                WRMEM(ea, rol(RDMEM(ea), 2, 1));
                break;

            case 0x0079:        /* ROL extended */
#ifdef FULL6809
            case 0x1079:        /* ROL extended (UNDOC) */
            case 0x1179:        /* ROL extended (UNDOC) */
#endif
                extended();
                WRMEM(ea, rol(RDMEM(ea), 2, 1));
                break;

            case 0x0069:        /* ROL indexed */
#ifdef FULL6809
            case 0x1069:        /* ROL indexed (UNDOC) */
            case 0x1169:        /* ROL indexed (UNDOC) */
#endif
                indexed();
                WRMEM(ea, rol(RDMEM(ea), 2, 2));
                break;

            case 0x0046:        /* RORA */
#ifdef FULL6809
            case 0x1046:        /* RORA (UNDOC) */
            case 0x1146:        /* RORA (UNDOC) */
#endif
                A = ror(A, 1, 0);
                break;

            case 0x0056:        /* RORB */
#ifdef FULL6809
            case 0x1056:        /* RORB (UNDOC) */
            case 0x1156:        /* RORB (UNDOC) */
#endif
                B = ror(B, 1, 0);
                break;

#ifdef H6309
            case 0x1046:        /* RORD */
                D = ror16(D);
                break;

            case 0x1056:        /* RORW */
                W = ror16(W);
                break;
#endif

            case 0x0006:        /* ROR direct */
#ifdef FULL6809
            case 0x1006:        /* ROR direct (UNDOC) */
            case 0x1106:        /* ROR direct (UNDOC) */
#endif
                direct();
                WRMEM(ea, ror(RDMEM(ea), 2, 1));
                break;

            case 0x0076:        /* ROR extended */
#ifdef FULL6809
            case 0x1076:        /* ROR extended (UNDOC) */
            case 0x1176:        /* ROR extended (UNDOC) */
#endif
                extended();
                WRMEM(ea, ror(RDMEM(ea), 2, 1));
                break;

            case 0x0066:        /* ROR indexed */
#ifdef FULL6809
            case 0x1066:        /* ROR indexed (UNDOC) */
            case 0x1166:        /* ROR indexed (UNDOC) */
#endif
                indexed();
                WRMEM(ea, ror(RDMEM(ea), 2, 2));
                break;

            case 0x003b:        /* RTI */
#ifdef FULL6809
            case 0x103b:        /* RTI (UNDOC) */
            case 0x113b:        /* RTI (UNDOC) */
#endif
                rti();
                break;

            case 0x0039:        /* RTS */
#ifdef FULL6809
            case 0x1039:        /* RTS (UNDOC) */
            case 0x1139:        /* RTS (UNDOC) */
#endif
                rts();
                break;

            case 0x0082:        /* SBCA immediate */
#ifdef FULL6809
            case 0x1082:        /* SBCA immediate (UNDOC) */
            case 0x1182:        /* SBCA immediate (UNDOC) */
#endif
                A = sbc(A, imm_byte(), 0, 0);
                break;

            case 0x0092:        /* SBCA direct */
#ifdef FULL6809
            case 0x1092:        /* SBCA direct (UNDOC) */
            case 0x1192:        /* SBCA direct (UNDOC) */
#endif
                direct();
                A = sbc(A, RDMEM(ea), 1, 0);
                break;

            case 0x00b2:        /* SBCA extended */
#ifdef FULL6809
            case 0x10b2:        /* SBCA extended (UNDOC) */
            case 0x11b2:        /* SBCA extended (UNDOC) */
#endif
                extended();
                A = sbc(A, RDMEM(ea), 1, 0);
                break;

            case 0x00a2:        /* SBCA indexed */
#ifdef FULL6809
            case 0x10a2:        /* SBCA indexed (UNDOC) */
            case 0x11a2:        /* SBCA indexed (UNDOC) */
#endif
                indexed();
                A = sbc(A, RDMEM(ea), 1, 1);
                break;

            case 0x00c2:        /* SBCB immediate */
#ifdef FULL6809
            case 0x10c2:        /* SBCB immediate (UNDOC) */
            case 0x11c2:        /* SBCB immediate (UNDOC) */
#endif
                B = sbc(B, imm_byte(), 0, 0);
                break;

            case 0x00d2:        /* SBCB direct */
#ifdef FULL6809
            case 0x10d2:        /* SBCB direct (UNDOC) */
            case 0x11d2:        /* SBCB direct (UNDOC) */
#endif
                direct();
                B = sbc(B, RDMEM(ea), 1, 0);
                break;

            case 0x00f2:        /* SBCB extended */
#ifdef FULL6809
            case 0x10f2:        /* SBCB extended (UNDOC) */
            case 0x11f2:        /* SBCB extended (UNDOC) */
#endif
                extended();
                B = sbc(B, RDMEM(ea), 1, 0);
                break;

            case 0x00e2:        /* SBCB indexed */
#ifdef FULL6809
            case 0x10e2:        /* SBCB indexed (UNDOC) */
            case 0x11e2:        /* SBCB indexed (UNDOC) */
#endif
                indexed();
                B = sbc(B, RDMEM(ea), 1, 1);
                break;

#ifdef H6309
            case 0x1082:        /* SBCD immediate */
                D = sbc16(D, imm_word(), 1, 0);
                break;

            case 0x1092:        /* SBCD direct */
                direct();
                D = sbc16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x10b2:        /* SBCD extended */
                extended();
                D = sbc16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x10a2:        /* SBCD indexed */
                indexed();
                D = sbc16(D, RDMEM16(ea), 2, 1);
                break;

            case 0x1033:        /* SBCR post */
                post_byte = imm_byte();
                set_reg((BYTE)(post_byte & 0x0f), sbc16(get_reg((BYTE)(post_byte >> 4)), get_reg((BYTE)(post_byte & 0x0f)), 1, 1));
                break;
#endif

#ifdef FULL6809
            case 0x0087:        /* SCC immediate (UNDOC) */
            case 0x00c7:        /* SCC immediate (UNDOC) */
            case 0x1087:        /* SCC immediate (UNDOC) */
            case 0x10c7:        /* SCC immediate (UNDOC) */
            case 0x1187:        /* SCC immediate (UNDOC) */
            case 0x11c7:        /* SCC immediate (UNDOC) */
                scc(imm_byte());
                break;
#endif

            case 0x001d:        /* SEX */
#ifdef FULL6809
            case 0x101d:        /* SEX (UNDOC) */
            case 0x111d:        /* SEX (UNDOC) */
#endif
                sex();
                break;
#ifdef H6309
            case 0x0014:        /* SEXW */
                sexw();
                break;
#endif

            case 0x0097:        /* STA direct */
#ifdef FULL6809
            case 0x1097:        /* STA direct (UNDOC) */
            case 0x1197:        /* STA direct (UNDOC) */
#endif
                direct();
                st(A, 1, 0);
                break;

            case 0x00b7:        /* STA extended */
#ifdef FULL6809
            case 0x10b7:        /* STA extended (UNDOC) */
            case 0x11b7:        /* STA extended (UNDOC) */
#endif
                extended();
                st(A, 1, 0);
                break;

            case 0x00a7:        /* STA indexed */
#ifdef FULL6809
            case 0x10a7:        /* STA indexed (UNDOC) */
            case 0x11a7:        /* STA indexed (UNDOC) */
#endif
                indexed();
                st(A, 1, 1);
                break;

            case 0x00d7:        /* STB direct */
#ifdef FULL6809
            case 0x10d7:        /* STB direct (UNDOC) */
            case 0x11d7:        /* STB direct (UNDOC) */
#endif
                direct();
                st(B, 1, 0);
                break;

            case 0x00f7:        /* STB extended */
#ifdef FULL6809
            case 0x10f7:        /* STB extended (UNDOC) */
            case 0x11f7:        /* STB extended (UNDOC) */
#endif
                extended();
                st(B, 1, 0);
                break;

            case 0x00e7:        /* STB indexed */
#ifdef FULL6809
            case 0x10e7:        /* STB indexed (UNDOC) */
            case 0x11e7:        /* STB indexed (UNDOC) */
#endif
                indexed();
                st(B, 1, 1);
                break;

#ifdef H6309
            case 0x1137:        /* STBT post, direct */
                post_byte = imm_byte();
                direct();
                WRMEM(ea, stbt(post_byte, RDMEM(ea)));
                break;
#endif

            case 0x00dd:        /* STD direct */
#ifdef FULL6809
            case 0x10dd:        /* STD direct (UNDOC) */
            case 0x11dd:        /* STD direct (UNDOC) */
#endif
                direct();
                st16(D, 1, 0);
                break;

            case 0x00fd:        /* STD extended */
#ifdef FULL6809
            case 0x10fd:        /* STD extended (UNDOC) */
            case 0x11fd:        /* STD extended (UNDOC) */
#endif
                extended();
                st16(D, 1, 0);
                break;

            case 0x00ed:        /* STD indexed */
#ifdef FULL6809
            case 0x10ed:        /* STD indexed (UNDOC) */
            case 0x11ed:        /* STD indexed (UNDOC) */
#endif
                indexed();
                st16(D, 1, 1);
                break;

#ifdef H6309
            case 0x1197:        /* STE direct */
                direct();
                st(E, 1, 0);
                break;

            case 0x11b7:        /* STE extended */
                extended();
                st(E, 1, 0);
                break;

            case 0x11a7:        /* STE indexed */
                indexed();
                st(E, 1, 1);
                break;

            case 0x11d7:        /* STF direct */
                direct();
                st(F, 1, 0);
                break;

            case 0x11f7:        /* STF extended */
                extended();
                st(F, 1, 0);
                break;

            case 0x11e7:        /* STF indexed (UNDOC) */
                indexed();
                st(F, 1, 1);
                break;

            case 0x10dd:        /* STQ direct */
                direct();
                st32(Q, 1, 0);
                break;

            case 0x10fd:        /* STQ extended */
                extended();
                st32(Q, 1, 0);
                break;

            case 0x10ed:        /* STQ indexed */
                indexed();
                st32(Q, 1, 1);
                break;
#endif

            case 0x10df:        /* STS direct */
                direct();
                st16(S, 1, 0);
                break;

            case 0x10ff:        /* STS extended */
                extended();
                st16(S, 1, 0);
                break;

            case 0x10ef:        /* STS indexed */
                indexed();
                st16(S, 1, 1);
                break;

#ifdef FULL6809
            case 0x00cf:        /* STU immediate (UNDOC) */
            case 0x10cf:        /* STU immediate (UNDOC) */
            case 0x11cf:        /* STU immediate (UNDOC) */
                st_imm(U);
                break;
#endif

            case 0x00df:        /* STU direct */
#ifdef FULL6809
            case 0x11df:        /* STU direct (UNDOC) */
#endif
                direct();
                st16(U, 1, 0);
                break;

            case 0x00ff:        /* STU extended */
#ifdef FULL6809
            case 0x11ff:        /* STU extended (UNDOC) */
#endif
                extended();
                st16(U, 1, 0);
                break;

            case 0x00ef:        /* STU indexed */
#ifdef FULL6809
            case 0x11ef:        /* STU indexed (UNDOC) */
#endif
                indexed();
                st16(U, 1, 1);
                break;

#ifdef H6309
            case 0x1097:        /* STW direct */
                direct();
                st16(W, 1, 0);
                break;

            case 0x10b7:        /* STW extended */
                extended();
                st16(W, 1, 0);
                break;

            case 0x10a7:        /* STW indexed */
                indexed();
                st16(W, 1, 1);
                break;
#endif

#ifdef FULL6809
            case 0x008f:        /* STX immediate (UNDOC) */
            case 0x108f:        /* STX immediate (UNDOC) */
            case 0x118f:        /* STX immediate (UNDOC) */
                st_imm(X);
                break;
#endif

            case 0x009f:        /* STX direct */
#ifdef FULL6809
            case 0x119f:        /* STX direct (UNDOC) */
#endif
                direct();
                st16(X, 1, 0);
                break;

            case 0x00bf:        /* STX extended */
#ifdef FULL6809
            case 0x11bf:        /* STX extended (UNDOC) */
#endif
                extended();
                st16(X, 1, 0);
                break;

            case 0x00af:        /* STX indexed */
#ifdef FULL6809
            case 0x11af:        /* STX indexed (UNDOC) */
#endif
                indexed();
                st16(X, 1, 1);
                break;

            case 0x109f:        /* STY direct */
                direct();
                st16(Y, 1, 0);
                break;

            case 0x10bf:        /* STY extended */
                extended();
                st16(Y, 1, 0);
                break;

            case 0x10af:        /* STY indexed */
                indexed();
                st16(Y, 1, 1);
                break;

            case 0x0080:        /* SUBA immediate */
#ifdef FULL6809
            case 0x1080:        /* SUBA immediate (UNDOC) */
            case 0x1180:        /* SUBA immediate (UNDOC) */
#endif
                A = sub(A, imm_byte(), 0, 0);
                break;

            case 0x0090:        /* SUBA direct */
#ifdef FULL6809
            case 0x1090:        /* SUBA direct (UNDOC) */
            case 0x1190:        /* SUBA direct (UNDOC) */
#endif
                direct();
                A = sub(A, RDMEM(ea), 1, 0);
                break;

            case 0x00b0:        /* SUBA extended */
#ifdef FULL6809
            case 0x10b0:        /* SUBA extended (UNDOC) */
            case 0x11b0:        /* SUBA extended (UNDOC) */
#endif
                extended();
                A = sub(A, RDMEM(ea), 1, 0);
                break;

            case 0x00a0:        /* SUBA indexed */
#ifdef FULL6809
            case 0x10a0:        /* SUBA indexed (UNDOC) */
            case 0x11a0:        /* SUBA indexed (UNDOC) */
#endif
                indexed();
                A = sub(A, RDMEM(ea), 1, 1);
                break;

            case 0x00c0:        /* SUBB immediate */
#ifdef FULL6809
            case 0x10c0:        /* SUBB immediate (UNDOC) */
            case 0x11c0:        /* SUBB immediate (UNDOC) */
#endif
                B = sub(B, imm_byte(), 0, 0);
                break;

            case 0x00d0:        /* SUBB direct */
#ifdef FULL6809
            case 0x10d0:        /* SUBB direct (UNDOC) */
            case 0x11d0:        /* SUBB direct (UNDOC) */
#endif
                direct();
                B = sub(B, RDMEM(ea), 1, 0);
                break;

            case 0x00f0:        /* SUBB extended */
#ifdef FULL6809
            case 0x10f0:        /* SUBB extended (UNDOC) */
            case 0x11f0:        /* SUBB extended (UNDOC) */
#endif
                extended();
                B = sub(B, RDMEM(ea), 1, 0);
                break;

            case 0x00e0:        /* SUBB indexed */
#ifdef FULL6809
            case 0x10e0:        /* SUBB indexed (UNDOC) */
            case 0x11e0:        /* SUBB indexed (UNDOC) */
#endif
                indexed();
                B = sub(B, RDMEM(ea), 1, 1);
                break;

            case 0x0083:        /* SUBD immediate */
                D = sub16(D, imm_word(), 1, 0);
                break;

            case 0x0093:        /* SUBD direct */
                direct();
                D = sub16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x00b3:        /* SUBD extended */
                extended();
                D = sub16(D, RDMEM16(ea), 2, 0);
                break;

            case 0x00a3:        /* SUBD indexed */
                indexed();
                D = sub16(D, RDMEM16(ea), 2, 1);
                break;

#ifdef H6309
            case 0x1180:        /* SUBE immediate */
                E = sub(E, imm_byte(), 0, 0);
                break;

            case 0x1190:        /* SUBE direct */
                direct();
                E = sub(E, RDMEM(ea), 1, 0);
                break;

            case 0x11b0:        /* SUBE extended */
                extended();
                E = sub(E, RDMEM(ea), 1, 0);
                break;

            case 0x11a0:        /* SUBE indexed */
                indexed();
                E = sub(E, RDMEM(ea), 1, 1);
                break;

            case 0x11c0:        /* SUBF immediate */
                F = sub(F, imm_byte(), 0, 0);
                break;

            case 0x11d0:        /* SUBF direct */
                direct();
                F = sub(F, RDMEM(ea), 1, 0);
                break;

            case 0x11f0:        /* SUBF extended */
                extended();
                F = sub(F, RDMEM(ea), 1, 0);
                break;

            case 0x11e0:        /* SUBF indexed */
                indexed();
                F = sub(F, RDMEM(ea), 1, 1);
                break;

            case 0x1032:        /* SUBR post */
                post_byte = imm_byte();
                set_reg((BYTE)(post_byte & 0x0f), sub16(get_reg((BYTE)(post_byte >> 4)), get_reg((BYTE)(post_byte & 0x0f)), 1, 1));
                break;

            case 0x1080:        /* SUBW immediate */
                W = sub16(W, imm_word(), 1, 0);
                break;

            case 0x1090:        /* SUBW direct */
                direct();
                W = sub16(W, RDMEM16(ea), 2, 0);
                break;

            case 0x10b0:        /* SUBW extended */
                extended();
                W = sub16(W, RDMEM16(ea), 2, 0);
                break;

            case 0x10a0:        /* SUBW indexed */
                indexed();
                W = sub16(W, RDMEM16(ea), 2, 1);
                break;
#endif

            case 0x003f:        /* SWI */
                swi();
                break;

            case 0x103f:        /* SWI2 */
                swi2();
                break;

            case 0x113f:        /* SWI3 */
                swi3();
                break;

#ifdef FULL6809
            case 0x003e:        /* SWIRES (UNDOC) */
            case 0x103e:        /* SWIRES (UNDOC) */
            case 0x113e:        /* SWIRES (UNDOC) */
                swires();
                break;
#endif

            case 0x0013:        /* SYNC */
#ifdef FULL6809
            case 0x1013:        /* SYNC (UNDOC) */
            case 0x1113:        /* SYNC (UNDOC) */
#endif
                sync();
                break;

#ifdef H6309
            case 0x1138:        /* TFM R+,R+ */
                tfmpp(imm_byte());
                break;

            case 0x1139:        /* TFM R-,R- */
                tfmmm(imm_byte());
                break;

            case 0x113a:        /* TFM R+,R */
                tfmpc(imm_byte());
                break;

            case 0x113b:        /* TFM R,R+ */
                tfmcp(imm_byte());
                break;
#endif

            case 0x001f:        /* TFR post */
#ifdef FULL6809
            case 0x101f:        /* TFR post (UNDOC) */
            case 0x111f:        /* TFR post (UNDOC) */
#endif
                tfr();
                break;

#ifdef H6309
            case 0x000b:        /* TIM post, direct */
                post_byte = imm_byte();
                direct();
                WRMEM(ea, tim(post_byte, 2));
                break;

            case 0x007b:        /* TIM extended */
                post_byte = imm_byte();
                extended();
                WRMEM(ea, tim(post_byte, 1));
                break;

            case 0x006b:        /* TIM indexed */
                post_byte = imm_byte();
                indexed();
                WRMEM(ea, tim(post_byte, 3));
                break;
#endif

            case 0x004d:        /* TSTA */
#ifdef FULL6809
            case 0x104d:        /* TSTA (UNDOC) */
            case 0x114d:        /* TSTA (UNDOC) */
#endif
                tst(A, 1, 0);
                break;

            case 0x005d:        /* TSTB */
#ifdef FULL6809
            case 0x105d:        /* TSTB (UNDOC) */
            case 0x115d:        /* TSTB (UNDOC) */
#endif
                tst(B, 1, 0);
                break;

#ifdef H6309
            case 0x104d:        /* TSTD */
                tst16(D);
                break;

            case 0x114d:        /* TSTE */
                tst(E, 1, 0);
                break;

            case 0x115d:        /* TSTF */
                tst(F, 1, 0);
                break;

            case 0x105d:        /* TSTW */
                tst16(W);
                break;
#endif

            case 0x000d:        /* TST direct */
#ifdef FULL6809
            case 0x100d:        /* TST direct (UNDOC) */
            case 0x110d:        /* TST direct (UNDOC) */
#endif
                direct();
                tst(RDMEM(ea), 3, 1);
                break;

            case 0x007d:        /* TST extended */
#ifdef FULL6809
            case 0x107d:        /* TST extended (UNDOC) */
            case 0x117d:        /* TST extended (UNDOC) */
#endif
                extended();
                tst(RDMEM(ea), 3, 1);
                break;

            case 0x006d:        /* TST indexed */
#ifdef FULL6809
            case 0x106d:        /* TST indexed (UNDOC) */
            case 0x116d:        /* TST indexed (UNDOC) */
#endif
                indexed();
                tst(RDMEM(ea), 3, 2);
                break;

#ifdef H6309
            default:    /* 6309 illegal opcode trap */
                opcode_trap();
                break;
#else
            default:
                sim_error("invalid opcode %04X\n", opcode);
                break;
#endif
        }

        if (cc_changed) {
            cc_modified();
        }
    } while (1);

/* cpu_exit: */
    return;
}

void cpu6809_reset (void)
{
    X = Y = S = U = DP = 0;
    H = N = OV = C = 0;
    A = B = 0;
    Z = 1;
    EFI = F_FLAG | I_FLAG;
#ifdef H6309
    MD = E = F = 0;
#endif

    PC = read16(0xfffe);
}

/* ------------------------------------------------------------------------- */

static char snap_module_name[] = "CPU6809";
#define SNAP_MAJOR 1
#define SNAP_MINOR 0

int cpu6809_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;
    BYTE md, e, f;
    WORD v;

    m = snapshot_module_create(s, snap_module_name, ((BYTE)SNAP_MAJOR),
                               ((BYTE)SNAP_MINOR));
    if (m == NULL) {
        return -1;
    }

    EXPORT_REGISTERS();

    if (0
        || SMW_DW(m, maincpu_clk) < 0
        || SMW_W(m, GLOBAL_REGS.reg_x) < 0
        || SMW_W(m, GLOBAL_REGS.reg_y) < 0
        || SMW_W(m, GLOBAL_REGS.reg_u) < 0
        || SMW_W(m, GLOBAL_REGS.reg_s) < 0
        || SMW_W(m, GLOBAL_REGS.reg_pc) < 0
        || SMW_B(m, GLOBAL_REGS.reg_dp) < 0
        || SMW_B(m, GLOBAL_REGS.reg_cc) < 0
        || SMW_B(m, GLOBAL_REGS.reg_a) < 0
        || SMW_B(m, GLOBAL_REGS.reg_b) < 0) {
        goto fail;
    }

#ifdef H6309
    v = get_v();
    e = get_e();
    f = get_f();
    md = MD;
#else
    v = e = f = md = 0;
#endif

    /* Extra 6309 registers, already filled in for the future */
    if (0
        || SMW_W(m, v) < 0
        || SMW_B(m, e) < 0
        || SMW_B(m, f) < 0
        || SMW_B(m, md) < 0) {
        goto fail;
    }

#if 0
    /*
     * Don't need to do this, since it was loaded by the 6502.
     */
    if (interrupt_write_snapshot(maincpu_int_status, m) < 0) {
        goto fail;
    }

    if (interrupt_write_new_snapshot(maincpu_int_status, m) < 0) {
        goto fail;
    }
#endif

    return snapshot_module_close(m);

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}

int cpu6809_snapshot_read_module(snapshot_t *s)
{
    BYTE major, minor;
    snapshot_module_t *m;
    DWORD my_maincpu_clk;
    WORD v;
    BYTE e, f, md;

    m = snapshot_module_open(s, snap_module_name, &major, &minor);
    if (m == NULL) {
        /* This module is optional */
        cpu6809_reset();
        return 0;
    }

    if (major != SNAP_MAJOR) {
        goto fail;
    }

    if (0
        || SMR_DW(m, &my_maincpu_clk) < 0
        || SMR_W(m, &GLOBAL_REGS.reg_x) < 0
        || SMR_W(m, &GLOBAL_REGS.reg_y) < 0
        || SMR_W(m, &GLOBAL_REGS.reg_u) < 0
        || SMR_W(m, &GLOBAL_REGS.reg_s) < 0
        || SMR_W(m, &GLOBAL_REGS.reg_pc) < 0
        || SMR_B(m, &GLOBAL_REGS.reg_dp) < 0
        || SMR_B(m, &GLOBAL_REGS.reg_cc) < 0
        || SMR_B(m, &GLOBAL_REGS.reg_a) < 0
        || SMR_B(m, &GLOBAL_REGS.reg_b) < 0) {
        goto fail;
    }

    IMPORT_REGISTERS();

    /* Extra 6309 registers, already filled in for the future */
    if (0
        || SMR_W(m, &v) < 0
        || SMR_B(m, &e) < 0
        || SMR_B(m, &f) < 0
        || SMR_B(m, &md) < 0) {
        goto fail;
    }

#ifdef H6309
    set_v(v);
    set_e(e);
    set_f(f);
    MD = md;
#endif

    maincpu_clk = my_maincpu_clk;

#if 0
    /*
     * Don't need to do this, since it was loaded by the 6502.
     */
    if (interrupt_read_snapshot(maincpu_int_status, m) < 0) {
        goto fail;
    }

    if (interrupt_read_new_snapshot(maincpu_int_status, m) < 0) {
        goto fail;
    }
#endif

    return snapshot_module_close(m);

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}
