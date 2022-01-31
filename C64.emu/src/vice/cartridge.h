/*
 * cartridge.h - Cartridge emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_CARTRIDGE_H
#define VICE_CARTRIDGE_H

#include "types.h"
#include "sound.h"

/*
    This is the toplevel generic cartridge interface
*/

/* init the cartridge system */
extern void cartridge_init(void);
extern int cartridge_resources_init(void);
extern int cartridge_cmdline_options_init(void);
/* shutdown the cartridge system */
extern void cartridge_shutdown(void);
extern void cartridge_resources_shutdown(void);

/* init the cartridge config so the cartridge can start (or whatever) */
extern void cartridge_init_config(void);

/* detect cartridge type (takes crt and bin files) */
extern int cartridge_detect(const char *filename);
/* attach (and enable) a cartridge by type and filename (takes crt and bin files) */
extern int cartridge_attach_image(int type, const char *filename);
/* enable cartridge by type. loads default image if any.
   should be used by the UI instead of using the resources directly */
extern int cartridge_enable(int type);

/* disable cartridge by type */
extern int cartridge_disable(int type);

/* detaches/disables the cartridge with the associated id. pass -1 to detach all */
extern void cartridge_detach_image(int type);

/* FIXME: this should also be made a generic function that takes the type */
/* set current "Main Slot" cart as default */
extern void cartridge_set_default(void);

void cartridge_unset_default(void);

/* reset button pressed in UI */
extern void cartridge_reset(void);
/* powerup / hardreset */
extern void cartridge_powerup(void);

/* FIXME: this should also be made a generic function that takes the type */
/* freeze button pressed in UI */
extern void cartridge_trigger_freeze(void);

/* FIXME: this should also be made a generic function that takes the type */
/* trigger a freeze, but don't trigger the cartridge logic (which might release it). used by monitor */
extern void cartridge_trigger_freeze_nmi_only(void);

/* FIXME: this should also be made a generic function that takes the type */
extern void cartridge_release_freeze(void);

extern const char *cartridge_get_file_name(int type);
extern int cartridge_type_enabled(int type);

/* save the (rom/ram)image of the give cart type to a file */
extern int cartridge_save_image(int type, const char *filename);
extern int cartridge_bin_save(int type, const char *filename);
extern int cartridge_crt_save(int type, const char *filename);
extern int cartridge_flush_image(int type);

/* returns 1 when cartridge (ROM) image can be flushed */
extern int cartridge_can_flush_image(int crtid);
/* returns 1 when cartridge (ROM) image can be saved */
extern int cartridge_can_save_image(int crtid);

/* load/write snapshot modules for attached cartridges */
struct snapshot_s;
extern int cartridge_snapshot_read_modules(struct snapshot_s *s);
extern int cartridge_snapshot_write_modules(struct snapshot_s *s);

/* setup context */
struct machine_context_s;
extern void cartridge_setup_context(struct machine_context_s *machine_context);

/* generic cartridge memory peek for the monitor */
extern uint8_t cartridge_peek_mem(uint16_t addr);

/* mmu translation */
extern void cartridge_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit);

/* Initialize RAM for power-up.  */
extern void cartridge_ram_init(void);

extern void cartridge_sound_chip_init(void);

/* Carts that don't have a rom image */
#define CARTRIDGE_DIGIMAX            -100 /* digimax.c */
#define CARTRIDGE_DQBB               -101 /* dqbb.c */
#define CARTRIDGE_GEORAM             -102 /* georam.c */
#define CARTRIDGE_ISEPIC             -103 /* isepic.c */
#define CARTRIDGE_RAMCART            -104 /* ramcart.c */
#define CARTRIDGE_REU                -105 /* reu.c */
#define CARTRIDGE_SFX_SOUND_EXPANDER -106 /* sfx_soundexpander.c, fmopl.c */
#define CARTRIDGE_SFX_SOUND_SAMPLER  -107 /* sfx_soundsampler.c */
#define CARTRIDGE_MIDI_PASSPORT      -108 /* c64-midi.c */
#define CARTRIDGE_MIDI_DATEL         -109 /* c64-midi.c */
#define CARTRIDGE_MIDI_SEQUENTIAL    -110 /* c64-midi.c */
#define CARTRIDGE_MIDI_NAMESOFT      -111 /* c64-midi.c */
#define CARTRIDGE_MIDI_MAPLIN        -112 /* c64-midi.c */
#define CARTRIDGE_DS12C887RTC        -113 /* ds12c887rtc.c */
#define CARTRIDGE_TFE                -116 /* ethernetcart.c */
#define CARTRIDGE_TURBO232           -117 /* c64acia1.c */
#define CARTRIDGE_SWIFTLINK          -118 /* c64acia1.c */
#define CARTRIDGE_ACIA               -119 /* c64acia1.c */
#define CARTRIDGE_PLUS60K            -120 /* plus60k.c */
#define CARTRIDGE_PLUS256K           -121 /* plus256k.c */
#define CARTRIDGE_C64_256K           -122 /* c64_256k.c */
#define CARTRIDGE_CPM                -123 /* cpmcart.c */
#define CARTRIDGE_DEBUGCART          -124 /* debugcart.c */

