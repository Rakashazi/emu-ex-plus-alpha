/*
 * tpi.h - Chip register definitions.
 *
 * Written by
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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

#ifndef VICE_TPI_H
#define VICE_TPI_H

#include "types.h"

#define TPI_PA          0
#define TPI_PB          1
#define TPI_PC          2
#define TPI_ILR         2
#define TPI_DDPA        3
#define TPI_DDPB        4
#define TPI_DDPC        5
#define TPI_IMR         5
#define TPI_CREG        6
#define TPI_AIR         7

struct snapshot_s;

typedef struct tpi_context_s {
    uint8_t c_tpi[8];

    uint8_t irq_previous;
    uint8_t irq_stack;

    uint8_t tpi_last_read;
    unsigned int tpi_int_num;

    uint8_t oldpa;
    uint8_t oldpb;
    uint8_t oldpc;

    uint8_t ca_state;
    uint8_t cb_state;

    signed int log;

    char *myname;

    int irq_line;                 /* IK_... */
    unsigned int int_num;

    CLOCK *clk_ptr;
    int *rmw_flag;

    void *prv;
    void *context;

    void (*store_pa)(struct tpi_context_s *, uint8_t);
    void (*store_pb)(struct tpi_context_s *, uint8_t);
    void (*store_pc)(struct tpi_context_s *, uint8_t);
    uint8_t (*read_pa)(struct tpi_context_s *);
    uint8_t (*read_pb)(struct tpi_context_s *);
    uint8_t (*read_pc)(struct tpi_context_s *);
    void (*undump_pa)(struct tpi_context_s *, uint8_t);
    void (*undump_pb)(struct tpi_context_s *, uint8_t);
    void (*undump_pc)(struct tpi_context_s *, uint8_t);
    void (*reset)(struct tpi_context_s *);
    void (*set_ca)(struct tpi_context_s *, int);
    void (*set_cb)(struct tpi_context_s *, int);
    void (*set_int)(unsigned int, int);
    void (*restore_int)(unsigned int, int);
} tpi_context_t;

extern void tpicore_setup_context(struct tpi_context_s *tpi_context);
extern void tpicore_shutdown(struct tpi_context_s *tpi_context);
extern void tpicore_reset(tpi_context_t *tpi_context);
extern void tpicore_store(struct tpi_context_s *tpi_context, uint16_t addr, uint8_t byte);
extern uint8_t tpicore_read(struct tpi_context_s *tpi_context, uint16_t addr);
extern uint8_t tpicore_peek(struct tpi_context_s *tpi_context, uint16_t addr);
extern void tpicore_set_int(tpi_context_t *tpi_context, int bit, int state);
extern void tpicore_restore_int(tpi_context_t *tpi_context, int bit, int state);

extern int tpicore_snapshot_write_module(tpi_context_t *tpi_context, struct snapshot_s *p);
extern int tpicore_snapshot_read_module(tpi_context_t *tpi_context, struct snapshot_s *p);
extern int tpicore_dump(struct tpi_context_s *tpi_context);

#endif
