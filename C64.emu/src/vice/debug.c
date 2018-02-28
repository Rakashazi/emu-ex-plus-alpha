/*
 * debug.c - Various debugging options.
 *
 * Written by
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
#include <string.h>

#include "archdep.h"
#include "cmdline.h"
#include "debug.h"
#include "drive.h"
#include "interrupt.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "translate.h"
#include "types.h"
#include "uiapi.h"
#include "util.h"
#include "vice-event.h"


debug_t debug;


#ifdef DEBUG
inline static void debug_history_step(const char *st);
#endif

static int set_do_core_dumps(int val, void *param)
{
    debug.do_core_dumps = val ? 1 : 0;

    return 0;
}

#ifdef DEBUG
static int debug_autoplay_frames;

static int set_maincpu_traceflg(int val, void *param)
{
    debug.maincpu_traceflg = val ? 1 : 0;

    return 0;
}

static int set_drive_traceflg(int val, void *param)
{
    debug.drivecpu_traceflg[vice_ptr_to_uint(param)] = val ? 1 : 0;

    return 0;
}

static int set_trace_mode(int val, void *param)
{
    switch (val) {
        case DEBUG_NORMAL:
        case DEBUG_SMALL:
        case DEBUG_HISTORY:
        case DEBUG_AUTOPLAY:
            break;
        default:
            return -1;
    }

    debug.trace_mode = val;
    return 0;
}

static int set_autoplay_frames(int val, void *param)
{
    if (val < 0) {
        return -1;
    }
    debug_autoplay_frames = val;

    return 0;
}

#endif

/* Debug-related resources. */
static const resource_int_t resources_int[] = {
    { "DoCoreDump", 0, RES_EVENT_NO, NULL,
      &debug.do_core_dumps, set_do_core_dumps, NULL },
#ifdef DEBUG
    { "MainCPU_TRACE", 0, RES_EVENT_NO, NULL,
      &debug.maincpu_traceflg, set_maincpu_traceflg, NULL },
    { "Drive0CPU_TRACE", 0, RES_EVENT_NO, NULL,
      &debug.drivecpu_traceflg[0], set_drive_traceflg, (void *)0 },
    { "Drive1CPU_TRACE", 0, RES_EVENT_NO, NULL,
      &debug.drivecpu_traceflg[1], set_drive_traceflg, (void *)1 },
    { "Drive2CPU_TRACE", 0, RES_EVENT_NO, NULL,
      &debug.drivecpu_traceflg[2], set_drive_traceflg, (void *)2 },
    { "Drive3CPU_TRACE", 0, RES_EVENT_NO, NULL,
      &debug.drivecpu_traceflg[3], set_drive_traceflg, (void *)3 },
    { "TraceMode", DEBUG_NORMAL, RES_EVENT_NO, NULL,
      &debug.trace_mode, set_trace_mode, NULL },
    { "AutoPlaybackFrames", 200, RES_EVENT_NO, NULL,
      &debug_autoplay_frames, set_autoplay_frames, NULL },
#endif
    RESOURCE_INT_LIST_END
};

int debug_resources_init(void)
{
    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] = {
#ifdef DEBUG
    { "-trace_maincpu", SET_RESOURCE, 0,
      NULL, NULL, "MainCPU_TRACE", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_TRACE_MAIN_CPU,
      NULL, NULL },
    { "+trace_maincpu", SET_RESOURCE, 0,
      NULL, NULL, "MainCPU_TRACE", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DONT_TRACE_MAIN_CPU,
      NULL, NULL },
    { "-trace_drive0", SET_RESOURCE, 0,
      NULL, NULL, "Drive0CPU_TRACE", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_TRACE_DRIVE0_CPU,
      NULL, NULL },
    { "+trace_drive0", SET_RESOURCE, 0,
      NULL, NULL, "Drive0CPU_TRACE", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DONT_TRACE_DRIVE0_CPU,
      NULL, NULL },
    { "-trace_drive1", SET_RESOURCE, 0,
      NULL, NULL, "Drive1CPU_TRACE", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_TRACE_DRIVE1_CPU,
      NULL, NULL },
    { "+trace_drive1", SET_RESOURCE, 0,
      NULL, NULL, "Drive1CPU_TRACE", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DONT_TRACE_DRIVE1_CPU,
      NULL, NULL },
    { "-trace_drive2", SET_RESOURCE, 0,
      NULL, NULL, "Drive2CPU_TRACE", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_TRACE_DRIVE2_CPU,
      NULL, NULL },
    { "+trace_drive2", SET_RESOURCE, 0,
      NULL, NULL, "Drive2CPU_TRACE", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DONT_TRACE_DRIVE2_CPU,
      NULL, NULL },
    { "-trace_drive3", SET_RESOURCE, 0,
      NULL, NULL, "Drive3CPU_TRACE", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_TRACE_DRIVE3_CPU,
      NULL, NULL },
    { "+trace_drive3", SET_RESOURCE, 0,
      NULL, NULL, "Drive3CPU_TRACE", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DONT_TRACE_DRIVE3_CPU,
      NULL, NULL },
    { "-trace_mode", SET_RESOURCE, 1,
      NULL, NULL, "TraceMode", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_VALUE, IDCLS_TRACE_MODE,
      NULL, NULL },
    { "-autoplaybackframes", SET_RESOURCE, 1,
      NULL, NULL, "AutoPlaybackFrames", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_FRAMES, IDCLS_SET_AUTO_PLAYBACK_FRAMES,
      NULL, NULL },
#endif
    CMDLINE_LIST_END
};

