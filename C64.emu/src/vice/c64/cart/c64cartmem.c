/*
 * c64cartmem.c -- C64 cartridge emulation, memory handling.
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c64cart.h"
#include "c64mem.h"
#include "c64cartmem.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "cartio.h"
#include "cartridge.h"
#include "crt.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "resources.h"
#include "types.h"
#include "vicii-phi1.h"

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "actionreplay2.h"
#include "actionreplay3.h"
#include "actionreplay4.h"
#include "actionreplay.h"
#include "atomicpower.h"
#include "c64acia.h"
#include "c64-generic.h"
#include "c64-midi.h"
#include "c64tpi.h"
#include "comal80.h"
#include "capture.h"
#include "delaep256.h"
#include "delaep64.h"
#include "delaep7x8.h"
#include "dinamic.h"
#include "dqbb.h"
#include "easyflash.h"
#include "epyxfastload.h"
#include "exos.h"
#include "expert.h"
#include "final.h"
#include "finalplus.h"
#include "final3.h"
#include "formel64.h"
#include "freezeframe.h"
#include "freezemachine.h"
#include "funplay.h"
#include "gamekiller.h"
#include "georam.h"
#include "gs.h"
#include "ide64.h"
#include "isepic.h"
#include "kcs.h"
#include "kingsoft.h"
#include "mach5.h"
#include "magicdesk.h"
#include "magicformel.h"
#include "magicvoice.h"
#include "mikroass.h"
#include "mmc64.h"
#include "mmcreplay.h"
#include "ocean.h"
#include "pagefox.h"
#include "prophet64.h"
#include "ramcart.h"
#include "retroreplay.h"
#include "reu.h"
#include "rexep256.h"
#include "rexutility.h"
#include "ross.h"
#include "simonsbasic.h"
#include "snapshot64.h"
#include "stardos.h"
#include "stb.h"
#include "supergames.h"
#include "superexplode5.h"
#include "supersnapshot.h"
#include "supersnapshot4.h"
#include "warpspeed.h"
#include "westermann.h"
#include "zaxxon.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API

/* #define DEBUGCART */

/* FIXME: test and then remove all the old code */
#define USESLOTS /* define to use passthrough code */

#ifdef DEBUGCART
#define DBG(x)  printf x; fflush(stdout);
#else
#define DBG(x)
#endif

/* Expansion port signals.  */
export_t export = { 0, 0, 0, 0 }; /* c64 export */
export_t export_slot1 = { 0, 0, 0, 0 };
export_t export_slotmain = { 0, 0, 0, 0 };
export_t export_passthrough = { 0, 0, 0, 0 }; /* slot1 and main combined, goes into slot0 passthrough or c64 export */

/* from c64cart.c */
extern int mem_cartridge_type; /* Type of the cartridge attached. ("Main Slot") */

static void ultimax_memptr_update(void);

/*
  mode_phiN:

  bits N..2: bank (currently max 0x3f)

  bits 1,0: !exrom, game

  mode_phiN & 3 = 0 : roml
  mode_phiN & 3 = 1 : roml & romh
  mode_phiN & 3 = 2 : ram
  mode_phiN & 3 = 3 : ultimax

  wflag:

  bit 4  0x10   - trigger nmi after config changed
  bit 3  0x08   - export ram enabled
  bit 2  0x04   - vic phi2 mode (always sees ram if set)
  bit 1  0x02   - release freeze (stop asserting NMI)
  bit 0  0x01   - r/w flag
*/


#ifndef USESLOTS
static void cart_config_changed(int slot, BYTE mode_phi1, BYTE mode_phi2, unsigned int wflag)
{
#ifdef DEBUGCART
    static int old1 = 0, old2 = 0, old3 = 0;
    if ((mode_phi1 != old1) || (mode_phi2 != old2) || (wflag != old3)) {
        DBG(("CARTMEM: cart_config_changed slot %d phi1:%d phi2:%d bank: %d flags:%02x\n", slot, mode_phi1 & 3, mode_phi2 & 3, (mode_phi2 >> CMODE_BANK_SHIFT) & CMODE_BANK_MASK, wflag));
    }
    old1 = mode_phi1; old2 = mode_phi2; old3 = wflag;
#endif

    if ((wflag & CMODE_WRITE) == CMODE_WRITE) {
        machine_handle_pending_alarms(maincpu_rmw_flag + 1);
    } else {
        machine_handle_pending_alarms(0);
    }

    export.game = mode_phi2 & 1;
    export.exrom = ((mode_phi2 >> 1) & 1) ^ 1;

    if (slot == 2) {
        cart_romhbank_set_slotmain((mode_phi2 >> CMODE_BANK_SHIFT) & CMODE_BANK_MASK);
        cart_romlbank_set_slotmain((mode_phi2 >> CMODE_BANK_SHIFT) & CMODE_BANK_MASK);
        export_ram = (wflag >> CMODE_EXPORT_RAM_SHIFT) & 1;
    }

    mem_pla_config_changed();
    if ((wflag & CMODE_RELEASE_FREEZE) == CMODE_RELEASE_FREEZE) {
        cartridge_release_freeze();
    }
    export.ultimax_phi1 = (mode_phi1 & 1) & ((mode_phi1 >> 1) & 1);
    export.ultimax_phi2 = export.game & (export.exrom ^ 1) & ((~wflag >> CMODE_PHI2_RAM_SHIFT) & 1);

    /* TODO
    if (slot == 2) {
        cart_romhbank_phi1_set_slotmain((mode_phi1 >> CMODE_BANK_SHIFT) & CMODE_BANK_MASK);
        cart_romlbank_phi1_set_slotmain((mode_phi1 >> CMODE_BANK_SHIFT) & CMODE_BANK_MASK);
    }
    */

    machine_update_memory_ptrs();

    if ((wflag & CMODE_TRIGGER_FREEZE_NMI_ONLY) == CMODE_TRIGGER_FREEZE_NMI_ONLY) {
        cartridge_trigger_freeze_nmi_only();
    }
}
#endif

void cart_set_port_exrom_slot0(int n)
{
    export.exrom = n;
}

void cart_set_port_game_slot0(int n)
{
    export.game = n;
}

void cart_port_config_changed_slot0(void)
{
    mem_pla_config_changed();
    ultimax_memptr_update();
}

void cart_config_changed_slot0(BYTE mode_phi1, BYTE mode_phi2, unsigned int wflag)
{
    assert(((mode_phi2 >> CMODE_BANK_SHIFT) & CMODE_BANK_MASK) == 0);
    assert((wflag & CMODE_RELEASE_FREEZE) == 0);
    assert((wflag & CMODE_TRIGGER_FREEZE_NMI_ONLY) == 0);
    assert((wflag & CMODE_PHI2_RAM) == 0);
    assert((wflag & CMODE_EXPORT_RAM) == 0);

#ifndef USESLOTS
    cart_config_changed(0, mode_phi1, mode_phi2, wflag);
#else

    if ((wflag & CMODE_WRITE) == CMODE_WRITE) {
        machine_handle_pending_alarms(maincpu_rmw_flag + 1);
    } else {
        machine_handle_pending_alarms(0);
    }

    export.game = mode_phi2 & 1;
    export.exrom = ((mode_phi2 >> 1) & 1) ^ 1;
    export.ultimax_phi1 = (mode_phi1 & 1) & ((mode_phi1 >> 1) & 1);
    export.ultimax_phi2 = (mode_phi2 & 1) & ((mode_phi2 >> 1) & 1);

    mem_pla_config_changed();
    ultimax_memptr_update();
    machine_update_memory_ptrs();

#endif
}

#ifdef USESLOTS
void cart_passthrough_changed(void)
{
    export_passthrough.game = 0;
    export_passthrough.exrom = 0;
    export_passthrough.ultimax_phi1 = 0;
    export_passthrough.ultimax_phi2 = 0;

    if (cart_getid_slot1() != CARTRIDGE_NONE) {
        export_passthrough.game |= export_slot1.game;
        export_passthrough.exrom |= export_slot1.exrom;
        export_passthrough.ultimax_phi1 |= export_slot1.ultimax_phi1;
        export_passthrough.ultimax_phi2 |= export_slot1.ultimax_phi2;
    }

    if (cart_getid_slotmain() != CARTRIDGE_NONE) {
        export_passthrough.game |= export_slotmain.game;
        export_passthrough.exrom |= export_slotmain.exrom;
        export_passthrough.ultimax_phi1 |= export_slotmain.ultimax_phi1;
        export_passthrough.ultimax_phi2 |= export_slotmain.ultimax_phi2;
    }

    export.game = export_passthrough.game;
    export.exrom = export_passthrough.exrom;
    export.ultimax_phi1 = export_passthrough.ultimax_phi1;
    export.ultimax_phi2 = export_passthrough.ultimax_phi2;

    switch (cart_getid_slot0()) {
        case CARTRIDGE_MMC64:
            mmc64_passthrough_changed((struct export_s*)&export_passthrough);
            break;
        case CARTRIDGE_MAGIC_VOICE:
            magicvoice_passthrough_changed((struct export_s*)&export_passthrough);
            break;
        case CARTRIDGE_IEEE488:
            tpi_passthrough_changed((struct export_s*)&export_passthrough);
            break;
        default:
            /* no slot 0 cartridge */
            break;
    }
}
#endif

