/*
 * actionreplay2.c - Cartridge handling, Action Replay II cart.
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#include "actionreplay.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    FIXME: everything here is purely guesswork and doesnt quite work like it should

    Action Replay 2

    Short Instructions:

    after reset, there dont seem to be any basic extensions, just a fastloader

    press freeze, then:
    - F1 / F2          Reset to Fastload
    - F7 / F8          Restart
    - left Arrow       normal Reset (Cartridge disabled)
    - F                Novaload Tape Transfer
    - D                Backup to Disk
    - M                Backup to Disk
    - S                Backup to Disk
    - C                Code Inspector
    - T                Backup to Tape
    - X                Backup to Tape
    - B                Hires Saver
    - H                Koala Saver

    Technical:

    - Freeze Button, Reset Button. rumours are that there exist ARs with a switch too
    - 16k ROM, 2*8kb banks

    io1:

    - is accessed in a loop, probably to re-enable the rom. however how exactly it does
      that remains unclear.

    io2:

    - last page of selected rom bank is visible here
    - accesses disable the ROM
*/

/*

; reset vector in bank 1
.C:8000
    .word $8030

; setup flags and stack pointer
.C:8030   78         SEI
.C:8031   D8         CLD
.C:8032   A2 FF      LDX #$FF
.C:8034   9A         TXS
; setup pla
.C:8035   A9 27      LDA #$27
.C:8037   85 01      STA $01
.C:8039   A9 2F      LDA #$2F
.C:803b   85 00      STA $00
; push some return values to stack
.C:803d   A2 0F      LDX #$0F
.C:803f   BD 47 80   LDA $8047,X
.C:8042   48         PHA
.C:8043   CA         DEX
.C:8044   10 F9      BPL $803F
.C:8046   60         RTS

.C:8047
    .word $ff84-1  1
    .word $df15-1
    .word $80f9-1
    .word $80ff-1
    .word $df15-1  2
    .word $80c3-1  3
    .word $8057-1
    .word $ff84-1

.C:ff84   4C A3 FD   JMP $FDA3 ; Init I/O Devices, Ports & Timers
...


.C:8057   A2 09      LDX #$09
.C:8059   86 C6      STX $C6

.C:805b   BD 26 80   LDA $8026,X
.C:805e   9D 76 02   STA $0276,X
.C:8061   BD B9 80   LDA $80B9,X
.C:8064   9D 0A C0   STA $C00A,X
.C:8067   CA         DEX
.C:8068   D0 F1      BNE $805B

.C:806a   A9 AA      LDA #$AA
.C:806c   8D 05 80   STA $8005

#.C:806f   20 0B C0   JSR $C00B
#.C:c00b   20 38 DF   JSR $DF38

; accessing io2 seems to disable the ROM somehow

; save akku and status on stack
.C:df38   08         PHP ; carry saved
.C:df39   48         PHA

; disable the ROM
.C:df3a   A9 FF      LDA #$FF
.C:df3c   E9 01      SBC #$01
.C:df3e   48         PHA
.C:df3f   68         PLA
.C:df40   B0 FA      BCS $DF3C

; restore akku and status
.C:df42   68         PLA
.C:df43   28         PLP ; carry restored
.C:df44   58         CLI

#.C:df45   60         RTS

.C:c00e   CD 05 80   CMP $8005

#.C:c011   4C 15 DF   JMP $DF15

.C:df15   78         SEI
; save akku and status on stack
.C:df16   08         PHP
.C:df17   48         PHA

; this loop perhaps re-enables the ROM
.C:df18   A9 0C      LDA #$0C
.C:df1a   FE 00 DE   INC $DE00,X ; x = 0
.C:df1d   FE 00 DE   INC $DE00,X
.C:df20   E9 01      SBC #$01
.C:df22   B0 F6      BCS $DF1A

; restore akku and status
.C:df24   68         PLA
.C:df25   28         PLP

#.C:df26   60         RTS

; loop forever ...
.C:8072   D0 FE      BNE $8072

*/