/* Known cartridge types.  */
/* TODO: cartconv (4k and 12k binaries) */
#define CARTRIDGE_ULTIMAX              -6 /* generic.c */
#define CARTRIDGE_GENERIC_8KB          -3 /* generic.c */
#define CARTRIDGE_GENERIC_16KB         -2 /* generic.c */

#define CARTRIDGE_NONE                 -1
#define CARTRIDGE_CRT                   0

/* the following must match the CRT IDs */
#define CARTRIDGE_ACTION_REPLAY         1 /* actionreplay.c */
#define CARTRIDGE_KCS_POWER             2 /* kcs.c */
#define CARTRIDGE_FINAL_III             3 /* final3.c */
#define CARTRIDGE_SIMONS_BASIC          4 /* simonsbasic.c */
#define CARTRIDGE_OCEAN                 5 /* ocean.c */
#define CARTRIDGE_EXPERT                6 /* expert.c */
#define CARTRIDGE_FUNPLAY               7 /* funplay.c */
#define CARTRIDGE_SUPER_GAMES           8 /* supergames.c */
#define CARTRIDGE_ATOMIC_POWER          9 /* atomicpower.c */
#define CARTRIDGE_EPYX_FASTLOAD        10 /* epyxfastload.c */
#define CARTRIDGE_WESTERMANN           11 /* westermann.c */
#define CARTRIDGE_REX                  12 /* rexutility.c */
#define CARTRIDGE_FINAL_I              13 /* final.c */
#define CARTRIDGE_MAGIC_FORMEL         14 /* magicformel.c */
#define CARTRIDGE_GS                   15 /* gs.c */
#define CARTRIDGE_WARPSPEED            16 /* warpspeed.c */
#define CARTRIDGE_DINAMIC              17 /* dinamic.c */
#define CARTRIDGE_ZAXXON               18 /* zaxxon.c */
#define CARTRIDGE_MAGIC_DESK           19 /* magicdesk.c */
#define CARTRIDGE_SUPER_SNAPSHOT_V5    20 /* supersnapshot.c */
#define CARTRIDGE_COMAL80              21 /* comal80.c */
#define CARTRIDGE_STRUCTURED_BASIC     22 /* stb.c */
#define CARTRIDGE_ROSS                 23 /* ross.c */
#define CARTRIDGE_DELA_EP64            24 /* delaep64.c */
#define CARTRIDGE_DELA_EP7x8           25 /* delaep7x8.c */
#define CARTRIDGE_DELA_EP256           26 /* delaep256.c */

/* TODO: cartconv */
#define CARTRIDGE_REX_EP256            27 /* rexep256.c */

#define CARTRIDGE_MIKRO_ASSEMBLER      28 /* mikroass.c */

/* TODO: cartconv (24k binary) */
#define CARTRIDGE_FINAL_PLUS           29 /* finalplus.c */

#define CARTRIDGE_ACTION_REPLAY4       30 /* actionreplay4.c */
#define CARTRIDGE_STARDOS              31 /* stardos.c */
#define CARTRIDGE_EASYFLASH            32 /* easyflash.c */

/* TODO: cartconv (no cart exists?) */
#define CARTRIDGE_EASYFLASH_XBANK      33 /* easyflash.c */

