/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/VideoChips/Common.h,v $
**
** $Revision: 1.57 $
**
** $Date: 2009-04-08 02:52:32 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/

#include <Config/VideoChips.h>
#ifndef BORDER_WIDTH
#define BORDER_WIDTH 8
#endif
#define DISPLAY_WIDTH  256
#define SCREEN_WIDTH   (2 * BORDER_WIDTH + DISPLAY_WIDTH)
#define SCREEN_HEIGHT  240


#define UPDATE_TABLE_4() if ((++scroll & 0x1f) == 0) charTable += jump[page ^= 1];
#define UPDATE_TABLE_5() if ((++scroll & 0x7f) == 0) charTable += jump[page ^= 1];
#define UPDATE_TABLE_6() if ((++scroll & 0xff) == 0) charTable += jump[page ^= 1];
#define UPDATE_TABLE_7() if ((++scroll & 0xff) == 0) charTable += jump[page ^= 1];
#define UPDATE_TABLE_8() if ((++scroll & 0xff) == 0) charTable += jump[page ^= 1];
#define UPDATE_TABLE_10() if ((++scroll & 0xff) == 0) charTable += jump[page ^= 1];
#define UPDATE_TABLE_12() if ((++scroll & 0xff) == 0) charTable += jump[page ^= 1];


#ifdef MAX_VIDEO_WIDTH_320

static UInt32 ca;
static UInt32 cb;
static UInt32 cr;

#define CM1 ((COLMASK_R << COLSHIFT_R) | (COLMASK_B << COLSHIFT_B))
#define CM2 (COLMASK_G << COLSHIFT_G)

#define MIX_COLOR(a, b) ( ca = a, cb = b,   \
    (Pixel)(((((ca & CM1) + (cb & CM1)) / 2) & CM1) | ((((ca & CM2) + (cb & CM2)) / 2) & CM2)))

#endif


static int jumpTable[] =  { -128, -128, -0x8080, 0x7f80 };
static int jumpTable4[] = {  -32, -32,  -0x8020, 0x7fe0 };

static Pixel* linePtr0 = NULL;
static Pixel* linePtr0p = NULL;
static Pixel* linePtr0m = NULL;
static Pixel* linePtr0w = NULL;
static Pixel* linePtr1 = NULL;
static Pixel* linePtr2 = NULL;
static Pixel* linePtr3 = NULL;
static Pixel* linePtr4 = NULL;
static Pixel* linePtr5 = NULL;
static Pixel* linePtr6 = NULL;
static Pixel* linePtr7 = NULL;
static Pixel* linePtr8 = NULL;
static Pixel* linePtr10 = NULL;
static Pixel* linePtr12 = NULL;
static Pixel* linePtrBlank = NULL;


void RefreshLineReset()
{
    linePtr0 = NULL;
    linePtr0p = NULL;
    linePtr0m = NULL;
    linePtr0w = NULL;
    linePtr1 = NULL;
    linePtr2 = NULL;
    linePtr3 = NULL;
    linePtr4 = NULL;
    linePtr5 = NULL;
    linePtr6 = NULL;
    linePtr7 = NULL;
    linePtr8 = NULL;
    linePtr10 = NULL;
    linePtr12 = NULL;
    linePtrBlank = NULL;
}

Pixel *RefreshBorder(VDP* vdp, int Y, Pixel bgColor, int line512, int borderExtra)
{
    FrameBuffer* frameBuffer = frameBufferGetDrawFrame();
    int lineSize = line512 ? 2 : 1;
    Pixel *linePtr;
    int offset;

    if (frameBuffer == NULL) {
        return NULL;
    }

    Y -= vdp->displayOffest;

    frameBufferSetScanline(Y);

    linePtr = frameBufferGetLine(frameBuffer, Y);
    //if(!linePtr) return 0;

    if (frameBufferGetDoubleWidth(frameBuffer, Y) && !line512) {
        for(offset = 256 + 16; offset < 512 + 16; offset++) {
            linePtr[offset] = 0;
        }
    }

    frameBufferSetDoubleWidth(frameBuffer, Y, line512);

    for (offset = lineSize * (BORDER_WIDTH + vdp->HAdjust + borderExtra) - 1; offset >= 0; offset--) {
        *linePtr++ = bgColor;
    }

    return linePtr;
}

Pixel *RefreshBorder6(VDP* vdp, int Y, Pixel bgColor1, Pixel bgColor2, int line512, int borderExtra)
{
    FrameBuffer* frameBuffer = frameBufferGetDrawFrame();
    int lineSize = line512 ? 2 : 1;
    Pixel *linePtr;
    int offset;

    if (frameBuffer == NULL) {
        return NULL;
    }

    Y -= vdp->displayOffest;

    frameBufferSetScanline(Y);

    linePtr = frameBufferGetLine(frameBuffer, Y);
    //if(!linePtr) return 0;

    if (frameBufferGetDoubleWidth(frameBuffer, Y) && !line512) {
        for(offset = 256 + 16; offset < 512 + 16; offset++) {
            linePtr[offset] = 0;
        }
    }

    frameBufferSetDoubleWidth(frameBuffer, Y, line512);

    for (offset = lineSize * (BORDER_WIDTH + vdp->HAdjust + borderExtra) - 1; offset >= 0; offset -= 2) {
        *linePtr++ = bgColor1;
        *linePtr++ = bgColor2;
    }

    return linePtr;
}

static void RefreshRightBorder(VDP* vdp, int Y, Pixel bgColor, int line512, int borderExtra) {
    FrameBuffer* frameBuffer = frameBufferGetDrawFrame();
    int lineSize = line512 ? 2 : 1;
    Pixel *linePtr;
    int offset;

    if (frameBuffer == NULL) {
        return;
    }

    Y -= vdp->displayOffest;

    if (!displayEnable) {
        return;
    }
    
    linePtr = frameBufferGetLine(frameBuffer, Y);

    for(offset = lineSize * (BORDER_WIDTH - vdp->HAdjust + borderExtra); offset > 0; offset--) {
        linePtr[lineSize * SCREEN_WIDTH - offset] = bgColor;
    }
}

static void RefreshRightBorder6(VDP* vdp, int Y, Pixel bgColor1, Pixel bgColor2) {
    FrameBuffer* frameBuffer = frameBufferGetDrawFrame();
    Pixel *linePtr;
    int offset;

    if (frameBuffer == NULL) {
        return;
    }

    Y -= vdp->displayOffest;

    if (!displayEnable) {
        return;
    }
    
    linePtr = frameBufferGetLine(frameBuffer, Y);

    for(offset = 2 * (BORDER_WIDTH - vdp->HAdjust); offset > 0; offset-= 2) {
        linePtr[2 * SCREEN_WIDTH - offset] = bgColor1;
        linePtr[2 * SCREEN_WIDTH - offset + 1] = bgColor2;
    }
}

static void RefreshLineBlank(VDP* vdp, int Y, int X, int X2)
{
    Pixel bgColor = vdp->palette[0];
    int rightBorder;

    if (X == -1) {
        X++;
        linePtrBlank = RefreshBorder(vdp, Y, bgColor, 0, 0);
    }

    if (linePtrBlank == NULL) {
        return;
    }
    
    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    while (X < X2) {
        linePtrBlank[0] = bgColor;
        linePtrBlank[1] = bgColor;
        linePtrBlank[2] = bgColor;
        linePtrBlank[3] = bgColor;
        linePtrBlank[4] = bgColor;
        linePtrBlank[5] = bgColor;
        linePtrBlank[6] = bgColor;
        linePtrBlank[7] = bgColor;
        linePtrBlank += 8; 
        X++;
    }

    if (rightBorder) {
        RefreshRightBorder(vdp, Y, bgColor, 0, 0);
    }
}

static void RefreshLine0(VDP* vdp, int Y, int X, int X2)
{
    static int     patternBase;
    static int     pattern;
    static int     x;
    static int     y;
    static int     shift;

    static int     hScroll;

    int    rightBorder;

    if (X == -1) {
        int i;

        X++;
        linePtr0 = RefreshBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, vdp->hAdjustSc0);
        //if(!linePtr0) return;

        hScroll    = vdpHScroll(vdp) % 6;

        y = Y - vdp->firstLine + vdpVScroll(vdp) - vdp->scr0splitLine;
        x = 0;
        patternBase = vdp->chrGenBase & ((-1 << 11) | (y & 7));
        shift = 0;

        for (i = 0; i < hScroll; i++) {
            *linePtr0++ = vdp->palette[vdp->BGColor];
        }
    }

    if (linePtr0 == NULL) {
        return;
    }

    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor = vdp->palette[vdp->BGColor];
        while (X < X2) {
            linePtr0[0] = bgColor;
            linePtr0[1] = bgColor;
            linePtr0[2] = bgColor;
            linePtr0[3] = bgColor;
            linePtr0[4] = bgColor;
            linePtr0[5] = bgColor;
            linePtr0[6] = bgColor;
            linePtr0[7] = bgColor;
            linePtr0 += 8; 
            X++;
        }
    }
    else {
        Pixel color[2];
        
        color[0] = vdp->palette[vdp->BGColor];
        color[1] = vdp->palette[vdp->FGColor];

        while (X < X2) {
            if (X == 0 || X == 31) {
                if (X == 31) linePtr0 -= hScroll;
                linePtr0[0] = color[0];
                linePtr0[1] = color[0];
                linePtr0[2] = color[0];
                linePtr0[3] = color[0];
                linePtr0[4] = color[0];
                linePtr0[5] = color[0];
                linePtr0[6] = color[0];
                linePtr0[7] = color[0];
                linePtr0 += 8; 
                X++;
            }
            else {
                int j;
                for (j = 0; j < 4; j++) {
                    if (shift <= 2) { 
                        int charIdx = 0xc00 + 40 * (y / 8) + x++;
                        UInt8* charTable = vdp->vram + (vdp->chrTabBase & ((-1 << 12) | charIdx));
                        pattern = vdp->vram[patternBase | ((int)*charTable * 8)];
                        shift = 8; 
                    }

                    linePtr0[0] = color[(pattern >> --shift) & 1];
                    linePtr0[1] = color[(pattern >> --shift) & 1];
                    linePtr0 += 2; 
                }
                X++;
            }
        }
    }
    if (rightBorder) {
        RefreshRightBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, -vdp->hAdjustSc0);
    }
}


static void RefreshLine0Plus(VDP* vdp, int Y, int X, int X2)
{
    static int     patternBase;
    static int     pattern;
    static int     x;
    static int     y;
    static int     shift;

    static int     hScroll;

    int    rightBorder;

    if (X == -1) {
        int i;

        X++;
        linePtr0p = RefreshBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, vdp->hAdjustSc0);
        //if(!linePtr0p) return;

        hScroll    = vdpHScroll(vdp) % 6;

        y = Y - vdp->firstLine + vdpVScroll(vdp) - vdp->scr0splitLine;
        x = 0;
        patternBase =  (-1 << 13) | ((y & 0xc0) << 5) | (y & 7);
        shift = 0;

        for (i = 0; i < hScroll; i++) {
            *linePtr0p++ = vdp->palette[vdp->BGColor];
        }
    }

    if (linePtr0p == NULL) {
        return;
    }

    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor = vdp->palette[vdp->BGColor];
        while (X < X2) {
            linePtr0p[0] = bgColor;
            linePtr0p[1] = bgColor;
            linePtr0p[2] = bgColor;
            linePtr0p[3] = bgColor;
            linePtr0p[4] = bgColor;
            linePtr0p[5] = bgColor;
            linePtr0p[6] = bgColor;
            linePtr0p[7] = bgColor;
            linePtr0p += 8; 
            X++;
        }
    }
    else {
        Pixel color[2];
        
        color[0] = vdp->palette[vdp->BGColor];
        color[1] = vdp->palette[vdp->FGColor];

        while (X < X2) {
            if (X == 0 || X == 31) {
                if (X == 31) linePtr0p -= hScroll;
                linePtr0p[0] = color[0];
                linePtr0p[1] = color[0];
                linePtr0p[2] = color[0];
                linePtr0p[3] = color[0];
                linePtr0p[4] = color[0];
                linePtr0p[5] = color[0];
                linePtr0p[6] = color[0];
                linePtr0p[7] = color[0];
                linePtr0p += 8; 
                X++;
            }
            else {
                int j;
                for (j = 0; j < 4; j++) {
                    if (shift <= 2) { 
                        int charIdx = 0xc00 + 40 * (y / 8) + x++;
                        UInt8* charTable = vdp->vram + (vdp->chrTabBase & ((-1 << 12) | charIdx));
                        pattern = vdp->vram[vdp->chrGenBase & (patternBase | ((int)*charTable * 8))];
                        shift = 8; 
                    }

                    linePtr0p[0] = color[(pattern >> --shift) & 1];
                    linePtr0p[1] = color[(pattern >> --shift) & 1];
                    linePtr0p += 2; 
                }
                X++;
            }
        }
    }
    if (rightBorder) {
        RefreshRightBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, -vdp->hAdjustSc0);
    }
}

static void RefreshLine0Mix(VDP* vdp, int Y, int X, int X2)
{
    static int     patternBase;
    static int     pattern;
    static int     x;
    static int     y;
    static int     shift;

    static int     hScroll;

    int    rightBorder;

    if (X == -1) {
        int i;

        X++;
        linePtr0m = RefreshBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, vdp->hAdjustSc0);
        //if(!linePtr0m) return;

        hScroll    = vdpHScroll(vdp) % 6;

        y = Y - vdp->firstLine + vdpVScroll(vdp) - vdp->scr0splitLine;
        x = 0;
        patternBase = vdp->chrGenBase & ((-1 << 11) | (y & 7));
        shift = 0;

        for (i = 0; i < hScroll; i++) {
            *linePtr0m++ = vdp->palette[vdp->BGColor];
        }
    }

    if (linePtr0m == NULL) {
        return;
    }

    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor = vdp->palette[vdp->BGColor];
        while (X < X2) {
            linePtr0m[0] = bgColor;
            linePtr0m[1] = bgColor;
            linePtr0m[2] = bgColor;
            linePtr0m[3] = bgColor;
            linePtr0m[4] = bgColor;
            linePtr0m[5] = bgColor;
            linePtr0m[6] = bgColor;
            linePtr0m[7] = bgColor;
            linePtr0m += 8; 
            X++;
        }
    }
    else {
        Pixel bgColor = vdp->palette[vdp->BGColor];
        Pixel fgColor = vdp->palette[vdp->FGColor];

        while (X < X2) {
            if (X == 0 || X == 31) {
                if (X == 31) linePtr0m -= hScroll;
                linePtr0m[0] = bgColor;
                linePtr0m[1] = bgColor;
                linePtr0m[2] = bgColor;
                linePtr0m[3] = bgColor;
                linePtr0m[4] = bgColor;
                linePtr0m[5] = bgColor;
                linePtr0m[6] = bgColor;
                linePtr0m[7] = bgColor;
                linePtr0m += 8; 
                X++;
            }
            else {
                int j;
                for (j = 0; j < 4; j++) {
                    if (++shift >= 3) {
                        linePtr0m[0] = bgColor;
                        linePtr0m[1] = bgColor;
                        shift = 0;
                    }
                    else {
                        linePtr0m[0] = fgColor;
                        linePtr0m[1] = fgColor;
                    }
                    linePtr0m += 2; 
                }
                X++;
            }
        }
    }
    if (rightBorder) {
        RefreshRightBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, -vdp->hAdjustSc0);
    }
}

