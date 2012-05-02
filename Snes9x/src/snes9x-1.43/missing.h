/*******************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 
  (c) Copyright 1996 - 2002 Gary Henderson (gary.henderson@ntlworld.com) and
                            Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2001 - 2004 John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2004 Brad Jorsch (anomie@users.sourceforge.net),
                            funkyass (funkyass@spam.shaw.ca),
                            Joel Yliluoma (http://iki.fi/bisqwit/)
                            Kris Bleakley (codeviolation@hotmail.com),
                            Matthew Kendora,
                            Nach (n-a-c-h@users.sourceforge.net),
                            Peter Bortas (peter@bortas.org) and
                            zones (kasumitokoduck@yahoo.com)

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003 zsKnight (zsknight@zsnes.com),
                            _Demo_ (_demo_@zsnes.com), and Nach

  C4 C++ code
  (c) Copyright 2003 Brad Jorsch

  DSP-1 emulator code
  (c) Copyright 1998 - 2004 Ivar (ivar@snes9x.com), _Demo_, Gary Henderson,
                            John Weidman, neviksti (neviksti@hotmail.com),
                            Kris Bleakley, Andreas Naive

  DSP-2 emulator code
  (c) Copyright 2003 Kris Bleakley, John Weidman, neviksti, Matthew Kendora, and
                     Lord Nightmare (lord_nightmare@users.sourceforge.net

  OBC1 emulator code
  (c) Copyright 2001 - 2004 zsKnight, pagefault (pagefault@zsnes.com) and
                            Kris Bleakley
  Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code
  (c) Copyright 2002 Matthew Kendora with research by
                     zsKnight, John Weidman, and Dark Force

  S-DD1 C emulator code
  (c) Copyright 2003 Brad Jorsch with research by
                     Andreas Naive and John Weidman
 
  S-RTC C emulator code
  (c) Copyright 2001 John Weidman
  
  ST010 C++ emulator code
  (c) Copyright 2003 Feather, Kris Bleakley, John Weidman and Matthew Kendora

  Super FX x86 assembler emulator code 
  (c) Copyright 1998 - 2003 zsKnight, _Demo_, and pagefault 

  Super FX C emulator code 
  (c) Copyright 1997 - 1999 Ivar, Gary Henderson and John Weidman


  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004 Marcus Comstedt (marcus@mc.pp.se) 

 
  Specific ports contains the works of other authors. See headers in
  individual files.
 
  Snes9x homepage: http://www.snes9x.com
 
  Permission to use, copy, modify and distribute Snes9x in both binary and
  source form, for non-commercial purposes, is hereby granted without fee,
  providing that this license information and copyright notice appear with
  all copies and any derived work.
 
  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software.
 
  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes
  charging money for Snes9x or software derived from Snes9x.
 
  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.
 
  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
*******************************************************************************/

#ifndef _MISSING_H_
#define _MISSING_H_

struct HDMA
{
    uint8 used;
    uint8 bbus_address;
    uint8 abus_bank;
    uint16 abus_address;
    uint8 indirect_address;
    uint8 force_table_address_write;
    uint8 force_table_address_read;
    uint8 line_count_write;
    uint8 line_count_read;
};

struct Missing
{
    uint8 emulate6502;
    uint8 decimal_mode;
    uint8 mv_8bit_index;
    uint8 mv_8bit_acc;
    uint8 interlace;
    uint8 lines_239;
    uint8 pseudo_512;
    struct HDMA hdma [8];
    uint8 modes [8];
    uint8 mode7_fx;
    uint8 mode7_flip;
    uint8 mode7_bgmode;
    uint8 direct;
    uint8 matrix_multiply;
    uint8 oam_read;
    uint8 vram_read;
    uint8 cgram_read;
    uint8 wram_read;
    uint8 dma_read;
    uint8 vram_inc;
    uint8 vram_full_graphic_inc;
    uint8 virq;
    uint8 hirq;
    uint16 virq_pos;
    uint16 hirq_pos;
    uint8 h_v_latch;
    uint8 h_counter_read;
    uint8 v_counter_read;
    uint8 fast_rom;
    uint8 window1 [6];
    uint8 window2 [6];
    uint8 sprite_priority_rotation;
    uint8 subscreen;
    uint8 subscreen_add;
    uint8 subscreen_sub;
    uint8 fixed_colour_add;
    uint8 fixed_colour_sub;
    uint8 mosaic;
    uint8 sprite_double_height;
    uint8 dma_channels;
    uint8 dma_this_frame;
    uint8 oam_address_read;
    uint8 bg_offset_read;
    uint8 matrix_read;
    uint8 hdma_channels;
    uint8 hdma_this_frame;
    uint16 unknownppu_read;
    uint16 unknownppu_write;
    uint16 unknowncpu_read;
    uint16 unknowncpu_write;
    uint16 unknowndsp_read;
    uint16 unknowndsp_write;
};

EXTERN_C struct Missing missing;
#endif

