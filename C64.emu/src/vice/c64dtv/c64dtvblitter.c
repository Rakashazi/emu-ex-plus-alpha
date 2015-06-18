/*
 * c64dtvblitter.c - C64DTV blitter
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

#include <string.h>

#include "c64mem.h"
#include "c64dtvmem.h"
#include "c64dtvflash.h"
#include "c64dtvblitter.h"
#include "c64dtvdma.h"
#include "vicii-mem.h"
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
static log_t c64dtvblitter_log = LOG_ERR;
#endif

static unsigned int c64dtv_blitter_int_num;

/* I/O of the blitter engine ($D3XX) */
BYTE c64dtvmem_blitter[0x20];

int blitter_active;
int blitter_on_irq;

static int blit_sourceA_off;
static int blit_sourceB_off;
static int blit_dest_off;
static int blitter_busy;
static int blitter_irq;

#ifdef DEBUG
static int blitter_log_enabled = 0;
#endif

static BYTE srca_data[4];
static int srca_data_offs;
static int srca_fetched;
static BYTE srcb_data[4];
static int srcb_data_offs;
static BYTE sourceA, sourceB;
static int blitter_count;
static enum { BLITTER_IDLE, BLITTER_READ_A, BLITTER_READ_B, BLITTER_WRITE } blitter_state;
static int sourceA_line_off;
static int sourceB_line_off;
static int dest_line_off;
static BYTE lastA;


/* resource stuff */
static int dtvrevision;
static int have_blitter_bug;


#define GET_REG24(a) ((c64dtvmem_blitter[a + 2] << 16) | (c64dtvmem_blitter[a + 1] << 8) | c64dtvmem_blitter[a])
#define GET_REG16(a) ((c64dtvmem_blitter[a + 1] << 8) | c64dtvmem_blitter[a])
#define GET_REG8(a) (c64dtvmem_blitter[a])


/* shadow register fields */
static int reg03_sourceA_modulo;
static int reg05_sourceA_line_length;
static int reg07_sourceA_step;
static int reg0b_sourceB_modulo;
static int reg0d_sourceB_line_length;
static int reg0f_sourceB_step;
static int reg13_dest_modulo;
static int reg15_dest_line_length;
static int reg17_dest_step;
static int reg1a_sourceA_direction;
static int reg1a_sourceB_direction;
static int reg1a_dest_direction;
static int reg1b_force_sourceB_zero;
static int reg1b_write_if_sourceA_zero;
static int reg1b_write_if_sourceA_nonzero;
static int reg1e_sourceA_right_shift;
static int reg1e_mintermALU;


/* ------------------------------------------------------------------------- */

void c64dtvblitter_init(void)
{
#ifdef DEBUG
    if (c64dtvblitter_log == LOG_ERR) {
        c64dtvblitter_log = log_open("C64DTVBLITTER");
    }
#endif

    /* init Blitter IRQ */
    c64dtv_blitter_int_num = interrupt_cpu_status_int_new(maincpu_int_status, "C64DTVBLITTER");
}


void c64dtvblitter_shutdown(void)
{
}

void c64dtvblitter_reset(void)
{
    int i;
#ifdef DEBUG
    if (blitter_log_enabled) {
        log_message(c64dtvblitter_log, "reset");
    }
#endif

    /* TODO move register file initialization somewhere else? */
    for (i = 0; i < 0x20; ++i) {
        c64dtvmem_blitter[i] = 0;
    }

    c64dtvmem_blitter[0x07] = 0x10;
    c64dtvmem_blitter[0x0f] = 0x10;
    c64dtvmem_blitter[0x17] = 0x10;

    /* reset internal states */
    blit_sourceA_off = 0;
    blit_sourceB_off = 0;
    blit_dest_off = 0;
    blitter_busy = 0;
    blitter_irq = 0;
    blitter_on_irq = 0;
    blitter_active = 0;

    blitter_count = 0;
    blitter_state = BLITTER_IDLE;
    srca_data_offs = -1;
    srcb_data_offs = -1;
    srca_fetched = 0;
    sourceA = 0;
    lastA = 0;
    sourceB = 0;
    sourceA_line_off = 0;
    sourceB_line_off = 0;
    dest_line_off = 0;
}

/* ------------------------------------------------------------------------- */
/* blitter transfer state machine */