#ifdef MAX_VIDEO_WIDTH_320

static void RefreshLineTx80(VDP* vdp, int Y, int X, int X2)
{

    static int     patternBase;
    static UInt8   colPattern;
    static int     pattern;
    static int     x;
    static int     y;
    static int     shift;
    static Pixel color[2];
    int    rightBorder;

    if (X == -1) {
        X++;
        linePtr0w = RefreshBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, 0);
        //if(!linePtr0w) return;
        y = Y - vdp->firstLine + vdpVScroll(vdp) - vdp->scr0splitLine;
        x = 0;
        patternBase = vdp->chrGenBase & ((-1 << 11) | (y & 7));
        shift = 0;
    }

    if (linePtr0w == NULL) {
        return;
    }

    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor = vdp->palette[vdp->BGColor];
        while (X < X2) {
            int j;
            for (j = 0; j < 1; j++) {
                linePtr0w[0] = bgColor;
                linePtr0w[1] = bgColor;
                linePtr0w[2] = bgColor;
                linePtr0w[3] = bgColor;
                linePtr0w[4] = bgColor;
                linePtr0w[5] = bgColor;
                linePtr0w[6] = bgColor;
                linePtr0w[7] = bgColor;
                linePtr0w += 8; 
            }
            X++;
        }
    }
    else {
        while (X < X2) {
            if (X == 0 || X == 31) {
                Pixel bgColor = vdp->palette[vdp->BGColor];
                int j;
                for (j = 0; j < 1; j++) {
                    linePtr0w[0] = bgColor;
                    linePtr0w[1] = bgColor;
                    linePtr0w[2] = bgColor;
                    linePtr0w[3] = bgColor;
                    linePtr0w[4] = bgColor;
                    linePtr0w[5] = bgColor;
                    linePtr0w[6] = bgColor;
                    linePtr0w[7] = bgColor;
                    linePtr0w += 8; 
                }
                X++;
            }
            else {
                int j;
                for (j = 0; j < 8; j++) {
                    if (shift <= 2) { 
                        int charIdx = 80 * (y / 8) + x;
                        UInt8*  charTable   = vdp->vram + (vdp->chrTabBase & ((-1 << 12) | charIdx));
                        pattern = vdp->vram[patternBase | ((int)*charTable * 8)];
                        shift = 8; 

                        if ((x & 0x07) == 0) {
                            colPattern = vdp->vram[vdp->colTabBase & ((-1 << 9) | (charIdx / 8))];
                        }
                        if (colPattern & 0x80) {
                            color[0] = vdp->palette[vdp->XBGColor];
                            color[1] = vdp->palette[vdp->XFGColor];
                        }
                        else {
                            color[0] = vdp->palette[vdp->BGColor];
                            color[1] = vdp->palette[vdp->FGColor];
                        }
                        colPattern <<= 1;

                        x++;
                    }

                    linePtr0w[0] = MIX_COLOR(color[(pattern >> --shift) & 1], color[(pattern >> --shift) & 1]);
                    linePtr0w += 1; 
                }
                X++;
            }
        }
    }
    if (rightBorder) {
        RefreshRightBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, 0);
    }
}

#else

static void RefreshLineTx80(VDP* vdp, int Y, int X, int X2)
{
    static UInt8*  sprLine;
    static int     patternBase;
    static UInt8   colPattern;
    static int     pattern;
    static int     x;
    static int     y;
    static int     shift;
    static Pixel color[2];
    int    rightBorder;

    static int     hScroll;

    if (X == -1) {
        int i;

        X++;
        linePtr0w = RefreshBorder(vdp, Y, vdp->palette[vdp->BGColor], 1, vdp->hAdjustSc0);
        //if(!linePtr0w) return;
        y = Y - vdp->firstLine + vdpVScroll(vdp) - vdp->scr0splitLine;
        x = 0;
        patternBase = vdp->chrGenBase & ((-1 << 11) | (y & 7));
        shift = 0;

        hScroll    = vdpHScroll(vdp) % 6;

        for (i = 0; i < hScroll; i++) {
            *linePtr0w++ = vdp->palette[vdp->BGColor];
        }
    }

    if (linePtr0w == NULL) {
        return;
    }

    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor = vdp->palette[vdp->BGColor];
        while (X < X2) {
            int j;
            for (j = 0; j < 2; j++) {
                linePtr0w[0] = bgColor;
                linePtr0w[1] = bgColor;
                linePtr0w[2] = bgColor;
                linePtr0w[3] = bgColor;
                linePtr0w[4] = bgColor;
                linePtr0w[5] = bgColor;
                linePtr0w[6] = bgColor;
                linePtr0w[7] = bgColor;
                linePtr0w += 8; 
            }
            X++;
        }
    }
    else {
        while (X < X2) {
            if (X == 0 || X == 31) {
                Pixel bgColor = vdp->palette[vdp->BGColor];
                int j;

                if (X == 31) linePtr0w -= hScroll;

                for (j = 0; j < 2; j++) {
                    linePtr0w[0] = bgColor;
                    linePtr0w[1] = bgColor;
                    linePtr0w[2] = bgColor;
                    linePtr0w[3] = bgColor;
                    linePtr0w[4] = bgColor;
                    linePtr0w[5] = bgColor;
                    linePtr0w[6] = bgColor;
                    linePtr0w[7] = bgColor;
                    linePtr0w += 8; 
                }
                X++;
            }
            else {
                int j;
                for (j = 0; j < 8; j++) {
                    if (shift <= 2) { 
                        int charIdx = 80 * (y / 8) + x;
                        UInt8*  charTable   = vdp->vram + (vdp->chrTabBase & ((-1 << 12) | charIdx));
                        pattern = vdp->vram[patternBase | ((int)*charTable * 8)];
                        shift = 8; 

                        if ((x & 0x07) == 0) {
                            colPattern = vdp->vram[vdp->colTabBase & ((-1 << 9) | (charIdx / 8))];
                        }
                        if (colPattern & 0x80) {
                            color[0] = vdp->palette[vdp->XBGColor];
                            color[1] = vdp->palette[vdp->XFGColor];
                        }
                        else {
                            color[0] = vdp->palette[vdp->BGColor];
                            color[1] = vdp->palette[vdp->FGColor];
                        }
                        colPattern <<= 1;

                        x++;
                    }

                    linePtr0w[0] = color[(pattern >> --shift) & 1];
                    linePtr0w[1] = color[(pattern >> --shift) & 1];
                    linePtr0w += 2; 
                }
                X++;
            }
        }
    }
    if (rightBorder) {
        RefreshRightBorder(vdp, Y, vdp->palette[vdp->BGColor], 1, -vdp->hAdjustSc0);
    }
}

#endif


static void RefreshLine1(VDP* vdp, int Y, int X, int X2)
{
    static UInt8*  sprLine;
    static UInt8*  charTable;
    static int     patternBase;
    static int     hScroll;
    static int     hScroll512;
    static int*    jump;
    static int     page;
    static int     scroll;
    UInt8  charPattern;
    UInt8  colPattern;
    UInt8  col;
    Pixel color[2];
    int    y;
    int    rightBorder;

    if (X == -1) {
        X++;
        linePtr1 = RefreshBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, 0);
        //if(!linePtr1) return;
        sprLine = getSpritesLine(vdp, Y);

        hScroll    = vdpHScroll(vdp) & 7;
        hScroll512 = 0;//vdpHScroll512(vdp);
        jump       = jumpTable4 + hScroll512 * 2;
        page       = (vdp->chrTabBase / 0x8000) & 1;
        scroll     = hScroll >> 3;

        y = Y - vdp->firstLine + vdpVScroll(vdp);
        charTable   = vdp->vram + (vdp->chrTabBase & ((-1 << 10) | (32 * (y / 8)))) + scroll;
        patternBase = vdp->chrGenBase & ((-1 << 11) | (y & 7));

        if (hScroll512) {
            if (scroll & 0x20) charTable += jump[page ^= 1];
            if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 32;
        }

        if (vdpIsEdgeMasked(vdp->vdpRegs)) {
            Pixel bgColor = vdp->palette[vdp->BGColor];
            linePtr1[0] = bgColor;
            linePtr1[1] = bgColor;
            linePtr1[2] = bgColor;
            linePtr1[3] = bgColor;
            linePtr1[4] = bgColor;
            linePtr1[5] = bgColor;
            linePtr1[6] = bgColor;
            linePtr1[7] = bgColor;
            X++;
            sprLine += sprLine != NULL ? 8 : 0;
            linePtr1 += 8;
        }

        if (!vdp->screenOn || !vdp->drawArea) {
            Pixel bgColor = vdp->palette[vdp->BGColor];

            switch (hScroll & 7) {
            case 1: *linePtr1++ = bgColor; 
            case 2: *linePtr1++ = bgColor; 
            case 3: *linePtr1++ = bgColor; 
            case 4: *linePtr1++ = bgColor; 
            case 5: *linePtr1++ = bgColor; 
            case 6: *linePtr1++ = bgColor; 
            case 7: *linePtr1++ = bgColor;  charTable++; UPDATE_TABLE_4();
            }
        }
        else {
            if (vdpIsEdgeMasked(vdp->vdpRegs)) {
                colPattern = vdp->vram[vdp->colTabBase & ((*charTable / 8) | (-1 << 6))];
                color[0] = vdp->palette[colPattern & 0x0f];
                color[1] = vdp->palette[colPattern >> 4];
                charPattern = vdp->vram[patternBase | ((int)*charTable * 8)];

                switch (hScroll & 7) {
                case 1: col = *sprLine++; *linePtr1++ = col ? vdp->palette[col] : color[(charPattern >> 6) & 1]; 
                case 2: col = *sprLine++; *linePtr1++ = col ? vdp->palette[col] : color[(charPattern >> 5) & 1]; 
                case 3: col = *sprLine++; *linePtr1++ = col ? vdp->palette[col] : color[(charPattern >> 4) & 1]; 
                case 4: col = *sprLine++; *linePtr1++ = col ? vdp->palette[col] : color[(charPattern >> 3) & 1]; 
                case 5: col = *sprLine++; *linePtr1++ = col ? vdp->palette[col] : color[(charPattern >> 2) & 1]; 
                case 6: col = *sprLine++; *linePtr1++ = col ? vdp->palette[col] : color[(charPattern >> 1) & 1]; 
                case 7: col = *sprLine++; *linePtr1++ = col ? vdp->palette[col] : color[(charPattern >> 0) & 1]; charTable++; UPDATE_TABLE_4();
                }
            }
            else {
                Pixel bgColor = vdp->palette[vdp->BGColor];

                switch (hScroll & 7) {
                case 1: col = *sprLine++; *linePtr1++ = bgColor; 
                case 2: col = *sprLine++; *linePtr1++ = bgColor; 
                case 3: col = *sprLine++; *linePtr1++ = bgColor; 
                case 4: col = *sprLine++; *linePtr1++ = bgColor; 
                case 5: col = *sprLine++; *linePtr1++ = bgColor; 
                case 6: col = *sprLine++; *linePtr1++ = bgColor; 
                case 7: col = *sprLine++; *linePtr1++ = bgColor; 
                }
            }
        }
    }

    if (linePtr1 == NULL) {
        return;
    }

    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor = vdp->palette[vdp->BGColor];
        while (X < X2) {
            linePtr1[0] = bgColor;
            linePtr1[1] = bgColor;
            linePtr1[2] = bgColor;
            linePtr1[3] = bgColor;
            linePtr1[4] = bgColor;
            linePtr1[5] = bgColor;
            linePtr1[6] = bgColor;
            linePtr1[7] = bgColor;
            linePtr1 += 8; 
            X++;
        }
    }
    else {
        while (X < X2) {
            colPattern = vdp->vram[vdp->colTabBase & ((*charTable / 8) | (-1 << 6))];
            color[0] = vdp->palette[colPattern & 0x0f];
            color[1] = vdp->palette[colPattern >> 4];
            charPattern = vdp->vram[patternBase | ((int)*charTable * 8)];

            col = sprLine[0]; linePtr1[0] = col ? vdp->palette[col] : color[(charPattern >> 7) & 1]; 
            col = sprLine[1]; linePtr1[1] = col ? vdp->palette[col] : color[(charPattern >> 6) & 1];
            col = sprLine[2]; linePtr1[2] = col ? vdp->palette[col] : color[(charPattern >> 5) & 1]; 
            col = sprLine[3]; linePtr1[3] = col ? vdp->palette[col] : color[(charPattern >> 4) & 1];
            col = sprLine[4]; linePtr1[4] = col ? vdp->palette[col] : color[(charPattern >> 3) & 1]; 
            col = sprLine[5]; linePtr1[5] = col ? vdp->palette[col] : color[(charPattern >> 2) & 1];
            col = sprLine[6]; linePtr1[6] = col ? vdp->palette[col] : color[(charPattern >> 1) & 1]; 
            col = sprLine[7]; linePtr1[7] = col ? vdp->palette[col] : color[(charPattern >> 0) & 1];
            sprLine += 8;
            charTable++; 
            linePtr1 += 8; 
            X++;
        }
    }
    if (rightBorder) {
        spritesLine(vdp, Y);
        RefreshRightBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, 0);
    }
}


