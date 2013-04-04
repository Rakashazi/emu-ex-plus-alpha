/*
 * vicii-fetch.c - Phi2 data fetch for the MOS 6569 (VIC-II) emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *
 * DTV sections written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Daniel Kahlin <daniel@kahlin.net>
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

#include "alarm.h"
#include "debug.h"
#include "c64cart.h"
#include "c64cartmem.h"
#include "dma.h"
#include "log.h"
#include "maincpu.h"
#include "mem.h"
#include "raster-changes.h"
#include "raster-sprite-status.h"
#include "raster-sprite.h"
#include "raster.h"
#include "types.h"
#include "vicii-fetch.h"
#include "vicii-irq.h"
#include "vicii-sprites.h"
#include "viciitypes.h"


/* Emulate a matrix line fetch, `num' bytes starting from `offs'.  This takes
   care of the 10-bit counter wraparound.  */
void vicii_fetch_matrix(int offs, int num, int num_0xff, int cycle)
{
    int start_char;
    int c;
    BYTE *colorram = NULL;

    /*log_debug("OFF %02i NUM %02i NFF %02i",offs,num,num_0xff);*/

    if (vicii.viciidtv) {
        num_0xff = 0;
        colorram = vicii.color_ram_ptr;
    } else {
        colorram = mem_color_ram_vicii;
    }

    if (num_0xff > 0) {
        if (num <= num_0xff) {
            memset(vicii.vbuf + offs, 0xff, num);
            memset(vicii.cbuf + offs, vicii.ram_base_phi2[reg_pc] & 0xf, num);
            /* FIXME: Crunch table in Multiplexer part of Krestage */
            vicii.background_color_source = 0xff;
        } else {
            memset(vicii.vbuf + offs, 0xff, num_0xff);
            memset(vicii.cbuf + offs, vicii.ram_base_phi2[reg_pc] & 0xf, num_0xff);
        }
    }

    if (num > num_0xff) {
        offs += num_0xff;
        num -= num_0xff;

        /* Matrix fetches are done during Phi2, the fabulous "bad lines" */
        start_char = (vicii.mem_counter + offs) & 0x3ff;
        c = 0x3ff - start_char + 1;

        if (c >= num) {
            memcpy(vicii.vbuf + offs, vicii.screen_base_phi2 + start_char, num);
            if (!vicii.colorfetch_disable) {
                memcpy(vicii.cbuf + offs, colorram + start_char, num);
            }
        } else {
            memcpy(vicii.vbuf + offs, vicii.screen_base_phi2 + start_char, c);
            memcpy(vicii.vbuf + offs + c, vicii.screen_base_phi2, num - c);

            if (!vicii.colorfetch_disable) {
                memcpy(vicii.cbuf + offs, colorram + start_char, c);
            }

            if (!vicii.colorfetch_disable) {
                memcpy(vicii.cbuf + offs + c, colorram, num - c);
            }
        }
        vicii.background_color_source = vicii.vbuf[VICII_SCREEN_TEXTCOLS - 1 /*- vicii.buf_offset*/];
    }

    /* Set correct background color in in the xsmooth area.
       As this only affects the next line, the xsmooth color is immediately
       set if the right border is opened.  */
    if (offs + num >= VICII_SCREEN_TEXTCOLS) {
        switch (vicii.get_background_from_vbuf) {
            case VICII_HIRES_BITMAP_MODE:
                raster_changes_next_line_add_int(
                    &vicii.raster,
                    &vicii.raster.xsmooth_color,
                    vicii.background_color_source & 0x0f);
                break;
            case VICII_EXTENDED_TEXT_MODE:
                raster_changes_next_line_add_int(
                    &vicii.raster,
                    &vicii.raster.xsmooth_color,
                    vicii.regs[0x21 + (vicii.background_color_source >> 6)]);
                break;
        }
    }
}

/* If we are on a bad line, do the DMA.  Return nonzero if cycles have been
   stolen.  */