void cart_set_port_exrom_slot1(int n)
{
#ifndef USESLOTS
    export.exrom = n;
#else
    export_slot1.exrom = n;
    cart_passthrough_changed();
#endif
}

void cart_set_port_game_slot1(int n)
{
#ifndef USESLOTS
    export.game = n;
#else
    export_slot1.game = n;
    cart_passthrough_changed();
#endif
}

void cart_port_config_changed_slot1(void)
{
    mem_pla_config_changed();
    ultimax_memptr_update();
}

void cart_config_changed_slot1(BYTE mode_phi1, BYTE mode_phi2, unsigned int wflag)
{
    assert(((mode_phi2 >> CMODE_BANK_SHIFT) & CMODE_BANK_MASK) == 0);
    assert(((wflag >> CMODE_EXPORT_RAM_SHIFT) & 1) == 0);
#ifndef USESLOTS
    cart_config_changed(1, mode_phi1, mode_phi2, wflag);
#else

#ifdef DEBUGCART
    static int old1 = 0, old2 = 0, old3 = 0;
    if ((mode_phi1 != old1) || (mode_phi2 != old2) || (wflag != old3)) {
        DBG(("CARTMEM: cart_config_changed_slot1 phi1:%d phi2:%d bank: %d flags:%02x\n", mode_phi1 & 3, mode_phi2 & 3, (mode_phi2 >> CMODE_BANK_SHIFT) & CMODE_BANK_MASK, wflag));
    }
    old1 = mode_phi1; old2 = mode_phi2; old3 = wflag;
#endif

    if ((wflag & CMODE_WRITE) == CMODE_WRITE) {
        machine_handle_pending_alarms(maincpu_rmw_flag + 1);
    } else {
        machine_handle_pending_alarms(0);
    }

    export_slot1.game = mode_phi2 & 1;
    export_slot1.exrom = ((mode_phi2 >> 1) & 1) ^ 1;

    export_slot1.ultimax_phi1 = (mode_phi1 & 1) & ((mode_phi1 >> 1) & 1);
    export_slot1.ultimax_phi2 = export_slot1.game & (export_slot1.exrom ^ 1) & ((~wflag >> CMODE_PHI2_RAM_SHIFT) & 1);

    cart_passthrough_changed();
    mem_pla_config_changed();
    ultimax_memptr_update();

    if ((wflag & CMODE_RELEASE_FREEZE) == CMODE_RELEASE_FREEZE) {
        cartridge_release_freeze();
    }
    machine_update_memory_ptrs();

    if ((wflag & CMODE_TRIGGER_FREEZE_NMI_ONLY) == CMODE_TRIGGER_FREEZE_NMI_ONLY) {
        cartridge_trigger_freeze_nmi_only();
    }

#endif
}

void cart_set_port_exrom_slotmain(int n)
{
#ifndef USESLOTS
    export.exrom = n;
#else
    export_slotmain.exrom = n;
    cart_passthrough_changed();
#endif
}

void cart_set_port_game_slotmain(int n)
{
#ifndef USESLOTS
    export.game = n;
#else
    export_slotmain.game = n;
    cart_passthrough_changed();
#endif
}

void cart_set_port_phi1_slotmain(int n)
{
#ifndef USESLOTS
    export.ultimax_phi1 = n;
#else
    export_slotmain.ultimax_phi1 = n;
    cart_passthrough_changed();
#endif
}

void cart_set_port_phi2_slotmain(int n)
{
#ifndef USESLOTS
    export.ultimax_phi2 = n;
#else
    export_slotmain.ultimax_phi2 = n;
    cart_passthrough_changed();
#endif
}

void cart_port_config_changed_slotmain(void)
{
    mem_pla_config_changed();
    ultimax_memptr_update();
}

void cart_config_changed_slotmain(BYTE mode_phi1, BYTE mode_phi2, unsigned int wflag)
{
#ifndef USESLOTS
    cart_config_changed(2, mode_phi1, mode_phi2, wflag);
#else

#ifdef DEBUGCART
    static int old1 = 0, old2 = 0, old3 = 0;
    if ((mode_phi1 != old1) || (mode_phi2 != old2) || (wflag != old3)) {
        DBG(("CARTMEM: cart_config_changed_slotmain phi1:%d phi2:%d bank: %d flags:%02x\n", mode_phi1 & 3, mode_phi2 & 3, (mode_phi2 >> CMODE_BANK_SHIFT) & CMODE_BANK_MASK, wflag));
    }
    old1 = mode_phi1; old2 = mode_phi2; old3 = wflag;
#endif

    if ((wflag & CMODE_WRITE) == CMODE_WRITE) {
        machine_handle_pending_alarms(maincpu_rmw_flag + 1);
    } else {
        machine_handle_pending_alarms(0);
    }

    export_slotmain.game = mode_phi2 & 1;
    export_slotmain.exrom = ((mode_phi2 >> 1) & 1) ^ 1;

    cart_romhbank_set_slotmain((mode_phi2 >> CMODE_BANK_SHIFT) & CMODE_BANK_MASK);
    cart_romlbank_set_slotmain((mode_phi2 >> CMODE_BANK_SHIFT) & CMODE_BANK_MASK);
    export_ram = (wflag >> CMODE_EXPORT_RAM_SHIFT) & 1;

    export_slotmain.ultimax_phi1 = (mode_phi1 & 1) & ((mode_phi1 >> 1) & 1);
    export_slotmain.ultimax_phi2 = export_slotmain.game & (export_slotmain.exrom ^ 1) & ((~wflag >> CMODE_PHI2_RAM_SHIFT) & 1);

    /* TODO
    cart_romhbank_phi1_set_slotmain((mode_phi1 >> CMODE_BANK_SHIFT) & CMODE_BANK_MASK);
    cart_romlbank_phi1_set_slotmain((mode_phi1 >> CMODE_BANK_SHIFT) & CMODE_BANK_MASK);
    */

    cart_passthrough_changed();
    mem_pla_config_changed();
    ultimax_memptr_update();

    if ((wflag & CMODE_RELEASE_FREEZE) == CMODE_RELEASE_FREEZE) {
        cartridge_release_freeze();
    }
    machine_update_memory_ptrs();

    if ((wflag & CMODE_TRIGGER_FREEZE_NMI_ONLY) == CMODE_TRIGGER_FREEZE_NMI_ONLY) {
        cartridge_trigger_freeze_nmi_only();
    }

#endif
}

/*
    generic helper routines, should be used *only* by carts that are in the
    "main cartridge" category
*/
void cart_romhbank_set_slotmain(unsigned int bank)
{
    romh_bank = (int)bank;
}

void cart_romlbank_set_slotmain(unsigned int bank)
{
    roml_bank = (int)bank;
}