static inline int do_blitter_read_a(void)
{
    int was_read = 0;
    int offs = (blit_sourceA_off >> 4) & 0x1ffffc;
    int loffs = (blit_sourceA_off >> 4) & 0x000003;

    srca_fetched = 0;
    if (offs != srca_data_offs) {
        memcpy(srca_data, &mem_ram[offs], 4);
        srca_data_offs = offs;
        srca_fetched = 1;
        was_read = 1;
    }
    sourceA = srca_data[loffs];
    return was_read;
}

static inline int do_blitter_read_b(void)
{
    int was_read = 0;
    int offs = (blit_sourceB_off >> 4) & 0x1ffffc;
    int loffs = (blit_sourceB_off >> 4) & 0x000003;

    if (reg1b_force_sourceB_zero) {
        sourceB = 0;
        return 0;
    }

    if (offs != srcb_data_offs) {
        memcpy(srcb_data, &mem_ram[offs], 4);
        srcb_data_offs = offs;
        was_read = 1;
    }
    sourceB = srcb_data[loffs];
    return was_read;
}


static inline int do_blitter_write(void)
{
    int was_write = 0;
    int offs = (blit_dest_off >> 4) & 0x1fffff;

    if ((reg1b_write_if_sourceA_zero && sourceA == 0) ||
        (reg1b_write_if_sourceA_nonzero && sourceA != 0) ||
        (have_blitter_bug && srca_fetched)) {
        BYTE dest;
        BYTE lastA_tmp = sourceA;
        sourceA >>= reg1e_sourceA_right_shift;
        sourceA |= lastA << (8 - reg1e_sourceA_right_shift);
        lastA = lastA_tmp;

        dest = 0;
        switch (reg1e_mintermALU) {
            case 0: dest = sourceA & sourceB; break;
            case 1: dest = ~(sourceA & sourceB); break;
            case 2: dest = ~(sourceA | sourceB); break;
            case 3: dest = sourceA | sourceB; break;
            case 4: dest = sourceA ^ sourceB; break;
            case 5: dest = ~(sourceA ^ sourceB); break;
            case 6: dest = sourceA + sourceB; break;
            case 7: dest = sourceA - sourceB; break;
            default:
                break;
        }
        mem_ram[offs] = dest;
        was_write = 1;
    }
#ifdef DEBUG
    if (blitter_log_enabled) {
        log_message(c64dtvblitter_log, "Blitter: %s %x.%x/%x.%x to %x.%x, %d to go, minterm %d", was_write ? "transferred" : "skipped", blit_sourceA_off >> 4, blit_sourceA_off & 15, blit_sourceB_off >> 4, blit_sourceB_off & 15, blit_dest_off >> 4, blit_dest_off & 15, blitter_count - 1, reg1e_mintermALU);
    }
#endif
    return was_write;
}

static inline void update_counters(void)
{
    if (sourceA_line_off >= reg05_sourceA_line_length) {
        lastA = 0;
        sourceA_line_off = 0;
        blit_sourceA_off = ((blit_sourceA_off >> 4) + reg03_sourceA_modulo * reg1a_sourceA_direction) << 4;
    } else {
        sourceA_line_off++;
        blit_sourceA_off += reg07_sourceA_step * reg1a_sourceA_direction;
    }
    if (sourceB_line_off >= reg0d_sourceB_line_length) {
        sourceB_line_off = 0;
        blit_sourceB_off = ((blit_sourceB_off >> 4) + reg0b_sourceB_modulo * reg1a_sourceB_direction) << 4;
    } else {
        sourceB_line_off++;
        blit_sourceB_off += reg0f_sourceB_step * reg1a_sourceB_direction;
    }
    if (dest_line_off >= reg15_dest_line_length) {
        dest_line_off = 0;
        blit_dest_off = ((blit_dest_off >> 4) + reg13_dest_modulo * reg1a_dest_direction) << 4;
    } else {
        dest_line_off++;
        blit_dest_off += reg17_dest_step * reg1a_dest_direction;
    }
}