inline static int do_matrix_fetch(CLOCK sub)
{
    if (!vicii.memory_fetch_done) {
        raster_t *raster;

        raster = &vicii.raster;

        vicii.memory_fetch_done = 1;
        vicii.mem_counter = vicii.memptr;

        if ((raster->current_line & 7) == (unsigned int)raster->ysmooth
            && vicii.allow_bad_lines
            && raster->current_line >= vicii.first_dma_line
            && raster->current_line <= vicii.last_dma_line) {
            vicii_fetch_matrix(0, VICII_SCREEN_TEXTCOLS, 0, VICII_FETCH_CYCLE);

            raster->draw_idle_state = 0;
            raster->ycounter = 0;

            vicii.idle_state = 0;
            vicii.idle_data_location = IDLE_NONE;
            vicii.ycounter_reset_checked = 1;
            vicii.memory_fetch_done = 2;

            if ((vicii.fastmode == 0) && !vicii.badline_disable && !vicii.colorfetch_disable) {
                dma_maincpu_steal_cycles(vicii.fetch_clk,
                                         VICII_SCREEN_TEXTCOLS + 3 - sub, sub);
            } else if (vicii.viciidtv && !vicii.colorfetch_disable) {
                /* Steal cycles from DMA/Blitter */
                dtvclockneg += VICII_SCREEN_TEXTCOLS + 3;
            }
            vicii.bad_line = 1;
            return 1;
        }
    }

    return 0;
}

inline static int handle_fetch_matrix(long offset, CLOCK sub,
                                      CLOCK *write_offset)
{
    raster_t *raster;
    raster_sprite_status_t *sprite_status;

    *write_offset = 0;

    raster = &vicii.raster;
    sprite_status = raster->sprite_status;

    if (sprite_status->visible_msk == 0 && sprite_status->dma_msk == 0) {
        do_matrix_fetch(sub);

        /* As sprites are all turned off, there is no need for a sprite DMA
           check; next time we will VICII_FETCH_MATRIX again.  This works
           because a VICII_CHECK_SPRITE_DMA is forced in `vic_store()'
           whenever the mask becomes nonzero.  */

        /* This makes sure we only create VICII_FETCH_MATRIX events in the bad
           line range.  These checks are (a little) redundant for safety.  */
        if (raster->current_line < vicii.first_dma_line) {
            vicii.fetch_clk += ((vicii.first_dma_line - raster->current_line) * vicii.cycles_per_line);
        } else {
            if (raster->current_line >= vicii.last_dma_line) {
                vicii.fetch_clk += ((vicii.screen_height
                                     - raster->current_line
                                     + vicii.first_dma_line)
                                    * vicii.cycles_per_line);
            } else {
                vicii.fetch_clk += vicii.cycles_per_line;
            }
        }

        alarm_set(vicii.raster_fetch_alarm, vicii.fetch_clk);
        return 1;
    } else {
        int fetch_done;

        fetch_done = do_matrix_fetch(sub);

        /* Sprites might be turned on, check for sprite DMA next
           time.  */
        vicii.fetch_idx = VICII_CHECK_SPRITE_DMA;

        /* Calculate time for next event.  */
        vicii.fetch_clk = VICII_LINE_START_CLK(maincpu_clk)
                          + vicii.sprite_fetch_cycle;

        if (vicii.fetch_clk > maincpu_clk || offset == 0) {
            /* Prepare the next fetch event.  */
            alarm_set(vicii.raster_fetch_alarm, vicii.fetch_clk);
            return 1;
        }

        if (fetch_done && sub == 0) {
            *write_offset = VICII_SCREEN_TEXTCOLS + 3;
        }
    }

    return 0;
}

/*-----------------------------------------------------------------------*/

inline static void swap_sprite_data_buffers(void)
{
    DWORD *tmp;
    raster_sprite_status_t *sprite_status;

    /* Swap sprite data buffers.  */
    sprite_status = vicii.raster.sprite_status;
    tmp = sprite_status->sprite_data;
    sprite_status->sprite_data = sprite_status->new_sprite_data;
    sprite_status->new_sprite_data = tmp;
}

/* Enable DMA for sprite `num'.  */
inline static void turn_sprite_dma_on(unsigned int num)
{
    raster_sprite_status_t *sprite_status;
    raster_sprite_t *sprite;

    sprite_status = vicii.raster.sprite_status;
    sprite = sprite_status->sprites + num;

    sprite_status->new_dma_msk |= 1 << num;
    sprite->dma_flag = 1;
    sprite->memptr = 0;
    sprite->exp_flag = sprite->y_expanded ? 0 : 1;
    sprite->memptr_inc = sprite->exp_flag ? 3 : 0;
}

