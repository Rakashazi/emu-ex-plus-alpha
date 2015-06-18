/*
 * c64cia1.c - Definitions for the first MOS6526 (CIA) chip in the C64
 * ($DC00).
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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
 * */

#include "vice.h"

#include <stdio.h>

#include "c64fastiec.h"
#include "c64-resources.h"
#include "c64.h"
#include "c64cia.h"
#include "cia.h"
#include "interrupt.h"
#include "drive.h"
#include "joystick.h"
#include "keyboard.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "types.h"
#include "userport_joystick.h"
#include "vicii.h"

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
#include "rsuser.h"
#endif

#ifdef HAVE_MOUSE
#include "mouse.h"
#endif

/* #define DEBUG_KBD */

#ifdef DEBUG_KBD
#define DBGA(x) printf x
#define DBGB(x) printf x
#else
#define DBGA(x)
#define DBGB(x)
#endif

void cia1_store(WORD addr, BYTE data)
{
    ciacore_store(machine_context.cia1, addr, data);
}

BYTE cia1_read(WORD addr)
{
    return ciacore_read(machine_context.cia1, addr);
}

BYTE cia1_peek(WORD addr)
{
    return ciacore_peek(machine_context.cia1, addr);
}

void cia1_update_model(void)
{
    if (machine_context.cia1) {
        machine_context.cia1->model = cia1_model;
    }
}

static void cia_set_int_clk(cia_context_t *cia_context, int value, CLOCK clk)
{
    interrupt_set_irq(maincpu_int_status, cia_context->int_num, value, clk);
}

static void cia_restore_int(cia_context_t *cia_context, int value)
{
    interrupt_restore_irq(maincpu_int_status, cia_context->int_num, value);
}

/*************************************************************************
 * I/O
 */

void cia1_set_extended_keyboard_rows_mask(BYTE value)
{
}

static void pulse_ciapc(cia_context_t *cia_context, CLOCK rclk)
{
}

static void pre_store(void)
{
    vicii_handle_pending_alarms_external_write();
}

static void pre_read(void)
{
    vicii_handle_pending_alarms_external(0);
}

static void pre_peek(void)
{
    vicii_handle_pending_alarms_external(0);
}

static void do_reset_cia(cia_context_t *cia_context)
{
}

static void cia1_internal_lightpen_check(BYTE pa, BYTE pb)
{
    BYTE val = 0xff;
    BYTE msk = pa & ~joystick_value[2];
    BYTE m;
    int i;

    for (m = 0x1, i = 0; i < 8; m <<= 1, i++) {
        if (!(msk & m)) {
            val &= ~keyarr[i];
        }
    }

    m = val & pb & ~joystick_value[1];

    vicii_set_light_pen(maincpu_clk, !(m & 0x10));
}

void cia1_check_lightpen(void)
{
    cia1_internal_lightpen_check(machine_context.cia1->old_pa, machine_context.cia1->old_pb);
}

static void store_ciapa(cia_context_t *cia_context, CLOCK rclk, BYTE b)
{
    cia1_internal_lightpen_check(b, machine_context.cia1->old_pb);

#ifdef HAVE_MOUSE
    mouse_set_input((b >> 6) & 0x03);

    if (_mouse_enabled && (mouse_port == 2)) {
        if (mouse_type == MOUSE_TYPE_NEOS) {
            neos_mouse_store(b);
        } else if (mouse_type == MOUSE_TYPE_SMART) {
            smart_mouse_store(b);
        }
    }
#endif
}

static void undump_ciapa(cia_context_t *cia_context, CLOCK rclk, BYTE b)
{
}

static void store_ciapb(cia_context_t *cia_context, CLOCK rclk, BYTE byte)
{
    cia1_internal_lightpen_check(machine_context.cia1->old_pa, byte);

#ifdef HAVE_MOUSE
    if (_mouse_enabled && (mouse_port == 1)) {
        if (mouse_type == MOUSE_TYPE_NEOS) {
            neos_mouse_store(byte);
        } else if (mouse_type == MOUSE_TYPE_SMART) {
            smart_mouse_store(byte);
        }
    }
#endif
}

static void undump_ciapb(cia_context_t *cia_context, CLOCK rclk, BYTE byte)
{
}

