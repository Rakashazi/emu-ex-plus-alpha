/*
 * vic20sound.c - Implementation of VIC20 sound code.
 *
 * Written by
 *  Rami Rasanen <raipsu@users.sf.net>
 *  Ville-Matias Heikkila <viznut@iki.fi>
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
#include <stdlib.h>
#include <string.h>

#include "fastsid.h"
#include "lib.h"
#include "maincpu.h"
#include "sid.h"
#include "sidcart.h"
#include "sid-resources.h"
#include "sound.h"
#include "types.h"
#include "vic20sound.h"
#include "vic20.h"

/* ---------------------------------------------------------------------*/

/* Some prototypes are needed */
static int vic_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec);
static int vic_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int sound_output_channels, int sound_chip_channels, int *delta_t);
static void vic_sound_machine_store(sound_t *psid, WORD addr, BYTE value);
static BYTE vic_sound_machine_read(sound_t *psid, WORD addr);

static int vic_sound_machine_cycle_based(void)
{
    return 1;
}

static int vic_sound_machine_channels(void)
{
    return 1;
}

static sound_chip_t vic_sound_chip = {
    NULL, /* no open */
    vic_sound_machine_init,
    NULL, /* no close */
    vic_sound_machine_calculate_samples,
    vic_sound_machine_store,
    vic_sound_machine_read,
    vic_sound_reset,
    vic_sound_machine_cycle_based,
    vic_sound_machine_channels,
    1 /* chip enabled */
};

static WORD vic_sound_chip_offset = 0;

void vic_sound_chip_init(void)
{
    vic_sound_chip_offset = sound_chip_register(&vic_sound_chip);
}

/* ---------------------------------------------------------------------*/

