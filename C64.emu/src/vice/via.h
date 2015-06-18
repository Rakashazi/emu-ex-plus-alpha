/*
 * via.h - VIA emulation.
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Andreas Boose <viceteam@t-online.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifndef VICE_VIA_H
#define VICE_VIA_H

#include "types.h"

/* MOS 6522 registers */
#define VIA_PRB         0  /* Port B */
#define VIA_PRA         1  /* Port A */
#define VIA_DDRB        2  /* Data direction register for port B */
#define VIA_DDRA        3  /* Data direction register for port A */

#define VIA_T1CL        4  /* Timer 1 count low */
#define VIA_T1CH        5  /* Timer 1 count high */
#define VIA_T1LL        6  /* Timer 1 latch low */
#define VIA_T1LH        7  /* Timer 1 latch high */
#define VIA_T2CL        8  /* Timer 2 count low - read only */
#define VIA_T2LL        8  /* Timer 2 latch low - write only */
#define VIA_T2CH        9  /* Timer 2 count high - read only */
#define VIA_T2LH        9  /* Timer 2 latch high - write only */

#define VIA_SR          10 /* Serial port shift register */
#define VIA_ACR         11 /* Auxiliary control register */
#define VIA_PCR         12 /* Peripheral control register */

#define VIA_IFR         13 /* Interrupt flag register */
#define VIA_IER         14 /* Interrupt control register */
#define VIA_PRA_NHS     15 /* Port A with no handshake */

/* Interrupt Masks  */
/* MOS 6522 */
#define VIA_IM_IRQ      128     /* Control Bit */
#define VIA_IM_T1       64      /* Timer 1 underflow */
#define VIA_IM_T2       32      /* Timer 2 underflow */
#define VIA_IM_CB1      16      /* Handshake */
#define VIA_IM_CB2      8       /* Handshake */
#define VIA_IM_SR       4       /* Shift Register completion */
#define VIA_IM_CA1      2       /* Handshake */
#define VIA_IM_CA2      1       /* Handshake */


/* Signal values (for signaling edges on the control lines)  */
#define VIA_SIG_CA1     0
#define VIA_SIG_CA2     1
#define VIA_SIG_CB1     2
#define VIA_SIG_CB2     3

#define VIA_SIG_FALL    0
#define VIA_SIG_RISE    1


struct alarm_context_s;
struct clk_guard_s;
struct interrupt_cpu_status_s;
struct snapshot_s;
struct via_context_s;

typedef struct via_context_s {
    BYTE via[16];
    int ifr;
    int ier;
    unsigned int tal;
    BYTE t2cl; /* IF: T2 counter low */
    BYTE t2ch; /* IF: T2 counter high */
    CLOCK tau;
    CLOCK tbu;
    CLOCK tai;
    CLOCK tbi;
    int pb7;
    int pb7x;
    int pb7o;
    int pb7xx;
    int pb7sx;
    BYTE oldpa;
    BYTE oldpb;
    BYTE ila;
    BYTE ilb;
    int ca2_state;
    int cb2_state;
    BYTE shift_state;          /* IF: state helper for shift register */
    struct alarm_s *t1_alarm;
    struct alarm_s *t2_alarm;
    signed int log;            /* init to LOG_ERR */

    CLOCK read_clk;            /* init to 0 */
    int read_offset;           /* init to 0 */
    BYTE last_read;            /* init to 0 */

    int irq_line;              /* IK_... */
    unsigned int int_num;

    char *myname;              /* init to "DriveXViaY" */
    char *my_module_name;      /* init to "VIAXDY" */
    char *my_module_name_alt1; /* Legacy names. */
    char *my_module_name_alt2;

    CLOCK *clk_ptr;
    int *rmw_flag;
    int write_offset;          /* 1 if CPU core does CLK++ before store */

    int enabled;

    void *prv;
    void *context;

    void (*undump_pra)(struct via_context_s *, BYTE);
    void (*undump_prb)(struct via_context_s *, BYTE);
    void (*undump_pcr)(struct via_context_s *, BYTE);
    void (*undump_acr)(struct via_context_s *, BYTE);
    void (*store_pra)(struct via_context_s *, BYTE, BYTE, WORD);
    void (*store_prb)(struct via_context_s *, BYTE, BYTE, WORD);
    BYTE (*store_pcr)(struct via_context_s *, BYTE, WORD);
    void (*store_acr)(struct via_context_s *, BYTE);
    void (*store_sr)(struct via_context_s *, BYTE);
    void (*store_t2l)(struct via_context_s *, BYTE);
    BYTE (*read_pra)(struct via_context_s *, WORD);
    BYTE (*read_prb)(struct via_context_s *);
    void (*set_int)(struct via_context_s *, unsigned int, int, CLOCK);
    void (*restore_int)(struct via_context_s *, unsigned int, int);
    void (*set_ca2)(struct via_context_s *, int state);
    void (*set_cb2)(struct via_context_s *, int state);
    void (*reset)(struct via_context_s *);
} via_context_t;


extern void viacore_setup_context(struct via_context_s *via_context);
extern void viacore_init(struct via_context_s *via_context,
                         struct alarm_context_s *alarm_context,
                         struct interrupt_cpu_status_s *int_status,
                         struct clk_guard_s *clk_guard);
extern void viacore_shutdown(struct via_context_s *via_context);
extern void viacore_reset(struct via_context_s *via_context);
extern void viacore_disable(struct via_context_s *via_context);
extern void viacore_signal(struct via_context_s *via_context, int line,
                           int edge);

extern void viacore_store(struct via_context_s *via_context,
                          WORD addr, BYTE data);
extern BYTE viacore_read(struct via_context_s *via_context,
                         WORD addr);
extern BYTE viacore_peek(struct via_context_s *via_context,
                         WORD addr);

extern void viacore_set_sr(via_context_t *via_context, BYTE data);

extern int viacore_snapshot_write_module(struct via_context_s *via_context,
                                         struct snapshot_s *s);
extern int viacore_snapshot_read_module(struct via_context_s *via_context,
                                        struct snapshot_s *s);
extern int viacore_dump(via_context_t *via_context);
#endif