static void RefreshLine2(VDP* vdp, int Y, int X, int X2)
{
    static UInt8*  sprLine;
    static UInt8*  charTable;
    static int     base;
    static int     hScroll;
    static int     hScroll512;
    static int*    jump;
    static int     page;
    static int     scroll;
    UInt8  charPattern;
    UInt8  colPattern;
    UInt8  col;
    Pixel color[2];
    int    index;
    int    rightBorder;

    if (X == -1) {
        int y;

        X++;
        linePtr2 = RefreshBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, 0);
        //if(!linePtr2) return;
        sprLine = getSpritesLine(vdp, Y);

        if (linePtr2 == NULL) {
            return;
        }

        hScroll    = vdpHScroll(vdp);
        hScroll512 = 0;//vdpHScroll512(vdp);
        jump       = jumpTable4 + hScroll512 * 2;
        page       = (vdp->chrTabBase / 0x8000) & 1;
        scroll     = hScroll >> 3;

        y = Y - vdp->firstLine + vdpVScroll(vdp);

        charTable   = vdp->vram + (vdp->chrTabBase & ((-1 << 10) | (32 * (y / 8)))) + scroll;
        base        = (-1 << 13) | ((y & 0xc0) << 5) | (y & 7);

        if (hScroll512) {
            if (scroll & 0x20) charTable += jump[page ^= 1];
            if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 32;
        }

        if (vdpIsEdgeMasked(vdp->vdpRegs)) {
            Pixel bgColor = vdp->palette[vdp->BGColor];
            linePtr2[0] = bgColor;
            linePtr2[1] = bgColor;
            linePtr2[2] = bgColor;
            linePtr2[3] = bgColor;
            linePtr2[4] = bgColor;
            linePtr2[5] = bgColor;
            linePtr2[6] = bgColor;
            linePtr2[7] = bgColor;
            charTable++; 
            UPDATE_TABLE_4(); 
            X++;
            sprLine += sprLine != NULL ? 8 : 0;
            linePtr2 += 8;
        }

        index       = base | ((int)*charTable * 8);
        colPattern = vdp->vram[vdp->colTabBase & index];
        color[0]   = vdp->palette[colPattern & 0x0f];
        color[1]   = vdp->palette[colPattern >> 4];
        charPattern = vdp->vram[vdp->chrGenBase & index];

        if (!vdp->screenOn || !vdp->drawArea) {
            Pixel bgColor = vdp->palette[vdp->BGColor];

            switch (hScroll & 7) {
            case 1: *linePtr2++ = bgColor; 
            case 2: *linePtr2++ = bgColor; 
            case 3: *linePtr2++ = bgColor; 
            case 4: *linePtr2++ = bgColor; 
            case 5: *linePtr2++ = bgColor; 
            case 6: *linePtr2++ = bgColor; 
            case 7: *linePtr2++ = bgColor;  charTable++; UPDATE_TABLE_4();
            }
        }
        else {
            if (vdpIsEdgeMasked(vdp->vdpRegs)) {
                switch (hScroll & 7) {
                case 1: col = *sprLine++; *linePtr2++ = col ? vdp->palette[col] : color[(charPattern >> 6) & 1]; 
                case 2: col = *sprLine++; *linePtr2++ = col ? vdp->palette[col] : color[(charPattern >> 5) & 1]; 
                case 3: col = *sprLine++; *linePtr2++ = col ? vdp->palette[col] : color[(charPattern >> 4) & 1]; 
                case 4: col = *sprLine++; *linePtr2++ = col ? vdp->palette[col] : color[(charPattern >> 3) & 1]; 
                case 5: col = *sprLine++; *linePtr2++ = col ? vdp->palette[col] : color[(charPattern >> 2) & 1]; 
                case 6: col = *sprLine++; *linePtr2++ = col ? vdp->palette[col] : color[(charPattern >> 1) & 1]; 
                case 7: col = *sprLine++; *linePtr2++ = col ? vdp->palette[col] : color[(charPattern >> 0) & 1]; charTable++; UPDATE_TABLE_4();
                }
            }
            else {
                Pixel bgColor = vdp->palette[vdp->BGColor];

                switch (hScroll & 7) {
                case 1: col = *sprLine++; *linePtr2++ = bgColor; 
                case 2: col = *sprLine++; *linePtr2++ = bgColor; 
                case 3: col = *sprLine++; *linePtr2++ = bgColor; 
                case 4: col = *sprLine++; *linePtr2++ = bgColor; 
                case 5: col = *sprLine++; *linePtr2++ = bgColor; 
                case 6: col = *sprLine++; *linePtr2++ = bgColor; 
                case 7: col = *sprLine++; *linePtr2++ = bgColor; charTable++; UPDATE_TABLE_4();
                }
            }
        }
    }

    if (linePtr2 == NULL) {
        return;
    }

    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor = vdp->palette[vdp->BGColor];
        while (X < X2) {
            linePtr2[0] = bgColor;
            linePtr2[1] = bgColor;
            linePtr2[2] = bgColor;
            linePtr2[3] = bgColor;
            linePtr2[4] = bgColor;
            linePtr2[5] = bgColor;
            linePtr2[6] = bgColor;
            linePtr2[7] = bgColor;
            linePtr2 += 8; 
            X++;
        }
    }
    else {
        while (X < X2) {
            index       = base | ((int)*charTable * 8);
            colPattern = vdp->vram[vdp->colTabBase & index];
            color[0]   = vdp->palette[colPattern & 0x0f];
            color[1]   = vdp->palette[colPattern >> 4];
            charPattern = vdp->vram[vdp->chrGenBase & index];

            linePtr2[0] = (col = sprLine[0]) ? vdp->palette[col] : color[(charPattern >> 7) & 1]; 
            linePtr2[1] = (col = sprLine[1]) ? vdp->palette[col] : color[(charPattern >> 6) & 1];
            linePtr2[2] = (col = sprLine[2]) ? vdp->palette[col] : color[(charPattern >> 5) & 1];
            linePtr2[3] = (col = sprLine[3]) ? vdp->palette[col] : color[(charPattern >> 4) & 1];
            linePtr2[4] = (col = sprLine[4]) ? vdp->palette[col] : color[(charPattern >> 3) & 1];
            linePtr2[5] = (col = sprLine[5]) ? vdp->palette[col] : color[(charPattern >> 2) & 1];
            linePtr2[6] = (col = sprLine[6]) ? vdp->palette[col] : color[(charPattern >> 1) & 1];
            linePtr2[7] = (col = sprLine[7]) ? vdp->palette[col] : color[(charPattern >> 0) & 1];
            sprLine += 8;
            charTable++;
            UPDATE_TABLE_4();
            linePtr2 += 8; 
            X++;
        }
    }
    if (rightBorder) {
        spritesLine(vdp, Y);
        RefreshRightBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, 0);
    }
}

static void RefreshLine3(VDP* vdp, int Y, int X, int X2)
{
    static UInt8*  sprLine;
    int    rightBorder;

    if (X == -1) {
        X++;
        linePtr3 = RefreshBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, 0);
        //if(!linePtr3) return;
        sprLine = getSpritesLine(vdp, Y);
    }

    if (linePtr3 == NULL) {
        return;
    }

    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }


    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor = vdp->palette[vdp->BGColor];
        while (X < X2) {
            linePtr3[0] = bgColor;
            linePtr3[1] = bgColor;
            linePtr3[2] = bgColor;
            linePtr3[3] = bgColor;
            linePtr3[4] = bgColor;
            linePtr3[5] = bgColor;
            linePtr3[6] = bgColor;
            linePtr3[7] = bgColor;
            linePtr3 += 8; 
            X++;
        }
    }
    else {
        int    y = Y - vdp->firstLine + vdpVScroll(vdp);
        UInt8* charTable   = vdp->vram + (vdp->chrTabBase & ((-1 << 10) | (32 * (y / 8)))) + X;
        int    patternBase = vdp->chrGenBase & ((-1 << 11) | ((y >> 2) & 7));

        while (X < X2) {
            UInt8  colPattern = vdp->vram[patternBase | ((int)*charTable * 8)];
            Pixel fc = vdp->palette[colPattern >> 4];
            Pixel bc = vdp->palette[colPattern & 0x0f];
            UInt8 col;

            col = sprLine[0]; linePtr3[0] = col ? vdp->palette[col] : fc; 
            col = sprLine[1]; linePtr3[1] = col ? vdp->palette[col] : fc;
            col = sprLine[2]; linePtr3[2] = col ? vdp->palette[col] : fc; 
            col = sprLine[3]; linePtr3[3] = col ? vdp->palette[col] : fc;
            col = sprLine[4]; linePtr3[4] = col ? vdp->palette[col] : bc; 
            col = sprLine[5]; linePtr3[5] = col ? vdp->palette[col] : bc;
            col = sprLine[6]; linePtr3[6] = col ? vdp->palette[col] : bc; 
            col = sprLine[7]; linePtr3[7] = col ? vdp->palette[col] : bc;
            sprLine += 8;
            charTable++; 
            linePtr3 += 8; 
            X++;
        }
    }
    if (rightBorder) {
        spritesLine(vdp, Y);
        RefreshRightBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, 0);
    }
}

#if 1

static void RefreshLine4(VDP* vdp, int Y, int X, int X2)
{
    static UInt8*  sprLine;
    static UInt8*  charTable;
    static int     base;
    static int     hScroll;
    static int     hScroll512;
    static int*    jump;
    static int     page;
    static int     scroll;
    UInt8  charPattern;
    UInt8  colPattern;
    UInt8  col;
    Pixel color[2];
    int    index;
    int    rightBorder;

    if (X == -1) {
        int y;

        X++;
        linePtr4 = RefreshBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, 0);
        //if(!linePtr4) return;
        sprLine = getSpritesLine(vdp, Y);

        if (linePtr4 == NULL) {
            return;
        }

        //hScroll    =  ((((int)(vdp->vdpRegs[26] & 0x3F & ~(~vdpHScroll512(vdp) << 5))) << 3) - (int)(vdp->vdpRegs[27] & 0x07) & 0xffffffff);
        hScroll    = vdpHScroll(vdp);
        hScroll512 = vdpHScroll512(vdp);
        jump       = jumpTable4 + hScroll512 * 2;
        page       = (vdp->chrTabBase / 0x8000) & 1;
        scroll     = hScroll >> 3;

        y = Y - vdp->firstLine + vdpVScroll(vdp);
        charTable   = vdp->vram + (vdp->chrTabBase & ((-1 << 10) | (32 * (y / 8)))) + scroll;
        base        = (-1 << 13) | ((y & 0xc0) << 5) | (y & 7);

        if (hScroll512) {
            if (scroll & 0x20) charTable += jump[page ^= 1];
            if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 32;
        }

        if (vdpIsEdgeMasked(vdp->vdpRegs)) {
            Pixel bgColor = vdp->palette[vdp->BGColor];
            linePtr4[0] = bgColor;
            linePtr4[1] = bgColor;
            linePtr4[2] = bgColor;
            linePtr4[3] = bgColor;
            linePtr4[4] = bgColor;
            linePtr4[5] = bgColor;
            linePtr4[6] = bgColor;
            linePtr4[7] = bgColor;
            charTable++; 
            UPDATE_TABLE_4(); 
            X++;
            sprLine += sprLine != NULL ? 8 : 0;
            linePtr4 += 8;
        }

        index       = base | ((int)*charTable * 8);
        colPattern = vdp->vram[vdp->colTabBase & index];
        color[0]   = vdp->palette[colPattern & 0x0f];
        color[1]   = vdp->palette[colPattern >> 4];
        charPattern = vdp->vram[vdp->chrGenBase & index];

        if (!vdp->screenOn || !vdp->drawArea) {
            Pixel bgColor = vdp->palette[vdp->BGColor];

            switch (hScroll & 7) {
            case 1: *linePtr4++ = bgColor; 
            case 2: *linePtr4++ = bgColor; 
            case 3: *linePtr4++ = bgColor; 
            case 4: *linePtr4++ = bgColor; 
            case 5: *linePtr4++ = bgColor; 
            case 6: *linePtr4++ = bgColor; 
            case 7: *linePtr4++ = bgColor;  charTable++; UPDATE_TABLE_4();
            }
        }
        else {
            switch (hScroll & 7) {
            case 1: col = *sprLine++; *linePtr4++ = col ? vdp->palette[col >> 1] : color[(charPattern >> 6) & 1]; 
            case 2: col = *sprLine++; *linePtr4++ = col ? vdp->palette[col >> 1] : color[(charPattern >> 5) & 1]; 
            case 3: col = *sprLine++; *linePtr4++ = col ? vdp->palette[col >> 1] : color[(charPattern >> 4) & 1]; 
            case 4: col = *sprLine++; *linePtr4++ = col ? vdp->palette[col >> 1] : color[(charPattern >> 3) & 1]; 
            case 5: col = *sprLine++; *linePtr4++ = col ? vdp->palette[col >> 1] : color[(charPattern >> 2) & 1]; 
            case 6: col = *sprLine++; *linePtr4++ = col ? vdp->palette[col >> 1] : color[(charPattern >> 1) & 1]; 
            case 7: col = *sprLine++; *linePtr4++ = col ? vdp->palette[col >> 1] : color[(charPattern >> 0) & 1]; charTable++; UPDATE_TABLE_4();
            }
        }
    }

    if (linePtr4 == NULL) {
        return;
    }

    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor = vdp->palette[vdp->BGColor];
        while (X < X2) {
            linePtr4[0] = bgColor;
            linePtr4[1] = bgColor;
            linePtr4[2] = bgColor;
            linePtr4[3] = bgColor;
            linePtr4[4] = bgColor;
            linePtr4[5] = bgColor;
            linePtr4[6] = bgColor;
            linePtr4[7] = bgColor;
            linePtr4 += 8; 
            X++;
        }
    }
    else {
        while (X < X2) {
            index       = base | ((int)*charTable * 8);
            colPattern = vdp->vram[vdp->colTabBase & index];
            color[0]   = vdp->palette[colPattern & 0x0f];
            color[1]   = vdp->palette[colPattern >> 4];
            charPattern = vdp->vram[vdp->chrGenBase & index];

            linePtr4[0] = (col = sprLine[0]) ? vdp->palette[col >> 1] : color[(charPattern >> 7) & 1]; 
            linePtr4[1] = (col = sprLine[1]) ? vdp->palette[col >> 1] : color[(charPattern >> 6) & 1];
            linePtr4[2] = (col = sprLine[2]) ? vdp->palette[col >> 1] : color[(charPattern >> 5) & 1];
            linePtr4[3] = (col = sprLine[3]) ? vdp->palette[col >> 1] : color[(charPattern >> 4) & 1];
            linePtr4[4] = (col = sprLine[4]) ? vdp->palette[col >> 1] : color[(charPattern >> 3) & 1];
            linePtr4[5] = (col = sprLine[5]) ? vdp->palette[col >> 1] : color[(charPattern >> 2) & 1];
            linePtr4[6] = (col = sprLine[6]) ? vdp->palette[col >> 1] : color[(charPattern >> 1) & 1];
            linePtr4[7] = (col = sprLine[7]) ? vdp->palette[col >> 1] : color[(charPattern >> 0) & 1];
            sprLine += 8;
            charTable++;
            UPDATE_TABLE_4();
            linePtr4 += 8; 
            X++;
        }
    }
    if (rightBorder) {
        colorSpritesLine(vdp, Y, 0);
        RefreshRightBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, 0);
    }
}


#else

static void RefreshLine4(VDP* vdp, int Y, int X, int X2)
{
    static UInt8*  sprLine;
    static UInt8*  charTable;
    static int     base;
    static int     hScroll;
    static int     hScroll512;
    static int*    jump;
    static int     page;
    static int     scroll;
    UInt8  charPattern;
    UInt8  colPattern;
    UInt8  col;
    Pixel color[2];
    int    index;
    int    rightBorder;

    if (X == -1) {
        int y;

        X++;
        linePtr4 = RefreshBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, 0);
        //if(!linePtr4) return;
        sprLine = getSpritesLine(vdp, Y);

        if (linePtr4 == NULL) {
            return;
        }

        hScroll    =  ((((int)(vdp->vdpRegs[26] & 0x3F & ~(~vdpHScroll512(vdp) << 5))) << 3) - (int)(vdp->vdpRegs[27] & 0x07) & 0xffffffff);
        hScroll    = vdpHScroll(vdp);
        hScroll512 = vdpHScroll512(vdp);
        jump       = jumpTable4 + hScroll512 * 2;
        page       = (vdp->chrTabBase / 0x8000) & 1;
        scroll     = hScroll >> 3;

        y = Y - vdp->firstLine + vdpVScroll(vdp);
        charTable   = vdp->vram + (vdp->chrTabBase & ((-1 << 10) | (32 * (y / 8)))) + scroll;
        base        = (-1 << 13) | ((y & 0xc0) << 5) | (y & 7);

        if (hScroll512) {
            if (scroll & 0x20) charTable += jump[page ^= 1];
            if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 32;
        }

        index       = base | ((int)*charTable * 8);
        colPattern = vdp->vram[vdp->colTabBase & index];
        color[0]   = vdp->palette[colPattern & 0x0f];
        color[1]   = vdp->palette[colPattern >> 4];
        charPattern = vdp->vram[vdp->chrGenBase & index];
    }

    if (linePtr4 == NULL || X2 <= 0) {
        return;
    }

