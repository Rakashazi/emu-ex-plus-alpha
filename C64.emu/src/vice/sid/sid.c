/** \file   sid.c
 * \brief   MOS6581 (SID) emulation, hooks to actual implementation
 *
 * \author  Teemu Rantanen <tvr@cs.hut.fi>
 * \author  Michael Schwendt <sidplay@geocities.com>
 * \author  Ettore Perazzoli <ettore@comm2000.it>
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

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "catweaselmkiii.h"
#include "fastsid.h"
#include "hardsid.h"
#include "joyport.h"
#include "lib.h"
#include "machine.h"
#include "maincpu.h"
#include "parsid.h"
#include "resources.h"
#include "sid-resources.h"
#include "sid-snapshot.h"
#include "sid.h"
#include "sound.h"
#include "ssi2001.h"
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
static uint8_t lastsidread;

/* register data */
static uint8_t siddata[SOUND_SIDS_MAX][32];

static int (*sid_read_func)(uint16_t addr, int chipno);
static void (*sid_store_func)(uint16_t addr, uint8_t val, int chipno);
static int (*sid_dump_func)(int chipno);

static int sid_enable, sid_engine_type = -1;

#ifdef HAVE_MOUSE
static CLOCK pot_cycle = 0;  /* pot sampling cycle */
static uint8_t val_pot_x = 0xff, val_pot_y = 0xff; /* last sampling value */
#endif

uint8_t *sid_get_siddata(unsigned int channel)
{
    return siddata[channel];
}

/* ------------------------------------------------------------------------- */

static int sid_read_off(uint16_t addr, int chipno)
{
    uint8_t val;

    if (addr == 0x19 || addr == 0x1a) {
        val = 0xff;
    } else {
        if (addr == 0x1b || addr == 0x1c) {
            val = (uint8_t)(maincpu_clk % 256);
        } else {
            val = 0;
        }
    }

    /* FIXME: Change API, return BYTE! */
    return (int)val;
}

static void sid_write_off(uint16_t addr, uint8_t val, int chipno)
{
}

/* ------------------------------------------------------------------------- */