/* the following code is used to determine all lines connected to one given line
   in the keyboard matrix. this must be done to emulate connecting two arbitrary
   lines of either port by pressing more than one key on the keyboard at once.

   NOTE: the current code solves the matrix "digitally", and then the various
         analog side effects are faked into it later. For a really 100% correct
         result the matrix should be solved as a 8x8 resistor array, including
         modelling the CIA in/output stages as current sources/drains.

   NOTE: the matrix solver itself is generic enough to consider moving it into
         the toplevel keyboard emulation later, so it can be used by all machines
         with a similar keyboard matrix. (VIC20, C16 ...)
 */
static void matrix_activate_column(int column, BYTE *activerows, BYTE *activecolumns);
static void matrix_activate_row(int row, BYTE *activerows, BYTE *activecolumns);

static void matrix_activate_row(int row, BYTE *activerows, BYTE *activecolumns)
{
    BYTE msk;
    int m, i;
    if ((1 << row) & ~(*activerows)) {
        *activerows |= (1 << row);
        msk = keyarr[row];
        /* loop over columns */
        for (m = 0x1, i = 0; i < 8; m <<= 1, i++) {
            /* activate each column connected to the given row */
            if ((msk & m) & ~(*activecolumns)) {
                matrix_activate_column(i, activerows, activecolumns);
            }
        }
    }
}

static void matrix_activate_column(int column, BYTE *activerows, BYTE *activecolumns)
{
    BYTE msk;
    int m, i;
    if ((1 << column) & ~(*activecolumns)) {
        *activecolumns |= (1 << column);
        msk = rev_keyarr[column];
        /* loop over rows */
        for (m = 0x1, i = 0; i < 8; m <<= 1, i++) {
            /* activate each row connected to the given column */
            if ((msk & m) & ~(*activerows)) {
                matrix_activate_row(i, activerows, activecolumns);
            }
        }
    }
}

/* get all connected rows for one active column */
inline static BYTE matrix_get_active_rows_by_column(int column)
{
    BYTE activerows = 0;
    BYTE activecolumns = 0;
    matrix_activate_column(column, &activerows, &activecolumns);
    return activerows;
}

/* get all connected rows for one active row */
inline static BYTE matrix_get_active_rows_by_row(int row)
{
    BYTE activerows = 0;
    BYTE activecolumns = 0;
    matrix_activate_row(row, &activerows, &activecolumns);
    return activerows;
}

/* get all connected columns for one active row */
inline static BYTE matrix_get_active_columns_by_column(int column)
{
    BYTE activerows = 0;
    BYTE activecolumns = 0;
    matrix_activate_column(column, &activerows, &activecolumns);
    return activecolumns;
}

/* get all connected columns for one active row */
inline static BYTE matrix_get_active_columns_by_row(int row)
{
    BYTE activerows = 0;
    BYTE activecolumns = 0;
    matrix_activate_row(row, &activerows, &activecolumns);
    return activecolumns;
}

/*
   TODO:
    - do more testing (see testprogs/CIA/ciaports) and handle more strange side
      effects
    - perhaps write common code to solve the entire matrix that is used for both
      read_ciapa and read_ciapb.
    - add improvements also to C128
*/