#define BRK_4 24
    if (X < BRK_4 && X2 >= BRK_4) colorSpritesLine(vdp, Y, 0);

    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor = vdp->palette[vdp->BGColor];
        while (X < X2) {
            linePtr4[0] = bgColor;
            linePtr4[1] = bgColor;
            linePtr4[2] = bgColor;
            linePtr4[3] = bgColor;
            linePtr4[4] = bgColor;
            linePtr4[5] = bgColor;
            linePtr4[6] = bgColor;
            linePtr4[7] = bgColor;
            linePtr4 += 8; 
            X++;
        }
    }
    else {
        if (X == 0) {
            if (vdpIsEdgeMasked(vdp->vdpRegs)) {
                Pixel bgColor = vdp->palette[vdp->BGColor];
                linePtr4[0] = bgColor;
                linePtr4[1] = bgColor;
                linePtr4[2] = bgColor;
                linePtr4[3] = bgColor;
                linePtr4[4] = bgColor;
                linePtr4[5] = bgColor;
                linePtr4[6] = bgColor;
                linePtr4[7] = bgColor;
                X++;
                sprLine += sprLine != NULL ? 8 : 0;
                linePtr4 += 8;
            }

            if (!vdp->screenOn || !vdp->drawArea) {
                Pixel bgColor = vdp->palette[vdp->BGColor];

                switch (hScroll & 7) {
                case 1: *linePtr4++ = bgColor; 
                case 2: *linePtr4++ = bgColor; 
                case 3: *linePtr4++ = bgColor; 
                case 4: *linePtr4++ = bgColor; 
                case 5: *linePtr4++ = bgColor; 
                case 6: *linePtr4++ = bgColor; 
                case 7: *linePtr4++ = bgColor;  charTable++; UPDATE_TABLE_4();
                }
            }
            else {
                if (vdpIsEdgeMasked(vdp->vdpRegs)) {
                    switch (hScroll & 7) {
                    case 1: col = *sprLine++; *linePtr4++ = col ? vdp->palette[col >> 1] : color[(charPattern >> 6) & 1]; 
                    case 2: col = *sprLine++; *linePtr4++ = col ? vdp->palette[col >> 1] : color[(charPattern >> 5) & 1]; 
                    case 3: col = *sprLine++; *linePtr4++ = col ? vdp->palette[col >> 1] : color[(charPattern >> 4) & 1]; 
                    case 4: col = *sprLine++; *linePtr4++ = col ? vdp->palette[col >> 1] : color[(charPattern >> 3) & 1]; 
                    case 5: col = *sprLine++; *linePtr4++ = col ? vdp->palette[col >> 1] : color[(charPattern >> 2) & 1]; 
                    case 6: col = *sprLine++; *linePtr4++ = col ? vdp->palette[col >> 1] : color[(charPattern >> 1) & 1]; 
                    case 7: col = *sprLine++; *linePtr4++ = col ? vdp->palette[col >> 1] : color[(charPattern >> 0) & 1]; charTable++; UPDATE_TABLE_4();
                    }
                }
                else {
                    Pixel bgColor = vdp->palette[vdp->BGColor];

                    switch (hScroll & 7) {
                    case 1: col = *sprLine++; *linePtr4++ = bgColor; 
                    case 2: col = *sprLine++; *linePtr4++ = bgColor; 
                    case 3: col = *sprLine++; *linePtr4++ = bgColor; 
                    case 4: col = *sprLine++; *linePtr4++ = bgColor; 
                    case 5: col = *sprLine++; *linePtr4++ = bgColor; 
                    case 6: col = *sprLine++; *linePtr4++ = bgColor; 
                    case 7: col = *sprLine++; *linePtr4++ = bgColor; charTable++; UPDATE_TABLE_4();
                    }
                }
            }
        }

        while (X < X2) {
            index       = base | ((int)*charTable * 8);
            colPattern = vdp->vram[vdp->colTabBase & index];
            color[0]   = vdp->palette[colPattern & 0x0f];
            color[1]   = vdp->palette[colPattern >> 4];
            charPattern = vdp->vram[vdp->chrGenBase & index];

            linePtr4[0] = (col = sprLine[0]) ? vdp->palette[col >> 1] : color[(charPattern >> 7) & 1]; 
            linePtr4[1] = (col = sprLine[1]) ? vdp->palette[col >> 1] : color[(charPattern >> 6) & 1];
            linePtr4[2] = (col = sprLine[2]) ? vdp->palette[col >> 1] : color[(charPattern >> 5) & 1];
            linePtr4[3] = (col = sprLine[3]) ? vdp->palette[col >> 1] : color[(charPattern >> 4) & 1];
            linePtr4[4] = (col = sprLine[4]) ? vdp->palette[col >> 1] : color[(charPattern >> 3) & 1];
            linePtr4[5] = (col = sprLine[5]) ? vdp->palette[col >> 1] : color[(charPattern >> 2) & 1];
            linePtr4[6] = (col = sprLine[6]) ? vdp->palette[col >> 1] : color[(charPattern >> 1) & 1];
            linePtr4[7] = (col = sprLine[7]) ? vdp->palette[col >> 1] : color[(charPattern >> 0) & 1];
            sprLine += 8;
            charTable++;
            UPDATE_TABLE_4();
            linePtr4 += 8; 
            X++;
        }
    }
    if (rightBorder) {
//        colorSpritesLine(vdp, Y, 0);
        RefreshRightBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, 0);
    }
}

#endif

static void RefreshLine5(VDP* vdp, int Y, int X, int X2)
{
    static UInt8*  charTable;
    static UInt8*  sprLine;
    static int     hScroll512;
    static int*    jump;
    static int     page;
    static int     scroll;
    static int     hScroll;
    static int     vscroll;
    static int     chrTabO;
    int col;
    int rightBorder;

    if (X == -1) {
        X++;
        linePtr5 = RefreshBorder(vdp, Y,  vdp->palette[vdp->BGColor], 0, 0);
        //if(!linePtr5) return;
        sprLine   = getSpritesLine(vdp, Y);

        if (linePtr5 == NULL) {
            return;
        }

        hScroll512 = vdpHScroll512(vdp);
        jump       = jumpTable + hScroll512 * 2;
        page       = (vdp->chrTabBase / 0x8000) & 1;
        hScroll    = vdpHScroll(vdp);
        vscroll    = vdpVScroll(vdp);
        chrTabO    = vdp->chrTabBase;
        scroll     = hScroll / 2;

        charTable = vdp->vram + (vdp->chrTabBase & (~vdpIsOddPage(vdp) << 7) & ((-1 << 15) | ((Y - vdp->firstLine + vdpVScroll(vdp)) << 7))) + scroll;

        if (hScroll512) {
            if (scroll & 0x80) charTable += jump[page ^= 1];
            if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 128;
        }
    }

    if (linePtr5 == NULL || X2 <= 0) {
        return;
    }

#define BRK_5 24
    if (X < BRK_5 && X2 >= BRK_5) colorSpritesLine(vdp, Y, 0);

    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor = vdp->palette[vdp->BGColor];
        while (X < X2) {
            linePtr5[0] = bgColor;
            linePtr5[1] = bgColor;
            linePtr5[2] = bgColor;
            linePtr5[3] = bgColor;
            linePtr5[4] = bgColor;
            linePtr5[5] = bgColor;
            linePtr5[6] = bgColor;
            linePtr5[7] = bgColor;
            linePtr5 += 8; 
            X++;
        }
    }
    else {
        // Update vscroll if needed
        if (vscroll != vdpVScroll(vdp) || chrTabO != vdp->chrTabBase) {
            jump       = jumpTable + hScroll512 * 2;
            page       = (vdp->chrTabBase / 0x8000) & 1;
            hScroll    = vdpHScroll(vdp) + X * 8;
            scroll     = hScroll / 2;
            vscroll    = vdpVScroll(vdp);
            chrTabO    = vdp->chrTabBase;

            charTable = vdp->vram + (vdp->chrTabBase & (~vdpIsOddPage(vdp) << 7) & ((-1 << 15) | ((Y - vdp->firstLine + vdpVScroll(vdp)) << 7))) + scroll;

            if (hScroll512) {
                if (scroll & 0x80) charTable += jump[page ^= 1];
                if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 128;
            }
        }

        if (X == 0 && vdpIsEdgeMasked(vdp->vdpRegs)) {
            Pixel bgColor = vdp->palette[vdp->BGColor];
            linePtr5[0] = bgColor;
            linePtr5[1] = bgColor;
            linePtr5[2] = bgColor;
            linePtr5[3] = bgColor;
            linePtr5[4] = bgColor;
            linePtr5[5] = bgColor;
            linePtr5[6] = bgColor;
            linePtr5[7] = bgColor;

            UPDATE_TABLE_5(); UPDATE_TABLE_5(); UPDATE_TABLE_5(); UPDATE_TABLE_5();
            sprLine   += sprLine != NULL ? 8 : 0;
            linePtr5 += 8;
            charTable += 4;
            X++;
        }

        if (X == 0 && vdp->screenOn && vdp->drawArea) {
            Pixel bgColor = vdp->palette[vdp->BGColor];
            int i = hScroll & 7;
            int j;

            for (j = 0; j < i; j++) {
                linePtr5[0] = bgColor;
                if ((hScroll ^ j) & 1) {
                    charTable++; UPDATE_TABLE_5();
                }
                sprLine++;
                linePtr5 += 1;
            }
            
            for (;j < 8; j++) {
                if ((hScroll ^ j) & 1) {
                    linePtr5[0] = vdp->palette[(col = sprLine[0]) ? col >> 1 : charTable[0] & 0x0f]; charTable++; UPDATE_TABLE_5();
                }
                else {
                    linePtr5[0] = vdp->palette[(col = sprLine[0]) ? col >> 1 : charTable[0] >> 4];
                }
                sprLine++;
                linePtr5++;
            } 
            X++;
        }

        while (X < X2) {
            if (hScroll & 1) {
                linePtr5[0] = vdp->palette[(col = sprLine[0]) ? col >> 1 : charTable[0] & 0x0f]; UPDATE_TABLE_5();
                linePtr5[1] = vdp->palette[(col = sprLine[1]) ? col >> 1 : charTable[1] >> 4];
                linePtr5[2] = vdp->palette[(col = sprLine[2]) ? col >> 1 : charTable[1] & 0x0f]; UPDATE_TABLE_5();
                linePtr5[3] = vdp->palette[(col = sprLine[3]) ? col >> 1 : charTable[2] >> 4];
                linePtr5[4] = vdp->palette[(col = sprLine[4]) ? col >> 1 : charTable[2] & 0x0f]; UPDATE_TABLE_5();
                linePtr5[5] = vdp->palette[(col = sprLine[5]) ? col >> 1 : charTable[3] >> 4];
                linePtr5[6] = vdp->palette[(col = sprLine[6]) ? col >> 1 : charTable[3] & 0x0f]; UPDATE_TABLE_5();
                linePtr5[7] = vdp->palette[(col = sprLine[7]) ? col >> 1 : charTable[4] >> 4];
            }
            else {     
                linePtr5[0] = vdp->palette[(col = sprLine[0]) ? col >> 1 : charTable[0] >> 4];
                linePtr5[1] = vdp->palette[(col = sprLine[1]) ? col >> 1 : charTable[0] & 0x0f]; UPDATE_TABLE_5();
                linePtr5[2] = vdp->palette[(col = sprLine[2]) ? col >> 1 : charTable[1] >> 4];
                linePtr5[3] = vdp->palette[(col = sprLine[3]) ? col >> 1 : charTable[1] & 0x0f]; UPDATE_TABLE_5();
                linePtr5[4] = vdp->palette[(col = sprLine[4]) ? col >> 1 : charTable[2] >> 4];
                linePtr5[5] = vdp->palette[(col = sprLine[5]) ? col >> 1 : charTable[2] & 0x0f]; UPDATE_TABLE_5();
                linePtr5[6] = vdp->palette[(col = sprLine[6]) ? col >> 1 : charTable[3] >> 4];
                linePtr5[7] = vdp->palette[(col = sprLine[7]) ? col >> 1 : charTable[3] & 0x0f]; UPDATE_TABLE_5();
            }
            sprLine += 8;

            linePtr5 += 8; 
            charTable += 4;
            X++;
        }
    }

    if (rightBorder) {
//        colorSpritesLine(vdp, Y, 0);
        RefreshRightBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, 0);
    }
}

#ifdef MAX_VIDEO_WIDTH_320