inline static void check_sprite_dma(void)
{
    raster_sprite_status_t *sprite_status;
    int i, b;

    sprite_status = vicii.raster.sprite_status;

    if (!sprite_status->visible_msk && !sprite_status->dma_msk) {
        return;
    }

    sprite_status->new_dma_msk = sprite_status->dma_msk;

    for (i = 0, b = 1; i < VICII_NUM_SPRITES; i++, b <<= 1) {
        raster_sprite_t *sprite;

        sprite = sprite_status->sprites + i;

        if ((sprite_status->visible_msk & b)
            && sprite->y == ((int)vicii.raster.current_line & 0xff)
            && !sprite->dma_flag) {
            turn_sprite_dma_on(i);
        } else if (sprite->dma_flag) {
            sprite->memptr = (sprite->memptr + sprite->memptr_inc) & 0x3f;

            if (sprite->y_expanded) {
                sprite->exp_flag = !sprite->exp_flag;
            }

            sprite->memptr_inc = sprite->exp_flag ? 3 : 0;

            if (sprite->memptr == 63) {
                sprite->dma_flag = 0;
                sprite_status->new_dma_msk &= ~b;

                if ((sprite_status->visible_msk & b)
                    && sprite->y == ((int)vicii.raster.current_line & 0xff)) {
                    turn_sprite_dma_on(i);
                }
            }
        }
    }
}

inline static int handle_check_sprite_dma(long offset, CLOCK sub)
{
    swap_sprite_data_buffers();

    check_sprite_dma();

    if (vicii.raster.sprite_status->dma_msk || vicii.raster.sprite_status->new_dma_msk) {
        vicii_sprites_reset_sprline();
    }

    /* FIXME?  Slow!  */
    vicii.sprite_fetch_clk = (VICII_LINE_START_CLK(maincpu_clk) + vicii.sprite_fetch_cycle);
    vicii.sprite_fetch_msk = vicii.raster.sprite_status->new_dma_msk;

    if (vicii_sprites_fetch_table[vicii.sprite_fetch_msk][0].cycle == -1) {
        if (vicii.raster.current_line >= vicii.first_dma_line - 1
            && vicii.raster.current_line <= vicii.last_dma_line + 1) {
            vicii.fetch_idx = VICII_FETCH_MATRIX;
            vicii.fetch_clk = (vicii.sprite_fetch_clk
                               - vicii.sprite_fetch_cycle
                               + VICII_FETCH_CYCLE
                               + vicii.cycles_per_line);
        } else {
            vicii.fetch_idx = VICII_CHECK_SPRITE_DMA;
            vicii.fetch_clk = (vicii.sprite_fetch_clk + vicii.cycles_per_line);
        }
    } else {
        /* Next time, fetch sprite data.  */
        vicii.fetch_idx = VICII_FETCH_SPRITE;
        vicii.sprite_fetch_idx = 0;
        vicii.fetch_clk = (vicii.sprite_fetch_clk + vicii_sprites_fetch_table[vicii.sprite_fetch_msk][0].cycle);
    }

    /*log_debug("HCSD SCLK %i FCLK %i CLK %i OFFSET %li SUB %i",
                vicii.store_clk, vicii.fetch_clk, clk, offset, sub);*/

    if (vicii.store_clk != CLOCK_MAX) {
        if (vicii.store_clk + offset - 3 < vicii.fetch_clk) {
            vicii.ram_base_phi2[vicii.store_addr] = vicii.store_value;
        }
        vicii.store_clk = CLOCK_MAX;
    }

    vicii.num_idle_3fff_old = vicii.num_idle_3fff;
    if (vicii.num_idle_3fff > 0) {
        memcpy(vicii.idle_3fff_old, vicii.idle_3fff,
               sizeof(idle_3fff_t) * vicii.num_idle_3fff);
    }
    vicii.num_idle_3fff = 0;

    if (vicii.fetch_clk > maincpu_clk || offset == 0) {
        alarm_set(vicii.raster_fetch_alarm, vicii.fetch_clk);
        return 1;
    }

    return 0;
}

/*-----------------------------------------------------------------------*/

