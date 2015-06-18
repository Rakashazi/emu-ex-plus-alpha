/*
 * sid.c - MOS6581 (SID) emulation, hooks to actual implementation.
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Michael Schwendt <sidplay@geocities.com>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "catweaselmkiii.h"
#include "fastsid.h"
#include "hardsid.h"
#include "lib.h"
#include "machine.h"
#include "maincpu.h"
#ifdef HAVE_PARSID
#include "parsid.h"
#endif
#include "resources.h"
#include "sid-resources.h"
#include "sid-snapshot.h"
#include "sid.h"
#include "sound.h"
#include "types.h"

#ifdef HAVE_MOUSE
#include "mouse.h"
#include "lightpen.h"
#endif

#ifdef HAVE_RESID
#include "resid.h"
#endif

/* SID engine hooks. */
static sid_engine_t sid_engine;

/* read register value from sid */
static BYTE lastsidread;

/* register data */
static BYTE siddata[SOUND_SIDS_MAX][32];

static int (*sid_read_func)(WORD addr, int chipno);
static void (*sid_store_func)(WORD addr, BYTE val, int chipno);

static int sid_enable, sid_engine_type = -1;

#ifdef HAVE_MOUSE
static CLOCK pot_cycle = 0;  /* pot sampling cycle */
static BYTE val_pot_x = 0xff, val_pot_y = 0xff; /* last sampling value */
#endif

BYTE *sid_get_siddata(unsigned int channel)
{
    return siddata[channel];
}

/* ------------------------------------------------------------------------- */

static int sid_read_off(WORD addr, int chipno)
{
    BYTE val;

    if (addr == 0x19 || addr == 0x1a) {
        val = 0xff;
    } else {
        if (addr == 0x1b || addr == 0x1c) {
            val = (BYTE)(maincpu_clk % 256);
        } else {
            val = 0;
        }
    }

    /* FIXME: Change API, return BYTE! */
    return (int)val;
}

static void sid_write_off(WORD addr, BYTE val, int chipno)
{
}

/* ------------------------------------------------------------------------- */

static BYTE sid_read_chip(WORD addr, int chipno)
{
    int val = -1;

    addr &= 0x1f;

    machine_handle_pending_alarms(0);

#ifdef HAVE_MOUSE
    if (chipno == 0 && (addr == 0x19 || addr == 0x1a)) {
        if ((maincpu_clk ^ pot_cycle) & ~511) {
            pot_cycle = maincpu_clk & ~511; /* simplistic 512 cycle sampling */
            if (_mouse_enabled) {
                val_pot_x = mouse_get_x();
                val_pot_y = mouse_get_y();
            } else if (lightpen_enabled) {
                val_pot_x = lightpen_read_button_x();
                val_pot_y = lightpen_read_button_y();
            } else {
                val_pot_x = 0xff;
                val_pot_y = 0xff;
            }
        }
        val = (addr == 0x19) ? val_pot_x : val_pot_y;
    } else
#endif
    {
        if (machine_class == VICE_MACHINE_C64SC
            || machine_class == VICE_MACHINE_SCPU64) {
            /* On x64sc, the read/write calls both happen before incrementing
               the clock, so don't mess with maincpu_clk here.  */
            val = sid_read_func(addr, chipno);
        } else {
            /* Account for that read functions in VICE are called _before_
               incrementing the clock. */
            maincpu_clk++;
            val = sid_read_func(addr, chipno);
            maincpu_clk--;
        }
    }

    /* Fallback when sound is switched off. */
    if (val < 0) {
        if (addr == 0x19 || addr == 0x1a) {
            val = 0xff;
        } else {
            if (addr == 0x1b || addr == 0x1c) {
                val = maincpu_clk % 256;
            } else {
                val = 0;
            }
        }
    }

    lastsidread = val;
    return val;
}

static BYTE sid_peek_chip(WORD addr, int chipno)
{
    addr &= 0x1f;

    /* FIXME: get 0x1b and 0x1c from engine */
    return siddata[chipno][addr];
}

