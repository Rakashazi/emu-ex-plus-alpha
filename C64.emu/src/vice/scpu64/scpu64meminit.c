/*
 * scpu64meminit.c -- Initialize SCPU64 memory.
 *
 * Written by
 *  Kajtar Zsolt <soci@c64.rulez.org>
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

#include "c64cart.h"
#include "c64cartmem.h"
#include "c64cia.h"
#include "scpu64mem.h"
#include "scpu64rom.h"
#include "scpu64meminit.h"
#include "cartio.h"
#include "sid.h"
#include "vicii-mem.h"

/*

 missing: BA,CAS(not needed),RW(through tables),AEC

 bit 7 - boot
 bit 6 - dos
 bit 5 - hw
 bit 4 - !game
 bit 3 - !exrom
 bit 2 - loram
 bit 1 - hiram
 bit 0 - charen

*/

enum {
    R0, /* SRAM bank 0 */
    R1, /* SRAM bank 1 */
    RC, /* RAM */
    UM, /* Ultimax */
    RL, /* ROML */
    RH, /* ROML */
    IO, /* I/O */
    CR, /* CHARROM */
    F8, /* EPROM */
    KS, /* Kernal shadow */
    KT, /* Kernal trap */
    CO, /* Color RAM */
    OP, /* Internal Color RAM */
};

#define AREAS 10
static const unsigned int areas[AREAS][2] = {
    { 0x00, 0x0f },
    { 0x10, 0x5f },
    { 0x60, 0x7f },
    { 0x80, 0x9f },
    { 0xa0, 0xbf },
    { 0xc0, 0xcf },
    { 0xd0, 0xd7 },
    { 0xd8, 0xdb },
    { 0xdc, 0xdf },
    { 0xe0, 0xff }
};