/*

freezer "main menu":

; wait for keypress
.C:f06c   AD 20 D0   LDA $D020
.C:f06f   48         PHA

.C:f070   A9 00      LDA #$00
.C:f072   8D 00 DC   STA $DC00
.C:f075   EE 20 D0   INC $D020
.C:f078   AE 01 DC   LDX $DC01
.C:f07b   EC 01 DC   CPX $DC01
.C:f07e   D0 F8      BNE $F078
.C:f080   E0 FF      CPX #$FF
.C:f082   F0 EC      BEQ $F070

.C:f084   68         PLA
.C:f085   8D 20 D0   STA $D020

.C:f088   A9 F7      LDA #$F7   ; check row 3
.C:f08a   20 F8 F0   JSR $F0F8

.C:f08d   A0 01      LDY #$01
.C:f08f   C9 DF      CMP #$DF   ; "H"
.C:f091   F0 5B      BEQ $F0EE
.C:f093   88         DEY
.C:f094   C9 EF      CMP #$EF   ; "B"
.C:f096   F0 56      BEQ $F0EE

.C:f098   A9 7F      LDA #$7F   ; check row 7
.C:f09a   20 F8 F0   JSR $F0F8

.C:f09d   C9 FD      CMP #$FD   ; "arrow left"
.C:f09f   F0 53      BEQ $F0F4

.C:f0a1   A9 FE      LDA #$FE   ; check row 0
.C:f0a3   20 F8 F0   JSR $F0F8

.C:f0a6   C9 F7      CMP #$F7   ; "F7"
.C:f0a8   F0 3E      BEQ $F0E8
.C:f0aa   C9 EF      CMP #$EF   ; "F1"
.C:f0ac   F0 3D      BEQ $F0EB

.C:f0ae   A9 EF      LDA #$EF   ; check row 4
.C:f0b0   20 F8 F0   JSR $F0F8

.C:f0b3   A0 81      LDY #$81
.C:f0b5   C9 EF      CMP #$EF   ; "M"
.C:f0b7   F0 2B      BEQ $F0E4

.C:f0b9   A9 FB      LDA #$FB   ; check row 2
.C:f0bb   20 F8 F0   JSR $F0F8

.C:f0be   A0 03      LDY #$03
.C:f0c0   C9 EF      CMP #$EF   ; "C"
.C:f0c2   F0 20      BEQ $F0E4
.C:f0c4   C9 DF      CMP #$DF   ; "F"
.C:f0c6   F0 29      BEQ $F0F1
.C:f0c8   A0 00      LDY #$00
.C:f0ca   C9 7F      CMP #$7F   ; "X"
.C:f0cc   F0 16      BEQ $F0E4
.C:f0ce   A0 01      LDY #$01
.C:f0d0   C9 FB      CMP #$FB   ; "D"
.C:f0d2   F0 10      BEQ $F0E4
.C:f0d4   88         DEY
.C:f0d5   C9 BF      CMP #$BF   ; "T"
.C:f0d7   F0 0B      BEQ $F0E4

.C:f0d9   A9 FD      LDA #$FD   ; check row 1
.C:f0db   20 F8 F0   JSR $F0F8

.C:f0de   A0 02      LDY #$02
.C:f0e0   C9 DF      CMP #$DF   ; "S"
.C:f0e2   D0 88      BNE $F06C

; write port A, get port B
.C:f0f8   8D 00 DC   STA $DC00
.C:f0fb   AD 01 DC   LDA $DC01
.C:f0fe   CD 01 DC   CMP $DC01
.C:f101   D0 F8      BNE $F0FB
.C:f103   60         RTS

$F0E4 Backup           Y=$00 'X' 'T' Y=$01 'D' Y=$02 'S' Y=$03 'C' Y=$81 'M'
$F0E8 Restart          Y=$00 'F7'
$F0EB Reset to Loader  Y=$00 'F1'
$F0EE Koala Saver      Y=$00 'B' Y=$01 'H'
$F0F1 Novaload Xfer    Y=$03 'F'
$F0F4 normal Reset     Y=$00 'arrow left

*/

/* #define DEBUGAR */

#ifdef DEBUGAR
#define DBG(x) printf("AR2: @%04x enabled: %d bank: %d cap %d:%d ", reg_pc, ar_enabled, roml_bank, ar_cap_disable, ar_cap_enable); printf x
#else
#define DBG(x)
#endif

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static BYTE actionreplay2_io1_read(WORD addr);
static BYTE actionreplay2_io1_peek(WORD addr);
static void actionreplay2_io1_store(WORD addr, BYTE value);
static BYTE actionreplay2_io2_read(WORD addr);
static BYTE actionreplay2_io2_peek(WORD addr);
static void actionreplay2_io2_store(WORD addr, BYTE value);

static io_source_t actionreplay2_io1_device = {
    CARTRIDGE_NAME_ACTION_REPLAY2,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0, /* read is never valid */
    actionreplay2_io1_store,
    actionreplay2_io1_read,
    actionreplay2_io1_peek,
    NULL, /* TODO: dump */
    CARTRIDGE_ACTION_REPLAY2,
    0,
    0
};

static io_source_t actionreplay2_io2_device = {
    CARTRIDGE_NAME_ACTION_REPLAY2,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    1, /* read is always valid */
    actionreplay2_io2_store,
    actionreplay2_io2_read,
    actionreplay2_io2_peek,
    NULL, /* TODO: dump */
    CARTRIDGE_ACTION_REPLAY2,
    0,
    0
};

static io_source_list_t *actionreplay2_io1_list_item = NULL;
static io_source_list_t *actionreplay2_io2_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_ACTION_REPLAY2, 1, 1, &actionreplay2_io1_device, &actionreplay2_io2_device, CARTRIDGE_ACTION_REPLAY2
};

/* ---------------------------------------------------------------------*/

/* these two values are hand tuned */
#define CAPENABLE  65
#define CAPDISABLE (50 + (7 * 16))       /* 7*16: (bytes routine at $df27 * max filename max len) */

static int ar_enabled = 0, ar_cap_enable = 0, ar_cap_disable = 0;