static BYTE read_ciapa(cia_context_t *cia_context)
{
    BYTE byte;
    BYTE val = 0xff;
    BYTE msk;
    BYTE m, tmp;
    int i;

    DBGA(("PA ddra:%02x pa:%02x ddrb:%02x pb:%02x ",
          cia_context->c_cia[CIA_DDRA], cia_context->c_cia[CIA_PRA],
          cia_context->c_cia[CIA_DDRB], cia_context->c_cia[CIA_PRB]));

    /* loop over columns,
       pull down all bits connected to a column which is output and active.
     */
    msk = cia_context->old_pb & ~joystick_value[1];
    for (m = 0x1, i = 0; i < 8; m <<= 1, i++) {
        if (!(msk & m)) {
            tmp = matrix_get_active_columns_by_column(i);

            /* when scanning from port B to port A with inactive bits set to 1
               in port B, ghostkeys will be eliminated (pulled high) if the
               matrix is connected to more 1 bits of port B. this does NOT happen
               when the respective bits are set to input. (see testprogs/CIA/ciaports)
             */
            if (tmp & cia_context->c_cia[CIA_PRB] & cia_context->c_cia[CIA_DDRB]) {
                val &= ~rev_keyarr[i];
                DBGA(("<force high %02x>", m));
            } else {
                val &= ~matrix_get_active_rows_by_column(i);
            }
        }
    }
    DBGA((" val:%02x", val));

    /* loop over rows,
       pull down all bits connected to a row which is output and active.
       handles the case when port a is used for both input and output
     */
    msk = cia_context->old_pa & ~joystick_value[2];
    for (m = 0x1, i = 0; i < 8; m <<= 1, i++) {
        if (!(msk & m)) {
            val &= ~matrix_get_active_rows_by_row(i);
        }
    }
    DBGA((" val:%02x", val));

    byte = (val & (cia_context->c_cia[CIA_PRA] | ~(cia_context->c_cia[CIA_DDRA]))) & ~joystick_value[2];

    DBGA((" out:%02x\n", byte));

#ifdef HAVE_MOUSE
    if (_mouse_enabled && (mouse_port == 2)) {
        switch (mouse_type) {
        case MOUSE_TYPE_NEOS:
            byte &= neos_mouse_read();
            break;
        case MOUSE_TYPE_SMART:
            byte &= smart_mouse_read();
            break;
        case MOUSE_TYPE_ST:
        case MOUSE_TYPE_AMIGA:
        case MOUSE_TYPE_CX22:
            byte &= mouse_poll();
            break;
        case MOUSE_TYPE_MICROMYS:
            byte &= micromys_mouse_read();
            break;
        default:
            break;
        }
    }
#endif

    return byte;
}

inline static int ciapb_forcelow(int row, BYTE mask)
{
    BYTE v;

    /* Check for shift lock.
       FIXME: keyboard_shiftlock state may be inconsistent
              with the (rev_)keyarr state. */
    if ((row == 1) && keyboard_shiftlock) {
        return 1;
    }

    /* Check if two or more rows are connected */
    v = matrix_get_active_rows_by_row(row) & mask;
    if ((v & (v - 1)) != 0) {
        return 1;
    }

    /* TODO: check joysticks? */
    return 0;
}

static BYTE read_ciapb(cia_context_t *cia_context)
{
    BYTE byte;
    BYTE val = 0xff;
    BYTE val_outhi;
    BYTE msk, tmp;
    BYTE m;
    int i;

    /* loop over rows,
       pull down all bits connected to a row which is output and active.
     */
    val_outhi = (cia_context->c_cia[CIA_DDRB]) & (cia_context->c_cia[CIA_PRB]);

    DBGB(("PB val_outhi:%02x ddra:%02x pa:%02x ddrb:%02x pb:%02x ", val_outhi,
          cia_context->c_cia[CIA_DDRA], cia_context->c_cia[CIA_PRA],
          cia_context->c_cia[CIA_DDRB], cia_context->c_cia[CIA_PRB]));

    msk = cia_context->old_pa & ~joystick_value[2];
    for (m = 0x1, i = 0; i < 8; m <<= 1, i++) {
        if (!(msk & m)) {
            tmp = matrix_get_active_columns_by_row(i);
            val &= ~tmp;

            /*
                Handle the special case when both port A and port B are programmed as output,
                port A outputs (active) low, and port B outputs high.

                In this case either connecting one port A 0 bit (by pressing either shift-lock)
                or two or more port A 0 bits (by pressing keys of the same column) to one port B
                bit is required to drive port B low (see testprogs/CIA/ciaports)
            */
            if ((cia_context->c_cia[CIA_DDRA] & ~cia_context->c_cia[CIA_PRA] & m) &&
                (cia_context->c_cia[CIA_DDRB] & cia_context->c_cia[CIA_PRB] & tmp)) {
                DBGB(("(%d)", i));
                if (ciapb_forcelow(i, (BYTE)(cia_context->c_cia[CIA_DDRA] & ~cia_context->c_cia[CIA_PRA]))) {
                    val_outhi &= ~tmp;
                    DBGB(("<force low, val_outhi:%02x>", val_outhi));
                }
            }
        }
    }
    DBGB((" val:%02x val_outhi:%02x", val, val_outhi));

    /* loop over columns,
       pull down all bits connected to a column which is output and active.
       handles the case when port b is used for both input and output
     */
    msk = cia_context->old_pb & ~joystick_value[1];
    for (m = 0x1, i = 0; i < 8; m <<= 1, i++) {
        if (!(msk & m)) {
            val &= ~matrix_get_active_columns_by_column(i);
        }
    }
    DBGB((" val:%02x", val));

    byte = val & (cia_context->c_cia[CIA_PRB] | ~(cia_context->c_cia[CIA_DDRB]));
    byte |= val_outhi;
    byte &= ~joystick_value[1];

    DBGB((" out:%02x\n", byte));

#ifdef HAVE_MOUSE
    if (_mouse_enabled && (mouse_port == 1)) {
        switch (mouse_type) {
        case MOUSE_TYPE_NEOS:
            byte &= neos_mouse_read();
            break;
        case MOUSE_TYPE_SMART:
            byte &= smart_mouse_read();
            break;
        case MOUSE_TYPE_ST:
        case MOUSE_TYPE_AMIGA:
        case MOUSE_TYPE_CX22:
            byte &= mouse_poll();
            break;
        case MOUSE_TYPE_MICROMYS:
            byte &= micromys_mouse_read();
            break;
        default:
            break;
        }
    }
#endif

    return byte;
}