#define CARTRIDGE_CAPTURE              34 /* capture.c */
#define CARTRIDGE_ACTION_REPLAY3       35 /* actionreplay3.c */
#define CARTRIDGE_RETRO_REPLAY         36 /* retroreplay.c */
#define CARTRIDGE_MMC64                37 /* mmc64.c, spi-sdcard.c */
#define CARTRIDGE_MMC_REPLAY           38 /* mmcreplay.c, ser-eeprom.c, spi-sdcard.c */
#define CARTRIDGE_IDE64                39 /* ide64.c */
#define CARTRIDGE_SUPER_SNAPSHOT       40 /* supersnapshot4.c */
#define CARTRIDGE_IEEE488              41 /* c64tpi.c */
#define CARTRIDGE_GAME_KILLER          42 /* gamekiller.c */
#define CARTRIDGE_P64                  43 /* prophet64.c */
#define CARTRIDGE_EXOS                 44 /* exos.c */
#define CARTRIDGE_FREEZE_FRAME         45 /* freezeframe.c */
#define CARTRIDGE_FREEZE_MACHINE       46 /* freezemachine.c */
#define CARTRIDGE_SNAPSHOT64           47 /* snapshot64.c */
#define CARTRIDGE_SUPER_EXPLODE_V5     48 /* superexplode5.c */
#define CARTRIDGE_MAGIC_VOICE          49 /* magicvoice.c, tpicore.c, t6721.c */
#define CARTRIDGE_ACTION_REPLAY2       50 /* actionreplay2.c */
#define CARTRIDGE_MACH5                51 /* mach5.c */
#define CARTRIDGE_DIASHOW_MAKER        52 /* diashowmaker.c */
#define CARTRIDGE_PAGEFOX              53 /* pagefox.c */
#define CARTRIDGE_KINGSOFT             54 /* kingsoft.c */
#define CARTRIDGE_SILVERROCK_128       55 /* silverrock128.c */
#define CARTRIDGE_FORMEL64             56 /* formel64.c */
#define CARTRIDGE_RGCD                 57 /* rgcd.c */
#define CARTRIDGE_RRNETMK3             58 /* rrnetmk3.c */
#define CARTRIDGE_EASYCALC             59 /* easycalc.c */
#define CARTRIDGE_GMOD2                60 /* gmod2.c */
#define CARTRIDGE_MAX_BASIC            61 /* maxbasic.c */
#define CARTRIDGE_GMOD3                62 /* gmod3.c */
#define CARTRIDGE_ZIPPCODE48           63 /* zippcode48.c */
#define CARTRIDGE_BLACKBOX8            64 /* blackbox8.c */
#define CARTRIDGE_BLACKBOX3            65 /* blackbox3.c */
#define CARTRIDGE_BLACKBOX4            66 /* blackbox4.c */
#define CARTRIDGE_REX_RAMFLOPPY        67 /* rexramfloppy.c */
#define CARTRIDGE_BISPLUS              68 /* bisplus.c */
#define CARTRIDGE_SDBOX                69 /* sdbox.c */
#define CARTRIDGE_MULTIMAX             70 /* multimax.c */
#define CARTRIDGE_BLACKBOX9            71 /* blackbox9.c */
#define CARTRIDGE_LT_KERNAL            72 /* ltkernal.c */
#define CARTRIDGE_RAMLINK              73 /* ramlink.c */
#define CARTRIDGE_DREAN                74 /* drean.c */
#define CARTRIDGE_IEEEFLASH64          75 /* ieeeflash64.c */
#define CARTRIDGE_TURTLE_GRAPHICS_II   76 /* turtlegraphics.c */
#define CARTRIDGE_FREEZE_FRAME_MK2     77 /* freezeframe2.c */
#define CARTRIDGE_LAST                 77 /* cartconv: last cartridge in list */