static const BYTE config[AREAS][256] = 
{
    { /* 0000-0fff */
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        RC, RC, RC, RC, RC, RC, RC, RC,  RC, RC, RC, RC, RC, RC, RC, RC,
        RC, RC, RC, RC, RC, RC, RC, RC,  RC, RC, RC, RC, RC, RC, RC, RC },
    {  /* 1000-5fff */
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R1, R1, R1, R1, R1, R1, R1, R1,  R1, R1, R1, R1, R1, R1, R1, R1,
        R1, R1, R1, R1, R1, R1, R1, R1,  R1, R1, R1, R1, R1, R1, R1, R1,
        R1, R1, R1, R1, R1, R1, R1, R1,  R1, R1, R1, R1, R1, R1, R1, R1,
        R1, R1, R1, R1, R1, R1, R1, R1,  R1, R1, R1, R1, R1, R1, R1, R1,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R1, R1, R1, R1, R1, R1, R1, R1,  R1, R1, R1, R1, R1, R1, R1, R1,
        R1, R1, R1, R1, R1, R1, R1, R1,  R1, R1, R1, R1, R1, R1, R1, R1,
        RC, RC, RC, RC, RC, RC, RC, RC,  RC, RC, RC, RC, RC, RC, RC, RC,
        UM, UM, UM, UM, UM, UM, UM, UM,  RC, RC, RC, RC, RC, RC, RC, RC },
    {  /* 6000-7fff */
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        RC, RC, RC, RC, RC, RC, RC, RC,  RC, RC, RC, RC, RC, RC, RC, RC,
        UM, UM, UM, UM, UM, UM, UM, UM,  RC, RC, RC, RC, RC, RC, RC, RC },
    {  /* 8000-9fff */
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, RL, R0, R0, R0, RL,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, RL, R0, R0, R0, RL,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, RL, R0, R0, R0, RL,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, RL, R0, R0, R0, RL,
        R1, R1, R1, R1, R1, R1, R1, R1,  R1, R1, R1, R1, R1, R1, R1, R1,
        R1, R1, R1, R1, R1, R1, R1, R1,  R1, R1, R1, R1, R1, R1, R1, R1,
        R1, R1, R1, R1, R1, R1, R1, R1,  R1, R1, R1, R1, R1, R1, R1, R1,
        R1, R1, R1, R1, R1, R1, R1, R1,  R1, R1, R1, R1, R1, R1, R1, R1,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        RC, RC, RC, RC, RC, RC, RC, RC,  RL, RL, RL, RL, RL, RL, RL, RL,
        UM, UM, UM, UM, UM, UM, UM, UM,  RL, RL, RL, RL, RL, RL, RL, RL },
    {  /* a000-bfff */
        R0, R0, R0, R1, R0, R0, R0, R1,  R0, R0, R0, R1, R0, R0, R0, R1,
        R0, R0, R0, R1, R0, R0, R0, R1,  R0, R0, RH, RH, R0, R0, RH, RH,
        R0, R0, R0, R1, R0, R0, R0, R1,  R0, R0, R0, R1, R0, R0, R0, R1,
        R0, R0, R0, R1, R0, R0, R0, R1,  R0, R0, RH, RH, R0, R0, RH, RH,
        R0, R0, R0, R1, R0, R0, R0, R1,  R0, R0, R0, R1, R0, R0, R0, R1,
        R0, R0, R0, R1, R0, R0, R0, R1,  R0, R0, RH, RH, R0, R0, RH, RH,
        R0, R0, R0, R1, R0, R0, R0, R1,  R0, R0, R0, R1, R0, R0, R0, R1,
        R0, R0, R0, R1, R0, R0, R0, R1,  R0, R0, RH, RH, R0, R0, RH, RH,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        UM, UM, UM, UM, UM, UM, UM, UM,  RH, RH, RH, RH, RH, RH, RH, RH },
    {  /* c000-cfff */
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        R0, R0, R0, R0, R0, R0, R0, R0,  R0, R0, R0, R0, R0, R0, R0, R0,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        RC, RC, RC, RC, RC, RC, RC, RC,  RC, RC, RC, RC, RC, RC, RC, RC,
        UM, UM, UM, UM, UM, UM, UM, UM,  RC, RC, RC, RC, RC, RC, RC, RC },
    {  /* d000-d7ff */
        R0, CR, CR, CR, R0, IO, IO, IO,  R0, CR, CR, CR, R0, IO, IO, IO,
        R0, CR, CR, CR, R0, IO, IO, IO,  R0, R0, CR, CR, R0, IO, IO, IO,
        R0, CR, CR, CR, R0, IO, IO, IO,  R0, CR, CR, CR, R0, IO, IO, IO,
        R0, CR, CR, CR, R0, IO, IO, IO,  R0, R0, CR, CR, R0, IO, IO, IO,
        R0, CR, CR, CR, R0, IO, IO, IO,  R0, CR, CR, CR, R0, IO, IO, IO,
        R0, CR, CR, CR, R0, IO, IO, IO,  R0, R0, CR, CR, R0, IO, IO, IO,
        R0, CR, CR, CR, R0, IO, IO, IO,  R0, CR, CR, CR, R0, IO, IO, IO,
        R0, CR, CR, CR, R0, IO, IO, IO,  R0, R0, CR, CR, R0, IO, IO, IO,
        F8, CR, CR, CR, F8, IO, IO, IO,  F8, CR, CR, CR, F8, IO, IO, IO,
        F8, CR, CR, CR, F8, IO, IO, IO,  F8, F8, CR, CR, F8, IO, IO, IO,
        F8, CR, CR, CR, F8, IO, IO, IO,  F8, CR, CR, CR, F8, IO, IO, IO,
        F8, CR, CR, CR, F8, IO, IO, IO,  F8, F8, CR, CR, F8, IO, IO, IO,
        F8, CR, CR, CR, F8, IO, IO, IO,  F8, CR, CR, CR, F8, IO, IO, IO,
        F8, CR, CR, CR, F8, IO, IO, IO,  F8, F8, CR, CR, F8, IO, IO, IO,
        CR, CR, CR, CR, CR, IO, IO, IO,  CR, CR, CR, CR, CR, IO, IO, IO,
        CR, CR, CR, CR, CR, IO, IO, IO,  CR, CR, CR, CR, CR, IO, IO, IO },
    {  /* d800-dbff */
        R0, CR, CR, CR, R0, CO, CO, CO,  R0, CR, CR, CR, R0, CO, CO, CO,
        R0, CR, CR, CR, R0, CO, CO, CO,  R0, R0, CR, CR, R0, CO, CO, CO,
        R0, CR, CR, CR, R0, CO, CO, CO,  R0, CR, CR, CR, R0, CO, CO, CO,
        R0, CR, CR, CR, R0, CO, CO, CO,  R0, R0, CR, CR, R0, CO, CO, CO,
        R0, CR, CR, CR, R0, CO, CO, CO,  R0, CR, CR, CR, R0, CO, CO, CO,
        R0, CR, CR, CR, R0, CO, CO, CO,  R0, R0, CR, CR, R0, CO, CO, CO,
        R0, CR, CR, CR, R0, CO, CO, CO,  R0, CR, CR, CR, R0, CO, CO, CO,
        R0, CR, CR, CR, R0, CO, CO, CO,  R0, R0, CR, CR, R0, CO, CO, CO,
        F8, CR, CR, CR, F8, CO, CO, CO,  F8, CR, CR, CR, F8, CO, CO, CO,
        F8, CR, CR, CR, F8, CO, CO, CO,  F8, F8, CR, CR, F8, CO, CO, CO,
        F8, CR, CR, CR, F8, CO, CO, CO,  F8, CR, CR, CR, F8, CO, CO, CO,
        F8, CR, CR, CR, F8, CO, CO, CO,  F8, F8, CR, CR, F8, CO, CO, CO,
        F8, CR, CR, CR, F8, CO, CO, CO,  F8, CR, CR, CR, F8, CO, CO, CO,
        F8, CR, CR, CR, F8, CO, CO, CO,  F8, F8, CR, CR, F8, CO, CO, CO,
        CR, CR, CR, CR, CR, OP, OP, OP,  CR, CR, CR, CR, CR, OP, OP, OP,
        CR, CR, CR, CR, CR, OP, OP, OP,  CR, CR, CR, CR, CR, OP, OP, OP },
    {  /* dc00-dfff */
        R0, CR, CR, CR, R0, IO, IO, IO,  R0, CR, CR, CR, R0, IO, IO, IO,
        R0, CR, CR, CR, R0, IO, IO, IO,  R0, R0, CR, CR, R0, IO, IO, IO,
        R0, CR, CR, CR, R0, IO, IO, IO,  R0, CR, CR, CR, R0, IO, IO, IO,
        R0, CR, CR, CR, R0, IO, IO, IO,  R0, R0, CR, CR, R0, IO, IO, IO,
        R0, CR, CR, CR, R0, IO, IO, IO,  R0, CR, CR, CR, R0, IO, IO, IO,
        R0, CR, CR, CR, R0, IO, IO, IO,  R0, R0, CR, CR, R0, IO, IO, IO,
        R0, CR, CR, CR, R0, IO, IO, IO,  R0, CR, CR, CR, R0, IO, IO, IO,
        R0, CR, CR, CR, R0, IO, IO, IO,  R0, R0, CR, CR, R0, IO, IO, IO,
        F8, CR, CR, CR, F8, IO, IO, IO,  F8, CR, CR, CR, F8, IO, IO, IO,
        F8, CR, CR, CR, F8, IO, IO, IO,  F8, F8, CR, CR, F8, IO, IO, IO,
        F8, CR, CR, CR, F8, IO, IO, IO,  F8, CR, CR, CR, F8, IO, IO, IO,
        F8, CR, CR, CR, F8, IO, IO, IO,  F8, F8, CR, CR, F8, IO, IO, IO,
        F8, CR, CR, CR, F8, IO, IO, IO,  F8, CR, CR, CR, F8, IO, IO, IO,
        F8, CR, CR, CR, F8, IO, IO, IO,  F8, F8, CR, CR, F8, IO, IO, IO,
        CR, CR, CR, CR, CR, IO, IO, IO,  CR, CR, CR, CR, CR, IO, IO, IO,
        CR, CR, CR, CR, CR, IO, IO, IO,  CR, CR, CR, CR, CR, IO, IO, IO },
    {  /* e000-ffff */
        R0, R0, KT, KT, R0, R0, KT, KT,  R0, R0, KT, KT, R0, R0, KT, KT,
        R0, R0, KT, KT, R0, R0, KT, KT,  R0, R0, KT, KT, R0, R0, KT, KT,
        R0, R0, KS, KS, R0, R0, KS, KS,  R0, R0, KS, KS, R0, R0, KS, KS,
        R0, R0, KS, KS, R0, R0, KS, KS,  R0, R0, KS, KS, R0, R0, KS, KS,
        R0, R0, KT, KT, R0, R0, KT, KT,  R0, R0, KT, KT, R0, R0, KT, KT,
        R0, R0, KT, KT, R0, R0, KT, KT,  R0, R0, KT, KT, R0, R0, KT, KT,
        R0, R0, KS, KS, R0, R0, KS, KS,  R0, R0, KS, KS, R0, R0, KS, KS,
        R0, R0, KS, KS, R0, R0, KS, KS,  R0, R0, KS, KS, R0, R0, KS, KS,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        F8, F8, F8, F8, F8, F8, F8, F8,  F8, F8, F8, F8, F8, F8, F8, F8,
        UM, UM, UM, UM, UM, UM, UM, UM,  F8, F8, F8, F8, F8, F8, F8, F8 }
};