static BYTE noisepattern[1024] = {
      7, 30, 30, 28, 28, 62, 60, 56,120,248,124, 30, 31,143,  7,  7,193,192,224,
    241,224,240,227,225,192,224,120,126, 60, 56,224,225,195,195,135,199,  7, 30,
     28, 31, 14, 14, 30, 14, 15, 15,195,195,241,225,227,193,227,195,195,252, 60,
     30, 15,131,195,193,193,195,195,199,135,135,199, 15, 14, 60,124,120, 60, 60,
     60, 56, 62, 28,124, 30, 60, 15, 14, 62,120,240,240,224,225,241,193,195,199,
    195,225,241,224,225,240,241,227,192,240,224,248,112,227,135,135,192,240,224,
    241,225,225,199,131,135,131,143,135,135,199,131,195,131,195,241,225,195,199,
    129,207,135,  3,135,199,199,135,131,225,195,  7,195,135,135,  7,135,195,135,
    131,225,195,199,195,135,135,143, 15,135,135, 15,207, 31,135,142, 14,  7,129,
    195,227,193,224,240,224,227,131,135,  7,135,142, 30, 15,  7,135,143, 31,  7,
    135,193,240,225,225,227,199, 15,  3,143,135, 14, 30, 30, 15,135,135, 15,135,
     31, 15,195,195,240,248,240,112,241,240,240,225,240,224,120,124,120,124,112,
    113,225,225,195,195,199,135, 28, 60, 60, 28, 60,124, 30, 30, 30, 28, 60,120,
    248,248,225,195,135, 30, 30, 60, 62, 15, 15,135, 31,142, 15, 15,142, 30, 30,
     30, 30, 15, 15,143,135,135,195,131,193,225,195,193,195,199,143, 15, 15, 15,
     15,131,199,195,193,225,224,248, 62, 60, 60, 60, 60, 60,120, 62, 30, 30, 30,
     15, 15, 15, 30, 14, 30, 30, 15, 15,135, 31,135,135, 28, 62, 31, 15, 15,142,
     62, 14, 62, 30, 28, 60,124,252, 56,120,120, 56,120,112,248,124, 30, 60, 60,
     48,241,240,112,112,224,248,240,248,120,120,113,225,240,227,193,240,113,227,
    199,135,142, 62, 14, 30, 62, 15,  7,135, 12, 62, 15,135, 15, 30, 60, 60, 56,
    120,241,231,195,195,199,142, 60, 56,240,224,126, 30, 62, 14, 15, 15, 15,  3,
    195,195,199,135, 31, 14, 30, 28, 60, 60, 15,  7,  7,199,199,135,135,143, 15,
    192,240,248, 96,240,240,225,227,227,195,195,195,135, 15,135,142, 30, 30, 63,
     30, 14, 28, 60,126, 30, 60, 56,120,120,120, 56,120, 60,225,227,143, 31, 28,
    120,112,126, 15,135,  7,195,199, 15, 30, 60, 14, 15, 14, 30,  3,240,240,241,
    227,193,199,192,225,225,225,225,224,112,225,240,120,112,227,199, 15,193,225,
    227,195,192,240,252, 28, 60,112,248,112,248,120, 60,112,240,120,112,124,124,
     60, 56, 30, 62, 60,126,  7,131,199,193,193,225,195,195,195,225,225,240,120,
    124, 62, 15, 31,  7,143, 15,131,135,193,227,227,195,195,225,240,248,240, 60,
    124, 60, 15,142, 14, 31, 31, 14, 60, 56,120,112,112,240,240,248,112,112,120,
     56, 60,112,224,240,120,241,240,120, 62, 60, 15,  7, 14, 62, 30, 63, 30, 14,
     15,135,135,  7, 15,  7,199,143, 15,135, 30, 30, 31, 30, 30, 60, 30, 28, 62,
     15,  3,195,129,224,240,252, 56, 60, 62, 14, 30, 28,124, 30, 31, 14, 62, 28,
    120,120,124, 30, 62, 30, 60, 31, 15, 31, 15, 15,143, 28, 60,120,248,240,248,
    112,240,120,120, 60, 60,120, 60, 31, 15,  7,134, 28, 30, 28, 30, 30, 31,  3,
    195,199,142, 60, 60, 28, 24,240,225,195,225,193,225,227,195,195,227,195,131,
    135,131,135, 15,  7,  7,225,225,224,124,120, 56,120,120, 60, 31, 15,143, 14,
      7, 15,  7,131,195,195,129,240,248,241,224,227,199, 28, 62, 30, 15, 15,195,
    240,240,227,131,195,199,  7, 15, 15, 15, 15, 15,  7,135, 15, 15, 14, 15, 15,
     30, 15, 15,135,135,135,143,199,199,131,131,195,199,143,135,  7,195,142, 30,
     56, 62, 60, 56,124, 31, 28, 56, 60,120,124, 30, 28, 60, 63, 30, 14, 62, 28,
     60, 31, 15,  7,195,227,131,135,129,193,227,207, 14, 15, 30, 62, 30, 31, 15,
    143,195,135, 14,  3,240,240,112,224,225,225,199,142, 15, 15, 30, 14, 30, 31,
     28,120,240,241,241,224,241,225,225,224,224,241,193,240,113,225,195,131,199,
    131,225,225,248,112,240,240,240,240,240,112,248,112,112, 97,224,240,225,224,
    120,113,224,240,248, 56, 30, 28, 56,112,248, 96,120, 56, 60, 63, 31, 15, 31,
     15, 31,135,135,131,135,131,225,225,240,120,241,240,112, 56, 56,112,224,227,
    192,224,248,120,120,248, 56,241,225,225,195,135,135, 14, 30, 31, 14, 14, 15,
     15,135,195,135,  7,131,192,240, 56, 60, 60, 56,240,252, 62, 30, 28, 28, 56,
    112,240,241,224,240,224,224,241,227,224,225,240,240,120,124,120, 60,120,120,
     56,120,120,120,120,112,227,131,131,224,195,193,225,193,193,193,227,195,199,
     30, 14, 31, 30, 30, 15, 15, 14, 14, 14,  7,131,135,135, 14,  7,143, 15, 15,
     15, 14, 28,112,225,224,113,193,131,131,135, 15, 30, 24,120,120,124, 62, 28,
     56,240,225,224,120,112, 56, 60, 62, 30, 60, 30, 28,112, 60, 56, 63
};