static uint8_t sid_read_chip(uint16_t addr, int chipno)
{
    int val = -1;

    addr &= 0x1f;

    machine_handle_pending_alarms(0);

#ifdef HAVE_MOUSE
    if (chipno == 0 && (addr == 0x19 || addr == 0x1a)) {
        if ((maincpu_clk ^ pot_cycle) & ~511) {
            pot_cycle = maincpu_clk & ~511; /* simplistic 512 cycle sampling */

            if (_mouse_enabled) {
                mouse_poll();
            }

            val_pot_x = read_joyport_potx();
            val_pot_y = read_joyport_poty();
        }
        val = (addr == 0x19) ? val_pot_x : val_pot_y;

    } else {
#endif
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
#ifdef HAVE_MOUSE
    }
#endif

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

static uint8_t sid_peek_chip(uint16_t addr, int chipno)
{
    addr &= 0x1f;

    /* FIXME: get 0x1b and 0x1c from engine */
    return siddata[chipno][addr];
}

/* write register value to sid */
static void sid_store_chip(uint16_t addr, uint8_t byte, int chipno)
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

static int sid_dump_chip(int chipno)
{
    if (sid_dump_func) {
        return sid_dump_func(chipno);
    }

    return -1;
}

/* ------------------------------------------------------------------------- */

uint8_t sid_read(uint16_t addr)
{
    if (sid_stereo >= 1
        && addr >= sid2_address_start
        && addr < sid2_address_end) {
        return sid_read_chip(addr, 1);
    }

    if (sid_stereo >= 2
        && addr >= sid3_address_start
        && addr < sid3_address_end) {
        return sid_read_chip(addr, 2);
    }

    if (sid_stereo >= 3
        && addr >= sid4_address_start
        && addr < sid4_address_end) {
        return sid_read_chip(addr, 3);
    }

    if (sid_stereo >= 4
        && addr >= sid5_address_start
        && addr < sid5_address_end) {
        return sid_read_chip(addr, 4);
    }

    if (sid_stereo >= 5
        && addr >= sid6_address_start
        && addr < sid6_address_end) {
        return sid_read_chip(addr, 5);
    }

    if (sid_stereo >= 6
        && addr >= sid7_address_start
        && addr < sid7_address_end) {
        return sid_read_chip(addr, 6);
    }

    if (sid_stereo >= 7
        && addr >= sid8_address_start
        && addr < sid8_address_end) {
        return sid_read_chip(addr, 7);
    }

    return sid_read_chip(addr, 0);
}

#define SIDx_READ(sid_nr)                       \
    uint8_t sid##sid_nr##_read(uint16_t addr)   \
    {                                           \
        return sid_read_chip(addr, sid_nr - 1); \
    }

SIDx_READ(2)
SIDx_READ(3)
SIDx_READ(4)
SIDx_READ(5)
SIDx_READ(6)
SIDx_READ(7)
SIDx_READ(8)

uint8_t sid_peek(uint16_t addr)
{
    if (sid_stereo >= 1
        && addr >= sid2_address_start
        && addr < sid2_address_end) {
        return sid_peek_chip(addr, 1);
    }

    if (sid_stereo >= 2
        && addr >= sid3_address_start
        && addr < sid3_address_end) {
        return sid_peek_chip(addr, 2);
    }

    if (sid_stereo >= 3
        && addr >= sid4_address_start
        && addr < sid4_address_end) {
        return sid_peek_chip(addr, 3);
    }

    if (sid_stereo >= 4
        && addr >= sid5_address_start
        && addr < sid5_address_end) {
        return sid_peek_chip(addr, 4);
    }

    if (sid_stereo >= 5
        && addr >= sid6_address_start
        && addr < sid6_address_end) {
        return sid_peek_chip(addr, 5);
    }

    if (sid_stereo >= 6
        && addr >= sid7_address_start
        && addr < sid7_address_end) {
        return sid_peek_chip(addr, 6);
    }

    if (sid_stereo >= 7
        && addr >= sid8_address_start
        && addr < sid8_address_end) {
        return sid_peek_chip(addr, 7);
    }
    return sid_peek_chip(addr, 0);
}

#define SIDx_PEEK(sid_nr)                       \
    uint8_t sid##sid_nr##_peek(uint16_t addr)   \
    {                                           \
        return sid_peek_chip(addr, sid_nr - 1); \
    }

SIDx_PEEK(2)
SIDx_PEEK(3)
SIDx_PEEK(4)
SIDx_PEEK(5)
SIDx_PEEK(6)
SIDx_PEEK(7)
SIDx_PEEK(8)

void sid_store(uint16_t addr, uint8_t byte)
{
    if (sid_stereo >= 1
        && addr >= sid2_address_start
        && addr < sid2_address_end) {
        sid_store_chip(addr, byte, 1);
        return;
    }
    if (sid_stereo >= 2
        && addr >= sid3_address_start
        && addr < sid3_address_end) {
        sid_store_chip(addr, byte, 2);
        return;
    }
    if (sid_stereo >= 3
        && addr >= sid4_address_start
        && addr < sid4_address_end) {
        sid_store_chip(addr, byte, 3);
    }

    if (sid_stereo >= 4
        && addr >= sid5_address_start
        && addr < sid5_address_end) {
        sid_store_chip(addr, byte, 4);
    }

    if (sid_stereo >= 5
        && addr >= sid6_address_start
        && addr < sid6_address_end) {
        sid_store_chip(addr, byte, 5);
    }

    if (sid_stereo >= 6
        && addr >= sid7_address_start
        && addr < sid7_address_end) {
        sid_store_chip(addr, byte, 6);
    }

    if (sid_stereo >= 7
        && addr >= sid8_address_start
        && addr < sid8_address_end) {
        sid_store_chip(addr, byte, 7);
    }
    sid_store_chip(addr, byte, 0);
}

#define SIDx_STORE(sid_nr)                                \
    void sid##sid_nr##_store(uint16_t addr, uint8_t byte) \
    {                                                     \
        sid_store_chip(addr, byte, sid_nr - 1);           \
    }

SIDx_STORE(2)
SIDx_STORE(3)
SIDx_STORE(4)
SIDx_STORE(5)
SIDx_STORE(6)
SIDx_STORE(7)
SIDx_STORE(8)

int sid_dump(void)
{
    return sid_dump_chip(0);
}

#define SIDx_DUMP(sid_nr)                 \
    int sid##sid_nr##_dump(void)          \
    {                                     \
        return sid_dump_chip(sid_nr - 1); \
    }

SIDx_DUMP(2)
SIDx_DUMP(3)
SIDx_DUMP(4)
SIDx_DUMP(5)
SIDx_DUMP(6)
SIDx_DUMP(7)
SIDx_DUMP(8)

/* ------------------------------------------------------------------------- */

void sid_reset(void)
{
    sound_reset();

    memset(siddata, 0, sizeof(siddata));
}

static int sidengine;

bool sid_sound_machine_set_engine_hooks(void)
{
    sidengine = -1;

    if (resources_get_int("SidEngine", &sidengine) < 0) {
        return false;
    }

#ifdef HAVE_FASTSID
    sid_engine = fastsid_hooks;
#endif

#ifdef HAVE_RESID
    if (sidengine == SID_ENGINE_RESID) {
        sid_engine = resid_hooks;
    }
#endif
    if (sidengine >= 0) {
        return true;
    }
    return false;
}

sound_t *sid_sound_machine_open(int chipno)
{
    if (!sid_sound_machine_set_engine_hooks()) {
        return NULL;
    }

    return sid_engine.open(siddata[chipno]);
}

/* manage temporary buffers. if the requested size is smaller or equal to the
 * size of the already allocated buffer, reuse it.  */
static int16_t *buf1 = NULL;
static int16_t *buf2 = NULL;
static int16_t *buf3 = NULL;
static int16_t *buf4 = NULL;
static int16_t *buf5 = NULL;
static int16_t *buf6 = NULL;
static int16_t *buf7 = NULL;

static int blen1 = 0;
static int blen2 = 0;
static int blen3 = 0;
static int blen4 = 0;
static int blen5 = 0;
static int blen6 = 0;
static int blen7 = 0;

#define GETBUFx(nr)                                 \
    static int16_t *getbuf##nr(int len)             \
    {                                               \
        if (buf##nr != NULL) {                      \
            if (blen##nr >= len) {                  \
                /* large enough */                  \
                return buf##nr;                     \
            }                                       \
            lib_free(buf##nr);                      \
        }                                           \
        buf##nr = lib_calloc(len, sizeof(int16_t)); \
        blen##nr = len;                             \
        return buf##nr;                             \
    }


GETBUFx(1)
GETBUFx(2)
GETBUFx(3)
GETBUFx(4)
GETBUFx(5)
GETBUFx(6)
GETBUFx(7)

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
        blen1 = 0;
        buf1 = NULL;
    }
    if (buf2) {
        lib_free(buf2);
        blen2 = 0;
        buf2 = NULL;
    }
    if (buf3) {
        lib_free(buf3);
        blen3 = 0;
        buf3 = NULL;
    }
    if (buf4) {
        lib_free(buf4);
        blen4 = 0;
        buf4 = NULL;
    }
    if (buf5) {
        lib_free(buf5);
        blen5 = 0;
        buf5 = NULL;
    }
    if (buf6) {
        lib_free(buf6);
        blen6 = 0;
        buf6 = NULL;
    }
    if (buf7) {
        lib_free(buf7);
        blen7 = 0;
        buf7 = NULL;
    }
}

