/*
 * riotcore.c - Core functions for 6532 RAM Input/Output Timer (RIOT) emulation.
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

#include "vice.h"

#include <stdio.h>

#include "alarm.h"
#include "lib.h"
#include "log.h"
#include "monitor.h"
#include "riot.h"
#include "snapshot.h"
#include "types.h"


static const int divider[] = {
    1, 8, 64, 1024
};

static void update_irq(riot_context_t *riot_context, uint8_t new_irqfl)
{
    int new_irqline;

    new_irqline = (new_irqfl & 0x80)
                  || ((new_irqfl & 0x40) && (riot_context->r_edgectrl & 2));

    if (new_irqline && !(riot_context->r_irqline)) {
        riot_context->set_irq(riot_context, 1, *(riot_context->clk_ptr));
    }
    if (riot_context->r_irqline && !new_irqline) {
        riot_context->set_irq(riot_context, 0, *(riot_context->clk_ptr));
    }

    riot_context->r_irqline = new_irqline;
    riot_context->r_irqfl = new_irqfl;
}

void riotcore_signal(riot_context_t *riot_context, int sig, int type)
{
    uint8_t newirq = riot_context->r_irqfl & 0xbf;

    /* You better not call that twice with the same flag - the IRQ
     * will be set twice... */

    if ((type == RIOT_SIG_FALL) && !(riot_context->r_edgectrl & 1)) {
        newirq |= 0x40;
    } else
    if ((type == RIOT_SIG_RISE) && (riot_context->r_edgectrl & 1)) {
        newirq |= 0x40;
    }

    update_irq(riot_context, newirq);
}

static void update_timer(riot_context_t *riot_context)
{
    CLOCK underfl = (*(riot_context->clk_ptr) - riot_context->r_write_clk)
                  / riot_context->r_divider;

    if (underfl > riot_context->r_N) {
        riot_context->r_write_clk += riot_context->r_N
                                     * riot_context->r_divider;
        riot_context->r_N = 255;
        riot_context->r_divider = 1;
    }
    riot_context->r_write_clk += (*(riot_context->clk_ptr)
                                  - riot_context->r_write_clk) & 0xff00;
}

void riotcore_disable(riot_context_t *riot_context)
{
    alarm_unset(riot_context->alarm);
    riot_context->enabled = 0;
}

void riotcore_reset(riot_context_t *riot_context)
{
    riot_context->riot_io[0] = 0;
    riot_context->riot_io[1] = 0;
    riot_context->riot_io[2] = 0;
    riot_context->riot_io[3] = 0;

    riot_context->read_clk = 0;

    alarm_unset(riot_context->alarm);

    riot_context->old_pa = 0xff;
    riot_context->old_pb = 0xff;

    riot_context->r_edgectrl = 0;
    riot_context->r_irqfl = 0;
    riot_context->r_irqline = 0;
    riot_context->set_irq(riot_context, 0, *(riot_context->clk_ptr));

    riot_context->r_write_clk = *(riot_context->clk_ptr);
    riot_context->r_N = 255;
    riot_context->r_divider = 1;
    riot_context->r_irqen = 0;

    riot_context->reset(riot_context);

    riot_context->enabled = 1;
}