static float voltagefunction[] = {
        0.00f,   148.28f,   296.55f,   735.97f,   914.88f,  1126.89f,  1321.86f,  1503.07f,  1603.50f,
     1758.00f,  1913.98f,  2070.94f,  2220.36f,  2342.91f,  2488.07f,  3188.98f,  3285.76f,  3382.53f,
     3479.31f,  3576.08f,  3672.86f,  3769.63f,  3866.41f,  3963.18f,  4059.96f,  4248.10f,  4436.24f,
     4624.38f,  4812.53f,  5000.67f,  5188.81f,  5192.91f,  5197.00f,  5338.52f,  5480.04f,  5621.56f,
     5763.07f,  5904.59f,  6046.11f,  6187.62f,  6329.14f,  6609.31f,  6889.47f,  7169.64f,  7449.80f,
     7729.97f,  7809.36f,  7888.75f,  7968.13f,  8047.52f,  8126.91f,  8206.30f,  8285.69f,  8365.07f,
     8444.46f,  8523.85f,  8603.24f,  8905.93f,  9208.63f,  9511.32f,  9814.02f,  9832.86f,  9851.70f,
     9870.54f,  9889.38f,  9908.22f,  9927.07f,  9945.91f,  9964.75f,  9983.59f, 10002.43f, 10021.27f,
    10040.12f, 10787.23f, 11534.34f, 12281.45f, 12284.98f, 12288.50f, 12292.03f, 12295.56f, 12299.09f,
    12302.62f, 12306.15f, 12309.68f, 12313.21f, 12316.74f, 12320.26f, 12323.79f, 12327.32f, 13113.05f,
    13898.78f, 13910.58f, 13922.39f, 13934.19f, 13945.99f, 13957.80f, 13969.60f, 13981.40f, 13993.21f,
    14005.01f, 14016.81f, 14028.62f, 14040.42f, 14052.22f, 14064.03f, 16926.31f, 16987.04f, 17047.77f,
    17108.50f, 17169.23f, 17229.96f, 17290.69f, 17351.42f, 17412.15f, 17472.88f, 17533.61f, 17594.34f,
    17655.07f, 17715.80f, 17776.53f, 17837.26f, 18041.51f, 18245.77f, 18450.02f, 18654.28f, 18858.53f,
    19062.78f, 19267.04f, 19471.29f, 19675.55f, 19879.80f, 20084.05f, 20288.31f, 20417.74f, 20547.17f,
    20676.61f, 20774.26f, 20871.91f, 20969.55f, 21067.20f, 21164.85f, 21262.50f, 21360.15f, 21457.80f,
    21555.45f, 21653.09f, 21750.74f, 21848.39f, 21946.04f, 22043.69f, 22141.34f, 22212.33f, 22283.33f,
    22354.33f, 22425.33f, 22496.32f, 22567.32f, 22638.32f, 22709.32f, 22780.31f, 22851.31f, 22922.31f,
    22993.31f, 23064.30f, 23135.30f, 23206.30f, 23255.45f, 23304.60f, 23353.75f, 23402.91f, 23452.06f,
    23501.21f, 23550.36f, 23599.51f, 23648.67f, 23768.81f, 23888.96f, 24009.11f, 24129.26f, 24249.41f,
    24369.56f, 24451.92f, 24534.28f, 24616.63f, 24698.99f, 24781.35f, 24863.70f, 24946.06f, 25028.42f,
    25110.77f, 25193.13f, 25275.49f, 25357.84f, 25440.20f, 25522.56f, 25604.92f, 25658.87f, 25712.83f,
    25766.79f, 25820.75f, 25874.71f, 25928.66f, 25982.62f, 26036.58f, 26090.54f, 26144.49f, 26198.45f,
    26252.41f, 26306.37f, 26360.33f, 26414.28f, 26501.23f, 26588.17f, 26675.12f, 26762.06f, 26849.01f,
    26935.95f, 27022.90f, 27109.84f, 27196.78f, 27283.73f, 27370.67f, 27457.62f, 27544.56f, 27631.51f,
    27718.45f, 27726.89f, 27735.33f, 27743.78f, 27752.22f, 27760.66f, 27769.10f, 27777.54f, 27785.98f,
    27794.43f, 27802.87f, 27811.31f, 27819.75f, 27828.19f, 27836.63f, 27845.08f, 27853.52f, 27861.96f,
    27870.40f, 27878.84f, 27887.28f, 27895.73f, 27904.17f, 27912.61f, 27921.05f, 27929.49f, 27937.93f,
    27946.38f, 27954.82f, 27963.26f, 27971.70f, 27980.14f, 27988.58f, 27997.03f, 28005.47f, 28013.91f,
    28022.35f, 28030.79f, 28039.23f, 28047.68f, 28056.12f, 28064.56f, 28073.00f, 28081.44f, 28089.88f,
    28098.33f, 28106.77f, 28115.21f, 28123.65f, 28132.09f, 28140.53f, 28148.98f, 28157.42f, 28165.86f,
    28174.30f, 28182.74f, 28191.18f, 28199.63f, 28208.07f, 28216.51f, 28224.95f, 28233.39f, 28241.83f,
    28250.28f, 28258.72f, 28267.16f, 28275.60f, 28284.04f, 28292.48f, 28300.93f, 28309.37f, 28317.81f,
    28326.25f, 28334.69f, 28343.13f, 28351.58f, 28360.02f, 28368.46f, 28376.90f, 28385.34f, 28393.78f,
    28402.23f, 28410.67f, 28419.11f, 28427.55f, 28435.99f, 28444.43f, 28452.88f, 28461.32f, 28469.76f,
    28478.20f, 28486.64f, 28495.08f, 28503.53f, 28511.97f, 28520.41f, 28528.85f, 28537.29f, 28545.73f,
    28554.18f, 28562.62f, 28571.06f, 28579.50f, 28587.94f, 28596.38f, 28604.83f, 28613.27f, 28621.71f,
    28630.15f, 28638.59f, 28647.03f, 28655.48f, 28663.92f, 28672.36f, 28680.80f, 28689.24f, 28697.68f,
    28706.13f, 28714.57f, 28723.01f, 28731.45f, 28739.89f, 28748.33f, 28756.78f, 28765.22f, 28773.66f,
    28782.10f, 28790.54f, 28798.98f, 28807.43f, 28815.87f, 28824.31f, 28832.75f, 28841.19f, 28849.63f,
    28858.08f, 28866.52f, 28874.96f, 28883.40f, 28891.84f, 28900.28f, 28908.73f, 28917.17f, 28925.61f,
    28934.05f, 28942.49f, 28950.93f, 28959.38f, 28967.82f, 28976.26f, 28984.70f, 28993.14f, 29001.58f,
    29010.03f, 29018.47f, 29026.91f, 29035.35f, 29043.79f, 29052.23f, 29060.68f, 29069.12f, 29077.56f,
    29086.00f, 29094.44f, 29102.88f, 29111.33f, 29119.77f, 29128.21f, 29136.65f, 29145.09f, 29153.53f,
    29161.98f, 29170.42f, 29178.86f, 29187.30f, 29195.74f, 29204.18f, 29212.63f, 29221.07f, 29229.51f,
    29237.95f, 29246.39f, 29254.83f, 29263.28f, 29271.72f, 29280.16f, 29288.60f, 29297.04f, 29305.48f,
    29313.93f, 29322.37f, 29330.81f, 29339.25f, 29347.69f, 29356.13f, 29364.58f, 29373.02f, 29381.46f,
    29389.90f, 29398.34f, 29406.78f, 29415.23f, 29423.67f, 29432.11f, 29440.55f, 29448.99f, 29457.43f,
    29465.88f, 29474.32f, 29482.76f, 29491.20f
};