/* list of canonical names for the c64 cartridges:
   note: often it is hard to determine "the" official name, let alone the way it
   should be capitalized. because of that we go by the following rules:
   - if even the actual spelling and/or naming is unclear, then the most "common"
     variant is choosen ("Expert Cartridge" vs "The Expert")
   - in many cases the name is printed all uppercase both on screen and in other
     sources (manual, adverts). we refrain from doing the same here and convert
     to Camel Case ("ACTION REPLAY V5" -> "Action Replay V5"), *except* if the
     cart name constitutes an actual name (as in noun) by itself ("ISEPIC", "EXOS").
     additionally common abrevations such as RAM or EPROM will get written uppercase
     if in doubt.
   - although generally these cartridge names should never get translated, some
     generic stuff is translated to english ("EPROM Karte" -> "EPROM Cart")
*/
#define CARTRIDGE_NAME_ACIA               "ACIA"
#define CARTRIDGE_NAME_ACTION_REPLAY      "Action Replay V5" /* http://rr.pokefinder.org/wiki/Action_Replay */
#define CARTRIDGE_NAME_ACTION_REPLAY2     "Action Replay MK2" /* http://rr.pokefinder.org/wiki/Action_Replay */
#define CARTRIDGE_NAME_ACTION_REPLAY3     "Action Replay MK3" /* http://rr.pokefinder.org/wiki/Action_Replay */
#define CARTRIDGE_NAME_ACTION_REPLAY4     "Action Replay MK4" /* http://rr.pokefinder.org/wiki/Action_Replay */
#define CARTRIDGE_NAME_ATOMIC_POWER       "Atomic Power" /* also: "Nordic Power" */ /* http://rr.pokefinder.org/wiki/Nordic_Power */
#define CARTRIDGE_NAME_BISPLUS            "BIS-Plus"
#define CARTRIDGE_NAME_BLACKBOX3          "Blackbox V3"
#define CARTRIDGE_NAME_BLACKBOX4          "Blackbox V4"
#define CARTRIDGE_NAME_BLACKBOX8          "Blackbox V8"
#define CARTRIDGE_NAME_BLACKBOX9          "Blackbox V9"
#define CARTRIDGE_NAME_GS                 "C64 Games System" /* http://retro.lonningdal.net/home.php?page=Computers&select=c64gs&image=c64gs4.jpg */
#define CARTRIDGE_NAME_CAPTURE            "Capture" /* see manual http://rr.pokefinder.org/wiki/Capture */
#define CARTRIDGE_NAME_COMAL80            "Comal 80" /* http://www.retroport.de/C64_C128_Hardware.html */
#define CARTRIDGE_NAME_CPM                "CP/M cartridge"
#define CARTRIDGE_NAME_MIDI_DATEL         "Datel MIDI"
#define CARTRIDGE_NAME_DEBUGCART          "Debug Cartridge"
#define CARTRIDGE_NAME_DELA_EP64          "Dela EP64"
#define CARTRIDGE_NAME_DELA_EP7x8         "Dela EP7x8"
#define CARTRIDGE_NAME_DELA_EP256         "Dela EP256"
#define CARTRIDGE_NAME_DIASHOW_MAKER      "Diashow-Maker" /* http://www.retroport.de/Rex.html */
#define CARTRIDGE_NAME_DIGIMAX            "DigiMAX" /* http://starbase.globalpc.net/~ezekowitz/vanessa/hobbies/projects.html */
#define CARTRIDGE_NAME_DINAMIC            "Dinamic"
#define CARTRIDGE_NAME_DQBB               "Double Quick Brown Box" /* on the cart itself its all uppercase ? */
#define CARTRIDGE_NAME_DS12C887RTC        "DS12C887 Real Time Clock" /* Title of the page at http://ytm.bossstation.dnsalias.org/html/rtcds12c887.html */
#define CARTRIDGE_NAME_EASYCALC           "Easy Calc Result" /* on the cart itself it's "Calc Result EASY", in the manual it's EASYCALC, but we'll go with what is defined ;) */
#define CARTRIDGE_NAME_EASYFLASH          "EasyFlash" /* see http://skoe.de/easyflash/ */
#define CARTRIDGE_NAME_EASYFLASH_XBANK    "EasyFlash Xbank" /* see http://skoe.de/easyflash/ */
#define CARTRIDGE_NAME_EPYX_FASTLOAD      "Epyx FastLoad" /* http://rr.pokefinder.org/wiki/Epyx_FastLoad */
#define CARTRIDGE_NAME_ETHERNETCART       "Ethernet cartridge"
#define CARTRIDGE_NAME_EXOS               "EXOS" /* http://rr.pokefinder.org/wiki/ExOS */
#define CARTRIDGE_NAME_EXPERT             "Expert Cartridge" /* http://rr.pokefinder.org/wiki/Expert_Cartridge */
#define CARTRIDGE_NAME_FINAL_I            "The Final Cartridge" /* http://rr.pokefinder.org/wiki/Final_Cartridge */
#define CARTRIDGE_NAME_FINAL_III          "The Final Cartridge III" /* http://rr.pokefinder.org/wiki/Final_Cartridge */
#define CARTRIDGE_NAME_FINAL_PLUS         "Final Cartridge Plus" /* http://rr.pokefinder.org/wiki/Final_Cartridge */
#define CARTRIDGE_NAME_TFE                "The Final Ethernet"
#define CARTRIDGE_NAME_FORMEL64           "Formel 64"
#define CARTRIDGE_NAME_FREEZE_FRAME       "Freeze Frame" /* http://rr.pokefinder.org/wiki/Freeze_Frame */
#define CARTRIDGE_NAME_FREEZE_FRAME_MK2   "Freeze Frame MK2" /* http://rr.pokefinder.org/wiki/Freeze_Frame */
#define CARTRIDGE_NAME_FREEZE_MACHINE     "Freeze Machine" /* http://rr.pokefinder.org/wiki/Freeze_Frame */
#define CARTRIDGE_NAME_FUNPLAY            "Fun Play" /* also: "Power Play" */ /* http://home.nomansland.biz/~zerqent/commodore_salg/CIMG2132.JPG */
#define CARTRIDGE_NAME_GAME_KILLER        "Game Killer" /* http://rr.pokefinder.org/wiki/Game_Killer */
#define CARTRIDGE_NAME_GEORAM             "GEO-RAM" /* http://www.retroport.de/Rex.html */
#define CARTRIDGE_NAME_GMOD2              "GMod2" /* http://wiki.icomp.de/wiki/GMod2 */
#define CARTRIDGE_NAME_GMOD3              "GMod3" /* http://wiki.icomp.de/wiki/GMod3 */
#define CARTRIDGE_NAME_DREAN              "Drean"
#define CARTRIDGE_NAME_IDE64              "IDE64" /* see http://www.ide64.org/ */
#define CARTRIDGE_NAME_IEEE488            "IEEE-488 Interface"
#define CARTRIDGE_NAME_IEEEFLASH64        "IEEE Flash! 64"
#define CARTRIDGE_NAME_ISEPIC             "ISEPIC" /* http://rr.pokefinder.org/wiki/Isepic */
#define CARTRIDGE_NAME_KCS_POWER          "KCS Power Cartridge" /* http://rr.pokefinder.org/wiki/Power_Cartridge */
#define CARTRIDGE_NAME_KINGSOFT           "Kingsoft"
#define CARTRIDGE_NAME_LT_KERNAL          "Lt. Kernal Host Adaptor"
#define CARTRIDGE_NAME_MACH5              "MACH 5" /* http://rr.pokefinder.org/wiki/MACH_5 */
#define CARTRIDGE_NAME_MAGIC_DESK         "Magic Desk" /* also: "Domark, Hes Australia" */
#define CARTRIDGE_NAME_MAGIC_FORMEL       "Magic Formel" /* http://rr.pokefinder.org/wiki/Magic_Formel */
#define CARTRIDGE_NAME_MAGIC_VOICE        "Magic Voice" /* all lowercase on cart ? */
#define CARTRIDGE_NAME_MAX_BASIC          "MAX Basic"
#define CARTRIDGE_NAME_MIDI_MAPLIN        "Maplin MIDI"
#define CARTRIDGE_NAME_MIKRO_ASSEMBLER    "Mikro Assembler"
#define CARTRIDGE_NAME_MMC64              "MMC64" /* see manual */
#define CARTRIDGE_NAME_MMC_REPLAY         "MMC Replay" /* see manual */
#define CARTRIDGE_NAME_MIDI_NAMESOFT      "Namesoft MIDI"
#define CARTRIDGE_NAME_MIDI_PASSPORT      "Passport MIDI"
#define CARTRIDGE_NAME_MIDI_SEQUENTIAL    "Sequential MIDI"
#define CARTRIDGE_NAME_MULTIMAX           "MultiMAX" /* http://www.multimax.co/ */
#define CARTRIDGE_NAME_NORDIC_REPLAY      "Nordic Replay" /* "Retro Replay v2" see manual */
#define CARTRIDGE_NAME_OCEAN              "Ocean"
#define CARTRIDGE_NAME_PAGEFOX            "Pagefox"
#define CARTRIDGE_NAME_P64                "Prophet64" /* see http://www.prophet64.com/ */
#define CARTRIDGE_NAME_RAMCART            "RamCart" /* see cc65 driver */
#define CARTRIDGE_NAME_RAMLINK            "RAMLink"
#define CARTRIDGE_NAME_REU                "RAM Expansion Module" /* http://www.retroport.de/C64_C128_Hardware.html */
#define CARTRIDGE_NAME_REX_EP256          "REX 256K EPROM Cart" /* http://www.retroport.de/Rex.html */
#define CARTRIDGE_NAME_REX                "REX Utility"
#define CARTRIDGE_NAME_REX_RAMFLOPPY      "REX RAM-Floppy"
#define CARTRIDGE_NAME_RGCD               "RGCD"
#define CARTRIDGE_NAME_RRNET              "RR-Net" /* see manual */
#define CARTRIDGE_NAME_RRNETMK3           "RR-Net MK3" /* see manual */
#define CARTRIDGE_NAME_RETRO_REPLAY       "Retro Replay" /* see manual */
#define CARTRIDGE_NAME_ROSS               "ROSS"
#define CARTRIDGE_NAME_SDBOX              "SD-BOX" /* http://c64.com.pl/index.php/sdbox106.html */
#define CARTRIDGE_NAME_SFX_SOUND_EXPANDER "SFX Sound Expander" /* http://www.floodgap.com/retrobits/ckb/secret/cbm-sfx-fmbport.jpg */
#define CARTRIDGE_NAME_SFX_SOUND_SAMPLER  "SFX Sound Sampler" /* http://www.floodgap.com/retrobits/ckb/secret/cbm-ssm-box.jpg */
#define CARTRIDGE_NAME_SILVERROCK_128     "Silverrock 128KiB Cartridge"
#define CARTRIDGE_NAME_SIMONS_BASIC       "Simons' BASIC" /* http://en.wikipedia.org/wiki/Simons'_BASIC */
#define CARTRIDGE_NAME_SNAPSHOT64         "Snapshot 64" /* http://rr.pokefinder.org/wiki/Super_Snapshot */
#define CARTRIDGE_NAME_STARDOS            "Stardos" /* see manual http://rr.pokefinder.org/wiki/StarDOS */
#define CARTRIDGE_NAME_STRUCTURED_BASIC   "Structured BASIC"
#define CARTRIDGE_NAME_SUPER_EXPLODE_V5   "Super Explode V5.0" /* http://rr.pokefinder.org/wiki/Super_Explode */
#define CARTRIDGE_NAME_SUPER_GAMES        "Super Games"
#define CARTRIDGE_NAME_SUPER_SNAPSHOT     "Super Snapshot V4" /* http://rr.pokefinder.org/wiki/Super_Snapshot */
#define CARTRIDGE_NAME_SUPER_SNAPSHOT_V5  "Super Snapshot V5" /* http://rr.pokefinder.org/wiki/Super_Snapshot */
#define CARTRIDGE_NAME_SWIFTLINK          "Swiftlink" /* http://mikenaberezny.com/hardware/peripherals/swiftlink-rs232-interface/ */
#define CARTRIDGE_NAME_TURTLE_GRAPHICS_II "HES Turtle Graphics II"
#define CARTRIDGE_NAME_TURBO232           "Turbo232" /* also: "ACIA/SWIFTLINK" */ /*http://www.retroport.de/C64_C128_Hardware2.html */
#define CARTRIDGE_NAME_WARPSPEED          "Warp Speed" /* see manual http://rr.pokefinder.org/wiki/WarpSpeed */
#define CARTRIDGE_NAME_WESTERMANN         "Westermann Learning"
#define CARTRIDGE_NAME_ZAXXON             "Zaxxon"
#define CARTRIDGE_NAME_ZIPPCODE48         "ZIPP-CODE 48"

