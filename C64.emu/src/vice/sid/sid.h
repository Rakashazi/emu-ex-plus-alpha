/** \file   sid.h
 * \brief   MOS6581 (SID) emulation, hooks to actual implementation - header
 *
 * \author  Dag Lem <resid@nimrod.no>
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 */

/*
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

#include <stdbool.h>

#include "types.h"
#include "sound.h"

#define SID_SETTINGS_DIALOG

struct sound_s;
struct sid_snapshot_state_s;

enum {
    SID_ENGINE_FASTSID = 0,
    SID_ENGINE_RESID,
    SID_ENGINE_CATWEASELMKIII,
    SID_ENGINE_HARDSID,
    SID_ENGINE_PARSID
};

#define SID_ENGINE_DEFAULT       99

/* Maximum number of supported SIDs for each engine
 */

/** \brief  Maximum number of supported SIDs for the FastSID engine */
#define SID_ENGINE_FASTSID_NUM_SIDS         4

/** \brief  Maximum number of supported SIDs for the ReSID engine */
#define SID_ENGINE_RESID_NUM_SIDS           4

/** \brief  Maximum number of supported SIDs for the Catweasel Mk3 engine */
#define SID_ENGINE_CATWEASELMKIII_NUM_SIDS  2

/** \brief  Maximum number of supported SIDs for the HardSID engine */
#define SID_ENGINE_HARDSID_NUM_SIDS         2

/** \brief  Maximum number of supported SIDs for the ParSID engine */
#define SID_ENGINE_PARSID_NUM_SIDS          1

enum {
    SID_RESID_SAMPLING_FAST = 0,
    SID_RESID_SAMPLING_INTERPOLATION,
    SID_RESID_SAMPLING_RESAMPLING,
    SID_RESID_SAMPLING_FAST_RESAMPLING
};

enum {
    SID_MODEL_6581 = 0,
    SID_MODEL_8580,
    SID_MODEL_8580D,
    SID_MODEL_DTVSID
};

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
#define SID_PARSID                (SID_ENGINE_PARSID << 8)

#define SIDTYPE_SID       0
#define SIDTYPE_SIDDTV    1
#define SIDTYPE_SIDCART   2

#define SID_MACHINE_MAX_SID_C64     8
#define SID_MACHINE_MAX_SID_C64DTV  1
#define SID_MACHINE_MAX_SID_C128    8

/** \brief  The VIC20 has an optional SID cartridge */
#define SID_MACHINE_MAX_SID_VIC20   1

/** \brief  The Plus4 has an optional SIDCard expansion */
#define SID_MACHINE_MAX_SID_PLUS4   1

#define SID_MACHINE_MAX_SID_CBM5x0  1
#define SID_MACHINE_MAX_SID_CBM6x0  0

/** \brief  The PET has an optional SID Card expansion */
#define SID_MACHINE_MAX_SID_PET     1

/** \brief  VSID supports up to three SIDS
 *
 * This can be the same as C64 in emulation, but PSID currently only manages 3
 * SIDs.
 */
#define SID_MACHINE_MAX_SID_VSID    3


#define RESID_6581_PASSBAND_MIN             0
#define RESID_6581_PASSBAND_MAX             90
#define RESID_6581_PASSBAND_ONE             1
#define RESID_6581_PASSBAND_DEFAULT         90

#define RESID_6581_FILTER_GAIN_MIN          90
#define RESID_6581_FILTER_GAIN_MAX          100
#define RESID_6581_FILTER_GAIN_ONE          1
#define RESID_6581_FILTER_GAIN_DEFAULT      97

#define RESID_6581_FILTER_BIAS_MIN          -5000
#define RESID_6581_FILTER_BIAS_MAX          5000
#define RESID_6581_FILTER_BIAS_ONE          1000
#define RESID_6581_FILTER_BIAS_DEFAULT      500

#define RESID_8580_PASSBAND_MIN             0
#define RESID_8580_PASSBAND_MAX             90
#define RESID_8580_PASSBAND_ONE             1
#define RESID_8580_PASSBAND_DEFAULT         90

#define RESID_8580_FILTER_GAIN_MIN          90
#define RESID_8580_FILTER_GAIN_MAX          100
#define RESID_8580_FILTER_GAIN_ONE          1
#define RESID_8580_FILTER_GAIN_DEFAULT      97

#define RESID_8580_FILTER_BIAS_MIN          -5000
#define RESID_8580_FILTER_BIAS_MAX          5000
#define RESID_8580_FILTER_BIAS_ONE          1000
#define RESID_8580_FILTER_BIAS_DEFAULT      0