inline static int handle_fetch_sprite(long offset, CLOCK sub,
                                      CLOCK *write_offset)
{
    const vicii_sprites_fetch_t *sf;
    unsigned int i;
    int next_cycle, num_cycles;
    raster_sprite_status_t *sprite_status;
    BYTE *bank_phi1, *bank_phi2, *spr_base;

    sf = &vicii_sprites_fetch_table[vicii.sprite_fetch_msk][vicii.sprite_fetch_idx];

    sprite_status = vicii.raster.sprite_status;
    /* FIXME: the 3 byte sprite data is instead taken during a Ph1/Ph2/Ph1
       sequence. This is of minor interest, though, only for CBM-II... */
    bank_phi1 = vicii.ram_base_phi1 + vicii.vbank_phi1;
    bank_phi2 = vicii.ram_base_phi2 + vicii.vbank_phi2;
    spr_base = vicii.screen_base_phi1 + 0x3f8 + sf->first;
    if (vicii.viciidtv) {
        spr_base += (vicii.regs[0x4d] << 16);
    }

    /* Fetch sprite data.  */
    for (i = sf->first; i <= sf->last; i++, spr_base++) {
        if (vicii.sprite_fetch_msk & (1 << i)) {
            BYTE *src_phi1, *src_phi2;
            BYTE *dest;
            int my_memptr;

#ifdef DEBUG
            if (debug.maincpu_traceflg) {
                log_debug("SDMA %i", i);
            }
#endif

            src_phi1 = bank_phi1 + (*spr_base << 6);
            src_phi2 = bank_phi2 + (*spr_base << 6);
            my_memptr = sprite_status->sprites[i].memptr;
            dest = (BYTE *)(sprite_status->new_sprite_data + i);

            if (export.ultimax_phi1) {
                /* phi1 fetch from expansion port in ultimax mode */
#if 0
                if (*spr_base >= 0xc0) {
                    /* src_phi1 = (romh_banks + 0x1000 + (romh_bank << 13)
                               + ((*spr_base - 0xc0) << 6)); */
                    src_phi1 = ultimax_romh_phi1_ptr((WORD)(0x1000 + ((*spr_base - 0xc0) << 6)));
                }
#endif
                BYTE *ptr;
                if ((ptr = ultimax_romh_phi1_ptr((WORD)(0x1000 + ((*spr_base - 0xc0) << 6))))) {
                    if (*spr_base >= 0xc0) {
                        src_phi1 = ptr;
                    }
                } else {
                    goto phi1noultimax;
                }
            } else {
phi1noultimax:
                if (((vicii.vbank_phi1 + (*spr_base << 6)) & vicii.vaddr_chargen_mask_phi1) == vicii.vaddr_chargen_value_phi1) {
                    src_phi1 = mem_chargen_rom_ptr + ((*spr_base & 0x3f) << 6);
                }
            }

            if (export.ultimax_phi2) {
                /* phi2 fetch from expansion port in ultimax mode */
#if 0
                if (*spr_base >= 0xc0) {
                    /* src_phi2 = (romh_banks + 0x1000 + (romh_bank << 13)
                               + ((*spr_base - 0xc0) << 6)); */
                    src_phi2 = ultimax_romh_phi2_ptr((WORD)(0x1000 + ((*spr_base - 0xc0) << 6)));
                }
#endif
                BYTE *ptr;
                if ((ptr = ultimax_romh_phi2_ptr((WORD)(0x1000 + ((*spr_base - 0xc0) << 6))))) {
                    if (*spr_base >= 0xc0) {
                        src_phi2 = ptr;
                    }
                } else {
                    goto phi2noultimax;
                }
            } else {
phi2noultimax:
                if (((vicii.vbank_phi2 + (*spr_base << 6)) & vicii.vaddr_chargen_mask_phi2) == vicii.vaddr_chargen_value_phi2) {
                    src_phi2 = mem_chargen_rom_ptr + ((*spr_base & 0x3f) << 6);
                }
            }

            if (vicii.viciidtv) {
                src_phi1 += (vicii.regs[0x4d] << 16);
                src_phi2 += (vicii.regs[0x4d] << 16);
            }

            dest[0] = src_phi2[my_memptr];
            dest[1] = src_phi1[++my_memptr & 0x3f];
            dest[2] = src_phi2[++my_memptr & 0x3f];
        }
    }

    num_cycles = sf->num;

    /*log_debug("SF %i VBL %i SUB %i",sf->num,vicii.bad_line,sub);*/

    if ((vicii.fastmode == 0) && !vicii.badline_disable) {
        dma_maincpu_steal_cycles(vicii.fetch_clk, num_cycles - sub, sub);
    } else if (vicii.viciidtv) {
        /* Steal cycles from DMA/Blitter */
        dtvclockneg += num_cycles;
    }

    *write_offset = sub == 0 ? num_cycles : 0;

    next_cycle = (sf + 1)->cycle;
    vicii.sprite_fetch_idx++;

    if (next_cycle == -1) {
        /* Next time, handle bad lines.  */
        if (vicii.raster.current_line >= vicii.first_dma_line - 1
            && vicii.raster.current_line <= vicii.last_dma_line + 1) {
            vicii.fetch_idx = VICII_FETCH_MATRIX;
            vicii.fetch_clk = (vicii.sprite_fetch_clk
                               - vicii.sprite_fetch_cycle
                               + VICII_FETCH_CYCLE
                               + vicii.cycles_per_line);
        } else {
            vicii.fetch_idx = VICII_CHECK_SPRITE_DMA;
            vicii.fetch_clk = (vicii.sprite_fetch_clk + vicii.cycles_per_line);
        }
    } else {
        vicii.fetch_clk = vicii.sprite_fetch_clk + next_cycle;
    }

    if (maincpu_clk >= vicii.draw_clk) {
        vicii_raster_draw_alarm_handler(maincpu_clk - vicii.draw_clk, NULL);
    }

    if (vicii.fetch_clk > maincpu_clk || offset == 0) {
        alarm_set(vicii.raster_fetch_alarm, vicii.fetch_clk);
        return 1;
    }

    if (maincpu_clk >= vicii.raster_irq_clk) {
        vicii_irq_alarm_handler(maincpu_clk - vicii.raster_irq_clk, NULL);
    }

    return 0;
}