static void cap_charge(void)
{
    DBG(("cap_charge\n"));
    ar_cap_disable++;
    if (ar_cap_disable == CAPDISABLE) {
        ar_enabled = 0;
        ar_cap_enable = 0;
        cart_config_changed_slotmain((BYTE)(CMODE_RAM | (roml_bank << CMODE_BANK_SHIFT)), (BYTE)(CMODE_RAM | (roml_bank << CMODE_BANK_SHIFT)), CMODE_READ);
        DBG(("disabled\n"));
    }
}

static void cap_discharge(void)
{
    DBG(("cap_discharge\n"));
    ar_cap_enable++;
    if (ar_cap_enable == CAPENABLE) {
        roml_bank = 1;
        ar_enabled = 1;
        cart_config_changed_slotmain((BYTE)(CMODE_8KGAME | (roml_bank << CMODE_BANK_SHIFT)), (BYTE)(CMODE_8KGAME | (roml_bank << CMODE_BANK_SHIFT)), CMODE_READ);
        DBG(("enabled\n"));
    }
    ar_cap_disable = 0;
}

static BYTE actionreplay2_io1_read(WORD addr)
{
    cap_discharge();
    return 0;
}

static void actionreplay2_io1_store(WORD addr, BYTE value)
{
    cap_discharge();
}

static BYTE actionreplay2_io1_peek(WORD addr)
{
    return 0;
}

static BYTE actionreplay2_io2_peek(WORD addr)
{
    addr |= 0xdf00;
    return roml_banks[(addr & 0x1fff) + (1 << 13)];
}

static BYTE actionreplay2_io2_read(WORD addr)
{
    cap_charge();
    return actionreplay2_io2_peek(addr);
}

static void actionreplay2_io2_store(WORD addr, BYTE value)
{
    cap_charge();
}


/* ---------------------------------------------------------------------*/

BYTE actionreplay2_roml_read(WORD addr)
{
    if (addr < 0x9f00) {
        return roml_banks[(addr & 0x1fff) + (roml_bank << 13)];
    } else {
        return ram_read(addr);
        /* return mem_read_without_ultimax(addr); */
    }
}

BYTE actionreplay2_romh_read(WORD addr)
{
    return roml_banks[(addr & 0x1fff) + (roml_bank << 13)];
}
/* ---------------------------------------------------------------------*/

void actionreplay2_freeze(void)
{
    roml_bank = 0;
    ar_enabled = 1;
    ar_cap_enable = 0;
    ar_cap_disable = 0;
    DBG(("freeze\n"));
    cart_config_changed_slotmain((BYTE)(CMODE_ULTIMAX | (roml_bank << CMODE_BANK_SHIFT)), (BYTE)(CMODE_ULTIMAX | (roml_bank << CMODE_BANK_SHIFT)), CMODE_READ);
    cartridge_release_freeze();
}

void actionreplay2_config_init(void)
{
    roml_bank = 1;
    ar_enabled = 1;
    ar_cap_enable = 0;
    ar_cap_disable = 0;
    DBG(("config init\n"));
    cart_config_changed_slotmain((BYTE)(CMODE_8KGAME | (roml_bank << CMODE_BANK_SHIFT)), (BYTE)(CMODE_8KGAME | (roml_bank << CMODE_BANK_SHIFT)), CMODE_READ);
}

void actionreplay2_reset(void)
{
    roml_bank = 1;
    ar_enabled = 1;
    ar_cap_enable = 0;
    ar_cap_disable = 0;
    DBG(("reset\n"));
}

void actionreplay2_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x4000);
}

/* ---------------------------------------------------------------------*/

static int actionreplay2_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    actionreplay2_io1_list_item = io_source_register(&actionreplay2_io1_device);
    actionreplay2_io2_list_item = io_source_register(&actionreplay2_io2_device);

    return 0;
}

int actionreplay2_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x4000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return actionreplay2_common_attach();
}

int actionreplay2_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;
    int i;

    for (i = 0; i <= 1; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            return -1;
        }

        if (chip.bank > 1 || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }

    return actionreplay2_common_attach();
}

void actionreplay2_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(actionreplay2_io1_list_item);
    io_source_unregister(actionreplay2_io2_list_item);
    actionreplay2_io1_list_item = NULL;
    actionreplay2_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTAR2 snapshot module format:

   type  | name        | description
   ---------------------------------
   BYTE  | enabled     | cartidge enabled flag
   DWORD | cap enable  | capacitor enable counter
   DWORD | cap disable | capacitor disable counter
   ARRAY | ROML        | 16768 BYTES of ROML data
 */

static char snap_module_name[] = "CARTAR2";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int actionreplay2_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)ar_enabled) < 0)
        || (SMW_DW(m, (DWORD)ar_cap_enable) < 0)
        || (SMW_DW(m, (DWORD)ar_cap_disable) < 0)
        || (SMW_BA(m, roml_banks, 0x4000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int actionreplay2_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (vmajor > SNAP_MAJOR || vminor > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || (SMR_B_INT(m, &ar_enabled) < 0)
        || (SMR_DW_INT(m, &ar_cap_enable) < 0)
        || (SMR_DW_INT(m, &ar_cap_disable) < 0)
        || (SMR_BA(m, roml_banks, 0x4000) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    return actionreplay2_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
