/*
 * via.h - VIA emulation.
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Andreas Boose <viceteam@t-online.de>
 *  Olaf Seibert <rhialto@falu.nl>
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

#define VIA_ACR_T1_CONTROL      0xC0
# define VIA_ACR_T1_PB7_UNUSED   0x00
# define VIA_ACR_T1_PB7_USED     0x80
# define VIA_ACR_T1_ONE_SHOT     0x00
# define VIA_ACR_T1_FREE_RUN     0x40

#define VIA_ACR_T2_CONTROL      0x20
# define VIA_ACR_T2_TIMER        0x00
# define VIA_ACR_T2_COUNTPB6     0x20

#define VIA_ACR_SR_CONTROL      0x1C

# define VIA_ACR_SR_IN           0x00
# define VIA_ACR_SR_OUT          0x10

# define VIA_ACR_SR_DISABLED     0x00   /* mode 0 Shift register disabled */
# define VIA_ACR_SR_IN_T2        0x04   /* mode 1 Shift in under control of Timer 2*/
# define VIA_ACR_SR_IN_PHI2      0x08   /* mode 2 Shift at System Clock Rate */
# define VIA_ACR_SR_IN_CB1       0x0C   /* mode 3 Shift under Control of External Clock */
# define VIA_ACR_SR_OUT_FREE_T2  0x10   /* mode 4 Free-running output */
# define VIA_ACR_SR_OUT_T2       0x14   /* mode 5 Shift out under control of T2 */
# define VIA_ACR_SR_OUT_PHI2     0x18   /* mode 6 Shift out at System Clock Rate */
# define VIA_ACR_SR_OUT_CB1      0x1C   /* mode 7 Shift out under control of an External Pulse */

#define VIA_ACR_PB_LATCH         0x02
#define VIA_ACR_PA_LATCH         0x01

#define VIA_PCR_CB2_CONTROL      0xE0                        /* 3 bits */

#define VIA_PCR_CB2_I_OR_O                       0x80        /* bit */
#define VIA_PCR_CB2_INPUT                        0x00        /* bit */
#define VIA_PCR_CB2_INPUT_NEG_ACTIVE_EDGE        0x00        /* bit */
#define VIA_PCR_CB2_INPUT_POS_ACTIVE_EDGE        0x40        /* bit */
#define VIA_PCR_CB2_INDEPENDENT_INTERRUPT        0x20        /* bit */
/* If INDEPENDENT_INTERRUPT is set, "reading or writing ORB does not clear the CB2 interrupt flag" */

#define VIA_PCR_CB2_HANDSHAKE_OUTPUT             0x80        /* 3 bits */
#define VIA_PCR_CB2_PULSE_OUTPUT                 0xA0        /* 3 bits */
#define VIA_PCR_CB2_LOW_OUTPUT                   0xC0        /* 3 bits */
#define VIA_PCR_CB2_HIGH_OUTPUT                  0xE0        /* 3 bits */

#define VIA_PCR_CB1_CONTROL      0x10        /* 1 bit */

#define VIA_PCR_CB1_NEG_ACTIVE_EDGE              0x00        /* bit */
#define VIA_PCR_CB1_POS_ACTIVE_EDGE              0x10        /* bit */

#define VIA_PCR_CA2_CONTROL      0x0E        /* 3 bits */
#define VIA_PCR_CA2_I_OR_O                       0x08        /* bit */
#define VIA_PCR_CA2_INPUT                        0x00        /* bit */
#define VIA_PCR_CA2_INPUT_NEG_ACTIVE_EDGE        0x00        /* bit */
#define VIA_PCR_CA2_INPUT_POS_ACTIVE_EDGE        0x04        /* bit */
#define VIA_PCR_CA2_INDEPENDENT_INTERRUPT        0x02        /* bit */
/* If INDEPENDENT_INTERRUPT is set, "reading or writing ORA does not clear the CA2 interrupt flag" */

#define VIA_PCR_CA2_HANDSHAKE_OUTPUT             0x08        /* 3 bits */
#define VIA_PCR_CA2_PULSE_OUTPUT                 0x0A        /* 3 bits */
#define VIA_PCR_CA2_LOW_OUTPUT                   0x0C        /* 3 bits */
#define VIA_PCR_CA2_HIGH_OUTPUT                  0x0E        /* 3 bits */

#define VIA_PCR_CA1_CONTROL      0x01        /* 1 bit */

#define VIA_PCR_CA1_NEG_ACTIVE_EDGE              0x00        /* bit */
#define VIA_PCR_CA1_POS_ACTIVE_EDGE              0x01        /* bit */


/* Signal values (for signaling edges on the control lines)  */
#define VIA_SIG_CA1     0
#define VIA_SIG_CA2     1
#define VIA_SIG_CB1     2
#define VIA_SIG_CB2     3

#define VIA_SIG_FALL    0
#define VIA_SIG_RISE    1


struct alarm_context_s;
struct interrupt_cpu_status_s;
struct snapshot_s;
struct via_context_s;