static BYTE vic20_sound_data[16];

/* dummy function for now */
int machine_sid2_check_range(unsigned int sid2_adr)
{
    return 0;
}

/* dummy function for now */
int machine_sid3_check_range(unsigned int sid3_adr)
{
    return 0;
}

void machine_sid2_enable(int val)
{
}

struct sound_vic20_s {
    unsigned char div;
    struct {
        unsigned char out;
        unsigned char reg;
        unsigned char shift;
        signed short ctr;
    } ch[4];
    unsigned short noisectr;
    unsigned char volume;
    int cyclecount;

    int accum;
    int accum_cycles;

    float cycles_per_sample;
    float leftover_cycles;
    int speed;

    float highpassbuf;
    float highpassbeta;
    float lowpassbuf;
    float lowpassbeta;
};
typedef struct sound_vic20_s sound_vic20_t;

static struct sound_vic20_s snd;

void vic_sound_clock(int cycles);

static int vic_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int soc, int scc, int *delta_t)
{
    int s = 0;
    int i;
    float o;
    SWORD vicbuf;
    int samples_to_do;

    while (s < nr && *delta_t >= snd.cycles_per_sample - snd.leftover_cycles) {
        samples_to_do = (int)(snd.cycles_per_sample - snd.leftover_cycles);
        snd.leftover_cycles += samples_to_do - snd.cycles_per_sample;
        vic_sound_clock(samples_to_do);

        o = voltagefunction[(((snd.accum * 7) / snd.accum_cycles) + 1) * snd.volume];
        o = snd.lowpassbuf * snd.lowpassbeta + o * (1.0f - snd.lowpassbeta); /* 0.75f + o*0.25f; */
        snd.lowpassbuf = o;
        o -= snd.highpassbuf;
        snd.highpassbuf += o * snd.highpassbeta;

        if (o < -32768) {
            vicbuf = -32768;
        } else if (o > 32767) {
            vicbuf = 32767;
        } else {
            vicbuf = (SWORD)o;
        }

        for (i = 0; i < soc; i++) {
            pbuf[(s * soc) + i] = vicbuf;
        }
        s++;
        snd.accum = 0;
        snd.accum_cycles = 0;
        *delta_t -= samples_to_do;
    }
    if (*delta_t > 0) {
        snd.leftover_cycles += *delta_t;
        vic_sound_clock(*delta_t);
        *delta_t = 0;
    }
    return s;
}

