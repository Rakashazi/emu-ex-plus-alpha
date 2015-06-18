/*
 * c64dtvdma.c - C64DTV DMA controller
 *
 * Written by
 *  M.Kiesel <mayne@users.sourceforge.net>
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Daniel Kahlin <daniel@kahlin.net>
 * Based on code by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "c64mem.h"
#include "c64dtvmem.h"
#include "c64dtvflash.h"
#include "c64dtvdma.h"
#include "cmdline.h"
#include "debug.h"
#include "lib.h"
#include "log.h"
#include "util.h"
#include "resources.h"
#include "maincpu.h"
#include "interrupt.h"
#include "snapshot.h"
#include "translate.h"

#ifdef DEBUG
static log_t c64dtvdma_log = LOG_ERR;
#endif

static unsigned int c64dtv_dma_int_num;

/* I/O of the DMA engine ($D3XX) */
BYTE c64dtvmem_dma[0x20];

int dma_active;
int dma_on_irq;

static int dma_busy;

static int dma_source_off;
static int dma_dest_off;
static int dma_irq;

#ifdef DEBUG
static int dma_log_enabled = 0;
#endif

static BYTE dma_data;
static BYTE dma_data_swap;
static int dma_count;
static enum { DMA_IDLE, DMA_READ, DMA_READ_SWAP, DMA_WRITE_SWAP, DMA_WRITE } dma_state; 
static int source_line_off = 0;
static int dest_line_off = 0;
static BYTE source_memtype = 0x00;
static BYTE dest_memtype = 0x00;

#define GET_REG24(a) ((c64dtvmem_dma[a + 2] << 16) | (c64dtvmem_dma[a + 1] << 8) | c64dtvmem_dma[a])
#define GET_REG16(a) ((c64dtvmem_dma[a + 1] << 8) | c64dtvmem_dma[a])
#define GET_REG8(a) (c64dtvmem_dma[a])

/* ------------------------------------------------------------------------- */

void c64dtvdma_init(void)
{
#ifdef DEBUG
    if (c64dtvdma_log == LOG_ERR) {
        c64dtvdma_log = log_open("C64DTVDMA");
    }
#endif

    /* init DMA IRQ */
    c64dtv_dma_int_num = interrupt_cpu_status_int_new(maincpu_int_status, "C64DTVDMA");
}

void c64dtvdma_shutdown(void)
{
}

void c64dtvdma_reset(void)
{
    int i;

#ifdef DEBUG
    if (dma_log_enabled) {
        log_message(c64dtvdma_log, "reset");
    }
#endif

    /* TODO move register file initialization somewhere else? */
    for (i = 0; i < 0x20; ++i) {
        c64dtvmem_dma[i] = 0;
    }

    dma_source_off = 0;
    source_memtype = 0x00;
    dma_dest_off = 0;
    dest_memtype = 0x00;
    dma_busy = 0;
    dma_irq = 0;
    dma_on_irq = 0;
    dma_active = 0;

    dma_count = 0;
    dma_state = DMA_IDLE;
    dma_data = 0x00;
    source_line_off = 0;
    dest_line_off = 0;
}

/* ------------------------------------------------------------------------- */
/* DMA transfer state machine */

static inline void do_dma_read(int swap)
{
    BYTE data;
    int offs;
    int memtype;

    if (!swap) {
        offs = dma_source_off;
        memtype = source_memtype;
    } else {
        offs = dma_dest_off;
        memtype = dest_memtype;
    }
    offs &= 0x1fffff;

    switch (memtype) {
        case 0x00: /* ROM */
            data = c64dtvflash_read(offs);
            break;
        case 0x40: /* RAM */
            data = mem_ram[offs];
            break;
        case 0x80: /* RAM+registers */
            if ((offs >= 0xd000) && (offs < 0xe000)) {
                data = _mem_read_tab_ptr[offs >> 8]((WORD)offs);
            } else {
                data = mem_ram[offs];
            }
            break;
        case 0xc0: /* unknown */
            data = 0;
            break;
        default:
#ifdef DEBUG
            log_message(c64dtvdma_log, "invalid memtype in do_dma_read()");
#endif
            data = 0;
            break;
    }

    if (!swap) {
        dma_data = data;
    } else {
        dma_data_swap = data;
    }
}