/*
   - only CPU accesses go through the following hooks.
   - carts that switch game/exrom (ie, the memory config) based on adress, BA, phi2
     or similar can not be supported correctly with the existing system.
     the common workaround is to put the cart into ultimax mode, and wrap all hooks
     to fake the expected mapping.

    carts that use fake ultimax mapping:

        magic voice

        isepic
        expert

        stardos
        exos
        final plus
        game killer
        capture
        magicformel
        mmcreplay

    carts that use "unusual" mapping:

        expert       (Allow writing at ROML at 8000-9FFF in Ultimax mode.)
        ramcart

        ide64
        mmc64        (Allow writing at ROML at 8000-9fff)
        easyflash    (Allow writing at ROML at 8000-9FFF in Ultimax mode.)
        capture      (RAM at 6000-7fff in Ultimax mode.)

    internal extensions:

        plus60k
        plus256k
        c64_256k

    pure io extensions:

        georam
        digimax
        ds12c887rtc
        reu
        midi
        acia
        clockport (tfe)

              hook                    default

    8K Game Config:

    8000-9fff roml_read               roml_banks
              roml_no_ultimax_store   mem_store_without_romlh

    16K Game Config:

    8000-9fff roml_read               roml_banks
              roml_no_ultimax_store   mem_store_without_romlh
    a000-bfff romh_read               romh_banks
              romh_no_ultimax_store   mem_store_without_romlh

    Ultimax Config:

    1000-7fff ultimax_1000_7fff_read  vicii_read_phi1
              ultimax_1000_7fff_store n/a
    8000-9fff roml_read               roml_banks
              roml_store              n/a
    a000-bfff ultimax_a000_bfff_read  vicii_read_phi1
              ultimax_a000_bfff_store n/a
    c000-cfff ultimax_c000_cfff_read  vicii_read_phi1
              ultimax_c000_cfff_store n/a
    d000-dfff ultimax_d000_dfff_read  read_bank_io
              ultimax_d000_dfff_store store_bank_io
    e000-ffff romh_read               romh_banks
              romh_store              n/a
*/


/* ROML read - mapped to 8000 in 8k,16k,ultimax */
static BYTE roml_read_slotmain(WORD addr)
{
    /* "Main Slot" */
    switch (mem_cartridge_type) {
        case CARTRIDGE_ACTION_REPLAY:
            return actionreplay_roml_read(addr);
        case CARTRIDGE_ACTION_REPLAY2:
            return actionreplay2_roml_read(addr);
        case CARTRIDGE_ACTION_REPLAY3:
            return actionreplay3_roml_read(addr);
        case CARTRIDGE_ATOMIC_POWER:
            return atomicpower_roml_read(addr);
        case CARTRIDGE_EASYFLASH:
            return easyflash_roml_read(addr);
        case CARTRIDGE_EPYX_FASTLOAD:
            return epyxfastload_roml_read(addr);
        case CARTRIDGE_FINAL_I:
            return final_v1_roml_read(addr);
        case CARTRIDGE_FINAL_PLUS:
            return final_plus_roml_read(addr);
        case CARTRIDGE_FREEZE_MACHINE:
            return freezemachine_roml_read(addr);
        case CARTRIDGE_IDE64:
            return ide64_roml_read(addr);
        case CARTRIDGE_KINGSOFT:
            return kingsoft_roml_read(addr);
        case CARTRIDGE_MMC_REPLAY:
            return mmcreplay_roml_read(addr);
        case CARTRIDGE_PAGEFOX:
            return pagefox_roml_read(addr);
        case CARTRIDGE_RETRO_REPLAY:
            return retroreplay_roml_read(addr);
        case CARTRIDGE_STARDOS:
            return stardos_roml_read(addr);
        case CARTRIDGE_SNAPSHOT64:
            return snapshot64_roml_read(addr);
        case CARTRIDGE_SUPER_SNAPSHOT:
            return supersnapshot_v4_roml_read(addr);
        case CARTRIDGE_SUPER_SNAPSHOT_V5:
            return supersnapshot_v5_roml_read(addr);
        case CARTRIDGE_SUPER_EXPLODE_V5:
            return se5_roml_read(addr);
        case CARTRIDGE_ZAXXON:
            return zaxxon_roml_read(addr);
        case CARTRIDGE_CAPTURE:
        case CARTRIDGE_EXOS:
        case CARTRIDGE_FORMEL64:
        case CARTRIDGE_GAME_KILLER:
        case CARTRIDGE_MAGIC_FORMEL: /* ? */
            /* fake ultimax hack */
            return mem_read_without_ultimax(addr);
        case CARTRIDGE_ACTION_REPLAY4:
        case CARTRIDGE_FINAL_III:
        case CARTRIDGE_FREEZE_FRAME:
        default: /* use default cartridge */
            return generic_roml_read(addr);
        case CARTRIDGE_CRT: /* invalid */
            DBG(("CARTMEM: BUG! invalid type %d for main cart (addr %04x)\n", mem_cartridge_type, addr));
            break;
    }

    /* open bus */
    DBG(("CARTMEM: BUG! ROML open bus read (addr %04x)\n", addr));
    return vicii_read_phi1();
}

/* ROML read - mapped to 8000 in 8k,16k,ultimax */
static BYTE roml_read_slot1(WORD addr)
{
    /* "Slot 1" */
    if (isepic_cart_active()) {
        return isepic_page_read(addr);
    }
    if (expert_cart_enabled()) {
        return expert_roml_read(addr);
    }
    if (ramcart_cart_enabled()) {
        return ramcart_roml_read(addr);
    }
    if (dqbb_cart_enabled()) {
        return dqbb_roml_read(addr);
    }

    /* continue with "Main Slot" */
    return roml_read_slotmain(addr);
}

/* ROML read - mapped to 8000 in 8k,16k,ultimax */
BYTE roml_read(WORD addr)
{
    int res = CART_READ_THROUGH;
    BYTE value;
/*    DBG(("CARTMEM roml_read (addr %04x)\n", addr)); */

    /* "Slot 0" */

    if (mmc64_cart_enabled()) {
        if ((res = mmc64_roml_read(addr, &value)) == CART_READ_VALID) {
            return value;
        }
    } else if (magicvoice_cart_enabled()) {
        if ((res = magicvoice_roml_read(addr, &value)) == CART_READ_VALID) {
            return value;
        }
    } else if (tpi_cart_enabled()) {
        if ((res = tpi_roml_read(addr, &value)) == CART_READ_VALID) {
            return value;
        }
    }

    switch (res) {
        case CART_READ_C64MEM:
            return ram_read(addr);
        case CART_READ_THROUGH_NO_ULTIMAX:
            break;
    }

    /* continue with "Slot 1" */
    return roml_read_slot1(addr);
}

/* ROML store - mapped to 8000 in ultimax mode */
void roml_store(WORD addr, BYTE value)
{
    /* DBG(("ultimax w 8000: %04x %02x\n", addr, value)); */

    /* "Slot 0" */
    if (mmc64_cart_active()) {
        mmc64_roml_store(addr, value);
        return;
    }
    if (magicvoice_cart_enabled()) {
        /* fake ultimax hack */
        mem_store_without_ultimax(addr, value);
        return;
    }
    /* "Slot 1" */
    if (expert_cart_enabled()) {
        expert_roml_store(addr, value);
        return;
    }
    if (ramcart_cart_enabled()) {
        ramcart_roml_store(addr, value);
        return;
    }
    /* "Main Slot" */
    switch (mem_cartridge_type) {
        case CARTRIDGE_ACTION_REPLAY:
            actionreplay_roml_store(addr, value);
            return;
        case CARTRIDGE_ATOMIC_POWER:
            atomicpower_roml_store(addr, value);
            return;
        case CARTRIDGE_EASYFLASH:
            easyflash_roml_store(addr, value);
            return;
        case CARTRIDGE_MMC_REPLAY:
            mmcreplay_roml_store(addr, value);
            return;
        case CARTRIDGE_SUPER_SNAPSHOT:
            supersnapshot_v4_roml_store(addr, value);
            return;
        case CARTRIDGE_SUPER_SNAPSHOT_V5:
            supersnapshot_v5_roml_store(addr, value);
            return;
        case CARTRIDGE_RETRO_REPLAY:
            retroreplay_roml_store(addr, value);
            return;
        case CARTRIDGE_CAPTURE:
        case CARTRIDGE_EXOS:
        case CARTRIDGE_FORMEL64:
        case CARTRIDGE_GAME_KILLER:
        case CARTRIDGE_STARDOS:
        case CARTRIDGE_MAGIC_FORMEL: /* ? */
            /* fake ultimax hack */
            mem_store_without_ultimax(addr, value);
            return;
        default: /* use default cartridge */
            generic_roml_store(addr, value);
            return;
        case CARTRIDGE_CRT: /* invalid */
            DBG(("CARTMEM: BUG! invalid type %d for main cart (addr %04x)\n", mem_cartridge_type, addr));
            break;
    }

    /* open bus */
    DBG(("CARTMEM: BUG! ROML open bus store (addr %04x)\n", addr));
}