static void read_ciaicr(cia_context_t *cia_context)
{
    if (burst_mod == BURST_MOD_CIA1) {
        drive_cpu_execute_all(maincpu_clk);
    }
}

static void read_sdr(cia_context_t *cia_context)
{
    if (burst_mod == BURST_MOD_CIA1) {
        drive_cpu_execute_all(maincpu_clk);
    }
}

static void store_sdr(cia_context_t *cia_context, BYTE byte)
{
    if (burst_mod == BURST_MOD_CIA1) {
        c64fastiec_fast_cpu_write((BYTE)byte);
    }
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    if (rsuser_enabled) {
        rsuser_tx_byte(byte);
    }
#endif
    /* FIXME: in the upcoming userport system this call needs to be conditional */
    userport_joystick_store_sdr(byte);
}

void cia1_init(cia_context_t *cia_context)
{
    ciacore_init(machine_context.cia1, maincpu_alarm_context, maincpu_int_status, maincpu_clk_guard);
}

void cia1_setup_context(machine_context_t *machine_context)
{
    cia_context_t *cia;

    machine_context->cia1 = lib_calloc(1, sizeof(cia_context_t));
    cia = machine_context->cia1;

    cia->prv = NULL;
    cia->context = NULL;

    cia->rmw_flag = &maincpu_rmw_flag;
    cia->clk_ptr = &maincpu_clk;

    cia1_set_timing(cia, C64_PAL_CYCLES_PER_SEC, 50);

    ciacore_setup_context(cia);

    if (machine_class == VICE_MACHINE_C64SC
        || machine_class == VICE_MACHINE_SCPU64) {
        cia->write_offset = 0;
    }

    cia->model = cia1_model;

    cia->debugFlag = 0;
    cia->irq_line = IK_IRQ;
    cia->myname = lib_msprintf("CIA1");

    cia->undump_ciapa = undump_ciapa;
    cia->undump_ciapb = undump_ciapb;
    cia->store_ciapa = store_ciapa;
    cia->store_ciapb = store_ciapb;
    cia->store_sdr = store_sdr;
    cia->read_ciapa = read_ciapa;
    cia->read_ciapb = read_ciapb;
    cia->read_ciaicr = read_ciaicr;
    cia->read_sdr = read_sdr;
    cia->cia_set_int_clk = cia_set_int_clk;
    cia->cia_restore_int = cia_restore_int;
    cia->do_reset_cia = do_reset_cia;
    cia->pulse_ciapc = pulse_ciapc;
    cia->pre_store = pre_store;
    cia->pre_read = pre_read;
    cia->pre_peek = pre_peek;
}

void cia1_set_timing(cia_context_t *cia_context, int tickspersec, int powerfreq)
{
    cia_context->power_freq = powerfreq;
    cia_context->ticks_per_sec = tickspersec;
    cia_context->todticks = tickspersec / powerfreq;
    cia_context->power_tickcounter = 0;
    cia_context->power_ticks = 0;
}