#define CARTRIDGE_NAME_GENERIC_8KB        "generic 8KiB game"
#define CARTRIDGE_NAME_GENERIC_16KB       "generic 16KiB game"
#define CARTRIDGE_NAME_ULTIMAX            "generic Ultimax"

/*
 * C128 cartridge system
 */

#define CARTRIDGE_C128_WARPSPEED128 1
#define CARTRIDGE_C128_LAST 1

#define CARTRIDGE_C128_NAME_WARPSPEED128  "Warp Speed 128"

/*
 * VIC20 cartridge system
 */

/* I/O only C64 cartridges can be used with the "Masquerade" adapter */
/* FIXME: currently we need to make sure to use the same IDs here as for C64 */
#define CARTRIDGE_VIC20_DIGIMAX            -100 /* digimax.c */
#define CARTRIDGE_VIC20_GEORAM             -102 /* georam.c */
#define CARTRIDGE_VIC20_SFX_SOUND_EXPANDER -106 /* sfx_soundexpander.c, fmopl.c */
#define CARTRIDGE_VIC20_SFX_SOUND_SAMPLER  -107 /* sfx_soundsampler.c */
#define CARTRIDGE_VIC20_MIDI_PASSPORT      -108 /* c64-midi.c */
#define CARTRIDGE_VIC20_MIDI_DATEL         -109 /* c64-midi.c */
#define CARTRIDGE_VIC20_MIDI_SEQUENTIAL    -110 /* c64-midi.c */
#define CARTRIDGE_VIC20_MIDI_NAMESOFT      -111 /* c64-midi.c */
#define CARTRIDGE_VIC20_MIDI_MAPLIN        -112 /* c64-midi.c */
#define CARTRIDGE_VIC20_DS12C887RTC        -113 /* ds12c887rtc.c */
#define CARTRIDGE_VIC20_TFE                -116 /* ethernetcart.c */
#define CARTRIDGE_VIC20_TURBO232           -117 /* c64acia1.c */
#define CARTRIDGE_VIC20_SWIFTLINK          -118 /* c64acia1.c */
#define CARTRIDGE_VIC20_ACIA               -119 /* c64acia1.c */