/* ROMH read - mapped to A000 in 16k, to E000 in ultimax

   most carts that use romh_read also need to use ultimax_romh_read_hirom
   below. carts that map an "external kernal" wrap to ram_read here.
*/
static BYTE romh_read_slotmain(WORD addr)
{
    /* "Main Slot" */
    switch (mem_cartridge_type) {
        case CARTRIDGE_ACTION_REPLAY2:
            return actionreplay2_romh_read(addr);
        case CARTRIDGE_ACTION_REPLAY3:
            return actionreplay3_romh_read(addr);
        case CARTRIDGE_ATOMIC_POWER:
            return atomicpower_romh_read(addr);
        case CARTRIDGE_CAPTURE:
            return capture_romh_read(addr);
        case CARTRIDGE_EASYFLASH:
            return easyflash_romh_read(addr);
        case CARTRIDGE_FINAL_I:
            return final_v1_romh_read(addr);
        case CARTRIDGE_FINAL_PLUS:
            return final_plus_romh_read(addr);
        case CARTRIDGE_FORMEL64:
            return formel64_romh_read(addr);
        case CARTRIDGE_IDE64:
            return ide64_romh_read(addr);
        case CARTRIDGE_KINGSOFT:
            return kingsoft_romh_read(addr);
        case CARTRIDGE_MAGIC_FORMEL:
            return magicformel_romh_read(addr);
        case CARTRIDGE_MMC_REPLAY:
            return mmcreplay_romh_read(addr);
        case CARTRIDGE_OCEAN:
            return ocean_romh_read(addr);
        case CARTRIDGE_PAGEFOX:
            return pagefox_romh_read(addr);
        case CARTRIDGE_RETRO_REPLAY:
            return retroreplay_romh_read(addr);
        case CARTRIDGE_SNAPSHOT64:
            return snapshot64_romh_read(addr);
        case CARTRIDGE_EXOS:
        case CARTRIDGE_STARDOS:
            /* fake ultimax hack, read from ram */
            return ram_read(addr);
        /* return mem_read_without_ultimax(addr); */
        case CARTRIDGE_ACTION_REPLAY4:
        case CARTRIDGE_FINAL_III:
        case CARTRIDGE_FREEZE_FRAME:
        case CARTRIDGE_FREEZE_MACHINE:
        default: /* use default cartridge */
            return generic_romh_read(addr);
        case CARTRIDGE_CRT: /* invalid */
            DBG(("CARTMEM: BUG! invalid type %d for main cart (addr %04x)\n", mem_cartridge_type, addr));
            break;
    }

    /* open bus */
    DBG(("CARTMEM: BUG! ROMH open bus read (addr %04x)\n", addr));
    return vicii_read_phi1();
}

static BYTE romh_read_slot1(WORD addr)
{
    /* "Slot 1" */
    if (expert_cart_enabled()) {
        return expert_romh_read(addr);
    }
    if (dqbb_cart_enabled()) {
        return dqbb_romh_read(addr);
    }
    if (isepic_cart_active()) {
        return isepic_romh_read(addr);
    }

    /* continue with "Main Slot" */
    return romh_read_slotmain(addr);
}

BYTE romh_read(WORD addr)
{
    int res = CART_READ_THROUGH;
    BYTE value;
    /* DBG(("ultimax r e000: %04x\n", addr)); */

    /* "Slot 0" */
    if (magicvoice_cart_enabled()) {
        if ((res = magicvoice_romh_read(addr, &value)) == CART_READ_VALID) {
            return value;
        }
    }
    switch (res) {
        case CART_READ_C64MEM:
            return mem_read_without_ultimax(addr);
        case CART_READ_THROUGH_NO_ULTIMAX:
            break;
    }
    /* continue with "Slot 1" */
    return romh_read_slot1(addr);
}

/* ROMH read if hirom is selected - mapped to E000 in ultimax

   most carts that use romh_read also need to use this one. carts
   that map an "external kernal" _only_ use this one, and wrap to
   ram_read in romh_read.
*/
BYTE ultimax_romh_read_hirom_slotmain(WORD addr)
{
    /* "Main Slot" */
    switch (mem_cartridge_type) {
        case CARTRIDGE_ACTION_REPLAY2:
            return actionreplay2_romh_read(addr);
        case CARTRIDGE_ACTION_REPLAY3:
            return actionreplay3_romh_read(addr);
        case CARTRIDGE_ATOMIC_POWER:
            return atomicpower_romh_read(addr);
        case CARTRIDGE_CAPTURE:
            return capture_romh_read(addr);
        case CARTRIDGE_EASYFLASH:
            return easyflash_romh_read(addr);
        case CARTRIDGE_EXOS:
            return exos_romh_read_hirom(addr);
        case CARTRIDGE_FINAL_I:
            return final_v1_romh_read(addr);
        case CARTRIDGE_FINAL_PLUS:
            return final_plus_romh_read(addr);
        case CARTRIDGE_FORMEL64:
            return formel64_romh_read_hirom(addr);
        case CARTRIDGE_IDE64:
            return ide64_romh_read(addr);
        case CARTRIDGE_KINGSOFT:
            return kingsoft_romh_read(addr);
        case CARTRIDGE_MAGIC_FORMEL:
            return magicformel_romh_read_hirom(addr);
        case CARTRIDGE_MMC_REPLAY:
            return mmcreplay_romh_read(addr);
        case CARTRIDGE_OCEAN:
            return ocean_romh_read(addr);
        case CARTRIDGE_RETRO_REPLAY:
            return retroreplay_romh_read(addr);
        case CARTRIDGE_SNAPSHOT64:
            return snapshot64_romh_read(addr);
        case CARTRIDGE_STARDOS:
            return stardos_romh_read(addr);
        case CARTRIDGE_ACTION_REPLAY4:
        case CARTRIDGE_FINAL_III:
        case CARTRIDGE_FREEZE_FRAME:
        case CARTRIDGE_FREEZE_MACHINE:
        default: /* use default cartridge */
            return generic_romh_read(addr);
        case CARTRIDGE_CRT: /* invalid */
            DBG(("CARTMEM: BUG! invalid type %d for main cart (addr %04x)\n", mem_cartridge_type, addr));
            break;
    }

    /* open bus */
    DBG(("CARTMEM: BUG! ROMH open bus read (addr %04x)\n", addr));
    return vicii_read_phi1();
}

static BYTE ultimax_romh_read_hirom_slot1(WORD addr)
{
    /* "Slot 1" */
    if (dqbb_cart_enabled()) {
        return dqbb_romh_read(addr);
    }
    if (expert_cart_enabled()) {
        return expert_romh_read(addr);
    }
    if (isepic_cart_active()) {
        return isepic_romh_read(addr);
    }

    /* continue with "Main Slot" */
    return ultimax_romh_read_hirom_slotmain(addr);
}

BYTE ultimax_romh_read_hirom(WORD addr)
{
    int res;
    BYTE value;
    /* DBG(("ultimax r e000: %04x\n", addr)); */

    /* "Slot 0" */
    res = CART_READ_THROUGH;
    if (magicvoice_cart_enabled()) {
        if ((res = magicvoice_romh_read(addr, &value)) == CART_READ_VALID) {
            return value;
        }
    }
    switch (res) {
        case CART_READ_C64MEM:
            return mem_read_without_ultimax(addr);
        case CART_READ_THROUGH_NO_ULTIMAX:
            break;
    }
    /* continue with "Slot 1" */
    return ultimax_romh_read_hirom_slot1(addr);
}

/* ROMH store - mapped to E000 in ultimax mode
   - carts that use "external kernal" mode must wrap to ram_store here
*/
void romh_store(WORD addr, BYTE value)
{
    /* DBG(("ultimax w e000: %04x %02x\n", addr, value)); */

    /* "Slot 0" */
    if (magicvoice_cart_enabled()) {
        /* fake ultimax hack, c64 ram */
        mem_store_without_ultimax(addr, value);
    }
    /* "Slot 1" */
    if (isepic_cart_active()) {
        isepic_romh_store(addr, value);
    }

    /* "Main Slot" */
    /* to aid in debugging, use return instead of break incase of a successful store */
    switch (mem_cartridge_type) {
        case CARTRIDGE_CAPTURE:
            capture_romh_store(addr, value);
            return;
        case CARTRIDGE_EASYFLASH:
            easyflash_romh_store(addr, value);
            return;
        case CARTRIDGE_MMC_REPLAY:
            mmcreplay_romh_store(addr, value);
            return;
        case CARTRIDGE_EXOS:
        case CARTRIDGE_FINAL_PLUS:
        case CARTRIDGE_STARDOS:
        case CARTRIDGE_SNAPSHOT64: /* ? */
        case CARTRIDGE_MAGIC_FORMEL: /* ? */
            /* fake ultimax hack, c64 ram */
            /* mem_store_without_ultimax(addr, value); */
            ram_store(addr, value);
            return;
        case CARTRIDGE_CRT: /* invalid */
            DBG(("CARTMEM: BUG! invalid type %d for main cart (addr %04x)\n", mem_cartridge_type, addr));
            return;
    }

    /* open bus */
/*    DBG(("CARTMEM: possible BUG! ROMH open bus store (@$%04x, addr %04x)\n", reg_pc, addr)); */
}