static void RefreshLine6(VDP* vdp, int Y, int X, int X2)
{
    static int*    jump;
    static int     hScroll512;
    static int     scroll;
    static int     page;
    static UInt8*  charTable;
    static UInt8*  sprLine;
    int col;
    int rightBorder;

    if (X == -1) {
        Pixel bgColor = MIX_COLOR(vdp->palette[(vdp->BGColor >> 2) & 0x03], vdp->palette[vdp->BGColor & 0x03]);
        X++;
        linePtr6 = RefreshBorder(vdp, Y, bgColor, 0, 0);
        //if(!linePtr6) return;
        sprLine   = getSpritesLine(vdp, Y);

        if (linePtr6 == NULL) {
            return;
        }

        hScroll512 = vdpHScroll512(vdp);
        scroll     = vdpHScroll(vdp);
        jump       = jumpTable + hScroll512 * 2;
        page    = (vdp->chrTabBase / 0x8000) & 1;

        charTable = vdp->vram + (vdp->chrTabBase & (~vdpIsOddPage(vdp) << 7) & ((-1 << 15) | ((Y - vdp->firstLine + vdpVScroll(vdp)) << 7))) + scroll / 2;

        if (hScroll512) {
            if (scroll & 0x100) charTable += jump[page ^= 1];
            if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 128;
        }

        if (vdpIsEdgeMasked(vdp->vdpRegs)) {
            Pixel bgColor = vdp->palette[vdp->BGColor & 0x03];
            linePtr6[0] = bgColor; 
            linePtr6[1] = bgColor; 
            linePtr6[2] = bgColor; 
            linePtr6[3] = bgColor; 
            linePtr6[4] = bgColor; 
            linePtr6[5] = bgColor; 
            linePtr6[6] = bgColor; 
            linePtr6[7] = bgColor; 
            UPDATE_TABLE_6(); UPDATE_TABLE_6(); UPDATE_TABLE_6(); UPDATE_TABLE_6();
            UPDATE_TABLE_6(); UPDATE_TABLE_6(); UPDATE_TABLE_6(); UPDATE_TABLE_6();
            sprLine += sprLine != NULL ? 8 : 0; 
            linePtr6 += 8; 
            charTable += 4;
            X++;
        }
    }

    if (linePtr6 == NULL) {
        return;
    }

    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor = MIX_COLOR(vdp->palette[(vdp->BGColor >> 2) & 0x03], vdp->palette[vdp->BGColor & 0x03]);
        while (X < X2) {
            linePtr6[0] = bgColor; 
            linePtr6[1] = bgColor; 
            linePtr6[2] = bgColor; 
            linePtr6[3] = bgColor; 
            linePtr6[4] = bgColor; 
            linePtr6[5] = bgColor; 
            linePtr6[6] = bgColor; 
            linePtr6[7] = bgColor; 
            linePtr6 += 8; 
            X++;
        }
    }
    else {
        while (X < X2) {
            Pixel c1, c2;
            if (scroll & 1) {
                c1 = vdp->palette[(col = sprLine[0] >> 3) ? (col >> 1) & 3 : (charTable[0] >> 2) & 3];
                c2 = vdp->palette[(col = sprLine[0]  & 7) ? (col >> 1) & 3 : (charTable[0] >> 0) & 3]; UPDATE_TABLE_6(); 
                linePtr6[0] = MIX_COLOR(c1, c2);
                c1 = vdp->palette[(col = sprLine[1] >> 3) ? (col >> 1) & 3 : (charTable[1] >> 6) & 3];
                c2 = vdp->palette[(col = sprLine[1]  & 7) ? (col >> 1) & 3 : (charTable[1] >> 4) & 3]; UPDATE_TABLE_6();
                linePtr6[1] = MIX_COLOR(c1, c2);
                c1 = vdp->palette[(col = sprLine[2] >> 3) ? (col >> 1) & 3 : (charTable[1] >> 2) & 3];
                c2 = vdp->palette[(col = sprLine[2]  & 7) ? (col >> 1) & 3 : (charTable[1] >> 0) & 3]; UPDATE_TABLE_6();
                linePtr6[2] = MIX_COLOR(c1, c2);
                c1 = vdp->palette[(col = sprLine[3] >> 3) ? (col >> 1) & 3 : (charTable[2] >> 6) & 3];
                c2 = vdp->palette[(col = sprLine[3]  & 7) ? (col >> 1) & 3 : (charTable[2] >> 4) & 3]; UPDATE_TABLE_6();
                linePtr6[3] = MIX_COLOR(c1, c2);
                c1 = vdp->palette[(col = sprLine[4] >> 3) ? (col >> 1) & 3 : (charTable[2] >> 2) & 3];
                c2 = vdp->palette[(col = sprLine[4]  & 7) ? (col >> 1) & 3 : (charTable[2] >> 0) & 3]; UPDATE_TABLE_6();
                linePtr6[4] = MIX_COLOR(c1, c2);
                c1 = vdp->palette[(col = sprLine[5] >> 3) ? (col >> 1) & 3 : (charTable[3] >> 6) & 3];
                c2 = vdp->palette[(col = sprLine[5]  & 7) ? (col >> 1) & 3 : (charTable[3] >> 4) & 3]; UPDATE_TABLE_6();
                linePtr6[5] = MIX_COLOR(c1, c2);
                c1 = vdp->palette[(col = sprLine[6] >> 3) ? (col >> 1) & 3 : (charTable[3] >> 2) & 3];
                c2 = vdp->palette[(col = sprLine[6]  & 7) ? (col >> 1) & 3 : (charTable[3] >> 0) & 3]; UPDATE_TABLE_6();
                linePtr6[6] = MIX_COLOR(c1, c2);
                c1 = vdp->palette[(col = sprLine[7] >> 3) ? (col >> 1) & 3 : (charTable[4] >> 6) & 3];
                c2 = vdp->palette[(col = sprLine[7]  & 7) ? (col >> 1) & 3 : (charTable[4] >> 4) & 3]; UPDATE_TABLE_6();
                linePtr6[7] = MIX_COLOR(c1, c2);
            }
            else {
                c1 = vdp->palette[(col = sprLine[0] >> 3) ? (col >> 1) & 3 : (charTable[0] >> 6) & 3];
                c2 = vdp->palette[(col = sprLine[0]  & 7) ? (col >> 1) & 3 : (charTable[0] >> 4) & 3]; UPDATE_TABLE_6();
                linePtr6[0] = MIX_COLOR(c1, c2);
                c1 = vdp->palette[(col = sprLine[1] >> 3) ? (col >> 1) & 3 : (charTable[0] >> 2) & 3];
                c2 = vdp->palette[(col = sprLine[1]  & 7) ? (col >> 1) & 3 : (charTable[0] >> 0) & 3]; UPDATE_TABLE_6();
                linePtr6[1] = MIX_COLOR(c1, c2);
                c1 = vdp->palette[(col = sprLine[2] >> 3) ? (col >> 1) & 3 : (charTable[1] >> 6) & 3];
                c2 = vdp->palette[(col = sprLine[2]  & 7) ? (col >> 1) & 3 : (charTable[1] >> 4) & 3]; UPDATE_TABLE_6();
                linePtr6[2] = MIX_COLOR(c1, c2);
                c1 = vdp->palette[(col = sprLine[3] >> 3) ? (col >> 1) & 3 : (charTable[1] >> 2) & 3];
                c2 = vdp->palette[(col = sprLine[3]  & 7) ? (col >> 1) & 3 : (charTable[1] >> 0) & 3]; UPDATE_TABLE_6();
                linePtr6[3] = MIX_COLOR(c1, c2);
                c1 = vdp->palette[(col = sprLine[4] >> 3) ? (col >> 1) & 3 : (charTable[2] >> 6) & 3];
                c2 = vdp->palette[(col = sprLine[4]  & 7) ? (col >> 1) & 3 : (charTable[2] >> 4) & 3]; UPDATE_TABLE_6();
                linePtr6[4] = MIX_COLOR(c1, c2);
                c1 = vdp->palette[(col = sprLine[5] >> 3) ? (col >> 1) & 3 : (charTable[2] >> 2) & 3];
                c2 = vdp->palette[(col = sprLine[5]  & 7) ? (col >> 1) & 3 : (charTable[2] >> 0) & 3]; UPDATE_TABLE_6();
                linePtr6[5] = MIX_COLOR(c1, c2);
                c1 = vdp->palette[(col = sprLine[6] >> 3) ? (col >> 1) & 3 : (charTable[3] >> 6) & 3];
                c2 = vdp->palette[(col = sprLine[6]  & 7) ? (col >> 1) & 3 : (charTable[3] >> 4) & 3]; UPDATE_TABLE_6();
                linePtr6[6] = MIX_COLOR(c1, c2);
                c1 = vdp->palette[(col = sprLine[7] >> 3) ? (col >> 1) & 3 : (charTable[3] >> 2) & 3];
                c2 = vdp->palette[(col = sprLine[7]  & 7) ? (col >> 1) & 3 : (charTable[3] >> 0) & 3]; UPDATE_TABLE_6();
                linePtr6[7] = MIX_COLOR(c1, c2);
            }
            sprLine += 8; 

            linePtr6 += 8; 
            charTable += 4;
            X++;
        }
    }
    if (rightBorder) {
        Pixel bgColor = MIX_COLOR(vdp->palette[(vdp->BGColor >> 2) & 0x03], vdp->palette[vdp->BGColor & 0x03]);
        colorSpritesLine(vdp, Y, 1);
        RefreshRightBorder(vdp, Y, bgColor, 0, 0);
    }
}

static void RefreshLine7(VDP* vdp, int Y, int X, int X2)
{
    static UInt8*  charTable;
    static UInt8*  sprLine;
    static int     hScroll512;
    static int     page;
    static int*    jump;
    static int     scroll;
    static int     vscroll;
    static int     chrTabO;
    int col;
    int rightBorder;

    if (X == -1) {
        X++;
        linePtr7 = RefreshBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, 0);
        //if(!linePtr7) return;
        sprLine = getSpritesLine(vdp, Y);
    
        if (linePtr7 == NULL) {
            return;
        }

        hScroll512 = vdpHScroll512(vdp);
        jump       = jumpTable + hScroll512 * 2;
        page    = (vdp->chrTabBase / 0x8000) & 1;
        scroll     = vdpHScroll(vdp);
        vscroll    = vdpVScroll(vdp);
        chrTabO    = vdp->chrTabBase;

        charTable = vdp->vram + (vdp->chrTabBase & (~vdpIsOddPage(vdp) << 7) & ((-1 << 15) | ((Y - vdp->firstLine + vdpVScroll(vdp)) << 7))) + scroll / 2;


        if (hScroll512) {
            if (scroll & 0x100) charTable += jump[page ^= 1];
            if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 128;
        }

        if (vdpIsEdgeMasked(vdp->vdpRegs)) {
            Pixel bgColor = vdp->palette[vdp->BGColor];
            linePtr7[0] = bgColor; 
            linePtr7[1] = bgColor; 
            linePtr7[2] = bgColor; 
            linePtr7[3] = bgColor; 
            linePtr7[4] = bgColor; 
            linePtr7[5] = bgColor; 
            linePtr7[6] = bgColor; 
            linePtr7[7] = bgColor; 
            UPDATE_TABLE_7(); UPDATE_TABLE_7(); UPDATE_TABLE_7(); UPDATE_TABLE_7();
            UPDATE_TABLE_7(); UPDATE_TABLE_7(); UPDATE_TABLE_7(); UPDATE_TABLE_7();
            sprLine   += sprLine != NULL ? 8 : 0;
            linePtr7 += 8; 
            charTable += 4;
            X++; 
        }
    }

    if (linePtr7 == NULL) {
        return;
    }

    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor = vdp->palette[vdp->BGColor];
        while (X < X2) {
            linePtr7[0] = bgColor; 
            linePtr7[1] = bgColor; 
            linePtr7[2] = bgColor; 
            linePtr7[3] = bgColor; 
            linePtr7[4] = bgColor; 
            linePtr7[5] = bgColor; 
            linePtr7[6] = bgColor; 
            linePtr7[7] = bgColor; 
            linePtr7 += 8; 
            X++;
        }
    }
    else {
        // Update vscroll if needed
        if (vscroll != vdpVScroll(vdp) || chrTabO != vdp->chrTabBase) {
            scroll  = vdpHScroll(vdp) + X * 8;
            page = (vdp->chrTabBase / 0x8000) & 1;
            jump    = jumpTable + hScroll512 * 2;
            vscroll = vdpVScroll(vdp);
            chrTabO  = vdp->chrTabBase;

            charTable = vdp->vram + (vdp->chrTabBase & (~vdpIsOddPage(vdp) << 7) & ((-1 << 15) | ((Y - vdp->firstLine + vdpVScroll(vdp)) << 7))) + scroll / 2;

            if (hScroll512) {
                if (scroll & 0x100) charTable += jump[page ^= 1];
                if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 128;
            }
        }

        while (X < X2) {
            if (scroll & 1) {
                (col = sprLine[0]) ? linePtr7[0] = vdp->palette[col >> 1] : 
                (col = charTable[vdp->vram128], 
                linePtr7[0]  = MIX_COLOR(vdp->palette[col >> 4], vdp->palette[col & 0xf]));
                UPDATE_TABLE_7();
                (col = sprLine[1]) ? linePtr7[1]  = vdp->palette[col >> 1] :
                (col = charTable[1],  
                linePtr7[1]  = MIX_COLOR(vdp->palette[col >> 4], vdp->palette[col & 0xf])); 
                UPDATE_TABLE_7();
                (col = sprLine[2]) ? linePtr7[2]  = vdp->palette[col >> 1] : 
                (col = charTable[vdp->vram128|1],
                linePtr7[2]  = MIX_COLOR(vdp->palette[col >> 4], vdp->palette[col & 0xf]));
                UPDATE_TABLE_7();
                (col = sprLine[3]) ? linePtr7[3]  = vdp->palette[col >> 1] : 
                (col = charTable[2],   
                linePtr7[3]  = MIX_COLOR(vdp->palette[col >> 4], vdp->palette[col & 0xf])); 
                UPDATE_TABLE_7();
                (col = sprLine[4]) ? linePtr7[4]  = vdp->palette[col >> 1] :
                (col = charTable[vdp->vram128|2], 
                linePtr7[4]  = MIX_COLOR(vdp->palette[col >> 4], vdp->palette[col & 0xf]));
                UPDATE_TABLE_7();
                (col = sprLine[5]) ? linePtr7[5]  = vdp->palette[col >> 1] : 
                (col = charTable[3],      
                linePtr7[5] = MIX_COLOR(vdp->palette[col >> 4], vdp->palette[col & 0xf])); 
                UPDATE_TABLE_7();
                (col = sprLine[6]) ? linePtr7[6] = vdp->palette[col >> 1] : 
                (col = charTable[vdp->vram128|3], 
                linePtr7[6] = MIX_COLOR(vdp->palette[col >> 4], vdp->palette[col & 0xf]));
                UPDATE_TABLE_7();
                (col = sprLine[7]) ? linePtr7[7] = vdp->palette[col >> 1] : 
                (col = charTable[4],  
                linePtr7[7] = MIX_COLOR(vdp->palette[col >> 4], vdp->palette[col & 0xf])); 
                UPDATE_TABLE_7();
            }
            else {
                (col = sprLine[0]) ? linePtr7[0] = vdp->palette[col >> 1] : 
                (col = charTable[0],      
                linePtr7[0] = MIX_COLOR(vdp->palette[col >> 4], vdp->palette[col & 0xf]));
                UPDATE_TABLE_7();
                (col = sprLine[1]) ? linePtr7[1] = vdp->palette[col >> 1] : 
                (col = charTable[vdp->vram128], 
                linePtr7[1] = MIX_COLOR(vdp->palette[col >> 4], vdp->palette[col & 0xf]));
                UPDATE_TABLE_7();
                (col = sprLine[2]) ? linePtr7[2] = vdp->palette[col >> 1] :
                (col = charTable[1],    
                linePtr7[2] = MIX_COLOR(vdp->palette[col >> 4], vdp->palette[col & 0xf]));
                UPDATE_TABLE_7();
                (col = sprLine[3]) ? linePtr7[3] = vdp->palette[col >> 1] :
                (col = charTable[vdp->vram128|1],
                linePtr7[3] = MIX_COLOR(vdp->palette[col >> 4], vdp->palette[col & 0xf]));
                UPDATE_TABLE_7();
                (col = sprLine[4]) ? linePtr7[4] = vdp->palette[col >> 1] : 
                (col = charTable[2],      
                linePtr7[4] = MIX_COLOR(vdp->palette[col >> 4], vdp->palette[col & 0xf])); 
                UPDATE_TABLE_7();
                (col = sprLine[5]) ? linePtr7[5] = vdp->palette[col >> 1] :
                (col = charTable[vdp->vram128|2], 
                linePtr7[5] = MIX_COLOR(vdp->palette[col >> 4], vdp->palette[col & 0xf])); 
                UPDATE_TABLE_7();
                (col = sprLine[6]) ? linePtr7[6] = vdp->palette[col >> 1] : 
                (col = charTable[3],   
                linePtr7[6] = MIX_COLOR(vdp->palette[col >> 4], vdp->palette[col & 0xf])); 
                UPDATE_TABLE_7();
                (col = sprLine[7]) ? linePtr7[7] = vdp->palette[col >> 1] : 
                (col = charTable[vdp->vram128|3], 
                linePtr7[7] = MIX_COLOR(vdp->palette[col >> 4], vdp->palette[col & 0xf])); 
                UPDATE_TABLE_7();
            }
            sprLine += 8; 

            linePtr7 += 8; 
            charTable += 4;
            X++;
        }
    }    
    if (rightBorder) {
        colorSpritesLine(vdp, Y, 0);
        RefreshRightBorder(vdp, Y, vdp->palette[vdp->BGColor], 0, 0);
    }
}