void scpu64meminit(void)
{
    unsigned int i, j, k;

    for (i = 0; i < AREAS; i++) {
        for (j = areas[i][0]; j <= areas[i][1]; j++) {
            for (k = 0; k < 256; k++) {
                switch (config[i][k]) {
                case R0:
                    mem_read_tab_set(k, j, ram_read);
                    mem_read_base_set(k, j, mem_sram);
                    /* write hook preset, ram */
                    break;
                case R1:
                    mem_read_tab_set(k, j, ram1_read);
                    mem_read_base_set(k, j, mem_sram + 0x10000);
                    /* write hook preset, ram */
                    break;
                case KT:
                    mem_read_tab_set(k, j, ram1_read);
                    mem_read_base_set(k, j, mem_trap_ram - 0xe000);
                    /* write hook preset, ram */
                    break;
                case KS:
                    mem_read_tab_set(k, j, scpu64_kernalshadow_read);
                    mem_read_base_set(k, j, mem_sram + 0x8000);
                    /* write hook preset, ram */
                    break;
                case RC:
                    mem_read_tab_set(k, j, ram_read_int);
                    mem_read_base_set(k, j, mem_ram);
                    mem_set_write_hook(k, i, ram_store_int);
                    break;
                case UM:
                    switch (j & 0xf0) {
                    default:
                        mem_read_tab_set(k, j, scpu64_ultimax_1000_7fff_read);
                        mem_read_base_set(k, j, NULL);
                        mem_set_write_hook(k, i, scpu64_ultimax_1000_7fff_store);
                        break;
                    case 0x80:
                    case 0x90:
                        mem_read_tab_set(k, j, scpu64_roml_read);
                        mem_read_base_set(k, j, NULL);
                        mem_set_write_hook(k, j, scpu64_roml_store);
                        break;
                    case 0xa0:
                    case 0xb0:
                        mem_read_tab_set(k, j, scpu64_ultimax_a000_bfff_read);
                        mem_read_base_set(k, j, NULL);
                        mem_set_write_hook(k, i, scpu64_ultimax_a000_bfff_store);
                        break;
                    case 0xc0:
                        mem_read_tab_set(k, j, scpu64_ultimax_c000_cfff_read);
                        mem_read_base_set(k, j, NULL);
                        mem_set_write_hook(k, i, scpu64_ultimax_c000_cfff_store);
                        break;
                    case 0xe0:
                    case 0xf0:
                        mem_read_tab_set(k, j, scpu64_romh_read);
                        mem_read_base_set(k, j, NULL);
                        mem_set_write_hook(k, j, scpu64_romh_store);
                        break;
                    }
                    break;
                case RL:
                    mem_read_tab_set(k, j, scpu64_roml_read);
                    mem_read_base_set(k, j, NULL);
                    /* write hook preset, ram */
                    break;
                case RH:
                    mem_read_tab_set(k, j, scpu64_romh_read);
                    mem_read_base_set(k, j, NULL);
                    /* write hook preset, ram */
                    break;
                case IO:
                    switch (j) {
                    case 0xd0:
                        mem_read_tab_set(k, j, scpu64io_d000_read);
                        mem_read_base_set(k, j, NULL);
                        mem_set_write_hook(k, j, scpu64io_d000_store);
                        break;
                    case 0xd1:
                        mem_read_tab_set(k, j, scpu64io_d100_read);
                        mem_read_base_set(k, j, NULL);
                        mem_set_write_hook(k, j, scpu64io_d100_store);
                        break;
                    case 0xd2:
                        mem_read_tab_set(k, j, scpu64io_d200_read);
                        mem_read_base_set(k, j, mem_sram + 0x10000);
                        mem_set_write_hook(k, j, scpu64io_d200_store);
                        break;
                    case 0xd3:
                        mem_read_tab_set(k, j, scpu64io_d300_read);
                        mem_read_base_set(k, j, mem_sram + 0x10000);
                        mem_set_write_hook(k, j, scpu64io_d300_store);
                        break;
                    case 0xd4:
                        mem_read_tab_set(k, j, scpu64io_d400_read);
                        mem_read_base_set(k, j, NULL);
                        mem_set_write_hook(k, j, scpu64io_d400_store);
                        break;
                    case 0xd5:
                        mem_read_tab_set(k, j, scpu64io_d500_read);
                        mem_read_base_set(k, j, NULL);
                        mem_set_write_hook(k, j, scpu64io_d500_store);
                        break;
                    case 0xd6:
                        mem_read_tab_set(k, j, scpu64io_d600_read);
                        mem_read_base_set(k, j, NULL);
                        mem_set_write_hook(k, j, scpu64io_d600_store);
                        break;
                    case 0xd7:
                        mem_read_tab_set(k, j, scpu64io_d700_read);
                        mem_read_base_set(k, j, NULL);
                        mem_set_write_hook(k, j, scpu64io_d700_store);
                        break;
                    case 0xdc:
                        mem_read_tab_set(k, j, scpu64_cia1_read);
                        mem_read_base_set(k, j, NULL);
                        mem_set_write_hook(k, j, scpu64_cia1_store);
                        break;
                    case 0xdd:
                        mem_read_tab_set(k, j, scpu64_cia2_read);
                        mem_read_base_set(k, j, NULL);
                        mem_set_write_hook(k, j, scpu64_cia2_store);
                        break;
                    case 0xde:
                        mem_read_tab_set(k, j, scpu64io_de00_read);
                        mem_read_base_set(k, j, NULL);
                        mem_set_write_hook(k, j, scpu64io_de00_store);
                        break;
                    case 0xdf:
                        mem_read_tab_set(k, j, scpu64io_df00_read);
                        mem_read_base_set(k, j, NULL);
                        mem_set_write_hook(k, j, scpu64io_df00_store);
                        break;
                    }
                    break;
                case CO:
                    mem_read_tab_set(k, j, scpu64io_colorram_read);
                    mem_read_base_set(k, j, mem_sram + 0x10000);
                    mem_set_write_hook(k, j, scpu64io_colorram_store);
                    break;
                case OP:
                    mem_read_tab_set(k, j, scpu64io_colorram_read_int);
                    mem_read_base_set(k, j, NULL);
                    mem_set_write_hook(k, j, scpu64io_colorram_store_int);
                    break;
                case F8:
                    mem_read_tab_set(k, j, scpu64rom_scpu64_read);
                    mem_read_base_set(k, j, scpu64rom_scpu64_rom);
                    /* write hook preset, ram */
                    break;
                case CR:
                    mem_read_tab_set(k, j, chargen_read);
                    mem_read_base_set(k, j, mem_chargen_rom - 0xd000);
                    /* write hook preset, ram */
                    break;
                }
            }
        }
    }
}