/* ROMH store - A000-BFFF in 16kGame

   normally writes to ROM area would go to RAM an not generate
   a write select. some carts however map RAM here and also
   accept writes in this mode.
*/
void romh_no_ultimax_store(WORD addr, BYTE value)
{
    /* DBG(("game    w a000: %04x %02x\n", addr, value)); */

    /* "Slot 0" */
    /* "Slot 1" */
    if (dqbb_cart_enabled()) {
        dqbb_romh_store(addr, value);
        return;
    }

    /* "Main Slot" */
    switch (mem_cartridge_type) {
        case CARTRIDGE_ATOMIC_POWER:
            atomicpower_romh_store(addr, value);
            break;
        case CARTRIDGE_PAGEFOX:
            pagefox_romh_store(addr, value);
            break;
        case CARTRIDGE_RETRO_REPLAY:
            retroreplay_romh_store(addr, value);
            break;
        case CARTRIDGE_CRT: /* invalid */
            DBG(("CARTMEM: BUG! invalid type %d for main cart (addr %04x)\n", mem_cartridge_type, addr));
            break;
    }
    /* store to c64 ram */
    mem_store_without_romlh(addr, value);
}

/* ROML store - mapped to 8000-9fff in 8kGame, 16kGame

   normally writes to ROM area would go to RAM and not generate
   a write select. some carts however map ram here and also
   accept writes in this mode.
*/
void roml_no_ultimax_store(WORD addr, BYTE value)
{
    /* DBG(("game rom    w 8000: %04x %02x\n", addr, value)); */
    /* "Slot 0" */
#if 0
    if (mmc64_cart_active()) {
        mmc64_roml_store(addr, value);
        return; /* ? */
    }
#endif
    /* "Slot 1" */
    if (expert_cart_enabled()) {
        expert_roml_store(addr, value);
        return; /* ? */
    }
    if (dqbb_cart_enabled()) {
        dqbb_roml_store(addr, value);
        return; /* ? */
    }

    /* "Main Slot" */
    switch (mem_cartridge_type) {
        case CARTRIDGE_ACTION_REPLAY:
            actionreplay_roml_store(addr, value);
            break;
        case CARTRIDGE_ATOMIC_POWER:
            atomicpower_roml_store(addr, value);
            break;
        case CARTRIDGE_PAGEFOX:
            pagefox_roml_store(addr, value);
            break;
        case CARTRIDGE_RETRO_REPLAY:
            if (retroreplay_roml_no_ultimax_store(addr, value)) {
                return; /* FIXME: this is weird */
            }
            break;
        case CARTRIDGE_CRT: /* invalid */
            DBG(("CARTMEM: BUG! invalid type %d for main cart (addr %04x)\n", mem_cartridge_type, addr));
            break;
    }

    /* store to c64 ram */
    /* DBG(("c64 ram    w 8000: %04x %02x\n", addr, value)); */
    /* mem_store_without_romlh(addr, value); */
    ram_store(addr, value);
}

/* RAML store (ROML _NOT_ selected) - mapped to 8000-9fff in 8kGame, 16kGame

   WARNING:
      mem_store_without_ultimax(addr, value)
      must NOT be called by any functions called here, as this will cause an
      endless loop
*/
void raml_no_ultimax_store(WORD addr, BYTE value)
{
    /* DBG(("game ram    w 8000: %04x %02x\n", addr, value)); */
    /* "Slot 0" */
#if 0
    if (mmc64_cart_active()) {
        mmc64_roml_store(addr, value);
        /* return; */
    }
#endif
    /* "Slot 1" */
    if (expert_cart_enabled()) {
        expert_roml_store(addr, value);
        /* return; */
    }

    /* "Main Slot" */
    switch (mem_cartridge_type) {
        case CARTRIDGE_ACTION_REPLAY:
            actionreplay_roml_store(addr, value);
            break;
        case CARTRIDGE_ATOMIC_POWER:
            atomicpower_roml_store(addr, value);
            break;
        case CARTRIDGE_RETRO_REPLAY:
            if (retroreplay_roml_no_ultimax_store(addr, value)) {
                return; /* FIXME: this is weird */
            }
            break;
        case CARTRIDGE_CRT: /* invalid */
            DBG(("CARTMEM: BUG! invalid type %d for main cart (addr %04x)\n", mem_cartridge_type, addr));
            break;
    }

    /* store to c64 ram */
    /* DBG(("c64 ram    w 8000: %04x %02x\n", addr, value)); */
    ram_store(addr, value);
    /* mem_store_without_romlh(addr, value); */
}

/* ultimax read - 1000 to 7fff */
BYTE ultimax_1000_7fff_read_slot1(WORD addr)
{
    /* "Slot 1" */
    if (expert_cart_enabled()) {
        return mem_read_without_ultimax(addr); /* fake ultimax hack, c64 ram */
    }
    if (isepic_cart_active()) {
        /* return mem_read_without_ultimax(addr); */ /* fake ultimax hack */
        return isepic_page_read(addr);
    }

    /* "Main Slot" */
    switch (mem_cartridge_type) {
        case CARTRIDGE_CAPTURE:
            return capture_1000_7fff_read(addr);
        case CARTRIDGE_IDE64:
            return ide64_1000_7fff_read(addr);
        case CARTRIDGE_MMC_REPLAY:
            return mmcreplay_1000_7fff_read(addr);
        case CARTRIDGE_FORMEL64:
        case CARTRIDGE_MAGIC_FORMEL:
        case CARTRIDGE_GAME_KILLER:
        case CARTRIDGE_KINGSOFT:
        case CARTRIDGE_FINAL_PLUS:
        case CARTRIDGE_EXOS:
        case CARTRIDGE_STARDOS:
            /* fake ultimax hack, c64 ram */
            return mem_read_without_ultimax(addr);
        case CARTRIDGE_CRT: /* invalid */
            DBG(("CARTMEM: BUG! invalid type %d for main cart (addr %04x)\n", mem_cartridge_type, addr));
            break;
    }

    /* default; no cart, open bus */
    return vicii_read_phi1();
}

BYTE ultimax_1000_7fff_read(WORD addr)
{
    int res;
    BYTE value;

    /* "Slot 0" */
    res = CART_READ_THROUGH;

    if (magicvoice_cart_enabled()) {
        if ((res = magicvoice_ultimax_read(addr, &value)) == CART_READ_VALID) {
            return value;
        }
    }

    switch (res) {
        case CART_READ_C64MEM:
            /* fake ultimax hack, c64 ram */
            return mem_read_without_ultimax(addr);
        case CART_READ_THROUGH:
        case CART_READ_THROUGH_NO_ULTIMAX:
            break;
    }

    /* continue with "Slot 1" */
    return ultimax_1000_7fff_read_slot1(addr);
}

/* ultimax store - 1000 to 7fff */
void ultimax_1000_7fff_store(WORD addr, BYTE value)
{
    /* "Slot 0" */
    if (magicvoice_cart_enabled()) {
        mem_store_without_ultimax(addr, value); /* fake ultimax hack, c64 ram */
    }
    /* "Slot 1" */
    if (expert_cart_enabled()) {
        mem_store_without_ultimax(addr, value); /* fake ultimax hack, c64 ram */
    }
    if (isepic_cart_active()) {
        mem_store_without_ultimax(addr, value); /* fake ultimax hack */
    }

    /* "Main Slot" */
    switch (mem_cartridge_type) {
        case CARTRIDGE_IDE64:
            ide64_1000_7fff_store(addr, value);
            break;
        case CARTRIDGE_MMC_REPLAY:
            mmcreplay_1000_7fff_store(addr, value);
            break;
        case CARTRIDGE_CAPTURE:
            capture_1000_7fff_store(addr, value);
            break;
        case CARTRIDGE_FORMEL64:
        case CARTRIDGE_MAGIC_FORMEL:
        case CARTRIDGE_GAME_KILLER:
        case CARTRIDGE_FINAL_PLUS:
        case CARTRIDGE_EXOS:
        case CARTRIDGE_STARDOS:
        case CARTRIDGE_KINGSOFT:
            /* fake ultimax hack, c64 ram */
            mem_store_without_ultimax(addr, value);
            break;
        case CARTRIDGE_CRT: /* invalid */
            DBG(("CARTMEM: BUG! invalid type %d for main cart (addr %04x)\n", mem_cartridge_type, addr));
            break;
    }

    /* default; no cart, open bus */
}