static inline void do_dma_write(int swap)
{
    BYTE data;
    int offs;
    int memtype;

    if (!swap) {
        offs = dma_dest_off;
        memtype = dest_memtype;
        data = dma_data;
    } else {
        offs = dma_source_off;
        memtype = source_memtype;
        data = dma_data_swap;
    }
    offs &= 0x1fffff;

    switch (memtype) {
        case 0x00: /* ROM */
            c64dtvflash_store(offs, data);
            break;
        case 0x40: /* RAM */
            mem_ram[offs] = data;
            break;
        case 0x80: /* RAM+registers */
            if ((offs >= 0xd000) && (offs < 0xe000)) {
                _mem_write_tab_ptr[offs >> 8]((WORD)offs, data);
            } else {
                mem_ram[offs] = data;
            }
            break;
        case 0xc0: /* unknown */
            break;
        default:
#ifdef DEBUG
            log_message(c64dtvdma_log, "invalid memtype in do_dma_write()");
#endif
            break;
    }
}

static inline void update_counters(void)
{
    int source_step = GET_REG16(0x06);
    int dest_step = GET_REG16(0x08);
    int source_modulo = GET_REG16(0x0c);
    int dest_modulo = GET_REG16(0x0e);
    int source_line_length = GET_REG16(0x10);
    int dest_line_length = GET_REG16(0x12);
    int source_modulo_enable = GET_REG8(0x1e) & 0x01;
    int dest_modulo_enable = GET_REG8(0x1e) & 0x02;
    int source_direction = (GET_REG8(0x1f) & 0x04) ? +1 : -1;
    int dest_direction = (GET_REG8(0x1f) & 0x08) ? +1 : -1;

    /* update offsets */
    if (source_modulo_enable && (source_line_off >= source_line_length)) {
        source_line_off = 0;
        dma_source_off += source_modulo * source_direction;
    } else {
        source_line_off++;
        dma_source_off += source_step * source_direction;
    }

    if (dest_modulo_enable && (dest_line_off >= dest_line_length)) {
        dest_line_off = 0;
        dma_dest_off += dest_modulo * dest_direction;
    } else {
        dest_line_off++;
        dma_dest_off += dest_step * dest_direction;
    }
}

static inline void perform_dma_cycle(void)
{
    int swap = GET_REG8(0x1f) & 0x02;

    switch (dma_state) {
        case DMA_IDLE:
            break;
        case DMA_READ:
            if (dma_count == 0) {
                dma_state = DMA_IDLE;
                break;
            }
            do_dma_read(0);
            if (swap) {
                dma_state = DMA_READ_SWAP;
            } else {
                dma_state = DMA_WRITE;
            }
            break;
        case DMA_READ_SWAP:
            do_dma_read(1);
            dma_state = DMA_WRITE_SWAP;
            break;
        case DMA_WRITE_SWAP:
            do_dma_write(1);
            dma_state = DMA_WRITE;
            break;
        case DMA_WRITE:
            do_dma_write(0);
            update_counters();

            dma_count--;
            if (dma_count == 0) {
                dma_state = DMA_IDLE;
            } else {
                dma_state = DMA_READ;
            }
            break;
        default:
#ifdef DEBUG
            log_message(c64dtvdma_log, "invalid state in perform_dma_cycle()");
#endif
            dma_state = DMA_IDLE;
            break;
    }
}

/* ------------------------------------------------------------------------- */

/* These are the $D3xx DMA register engine handlers */