void riotcore_store(riot_context_t *riot_context, uint16_t addr, uint8_t byte)
{
    CLOCK rclk;

    if (riot_context->rmw_flag) {
        (*(riot_context->clk_ptr))--;
        riot_context->rmw_flag = 0;
        riotcore_store(riot_context, addr, riot_context->last_read);
        (*(riot_context->clk_ptr))++;
    }

    rclk = *(riot_context->clk_ptr) - 1;   /* stores have a one-cylce offset */

    addr &= 0x1f;

    /* manage the weird addressing schemes */

    if ((addr & 0x04) == 0) {           /* I/O */
        addr &= 3;
        switch (addr) {
            case 0:       /* ORA */
            case 1:       /* DDRA */
                (riot_context->riot_io)[addr] = byte;
                byte = (riot_context->riot_io)[0] | ~(riot_context->riot_io)[1];
                riot_context->store_pra(riot_context, byte);
                riot_context->old_pa = byte;
                break;
            case 2:       /* ORB */
            case 3:       /* DDRB */
                (riot_context->riot_io)[addr] = byte;
                byte = (riot_context->riot_io)[2] | ~(riot_context->riot_io)[3];
                riot_context->store_prb(riot_context, byte);
                riot_context->old_pb = byte;
                break;
        }
    } else
    if ((addr & 0x14) == 0x14) {        /* set timer */
        int newirq = riot_context->r_irqfl & 0x7f;
/*
        log_warning(riot_context->log,
                    "write timer %02x@%d not yet implemented\n",
                    byte, addr);
*/
        riot_context->r_divider = divider[addr & 3];
        riot_context->r_write_clk = rclk + 1;
        riot_context->r_N = byte;
        riot_context->r_irqen = (addr & 8);

        if (byte) {
            (riot_context->r_N)--;
            if (riot_context->r_irqen) {
                alarm_set(riot_context->alarm, riot_context->r_write_clk
                          + riot_context->r_N * riot_context->r_divider);
            }
        } else {
            /* setup IRQ? */
            riot_context->r_N = 255;
            riot_context->r_divider = 1;
            if (riot_context->r_irqen) {
                newirq |= 0x80;
            }
        }

        update_irq(riot_context, (uint8_t)(newirq));

        if (!(riot_context->r_irqen)) {
            alarm_unset(riot_context->alarm);
        }
    } else
    if ((addr & 0x14) == 0x04) {        /* set edge detect control */
/*
        log_message(riot_context->log, "edge control %02x@%d\n", byte, addr);
*/
        riot_context->r_edgectrl = addr & 3;

        update_irq(riot_context, riot_context->r_irqfl);
    }
}

uint8_t riotcore_read(riot_context_t *riot_context, uint16_t addr)
{
#ifdef MYRIOT_TIMER_DEBUG
    uint8_t myriot_read_(riot_context_t *, uint16_t);
    uint8_t retv = myriot_read_(riot_context, addr);
    addr &= 0x1f;
    if ((addr > 3 && addr < 10) || app_resources.debugFlag) {
        log_message(riot_context->log,
                    (riot_context->myname)
                    "(%x) -> %02x, clk=%d", addr, retv,
                    *(riot_context->clk_ptr));
    }
    return retv;
}

uint8_t myriot_read_(riot_context_t *riot_context, uint16_t addr)
{
#endif
    CLOCK rclk;

    addr &= 0x1f;

    /* Hack for opcode fetch, where the clock does not change */
    if (*(riot_context->clk_ptr) <= riot_context->read_clk) {
        rclk = riot_context->read_clk + (++(riot_context->read_offset));
    } else {
        riot_context->read_clk = *(riot_context->clk_ptr);
        riot_context->read_offset = 0;
        rclk = *(riot_context->clk_ptr);
    }

    /* manage the weird addressing schemes */

    if ((addr & 0x04) == 0) {           /* I/O */
        switch (addr & 3) {
            case 0:       /* ORA */
                riot_context->last_read = riot_context->read_pra(riot_context);
                return riot_context->last_read;
                break;
            case 1:       /* DDRA */
                riot_context->last_read = riot_context->riot_io[1];
                return riot_context->last_read;
                break;
            case 2:       /* ORB */
                riot_context->last_read = riot_context->read_prb(riot_context);
                return riot_context->last_read;
                break;
            case 3:       /* DDRB */
                riot_context->last_read = riot_context->riot_io[3];
                return riot_context->last_read;
                break;
        }
    } else
    if ((addr & 0x05) == 0x04) {        /* read timer */
/*
        log_warning(riot_context->log, "read timer @%d not yet implemented\n",
                addr);
*/
        update_irq(riot_context, (uint8_t)(riot_context->r_irqfl & 0x7f));

        update_timer(riot_context);

        riot_context->r_irqen = addr & 8;

        if (riot_context->r_irqen) {
            alarm_set(riot_context->alarm, riot_context->r_write_clk
                      + riot_context->r_N * riot_context->r_divider);
        } else {
            alarm_unset(riot_context->alarm);
        }

        riot_context->last_read = (uint8_t)(riot_context->r_N
                                         - (rclk - riot_context->r_write_clk)
                                         / riot_context->r_divider);
        return riot_context->last_read;
    } else
    if ((addr & 0x05) == 0x05) {        /* read irq flag */
/*
        log_message(riot_context->log, "read irq flag @%d\n", addr);
*/
        riot_context->last_read = riot_context->r_irqfl;

        if (riot_context->r_irqen) {
            update_timer(riot_context);
            alarm_set(riot_context->alarm, riot_context->r_write_clk
                      + riot_context->r_N * riot_context->r_divider);
        }

        update_irq(riot_context, (uint8_t)(riot_context->r_irqfl & 0xbf));
    }
    return 0xff;
}