#define CARTRIDGE_VIC20_DEBUGCART          -124 /* debugcart.c */

#define CARTRIDGE_VIC20_GENERIC              -2 /* generic.c */

#define CARTRIDGE_VIC20_SIDCART             -10 /* vic20-sidcart.c */
#define CARTRIDGE_VIC20_IEEE488             -11 /* vic20-ieee488.c */
#define CARTRIDGE_VIC20_IO2_RAM             -12 /* ioramcart.c */
#define CARTRIDGE_VIC20_IO3_RAM             -13 /* ioramcart.c */

/* #define CARTRIDGE_NONE               -1 */
/* #define CARTRIDGE_CRT                 0 */

/* the following must match the CRT IDs */
#define CARTRIDGE_VIC20_MEGACART        1   /* megacart.c */
#define CARTRIDGE_VIC20_BEHRBONZ        2   /* behrbonz.c */
#define CARTRIDGE_VIC20_FP              3   /* vic-fp.c */
#define CARTRIDGE_VIC20_UM              4   /* ultimem.c */
#define CARTRIDGE_VIC20_FINAL_EXPANSION 5   /* finalexpansion.c */

#define CARTRIDGE_VIC20_LAST            5   /* cartconv: last cartridge in list */

/*
 * VIC20 Generic cartridges
 *
 * The cartridge types below are only used during attach requests.
 * They will always be converted to CARTRIDGE_VIC20_GENERIC when
 * attached.  This also means they can be remapped at will.
 *
 * bit 0:  uses block 5 (a000-bfff)
 *     1:  uses block 3 (6000-7fff)
 *     2:  uses block 2 (4000-5fff)
 *     3:  uses block 1 (2000-3fff)
 *
 * bit 4+5:  0,0=1k, 0,1=2k 1,0=4k 1,1=8k (only useful if just one block is used)
 * bit 6:    starts at 0=first, 1=second 4k in block (only useful for 1k/2k/4k images)
 */
#define CARTRIDGE_VIC20_DETECT       0x8000

#define CARTRIDGE_VIC20_TYPEDEF(h, s1, s0, b1, b2, b3, b5) \
    (0x8000 | ((h) << 6) | ((s1) << 5) | ((s0) << 4) | ((b1) << 3) | ((b2) << 2) | ((b3) << 1) | ((b5) << 0))