/* ultimax read - a000 to bfff */
BYTE ultimax_a000_bfff_read_slot1(WORD addr)
{
    /* "Slot 1" */
    if (expert_cart_enabled()) {
        return mem_read_without_ultimax(addr); /* fake ultimax hack, c64 basic, ram */
    }
    if (isepic_cart_active()) {
        /* return mem_read_without_ultimax(addr); */ /* fake ultimax hack */
        return isepic_page_read(addr);
    }

    /* "Main Slot" */
    switch (mem_cartridge_type) {
        case CARTRIDGE_IDE64:
            return ide64_a000_bfff_read(addr);
        case CARTRIDGE_MMC_REPLAY:
            return mmcreplay_a000_bfff_read(addr);
        case CARTRIDGE_FINAL_PLUS:
            return final_plus_a000_bfff_read(addr);
        case CARTRIDGE_FORMEL64:
        case CARTRIDGE_MAGIC_FORMEL:
        case CARTRIDGE_CAPTURE:
        case CARTRIDGE_GAME_KILLER:
        case CARTRIDGE_EXOS:
        case CARTRIDGE_STARDOS:
            /* fake ultimax hack, c64 basic, ram */
            return mem_read_without_ultimax(addr);
        case CARTRIDGE_CRT: /* invalid */
            DBG(("CARTMEM: BUG! invalid type %d for main cart (addr %04x)\n", mem_cartridge_type, addr));
            break;
    }
    /* default; no cart, open bus */
    return vicii_read_phi1();
}

BYTE ultimax_a000_bfff_read(WORD addr)
{
    int res;
    BYTE value;

    /* "Slot 0" */
    res = CART_READ_THROUGH;
    if (magicvoice_cart_enabled()) {
        if ((res = magicvoice_a000_bfff_read(addr, &value)) == CART_READ_VALID) {
            return value;
        }
    }
    switch (res) {
        case CART_READ_C64MEM:
            /* fake ultimax hack, c64 basic, ram */
            return mem_read_without_ultimax(addr);
        case CART_READ_THROUGH_NO_ULTIMAX:
            return romh_read_slot1(addr);
    }

    /* continue with "Slot 1" */
    return ultimax_a000_bfff_read_slot1(addr);
}

/* ultimax store - a000 to bfff */
void ultimax_a000_bfff_store(WORD addr, BYTE value)
{
    /* "Slot 0" */
    if (magicvoice_cart_enabled()) {
        /* fake ultimax hack, c64 ram */
        mem_store_without_ultimax(addr, value);
    }
    /* "Slot 1" */
    if (expert_cart_enabled()) {
        mem_store_without_ultimax(addr, value); /* fake ultimax hack, c64 ram */
    }
    if (isepic_cart_active()) {
        mem_store_without_ultimax(addr, value); /* fake ultimax hack */
    }

    /* "Main Slot" */
    switch (mem_cartridge_type) {
        case CARTRIDGE_MMC_REPLAY:
            mmcreplay_a000_bfff_store(addr, value);
            break;
        case CARTRIDGE_CAPTURE:
        case CARTRIDGE_MAGIC_FORMEL:
        case CARTRIDGE_GAME_KILLER:
        case CARTRIDGE_FINAL_PLUS:
        case CARTRIDGE_FORMEL64:
        case CARTRIDGE_EXOS:
        case CARTRIDGE_STARDOS:
            /* fake ultimax hack, c64 ram */
            mem_store_without_ultimax(addr, value);
            break;
        case CARTRIDGE_CRT: /* invalid */
            DBG(("CARTMEM: BUG! invalid type %d for main cart (addr %04x)\n", mem_cartridge_type, addr));
            break;
    }

    /* default; no cart, open bus */
}

/* ultimax read - c000 to cfff */
BYTE ultimax_c000_cfff_read_slot1(WORD addr)
{
    /* "Slot 1" */
    if (expert_cart_enabled()) {
        return mem_read_without_ultimax(addr); /* fake ultimax hack, c64 ram */
    }
    if (isepic_cart_active()) {
        /* return mem_read_without_ultimax(addr); */ /* fake ultimax hack */
        return isepic_page_read(addr);
    }

    /* "Main Slot" */
    switch (mem_cartridge_type) {
        case CARTRIDGE_MMC_REPLAY:
            return mmcreplay_c000_cfff_read(addr);
        case CARTRIDGE_MAGIC_FORMEL:
        case CARTRIDGE_CAPTURE:
        case CARTRIDGE_GAME_KILLER:
        case CARTRIDGE_FINAL_PLUS:
        case CARTRIDGE_FORMEL64:
        case CARTRIDGE_EXOS:
        case CARTRIDGE_STARDOS:
        case CARTRIDGE_KINGSOFT:
            /* fake ultimax hack, c64 ram */
            return mem_read_without_ultimax(addr);
        case CARTRIDGE_CRT: /* invalid */
            DBG(("CARTMEM: BUG! invalid type %d for main cart (addr %04x)\n", mem_cartridge_type, addr));
            break;
    }
    /* default; no cart, open bus */
    return vicii_read_phi1();
}

BYTE ultimax_c000_cfff_read(WORD addr)
{
    int res;
    BYTE value;

    /* "Slot 0" */
    res = CART_READ_THROUGH;

    if (magicvoice_cart_enabled()) {
        if ((res = magicvoice_ultimax_read(addr, &value)) == CART_READ_VALID) {
            return value;
        }
    }

    switch (res) {
        case CART_READ_C64MEM:
            /* fake ultimax hack, c64 ram */
            return mem_read_without_ultimax(addr);
        case CART_READ_THROUGH:
        case CART_READ_THROUGH_NO_ULTIMAX:
            break;
    }

    /* continue with "Slot 1" */
    return ultimax_c000_cfff_read_slot1(addr);
}

/* ultimax store - c000 to cfff */
void ultimax_c000_cfff_store(WORD addr, BYTE value)
{
    /* "Slot 0" */
    if (magicvoice_cart_enabled()) {
        mem_store_without_ultimax(addr, value); /* fake ultimax hack, c64 ram */
    }
    /* "Slot 1" */
    if (expert_cart_enabled()) {
        mem_store_without_ultimax(addr, value); /* fake ultimax hack, c64 ram */
    }
    if (isepic_cart_active()) {
        mem_store_without_ultimax(addr, value); /* fake ultimax hack */
    }

    /* "Main Slot" */
    switch (mem_cartridge_type) {
        case CARTRIDGE_MMC_REPLAY:
            mmcreplay_c000_cfff_store(addr, value);
            break;
        case CARTRIDGE_MAGIC_FORMEL:
        case CARTRIDGE_CAPTURE:
        case CARTRIDGE_GAME_KILLER:
        case CARTRIDGE_FINAL_PLUS:
        case CARTRIDGE_FORMEL64:
        case CARTRIDGE_EXOS:
        case CARTRIDGE_STARDOS:
        case CARTRIDGE_KINGSOFT:
            /* fake ultimax hack, c64 ram */
            mem_store_without_ultimax(addr, value);
            break;
        case CARTRIDGE_CRT: /* invalid */
            DBG(("CARTMEM: BUG! invalid type %d for main cart (addr %04x)\n", mem_cartridge_type, addr));
            break;
    }

    /* default; no cart, open bus */
}

/* ultimax read - d000 to dfff */
static BYTE ultimax_d000_dfff_read_slot1(WORD addr)
{
    /* "Slot 1" */
    if (expert_cart_enabled()) {
        /* fake ultimax hack, c64 io,colram,ram */
        return mem_read_without_ultimax(addr);
    }

    /* "Main Slot" */
    switch (mem_cartridge_type) {
        case CARTRIDGE_CAPTURE:
        case CARTRIDGE_EXOS:
        case CARTRIDGE_FINAL_PLUS:
        case CARTRIDGE_FORMEL64:
        case CARTRIDGE_MAGIC_FORMEL:
        case CARTRIDGE_SNAPSHOT64: /* ? */
        case CARTRIDGE_STARDOS:
        case CARTRIDGE_KINGSOFT:
            /* fake ultimax hack, c64 io,colram,ram */
            return mem_read_without_ultimax(addr);
        case CARTRIDGE_CRT: /* invalid */
            DBG(("CARTMEM: BUG! invalid type %d for main cart (addr %04x)\n", mem_cartridge_type, addr));
            break;
    }
    /* default; no cart, c64 i/o */
    return read_bank_io(addr);
}