void vic_sound_reset(sound_t *psid, CLOCK cpu_clk)
{
    WORD i;

    for (i = 10; i < 15; i++) {
        vic_sound_store(i, 0);
    }
}

void vic_sound_store(WORD addr, BYTE value)
{
    addr &= 0x0f;
    vic20_sound_data[addr] = value;

    sound_store((WORD)(vic_sound_chip_offset | addr), value, 0);
}


void vic_sound_clock(int cycles)
{
    int i, j;

    if (cycles <= 0) {
        return;
    }

    for (j = 0; j < 3; j++) {
        int chspeed = "\4\3\2"[j];

        if (snd.ch[j].ctr > cycles) {
            snd.accum += snd.ch[j].out * cycles;
            snd.ch[j].ctr -= cycles;
        } else {
            for (i = cycles; i; i--) {
                snd.ch[j].ctr--;
                if (snd.ch[j].ctr <= 0) {
                    int a = (~snd.ch[j].reg) & 127;
                    a = a ? a : 128;
                    snd.ch[j].ctr += a << chspeed;
                    if (snd.ch[j].reg & 128) {
                        unsigned char shift = snd.ch[j].shift;
                        shift = ((shift << 1) | ((shift & 128) >> 7)) ^ 1;
                        snd.ch[j].shift = shift;
                        snd.ch[j].out = shift & 1;
                    } else {
                        snd.ch[j].shift <<= 1;
                        snd.ch[j].out = 0;
                    }
                }
                snd.accum += snd.ch[j].out;
            }
        }
    }

    if (snd.ch[3].ctr > cycles) {
        snd.accum += snd.ch[3].out * cycles;
        snd.ch[3].ctr -= cycles;
    } else {
        for (i = cycles; i; i--) {
            snd.ch[3].ctr--;
            if (snd.ch[3].ctr <= 0) {
                int a = (~snd.ch[3].reg) & 127;
                a = a ? a : 128;
                snd.ch[3].ctr += a << 4;
                if (snd.ch[3].reg & 128) {
                    snd.ch[3].out = (noisepattern[(snd.noisectr >> 3) & 1023] >> (snd.noisectr & 7)) & 1;
                } else {
                    snd.ch[3].out = 0;
                }
                snd.noisectr++;
            }
            snd.accum += snd.ch[3].out;
        }
    }

    snd.accum_cycles += cycles;
}

static void vic_sound_machine_store(sound_t *psid, WORD addr, BYTE value)
{
    switch (addr) {
        case 0xA:
            snd.ch[0].reg = value;
            break;
        case 0xB:
            snd.ch[1].reg = value;
            break;
        case 0xC:
            snd.ch[2].reg = value;
            break;
        case 0xD:
            snd.ch[3].reg = value;
            break;
        case 0xE:
            snd.volume = value & 0x0f;
            break;
    }
}

static int vic_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
    DWORD i;

    memset((unsigned char*)&snd, 0, sizeof(snd));

    snd.cycles_per_sample = (float)cycles_per_sec / speed;
    snd.leftover_cycles = 0.0f;

    snd.speed = speed;

    snd.lowpassbeta = 1.0f - snd.cycles_per_sample / ( snd.cycles_per_sample + 62.0f );
    snd.highpassbeta = 1.0f - snd.cycles_per_sample / ( snd.cycles_per_sample + 0.04f );

    for (i = 0; i < 16; i++) {
        vic_sound_machine_store(psid, (WORD)i, vic20_sound_data[i]);
    }

    return 1;
}

static BYTE vic_sound_machine_read(sound_t *psid, WORD addr)
{
    return 0;
}

void sound_machine_prevent_clk_overflow(sound_t *psid, CLOCK sub)
{
    sid_sound_machine_prevent_clk_overflow(psid, sub);
}

char *sound_machine_dump_state(sound_t *psid)
{
    return sid_sound_machine_dump_state(psid);
}

void sound_machine_enable(int enable)
{
    sid_sound_machine_enable(enable);
}