uint8_t sid_sound_machine_read(sound_t *psid, uint16_t addr)
{
    return sid_engine.read(psid, addr);
}

void sid_sound_machine_store(sound_t *psid, uint16_t addr, uint8_t byte)
{
    sid_engine.store(psid, addr, byte);
}

void sid_sound_machine_reset(sound_t *psid, CLOCK cpu_clk)
{
    sid_engine.reset(psid, cpu_clk);
}

int sid_sound_machine_calculate_samples(sound_t **psid, int16_t *pbuf, int nr, int soc, int scc, CLOCK *delta_t)
{
    int i;
    int16_t *tmp_buf1;
    int16_t *tmp_buf2;
    int16_t *tmp_buf3;
    int16_t *tmp_buf4;
    int16_t *tmp_buf5;
    int16_t *tmp_buf6;
    int16_t *tmp_buf7;
    int tmp_nr = 0;
    CLOCK tmp_delta_t = *delta_t;

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
    if (soc == 1 && scc == 4) {
        tmp_buf1 = getbuf1(2 * nr);
        tmp_buf2 = getbuf2(2 * nr);
        tmp_buf3 = getbuf3(2 * nr);
        tmp_nr = sid_engine.calculate_samples(psid[0], tmp_buf1, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[2], tmp_buf2, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[3], tmp_buf3, nr, 1, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf, nr, 1, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf1[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf2[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf3[i]);
        }
        return tmp_nr;
    }
    if (soc == 1 && scc == 5) {
        tmp_buf1 = getbuf1(2 * nr);
        tmp_buf2 = getbuf2(2 * nr);
        tmp_buf3 = getbuf3(2 * nr);
        tmp_buf4 = getbuf4(2 * nr);
        tmp_nr = sid_engine.calculate_samples(psid[0], tmp_buf1, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[2], tmp_buf2, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[3], tmp_buf3, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[4], tmp_buf4, nr, 1, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf, nr, 1, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf1[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf2[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf3[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf4[i]);
        }
        return tmp_nr;
    }
    if (soc == 1 && scc == 6) {
        tmp_buf1 = getbuf1(2 * nr);
        tmp_buf2 = getbuf2(2 * nr);
        tmp_buf3 = getbuf3(2 * nr);
        tmp_buf4 = getbuf4(2 * nr);
        tmp_buf5 = getbuf5(2 * nr);
        tmp_nr = sid_engine.calculate_samples(psid[0], tmp_buf1, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[2], tmp_buf2, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[3], tmp_buf3, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[4], tmp_buf4, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[5], tmp_buf5, nr, 1, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf, nr, 1, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf1[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf2[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf3[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf4[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf5[i]);
        }
        return tmp_nr;
    }
    if (soc == 1 && scc == 7) {
        tmp_buf1 = getbuf1(2 * nr);
        tmp_buf2 = getbuf2(2 * nr);
        tmp_buf3 = getbuf3(2 * nr);
        tmp_buf4 = getbuf4(2 * nr);
        tmp_buf5 = getbuf5(2 * nr);
        tmp_buf6 = getbuf6(2 * nr);
        tmp_nr = sid_engine.calculate_samples(psid[0], tmp_buf1, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[2], tmp_buf2, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[3], tmp_buf3, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[4], tmp_buf4, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[5], tmp_buf5, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[6], tmp_buf6, nr, 1, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf, nr, 1, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf1[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf2[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf3[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf4[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf5[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf6[i]);
        }
        return tmp_nr;
    }
    if (soc == 1 && scc == 8) {
        tmp_buf1 = getbuf1(2 * nr);
        tmp_buf2 = getbuf2(2 * nr);
        tmp_buf3 = getbuf3(2 * nr);
        tmp_buf4 = getbuf4(2 * nr);
        tmp_buf5 = getbuf5(2 * nr);
        tmp_buf6 = getbuf6(2 * nr);
        tmp_buf7 = getbuf7(2 * nr);
        tmp_nr = sid_engine.calculate_samples(psid[0], tmp_buf1, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[2], tmp_buf2, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[3], tmp_buf3, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[4], tmp_buf4, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[5], tmp_buf5, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[6], tmp_buf6, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[7], tmp_buf7, nr, 1, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf, nr, 1, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf1[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf2[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf3[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf4[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf5[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf6[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf7[i]);
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
    if (soc == 2 && scc == 4) {
        tmp_buf1 = getbuf1(2 * nr);
        tmp_nr = sid_engine.calculate_samples(psid[2], tmp_buf1, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[3], tmp_buf1 + 1, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[0], pbuf, nr, 2, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf + 1, nr, 2, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[i * 2] = sound_audio_mix(pbuf[i * 2], tmp_buf1[i * 2]);
            pbuf[(i * 2) + 1] = sound_audio_mix(pbuf[(i * 2) + 1], tmp_buf1[(i * 2) + 1]);
        }
    }
    if (soc == 2 && scc == 5) {
        tmp_buf1 = getbuf1(2 * nr);
        tmp_buf2 = getbuf2(2 * nr);
        tmp_nr = sid_engine.calculate_samples(psid[2], tmp_buf1, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[3], tmp_buf1 + 1, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[4], tmp_buf2, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[0], pbuf, nr, 2, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf + 1, nr, 2, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[i * 2] = sound_audio_mix(pbuf[i * 2], tmp_buf1[i * 2]);
            pbuf[i * 2] = sound_audio_mix(pbuf[i * 2], tmp_buf2[i]);
            pbuf[(i * 2) + 1] = sound_audio_mix(pbuf[(i * 2) + 1], tmp_buf1[(i * 2) + 1]);
            pbuf[(i * 2) + 1] = sound_audio_mix(pbuf[(i * 2) + 1], tmp_buf2[i]);
        }
    }
    if (soc == 2 && scc == 6) {
        tmp_buf1 = getbuf1(2 * nr);
        tmp_buf2 = getbuf2(2 * nr);
        tmp_nr = sid_engine.calculate_samples(psid[2], tmp_buf1, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[3], tmp_buf1 + 1, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[4], tmp_buf2, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[5], tmp_buf2 + 1, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[0], pbuf, nr, 2, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf + 1, nr, 2, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[i * 2] = sound_audio_mix(pbuf[i * 2], tmp_buf1[i * 2]);
            pbuf[i * 2] = sound_audio_mix(pbuf[i * 2], tmp_buf2[i * 2]);
            pbuf[(i * 2) + 1] = sound_audio_mix(pbuf[(i * 2) + 1], tmp_buf1[(i * 2) + 1]);
            pbuf[(i * 2) + 1] = sound_audio_mix(pbuf[(i * 2) + 1], tmp_buf2[(i * 2) + 1]);
        }
    }
    if (soc == 2 && scc == 7) {
        tmp_buf1 = getbuf1(2 * nr);
        tmp_buf2 = getbuf2(2 * nr);
        tmp_buf3 = getbuf3(2 * nr);
        tmp_nr = sid_engine.calculate_samples(psid[2], tmp_buf1, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[3], tmp_buf1 + 1, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[4], tmp_buf2, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[5], tmp_buf2 + 1, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[6], tmp_buf3, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[0], pbuf, nr, 2, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf + 1, nr, 2, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[i * 2] = sound_audio_mix(pbuf[i * 2], tmp_buf1[i * 2]);
            pbuf[i * 2] = sound_audio_mix(pbuf[i * 2], tmp_buf2[i * 2]);
            pbuf[i * 2] = sound_audio_mix(pbuf[i * 2], tmp_buf3[i]);
            pbuf[(i * 2) + 1] = sound_audio_mix(pbuf[(i * 2) + 1], tmp_buf1[(i * 2) + 1]);
            pbuf[(i * 2) + 1] = sound_audio_mix(pbuf[(i * 2) + 1], tmp_buf2[(i * 2) + 1]);
            pbuf[(i * 2) + 1] = sound_audio_mix(pbuf[(i * 2) + 1], tmp_buf3[i]);
        }
    }
    if (soc == 2 && scc == 8) {
        tmp_buf1 = getbuf1(2 * nr);
        tmp_buf2 = getbuf2(2 * nr);
        tmp_buf3 = getbuf3(2 * nr);
        tmp_nr = sid_engine.calculate_samples(psid[2], tmp_buf1, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[3], tmp_buf1 + 1, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[4], tmp_buf2, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[5], tmp_buf2 + 1, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[6], tmp_buf3, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[7], tmp_buf3 + 1, nr, 2, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[0], pbuf, nr, 2, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf + 1, nr, 2, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[i * 2] = sound_audio_mix(pbuf[i * 2], tmp_buf1[i * 2]);
            pbuf[i * 2] = sound_audio_mix(pbuf[i * 2], tmp_buf2[i * 2]);
            pbuf[i * 2] = sound_audio_mix(pbuf[i * 2], tmp_buf3[i * 2]);
            pbuf[(i * 2) + 1] = sound_audio_mix(pbuf[(i * 2) + 1], tmp_buf1[(i * 2) + 1]);
            pbuf[(i * 2) + 1] = sound_audio_mix(pbuf[(i * 2) + 1], tmp_buf2[(i * 2) + 1]);
            pbuf[(i * 2) + 1] = sound_audio_mix(pbuf[(i * 2) + 1], tmp_buf3[(i * 2) + 1]);
        }
    }
    return tmp_nr;
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
        case SID_ENGINE_PARSID:
            return 0;
#endif
#ifdef HAVE_SSI2001
        case SID_ENGINE_SSI2001:
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
            sid_dump_func = sound_dump;
        }
#ifdef HAVE_RESID
        if (sid_engine_type == SID_ENGINE_RESID) {
            sid_read_func = sound_read;
            sid_store_func = sound_store;
            sid_dump_func = sound_dump;
        }
#endif
#ifdef HAVE_CATWEASELMKIII
        if (sid_engine_type == SID_ENGINE_CATWEASELMKIII) {
            sid_read_func = catweaselmkiii_read;
            sid_store_func = catweaselmkiii_store;
            sid_dump_func = NULL; /* TODO: catweasel dump */
        }
#endif
#ifdef HAVE_HARDSID
        if (sid_engine_type == SID_ENGINE_HARDSID) {
            sid_read_func = hardsid_read;
            sid_store_func = hardsid_store;
            sid_dump_func = NULL; /* TODO: hardsid dump */
        }
#endif
#ifdef HAVE_PARSID
        if (sid_engine_type == SID_ENGINE_PARSID) {
            sid_read_func = parsid_read;
            sid_store_func = parsid_store;
            sid_dump_func = NULL; /* TODO: parsid dump */
        }
#endif
#ifdef HAVE_SSI2001
        if (sid_engine_type == SID_ENGINE_SSI2001) {
            sid_read_func = ssi2001_read;
            sid_store_func = ssi2001_store;
            sid_dump_func = NULL; /* TODO: hardsid dump */
        }
#endif
    } else {
        sid_read_func = sid_read_off;
        sid_store_func = sid_write_off;
        sid_dump_func = NULL;
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
    if ((engine == SID_ENGINE_PARSID)
        && sid_engine_type != engine) {
        if (parsid_open() < 0) {
            return -1;
        }
    }
    if (engine != SID_ENGINE_PARSID
        && sid_engine_type == SID_ENGINE_PARSID) {
        parsid_close();
    }
#endif
#ifdef HAVE_SSI2001
    if (engine == SID_ENGINE_SSI2001
        && sid_engine_type != SID_ENGINE_SSI2001) {
        if (ssi2001_open() < 0) {
            return -1;
        }
    }
    if (engine != SID_ENGINE_SSI2001
        && sid_engine_type == SID_ENGINE_SSI2001) {
        ssi2001_close();
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
    if (sid_engine.state_write == NULL) {
        fprintf(stderr, "%s:%d:%s(): sidengine.state_write is NULL\n",
                __FILE__, __LINE__, __func__);
    } else {
        sound_t *psid = sound_get_psid(channel);
        if (psid == NULL) {
            fprintf(stderr, "%s:%d:%s(): sound_get_psid() returned NULL\n",
                    __FILE__, __LINE__, __func__);
        } else {
            sid_engine.state_write(psid, sid_state);
        }
    }
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


/** \brief  Get maximum number of support SIDs for \a engine
 *
 * Helper function for UIs: determine number of supported SIDs to allow UIs
 * to not display impossibre settings.
 *
 * \param[in]   engine  engine ID
 *
 * \see sid.h for engine IDs
 */
int sid_engine_get_max_sids(int engine)
{
    switch (engine) {
        case SID_ENGINE_FASTSID:
            return SID_ENGINE_FASTSID_NUM_SIDS;
        case SID_ENGINE_RESID:
            return SID_ENGINE_RESID_NUM_SIDS;
       case SID_ENGINE_CATWEASELMKIII:
            return SID_ENGINE_CATWEASELMKIII_NUM_SIDS;
        case SID_ENGINE_HARDSID:
            return SID_ENGINE_HARDSID_NUM_SIDS;
        case SID_ENGINE_PARSID:
            return SID_ENGINE_PARSID_NUM_SIDS;
        case SID_ENGINE_SSI2001:
            return SID_ENGINE_SSI2001_NUM_SIDS;
        default:
            /* unknow engine */
            return -1;
    }
}


/** \brief  Get maximum number of SIDs for the current machine, using actual HW
 *
 * \return  number of SIDs
 */
int sid_machine_get_max_sids(void)
{
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_SCPU64:
            return SID_MACHINE_MAX_SID_C64;
        case VICE_MACHINE_C64DTV:
            return SID_MACHINE_MAX_SID_C64DTV;
        case VICE_MACHINE_C128:
            return SID_MACHINE_MAX_SID_C128;
        case VICE_MACHINE_VIC20:
            return SID_MACHINE_MAX_SID_VIC20;
        case VICE_MACHINE_PLUS4:
            return SID_MACHINE_MAX_SID_PLUS4;
        case VICE_MACHINE_PET:
            return SID_MACHINE_MAX_SID_PET;
        case VICE_MACHINE_CBM5x0:
            return SID_MACHINE_MAX_SID_CBM5x0;
        case VICE_MACHINE_CBM6x0:
            return SID_MACHINE_MAX_SID_CBM6x0;
        case VICE_MACHINE_VSID:
            return SID_MACHINE_MAX_SID_VSID;
        default:
            return 0;
    }
}


/** \brief  Determine maximum number of SIDs for current machine and a \engine
 *
 * Only assumes actually available HW expansion and actual old HW, so no
 * Ultimate64 etc.
 *
 * \param[in]   engine  SID engine
 *
 * \return  maximum number of SIDs
 */
int sid_machine_engine_get_max_sids(int engine)
{
    int emax = sid_engine_get_max_sids(engine);
    int mmax = sid_machine_get_max_sids();

    return emax < mmax ? emax : mmax;
}


/** \brief  Determine if the current machine can support multiple SIDs
 *
 * This ignores any expansions and the SID engine, just gives the theoretical
 * maximum that can be done with the current HW available.
 *
 * \return  boolean
 */
int sid_machine_can_have_multiple_sids(void)
{
    return sid_machine_get_max_sids() > 1;
}
