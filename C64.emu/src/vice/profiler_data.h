/*
 * profiler_data.h -- CPU Profiler Data Structures
 *
 * Written by
 *  Oskar Linde <oskar.linde@gmail.com>
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

#ifndef VICE_PROFILER_DATA_H
#define VICE_PROFILER_DATA_H

#include "types.h"

enum CallstackMagic {
    NMI   = 0xfffa,
    RESET = 0xfffc,
    IRQ   = 0xfffe,
};

typedef uint32_t profiling_counter_t;

typedef struct profiling_data_s {
    profiling_counter_t num_samples;
    profiling_counter_t num_cycles:31;
    profiling_counter_t touched:1;
} profiling_data_t;

typedef struct profiling_page_s {
    profiling_data_t data[256];
} profiling_page_t;

typedef struct profiling_context_s {
    profiling_page_t           *page[256];
    uint16_t                    pc_dst;
    uint16_t                    pc_src;
    uint16_t                    memory_bank_config;
    profiling_counter_t         num_enters;
    profiling_counter_t         num_exits;
    struct profiling_context_s *parent;
    struct profiling_context_s *child;
    struct profiling_context_s *next;            /* cyclic linked list of sibling contexts */
    struct profiling_context_s *next_mem_config; /* non-cyclic linked list of mem bank configs */

    profiling_counter_t total_stolen_cycles_self;

    /* the items below are populated by compute_aggregate_stats() */
    profiling_counter_t total_cycles;
    profiling_counter_t total_cycles_self;
    profiling_counter_t total_stolen_cycles;

    int id;
} profiling_context_t;

extern profiling_context_t  *root_context;
extern profiling_context_t  *current_context;

profiling_context_t *profile_context_by_id(int id);
int                  get_context_id(profiling_context_t *context);
void                 compute_aggregate_stats(profiling_context_t *context);
profiling_page_t    *profiling_get_page(profiling_context_t *ctx, uint8_t page);
profiling_context_t *alloc_profiling_context(void);
void                 free_profiling_context(profiling_context_t *data);
profiling_context_t *get_mem_config_context(profiling_context_t *main_context,
                                            uint16_t mem_config);

#endif /* VICE_PROFILER_DATA_H */