BYTE ultimax_d000_dfff_read(WORD addr)
{
    int res = CART_READ_THROUGH;
    BYTE value;

    /* "Slot 0" */

    if (magicvoice_cart_enabled()) {
        if ((res = magicvoice_ultimax_read(addr, &value)) == CART_READ_VALID) {
            return value;
        }
    }

    switch (res) {
        case CART_READ_C64MEM:
            /* fake ultimax hack, c64 io,colram,ram */
            return mem_read_without_ultimax(addr);
        case CART_READ_THROUGH:
        case CART_READ_THROUGH_NO_ULTIMAX:
            break;
    }

    /* continue with "Slot 1" */
    return ultimax_d000_dfff_read_slot1(addr);
}

/* ultimax store - d000 to dfff */
void ultimax_d000_dfff_store(WORD addr, BYTE value)
{
    /* "Slot 0" */
    if (magicvoice_cart_enabled()) {
        /* fake ultimax hack, c64 io,colram,ram */
        mem_store_without_ultimax(addr, value);
        return;
    }
    /* "Slot 1" */
    if (expert_cart_enabled()) {
        /* fake ultimax hack, c64 io,colram,ram */
        mem_store_without_ultimax(addr, value);
        return;
    }

    /* "Main Slot" */
    switch (mem_cartridge_type) {
        case CARTRIDGE_CAPTURE:
        case CARTRIDGE_EXOS:
        case CARTRIDGE_FINAL_PLUS:
        case CARTRIDGE_FORMEL64:
        case CARTRIDGE_MAGIC_FORMEL:
        case CARTRIDGE_SNAPSHOT64: /* ? */
        case CARTRIDGE_STARDOS:
        case CARTRIDGE_KINGSOFT:
            /* fake ultimax hack, c64 io,colram,ram */
            mem_store_without_ultimax(addr, value);
            return;
        case CARTRIDGE_CRT: /* invalid */
            DBG(("CARTMEM: BUG! invalid type %d for main cart (addr %04x)\n", mem_cartridge_type, addr));
            break;
    }
    /* default;no cart, c64 i/o */
    store_bank_io(addr, value);
}

/*
    VIC-II reads from cart memory

    most carts can simply wrap to ultimax_romh_read_hirom here, only
    those that handle the vic accesses differently than cpu accesses
    must provide their own functions.

    FIXME: lots of testing needed!
*/

static int ultimax_romh_phi1_read_slotmain(WORD addr, BYTE *value)
{
    int res = CART_READ_THROUGH;

    switch (mem_cartridge_type) {
        case CARTRIDGE_GENERIC_8KB:
        case CARTRIDGE_GENERIC_16KB:
            return 0;
        case CARTRIDGE_ULTIMAX:
            res = generic_romh_phi1_read(addr, value);
            break;
        case CARTRIDGE_CAPTURE:
            res = capture_romh_phi1_read(addr, value);
            break;
        case CARTRIDGE_EXOS:
            res = exos_romh_phi1_read(addr, value);
            break;
        case CARTRIDGE_FINAL_PLUS:
            res = final_plus_romh_phi1_read(addr, value);
            break;
        case CARTRIDGE_MAGIC_FORMEL:
            res = magicformel_romh_phi1_read(addr, value);
            break;
        case CARTRIDGE_MMC_REPLAY:
            res = mmcreplay_romh_phi1_read(addr, value);
            break;
        case CARTRIDGE_STARDOS:
            res = stardos_romh_phi1_read(addr, value);
            break;
        case CARTRIDGE_NONE:
            break;
        default:
            /* generic fallback */
            *value = ultimax_romh_read_hirom(addr);
            return 1;
    }

    switch (res) {
        case CART_READ_VALID:
            return 1;
        case CART_READ_C64MEM:
            /* fake ultimax hack, c64 basic, ram */
            return 0;
        case CART_READ_THROUGH_NO_ULTIMAX:
            break;
    }

    /* default; no cart, open bus */
    *value = vicii_read_phi1();
    return 1;
}

static int ultimax_romh_phi1_read_slot1(WORD addr, BYTE *value)
{
    int res = CART_READ_THROUGH;

    /* "Slot 1" */

    if (expert_cart_enabled()) {
        if ((res = expert_romh_phi1_read(addr, value)) == CART_READ_VALID) {
            return 1;
        }
    } else if (isepic_cart_enabled()) {
        if ((res = isepic_romh_phi1_read(addr, value)) == CART_READ_VALID) {
            return 1;
        }
    }

    switch (res) {
        case CART_READ_C64MEM:
            /* fake ultimax hack, c64 basic, ram */
            return 0;
        case CART_READ_THROUGH_NO_ULTIMAX:
            break;
    }

    /* continue with "Main Slot" */
    return ultimax_romh_phi1_read_slotmain(addr, value);
}

int ultimax_romh_phi1_read(WORD addr, BYTE *value)
{
    int res = CART_READ_THROUGH;

    /* "Slot 0" */

    if (magicvoice_cart_enabled()) {
        if ((res = magicvoice_romh_phi1_read(addr, value)) == CART_READ_VALID) {
            return 1;
        }
    }

    switch (res) {
        case CART_READ_C64MEM:
            return 0;
        case CART_READ_THROUGH_NO_ULTIMAX:
            break;
    }

    /* continue with "Slot 1" */
    return ultimax_romh_phi1_read_slot1(addr, value);
}

static int ultimax_romh_phi2_read_slotmain(WORD addr, BYTE *value)
{
    int res = CART_READ_THROUGH;

    switch (mem_cartridge_type) {
        case CARTRIDGE_GENERIC_8KB:
        case CARTRIDGE_GENERIC_16KB:
            return 0;
        case CARTRIDGE_ULTIMAX:
            res = generic_romh_phi2_read(addr, value);
            break;
        case CARTRIDGE_CAPTURE:
            res = capture_romh_phi2_read(addr, value);
            break;
        case CARTRIDGE_EXOS:
            res = exos_romh_phi2_read(addr, value);
            break;
        case CARTRIDGE_FINAL_PLUS:
            res = final_plus_romh_phi2_read(addr, value);
            break;
        case CARTRIDGE_MAGIC_FORMEL:
            res = magicformel_romh_phi2_read(addr, value);
            break;
        case CARTRIDGE_MMC_REPLAY:
            res = mmcreplay_romh_phi2_read(addr, value);
            break;
        case CARTRIDGE_STARDOS:
            res = stardos_romh_phi2_read(addr, value);
            break;
        case CARTRIDGE_NONE:
            break;
        default:
            /* generic fallback */
            *value = ultimax_romh_read_hirom(addr);
            return 1;
    }

    switch (res) {
        case CART_READ_VALID:
            return 1;
        case CART_READ_C64MEM:
            /* fake ultimax hack, c64 basic, ram */
            return 0;
        case CART_READ_THROUGH_NO_ULTIMAX:
            break;
    }

    /* default; no cart, open bus */
    *value = vicii_read_phi1();
    return 1;
}

static int ultimax_romh_phi2_read_slot1(WORD addr, BYTE *value)
{
    int res = CART_READ_THROUGH;

    /* "Slot 1" */

    if (expert_cart_enabled()) {
        if ((res = expert_romh_phi2_read(addr, value)) == CART_READ_VALID) {
            return 1;
        }
    } else if (isepic_cart_enabled()) {
        if ((res = isepic_romh_phi2_read(addr, value)) == CART_READ_VALID) {
            return 1;
        }
    }

    switch (res) {
        case CART_READ_C64MEM:
            /* fake ultimax hack, c64 basic, ram */
            return 0;
        case CART_READ_THROUGH_NO_ULTIMAX:
            break;
    }

    /* continue with "Main Slot " */
    return ultimax_romh_phi2_read_slotmain(addr, value);
}

int ultimax_romh_phi2_read(WORD addr, BYTE *value)
{
    int res = CART_READ_THROUGH;

    /* "Slot 0" */

    if (magicvoice_cart_enabled()) {
        if ((res = magicvoice_romh_phi2_read(addr, value)) == CART_READ_VALID) {
            return 1;
        }
    }

    switch (res) {
        case CART_READ_C64MEM:
            /* fake ultimax hack, c64 basic, ram */
            return 0;
        case CART_READ_THROUGH_NO_ULTIMAX:
            break;
    }

    /* continue with "Slot 1" */
    return ultimax_romh_phi2_read_slot1(addr, value);
}