typedef struct via_context_s {
    uint8_t via[16];
    int ifr;
    int ier;
    unsigned int tal;/* T1 latch */
    uint8_t t2cl;    /* T2 counter low */
    uint8_t t2ch;    /* T2 counter high */
    CLOCK t1reload;  /* T1 reload-from-latch time (it reads LLLL again) */
    CLOCK t2zero;    /* When T2 reaches/last read 0000 or at least yy00 */
    CLOCK t1zero;    /* T1: when alarm viacore_t1_zero_alarm() goes off, sets VIA_IM_T1, after 0000 */
    bool t2xx00;     /* T2: set if T2 should give an IRQ at the first 0000, or if it is in 8-bit mode */
    uint8_t t1_pb7;  /* 0x00 or 0x80 */
    uint8_t oldpa;
    uint8_t oldpb;
    uint8_t ila;
    uint8_t ilb;
    bool ca2_out_state;
    bool cb1_in_state;
    bool cb1_out_state;
    bool cb2_in_state;
    bool cb2_out_state;
    bool cb1_is_input;
    bool cb2_is_input;
    uint8_t shift_state;          /* state helper for shift register */
#define START_SHIFTING          0
#define FINISHED_SHIFTING       16
    struct alarm_s *t1_zero_alarm;
    struct alarm_s *t2_zero_alarm;          /* after T2 has reached xx00 */
    struct alarm_s *t2_underflow_alarm;     /* after T2 has reached xxFF */
    struct alarm_s *t2_shift_alarm;         /* 1 clock later than t2_underflow_alarm */
    struct alarm_s *phi2_sr_alarm;
    signed int log;             /* init to LOG_ERR */

    CLOCK read_clk;             /* init to 0 */
    int read_offset;            /* init to 0 */
    uint8_t last_read;          /* init to 0 */
    bool t2_irq_allowed;        /* each write to T2H allows one IRQ */

    int irq_line;              /* IK_... */
    unsigned int int_num;

    char *myname;              /* init to "DriveXViaY" */
    char *my_module_name;      /* init to "VIAXDY" */
    char *my_module_name_alt1; /* Legacy names. */
    char *my_module_name_alt2;

    CLOCK *clk_ptr;
    int *rmw_flag;
    int write_offset;          /* 1 if CPU core does CLK++ before store */

    bool enabled;

    void *prv;                /* typically drivevia1_context_t */
    void *context;            /* typically diskunit_context_t */
    struct alarm_context_s *alarm_context;


    void (*undump_pra)(struct via_context_s *, uint8_t);
    void (*undump_prb)(struct via_context_s *, uint8_t);
    void (*undump_pcr)(struct via_context_s *, uint8_t);
    void (*undump_acr)(struct via_context_s *, uint8_t);
    void (*store_pra)(struct via_context_s *, uint8_t, uint8_t, uint16_t);
    void (*store_prb)(struct via_context_s *, uint8_t, uint8_t, uint16_t);
    uint8_t (*store_pcr)(struct via_context_s *, uint8_t, uint16_t);
    void (*store_acr)(struct via_context_s *, uint8_t);
    void (*store_sr)(struct via_context_s *, uint8_t);
    void (*sr_underflow)(struct via_context_s *);
    void (*store_t2l)(struct via_context_s *, uint8_t);
    uint8_t (*read_pra)(struct via_context_s *, uint16_t);
    uint8_t (*read_prb)(struct via_context_s *);
    void (*set_int)(struct via_context_s *, unsigned int, int, CLOCK);
    void (*restore_int)(struct via_context_s *, unsigned int, int);
    void (*set_ca2)(struct via_context_s *, int state);
    void (*set_cb1)(struct via_context_s *, int state);
    void (*set_cb2)(struct via_context_s *, int state, int offset);
    void (*reset)(struct via_context_s *);
} via_context_t;


void viacore_setup_context(struct via_context_s *via_context);
void viacore_init(struct via_context_s *via_context,
                  struct alarm_context_s *alarm_context,
                  struct interrupt_cpu_status_s *int_status);
void viacore_shutdown(struct via_context_s *via_context);
void viacore_reset(struct via_context_s *via_context);
void viacore_disable(struct via_context_s *via_context);
void viacore_signal(struct via_context_s *via_context, int line, int edge);

void viacore_store(struct via_context_s *via_context,
                   uint16_t addr, uint8_t data);
uint8_t viacore_read(struct via_context_s *via_context, uint16_t addr);
uint8_t viacore_peek(struct via_context_s *via_context, uint16_t addr);

/* WARNING: this is a hack */
void viacore_set_sr(via_context_t *via_context, uint8_t data);

void viacore_set_cb1(struct via_context_s *via_context, bool data);
void viacore_set_cb2(struct via_context_s *via_context, bool data);

int viacore_snapshot_write_module(struct via_context_s *via_context, struct snapshot_s *s);
int viacore_snapshot_read_module(struct via_context_s *via_context, struct snapshot_s *s);
int viacore_dump(via_context_t *via_context);

#endif
