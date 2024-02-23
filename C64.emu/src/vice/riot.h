/*
 * riot.h - RIOT emulation.
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

#ifndef VICE_RIOT_H
#define VICE_RIOT_H

#include "types.h"

/*
 * signal values (for signaling edges on the control lines)
 */

#define RIOT_SIG_PA7    0

#define RIOT_SIG_FALL   0
#define RIOT_SIG_RISE   1

typedef struct riot_context_s {
    uint8_t riot_io[4];
    uint8_t old_pa;
    uint8_t old_pb;

    signed int log;       /* init to LOG_ERR */

    struct alarm_s *alarm;

    CLOCK read_clk;       /* init to 0 */
    int read_offset;      /* init to 0 */
    uint8_t last_read;       /* init to 0 */
    uint8_t r_edgectrl;      /* init to 0 */
    uint8_t r_irqfl;         /* init to 0 */
    uint8_t r_irqline;       /* init to 0 */

    CLOCK r_write_clk;
    int r_N;
    int r_divider;
    int r_irqen;

    char *myname;

    CLOCK *clk_ptr;
    int *rmw_flag;

    int enabled;

    void *prv;          /* Typically a driveriot2_context_t */
    void *context;      /* Typically a diskunit_context_t */

    void (*undump_pra)(struct riot_context_s *, uint8_t);
    void (*undump_prb)(struct riot_context_s *, uint8_t);
    void (*store_pra)(struct riot_context_s *, uint8_t);
    void (*store_prb)(struct riot_context_s *, uint8_t);
    uint8_t (*read_pra)(struct riot_context_s *);
    uint8_t (*read_prb)(struct riot_context_s *);
    void (*reset)(struct riot_context_s *riot_context);
    void (*set_irq)(struct riot_context_s *, int, CLOCK);
    void (*restore_irq)(struct riot_context_s *, int);
} riot_context_t;

struct alarm_context_s;
struct snapshot_s;

void riotcore_setup_context(riot_context_t *riot_context);
void riotcore_init(riot_context_t *riot_context, struct alarm_context_s *alarm_context, unsigned int number);
void riotcore_shutdown(struct riot_context_s *riot_context);
void riotcore_reset(riot_context_t *riot_context);
void riotcore_disable(riot_context_t *riot_context);
void riotcore_signal(riot_context_t *riot_context, int sig, int type);
void riotcore_store(riot_context_t *riot_context, uint16_t addr, uint8_t data);
uint8_t riotcore_read(riot_context_t *riot_context, uint16_t addr);
uint8_t riotcore_peek(riot_context_t *riot_context, uint16_t addr);
void riotcore_dump(riot_context_t *riot_context);

int riotcore_snapshot_write_module(struct riot_context_s *riot_context, struct snapshot_s *p);
int riotcore_snapshot_read_module(struct riot_context_s *riot_context, struct snapshot_s *p);

#endif