void machine_sid2_enable(int val);

uint8_t sid_read(uint16_t address);
uint8_t sid2_read(uint16_t address);
uint8_t sid3_read(uint16_t address);
uint8_t sid4_read(uint16_t address);
uint8_t sid5_read(uint16_t address);
uint8_t sid6_read(uint16_t address);
uint8_t sid7_read(uint16_t address);
uint8_t sid8_read(uint16_t address);

uint8_t sid_peek(uint16_t address);
uint8_t sid2_peek(uint16_t address);
uint8_t sid3_peek(uint16_t address);
uint8_t sid4_peek(uint16_t address);
uint8_t sid5_peek(uint16_t address);
uint8_t sid6_peek(uint16_t address);
uint8_t sid7_peek(uint16_t address);
uint8_t sid8_peek(uint16_t address);

void sid_store(uint16_t address, uint8_t byte);
void sid2_store(uint16_t address, uint8_t byte);
void sid3_store(uint16_t address, uint8_t byte);
void sid4_store(uint16_t address, uint8_t byte);
void sid5_store(uint16_t address, uint8_t byte);
void sid6_store(uint16_t address, uint8_t byte);
void sid7_store(uint16_t address, uint8_t byte);
void sid8_store(uint16_t address, uint8_t byte);

int sid_dump(void);
int sid2_dump(void);
int sid3_dump(void);
int sid4_dump(void);
int sid5_dump(void);
int sid6_dump(void);
int sid7_dump(void);
int sid8_dump(void);

void sid_reset(void);

void sid_set_machine_parameter(long clock_rate);
uint8_t *sid_get_siddata(unsigned int channel);
int sid_engine_set(int engine);
void sid_state_read(unsigned int channel, struct sid_snapshot_state_s *sid_state);
void sid_state_write(unsigned int channel, struct sid_snapshot_state_s *sid_state);

struct sid_engine_s {
    struct sound_s *(*open)(uint8_t *sidstate);
    int (*init)(struct sound_s *psid, int speed, int cycles_per_sec, int factor);
    void (*close)(struct sound_s *psid);
    uint8_t (*read)(struct sound_s *psid, uint16_t addr);
    void (*store)(struct sound_s *psid, uint16_t addr, uint8_t val);
    void (*reset)(struct sound_s *psid, CLOCK cpu_clk);
#ifdef SOUND_SYSTEM_FLOAT
    int (*calculate_samples)(struct sound_s *psid, float *pbuf, int nr, CLOCK *delta_t);
#else
    int (*calculate_samples)(struct sound_s *psid, short *pbuf, int nr, int interleave, CLOCK *delta_t);
#endif
    char *(*dump_state)(struct sound_s *psid);
    void (*state_read)(struct sound_s *psid, struct sid_snapshot_state_s *sid_state);
    void (*state_write)(struct sound_s *psid, struct sid_snapshot_state_s *sid_state);
};
typedef struct sid_engine_s sid_engine_t;

struct sid_engine_model_s {
    char *name;
    int value;
};
typedef struct sid_engine_model_s sid_engine_model_t;

bool sid_sound_machine_set_engine_hooks(void);
sound_t *sid_sound_machine_open(int chipno);
int sid_sound_machine_init_vbr(sound_t *psid, int speed, int cycles_per_sec, int factor);
int sid_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec);
void sid_sound_machine_close(sound_t *psid);
uint8_t sid_sound_machine_read(sound_t *psid, uint16_t addr);
void sid_sound_machine_store(sound_t *psid, uint16_t addr, uint8_t byte);
void sid_sound_machine_reset(sound_t *psid, CLOCK cpu_clk);
char *sid_sound_machine_dump_state(sound_t *psid);
int sid_sound_machine_cycle_based(void);
int sid_sound_machine_channels(void);
void sid_sound_machine_enable(int enable);
sid_engine_model_t **sid_get_engine_model_list(void);
int sid_set_engine_model(int engine, int model);
void sid_sound_chip_init(void);

#ifdef SOUND_SYSTEM_FLOAT
int sid_sound_machine_calculate_samples(sound_t **psid, float *pbuf, int nr, int sound_chip_channels, CLOCK *delta_t);
#else
int sid_sound_machine_calculate_samples(sound_t **psid, int16_t *pbuf, int nr, int sound_output_channels, int sound_chip_channels, CLOCK *delta_t);
#endif

void sid_set_enable(int value);

int sid_engine_get_max_sids(int engine);
int sid_machine_get_max_sids(void);
int sid_machine_engine_get_max_sids(int engine);
int sid_machine_can_have_multiple_sids(void);

#endif
