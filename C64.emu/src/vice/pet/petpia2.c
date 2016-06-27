/*
 * petpia2.c -- PIA#1 chip emulation.
 *
 * Written by
 *  Jouko Valta <jopi@stekt.oulu.fi>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#include "drive.h"
#include "interrupt.h"
#include "maincpu.h"
#include "parallel.h"
#include "petpia.h"
#include "piacore.h"
#include "types.h"

/* ------------------------------------------------------------------------- */
/* Renaming exported functions */

#define MYPIA_NAME      "PIA2"

#define mypia_init pia2_init
#define mypia_reset pia2_reset
#define mypia_store pia2_store
#define mypia_read pia2_read
#define mypia_peek pia2_peek
#define mypia_snapshot_write_module pia2_snapshot_write_module
#define mypia_snapshot_read_module pia2_snapshot_read_module
#define mypia_signal pia2_signal
#define mypia_dump pia2_dump

static piareg mypia;

/* ------------------------------------------------------------------------- */
/* CPU binding */

static void my_set_int(unsigned int pia_int_num, int a)
{
    maincpu_set_irq(pia_int_num, a ? IK_IRQ : IK_NONE);
}

static void my_restore_int(unsigned int pia_int_num, int a)
{
    interrupt_restore_irq(maincpu_int_status, pia_int_num,
                          a ? IK_IRQ : IK_NONE);
}

#define mycpu_rmw_flag   maincpu_rmw_flag
#define myclk            maincpu_clk
#define mycpu_int_status maincpu_int_status

/* ------------------------------------------------------------------------- */
/* I/O */

static void pia_set_ca2(int a)
{
    parallel_cpu_set_ndac((BYTE)((a) ? 0 : 1));
}

static void pia_set_cb2(int a)
{
    parallel_cpu_set_dav((BYTE)((a) ? 0 : 1));
}

static void pia_reset(void)
{
    parallel_cpu_set_bus(0xff); /* all data lines high, because of input mode */
}


/*
E820    PORT A      Input buffer for IEEE data lines
E821    CA2         IEEE NDAC out
        CA1         IEEE ATN in
E822    PORT B      Output buffer for IEEE data lines
E823    CB2         IEEE DAV out
        CB1         IEEE SRQ in
*/

static void store_pa(BYTE byte)
{
}

static void undump_pa(BYTE byte)
{
}


static void store_pb(BYTE byte)
{
    parallel_cpu_set_bus(byte);
}

static void undump_pb(BYTE byte)
{
    parallel_cpu_set_bus(byte);
}

static BYTE read_pa(void)
{
    BYTE byte;

    drive_cpu_execute_all(maincpu_clk);

    if (parallel_debug) {
        log_message(mypia_log,
                    "read pia2 port A %x, parallel_bus=%x, gives %x.",
                    mypia.port_a, parallel_bus,
                    ((parallel_bus & ~mypia.ddr_a)
                     | (mypia.port_a & mypia.ddr_a)));
    }

    byte = (parallel_bus & ~mypia.ddr_a) | (mypia.port_a & mypia.ddr_a);
    return byte;
}

static BYTE read_pb(void)
{
    return 0xff;
}

#include "piacore.c"