/*-----------------------------------------------------------------------*/

/* Handle sprite/matrix fetch events.  FIXME: could be made slightly
   faster.  */
void vicii_fetch_alarm_handler(CLOCK offset, void *data)
{
    CLOCK last_opcode_first_write_clk, last_opcode_last_write_clk;

    /* This kludgy thing is used to emulate the behavior of the 6510 when BA
       goes low.  When BA goes low, every read access stops the processor
       until BA is high again; write accesses happen as usual instead.  */

    if (offset > 0) {
        switch (OPINFO_NUMBER(last_opcode_info)) {
            case 0:
                /* In BRK, IRQ and NMI the 3rd, 4th and 5th cycles are write
                   accesses, while the 1st, 2nd, 6th and 7th are read accesses.  */
                last_opcode_first_write_clk = maincpu_clk - 5;
                last_opcode_last_write_clk = maincpu_clk - 3;
                break;

            case 0x20:
                /* In JSR, the 4th and 5th cycles are write accesses, while the
                   1st, 2nd, 3rd and 6th are read accesses.  */
                last_opcode_first_write_clk = maincpu_clk - 3;
                last_opcode_last_write_clk = maincpu_clk - 2;
                break;

            default:
                /* In all the other opcodes, all the write accesses are the last
                   ones.  */
                if (maincpu_num_write_cycles() != 0) {
                    last_opcode_last_write_clk = maincpu_clk - 1;
                    last_opcode_first_write_clk = maincpu_clk
                                                  - maincpu_num_write_cycles();
                } else {
                    last_opcode_first_write_clk = (CLOCK)0;
                    last_opcode_last_write_clk = last_opcode_first_write_clk;
                }
                break;
        }
    } else { /* offset <= 0, i.e. offset == 0 */
        /* If we are called with no offset, we don't have to care about write
           accesses.  */
        last_opcode_first_write_clk = last_opcode_last_write_clk = 0;
    }

    while (1) {
        CLOCK sub;
        CLOCK write_offset;
        int leave;

        if (vicii.fetch_clk < last_opcode_first_write_clk || vicii.fetch_clk > last_opcode_last_write_clk) {
            sub = 0;
        } else {
            sub = last_opcode_last_write_clk - vicii.fetch_clk + 1;
        }

        switch (vicii.fetch_idx) {
            case VICII_FETCH_MATRIX:
                leave = handle_fetch_matrix(offset, sub, &write_offset);
                last_opcode_first_write_clk += write_offset;
                last_opcode_last_write_clk += write_offset;
                break;

            case VICII_CHECK_SPRITE_DMA:
                leave = handle_check_sprite_dma(offset, sub);
                break;

            case VICII_FETCH_SPRITE:
            default:              /* Make compiler happy.  */
                leave = handle_fetch_sprite(offset, sub, &write_offset);
                last_opcode_first_write_clk += write_offset;
                last_opcode_last_write_clk += write_offset;
                break;
        }

        if (leave) {
            break;
        }
    }
}

void vicii_fetch_init(void)
{
    vicii.raster_fetch_alarm = alarm_new(maincpu_alarm_context,
                                         "VicIIRasterFetch",
                                         vicii_fetch_alarm_handler, NULL);
}