/* block 1 */    
#define CARTRIDGE_VIC20_4KB_2000        CARTRIDGE_VIC20_TYPEDEF(0, 1, 0,  1, 0, 0, 0)
#define CARTRIDGE_VIC20_8KB_2000        CARTRIDGE_VIC20_TYPEDEF(0, 1, 1,  1, 0, 0, 0)
#define CARTRIDGE_VIC20_4KB_3000        CARTRIDGE_VIC20_TYPEDEF(1, 1, 0,  1, 0, 0, 0)
/* block 2 */
#define CARTRIDGE_VIC20_4KB_4000        CARTRIDGE_VIC20_TYPEDEF(0, 1, 0,  0, 1, 0, 0)
#define CARTRIDGE_VIC20_8KB_4000        CARTRIDGE_VIC20_TYPEDEF(0, 1, 1,  0, 1, 0, 0)
#define CARTRIDGE_VIC20_4KB_5000        CARTRIDGE_VIC20_TYPEDEF(1, 1, 0,  0, 1, 0, 0)
/* block 3 */
#define CARTRIDGE_VIC20_4KB_6000        CARTRIDGE_VIC20_TYPEDEF(0, 1, 0,  0, 0, 1, 0)
#define CARTRIDGE_VIC20_8KB_6000        CARTRIDGE_VIC20_TYPEDEF(0, 1, 1,  0, 0, 1, 0)
#define CARTRIDGE_VIC20_4KB_7000        CARTRIDGE_VIC20_TYPEDEF(1, 1, 0,  0, 0, 1, 0)
/* block 5 */
#define CARTRIDGE_VIC20_4KB_A000        CARTRIDGE_VIC20_TYPEDEF(0, 0, 0,  0, 0, 0, 1)
#define CARTRIDGE_VIC20_8KB_A000        CARTRIDGE_VIC20_TYPEDEF(0, 1, 1,  0, 0, 0, 1)
#define CARTRIDGE_VIC20_4KB_B000        CARTRIDGE_VIC20_TYPEDEF(1, 1, 0,  0, 0, 0, 1)
#define CARTRIDGE_VIC20_2KB_B000        CARTRIDGE_VIC20_TYPEDEF(1, 0, 1,  0, 0, 0, 1)

/* block 1 + block 2 */
#define CARTRIDGE_VIC20_16KB_2000       CARTRIDGE_VIC20_TYPEDEF(0, 1, 1,  1, 1, 0, 0) 
/* block 1 + block 5 */
#define CARTRIDGE_VIC20_16KB_2000_A000  CARTRIDGE_VIC20_TYPEDEF(0, 1, 1,  1, 0, 0, 1) 
/* block 2 + block 3 */
#define CARTRIDGE_VIC20_16KB_4000       CARTRIDGE_VIC20_TYPEDEF(0, 1, 1,  0, 1, 1, 0) 
/* block 2 + block 5 */
#define CARTRIDGE_VIC20_16KB_4000_A000  CARTRIDGE_VIC20_TYPEDEF(0, 1, 1,  0, 1, 0, 1) 
/* block 3 + block 5 */
#define CARTRIDGE_VIC20_16KB_6000       CARTRIDGE_VIC20_TYPEDEF(0, 1, 1,  0, 0, 1, 1) 

/* block 1,2,3,5 */
#define CARTRIDGE_VIC20_32KB_2000       CARTRIDGE_VIC20_TYPEDEF(0, 1, 1,  1, 1, 1, 1)

/* list of canonical names for the VIC20 cartridges: */
#define CARTRIDGE_VIC20_NAME_BEHRBONZ        "Behr Bonz"
#define CARTRIDGE_VIC20_NAME_FINAL_EXPANSION "Final Expansion"
#define CARTRIDGE_VIC20_NAME_MEGACART        "Mega-Cart" /* http://mega-cart.com/ */
#define CARTRIDGE_VIC20_NAME_UM              "UltiMem"
#define CARTRIDGE_VIC20_NAME_FP              "Vic Flash Plugin" /* http://www.ktverkko.fi/~msmakela/8bit/vfp/index.en.html */
#define CARTRIDGE_VIC20_NAME_IO2_RAM         "I/O-2 RAM"
#define CARTRIDGE_VIC20_NAME_IO3_RAM         "I/O-3 RAM"
#define CARTRIDGE_VIC20_NAME_IEEE488         "IEEE488"
#define CARTRIDGE_VIC20_NAME_MIDI            "MIDI"
#define CARTRIDGE_VIC20_NAME_SIDCART         "SIDCART"

/*
 * plus4 cartridge system
 */

#define CARTRIDGE_PLUS4_GENERIC         -2 /* plus4-generic.c */