int debug_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

static unsigned int cycles_per_line;
static unsigned int screen_lines;

void debug_set_machine_parameter(unsigned int cycles, unsigned int lines)
{
    cycles_per_line = cycles;
    screen_lines = lines;
}

#ifdef DEBUG
#define RLINE(clk)  ((unsigned int)((clk) / cycles_per_line % screen_lines))
#define RCYCLE(clk) ((unsigned int)((clk) % cycles_per_line))

void debug_maincpu(DWORD reg_pc, CLOCK mclk, const char *dis, BYTE reg_a,
                   BYTE reg_x, BYTE reg_y, BYTE reg_sp)
{
    switch (debug.trace_mode) {
        case DEBUG_SMALL:
            {
                char small_dis[7];

                small_dis[0] = dis[0];
                small_dis[1] = dis[1];

                if (dis[3] == ' ') {
                    small_dis[2] = '\0';
                } else {
                    small_dis[2] = dis[3];
                    small_dis[3] = dis[4];
                    if (dis[6] == ' ') {
                        small_dis[4] = '\0';
                    } else {
                        small_dis[4] = dis[6];
                        small_dis[5] = dis[7];
                        small_dis[6] = '\0';
                    }
                }

                log_debug("%04X %ld %02X%02X%02X %s", (unsigned int)reg_pc,
                          (long)mclk, reg_a, reg_x, reg_y, small_dis);
                break;
            }
        case DEBUG_HISTORY:
        case DEBUG_AUTOPLAY:
            {
                char st[DEBUG_MAXLINELEN];

                sprintf(st, ".%04X %02X %02X %8lX %-20s "
                        "%02X%02X%02X%02X", (unsigned int)reg_pc,
                        RLINE(mclk), RCYCLE(mclk), (long)mclk, dis,
                        reg_a, reg_x, reg_y, reg_sp);
                debug_history_step(st);
                break;
            }
        case DEBUG_NORMAL:
            log_debug(".%04X %03i %03i %10ld  %-22s "
                      "%02x%02x%02x%02x", (unsigned int)reg_pc,
                      RLINE(mclk), RCYCLE(mclk), (long)mclk, dis,
                      reg_a, reg_x, reg_y, reg_sp);
            break;
        default:
            log_debug("Unknown debug format.");
    }
}