/* 32 MHz processing clock */
#define SUBCYCLES 32
static inline void perform_blitter_cycle(void)
{
    int subcycle = 0;
    while (subcycle < SUBCYCLES) {
        switch (blitter_state) {
            case BLITTER_IDLE:
                subcycle += SUBCYCLES;
                break;
            case BLITTER_READ_A:
                if (blitter_count == 0) {
                    blitter_state = BLITTER_IDLE;
                    break;
                }

                if (do_blitter_read_a()) {
                    subcycle += SUBCYCLES;
                }
                blitter_state = BLITTER_READ_B;
                break;
            case BLITTER_READ_B:
                if (do_blitter_read_b()) {
                    subcycle += SUBCYCLES;
                }
                blitter_state = BLITTER_WRITE;
                break;
            case BLITTER_WRITE:
                if (do_blitter_write()) {
                    subcycle += SUBCYCLES;
                } else {
                    subcycle += 1;
                }

                update_counters();
                blitter_count--;

                if (blitter_count == 0) {
                    blitter_state = BLITTER_IDLE;
                } else {
                    blitter_state = BLITTER_READ_A;
                }
                break;
            default:
#ifdef DEBUG
                log_message(c64dtvblitter_log, "invalid state in perform_blitter_cycle()");
#endif
                blitter_state = BLITTER_IDLE;
                break;
        }
    }
}


/* ------------------------------------------------------------------------- */

/* These are the $D3xx Blitter register engine handlers */

void c64dtvblitter_trigger_blitter(void)
{
    if (!blitter_active) {
        int sourceA_continue = GET_REG8(0x1f) & 0x02;
        int sourceB_continue = GET_REG8(0x1f) & 0x04;
        int dest_continue = GET_REG8(0x1f) & 0x08;

        /* last four bits of offsets are fractional */
        if (!sourceA_continue) {
            blit_sourceA_off = GET_REG24(0x00) & 0x3fffff;
            blit_sourceA_off <<= 4;
        }
        if (!sourceB_continue) {
            blit_sourceB_off = GET_REG24(0x08) & 0x3fffff;
            blit_sourceB_off <<= 4;
        }
        if (!dest_continue) {
            blit_dest_off = GET_REG24(0x10) & 0x3fffff;
            blit_dest_off <<= 4;
        }

#ifdef DEBUG
        if (blitter_log_enabled && (sourceA_continue || sourceB_continue || dest_continue)) {
            log_message(c64dtvblitter_log, "sourceA cont %s, sourceB cont %s, dest cont %s", sourceA_continue ? "on" : "off", sourceB_continue ? "on" : "off", dest_continue ? "on" : "off");
        }
#endif

        /* total number of bytes to transfer */
        blitter_count = GET_REG16(0x18);

        /* initialize state variables */
        sourceA_line_off = 0;
        sourceB_line_off = 0;
        dest_line_off = 0;
        lastA = 0;
        srca_data_offs = -1;
        srcb_data_offs = -1;

        blitter_state = BLITTER_READ_A;

        if (GET_REG8(0x1a) & 0x80) {
            blitter_irq = 1;
        } else {
            blitter_irq = 0;
        }

        blitter_busy = 1;
        blitter_active = 1;
    }
}

static inline void c64dtv_blitter_done(void)
{
#ifdef DEBUG
    if (blitter_log_enabled) {
        log_message(c64dtvblitter_log, "IRQ/Done");
    }
#endif
    if (blitter_irq) {
        maincpu_set_irq(c64dtv_blitter_int_num, 1);
        blitter_busy = 2;
    }
    blitter_busy &= 0xfe;
    blitter_active = 0;

    /* Scheduled DMA */
    if (dma_on_irq & 0x20) {
        c64dtvdma_trigger_dma();
    }
}


BYTE c64dtv_blitter_read(WORD addr)
{
    if (addr == 0x1f) {
        return blitter_busy;
        /* the default return value is 0x00 too but I have seen some strangeness
           here.  I've seen something that looks like DMAed data. - tlr */
    }
    return 0x00;
}