/* read from I/O without side effects */
/* FIXME: check if this is working correctly */
uint8_t riotcore_peek(riot_context_t *riot_context, uint16_t addr)
{
    CLOCK rclk = *(riot_context->clk_ptr); /* FIXME */
    uint8_t ret = 0xff;

    addr &= 0x1f;

    /* manage the weird addressing schemes */
    if ((addr & 0x04) == 0) {           /* I/O */
        switch (addr & 3) {
            case 0:       /* ORA */
                ret = riot_context->riot_io[0]; /* FIXME */
                break;
            case 1:       /* DDRA */
                ret = riot_context->riot_io[1];
                break;
            case 2:       /* ORB */
                ret = riot_context->riot_io[2]; /* FIXME */
                break;
            case 3:       /* DDRB */
                ret = riot_context->riot_io[3];
                break;
        }
    } else if ((addr & 0x05) == 0x04) {        /* read timer */
        ret = (uint8_t)(riot_context->r_N - (rclk - riot_context->r_write_clk) / riot_context->r_divider);
    } else if ((addr & 0x05) == 0x05) {        /* read irq flag */
        ret = riot_context->r_irqfl;
    }
    return ret;
}

void riotcore_dump(riot_context_t *riot_context)
{
    CLOCK rclk = *(riot_context->clk_ptr); /* FIXME */
    unsigned int timer = (uint8_t)(riot_context->r_N - (rclk - riot_context->r_write_clk) / riot_context->r_divider);
    mon_out("ORA: $%02x DDRA: $%02x\n", riot_context->riot_io[0], riot_context->riot_io[1]);
    mon_out("ORB: $%02x DDRB: $%02x\n", riot_context->riot_io[2], riot_context->riot_io[3]);
    mon_out("Timer: $%04x\n", timer);
    mon_out("IRQ Flags: $%02x\n", riot_context->r_irqfl);
}

static void riotcore_int_riot(CLOCK offset, void *data)
{
    riot_context_t *riot_context = (riot_context_t *)data;

/*  CLOCK rclk = *(riot_context->clk_ptr) - offset; */

    alarm_unset(riot_context->alarm);

    update_irq(riot_context, (uint8_t)(riot_context->r_irqfl | 0x80));
}

void riotcore_setup_context(riot_context_t *riot_context)
{
    riot_context->log = LOG_ERR;
    riot_context->read_clk = 0;
    riot_context->read_offset = 0;
    riot_context->last_read = 0;
    riot_context->r_edgectrl = 0;
    riot_context->r_irqfl = 0;
    riot_context->r_irqline = 0;
}

void riotcore_init(riot_context_t *riot_context,
                   alarm_context_t *alarm_context,
                   unsigned int number)
{
    char *buffer;

    riot_context->log = log_open(riot_context->myname);

    buffer = lib_msprintf("%sT1", riot_context->myname);
    riot_context->alarm = alarm_new(alarm_context, buffer, riotcore_int_riot,
                                    riot_context);
    lib_free(buffer);
}

void riotcore_shutdown(riot_context_t *riot_context)
{
    lib_free(riot_context->prv);
    lib_free(riot_context->myname);
    lib_free(riot_context);
}

/*-------------------------------------------------------------------*/

/* The dump format has a module header and the data generated by the
 * chip...
 *
 * The version of this dump description is 0.0
 *
 */

#define RIOT_DUMP_VER_MAJOR      0
#define RIOT_DUMP_VER_MINOR      0