/* write register value to sid */
static void sid_store_chip(WORD addr, BYTE byte, int chipno)
{
    addr &= 0x1f;

    siddata[chipno][addr] = byte;

    /* WARNING: assumes `maincpu_rmw_flag' is 0 or 1.  */
    machine_handle_pending_alarms(maincpu_rmw_flag + 1);

    if (maincpu_rmw_flag) {
        maincpu_clk--;
        sid_store_func(addr, lastsidread, chipno);
        maincpu_clk++;
    }

    sid_store_func(addr, byte, chipno);
}

/* ------------------------------------------------------------------------- */

BYTE sid_read(WORD addr)
{
    if (sid_stereo >= 1
        && addr >= sid_stereo_address_start
        && addr < sid_stereo_address_end) {
        return sid_read_chip(addr, 1);
    }

    if (sid_stereo >= 2
        && addr >= sid_triple_address_start
        && addr < sid_triple_address_end) {
        return sid_read_chip(addr, 2);
    }

    return sid_read_chip(addr, 0);
}

BYTE sid_peek(WORD addr)
{
    if (sid_stereo >= 1
        && addr >= sid_stereo_address_start
        && addr < sid_stereo_address_end) {
        return sid_peek_chip(addr, 1);
    }

    if (sid_stereo >= 2
        && addr >= sid_triple_address_start
        && addr < sid_triple_address_end) {
        return sid_peek_chip(addr, 2);
    }

    return sid_peek_chip(addr, 0);
}

BYTE sid2_read(WORD addr)
{
    return sid_read_chip(addr, 1);
}

BYTE sid3_read(WORD addr)
{
    return sid_read_chip(addr, 2);
}

void sid_store(WORD addr, BYTE byte)
{
    if (sid_stereo >= 1
        && addr >= sid_stereo_address_start
        && addr < sid_stereo_address_end) {
        sid_store_chip(addr, byte, 1);
        return;
    }
    if (sid_stereo >= 2
        && addr >= sid_triple_address_start
        && addr < sid_triple_address_end) {
        sid_store_chip(addr, byte, 2);
    }
    sid_store_chip(addr, byte, 0);
}

void sid2_store(WORD addr, BYTE byte)
{
    sid_store_chip(addr, byte, 1);
}

void sid3_store(WORD addr, BYTE byte)
{
    sid_store_chip(addr, byte, 2);
}

/* ------------------------------------------------------------------------- */

void sid_reset(void)
{
    sound_reset();

    memset(siddata, 0, sizeof(siddata));
}

static int sidengine;

sound_t *sid_sound_machine_open(int chipno)
{
    sidengine = 0;

    if (resources_get_int("SidEngine", &sidengine) < 0) {
        return NULL;
    }

    sid_engine = fastsid_hooks;

#ifdef HAVE_RESID
    if (sidengine == SID_ENGINE_RESID) {
        sid_engine = resid_hooks;
    }
#endif

    return sid_engine.open(siddata[chipno]);
}

/* manage temporary buffers. if the requested size is smaller or equal to the
 * size of the already allocated buffer, reuse it.  */
static SWORD *buf1 = NULL;
static SWORD *buf2 = NULL;
static int blen1 = 0;
static int blen2 = 0;

static SWORD *getbuf1(int len)
{
    if ((buf1 == NULL) || (blen1 < len)) {
        if (buf1) {
            lib_free(buf1);
        }
        blen1 = len;
        buf1 = lib_calloc(len, 1);
    }
    return buf1;
}

static SWORD *getbuf2(int len)
{
    if ((buf2 == NULL) || (blen2 < len)) {
        if (buf2) {
            lib_free(buf2);
        }
        blen2 = len;
        buf2 = lib_calloc(len, 1);
    }
    return buf2;
}

int sid_sound_machine_init_vbr(sound_t *psid, int speed, int cycles_per_sec, int factor)
{
    return sid_engine.init(psid, speed * factor / 1000, cycles_per_sec, factor);
}

int sid_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
    return sid_engine.init(psid, speed, cycles_per_sec, 1000);
}