void c64dtv_blitter_store(WORD addr, BYTE value)
{
    /* Store first, then check whether DMA access has been requested,
       perform if necessary. */
    c64dtvmem_blitter[addr] = value;

    switch (addr) {
        case 0x03:
        case 0x04:
            reg03_sourceA_modulo = GET_REG16(0x03);
            break;
        case 0x05:
        case 0x06:
            reg05_sourceA_line_length = GET_REG16(0x05);
            break;
        case 0x07:
            reg07_sourceA_step = GET_REG8(0x07);
            break;
        case 0x0b:
        case 0x0c:
            reg0b_sourceB_modulo = GET_REG16(0x0b);
            break;
        case 0x0d:
        case 0x0e:
            reg0d_sourceB_line_length = GET_REG16(0x0d);
            break;
        case 0x0f:
            reg0f_sourceB_step = GET_REG8(0x0f);
            break;
        case 0x13:
        case 0x14:
            reg13_dest_modulo = GET_REG16(0x13);
            break;
        case 0x15:
        case 0x16:
            reg15_dest_line_length = GET_REG16(0x15);
            break;
        case 0x17:
            reg17_dest_step = GET_REG8(0x17);
            break;
        case 0x1a:
            reg1a_sourceA_direction = (GET_REG8(0x1a) & 0x02) ? +1 : -1;
            reg1a_sourceB_direction = (GET_REG8(0x1a) & 0x04) ? +1 : -1;
            reg1a_dest_direction = (GET_REG8(0x1a) & 0x08) ? +1 : -1;
            break;
        case 0x1b:
            reg1b_force_sourceB_zero = GET_REG8(0x1b) & 0x01;
            reg1b_write_if_sourceA_zero = GET_REG8(0x1b) & 0x02;
            reg1b_write_if_sourceA_nonzero = GET_REG8(0x1b) & 0x04;

            /* zero and nonzero == 0 seems to do exactly the same as both ==1 */
            if (!(reg1b_write_if_sourceA_zero || reg1b_write_if_sourceA_nonzero)) {
                reg1b_write_if_sourceA_zero = reg1b_write_if_sourceA_nonzero = 1;
            }
            break;
        case 0x1e:
            reg1e_sourceA_right_shift = GET_REG8(0x1e) & 0x07;
            reg1e_mintermALU = (GET_REG8(0x1e) >> 3) & 0x07;
            break;
        default:
            break;
    }

    /* Blitter code */
    blitter_on_irq = GET_REG8(0x1a) & 0x70;

    /* Clear Blitter IRQ */
    if ((GET_REG8(0x1f) & 0x01) && (blitter_busy == 2)) {
#ifdef DEBUG
        if (blitter_log_enabled) {
            log_message(c64dtvblitter_log, "Clear IRQ (%i)", blitter_busy);
        }
#endif
        blitter_busy &= 0xfd;
        maincpu_set_irq(c64dtv_blitter_int_num, 0);
        blitter_irq = 0;
        /* reset clear IRQ strobe bit */
        c64dtvmem_blitter[0x1f] &= 0xfe;
    }

    if (blitter_on_irq && (blitter_busy == 0)) {
        blitter_busy = 1;
#ifdef DEBUG
        if (blitter_log_enabled) {
            log_message(c64dtvblitter_log, "Scheduled Blitter (%02x)", blitter_on_irq);
        }
#endif
        return;
    }

    /* Force Blitter start */
    if (GET_REG8(0x1a) & 0x01) {
        c64dtvblitter_trigger_blitter();
        /* reset force start strobe bit */
        c64dtvmem_blitter[0x1a] &= 0xfe;
    }
}


void c64dtvblitter_perform_blitter(void)
{
    perform_blitter_cycle();

    if (blitter_state == BLITTER_IDLE) {
        c64dtv_blitter_done();
    }
}

/* ------------------------------------------------------------------------- */
static int set_dtvrevision(int val, void *param)
{
    switch (val) {
        default:
        case 3:
            dtvrevision = 3;
            break;
        case 2:
            dtvrevision = 2;
            break;
    }
    have_blitter_bug = (dtvrevision == 2) ? 1 : 0;
    return 1;
}

#ifdef DEBUG
static int set_blitter_log(int val, void *param)
{
    blitter_log_enabled = val ? 1 : 0;
    return 0;
}
#endif

static const resource_int_t resources_int[] = {
    { "DtvRevision", 3, RES_EVENT_SAME, NULL,
      &dtvrevision, set_dtvrevision, NULL },
#ifdef DEBUG
    { "DtvBlitterLog", 0, RES_EVENT_NO, (resource_value_t)0,
      &blitter_log_enabled, set_blitter_log, NULL },
#endif
    { NULL }
};