void c64dtvdma_trigger_dma(void)
{
    if (!dma_active) {
        int source_continue = GET_REG8(0x1d) & 0x02;
        int dest_continue = GET_REG8(0x1d) & 0x08;

        if (!source_continue) {
            dma_source_off = GET_REG24(0x00) & 0x3fffff;
            source_memtype = GET_REG8(0x02) & 0xc0;
        }
        if (!dest_continue) {
            dma_dest_off = GET_REG24(0x03) & 0x3fffff;
            dest_memtype = GET_REG8(0x05) & 0xc0;
        }

        /* total number of bytes to transfer */
        dma_count = GET_REG16(0x0a);
        /* length=0 means 64 Kb */
        if (dma_count == 0) {
            dma_count = 0x10000;
        }

#ifdef DEBUG
        if (dma_log_enabled && (source_continue || dest_continue)) {
            log_message(c64dtvdma_log, "Source continue %s, dest continue %s", source_continue ? "on" : "off", dest_continue ? "on" : "off");
        }
#endif

        /* initialize state variables */
        source_line_off = 0;
        dest_line_off = 0;

        dma_state = DMA_READ;

        if (GET_REG8(0x1f) & 0x80) {
            dma_irq = 1;
        } else {
            dma_irq = 0;
        }

        dma_busy = 1;
        dma_active = 1;
    }
}

static inline void c64dtv_dma_done(void)
{
    if (dma_irq) {
        maincpu_set_irq(c64dtv_dma_int_num, 1);
        dma_busy = 2;
    }
    dma_busy &= 0xfe;
    dma_active = 0;
}

BYTE c64dtv_dma_read(WORD addr)
{
    if (addr == 0x1f) {
        return dma_busy;
        /* the default return value is 0x00 too but I have seen some strangeness
           here.  I've seen something that looks like DMAed data. - tlr */
    }
    return 0x00;
}

void c64dtv_dma_store(WORD addr, BYTE value)
{
    /* Store first, then check whether DMA access has been
       requested, perform if necessary. */
    c64dtvmem_dma[addr] = value;

    dma_on_irq = GET_REG8(0x1f) & 0x70;

    /* Clear DMA IRQ */
    if ((GET_REG8(0x1d) & 0x01) && (dma_busy == 2)) {
#ifdef DEBUG
        if (dma_log_enabled) {
            log_message(c64dtvdma_log, "Clear IRQ");
        }
#endif
        dma_busy &= 0xfd;
        c64dtvmem_dma[0x1f] = 0;
        maincpu_set_irq(c64dtv_dma_int_num, 0);
        dma_irq = 0;
        /* reset clear IRQ strobe bit */
        c64dtvmem_dma[0x1d] &= 0xfe;
    }

    if (dma_on_irq && (dma_busy == 0)) {
        dma_busy = 1;
#ifdef DEBUG
        if (dma_log_enabled) {
            log_message(c64dtvdma_log, "Scheduled DMA (%02x).", dma_on_irq);
        }
#endif
        return;
    }

    /* Force DMA start */
    if (GET_REG8(0x1f) & 0x01) {
        c64dtvdma_trigger_dma();
        /* reset force start strobe bit */
        c64dtvmem_dma[0x1f] &= 0xfe;
    }
}


void c64dtvdma_perform_dma(void)
{
    /* set maincpu_rmw_flag to 0 during DMA */
    int dma_maincpu_rmw = maincpu_rmw_flag;
    maincpu_rmw_flag = 0;
    perform_dma_cycle();
    maincpu_rmw_flag = dma_maincpu_rmw;

#ifdef DEBUG
    if (dma_log_enabled && (dma_state == DMA_WRITE)) {
        log_message(c64dtvdma_log, "%s from %x (%s) to %x (%s), %d to go", GET_REG8(0x1f) & 0x02 ? "Swapped" : "Copied", dma_source_off, source_memtype == 0 ? "Flash" : "RAM", dma_dest_off, dest_memtype == 0 ? "Flash" : "RAM", dma_count - 1);
    }
#endif

    if (dma_state == DMA_IDLE) {
        c64dtv_dma_done();
    }
}


/* ------------------------------------------------------------------------- */

