/*
 * mc6821core.c - Motorola MC6821 Emulation
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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


#include "vice.h"

/* define for debug messages */
/* #define MC6821_DEBUG */

#ifdef MC6821_DEBUG
#include <stdio.h>
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#include <stdlib.h>

#include "mc6821core.h"
#include "snapshot.h"

void mc6821core_reset(mc6821_state *ctx)
{
    ctx->CA2 = 0;
    ctx->CA2state = 0;

    ctx->CB2 = 0;
    ctx->CB2state = 0;

    ctx->ctrlA = 0;
    ctx->dataA = 0;
    ctx->ddrA = 0;

    ctx->ctrlB = 0;
    ctx->dataB = 0;
    ctx->ddrB = 0;

    if (ctx->set_ca2) {
        ctx->set_ca2(ctx);
    }
    if (ctx->set_cb2) {
        ctx->set_cb2(ctx);
    }
    if (ctx->set_pa) {
        ctx->set_pa(ctx);
    }
    if (ctx->set_pb) {
        ctx->set_pb(ctx);
    }
}

BYTE mc6821core_read(mc6821_state *ctx, int port /* rs1 */, int reg /* rs0 */)
{
    BYTE data = 0;

    if (port == 0) {
        /* MC6821 Port A */
        if (reg == 1) {
            /* control register */
            data = ctx->ctrlA;
        } else {
            if (ctx->ctrlA & MC6821_CTRL_REG) {
                /* data port */
                data = ctx->dataA & ctx->ddrA;

                /* FIXME: CA2 output mode 0x08 */
                if (ctx->CA2state == 1) {
                    ctx->CA2 = 0;
                    if (ctx->set_ca2) {
                        ctx->set_ca2(ctx);
                    }
                }

                /* get input from port */
                if (ctx->get_pa) {
                    data |= ctx->get_pa(ctx) & ~(ctx->ddrA);
                } else {
                    data |= 0xff & ~(ctx->ddrA);
                }

                /* FIXME: CA2 output mode 0x08 */
                if (ctx->CA2state == 1) {
                    ctx->CA2 = 1;
                    if (ctx->set_ca2) {
                        ctx->set_ca2(ctx);
                    }
                    ctx->CA2state = 0;
                }

                /* irq flags are cleared when reading output port */
                ctx->ctrlA &= ~(MC6821_CTRL_IRQ1 | MC6821_CTRL_IRQ2);
            } else {
                data = ctx->ddrA;
            }
        }
    } else {
        /* MC6821 Port B */
        if (reg == 1) {
            /* control register */
            data = ctx->ctrlB;
        } else {
            if (ctx->ctrlB & MC6821_CTRL_REG) {
                /* data port */
                data = ctx->dataB & ctx->ddrB;
                if (ctx->get_pb) {
                    data |= ctx->get_pb(ctx) & ~(ctx->ddrB);
                } else {
                    data |= 0xff & ~(ctx->ddrB);
                }

                /* irq flags are cleared when reading output port */
                ctx->ctrlB &= ~(MC6821_CTRL_IRQ1 | MC6821_CTRL_IRQ2);
            } else {
                data = ctx->ddrB;
            }
        }
    }
    return data;
}

BYTE mc6821core_peek(mc6821_state *ctx, int port /* rs1 */, int reg /* rs0 */)
{
    BYTE data = 0;

    if (port == 0) {
        /* MC6821 Port A */
        if (reg == 1) {
            /* control register */
            data = ctx->ctrlA;
        } else {
            if (ctx->ctrlA & MC6821_CTRL_REG) {
                data = ctx->dataA;
#if 0
                if (ctx->get_pa) {
                    data = ctx->get_pa(ctx);
                }
#endif
            } else {
                data = ctx->ddrA;
            }
        }
    } else {
        /* MC6821 Port B */
        if (reg == 1) {
            /* control register */
            data = ctx->ctrlB;
        } else {
            if (ctx->ctrlB & MC6821_CTRL_REG) {
                data = ctx->dataB;
#if 0
                if (ctx->get_pb) {
                    data = ctx->get_pb(ctx);
                }
#endif
            } else {
                data = ctx->ddrB;
            }
        }
    }
    return data;
}