/* #define CARTRIDGE_NONE               -1 */
#define CARTRIDGE_PLUS4_MAGIC           1   /* c264 magic cart */
#define CARTRIDGE_PLUS4_MULTI           2   /* plus4 multi cart */
#define CARTRIDGE_PLUS4_JACINT1MB       3   /* 1MB Cartridge */

#define CARTRIDGE_PLUS4_LAST            3

/* FIXME: get rid of this */
#define CARTRIDGE_PLUS4_DETECT          0x8200 /* low byte must be 0x00 */

#define CARTRIDGE_PLUS4_GENERIC_TYPE_MASK    0x000f

#define CARTRIDGE_PLUS4_IS_GENERIC(x)   (((x) & 0xff00) == 0x8200)

#define CARTRIDGE_PLUS4_GENERIC_C1LO    0x8201
#define CARTRIDGE_PLUS4_GENERIC_C1HI    0x8202
#define CARTRIDGE_PLUS4_GENERIC_C2LO    0x8204
#define CARTRIDGE_PLUS4_GENERIC_C2HI    0x8208
#define CARTRIDGE_PLUS4_GENERIC_C1      (CARTRIDGE_PLUS4_GENERIC_C1LO | CARTRIDGE_PLUS4_GENERIC_C1HI)
#define CARTRIDGE_PLUS4_GENERIC_C2      (CARTRIDGE_PLUS4_GENERIC_C2LO | CARTRIDGE_PLUS4_GENERIC_C2HI)
#define CARTRIDGE_PLUS4_GENERIC_ALL     (CARTRIDGE_PLUS4_GENERIC_C1 | CARTRIDGE_PLUS4_GENERIC_C2)

/* list of canonical names for the Plus4 cartridges: */
#define CARTRIDGE_PLUS4_NAME_JACINT1MB  "1MB Cartridge"
#define CARTRIDGE_PLUS4_NAME_MAGIC      "c264 magic cart"
#define CARTRIDGE_PLUS4_NAME_MULTI      "Plus4 multi cart"

/*
 * cbm2 cartridge system
 */
/* #define CARTRIDGE_NONE               -1 */
/* #define CARTRIDGE_CBM2_DETECT          0x9000 */ /* not implemented yet */
#define CARTRIDGE_CBM2_8KB_1000        0x9001
#define CARTRIDGE_CBM2_8KB_2000        0x9002
#define CARTRIDGE_CBM2_16KB_4000       0x9004
#define CARTRIDGE_CBM2_16KB_6000       0x9008

/* FIXME: cartconv: the sizes are used in a bitfield and also by their absolute values */
#define CARTRIDGE_SIZE_2KB     0x00000800
#define CARTRIDGE_SIZE_4KB     0x00001000
#define CARTRIDGE_SIZE_8KB     0x00002000
#define CARTRIDGE_SIZE_12KB    0x00003000
#define CARTRIDGE_SIZE_16KB    0x00004000
#define CARTRIDGE_SIZE_20KB    0x00005000
#define CARTRIDGE_SIZE_24KB    0x00006000
#define CARTRIDGE_SIZE_32KB    0x00008000
#define CARTRIDGE_SIZE_64KB    0x00010000
#define CARTRIDGE_SIZE_96KB    0x00018000
#define CARTRIDGE_SIZE_128KB   0x00020000
#define CARTRIDGE_SIZE_256KB   0x00040000
#define CARTRIDGE_SIZE_512KB   0x00080000
#define CARTRIDGE_SIZE_1024KB  0x00100000
#define CARTRIDGE_SIZE_2048KB  0x00200000
#define CARTRIDGE_SIZE_4096KB  0x00400000
#define CARTRIDGE_SIZE_8192KB  0x00800000
#define CARTRIDGE_SIZE_16384KB 0x01000000
#define CARTRIDGE_SIZE_MAX     CARTRIDGE_SIZE_16384KB

/* for convenience */
#define CARTRIDGE_SIZE_1MB      CARTRIDGE_SIZE_1024KB
#define CARTRIDGE_SIZE_2MB      CARTRIDGE_SIZE_2048KB
#define CARTRIDGE_SIZE_4MB      CARTRIDGE_SIZE_4096KB
#define CARTRIDGE_SIZE_8MB      CARTRIDGE_SIZE_8192KB
#define CARTRIDGE_SIZE_16MB     CARTRIDGE_SIZE_16384KB

#define CARTRIDGE_FILETYPE_BIN  1
#define CARTRIDGE_FILETYPE_CRT  2

/* FIXME: merge the cartridge list with the one used by cartconv */

/* cartridge info for GUIs */
typedef struct {
    char *name;
    int crtid;
    unsigned int flags;
} cartridge_info_t;

#define CARTRIDGE_GROUP_GENERIC         0x0001
#define CARTRIDGE_GROUP_RAMEX           0x0002

#define CARTRIDGE_GROUP_FREEZER         0x0004
#define CARTRIDGE_GROUP_GAME            0x0008
#define CARTRIDGE_GROUP_UTIL            0x0010

extern cartridge_info_t *cartridge_get_info_list(void);

#endif