void debug_main65816cpu(DWORD reg_pc, CLOCK mclk, const char *dis, WORD reg_c,
                   WORD reg_x, WORD reg_y, WORD reg_sp, BYTE reg_pbr)
{
    switch (debug.trace_mode) {
        case DEBUG_SMALL:
        {
            char small_dis[7];

            small_dis[0] = dis[0];
            small_dis[1] = dis[1];

            if (dis[3] == ' ') {
                small_dis[2] = '\0';
            } else {
                small_dis[2] = dis[3];
                small_dis[3] = dis[4];
                if (dis[6] == ' ') {
                    small_dis[4] = '\0';
                } else {
                    small_dis[4] = dis[6];
                    small_dis[5] = dis[7];
                    small_dis[6] = '\0';
                }  
            }

            log_debug("%02X%04X %ld %04X %04X %04X %s", reg_pbr, (unsigned int)reg_pc,
                    (long)mclk, reg_c, reg_x, reg_y, small_dis);
            break;
      }
      case DEBUG_HISTORY:
      case DEBUG_AUTOPLAY:
      {
            char st[DEBUG_MAXLINELEN];

            sprintf(st, ".%02X%04X %02X %02X %8lX %-23s "
                    "%04X %04X %04X %02X", reg_pbr, (unsigned int)reg_pc,
                    RLINE(mclk), RCYCLE(mclk), (long)mclk, dis,
                    reg_c, reg_x, reg_y, reg_sp);
            debug_history_step(st);
            break;
      }
      case DEBUG_NORMAL:
            log_debug(".%02X%04X %03i %03i %10ld  %-25s "
                    "%04x %04x %04x %04x", reg_pbr, (unsigned int)reg_pc,
                    RLINE(mclk), RCYCLE(mclk), (long)mclk, dis,
                    reg_c, reg_x, reg_y, reg_sp);
            break;
      default:
            log_debug("Unknown debug format.");

    }
}

void debug_drive(DWORD reg_pc, CLOCK mclk, const char *dis,
                 BYTE reg_a, BYTE reg_x, BYTE reg_y, BYTE reg_sp,
                 unsigned int driveno)
{
    char st[DEBUG_MAXLINELEN];

    sprintf(st, "Drive %2u:.%04X %10ld %-22s %02x%02x%02x%02x",
            driveno,
            (unsigned int)reg_pc, (long)mclk, dis, reg_a, reg_x, reg_y, reg_sp);
    if (debug.trace_mode == DEBUG_HISTORY || debug.trace_mode == DEBUG_AUTOPLAY) {
        debug_history_step(st);
    } else {
        log_debug("%s", st);
    }
}

void debug_text(const char *text)
{
    if (debug.trace_mode == DEBUG_HISTORY || debug.trace_mode == DEBUG_AUTOPLAY) {
        debug_history_step(text);
    } else {
        log_debug("%s", text);
    }
}

static void debug_int(interrupt_cpu_status_t *cs, const char *name,
                      unsigned int type, CLOCK iclk)
{
    unsigned int i;
    char *textout, *texttmp;

    textout = lib_stralloc(name);

    for (i = 0; i < cs->num_ints; i++) {
        if (cs->pending_int[i] & type) {
            texttmp = util_concat(textout, " ", cs->int_name[i], NULL);
            lib_free(textout);
            textout = texttmp;
        }
    }

    texttmp = lib_msprintf("%s %ld", textout, iclk);
    lib_free(textout);
    textout = texttmp;

    if (debug.trace_mode == DEBUG_HISTORY || debug.trace_mode == DEBUG_AUTOPLAY) {
        debug_history_step(textout);
    } else {
        log_debug("%s", textout);
    }

    lib_free(textout);
}

void debug_irq(interrupt_cpu_status_t *cs, CLOCK iclk)
{
    debug_int(cs, "*** IRQ", IK_IRQ, iclk);
}

void debug_nmi(interrupt_cpu_status_t *cs, CLOCK iclk)
{
    debug_int(cs, "*** NMI", IK_NMI, iclk);
}

void debug_dma(const char *txt, CLOCK dclk, int num)
{
    log_debug("*** DMA %s %10ld  %02i", txt, (long)dclk, num);
}

/*------------------------------------------------------------------------*/

static FILE *debug_file = NULL;
static char *debug_buffer;
static int debug_buffer_ptr;
static int debug_buffer_size;
static int debug_file_current;
static int debug_file_line;
static int debug_file_milestone;
static int debug_autoplay_nextmode;
static int debug_autoplay_current_frame;

static void debug_close_file(void)
{
    if (debug_file != NULL) {
        if (fwrite(debug_buffer, sizeof(char), debug_buffer_ptr, debug_file) < (size_t)debug_buffer_ptr) {
            fprintf(stderr, "error writing debug log.\n");
        }
        fclose(debug_file);
        debug_file = NULL;
        debug_buffer_ptr = 0;
        debug_file_current++;
    }
}

static void debug_create_new_file(void)
{
    char *filename, *st;
    const char *directory;

    debug_close_file();

    resources_get_string("EventSnapshotDir", &directory);

    st = lib_msprintf("debug%06d", debug_file_current);
    filename = util_concat(directory, st, FSDEV_EXT_SEP_STR, "log", NULL);
    lib_free(st);

    debug_file = fopen(filename, MODE_WRITE_TEXT);

    lib_free(filename);
}