#ifdef DEBUG
static int set_dma_log(int val, void *param)
{
    dma_log_enabled = val ? 1 : 0;

    return 0;
}
#endif

static const resource_int_t resources_int[] = {
#ifdef DEBUG
    { "DtvDMALog", 0, RES_EVENT_NO, (resource_value_t)0,
      &dma_log_enabled, set_dma_log, NULL },
#endif
    { NULL }
};

int c64dtvdma_resources_init(void)
{
    return resources_register_int(resources_int);
}

void c64dtvdma_resources_shutdown(void)
{
}

static const cmdline_option_t cmdline_options[] =
{
#ifdef DEBUG
    { "-dtvdmalog", SET_RESOURCE, 0,
      NULL, NULL, "DtvDMALog", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_DTV_DMA_LOG,
      NULL, NULL },
    { "+dtvdmalog", SET_RESOURCE, 0,
      NULL, NULL, "DtvDMALog", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_DTV_DMA_LOG,
      NULL, NULL },
#endif
    { NULL }
};

int c64dtvdma_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

#define SNAP_MAJOR 0
#define SNAP_MINOR 0

static log_t c64_snapshot_log = LOG_ERR;

static const char snap_dma_module_name[] = "C64DTVDMA";

int c64dtvdma_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    /* DMA module.  */
    m = snapshot_module_create(s, snap_dma_module_name, SNAP_MAJOR, SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (SMW_BA(m, c64dtvmem_dma, 0x20) < 0
        || SMW_DW(m, dma_source_off) < 0
        || SMW_DW(m, dma_dest_off) < 0
        || SMW_DW(m, dma_busy) < 0
        || SMW_DW(m, dma_irq) < 0
        || SMW_DW(m, dma_on_irq) < 0
        || SMW_DW(m, dma_active) < 0
        || SMW_B(m, dma_data) < 0
        || SMW_B(m, dma_data_swap) < 0
        || SMW_DW(m, dma_count) < 0
        || SMW_DW(m, dma_state) < 0
        || SMW_DW(m, source_line_off) < 0
        || SMW_DW(m, dest_line_off) < 0
        || SMW_B(m, source_memtype) < 0
        || SMW_B(m, dest_memtype) < 0) {
        goto fail;
    }

    if (snapshot_module_close(m) < 0) {
        goto fail;
    }
    m = NULL;

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}


int c64dtvdma_snapshot_read_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    int temp_dma_state;

    /* DMA module.  */
    m = snapshot_module_open(s, snap_dma_module_name,
                             &major_version, &minor_version);
    if (m == NULL) {
        return -1;
    }

    if (major_version > SNAP_MAJOR || minor_version > SNAP_MINOR) {
        log_error(c64_snapshot_log,
                  "Snapshot module version (%d.%d) newer than %d.%d.",
                  major_version, minor_version,
                  SNAP_MAJOR, SNAP_MINOR);
        goto fail;
    }

    if (SMR_BA(m, c64dtvmem_dma, 0x20) < 0
        || SMR_DW_INT(m, &dma_source_off) < 0
        || SMR_DW_INT(m, &dma_dest_off) < 0
        || SMR_DW_INT(m, &dma_busy) < 0
        || SMR_DW_INT(m, &dma_irq) < 0
        || SMR_DW_INT(m, &dma_on_irq) < 0
        || SMR_DW_INT(m, &dma_active) < 0
        || SMR_B(m, &dma_data) < 0
        || SMR_B(m, &dma_data_swap) < 0
        || SMR_DW_INT(m, &dma_count) < 0
        || SMR_DW_INT(m, &temp_dma_state) < 0
        || SMR_DW_INT(m, &source_line_off) < 0
        || SMR_DW_INT(m, &dest_line_off) < 0
        || SMR_B(m, &source_memtype) < 0
        || SMR_B(m, &dest_memtype) < 0) {
        goto fail;
    }

    dma_state = temp_dma_state;

    if (snapshot_module_close(m) < 0) {
        goto fail;
    }
    m = NULL;

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}