/*
 * The dump data:
 *
 * UBYTE        ORA
 * UBYTE        DDRA
 * UBYTE        ORB
 * UBYTE        DDRB
 * UBYTE        EDGECTRL        edge control value
 * UBYTE        IRQFL           IRQ fl:
 *                              Bit 6/7: see RIOT IRQ flag
 *                              Bit 0: state of IRQ line (1=active)
 * UBYTE        N               current timer value
 * UWORD        divider         1, 8, 64, 104
 * UWORD        rest            cycles that the divider has done since last
 *                              counter tick
 * UBYTE        irqen           0= timer IRQ disabled, 1= enabled
 *
 */

int riotcore_snapshot_write_module(riot_context_t *riot_context, snapshot_t *p)
{
    snapshot_module_t *m;

    m = snapshot_module_create(p, riot_context->myname, RIOT_DUMP_VER_MAJOR, RIOT_DUMP_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    update_timer(riot_context);

    if (0
        || SMW_B(m, riot_context->riot_io[0]) < 0
        || SMW_B(m, riot_context->riot_io[1]) < 0
        || SMW_B(m, riot_context->riot_io[2]) < 0
        || SMW_B(m, riot_context->riot_io[3]) < 0
        || SMW_B(m, riot_context->r_edgectrl) < 0
        || SMW_B(m, (uint8_t)(riot_context->r_irqfl | (riot_context->r_irqline ? 1 : 0))) < 0
        || SMW_B(m, (uint8_t)(riot_context->r_N - (*(riot_context->clk_ptr) - riot_context->r_write_clk) / riot_context->r_divider)) < 0
        || SMW_W(m, (uint16_t)(riot_context->r_divider)) < 0
        || SMW_W(m, (uint8_t)((*(riot_context->clk_ptr) - riot_context->r_write_clk) % riot_context->r_divider)) < 0
        || SMW_B(m, (uint8_t)(riot_context->r_irqen ? 1 : 0)) < 0) {
            snapshot_module_close(m);
            return -1;
    }

    return snapshot_module_close(m);
}

int riotcore_snapshot_read_module(riot_context_t *riot_context, snapshot_t *p)
{
    uint8_t vmajor, vminor;
    uint8_t byte_r_N;
    uint8_t byte;
    uint16_t word_r_divider;
    uint16_t word_r_write_clk;
    snapshot_module_t *m;

    m = snapshot_module_open(p, riot_context->myname, &vmajor, &vminor);

    if (m == NULL) {
        log_message(riot_context->log,
                    "Could not find snapshot module %s", riot_context->myname);
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, RIOT_DUMP_VER_MAJOR, RIOT_DUMP_VER_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        snapshot_module_close(m);
        return -1;
    }

    /* just to be safe */
    alarm_unset(riot_context->alarm);

    if (0
        || SMR_B(m, &(riot_context->riot_io)[0]) < 0
        || SMR_B(m, &(riot_context->riot_io)[1]) < 0
        || SMR_B(m, &(riot_context->riot_io)[2]) < 0
        || SMR_B(m, &(riot_context->riot_io)[3]) < 0
        || SMR_B(m, &(riot_context->r_edgectrl)) < 0
        || SMR_B(m, &(riot_context->r_irqfl)) < 0
        || SMR_B(m, &byte_r_N) < 0
        || SMR_W(m, &word_r_divider) < 0
        || SMR_W(m, &word_r_write_clk) < 0
        || SMR_B(m, &byte) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    riot_context->old_pa = riot_context->riot_io[0]
                           | ~(riot_context->riot_io)[1];
    riot_context->undump_pra(riot_context, riot_context->old_pa);

    riot_context->old_pb = riot_context->riot_io[2]
                           | ~(riot_context->riot_io)[3];
    riot_context->undump_prb(riot_context, riot_context->old_pb);

    riot_context->r_N = byte_r_N;

    riot_context->r_divider = word_r_divider;

    riot_context->r_write_clk = *(riot_context->clk_ptr) - word_r_write_clk;

    if (riot_context->r_irqfl & 1) {
        riot_context->r_irqline = 1;
        riot_context->restore_irq(riot_context, 1);
    }
    riot_context->r_irqfl &= 0xc0;

    riot_context->r_irqen = byte;
    if (riot_context->r_irqen) {
        alarm_set(riot_context->alarm, riot_context->r_write_clk
                  + riot_context->r_N * riot_context->r_divider);
    }

    riot_context->read_clk = 0;

    return snapshot_module_close(m);
}