void sid_sound_machine_close(sound_t *psid)
{
    sid_engine.close(psid);
    /* free the temp. buffers */
    if (buf1) {
        lib_free(buf1);
        buf1 = NULL;
    }
    if (buf2) {
        lib_free(buf2);
        buf2 = NULL;
    }
}

BYTE sid_sound_machine_read(sound_t *psid, WORD addr)
{
    return sid_engine.read(psid, addr);
}

void sid_sound_machine_store(sound_t *psid, WORD addr, BYTE byte)
{
    sid_engine.store(psid, addr, byte);
}

void sid_sound_machine_reset(sound_t *psid, CLOCK cpu_clk)
{
    sid_engine.reset(psid, cpu_clk);
}

int sid_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int soc, int scc, int *delta_t)
{
    int i;
    SWORD *tmp_buf1;
    SWORD *tmp_buf2;
    int tmp_nr = 0;
    int tmp_delta_t = *delta_t;

    if (soc == 1 && scc == 1) {
        return sid_engine.calculate_samples(psid[0], pbuf, nr, 1, delta_t);
    }
    if (soc == 1 && scc == 2) {
        tmp_buf1 = getbuf1(2 * nr);
        tmp_nr = sid_engine.calculate_samples(psid[0], tmp_buf1, nr, 1, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf, nr, 1, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf1[i]);
        }
        return tmp_nr;
    }
    if (soc == 1 && scc == 3) {
        tmp_buf1 = getbuf1(2 * nr);
        tmp_buf2 = getbuf2(2 * nr);
        tmp_nr = sid_engine.calculate_samples(psid[0], tmp_buf1, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[2], tmp_buf2, nr, 1, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf, nr, 1, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf1[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf2[i]);
        }
        return tmp_nr;
    }
    if (soc == 2 && scc == 1) {
        tmp_nr = sid_engine.calculate_samples(psid[0], pbuf, nr, 2, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[(i * 2) + 1] = pbuf[i * 2];
        }
        return tmp_nr;
    }
    if (soc == 2 && scc == 2) {
        tmp_nr = sid_engine.calculate_samples(psid[0], pbuf, nr, 2, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf + 1, nr, 2, delta_t);
        return tmp_nr;
    }
    if (soc == 2 && scc == 3) {
        tmp_buf1 = getbuf1(2 * nr);
        tmp_nr = sid_engine.calculate_samples(psid[2], tmp_buf1, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[0], pbuf, nr, 2, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf + 1, nr, 2, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[i * 2] = sound_audio_mix(pbuf[i * 2], tmp_buf1[i]);
            pbuf[(i * 2) + 1] = sound_audio_mix(pbuf[(i * 2) + 1], tmp_buf1[i]);
        }
    }
    return tmp_nr;
}

void sid_sound_machine_prevent_clk_overflow(sound_t *psid, CLOCK sub)
{
    sid_engine.prevent_clk_overflow(psid, sub);
}

char *sid_sound_machine_dump_state(sound_t *psid)
{
    return sid_engine.dump_state(psid);
}

int sid_sound_machine_cycle_based(void)
{
    switch (sidengine) {
        case SID_ENGINE_FASTSID:
            return 0;
#ifdef HAVE_RESID
        case SID_ENGINE_RESID:
            return 1;
#endif
#ifdef HAVE_CATWEASELMKIII
        case SID_ENGINE_CATWEASELMKIII:
            return 0;
#endif
#ifdef HAVE_HARDSID
        case SID_ENGINE_HARDSID:
            return 0;
#endif
#ifdef HAVE_PARSID
        case SID_ENGINE_PARSID_PORT1:
        case SID_ENGINE_PARSID_PORT2:
        case SID_ENGINE_PARSID_PORT3:
            return 0;
#endif
    }

    return 0;
}

int sid_sound_machine_channels(void)
{
    int channels = 0;

    resources_get_int("SidStereo", &channels);

    return channels + 1;
}