#else

static void RefreshLine6(VDP* vdp, int Y, int X, int X2)
{
    static int*    jump;
    static int     hScroll512;
    static int     scroll;
    static int     page;
    static UInt8*  charTable;
    static UInt8*  sprLine;
    int col;
    int rightBorder;

    if (X == -1) {
        X++;
        linePtr6 = RefreshBorder6(vdp, Y, vdp->palette[(vdp->BGColor >> 2) & 0x03], vdp->palette[vdp->BGColor & 0x03], 1, 0);
        //if(!linePtr6) return;
        sprLine   = getSpritesLine(vdp, Y);

        if (linePtr6 == NULL) {
            return;
        }

        hScroll512 = vdpHScroll512(vdp);
        scroll     = vdpHScroll(vdp);
        jump       = jumpTable + hScroll512 * 2;
        page    = (vdp->chrTabBase / 0x8000) & 1;

        charTable = vdp->vram + (vdp->chrTabBase & (~vdpIsOddPage(vdp) << 7) & ((-1 << 15) | ((Y - vdp->firstLine + vdpVScroll(vdp)) << 7))) + scroll / 2;

        if (hScroll512) {
            if (scroll & 0x100) charTable += jump[page ^= 1];
            if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 128;
        }
    }

    if (linePtr6 == NULL || X2 <= 0) {
        return;
    }

#define BRK_6 24
    if (X < BRK_6 && X2 >= BRK_6) colorSpritesLine(vdp, Y, 1);

    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor1 = vdp->palette[(vdp->BGColor >> 2) & 0x03];
        Pixel bgColor2 = vdp->palette[vdp->BGColor & 0x03];
        while (X < X2) {
            linePtr6[ 0] = bgColor1;
            linePtr6[ 1] = bgColor2;
            linePtr6[ 2] = bgColor1;
            linePtr6[ 3] = bgColor2;
            linePtr6[ 4] = bgColor1;
            linePtr6[ 5] = bgColor2;
            linePtr6[ 6] = bgColor1;
            linePtr6[ 7] = bgColor2;
            linePtr6[ 8] = bgColor1;
            linePtr6[ 9] = bgColor2;
            linePtr6[10] = bgColor1;
            linePtr6[11] = bgColor2;
            linePtr6[12] = bgColor1;
            linePtr6[13] = bgColor2;
            linePtr6[14] = bgColor1;
            linePtr6[15] = bgColor2;
            linePtr6 += 16; 
            X++;
        }
    }
    else {
        if (X == 0 && vdpIsEdgeMasked(vdp->vdpRegs)) {
            Pixel bgColor = vdp->palette[vdp->BGColor & 0x03];
            linePtr6[0]  = linePtr6[1]  = bgColor; 
            linePtr6[2]  = linePtr6[3]  = bgColor; 
            linePtr6[4]  = linePtr6[5]  = bgColor; 
            linePtr6[6]  = linePtr6[7]  = bgColor; 
            linePtr6[8]  = linePtr6[9]  = bgColor; 
            linePtr6[10] = linePtr6[11] = bgColor; 
            linePtr6[12] = linePtr6[13] = bgColor; 
            linePtr6[14] = linePtr6[15] = bgColor; 
            UPDATE_TABLE_6(); UPDATE_TABLE_6(); UPDATE_TABLE_6(); UPDATE_TABLE_6();
            UPDATE_TABLE_6(); UPDATE_TABLE_6(); UPDATE_TABLE_6(); UPDATE_TABLE_6();
            sprLine += sprLine != NULL ? 8 : 0; 
            linePtr6 += 16; 
            charTable += 4;
            X++;
        }

        linePtr6[ 0] = vdp->palette[(col = sprLine[0] >> 3) ? (col >> 1) & 3 : (charTable[0] >> 2) & 3];

        while (X < X2) {
            if (scroll & 1) {
                linePtr6[ 0] = vdp->palette[(col = sprLine[0] >> 3) ? (col >> 1) & 3 : (charTable[0] >> 2) & 3];
                linePtr6[ 1] = vdp->palette[(col = sprLine[0]  & 7) ? (col >> 1) & 3 : (charTable[0] >> 0) & 3]; UPDATE_TABLE_6();
                linePtr6[ 2] = vdp->palette[(col = sprLine[1] >> 3) ? (col >> 1) & 3 : (charTable[1] >> 6) & 3];
                linePtr6[ 3] = vdp->palette[(col = sprLine[1]  & 7) ? (col >> 1) & 3 : (charTable[1] >> 4) & 3]; UPDATE_TABLE_6();
                linePtr6[ 4] = vdp->palette[(col = sprLine[2] >> 3) ? (col >> 1) & 3 : (charTable[1] >> 2) & 3];
                linePtr6[ 5] = vdp->palette[(col = sprLine[2]  & 7) ? (col >> 1) & 3 : (charTable[1] >> 0) & 3]; UPDATE_TABLE_6();
                linePtr6[ 6] = vdp->palette[(col = sprLine[3] >> 3) ? (col >> 1) & 3 : (charTable[2] >> 6) & 3];
                linePtr6[ 7] = vdp->palette[(col = sprLine[3]  & 7) ? (col >> 1) & 3 : (charTable[2] >> 4) & 3]; UPDATE_TABLE_6();
                linePtr6[ 8] = vdp->palette[(col = sprLine[4] >> 3) ? (col >> 1) & 3 : (charTable[2] >> 2) & 3];
                linePtr6[ 9] = vdp->palette[(col = sprLine[4]  & 7) ? (col >> 1) & 3 : (charTable[2] >> 0) & 3]; UPDATE_TABLE_6();
                linePtr6[10] = vdp->palette[(col = sprLine[5] >> 3) ? (col >> 1) & 3 : (charTable[3] >> 6) & 3];
                linePtr6[11] = vdp->palette[(col = sprLine[5]  & 7) ? (col >> 1) & 3 : (charTable[3] >> 4) & 3]; UPDATE_TABLE_6();
                linePtr6[12] = vdp->palette[(col = sprLine[6] >> 3) ? (col >> 1) & 3 : (charTable[3] >> 2) & 3];
                linePtr6[13] = vdp->palette[(col = sprLine[6]  & 7) ? (col >> 1) & 3 : (charTable[3] >> 0) & 3]; UPDATE_TABLE_6();
                linePtr6[14] = vdp->palette[(col = sprLine[7] >> 3) ? (col >> 1) & 3 : (charTable[4] >> 6) & 3];
                linePtr6[15] = vdp->palette[(col = sprLine[7]  & 7) ? (col >> 1) & 3 : (charTable[4] >> 4) & 3]; UPDATE_TABLE_6();
            }
            else {
                linePtr6[ 0] = vdp->palette[(col = sprLine[0] >> 3) ? (col >> 1) & 3 : (charTable[0] >> 6) & 3];
                linePtr6[ 1] = vdp->palette[(col = sprLine[0]  & 7) ? (col >> 1) & 3 : (charTable[0] >> 4) & 3]; UPDATE_TABLE_6();
                linePtr6[ 2] = vdp->palette[(col = sprLine[1] >> 3) ? (col >> 1) & 3 : (charTable[0] >> 2) & 3];
                linePtr6[ 3] = vdp->palette[(col = sprLine[1]  & 7) ? (col >> 1) & 3 : (charTable[0] >> 0) & 3]; UPDATE_TABLE_6();
                linePtr6[ 4] = vdp->palette[(col = sprLine[2] >> 3) ? (col >> 1) & 3 : (charTable[1] >> 6) & 3];
                linePtr6[ 5] = vdp->palette[(col = sprLine[2]  & 7) ? (col >> 1) & 3 : (charTable[1] >> 4) & 3]; UPDATE_TABLE_6();
                linePtr6[ 6] = vdp->palette[(col = sprLine[3] >> 3) ? (col >> 1) & 3 : (charTable[1] >> 2) & 3];
                linePtr6[ 7] = vdp->palette[(col = sprLine[3]  & 7) ? (col >> 1) & 3 : (charTable[1] >> 0) & 3]; UPDATE_TABLE_6();
                linePtr6[ 8] = vdp->palette[(col = sprLine[4] >> 3) ? (col >> 1) & 3 : (charTable[2] >> 6) & 3];
                linePtr6[ 9] = vdp->palette[(col = sprLine[4]  & 7) ? (col >> 1) & 3 : (charTable[2] >> 4) & 3]; UPDATE_TABLE_6();
                linePtr6[10] = vdp->palette[(col = sprLine[5] >> 3) ? (col >> 1) & 3 : (charTable[2] >> 2) & 3];
                linePtr6[11] = vdp->palette[(col = sprLine[5]  & 7) ? (col >> 1) & 3 : (charTable[2] >> 0) & 3]; UPDATE_TABLE_6();
                linePtr6[12] = vdp->palette[(col = sprLine[6] >> 3) ? (col >> 1) & 3 : (charTable[3] >> 6) & 3];
                linePtr6[13] = vdp->palette[(col = sprLine[6]  & 7) ? (col >> 1) & 3 : (charTable[3] >> 4) & 3]; UPDATE_TABLE_6();
                linePtr6[14] = vdp->palette[(col = sprLine[7] >> 3) ? (col >> 1) & 3 : (charTable[3] >> 2) & 3];
                linePtr6[15] = vdp->palette[(col = sprLine[7]  & 7) ? (col >> 1) & 3 : (charTable[3] >> 0) & 3]; UPDATE_TABLE_6();
            }
            sprLine += 8; 

            linePtr6 += 16; 
            charTable += 4;
            X++;
        }
    }
    if (rightBorder) {
//        colorSpritesLine(vdp, Y, 1);
        RefreshRightBorder6(vdp, Y, vdp->palette[(vdp->BGColor >> 2) & 0x03], vdp->palette[vdp->BGColor & 0x03]);
    }
}

