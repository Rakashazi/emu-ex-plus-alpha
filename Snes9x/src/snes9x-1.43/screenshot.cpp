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


#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif
#include <stdio.h>

#ifndef __WIN32__
#include <unistd.h>
#else
#include <direct.h>
#endif
#include <string.h>
#include <fcntl.h>

#ifdef HAVE_LIBPNG
#include <png.h>
#endif

#include "snes9x.h"
#include "memmap.h"
#include "display.h"
#include "gfx.h"
#include "ppu.h"
#include "screenshot.h"

bool8 S9xDoScreenshot(int width, int height){
#ifdef HAVE_LIBPNG
    FILE *fp;
    png_structp png_ptr;
    png_infop info_ptr;
    png_color_8 sig_bit;
    png_color pngpal[256];
    int imgwidth;
    int imgheight;
    const char *fname=S9xGetFilenameInc(".png");
    
    Settings.TakeScreenshot=FALSE;

    if((fp=fopen(fname, "wb"))==NULL){
        perror("Screenshot failed");
        return FALSE;
    }

    png_ptr=png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png_ptr){
        fclose(fp);
        unlink(fname);
        return FALSE;
    }
    info_ptr=png_create_info_struct(png_ptr);
    if(!info_ptr){
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        fclose(fp);
        unlink(fname);
        return FALSE;
    }

    if(setjmp(png_jmpbuf(png_ptr))){
        perror("Screenshot: setjmp");
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        unlink(fname);
        return FALSE;
    }

    imgwidth=width;
    imgheight=height;
    if(Settings.StretchScreenshots==1){
        if(width<=256 && height>SNES_HEIGHT_EXTENDED) imgwidth=width<<1;
        if(width>256 && height<=SNES_HEIGHT_EXTENDED) imgheight=height<<1;
    } else if(Settings.StretchScreenshots==2){
        if(width<=256) imgwidth=width<<1;
        if(height<=SNES_HEIGHT_EXTENDED) imgheight=height<<1;
    }
    
    png_init_io(png_ptr, fp);
    if(!Settings.SixteenBit){
        // BJ: credit sanmaiwashi for the idea to do palettized pngs, and to
        //     S9xSetPalette in x11.cpp for how to calculate the RGB values
        int b=IPPU.MaxBrightness*140;
        for(int i=0; i<256; i++){
            pngpal[i].red   = (PPU.CGDATA[i] & 0x1f)*b>>8;
            pngpal[i].green = ((PPU.CGDATA[i] >> 5) & 0x1f)*b>>8;
            pngpal[i].blue  = ((PPU.CGDATA[i] >> 10) & 0x1f)*b>>8;
        }
        png_set_PLTE(png_ptr, info_ptr, pngpal, 256);
    }
    png_set_IHDR(png_ptr, info_ptr, imgwidth, imgheight, 8, 
                 (Settings.SixteenBit?PNG_COLOR_TYPE_RGB:PNG_COLOR_TYPE_PALETTE),
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    if(Settings.SixteenBit){
        /* 5 bits per color */
        sig_bit.red=5;
        sig_bit.green=5;
        sig_bit.blue=5;
        png_set_sBIT(png_ptr, info_ptr, &sig_bit);
        png_set_shift(png_ptr, &sig_bit);
    }

    png_write_info(png_ptr, info_ptr);
    
    png_set_packing(png_ptr);

    png_byte *row_pointer=new png_byte [png_get_rowbytes(png_ptr, info_ptr)];
    uint8 *screen=GFX.Screen;
    for(int y=0; y<height; y++, screen+=GFX.Pitch){
        png_byte *rowpix = row_pointer;
        for(int x=0; x<width; x++){
            if(Settings.SixteenBit){
                uint32 r, g, b;
                DECOMPOSE_PIXEL((*(uint16 *)(screen+2*x)), r, g, b);
                *(rowpix++) = r;
                *(rowpix++) = g;
                *(rowpix++) = b;
                if(imgwidth!=width){
                    *(rowpix++) = r;
                    *(rowpix++) = g;
                    *(rowpix++) = b;
                }
            } else {
                *(rowpix++)=*(uint8 *)(screen+x);
                if(imgwidth!=width)
                    *(rowpix++)=*(uint8 *)(screen+x);
            }
        }
        png_write_row(png_ptr, row_pointer);
        if(imgheight!=height)
            png_write_row(png_ptr, row_pointer);
    }

    delete [] row_pointer;
        
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    fclose(fp);
    fprintf(stderr, "%s saved.\n", fname);
    return TRUE;
#else
    perror("Screenshot support not available (libpng was not found at build time)");
	return FALSE;
#endif
}