static void debug_open_new_file(void)
{
    char *filename, *st;
    const char *directory;

    if (debug_file != NULL) {
        fclose(debug_file);
    }

    resources_get_string("EventSnapshotDir", &directory);

    st = lib_msprintf("debug%06d", debug_file_current);
    filename = util_concat(directory, st, FSDEV_EXT_SEP_STR, "log", NULL);
    lib_free(st);

    debug_file = fopen(filename, MODE_READ_TEXT);
    if (debug_file != NULL) {
        debug_buffer_size = fread(debug_buffer, sizeof(char), DEBUG_HISTORY_MAXFILESIZE, debug_file);
        debug_buffer_ptr = 0;
        debug_file_current++;
    } else {
        debug_buffer_size = 0;
    }

    debug_file_line = 0;

    lib_free(filename);
}

#ifdef DEBUG
inline static void debug_history_step(const char *st)
{
    if (event_record_active()) {
        if (debug_buffer_ptr + DEBUG_MAXLINELEN >= DEBUG_HISTORY_MAXFILESIZE) {
            debug_create_new_file();
        }

        debug_buffer_ptr += sprintf(debug_buffer + debug_buffer_ptr, "%s\n", st);
    }

    if (event_playback_active()) {
        char tempstr[DEBUG_MAXLINELEN];
        int line_len = sprintf(tempstr, "%s\n", st);

        if (debug_buffer_ptr >= debug_buffer_size) {
            debug_open_new_file();
        }

        debug_file_line++;

        if (strncmp(st, debug_buffer + debug_buffer_ptr, strlen(st)) != 0) {
            event_playback_stop();
            ui_error(translate_text(IDGS_PLAYBACK_ERROR_DIFFERENT)
                     , st, debug_file_line, debug_file_current - 1);
        }

        debug_buffer_ptr += line_len;
    }
}
#endif

void debug_start_recording(void)
{
    if (debug.trace_mode < DEBUG_HISTORY) {
        return;
    }

    debug_autoplay_current_frame = 0;
    debug_file_current = 0;
    debug_file_milestone = 0;
    debug_buffer_ptr = 0;
    debug_buffer = lib_malloc(DEBUG_HISTORY_MAXFILESIZE);
    debug_create_new_file();
}

void debug_stop_recording(void)
{
    if (debug.trace_mode < DEBUG_HISTORY) {
        return;
    }

    debug_close_file();
    lib_free(debug_buffer);
}

void debug_start_playback(void)
{
    if (debug.trace_mode < DEBUG_HISTORY) {
        return;
    }

    debug_file_current = 0;
    debug_file_milestone = 0;
    debug_buffer_ptr = 0;
    debug_buffer = lib_malloc(DEBUG_HISTORY_MAXFILESIZE);
    debug_open_new_file();
}

void debug_stop_playback(void)
{
    if (debug.trace_mode < DEBUG_HISTORY) {
        return;
    }

    if (debug_file != NULL) {
        fclose(debug_file);
        debug_file = NULL;
    }
    lib_free(debug_buffer);

    if (debug.trace_mode == DEBUG_AUTOPLAY) {
        debug_autoplay_nextmode = 1; /* start recording next */
    }
}

void debug_set_milestone(void)
{
    if (debug.trace_mode < DEBUG_HISTORY) {
        return;
    }

    debug_create_new_file();
    debug_file_milestone = debug_file_current;
}

void debug_reset_milestone(void)
{
    if (debug.trace_mode < DEBUG_HISTORY) {
        return;
    }

    debug_file_current = debug_file_milestone - 1;
    debug_create_new_file();
}

void debug_check_autoplay_mode(void)
{
    if (debug.trace_mode != DEBUG_AUTOPLAY) {
        return;
    }

    if (debug_autoplay_nextmode == 2) {
        event_playback_start();
        debug_autoplay_nextmode = 0;
        return;
    }

    if (debug_autoplay_nextmode == 1) {
        /* AUTPLAY mode needs to start recording */
        event_record_start();
        debug_autoplay_nextmode = 0;
        return;
    }

    debug_autoplay_current_frame++;

    if (debug_autoplay_current_frame >= debug_autoplay_frames) {
        debug_autoplay_current_frame = 0;

        if (event_record_active()) {
            event_record_stop();
            debug_autoplay_nextmode = 2; /* start playback next */
            return;
        }
    }
}

#endif