/*
    the following two are used by the old non cycle exact vic-ii emulation

    HACK: the following tries to pass valid pointers to the functions in vicii.c
    and vicii-fetch.c by caching the last valid visible memory. this way their
    above functions (which are used by viciisc) can be reused for vicii (which
    would have to be rewritten to not use pointers, like viciisc, to remove
    this hack).
*/

static BYTE mem_phi1[0x1000];
static BYTE mem_phi2[0x1000];
static BYTE *mem_phi1_ptr[0x1000];
static BYTE *mem_phi2_ptr[0x1000];
static int mem_phi1_valid = 0;
static int mem_phi2_valid = 0;
static int mem_phi1_last = 0;
static int mem_phi2_last = 0;

void cart_reset_memptr(void)
{
    mem_phi1_valid = 0;
    mem_phi2_valid = 0;
}

static void ultimax_memptr_update(void)
{
    if (mem_phi1_last != export.ultimax_phi1) {
        mem_phi1_valid = 0;
    }
    if (mem_phi2_last != export.ultimax_phi2) {
        mem_phi2_valid = 0;
    }
}

BYTE *ultimax_romh_phi1_ptr(WORD addr)
{
    int n;
    int res;
    /* DBG(("phi1 addr %04x\n", addr)); */
    n = addr & 0x0fff;
    res = ultimax_romh_phi1_read((WORD)(0x1000 + n), &mem_phi1[n]);
    if (mem_phi1_ptr[n] != (res ? &mem_phi1[n] : NULL)) {
        mem_phi1_valid = 0;
    }

    if (!mem_phi1_valid) {
        n = 0;
        while (n < 0x1000) {
            if (ultimax_romh_phi1_read((WORD)(0x1000 + n), &mem_phi1[n]) == 0) {
                mem_phi1_ptr[n] = NULL;
            } else {
                mem_phi1_ptr[n] = &mem_phi1[n];
            }
            n++;
        }
        mem_phi1_valid = 1;
        mem_phi1_last = export.ultimax_phi1;
    }
    return mem_phi1_ptr[addr & 0x0fff];
}

BYTE *ultimax_romh_phi2_ptr(WORD addr)
{
    int n;
    int res;
    /* DBG(("phi2 addr %04x\n", addr)); */
    n = addr & 0x0fff;
    res = ultimax_romh_phi2_read((WORD)(0x1000 + n), &mem_phi2[n]);
    if (mem_phi2_ptr[n] != (res ? &mem_phi2[n] : NULL)) {
        mem_phi2_valid = 0;
    }

    if (!mem_phi2_valid) {
        n = 0;
        while (n < 0x1000) {
            if (ultimax_romh_phi2_read((WORD)(0x1000 + n), &mem_phi2[n]) == 0) {
                mem_phi2_ptr[n] = NULL;
            } else {
                mem_phi2_ptr[n] = &mem_phi2[n];
            }
            n++;
        }
        mem_phi2_valid = 1;
        mem_phi2_last = export.ultimax_phi2;
    }
    return mem_phi2_ptr[addr & 0x0fff];
}

/*
    read from cart memory for monitor (without side effects)

    the majority of carts can use the generic fallback, custom functions
    must be provided by those carts where either:
    - the cart is not in "Main Slot"
    - "fake ultimax" mapping is used
    - memory can not be read without side effects
*/
static BYTE cartridge_peek_mem_slotmain(WORD addr)
{
    BYTE value;
    int res = CART_READ_THROUGH;

    /* "Main Slot" */
    switch (mem_cartridge_type) {
        case CARTRIDGE_ULTIMAX:
        case CARTRIDGE_GENERIC_8KB:
        case CARTRIDGE_GENERIC_16KB:
            res = generic_peek_mem((struct export_s*)&export_slotmain, addr, &value);
            break;
        case CARTRIDGE_CAPTURE:
            res = capture_peek_mem((struct export_s*)&export_slotmain, addr, &value);
            break;
        case CARTRIDGE_EXOS:
            res = exos_peek_mem((struct export_s*)&export_slotmain, addr, &value);
            break;
        case CARTRIDGE_FINAL_PLUS:
            res = final_plus_peek_mem((struct export_s*)&export_slotmain, addr, &value);
            break;
        case CARTRIDGE_FORMEL64:
            res = formel64_peek_mem((struct export_s*)&export_slotmain, addr, &value);
            break;
        case CARTRIDGE_GAME_KILLER:
            res = gamekiller_peek_mem((struct export_s*)&export_slotmain, addr, &value);
            break;
        case CARTRIDGE_MAGIC_FORMEL:
            res = magicformel_peek_mem((struct export_s*)&export_slotmain, addr, &value);
            break;
        case CARTRIDGE_RETRO_REPLAY:
            res = retroreplay_peek_mem((struct export_s*)&export_slotmain, addr, &value);
            break;
        case CARTRIDGE_STARDOS:
            res = stardos_peek_mem((struct export_s*)&export_slotmain, addr, &value);
            break;
        case CARTRIDGE_ZAXXON:
            res = zaxxon_peek_mem((struct export_s*)&export_slotmain, addr, &value);
            break;
        default:
            /* generic fallback */
            if (!export_slotmain.exrom && export_slotmain.game) {
                /* ultimax mode */
                if (addr >= 0x8000 && addr <= 0x9fff) {
                    return roml_read_slotmain(addr);
                }
                if (addr >= 0xe000) {
                    return ultimax_romh_read_hirom_slotmain(addr);
                }
            } else if (!export_slotmain.exrom && !export_slotmain.game) {
                /* 16k Game */
                if (addr >= 0x8000 && addr <= 0x9fff) {
                    return roml_read_slotmain(addr);
                }
                if (addr >= 0xa000 && addr <= 0xbfff) {
                    return romh_read_slotmain(addr);
                }
            } else if (export_slotmain.exrom && !export_slotmain.game) {
                /* 8k Game */
                if (addr >= 0x8000 && addr <= 0x9fff) {
                    return roml_read_slotmain(addr);
                }
            }
            break;
        case CARTRIDGE_NONE:
            break;
    }

    switch (res) {
        case CART_READ_VALID:
            return value;
        case CART_READ_THROUGH:
            break;
        case CART_READ_THROUGH_NO_ULTIMAX:
            break;
        case CART_READ_C64MEM:
            /* return ram_read(addr); */
            break;
    }

    return ram_read(addr);
}

static BYTE cartridge_peek_mem_slot1(WORD addr)
{
    BYTE value;
    int res = CART_READ_THROUGH;

    /* "Slot 1" */

    if (dqbb_cart_enabled()) {
        if ((res = dqbb_peek_mem(addr, &value)) == CART_READ_VALID) {
            return value;
        }
    } else if (expert_cart_enabled()) {
        if ((res = expert_peek_mem(addr, &value)) == CART_READ_VALID) {
            return value;
        }
    } else if (isepic_cart_enabled()) {
        if ((res = isepic_peek_mem(addr, &value)) == CART_READ_VALID) {
            return value;
        }
    } else if (ramcart_cart_enabled()) {
        if ((res = ramcart_peek_mem(addr, &value)) == CART_READ_VALID) {
            return value;
        }
    }

    switch (res) {
        case CART_READ_C64MEM:
            return ram_read(addr);
        case CART_READ_THROUGH_NO_ULTIMAX:
            break;
    }

    /* continue with "Main Slot" */
    return cartridge_peek_mem_slotmain(addr);
}

BYTE cartridge_peek_mem(WORD addr)
{
    BYTE value;
    int res = CART_READ_THROUGH;

    /* DBG(("CARTMEM cartridge_peek_mem (type %d addr %04x)\n", mem_cartridge_type, addr)); */

    /* "Slot 0" */

    if (magicvoice_cart_enabled()) {
        if ((res = magicvoice_peek_mem(addr, &value)) == CART_READ_VALID) {
            return value;
        }
    } else if (mmc64_cart_enabled()) {
        if ((res = mmc64_peek_mem(addr, &value)) == CART_READ_VALID) {
            return value;
        }
    } else if (tpi_cart_enabled()) {
        if ((res = tpi_peek_mem(addr, &value)) == CART_READ_VALID) {
            return value;
        }
    }

    switch (res) {
        case CART_READ_C64MEM:
            return ram_read(addr);
        case CART_READ_THROUGH_NO_ULTIMAX:
            break;
    }

    /* continue with "Slot 1" */
    return cartridge_peek_mem_slot1(addr);
}