void mc6821core_store(mc6821_state *ctx, int port /* rs1 */, int reg /* rs0 */, BYTE data)
{
    if (port == 0) {
        /* MC6821 Port A */
        if (reg == 1) {
            /* control register */
            /* DBG(("MC6821: PA CTRL %02x\n", data)); */
            ctx->ctrlA = data;
            if (data & MC6821_CTRL_C2DDR) {
                /* CA2 is output */
                switch (data & MC6821_CTRL_C2MODE) {
                    case MC6821_CTRL_C2_RESET_C2:
                        ctx->CA2 = 0;
                        /* update CA2 immediately */
                        if (ctx->set_ca2) {
                            ctx->set_ca2(ctx);
                        }
                        break;
                    case MC6821_CTRL_C2_SET_C2:
                        ctx->CA2 = 1;
                        /* update CA2 immediately */
                        if (ctx->set_ca2) {
                            ctx->set_ca2(ctx);
                        }
                        break;
                    case MC6821_CTRL_C2_STROBE_C:
                        DBG(("MC6821: PA CTRL unimplemented output mode %02x for CA2\n", data & MC6821_CTRL_C2MODE));
                        break;
                    case MC6821_CTRL_C2_STROBE_E:
                        DBG(("MC6821: PA CTRL FIXME output mode %02x for CA2\n", data & MC6821_CTRL_C2MODE));
                        ctx->CA2state = 1;
                        break;
                }
            } else {
                /* CA2 is input */
                if (data & MC6821_CTRL_C2_IRQEN) {
                    DBG(("MC6821: PA CTRL unimplemented irq mode %02x for CA2\n", data & MC6821_CTRL_C2MODE));
                }
            }
        } else {
            if (ctx->ctrlA & MC6821_CTRL_REG) {
                /* output register */
                /* DBG(("MC6821: PA DATA %02x\n",data)); */

                ctx->dataA = data;
                if (ctx->set_pa) {
                    ctx->set_pa(ctx);
                }
            } else {
                /* data direction register */
                /* DBG(("MC6821: PA DDR %02x\n",data)); */
                ctx->ddrA = data;
                /* update port if ddr changes */
                if (ctx->set_pa) {
                    ctx->set_pa(ctx);
                }
            }
        }
    } else {
        /* MC6821 Port B */
        if (reg == 1) {
            /* control register */
            DBG(("MC6821: PB CTRL %02x\n", data));
            ctx->ctrlB = data;
            if (data & MC6821_CTRL_C2DDR) {
                /* CB2 is output */
                switch (data & MC6821_CTRL_C2MODE) {
                    case MC6821_CTRL_C2_RESET_C2:
                        ctx->CB2 = 0;
                        /* update CB2 immediately */
                        if (ctx->set_cb2) {
                            ctx->set_cb2(ctx);
                        }
                        break;
                    case MC6821_CTRL_C2_SET_C2:
                        ctx->CB2 = 1;
                        /* update CB2 immediately */
                        if (ctx->set_cb2) {
                            ctx->set_cb2(ctx);
                        }
                        break;
                    case MC6821_CTRL_C2_STROBE_C:
                        DBG(("MC6821: PB CTRL unimplemented output mode %02x for CB2\n", data & MC6821_CTRL_C2MODE));
                        break;
                    case MC6821_CTRL_C2_STROBE_E:
                        DBG(("MC6821: PB CTRL FIXME output mode %02x for CB2\n", data & MC6821_CTRL_C2MODE));
                        ctx->CB2state = 1;
                        break;
                }
            } else {
                /* CB2 is input */
                if (data & MC6821_CTRL_C2_IRQEN) {
                    DBG(("MC6821: PB CTRL unimplemented irq mode %02x for CB2\n", data & MC6821_CTRL_C2MODE));
                }
            }
        } else {
            if (ctx->ctrlB & MC6821_CTRL_REG) {
                /* output register */
                /* DBG(("MC6821: PB DATA %02x\n",data)); */
                ctx->dataB = data;

                /* FIXME: CB2 output mode 0x08 */
                if (ctx->CB2state == 1) {
                    ctx->CB2 = 0;
                    if (ctx->set_cb2) {
                        ctx->set_cb2(ctx);
                    }
                }

                if (ctx->set_pb) {
                    ctx->set_pb(ctx);
                }

                /* FIXME: CB2 output mode 0x08 */
                if (ctx->CB2state == 1) {
                    ctx->CB2 = 1;
                    if (ctx->set_cb2) {
                        ctx->set_cb2(ctx);
                    }
                    ctx->CB2state = 0;
                }
            } else {
                /* data direction register */
                DBG(("MC6821: PB DDR %02x\n", data));
                ctx->ddrB = data;
                /* update port if ddr changes */
                if (ctx->set_pb) {
                    ctx->set_pb(ctx);
                }
            }
        }
    }
}

void mc6821core_set_signal(mc6821_state *ctx, int line)
{
/*    DBG(("MC6821: SIGNAL %02x\n", line)); */
    switch (line) {
        case MC6821_SIGNAL_CA1:
            ctx->ctrlA = (ctx->ctrlA | MC6821_CTRL_IRQ1);
            break;
        case MC6821_SIGNAL_CA2:
            ctx->ctrlA = (ctx->ctrlA | MC6821_CTRL_IRQ2);
            break;
        case MC6821_SIGNAL_CB1:
            ctx->ctrlB = (ctx->ctrlB | MC6821_CTRL_IRQ1);
            break;
        case MC6821_SIGNAL_CB2:
            ctx->ctrlB = (ctx->ctrlB | MC6821_CTRL_IRQ2);
            break;
    }
}

int mc6821core_snapshot_write_data(mc6821_state *ctx, snapshot_module_t *m)
{
    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (BYTE)ctx->ctrlA) < 0
        || SMW_B(m, (BYTE)ctx->ctrlB) < 0
        || SMW_B(m, (BYTE)ctx->dataA) < 0
        || SMW_B(m, (BYTE)ctx->dataB) < 0
        || SMW_B(m, (BYTE)ctx->ddrA) < 0
        || SMW_B(m, (BYTE)ctx->ddrB) < 0
        || SMW_B(m, (BYTE)ctx->CA2) < 0
        || SMW_B(m, (BYTE)ctx->CA2state) < 0
        || SMW_B(m, (BYTE)ctx->CB2) < 0
        || SMW_B(m, (BYTE)ctx->CB2state) < 0) {
        return -1;
    }

    return 0;
}

int mc6821core_snapshot_read_data(mc6821_state *ctx, snapshot_module_t *m)
{
    if (m == NULL) {
        return -1;
    }

    if (0
        || SMR_B(m, &ctx->ctrlA) < 0
        || SMR_B(m, &ctx->ctrlB) < 0
        || SMR_B(m, &ctx->dataA) < 0
        || SMR_B(m, &ctx->dataB) < 0
        || SMR_B(m, &ctx->ddrA) < 0
        || SMR_B(m, &ctx->ddrB) < 0
        || SMR_B_INT(m, &ctx->CA2) < 0
        || SMR_B_INT(m, &ctx->CA2state) < 0
        || SMR_B_INT(m, &ctx->CB2) < 0
        || SMR_B_INT(m, &ctx->CB2state) < 0) {
        return -1;
    }

    return 0;
}