static void RefreshLine7(VDP* vdp, int Y, int X, int X2)
{
    static UInt8*  charTable;
    static UInt8*  sprLine;
    static int     hScroll512;
    static int     page;
    static int*    jump;
    static int     scroll;
    static int     vscroll;
    static int     chrTabO;
    int col;
    int rightBorder;

    if (X == -1) {
        X++;
        linePtr7 = RefreshBorder(vdp, Y, vdp->palette[vdp->BGColor], 1, 0);
        //if(!linePtr7) return;
        sprLine = getSpritesLine(vdp, Y);
    
        if (linePtr7 == NULL) {
            return;
        }

        hScroll512 = vdpHScroll512(vdp);
        jump       = jumpTable + hScroll512 * 2;
        page    = (vdp->chrTabBase / 0x8000) & 1;
        scroll     = vdpHScroll(vdp);
        vscroll    = vdpVScroll(vdp);
        chrTabO    = vdp->chrTabBase;

        charTable = vdp->vram + (vdp->chrTabBase & (~vdpIsOddPage(vdp) << 7) & ((-1 << 15) | ((Y - vdp->firstLine + vdpVScroll(vdp)) << 7))) + scroll / 2;


        if (hScroll512) {
            if (scroll & 0x100) charTable += jump[page ^= 1];
            if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 128;
        }
    }

    if (linePtr7 == NULL || X2 <= 0) {
        return;
    }

#define BRK_7 24
    if (X < BRK_7 && X2 >= BRK_7) colorSpritesLine(vdp, Y, 0);

    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor = vdp->palette[vdp->BGColor];
        while (X < X2) {
            linePtr7[0] = bgColor;
            linePtr7[1] = bgColor;
            linePtr7[2] = bgColor;
            linePtr7[3] = bgColor;
            linePtr7[4] = bgColor;
            linePtr7[5] = bgColor;
            linePtr7[6] = bgColor;
            linePtr7[7] = bgColor;
            linePtr7[8] = bgColor;
            linePtr7[9] = bgColor;
            linePtr7[10] = bgColor;
            linePtr7[11] = bgColor;
            linePtr7[12] = bgColor;
            linePtr7[13] = bgColor;
            linePtr7[14] = bgColor;
            linePtr7[15] = bgColor;
            linePtr7 += 16; 
            X++;
        }
    }
    else {
        // Update vscroll if needed
        if (vscroll != vdpVScroll(vdp) || chrTabO != vdp->chrTabBase) {
            scroll  = vdpHScroll(vdp) + X * 8;
            page = (vdp->chrTabBase / 0x8000) & 1;
            jump    = jumpTable + hScroll512 * 2;
            vscroll = vdpVScroll(vdp);
            chrTabO  = vdp->chrTabBase;

            charTable = vdp->vram + (vdp->chrTabBase & (~vdpIsOddPage(vdp) << 7) & ((-1 << 15) | ((Y - vdp->firstLine + vdpVScroll(vdp)) << 7))) + scroll / 2;

            if (hScroll512) {
                if (scroll & 0x100) charTable += jump[page ^= 1];
                if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 128;
            }
        }

        if (X == 0 && vdpIsEdgeMasked(vdp->vdpRegs)) {
            Pixel bgColor = vdp->palette[vdp->BGColor];
            linePtr7[0]  = linePtr7[1]  = bgColor; 
            linePtr7[2]  = linePtr7[3]  = bgColor; 
            linePtr7[4]  = linePtr7[5]  = bgColor; 
            linePtr7[6]  = linePtr7[7]  = bgColor; 
            linePtr7[8]  = linePtr7[9]  = bgColor; 
            linePtr7[10] = linePtr7[11] = bgColor; 
            linePtr7[12] = linePtr7[13] = bgColor; 
            linePtr7[14] = linePtr7[15] = bgColor; 
            UPDATE_TABLE_7(); UPDATE_TABLE_7(); UPDATE_TABLE_7(); UPDATE_TABLE_7();
            UPDATE_TABLE_7(); UPDATE_TABLE_7(); UPDATE_TABLE_7(); UPDATE_TABLE_7();
            sprLine   += sprLine != NULL ? 8 : 0;
            linePtr7 += 16;
            charTable += 4;
            X++;
        }

        if (X == 0 && vdp->screenOn && vdp->drawArea) {
            Pixel bgColor = vdp->palette[vdp->BGColor];
            int i = scroll & 7;
            int j;

            for (j = 0; j < i; j++) {
                if ((j ^ i) & 1) charTable++;
                linePtr7[0] = bgColor;
                linePtr7[1] = bgColor;
                UPDATE_TABLE_7();
                sprLine++;
                linePtr7 += 2;
            }
            
            for (;j < 8; j++) {
                if ((j ^ i) & 1) {
                    (col = sprLine[1]) ? linePtr7[0]  = linePtr7[1]  = vdp->palette[col >> 1] : 
                    (col = charTable[vdp->vram128], 
                    linePtr7[0]  = vdp->palette[col >> 4],
                    linePtr7[1]  = vdp->palette[col & 0xf]);
                    UPDATE_TABLE_7();
                    charTable++;
                }
                else {
                    (col = sprLine[0]) ? linePtr7[0]  = linePtr7[1]  = vdp->palette[col >> 1] : 
                    (col = charTable[0],      
                    linePtr7[0]  = vdp->palette[col >> 4],
                    linePtr7[1]  = vdp->palette[col & 0xf]);
                    UPDATE_TABLE_7();
                }
                sprLine++;
                linePtr7 += 2;
            } 
//            charTable += 4;
            X++;
        }
        
        while (X < X2) {
            if (scroll & 1) {
                (col = sprLine[0]) ? linePtr7[0]  = linePtr7[1]  = vdp->palette[col >> 1] : 
                (col = charTable[vdp->vram128], 
                linePtr7[0]  = vdp->palette[col >> 4],
                linePtr7[1]  = vdp->palette[col & 0xf]);
                UPDATE_TABLE_7();
                (col = sprLine[1]) ? linePtr7[2]  = linePtr7[3]  = vdp->palette[col >> 1] :
                (col = charTable[1],  
                linePtr7[2]  = vdp->palette[col >> 4],
                linePtr7[3]  = vdp->palette[col & 0xf]); 
                UPDATE_TABLE_7();
                (col = sprLine[2]) ? linePtr7[4]  = linePtr7[5]  = vdp->palette[col >> 1] : 
                (col = charTable[vdp->vram128|1],
                linePtr7[4]  = vdp->palette[col >> 4],
                linePtr7[5]  = vdp->palette[col & 0xf]);
                UPDATE_TABLE_7();
                (col = sprLine[3]) ? linePtr7[6]  = linePtr7[7]  = vdp->palette[col >> 1] : 
                (col = charTable[2],   
                linePtr7[6]  = vdp->palette[col >> 4],
                linePtr7[7]  = vdp->palette[col & 0xf]); 
                UPDATE_TABLE_7();
                (col = sprLine[4]) ? linePtr7[8]  = linePtr7[9]  = vdp->palette[col >> 1] :
                (col = charTable[vdp->vram128|2], 
                linePtr7[8]  = vdp->palette[col >> 4], 
                linePtr7[9]  = vdp->palette[col & 0xf]);
                UPDATE_TABLE_7();
                (col = sprLine[5]) ? linePtr7[10] = linePtr7[11] = vdp->palette[col >> 1] : 
                (col = charTable[3],      
                linePtr7[10] = vdp->palette[col >> 4],
                linePtr7[11] = vdp->palette[col & 0xf]); 
                UPDATE_TABLE_7();
                (col = sprLine[6]) ? linePtr7[12] = linePtr7[13] = vdp->palette[col >> 1] : 
                (col = charTable[vdp->vram128|3], 
                linePtr7[12] = vdp->palette[col >> 4], 
                linePtr7[13] = vdp->palette[col & 0xf]);
                UPDATE_TABLE_7();
                (col = sprLine[7]) ? linePtr7[14] = linePtr7[15] = vdp->palette[col >> 1] : 
                (col = charTable[4],  
                linePtr7[14] = vdp->palette[col >> 4], 
                linePtr7[15] = vdp->palette[col & 0xf]); 
                UPDATE_TABLE_7();
            }
            else {
                (col = sprLine[0]) ? linePtr7[0]  = linePtr7[1]  = vdp->palette[col >> 1] : 
                (col = charTable[0],      
                linePtr7[0]  = vdp->palette[col >> 4],
                linePtr7[1]  = vdp->palette[col & 0xf]);
                UPDATE_TABLE_7();
                (col = sprLine[1]) ? linePtr7[2]  = linePtr7[3]  = vdp->palette[col >> 1] : 
                (col = charTable[vdp->vram128], 
                linePtr7[2]  = vdp->palette[col >> 4],
                linePtr7[3]  = vdp->palette[col & 0xf]);
                UPDATE_TABLE_7();
                (col = sprLine[2]) ? linePtr7[4]  = linePtr7[5]  = vdp->palette[col >> 1] :
                (col = charTable[1],    
                linePtr7[4]  = vdp->palette[col >> 4], 
                linePtr7[5]  = vdp->palette[col & 0xf]);
                UPDATE_TABLE_7();
                (col = sprLine[3]) ? linePtr7[6]  = linePtr7[7]  = vdp->palette[col >> 1] :
                (col = charTable[vdp->vram128|1],
                linePtr7[6]  = vdp->palette[col >> 4], 
                linePtr7[7]  = vdp->palette[col & 0xf]);
                UPDATE_TABLE_7();
                (col = sprLine[4]) ? linePtr7[8]  = linePtr7[9]  = vdp->palette[col >> 1] : 
                (col = charTable[2],      
                linePtr7[8]  = vdp->palette[col >> 4], 
                linePtr7[9]  = vdp->palette[col & 0xf]); 
                UPDATE_TABLE_7();
                (col = sprLine[5]) ? linePtr7[10] = linePtr7[11] = vdp->palette[col >> 1] :
                (col = charTable[vdp->vram128|2], 
                linePtr7[10] = vdp->palette[col >> 4], 
                linePtr7[11] = vdp->palette[col & 0xf]); 
                UPDATE_TABLE_7();
                (col = sprLine[6]) ? linePtr7[12] = linePtr7[13] = vdp->palette[col >> 1] : 
                (col = charTable[3],   
                linePtr7[12] = vdp->palette[col >> 4], 
                linePtr7[13] = vdp->palette[col & 0xf]); 
                UPDATE_TABLE_7();
                (col = sprLine[7]) ? linePtr7[14] = linePtr7[15] = vdp->palette[col >> 1] : 
                (col = charTable[vdp->vram128|3], 
                linePtr7[14] = vdp->palette[col >> 4], 
                linePtr7[15] = vdp->palette[col & 0xf]); 
                UPDATE_TABLE_7();
            }
            sprLine += 8; 

            linePtr7 += 16; 
            charTable += 4;
            X++;
        }
    }    
    if (rightBorder) {
//        colorSpritesLine(vdp, Y, 0);
        RefreshRightBorder(vdp, Y, vdp->palette[vdp->BGColor], 1, 0);
    }
}

#endif

static void RefreshLine8(VDP* vdp, int Y, int X, int X2)
{
    static UInt8*  charTable;
    static UInt8*  sprLine;
    static int     hScroll;
    static int     hScroll512;
    static int*    jump;
    static int     page;
    static int     scroll;
    static int     vscroll;
    static int     chrTabO;
    int col;
    int rightBorder;

    if (X == -1) {
        X++;
        linePtr8 = RefreshBorder(vdp, Y, vdp->paletteFixed[vdp->vdpRegs[7]], 0, 0);
        //if(!linePtr8) return;
        sprLine = getSpritesLine(vdp, Y);

        if (linePtr8 == NULL) {
            return;
        }

        hScroll    = vdpHScroll(vdp);
        hScroll512 = vdpHScroll512(vdp);
        jump       = jumpTable + hScroll512 * 2;
        page    = (vdp->chrTabBase / 0x8000) & 1;
        scroll     = hScroll;
        vscroll    = vdpVScroll(vdp);
        chrTabO    = vdp->chrTabBase;

        charTable = vdp->vram + (vdp->chrTabBase & (~vdpIsOddPage(vdp) << 7) & ((-1 << 15) | ((Y - vdp->firstLine + vdpVScroll(vdp)) << 7))) + scroll / 2;

        if (hScroll512) {
            if (scroll & 0x100) charTable += jump[page ^= 1];
            if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 128;
        }
    }

    if (linePtr8 == NULL || X2 <= 0) {
        return;
    }

#define BRK_8 24
    if (X < BRK_8 && X2 >= BRK_8) colorSpritesLine(vdp, Y, 0);

    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor = vdp->paletteFixed[vdp->vdpRegs[7]];
        while (X < X2) {
            linePtr8[0] = bgColor;
            linePtr8[1] = bgColor;
            linePtr8[2] = bgColor;
            linePtr8[3] = bgColor;
            linePtr8[4] = bgColor;
            linePtr8[5] = bgColor;
            linePtr8[6] = bgColor;
            linePtr8[7] = bgColor;
            linePtr8 += 8; 
            X++;
        }
    }
    else {
        // Update vscroll if needed
        if (vscroll != vdpVScroll(vdp) || chrTabO != vdp->chrTabBase) {
            scroll = vdpHScroll(vdp) + X * 8;
            jump   = jumpTable + hScroll512 * 2;
            vscroll = vdpVScroll(vdp);
            chrTabO = vdp->chrTabBase;

            charTable = vdp->vram + (vdp->chrTabBase & (~vdpIsOddPage(vdp) << 7) & ((-1 << 15) | ((Y - vdp->firstLine + vdpVScroll(vdp)) << 7))) + scroll / 2;

            if (hScroll512) {
                if (scroll & 0x100) charTable += jump[page ^= 1];
                if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 128;
            }
        }

        if (X == 0 && vdpIsEdgeMasked(vdp->vdpRegs)) {
            Pixel bgColor = vdp->paletteFixed[vdp->vdpRegs[7]];
            linePtr8[0] = bgColor;
            linePtr8[1] = bgColor;
            linePtr8[2] = bgColor;
            linePtr8[3] = bgColor;
            linePtr8[4] = bgColor;
            linePtr8[5] = bgColor;
            linePtr8[6] = bgColor;
            linePtr8[7] = bgColor;
            UPDATE_TABLE_8(); UPDATE_TABLE_8(); UPDATE_TABLE_8(); UPDATE_TABLE_8();
            UPDATE_TABLE_8(); UPDATE_TABLE_8(); UPDATE_TABLE_8(); UPDATE_TABLE_8();
            sprLine   += sprLine != NULL ? 8 : 0; 
            charTable += 4;
            linePtr8 += 8; 
            X++; 
        }

        while (X < X2) {
            if (scroll & 1) {
                col = sprLine[0]; linePtr8[0] = col ? vdp->paletteSprite8[col >> 1] : 
                vdp->paletteFixed[charTable[vdp->vram128]]; UPDATE_TABLE_8();
                col = sprLine[1]; linePtr8[1] = col ? vdp->paletteSprite8[col >> 1] : 
                vdp->paletteFixed[charTable[1]]; UPDATE_TABLE_8();
                col = sprLine[2]; linePtr8[2] = col ? vdp->paletteSprite8[col >> 1] : 
                vdp->paletteFixed[charTable[vdp->vram128|1]]; UPDATE_TABLE_8();
                col = sprLine[3]; linePtr8[3] = col ? vdp->paletteSprite8[col >> 1] : 
                vdp->paletteFixed[charTable[2]]; UPDATE_TABLE_8();
                col = sprLine[4]; linePtr8[4] = col ? vdp->paletteSprite8[col >> 1] : 
                vdp->paletteFixed[charTable[vdp->vram128|2]]; UPDATE_TABLE_8();
                col = sprLine[5]; linePtr8[5] = col ? vdp->paletteSprite8[col >> 1] : 
                vdp->paletteFixed[charTable[3]]; UPDATE_TABLE_8();
                col = sprLine[6]; linePtr8[6] = col ? vdp->paletteSprite8[col >> 1] : 
                vdp->paletteFixed[charTable[vdp->vram128|3]]; UPDATE_TABLE_8();
                col = sprLine[7]; linePtr8[7] = col ? vdp->paletteSprite8[col >> 1] : 
                vdp->paletteFixed[charTable[4]]; UPDATE_TABLE_8();
            }
            else {
                col = sprLine[0]; linePtr8[0] = col ? vdp->paletteSprite8[col >> 1] : 
                vdp->paletteFixed[charTable[0]]; UPDATE_TABLE_8();
                col = sprLine[1]; linePtr8[1] = col ? vdp->paletteSprite8[col >> 1] : 
                vdp->paletteFixed[charTable[vdp->vram128]]; UPDATE_TABLE_8();
                col = sprLine[2]; linePtr8[2] = col ? vdp->paletteSprite8[col >> 1] : 
                vdp->paletteFixed[charTable[1]]; UPDATE_TABLE_8();
                col = sprLine[3]; linePtr8[3] = col ? vdp->paletteSprite8[col >> 1] : 
                vdp->paletteFixed[charTable[vdp->vram128|1]]; UPDATE_TABLE_8();
                col = sprLine[4]; linePtr8[4] = col ? vdp->paletteSprite8[col >> 1] : 
                vdp->paletteFixed[charTable[2]]; UPDATE_TABLE_8();
                col = sprLine[5]; linePtr8[5] = col ? vdp->paletteSprite8[col >> 1] : 
                vdp->paletteFixed[charTable[vdp->vram128|2]]; UPDATE_TABLE_8();
                col = sprLine[6]; linePtr8[6] = col ? vdp->paletteSprite8[col >> 1] : 
                vdp->paletteFixed[charTable[3]]; UPDATE_TABLE_8();
                col = sprLine[7]; linePtr8[7] = col ? vdp->paletteSprite8[col >> 1] : 
                vdp->paletteFixed[charTable[vdp->vram128|3]]; UPDATE_TABLE_8();
            }
            sprLine += 8; 

            charTable += 4; linePtr8 += 8; X++;
        }
    }

    if (rightBorder) {
//        colorSpritesLine(vdp, Y, 0);
        RefreshRightBorder(vdp, Y, vdp->paletteFixed[vdp->vdpRegs[7]], 0, 0);
    }
}