static void set_sound_func(void)
{
    if (sid_enable) {
        if (sid_engine_type == SID_ENGINE_FASTSID) {
            sid_read_func = sound_read;
            sid_store_func = sound_store;
        }
#ifdef HAVE_RESID
        if (sid_engine_type == SID_ENGINE_RESID) {
            sid_read_func = sound_read;
            sid_store_func = sound_store;
        }
#endif
#ifdef HAVE_CATWEASELMKIII
        if (sid_engine_type == SID_ENGINE_CATWEASELMKIII) {
            sid_read_func = catweaselmkiii_read;
            sid_store_func = catweaselmkiii_store;
        }
#endif
#ifdef HAVE_HARDSID
        if (sid_engine_type == SID_ENGINE_HARDSID) {
            sid_read_func = hardsid_read;
            sid_store_func = hardsid_store;
        }
#endif
#ifdef HAVE_PARSID
        if (sid_engine_type == SID_ENGINE_PARSID_PORT1 ||
            sid_engine_type == SID_ENGINE_PARSID_PORT2 ||
            sid_engine_type == SID_ENGINE_PARSID_PORT3) {
            sid_read_func = parsid_read;
            sid_store_func = parsid_store;
        }
#endif
    } else {
        sid_read_func = sid_read_off;
        sid_store_func = sid_write_off;
    }
}

void sid_sound_machine_enable(int enable)
{
    sid_enable = enable;

    set_sound_func();
}

int sid_engine_set(int engine)
{
#ifdef HAVE_CATWEASELMKIII
    if (engine == SID_ENGINE_CATWEASELMKIII
        && sid_engine_type != SID_ENGINE_CATWEASELMKIII) {
        if (catweaselmkiii_open() < 0) {
            return -1;
        }
    }
    if (engine != SID_ENGINE_CATWEASELMKIII
        && sid_engine_type == SID_ENGINE_CATWEASELMKIII) {
        catweaselmkiii_close();
    }
#endif
#ifdef HAVE_HARDSID
    if (engine == SID_ENGINE_HARDSID
        && sid_engine_type != SID_ENGINE_HARDSID) {
        if (hardsid_open() < 0) {
            return -1;
        }
    }
    if (engine != SID_ENGINE_HARDSID
        && sid_engine_type == SID_ENGINE_HARDSID) {
        hardsid_close();
    }
#endif
#ifdef HAVE_PARSID
    if ((engine == SID_ENGINE_PARSID_PORT1 || engine == SID_ENGINE_PARSID_PORT2 || engine == SID_ENGINE_PARSID_PORT3)
        && sid_engine_type != engine) {
        if (engine == SID_ENGINE_PARSID_PORT1) {
            if (parsid_open(1) < 0) {
                return -1;
            }
        }
        if (engine == SID_ENGINE_PARSID_PORT2) {
            if (parsid_open(2) < 0) {
                return -1;
            }
        }
        if (engine == SID_ENGINE_PARSID_PORT3) {
            if (parsid_open(3) < 0) {
                return -1;
            }
        }
    }
    if (engine != SID_ENGINE_PARSID_PORT1 && engine != SID_ENGINE_PARSID_PORT2 && engine != SID_ENGINE_PARSID_PORT3
        && (sid_engine_type == SID_ENGINE_PARSID_PORT1 || sid_engine_type == SID_ENGINE_PARSID_PORT2 || sid_engine_type == SID_ENGINE_PARSID_PORT3)) {
        parsid_close();
    }
#endif

    sid_engine_type = engine;

    set_sound_func();

    return 0;
}

void sid_state_read(unsigned int channel, sid_snapshot_state_t *sid_state)
{
    sid_engine.state_read(sound_get_psid(channel), sid_state);
}

void sid_state_write(unsigned int channel, sid_snapshot_state_t *sid_state)
{
    sid_engine.state_write(sound_get_psid(channel), sid_state);
}

void sid_set_machine_parameter(long clock_rate)
{
#ifdef HAVE_CATWEASELMKIII
    catweaselmkiii_set_machine_parameter(clock_rate);
#endif
#ifdef HAVE_HARDSID
    hardsid_set_machine_parameter(clock_rate);
#endif
}