int c64dtvblitter_resources_init(void)
{
    return resources_register_int(resources_int);
}

void c64dtvblitter_resources_shutdown(void)
{
}

static const cmdline_option_t cmdline_options[] =
{
    { "-dtvrev", SET_RESOURCE, 1,
      NULL, NULL, "DtvRevision", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_REVISION, IDCLS_SPECIFY_DTV_REVISION,
      NULL, NULL },
#ifdef DEBUG
    { "-dtvblitterlog", SET_RESOURCE, 0,
      NULL, NULL, "DtvBlitterLog", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_DTV_BLITTER_LOG,
      NULL, NULL },
    { "+dtvblitterlog", SET_RESOURCE, 0,
      NULL, NULL, "DtvBlitterLog", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_DTV_BLITTER_LOG,
      NULL, NULL },
#endif
    { NULL }
};

int c64dtvblitter_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

#define SNAP_MAJOR 0
#define SNAP_MINOR 0

static log_t c64_snapshot_log = LOG_ERR;

static const char snap_blitter_module_name[] = "C64DTVBLITTER";

int c64dtvblitter_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    /* Blitter module.  */
    m = snapshot_module_create(s, snap_blitter_module_name, SNAP_MAJOR, SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (SMW_BA(m, c64dtvmem_blitter, 0x20) < 0
        || SMW_DW(m, blit_sourceA_off) < 0
        || SMW_DW(m, blit_sourceB_off) < 0
        || SMW_DW(m, blit_dest_off) < 0
        || SMW_DW(m, blitter_busy) < 0
        || SMW_DW(m, blitter_irq) < 0
        || SMW_DW(m, blitter_on_irq) < 0
        || SMW_DW(m, blitter_active) < 0
        || SMW_BA(m, srca_data, 4) < 0
        || SMW_DW(m, srca_data_offs) < 0
        || SMW_DW(m, srca_fetched) < 0
        || SMW_BA(m, srcb_data, 4) < 0
        || SMW_DW(m, srcb_data_offs) < 0
        || SMW_B(m, sourceA) < 0
        || SMW_B(m, sourceB) < 0
        || SMW_DW(m, blitter_count) < 0
        || SMW_DW(m, blitter_state) < 0
        || SMW_DW(m, sourceA_line_off) < 0
        || SMW_DW(m, sourceB_line_off) < 0
        || SMW_DW(m, dest_line_off) < 0
        || SMW_B(m, lastA) < 0) {
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


int c64dtvblitter_snapshot_read_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    int temp_blitter_state, i;

    /* Blitter module.  */
    m = snapshot_module_open(s, snap_blitter_module_name,
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

    if (SMR_BA(m, c64dtvmem_blitter, 0x20) < 0
        || SMR_DW_INT(m, &blit_sourceA_off) < 0
        || SMR_DW_INT(m, &blit_sourceB_off) < 0
        || SMR_DW_INT(m, &blit_dest_off) < 0
        || SMR_DW_INT(m, &blitter_busy) < 0
        || SMR_DW_INT(m, &blitter_irq) < 0
        || SMR_DW_INT(m, &blitter_on_irq) < 0
        || SMR_DW_INT(m, &blitter_active) < 0
        || SMR_BA(m, srca_data, 4) < 0
        || SMR_DW_INT(m, &srca_data_offs) < 0
        || SMR_DW_INT(m, &srca_fetched) < 0
        || SMR_BA(m, srcb_data, 4) < 0
        || SMR_DW_INT(m, &srcb_data_offs) < 0
        || SMR_B(m, &sourceA) < 0
        || SMR_B(m, &sourceB) < 0
        || SMR_DW_INT(m, &blitter_count) < 0
        || SMR_DW_INT(m, &temp_blitter_state) < 0
        || SMR_DW_INT(m, &sourceA_line_off) < 0
        || SMR_DW_INT(m, &sourceB_line_off) < 0
        || SMR_DW_INT(m, &dest_line_off) < 0
        || SMR_B(m, &lastA) < 0) {
        goto fail;
    }

    blitter_state = temp_blitter_state;

    for (i = 0; i < 0x20; ++i) {
        c64dtv_blitter_store((WORD)i, c64dtvmem_blitter[i]);
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