static void RefreshLine10(VDP* vdp, int Y, int X, int X2)
{
    static UInt8* charTable;
    static UInt8* sprLine;
    static int hScroll512;
    static int* jump;
    static int page;
    static int scroll;
    static int vscroll;
    static int hscroll;
    static int chrTabO;
    int col;
    UInt8 t0, t1, t2, t3;
    int y, J, K;
    int rightBorder;

    if (X == -1) {
        X++;
        linePtr10 = RefreshBorder(vdp, Y, vdp->paletteFixed[vdp->vdpRegs[7]], 0, 0);
        //if(!linePtr10) return;
        sprLine = getSpritesLine(vdp, Y);

        if (linePtr10 == NULL) {
            return;
        }

        hScroll512 = vdpHScroll512(vdp);
        jump       = jumpTable + hScroll512 * 2;
        page    = (vdp->chrTabBase / 0x8000) & 1;
        hscroll    = vdpHScroll(vdp);
        scroll     = hscroll & ~3;
        vscroll    = vdpVScroll(vdp);
        chrTabO    = vdp->chrTabBase;

        charTable = vdp->vram + (vdp->chrTabBase & (~vdpIsOddPage(vdp) << 7) & ((-1 << 15) | ((Y - vdp->firstLine + vdpVScroll(vdp)) << 7))) + scroll / 2;

        if (hScroll512) {
            if (scroll & 0x100) charTable += jump[page ^= 1];
            if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 128;
        }
    }

    if (linePtr10 == NULL || X2 <= 0) {
        return;
    }

#define BRK_10 24
    if (X < BRK_10 && X2 >= BRK_10) colorSpritesLine(vdp, Y, 0);

rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor = vdp->paletteFixed[vdp->vdpRegs[7]];
        while (X < X2) {
            linePtr10[0] = bgColor;
            linePtr10[1] = bgColor;
            linePtr10[2] = bgColor;
            linePtr10[3] = bgColor;
            linePtr10[4] = bgColor;
            linePtr10[5] = bgColor;
            linePtr10[6] = bgColor;
            linePtr10[7] = bgColor;
            linePtr10 += 8; 
            X++;
        }
    }
    else {
        // Update vscroll if needed
        if (vscroll != vdpVScroll(vdp) || chrTabO != vdp->chrTabBase) {
            hscroll = vdpHScroll(vdp);
            scroll = (hscroll & ~3) + X * 8;
            jump   = jumpTable + hScroll512 * 2;
            vscroll = vdpVScroll(vdp);
            chrTabO  = vdp->chrTabBase;

            charTable = vdp->vram + (vdp->chrTabBase & (~vdpIsOddPage(vdp) << 7) & ((-1 << 15) | ((Y - vdp->firstLine + vdpVScroll(vdp)) << 7))) + scroll / 2;
            charTable += 2; 

            if (hScroll512) {
                if (scroll & 0x100) charTable += jump[page ^= 1];
                if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 128;
            }
        }

        if (X == 0) {
            if (vdpIsEdgeMasked(vdp->vdpRegs)) {
                Pixel bgColor = vdp->paletteFixed[vdp->vdpRegs[7]];
                linePtr10[0] = bgColor;
                linePtr10[1] = bgColor;
                linePtr10[2] = bgColor;
                linePtr10[3] = bgColor;
                linePtr10[4] = bgColor;
                linePtr10[5] = bgColor;
                linePtr10[6] = bgColor;
                linePtr10[7] = bgColor;
                UPDATE_TABLE_10(); UPDATE_TABLE_10(); UPDATE_TABLE_10(); UPDATE_TABLE_10();
                UPDATE_TABLE_10(); UPDATE_TABLE_10(); UPDATE_TABLE_10(); UPDATE_TABLE_10();
                sprLine   += sprLine != NULL ? 8 : 0; 
                charTable += 4; 
                linePtr10 += 8;
                X++; 
            }

            t0 = charTable[0];              UPDATE_TABLE_10();
            t1 = charTable[vdp->vram128];   UPDATE_TABLE_10();
            t2 = charTable[1];              UPDATE_TABLE_10();
            t3 = charTable[vdp->vram128|1]; UPDATE_TABLE_10();


            K=(t0 & 0x07) | ((t1 & 0x07) << 3);
            J=(t2 & 0x07) | ((t3 & 0x07) << 3);

            if (vdp->screenOn && vdp->drawArea) {
                switch (hscroll & 3) {
                case 0:
                    col = sprLine[0]; y = t0 >> 3; *linePtr10++ = col ? vdp->palette[col >> 1] : y & 1 ? vdp->palette[y >> 1] : vdp->yjkColor[y][J][K];
                case 1:
                    col = sprLine[1]; y = t1 >> 3; *linePtr10++ = col ? vdp->palette[col >> 1] : y & 1 ? vdp->palette[y >> 1] : vdp->yjkColor[y][J][K];
                case 2:
                    col = sprLine[2]; y = t2 >> 3; *linePtr10++ = col ? vdp->palette[col >> 1] : y & 1 ? vdp->palette[y >> 1] : vdp->yjkColor[y][J][K];
                case 3:
                    col = sprLine[3]; y = t3 >> 3; *linePtr10++ = col ? vdp->palette[col >> 1] : y & 1 ? vdp->palette[y >> 1] : vdp->yjkColor[y][J][K];
                }
                sprLine += 4;
                charTable += 2;
            }
        }
        while (X < X2) {
            t0 = charTable[0];              UPDATE_TABLE_10();
            t1 = charTable[vdp->vram128];   UPDATE_TABLE_10();
            t2 = charTable[1];              UPDATE_TABLE_10();
            t3 = charTable[vdp->vram128|1]; UPDATE_TABLE_10();

            K=(t0 & 0x07) | ((t1 & 0x07) << 3);
            J=(t2 & 0x07) | ((t3 & 0x07) << 3);

            col = sprLine[0]; y = t0 >> 3; linePtr10[0] = col ? vdp->palette[col >> 1] : y & 1 ? vdp->palette[y >> 1] : vdp->yjkColor[y][J][K];
            col = sprLine[1]; y = t1 >> 3; linePtr10[1] = col ? vdp->palette[col >> 1] : y & 1 ? vdp->palette[y >> 1] : vdp->yjkColor[y][J][K];
            col = sprLine[2]; y = t2 >> 3; linePtr10[2] = col ? vdp->palette[col >> 1] : y & 1 ? vdp->palette[y >> 1] : vdp->yjkColor[y][J][K];
            col = sprLine[3]; y = t3 >> 3; linePtr10[3] = col ? vdp->palette[col >> 1] : y & 1 ? vdp->palette[y >> 1] : vdp->yjkColor[y][J][K];

            t0 = charTable[2];        UPDATE_TABLE_10();
            t1 = charTable[vdp->vram128|2];  UPDATE_TABLE_10();
            t2 = charTable[3];        UPDATE_TABLE_10();
            t3 = charTable[vdp->vram128|3];  UPDATE_TABLE_10();

            K=(t0 & 0x07) | ((t1 & 0x07) << 3);
            J=(t2 & 0x07) | ((t3 & 0x07) << 3);

            if (X == 31) {
                switch (hscroll & 3) {
                case 1:
                    y = t2 >> 3; col = sprLine[6]; linePtr10[6] = col ? vdp->palette[col >> 1] : y & 1 ? vdp->palette[y >> 1] : vdp->yjkColor[y][J][K];
                case 2:
                    y = t1 >> 3; col = sprLine[5]; linePtr10[5] = col ? vdp->palette[col >> 1] : y & 1 ? vdp->palette[y >> 1] : vdp->yjkColor[y][J][K];
                case 3:
                    y = t0 >> 3; col = sprLine[4]; linePtr10[4] = col ? vdp->palette[col >> 1] : y & 1 ? vdp->palette[y >> 1] : vdp->yjkColor[y][J][K];
                }
            }
            else {
                col = sprLine[4]; y = t0 >> 3; linePtr10[4] = col ? vdp->palette[col >> 1] : y & 1 ? vdp->palette[y >> 1] : vdp->yjkColor[y][J][K];
                col = sprLine[5]; y = t1 >> 3; linePtr10[5] = col ? vdp->palette[col >> 1] : y & 1 ? vdp->palette[y >> 1] : vdp->yjkColor[y][J][K];
                col = sprLine[6]; y = t2 >> 3; linePtr10[6] = col ? vdp->palette[col >> 1] : y & 1 ? vdp->palette[y >> 1] : vdp->yjkColor[y][J][K];
                col = sprLine[7]; y = t3 >> 3; linePtr10[7] = col ? vdp->palette[col >> 1] : y & 1 ? vdp->palette[y >> 1] : vdp->yjkColor[y][J][K];
                sprLine += 8; 
            }

            charTable += 4; linePtr10 += 8;
            X++;
        }
    }

    if (rightBorder) {
//        colorSpritesLine(vdp, Y, 0);
        RefreshRightBorder(vdp, Y, vdp->paletteFixed[vdp->vdpRegs[7]], 0, 0);
    }
}

static void RefreshLine12(VDP* vdp, int Y, int X, int X2)
{
    static UInt8* charTable;
    static UInt8* sprLine;
    static int hScroll512;
    static int* jump;
    static int page;
    static int scroll;
    static int vscroll;
    static int hscroll;
    static int chrTabO;
    int col;
    UInt8 t0, t1, t2, t3;
    int J, K;
    int rightBorder;

    if (X == -1) {
        X++;
        linePtr12 = RefreshBorder(vdp, Y, vdp->paletteFixed[vdp->vdpRegs[7]], 0, 0);
        //if(!linePtr12) return;
        sprLine = getSpritesLine(vdp, Y);

        if (linePtr12 == NULL) {
            return;
        }

        hScroll512 = vdpHScroll512(vdp);
        jump       = jumpTable + hScroll512 * 2;
        page    = (vdp->chrTabBase / 0x8000) & 1;
        hscroll    = vdpHScroll(vdp);
        scroll     = hscroll & ~3;
        vscroll    = vdpVScroll(vdp);
        chrTabO    = vdp->chrTabBase;

        charTable = vdp->vram + (vdp->chrTabBase & (~vdpIsOddPage(vdp) << 7) & ((-1 << 15) | ((Y - vdp->firstLine + vdpVScroll(vdp)) << 7))) + scroll / 2;

        if (hScroll512) {
            if (scroll & 0x100) charTable += jump[page ^= 1];
            if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 128;
        }
    }

    if (linePtr12 == NULL || X2 <= 0) {
        return;
    }

#define BRK_12 24
    if (X < BRK_12 && X2 >= BRK_12) colorSpritesLine(vdp, Y, 0);

    rightBorder = X2 == 33;
    if (rightBorder) {
        X2--;
    }

    if (!vdp->screenOn || !vdp->drawArea) {
        Pixel bgColor = vdp->paletteFixed[vdp->vdpRegs[7]];
        while (X < X2) {
            linePtr12[0] = bgColor;
            linePtr12[1] = bgColor;
            linePtr12[2] = bgColor;
            linePtr12[3] = bgColor;
            linePtr12[4] = bgColor;
            linePtr12[5] = bgColor;
            linePtr12[6] = bgColor;
            linePtr12[7] = bgColor;
            linePtr12 += 8; 
            X++;
        }
    }
    else {
        // Update vscroll if needed
        if (vscroll != vdpVScroll(vdp) || chrTabO != vdp->chrTabBase) {
            hscroll = vdpHScroll(vdp);
            scroll = (hscroll & ~3) + X * 8;
            jump   = jumpTable + hScroll512 * 2;
            vscroll = vdpVScroll(vdp);
            chrTabO  = vdp->chrTabBase;

            charTable = vdp->vram + (vdp->chrTabBase & (~vdpIsOddPage(vdp) << 7) & ((-1 << 15) | ((Y - vdp->firstLine + vdpVScroll(vdp)) << 7))) + scroll / 2;
            charTable += 2; 

            if (hScroll512) {
                if (scroll & 0x100) charTable += jump[page ^= 1];
                if (vdp->chrTabBase & (1 << 15)) charTable += jump[page ^= 1] + 128;
            }
        }

        if (X == 0) {
            if (vdpIsEdgeMasked(vdp->vdpRegs)) {
                Pixel bgColor = vdp->paletteFixed[vdp->vdpRegs[7]];
                linePtr12[0] = bgColor;
                linePtr12[1] = bgColor;
                linePtr12[2] = bgColor;
                linePtr12[3] = bgColor;
                linePtr12[4] = bgColor;
                linePtr12[5] = bgColor;
                linePtr12[6] = bgColor;
                linePtr12[7] = bgColor;
                UPDATE_TABLE_12(); UPDATE_TABLE_12(); UPDATE_TABLE_12(); UPDATE_TABLE_12();
                UPDATE_TABLE_12(); UPDATE_TABLE_12(); UPDATE_TABLE_12(); UPDATE_TABLE_12();
                sprLine   += sprLine != NULL ? 8 : 0; 
                charTable += 4; 
                linePtr12 += 8;
                X++; 
            }

            t0 = charTable[0];              UPDATE_TABLE_12();
            t1 = charTable[vdp->vram128];   UPDATE_TABLE_12();
            t2 = charTable[1];              UPDATE_TABLE_12();
            t3 = charTable[vdp->vram128|1]; UPDATE_TABLE_12();

            K=(t0 & 0x07) | ((t1 & 0x07) << 3);
            J=(t2 & 0x07) | ((t3 & 0x07) << 3);

            if (vdp->screenOn && vdp->drawArea) {
                switch (hscroll & 3) {
                case 0:
                    col = sprLine[0]; *linePtr12++ = col ? vdp->palette[col >> 1] : vdp->yjkColor[t0 >> 3][J][K];
                case 1:
                    col = sprLine[1]; *linePtr12++ = col ? vdp->palette[col >> 1] : vdp->yjkColor[t1 >> 3][J][K];
                case 2:
                    col = sprLine[2]; *linePtr12++ = col ? vdp->palette[col >> 1] : vdp->yjkColor[t2 >> 3][J][K];
                case 3:
                    col = sprLine[3]; *linePtr12++ = col ? vdp->palette[col >> 1] : vdp->yjkColor[t3 >> 3][J][K];
                }
                sprLine += 4;
                charTable += 2;
            }
        }

        while (X < X2) {
            t0 = charTable[0];         UPDATE_TABLE_12();
            t1 = charTable[vdp->vram128];   UPDATE_TABLE_12();
            t2 = charTable[1];         UPDATE_TABLE_12();
            t3 = charTable[vdp->vram128|1]; UPDATE_TABLE_12();

            K=(t0 & 0x07) | ((t1 & 0x07) << 3);
            J=(t2 & 0x07) | ((t3 & 0x07) << 3);

            col = sprLine[0]; linePtr12[0] = col ? vdp->palette[col >> 1] : vdp->yjkColor[t0 >> 3][J][K];
            col = sprLine[1]; linePtr12[1] = col ? vdp->palette[col >> 1] : vdp->yjkColor[t1 >> 3][J][K];
            col = sprLine[2]; linePtr12[2] = col ? vdp->palette[col >> 1] : vdp->yjkColor[t2 >> 3][J][K];
            col = sprLine[3]; linePtr12[3] = col ? vdp->palette[col >> 1] : vdp->yjkColor[t3 >> 3][J][K];

            t0 = charTable[2];        UPDATE_TABLE_12();
            t1 = charTable[vdp->vram128|2];  UPDATE_TABLE_12();
            t2 = charTable[3];        UPDATE_TABLE_12();
            t3 = charTable[vdp->vram128|3];  UPDATE_TABLE_12();

            K=(t0 & 0x07) | ((t1 & 0x07) << 3);
            J=(t2 & 0x07) | ((t3 & 0x07) << 3);

            if (X == 31) {
                switch (hscroll & 3) {
                case 1:
                    col = sprLine[6]; linePtr12[6] = col ? vdp->palette[col >> 1] : vdp->yjkColor[t2 >> 3][J][K];
                case 2:
                    col = sprLine[5]; linePtr12[5] = col ? vdp->palette[col >> 1] : vdp->yjkColor[t1 >> 3][J][K];
                case 3:
                    col = sprLine[4]; linePtr12[4] = col ? vdp->palette[col >> 1] : vdp->yjkColor[t0 >> 3][J][K];
                }
            }
            else {
                col = sprLine[4]; linePtr12[4] = col ? vdp->palette[col >> 1] : vdp->yjkColor[t0 >> 3][J][K];
                col = sprLine[5]; linePtr12[5] = col ? vdp->palette[col >> 1] : vdp->yjkColor[t1 >> 3][J][K];
                col = sprLine[6]; linePtr12[6] = col ? vdp->palette[col >> 1] : vdp->yjkColor[t2 >> 3][J][K];
                col = sprLine[7]; linePtr12[7] = col ? vdp->palette[col >> 1] : vdp->yjkColor[t3 >> 3][J][K];
                sprLine += 8; 
            }

            charTable += 4; linePtr12 += 8;
            X++;
        }
    }

    if (rightBorder) {
//        colorSpritesLine(vdp, Y, 0);
        RefreshRightBorder(vdp, Y, vdp->paletteFixed[vdp->vdpRegs[7]], 0, 0);
    }
}
