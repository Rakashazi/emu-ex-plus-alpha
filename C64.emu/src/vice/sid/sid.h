
/*! \file sid/sid.h */

/*
 * sid.h - MOS6581 (SID) emulation, hooks to actual implementation.
 *
 * Written by
 *  Dag Lem <resid@nimrod.no>
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

#ifndef VICE_SID_ENGINE_H
#define VICE_SID_ENGINE_H

#include "types.h"
#include "sound.h"

#if (defined(WIN32) || defined(MACOSX_COCOA) || defined(WATCOM_COMPILE) || defined(USE_BEOS_UI)) && !defined(USE_SDLUI)
#define SID_SETTINGS_DIALOG
#endif

struct sound_s;
struct sid_snapshot_state_s;

#define SID_ENGINE_FASTSID        0
#define SID_ENGINE_RESID          1
#define SID_ENGINE_CATWEASELMKIII 2
#define SID_ENGINE_HARDSID        3
#define SID_ENGINE_PARSID_PORT1   4
#define SID_ENGINE_PARSID_PORT2   5
#define SID_ENGINE_PARSID_PORT3   6
#define SID_ENGINE_DEFAULT       99

#define SID_MODEL_6581           0
#define SID_MODEL_8580           1
#define SID_MODEL_8580D          2
#define SID_MODEL_6581R4         3
#define SID_MODEL_DTVSID         4
#define SID_MODEL_DEFAULT       99

/* these definitions are the only valid combinations of
   software SID engines and model, and are used in the
   UI and command line code. */
#define SID_FASTSID_6581          ((SID_ENGINE_FASTSID << 8) | SID_MODEL_6581)
#define SID_FASTSID_8580          ((SID_ENGINE_FASTSID << 8) | SID_MODEL_8580)
#define SID_RESID_6581            ((SID_ENGINE_RESID << 8) | SID_MODEL_6581)
#define SID_RESID_8580            ((SID_ENGINE_RESID << 8) | SID_MODEL_8580)
#define SID_RESID_8580D           ((SID_ENGINE_RESID << 8) | SID_MODEL_8580D)
#define SID_RESID_DTVSID          ((SID_ENGINE_RESID << 8) | SID_MODEL_DTVSID)
#define SID_CATWEASELMKIII        (SID_ENGINE_CATWEASELMKIII << 8)
#define SID_HARDSID               (SID_ENGINE_HARDSID << 8)
#define SID_PARSID_PORT1          (SID_ENGINE_PARSID_PORT1 << 8)
#define SID_PARSID_PORT2          (SID_ENGINE_PARSID_PORT2 << 8)
#define SID_PARSID_PORT3          (SID_ENGINE_PARSID_PORT3 << 8)

extern void machine_sid2_enable(int val);

extern BYTE sid_read(WORD address);
extern BYTE sid_peek(WORD address);
extern BYTE sid2_read(WORD address);
extern BYTE sid3_read(WORD address);
extern void sid_store(WORD address, BYTE byte);
extern void sid2_store(WORD address, BYTE byte);
extern void sid3_store(WORD address, BYTE byte);
extern void sid_reset(void);

extern void sid_set_machine_parameter(long clock_rate);
extern BYTE *sid_get_siddata(unsigned int channel);
extern int sid_engine_set(int engine);
extern void sid_state_read(unsigned int channel,
                           struct sid_snapshot_state_s *sid_state);
extern void sid_state_write(unsigned int channel,
                            struct sid_snapshot_state_s *sid_state);

struct sid_engine_s {
    struct sound_s *(*open)(BYTE *sidstate);
    int (*init)(struct sound_s *psid, int speed, int cycles_per_sec, int factor);
    void (*close)(struct sound_s *psid);
    BYTE (*read)(struct sound_s *psid, WORD addr);
    void (*store)(struct sound_s *psid, WORD addr, BYTE val);
    void (*reset)(struct sound_s *psid, CLOCK cpu_clk);
    int (*calculate_samples)(struct sound_s *psid, SWORD *pbuf, int nr,
                             int interleave, int *delta_t);
    void (*prevent_clk_overflow)(struct sound_s *psid, CLOCK sub);
    char *(*dump_state)(struct sound_s *psid);
    void (*state_read)(struct sound_s *psid,
                       struct sid_snapshot_state_s *sid_state);
    void (*state_write)(struct sound_s *psid,
                        struct sid_snapshot_state_s *sid_state);
};
typedef struct sid_engine_s sid_engine_t;

struct sid_engine_model_s {
    char *name;
    int value;
};
typedef struct sid_engine_model_s sid_engine_model_t;

extern sound_t *sid_sound_machine_open(int chipno);
extern int sid_sound_machine_init_vbr(sound_t *psid, int speed, int cycles_per_sec, int factor);
extern int sid_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec);
extern void sid_sound_machine_close(sound_t *psid);
extern BYTE sid_sound_machine_read(sound_t *psid, WORD addr);
extern void sid_sound_machine_store(sound_t *psid, WORD addr, BYTE byte);
extern void sid_sound_machine_reset(sound_t *psid, CLOCK cpu_clk);
extern int sid_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int sound_output_channels, int sound_chip_channels, int *delta_t);
extern void sid_sound_machine_prevent_clk_overflow(sound_t *psid, CLOCK sub);
extern char *sid_sound_machine_dump_state(sound_t *psid);
extern int sid_sound_machine_cycle_based(void);
extern int sid_sound_machine_channels(void);
extern void sid_sound_machine_enable(int enable);
extern sid_engine_model_t **sid_get_engine_model_list(void);
extern int sid_set_engine_model(int engine, int model);
extern void sid_sound_chip_init(void);

#endif
