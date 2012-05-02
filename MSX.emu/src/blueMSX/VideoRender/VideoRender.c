/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/VideoRender/VideoRender.c,v $
**
** $Revision: 1.36 $
**
** $Date: 2007-05-22 06:23:18 $
** $Date: 2007-05-22 06:23:18 $
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
#include "VideoRender.h"
#include "FrameBuffer.h"
#include "ArchTimer.h"
#include "Emulator.h"
#include "Scalebit.h"
#include "hq2x.h"
#include "hq3x.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
 
#ifdef WII
static UInt16 empty_line_buffer[FB_MAX_LINE_WIDTH];

static int videoRender240(Video* pVideo, FrameBuffer* frame, int bitDepth, int zoom,
                          void* pDst, int dstOffset, int dstPitch, int canChangeZoom)
{
    UInt16* pDst1;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    //dstOffset = (dstOffset & ~3) << 2;
    pDst = (char*)pDst;// + zoom * dstOffset;
    pDst1 = (UInt16*)pDst;

    dstPitch /= (int)sizeof(UInt16);

    for (h = 0; h < height; h+=2) {
        UInt16* pDst1old = pDst1;
        UInt16* pSrc1;
        UInt16* pSrc2;
        UInt16* pSrc3;
        UInt16* pSrc4;
        if (frame->interlace != INTERLACE_ODD) {
            pSrc1 = frame->line[h].buffer;
            pSrc2 = frame->line[h].buffer;
            pSrc3 = frame->line[h+1].buffer;
            pSrc4 = frame->line[h+1].buffer;
        }else{
            pSrc1 = h? frame->line[h-1].buffer : empty_line_buffer;
            pSrc2 = frame->line[h].buffer;
            pSrc3 = frame->line[h].buffer;
            pSrc4 = frame->line[h+1].buffer;
        }

        if (frame->line[h].doubleWidth) {
            int width = srcWidth / 4 * 2;
            while (width--) {
                *pDst1++ = *pSrc1++;
                *pDst1++ = *pSrc1++;
                *pDst1++ = *pSrc1++;
                *pDst1++ = *pSrc1++;
                *pDst1++ = *pSrc2++;
                *pDst1++ = *pSrc2++;
                *pDst1++ = *pSrc2++;
                *pDst1++ = *pSrc2++;
                *pDst1++ = *pSrc3++;
                *pDst1++ = *pSrc3++;
                *pDst1++ = *pSrc3++;
                *pDst1++ = *pSrc3++;
                *pDst1++ = *pSrc4++;
                *pDst1++ = *pSrc4++;
                *pDst1++ = *pSrc4++;
                *pDst1++ = *pSrc4++;
            }
        }
        else {
            int width = srcWidth / 4;
            while (width--) {
                *pDst1++ = *pSrc1;
                *pDst1++ = *pSrc1++;
                *pDst1++ = *pSrc1;
                *pDst1++ = *pSrc1++;
                *pDst1++ = *pSrc2;
                *pDst1++ = *pSrc2++;
                *pDst1++ = *pSrc2;
                *pDst1++ = *pSrc2++;
                *pDst1++ = *pSrc3;
                *pDst1++ = *pSrc3++;
                *pDst1++ = *pSrc3;
                *pDst1++ = *pSrc3++;
                *pDst1++ = *pSrc4;
                *pDst1++ = *pSrc4++;
                *pDst1++ = *pSrc4;
                *pDst1++ = *pSrc4++;

                *pDst1++ = *pSrc1;
                *pDst1++ = *pSrc1++;
                *pDst1++ = *pSrc1;
                *pDst1++ = *pSrc1++;
                *pDst1++ = *pSrc2;
                *pDst1++ = *pSrc2++;
                *pDst1++ = *pSrc2;
                *pDst1++ = *pSrc2++;
                *pDst1++ = *pSrc3;
                *pDst1++ = *pSrc3++;
                *pDst1++ = *pSrc3;
                *pDst1++ = *pSrc3++;
                *pDst1++ = *pSrc4;
                *pDst1++ = *pSrc4++;
                *pDst1++ = *pSrc4;
                *pDst1++ = *pSrc4++;
            }
        }

        pDst1 = pDst1old + dstPitch * 4;
    }
    return zoom;
}

static int videoRender480(Video* pVideo, FrameBuffer* frame, int bitDepth, int zoom,
                          void* pDst, int dstOffset, int dstPitch, int canChangeZoom)
{
    UInt16* pDst1;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    //dstOffset = (dstOffset & ~3) << 2;
    pDst = (char*)pDst;// + zoom * dstOffset;
    pDst1 = (UInt16*)pDst;

    dstPitch /= (int)sizeof(UInt16);

    for (h = 0; h < height; h+=4) {
        UInt16* pDst1old = pDst1;
        UInt16* pSrc1 = frame->line[h].buffer;
        UInt16* pSrc2 = frame->line[h+1].buffer;
        UInt16* pSrc3 = frame->line[h+2].buffer;
        UInt16* pSrc4 = frame->line[h+3].buffer;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth / 4 * 2;
            while (width--) {
                *pDst1++ = *pSrc1++;
                *pDst1++ = *pSrc1++;
                *pDst1++ = *pSrc1++;
                *pDst1++ = *pSrc1++;
                *pDst1++ = *pSrc2++;
                *pDst1++ = *pSrc2++;
                *pDst1++ = *pSrc2++;
                *pDst1++ = *pSrc2++;
                *pDst1++ = *pSrc3++;
                *pDst1++ = *pSrc3++;
                *pDst1++ = *pSrc3++;
                *pDst1++ = *pSrc3++;
                *pDst1++ = *pSrc4++;
                *pDst1++ = *pSrc4++;
                *pDst1++ = *pSrc4++;
                *pDst1++ = *pSrc4++;
            }
        }
        else {
            int width = srcWidth / 4;
            while (width--) {
                *pDst1++ = *pSrc1;
                *pDst1++ = *pSrc1++;
                *pDst1++ = *pSrc1;
                *pDst1++ = *pSrc1++;
                *pDst1++ = *pSrc2;
                *pDst1++ = *pSrc2++;
                *pDst1++ = *pSrc2;
                *pDst1++ = *pSrc2++;
                *pDst1++ = *pSrc3;
                *pDst1++ = *pSrc3++;
                *pDst1++ = *pSrc3;
                *pDst1++ = *pSrc3++;
                *pDst1++ = *pSrc4;
                *pDst1++ = *pSrc4++;
                *pDst1++ = *pSrc4;
                *pDst1++ = *pSrc4++;

                *pDst1++ = *pSrc1;
                *pDst1++ = *pSrc1++;
                *pDst1++ = *pSrc1;
                *pDst1++ = *pSrc1++;
                *pDst1++ = *pSrc2;
                *pDst1++ = *pSrc2++;
                *pDst1++ = *pSrc2;
                *pDst1++ = *pSrc2++;
                *pDst1++ = *pSrc3;
                *pDst1++ = *pSrc3++;
                *pDst1++ = *pSrc3;
                *pDst1++ = *pSrc3++;
                *pDst1++ = *pSrc4;
                *pDst1++ = *pSrc4++;
                *pDst1++ = *pSrc4;
                *pDst1++ = *pSrc4++;
            }
        }

        pDst1 = pDst1old + dstPitch * 4;
    }
    return zoom;
}

/*****************************************************************************
**
** Public interface methods
**
******************************************************************************
*/

int videoRender(Video* pVideo, FrameBuffer* frame, int bitDepth, int zoom,
                void* pDst, int dstOffset, int dstPitch, int canChangeZoom)
{
    if (frame == NULL) {
        return zoom;
    }

    if (frame->interlace != INTERLACE_NONE && pVideo->deInterlace) {
        frame = frameBufferDeinterlace(frame);
    }

    if (frame->lines <= 240) {
        zoom = videoRender240(pVideo, frame, bitDepth, zoom, pDst, dstOffset, dstPitch, canChangeZoom);
    }
    else {
        zoom = videoRender480(pVideo, frame, bitDepth, zoom, pDst, dstOffset, dstPitch, canChangeZoom);
    }
    return zoom;
}

Video* videoCreate()
{
    Video* pVideo = (Video*)calloc(1, sizeof(Video));

    pVideo->deInterlace = 0;

    return pVideo;
}

void videoDestroy(Video* pVideo)
{
    free(pVideo);
}

void videoUpdateAll(Video* video, Properties* properties)
{
    video->deInterlace = properties->video.deInterlace;
}

#else

#define RGB_MASK 0x7fff

#define MAX_RGB_COLORS (1 << 16)

static UInt32 pRgbTableColor32[MAX_RGB_COLORS];
static UInt32 pRgbTableGreen32[MAX_RGB_COLORS];
static UInt32 pRgbTableWhite32[MAX_RGB_COLORS];
static UInt32 pRgbTableAmber32[MAX_RGB_COLORS];
static UInt16 pRgbTableColor16[MAX_RGB_COLORS];
static UInt16 pRgbTableGreen16[MAX_RGB_COLORS];
static UInt16 pRgbTableWhite16[MAX_RGB_COLORS];
static UInt16 pRgbTableAmber16[MAX_RGB_COLORS];

#define ABS(a) ((a) < 0 ? -1 * (a) : (a))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))


static int gammaTable[3 * 256];

static void generateGammaTable(Video* pVideo)
{
    int i;
    for (i = 0; i < 3 * 256; i++) {
    	DoubleT value = (i - 256 + pVideo->brightness) * pVideo->contrast;
        gammaTable[i] = 6;
        if (value > 0) {
        	DoubleT factor = pow(255., pVideo->gamma - 1.);
            value = (DoubleT)(factor * pow(value, pVideo->gamma));
            if (value > 0) {
                int gamma = (int)value;
                gammaTable[i] = MAX(6, MIN(247, gamma));
            }
        }
    }
}

#define videoGamma(index) gammaTable[index + 256]

static void initRGBTable(Video* pVideo) 
{
    int rgb;

    generateGammaTable(pVideo);

    for (rgb = 0; rgb < MAX_RGB_COLORS; rgb++) {
        int R = 8 * ((rgb >> 10) & 0x01f);
        int G = 8 * ((rgb >> 5) & 0x01f);
        int B = 8 * ((rgb >> 0) & 0x01f);
        int L = 0;

        int Y  = (int)(0.2989*R + 0.5866*G + 0.1145*B);
        int Cb = B - Y;
        int Cr = R - Y;

        if (pVideo->saturation < 0.999 || pVideo->saturation > 1.001) {
            Cb = (int)(Cb * pVideo->saturation);
            Cr = (int)(Cr * pVideo->saturation);
            Y  = MAX(0, MIN(255, Y));
            Cb = MAX(-256, MIN(255, Cb));
            Cr = MAX(-256, MIN(255, Cr));
            R = Cr + Y;
            G = (int)(Y - (0.1145/0.5866)*Cb - (0.2989/0.5866)*Cr);
            B = Cb + Y;
            R = MIN(255, MAX(0, R));
            G = MIN(255, MAX(0, G));
            B = MIN(255, MAX(0, B));
        }

        R = videoGamma(R);
        G = videoGamma(G);
        B = videoGamma(B);
        L = videoGamma(Y);

        if (pVideo->invertRGB) {
            pRgbTableColor32[rgb] = (R << 0) | (G << 8) | (B << 16);
        }
        else {
            pRgbTableColor32[rgb] = (R << 16) | (G << 8) | (B << 0);
        }
        pRgbTableColor16[rgb] = ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);

        pRgbTableGreen32[rgb] = 0x100010 | (L << 8);
        pRgbTableGreen16[rgb] = 0x0801 | (UInt16)((L >> 2) << 5);

        pRgbTableWhite32[rgb] = (L << 16) | (L << 8) | (L << 0);
        pRgbTableWhite16[rgb] = (UInt16)(((L >> 3) << 11) | ((L >> 2) << 5) | (L >> 3));

        if (pVideo->invertRGB) {
            pRgbTableAmber32[rgb] = (L << 0) | ((L * 176 / 255) << 8);
        }
        else {
            pRgbTableAmber32[rgb] = (L << 16) | ((L * 176 / 255) << 8);
        }
        pRgbTableAmber16[rgb] = ((L >> 3) << 11) | (((L * 176 / 255) >> 2) << 5);
    }
}


/*****************************************************************************
**
** PAL emulation rendering routines
**
******************************************************************************
*/
static void copySharpPAL_2x2_16(FrameBuffer* frame, void* pDestination, int dstPitch, UInt16* rgbTable, UInt32 rnd)
{
    static UInt32 rndVal = 51;
    UInt16* pDst1       = (UInt16*)pDestination;
    UInt16* pDst2       = pDst1 + dstPitch / (int)sizeof(UInt16);
    UInt16* pDst3       = pDst2;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    rndVal *= 13;
    rnd    *= rndVal;

    dstPitch /= (int)sizeof(UInt16);

    if (frame->interlace == INTERLACE_ODD) {
        pDst1 += dstPitch;
        pDst2 += dstPitch;
        height--;
    }

    for (h = 0; h < height; h++) {
        UInt16* pSrc = frame->line[h].buffer;
        UInt16 colCur = (rgbTable[pSrc[0]] & 0xe79c) >> 2;
        UInt16 colPrev = colCur;
        int dstIndex = 0;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth * 2;
            int w;
            for (w = 0; w < width;) {
                UInt16 colNext;
                UInt16 colRgb1;
                UInt16 colRgb2;
                UInt16 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xe79c) >> 2;
                colRgb1 = (colPrev + colNext + 2 * colCur) & 0xe79c;

                colPrev = colCur;
                colCur  = colNext;

                colNext = (rgbTable[pSrc[w++]] & 0xe79c) >> 2;
                colRgb2  = (colPrev + colNext + 2 * colCur) & 0xe79c;

                colPrev = colCur;
                colCur  = colNext;

                noise = (UInt16)(rnd >> 31) * 0x0821;
                pDst2[dstIndex] = colRgb1 + noise;
                pDst1[dstIndex] = (((pDst3[dstIndex] >> 1) & 0x7bef) + ((colRgb1 >> 1) & 0x7bef));
                dstIndex++;
                pDst2[dstIndex] = colRgb2 + noise;
                pDst1[dstIndex] = (((pDst3[dstIndex] >> 3) & 0x7bef) + ((colRgb2 >> 1) & 0x7bef));
                dstIndex++;

                rnd *= 23;
            }
        }
        else {
            int width = srcWidth;
            int w;
            for (w = 0; w < width;) {
                UInt16 colNext;
                UInt16 colRgb1;
                UInt16 colRgb2;
                UInt16 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xe79c) >> 2;
                colRgb1  = (colNext + 3 * colCur) & 0xe79c;
                colRgb2  = (colCur + 3 * colNext) & 0xe79c;
                
                colCur = colNext;

                noise = (UInt16)(rnd >> 31) * 0x0821;
                pDst2[dstIndex] = colRgb1 + noise;
                pDst1[dstIndex] = (((pDst3[dstIndex] >> 1) & 0x7bef) + ((colRgb1 >> 1) & 0x7bef));
                dstIndex++;
                pDst2[dstIndex] = colRgb2 + noise;
                pDst1[dstIndex] = (((pDst3[dstIndex] >> 1) & 0x7bef) + ((colRgb2 >> 1) & 0x7bef));
                dstIndex++;

                rnd *= 23;
            }
        }

        pDst3 = pDst2;
        pDst1 += dstPitch * 2;
        pDst2 += dstPitch * 2;
    }
}

static void copySharpPAL_2x2_32(FrameBuffer* frame, void* pDestination, int dstPitch, UInt32* rgbTable, UInt32 rnd)
{
    static UInt32 rndVal = 51;
    UInt32* pDst1       = (UInt32*)pDestination;
    UInt32* pDst2       = pDst1 + dstPitch / (int)sizeof(UInt32);
    UInt32* pDst3       = pDst2;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    rndVal *= 13;
    rnd    *= rndVal;

    dstPitch /= (int)sizeof(UInt32);

    if (frame->interlace == INTERLACE_ODD) {
        pDst1 += dstPitch;
        pDst2 += dstPitch;
        height--;
    }

    for (h = 0; h < height; h++) {
        UInt16* pSrc = frame->line[h].buffer;
        UInt32 colCur = (rgbTable[pSrc[0]] & 0xfcfcfc) >> 2;
        UInt32 colPrev = colCur;
        int dstIndex = 0;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth * 2;
            int w;
            for (w = 0; w < width;) {
                UInt32 colNext;
                UInt32 colRgb1;
                UInt32 colRgb2;
                UInt32 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xfcfcfc) >> 2;
                colRgb1 = (colPrev + colNext + 2 * colCur) & 0xfcfcfc;

                colPrev = colCur;
                colCur  = colNext;

                colNext = (rgbTable[pSrc[w++]] & 0xfcfcfc) >> 2;
                colRgb2  = (colPrev + colNext + 2 * colCur) & 0xfcfcfc;

                colPrev = colCur;
                colCur  = colNext;

                noise = (rnd >> 29) * 0x10101;
                pDst2[dstIndex] = colRgb1 + noise;
                pDst1[dstIndex] = ((pDst3[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb1 >> 1) & 0x7f7f7f);
                dstIndex++;
                pDst2[dstIndex] = colRgb2 + noise;
                pDst1[dstIndex] = ((pDst3[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb2 >> 1) & 0x7f7f7f);
                dstIndex++;

                rnd *= 23;
            }
        }
        else {
            int width = srcWidth;
            int w;
            for (w = 0; w < width;) {
                UInt32 colNext;
                UInt32 colRgb1;
                UInt32 colRgb2;
                UInt32 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xfcfcfc) >> 2;
                colRgb1  = (colNext + 3 * colCur) & 0xfcfcfc;
                colRgb2  = (colCur + 3 * colNext) & 0xfcfcfc;
                
                colCur = colNext;

                noise = (rnd >> 29) * 0x10101;
                pDst2[dstIndex] = colRgb1 + noise;
                pDst1[dstIndex] = ((pDst3[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb1 >> 1) & 0x7f7f7f);
                dstIndex++;
                pDst2[dstIndex] = colRgb2 + noise;
                pDst1[dstIndex] = ((pDst3[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb2 >> 1) & 0x7f7f7f);
                dstIndex++;

                rnd *= 23;
            }
        }

        pDst3 = pDst2;
        pDst1 += dstPitch * 2;
        pDst2 += dstPitch * 2;
    }
}

static void copySharpPAL_2x1_16(FrameBuffer* frame, void* pDestination, int dstPitch, UInt16* rgbTable, UInt32 rnd)
{
    static UInt32 rndVal = 51;
    UInt16* pDst1       = (UInt16*)pDestination;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    rndVal *= 13;
    rnd    *= rndVal;

    dstPitch /= (int)sizeof(UInt16);

    if (frame->interlace == INTERLACE_ODD) {
        pDst1 += dstPitch;
        height--;
    }

    for (h = 0; h < height; h++) {
        UInt16* pSrc = frame->line[h].buffer;
        UInt16 colCur = (rgbTable[pSrc[0]] & 0xe79c) >> 2;
        UInt16 colPrev = colCur;
        int dstIndex = 0;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth * 2;
            int w;
            for (w = 0; w < width;) {
                UInt16 colNext;
                UInt16 colRgb1;
                UInt16 colRgb2;
                UInt16 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xe79c) >> 2;
                colRgb1 = (colPrev + colNext + 2 * colCur) & 0xe79c;

                colPrev = colCur;
                colCur  = colNext;

                colNext = (rgbTable[pSrc[w++]] & 0xe79c) >> 2;
                colRgb2 = (colPrev + colNext + 2 * colCur) & 0xe79c;

                colPrev = colCur;
                colCur  = colNext;

                noise = (UInt16)((rnd >> 31) * 0x821);
                pDst1[dstIndex++] = colRgb1 + noise;
                pDst1[dstIndex++] = colRgb2 + noise;

                rnd *= 23;
            }
        }
        else {
            int width = srcWidth;
            int w;
            for (w = 0; w < width;) {
                UInt16 colNext;
                UInt16 colRgb1;
                UInt16 colRgb2;
                UInt16 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xe79c) >> 2;
                colRgb1 = (colPrev + colNext + 2 * colCur) & 0xe79c;

                colPrev = colCur;
                colCur  = colNext;

                colNext = (rgbTable[pSrc[w++]] & 0xe79c) >> 2;
                colRgb2 = (colPrev + colNext + 2 * colCur) & 0xe79c;

                colPrev = colCur;
                colCur  = colNext;

                noise = (UInt16)((rnd >> 31) * 0x821);
                pDst1[dstIndex++] = colRgb1 + noise;
                pDst1[dstIndex++] = colRgb2 + noise;

                rnd *= 23;
            }
        }

        pDst1 += dstPitch;
    }
}


static void copySharpPAL_2x1_32(FrameBuffer* frame, void* pDestination, int dstPitch, UInt32* rgbTable, UInt32 rnd)
{
    static UInt32 rndVal = 51;
    UInt32* pDst1       = (UInt32*)pDestination;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    rndVal *= 13;
    rnd    *= rndVal;

    dstPitch /= (int)sizeof(UInt32);

    if (frame->interlace == INTERLACE_ODD) {
        pDst1 += dstPitch;
        height--;
    }

    for (h = 0; h < height; h++) {
        UInt16* pSrc = frame->line[h].buffer;
        UInt32 colCur = (rgbTable[pSrc[0]] & 0xfcfcfc) >> 2;
        UInt32 colPrev = colCur;
        int dstIndex = 0;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth * 2;
            int w;
            for (w = 0; w < width;) {
                UInt32 colNext;
                UInt32 colRgb1;
                UInt32 colRgb2;
                UInt32 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xfcfcfc) >> 2;
                colRgb1 = (colPrev + colNext + 2 * colCur) & 0xfcfcfc;

                colPrev = colCur;
                colCur  = colNext;

                colNext = (rgbTable[pSrc[w++]] & 0xfcfcfc) >> 2;
                colRgb2 = (colPrev + colNext + 2 * colCur) & 0xfcfcfc;

                colPrev = colCur;
                colCur  = colNext;

                noise = (rnd >> 29) * 0x10101;
                pDst1[dstIndex++] = colRgb1 + noise;
                pDst1[dstIndex++] = colRgb2 + noise;

                rnd *= 23;
            }
        }
        else {
            int width = srcWidth;
            int w;
            for (w = 0; w < width;) {
                UInt32 colNext;
                UInt32 colRgb1;
                UInt32 colRgb2;
                UInt32 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xfcfcfc) >> 2;
                colRgb1 = (colPrev + 3 * colCur) & 0xfcfcfc;
                colRgb2 = (colNext + 3 * colCur) & 0xfcfcfc;

                colPrev = colCur;
                colCur  = colNext;

                noise = (rnd >> 29) * 0x10101;
                pDst1[dstIndex++] = colRgb1 + noise;
                pDst1[dstIndex++] = colRgb2 + noise;

                rnd *= 23;
                colCur = colNext;
            }
        }

        pDst1 += dstPitch;
    }
}

static void copyMonitorPAL_2x2_16(FrameBuffer* frame, void* pDestination, int dstPitch, UInt16* rgbTable, UInt32 rnd)
{
    static UInt32 rndVal = 51;
    UInt16* pDst1       = (UInt16*)pDestination;
    UInt16* pDst2       = pDst1 + dstPitch / (int)sizeof(UInt16);
    UInt16* pDst3       = pDst2;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    rndVal *= 13;
    rnd    *= rndVal;

    dstPitch /= (int)sizeof(UInt16);

    if (frame->interlace == INTERLACE_ODD) {
        pDst1 += dstPitch;
        pDst2 += dstPitch;
        height--;
    }

    for (h = 0; h < height; h++) {
        UInt16* pSrc = frame->line[h].buffer;
        UInt16 colCur = (rgbTable[pSrc[0]] & 0xe79c) >> 2;
        UInt16 colPrev = colCur;
        int dstIndex = 0;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth * 2;
            int w;
            for (w = 0; w < width;) {
                UInt16 colNext;
                UInt16 colRgb1;
                UInt16 colRgb2;
                UInt16 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xe79c) >> 2;
                colRgb1 = (colPrev + 3 * colCur) & 0xe79c;

                colPrev = colCur;
                colCur  = colNext;

                colNext = (rgbTable[pSrc[w++]] & 0xe79c) >> 2;
                colRgb2 = (colNext + 3 * colCur) & 0xe79c;

                colPrev = colCur;
                colCur  = colNext;

                noise = (UInt16)(rnd >> 31) * 0x0821;
                pDst2[dstIndex] = colRgb1 + noise;
                pDst1[dstIndex] = ((pDst3[dstIndex] >> 1) & 0x7bef) + ((colRgb1 >> 1) & 0x7bef);
                pDst1[dstIndex] = ((pDst1[dstIndex] >> 1) & 0x7bef) + ((colRgb1 >> 1) & 0x7bef);
                dstIndex++;
                pDst2[dstIndex] = colRgb2 + noise;
                pDst1[dstIndex] = ((pDst3[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb2 >> 1) & 0x7f7f7f);
                pDst1[dstIndex] = ((pDst1[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb2 >> 1) & 0x7f7f7f);
                dstIndex++;

                rnd *= 23;
            }
        }
        else {
            int width = srcWidth;
            int w;
            for (w = 0; w < width;) {
                UInt16 colRgb1;
                UInt16 colRgb2;
                UInt16 colNext;
                UInt16 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xfcfcfc) >> 2;
                colRgb1 = (3 * colCur + colNext) & 0xfcfcfc;
                colRgb2 = (4 * colNext) & 0xfcfcfc;
                
                colCur = colNext;

                noise = (UInt16)(rnd >> 31) * 0x0821;
                pDst2[dstIndex] = colRgb1 + noise;
                pDst1[dstIndex] = ((pDst3[dstIndex] >> 1) & 0x7bef) + ((colRgb1 >> 1) & 0x7bef);
                pDst1[dstIndex] = ((pDst1[dstIndex] >> 1) & 0x7bef) + ((colRgb1 >> 1) & 0x7bef);
                dstIndex++;
                pDst2[dstIndex] = colRgb2 + noise;
                pDst1[dstIndex] = ((pDst3[dstIndex] >> 1) & 0x7bef) + ((colRgb2 >> 1) & 0x7bef);
                pDst1[dstIndex] = ((pDst1[dstIndex] >> 1) & 0x7bef) + ((colRgb2 >> 1) & 0x7bef);
                dstIndex++;

                rnd *= 23;
            }
        }

        pDst3  = pDst2;
        pDst1 += dstPitch * 2;
        pDst2 += dstPitch * 2;
    }
}

static void copyMonitorPAL_2x2_32(FrameBuffer* frame, void* pDestination, int dstPitch, UInt32* rgbTable, UInt32 rnd)
{
    static UInt32 rndVal = 51;
    UInt32* pDst1       = (UInt32*)pDestination;
    UInt32* pDst2       = pDst1 + dstPitch / (int)sizeof(UInt32);
    UInt32* pDst3       = pDst2;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    rndVal *= 13;
    rnd    *= rndVal;

    dstPitch /= (int)sizeof(UInt32);

    if (frame->interlace == INTERLACE_ODD) {
        pDst1 += dstPitch;
        pDst2 += dstPitch;
        height--;
    }

    for (h = 0; h < height; h++) {
        UInt16* pSrc = frame->line[h].buffer;
        UInt32 colCur = (rgbTable[pSrc[0]] & 0xfcfcfc) >> 2;
        UInt32 colPrev = colCur;
        int dstIndex = 0;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth * 2;
            int w;
            for (w = 0; w < width;) {
                UInt32 colNext;
                UInt32 colRgb1;
                UInt32 colRgb2;
                UInt32 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xfcfcfc) >> 2;
                colRgb1 = (colPrev + 3 * colCur) & 0xfcfcfc;

                colPrev = colCur;
                colCur  = colNext;

                colNext = (rgbTable[pSrc[w++]] & 0xfcfcfc) >> 2;
                colRgb2 = (colNext + 3 * colCur) & 0xfcfcfc;

                colPrev = colCur;
                colCur  = colNext;

                noise = (rnd >> 30) * 0x10101;
                pDst2[dstIndex] = colRgb1 + noise;
                pDst1[dstIndex] = ((pDst3[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb1 >> 1) & 0x7f7f7f);
                pDst1[dstIndex] = ((pDst1[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb1 >> 1) & 0x7f7f7f);
                dstIndex++;
                pDst2[dstIndex] = colRgb2 + noise;
                pDst1[dstIndex] = ((pDst3[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb2 >> 1) & 0x7f7f7f);
                pDst1[dstIndex] = ((pDst1[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb2 >> 1) & 0x7f7f7f);
                dstIndex++;

                rnd *= 23;
            }
        }
        else {
            int width = srcWidth;
            int w;
            for (w = 0; w < width;) {
                UInt32 colRgb1;
                UInt32 colRgb2;
                UInt32 colNext;
                UInt32 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xfcfcfc) >> 2;
                colRgb1 = (3 * colCur + colNext) & 0xfcfcfc;
                colRgb2 = (4 * colNext) & 0xfcfcfc;
                
                colCur = colNext;

                noise = (rnd >> 30) * 0x10101;
                pDst2[dstIndex] = colRgb1 + noise;
                pDst1[dstIndex] = ((pDst3[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb1 >> 1) & 0x7f7f7f);
                pDst1[dstIndex] = ((pDst1[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb1 >> 1) & 0x7f7f7f);
                dstIndex++;
                pDst2[dstIndex] = colRgb2 + noise;
                pDst1[dstIndex] = ((pDst3[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb2 >> 1) & 0x7f7f7f);
                pDst1[dstIndex] = ((pDst1[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb2 >> 1) & 0x7f7f7f);
                dstIndex++;

                rnd *= 23;
            }
        }

        pDst3  = pDst2;
        pDst1 += dstPitch * 2;
        pDst2 += dstPitch * 2;
    }
}

static void copyMonitorPAL_2x1_16(FrameBuffer* frame, void* pDestination, int dstPitch, UInt16* rgbTable, UInt32 rnd)
{
    static UInt32 rndVal = 51;
    UInt16* pDst1       = (UInt16*)pDestination;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    rndVal *= 13;
    rnd    *= rndVal;

    dstPitch /= (int)sizeof(UInt16);

    if (frame->interlace == INTERLACE_ODD) {
        pDst1 += dstPitch;
        height--;
    }

    for (h = 0; h < height; h++) {
        UInt16* pSrc = frame->line[h].buffer;
        UInt16 colCur = (rgbTable[pSrc[0]] & 0xe79c) >> 2;
        int dstIndex = 0;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth * 2;
            int w;
            for (w = 0; w < width;) {
                UInt16 colNext;
                UInt16 colRgb1;
                UInt16 colRgb2;
                UInt16 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xe79c) >> 2;
                colRgb1 = (colNext + 3 * colCur) & 0xe79c;

                colCur = colNext;

                colNext = (rgbTable[pSrc[w++]] & 0xe79c) >> 2;
                colRgb2 = (colNext + 3 * colCur) & 0xe79c;

                colCur = colNext;

                noise = (UInt16)(rnd >> 31) * 0x0821;
                pDst1[dstIndex++] = colRgb1 + noise;
                pDst1[dstIndex++] = colRgb2 + noise;

                rnd *= 23;
            }
        }
        else {
            int width = srcWidth;
            int w;
            for (w = 0; w < width;) {
                UInt16 colNext;
                UInt16 colRgb1;
                UInt16 colRgb2;
                UInt16 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xe79c) >> 2;
                colRgb1 = (colNext + 3 * colCur) & 0xe79c;
                colRgb2 = (colNext * 4) & 0xe79c;
                
                colCur = colNext;


                noise = (UInt16)(rnd >> 31) * 0x0821;
                pDst1[dstIndex++] = colRgb1 + noise;
                pDst1[dstIndex++] = colRgb2 + noise;
                rnd *= 23;
            }
        }

        pDst1 += dstPitch;
    }
}

static void copyMonitorPAL_2x1_32(FrameBuffer* frame, void* pDestination, int dstPitch, UInt32* rgbTable, UInt32 rnd)
{
    static UInt32 rndVal = 51;
    UInt32* pDst1       = (UInt32*)pDestination;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    rndVal *= 13;
    rnd    *= rndVal;

    dstPitch /= (int)sizeof(UInt32);

    if (frame->interlace == INTERLACE_ODD) {
        pDst1 += dstPitch;
        height--;
    }

    for (h = 0; h < height; h++) {
        UInt16* pSrc = frame->line[h].buffer;
        UInt32 colCur = (rgbTable[pSrc[0]] & 0xfcfcfc) >> 2;
        int dstIndex = 0;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth * 2;
            int w;
            for (w = 0; w < width;) {
                UInt32 colNext;
                UInt32 colRgb1;
                UInt32 colRgb2;
                UInt32 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xfcfcfc) >> 2;
                colRgb1 = (colNext + 3 * colCur) & 0xfcfcfc;

                colCur = colNext;

                colNext = (rgbTable[pSrc[w++]] & 0xfcfcfc) >> 2;
                colRgb2 = (colNext + 3 * colCur) & 0xfcfcfc;

                colCur = colNext;

                noise = (rnd >> 30) * 0x10101;

                pDst1[dstIndex++] = colRgb1 + noise;
                pDst1[dstIndex++] = colRgb2 + noise;

                rnd *= 23;
            }
        }
        else {
            int width = srcWidth;
            int w;
            for (w = 0; w < width;) {
                UInt32 colNext;
                UInt32 colRgb1;
                UInt32 colRgb2;
                UInt32 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xfcfcfc) >> 2;
                colRgb1 = (colNext + 3 * colCur) & 0xfcfcfc;
                colRgb2 = (colNext * 4) & 0xfcfcfc;
                
                colCur = colNext;

                noise = (rnd >> 30) * 0x10101;
                pDst1[dstIndex++] = colRgb1 + noise;
                pDst1[dstIndex++] = colRgb2 + noise;
                rnd *= 23;
            }
        }

        pDst1 += dstPitch;
    }
}


static void copyPAL_2x2_32(FrameBuffer* frame, void* pDestination, int dstPitch, UInt32* rgbTable, UInt32 rnd)
{
    static UInt32 rndVal = 51;
    UInt32* pDst1       = (UInt32*)pDestination;
    UInt32* pDst2       = pDst1 + dstPitch / (int)sizeof(UInt32);
    UInt32* pDst3       = pDst2;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    rndVal *= 13;
    rnd    *= rndVal;

    dstPitch /= (int)sizeof(UInt32);

    if (frame->interlace == INTERLACE_ODD) {
        pDst1 += dstPitch;
        pDst2 += dstPitch;
        height--;
    }

    for (h = 0; h < height; h++) {
        UInt16* pSrc = frame->line[h].buffer;
        UInt32 colCur = (rgbTable[pSrc[0]] & 0xf0f0f0) >> 4;
        UInt32 colPrev2 = colCur;
        UInt32 colPrev1 = colCur;
        UInt32 colNext1 = colCur;
        int dstIndex = 0;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth * 2;
            int w;
            for (w = 0; w < width;) {
                UInt32 colNext2;
                UInt32 colRgb1;
                UInt32 colRgb2;
                UInt32 colLgt;
                UInt32 noise;

                colNext2 = (rgbTable[pSrc[w++]] & 0xf0f0f0) >> 4;
                colLgt   = colCur;

                colRgb1 = (colPrev2 + colNext2 + 2 * colNext1 + 4 * colPrev1 + 8 * colLgt) & 0xf0f0f0;

                colPrev2 = colPrev1;
                colPrev1 = colCur;
                colCur   = colNext1;
                colNext1 = colNext2;
                colNext2 = pSrc[w];

                colNext2 = (rgbTable[pSrc[w++]] & 0xf0f0f0) >> 4;
                colLgt   = colCur;

                colRgb2 = (colPrev2 + colNext2 + 2 * colPrev1 + 4 * colNext1 + 8 * colLgt) & 0xf0f0f0;

                colPrev2 = colPrev1;
                colPrev1 = colCur;
                colCur   = colNext1;
                colNext1 = colNext2;

                noise = (rnd >> 29) * 0x10101;
                pDst2[dstIndex] = colRgb1 + noise;
                pDst1[dstIndex] = (((pDst3[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb1 >> 1) & 0x7f7f7f));
                dstIndex++;
                pDst2[dstIndex] = colRgb2 + noise;
                pDst1[dstIndex] = (((pDst3[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb2 >> 1) & 0x7f7f7f));
                dstIndex++;

                rnd *= 23;
            }
        }
        else {
            int width = srcWidth;
            int w;
            for (w = 0; w < width;) {
                UInt32 colNext;
                UInt32 colRgb1;
                UInt32 colRgb2;
                UInt32 colLgt;
                UInt32 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xf0f0f0) >> 4;
                colLgt   = colCur;

                colRgb1 = (6 * colPrev1 + 2 * colNext + 8 * colLgt) & 0xf0f0f0;
                colRgb2 = (2 * colPrev1 + 6 * colNext + 8 * colLgt) & 0xf0f0f0;

                noise = (rnd >> 29) * 0x10101;
                pDst2[dstIndex] = colRgb1 + noise;
                pDst1[dstIndex] = (((pDst3[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb1 >> 1) & 0x7f7f7f));
                dstIndex++;
                pDst2[dstIndex] = colRgb2 + noise;
                pDst1[dstIndex] = (((pDst3[dstIndex] >> 1) & 0x7f7f7f) + ((colRgb2 >> 1) & 0x7f7f7f));
                dstIndex++;

                rnd *= 23;
                colPrev1 = colCur;
                colCur = colNext;
            }
        }
        pDst3 = pDst2;
        pDst1 += dstPitch * 2;
        pDst2 += dstPitch * 2;
    }
}


static void copyPAL_2x1_32(FrameBuffer* frame, void* pDestination, int dstPitch, UInt32* rgbTable, UInt32 rnd)
{
    static UInt32 rndVal = 51;
    UInt32* pDst1       = (UInt32*)pDestination;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    rndVal *= 13;
    rnd    *= rndVal;

    dstPitch /= (int)sizeof(UInt32);

    if (frame->interlace == INTERLACE_ODD) {
        pDst1 += dstPitch;
        height--;
    }

    for (h = 0; h < height; h++) {
        UInt16* pSrc = frame->line[h].buffer;
        UInt32 colCur = (rgbTable[pSrc[0]] & 0xf0f0f0) >> 4;
        UInt32 colPrev2 = colCur;
        UInt32 colPrev1 = colCur;
        UInt32 colNext1 = colCur;
        int dstIndex = 0;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth * 2;
            int w;
            for (w = 0; w < width;) {
                UInt32 colNext2;
                UInt32 colRgb1;
                UInt32 colRgb2;
                UInt32 colLgt;
                UInt32 noise;

                colNext2 = (rgbTable[pSrc[w++]] & 0xf0f0f0) >> 4;
                colLgt = colCur;
                colRgb1 = (colPrev2 + colNext2 + 2 * colNext1 + 4 * colPrev1 + 8 * colLgt) & 0xf0f0f0;

                colPrev2 = colPrev1;
                colPrev1 = colCur;
                colCur   = colNext1;
                colNext1 = colNext2;
                colNext2 = pSrc[w];

                colNext2 = (rgbTable[pSrc[w++]] & 0xf0f0f0) >> 4;
                colLgt = colCur;
                colRgb2 = (colPrev2 + colNext2 + 2 * colPrev1 + 4 * colNext1 + 8 * colLgt) & 0xf0f0f0;

                colPrev2 = colPrev1;
                colPrev1 = colCur;
                colCur   = colNext1;
                colNext1 = colNext2;

                noise = (rnd >> 29) * 0x10101;
                pDst1[dstIndex++] = colRgb1 + noise;
                pDst1[dstIndex++] = colRgb2 + noise;

                rnd *= 23;
            }
        }
        else {
            int width = srcWidth;
            int w;
            for (w = 0; w < width;) {
                UInt32 colNext;
                UInt32 colRgb1;
                UInt32 colRgb2;
                UInt32 colLgt;
                UInt32 noise;

                colNext = (rgbTable[pSrc[w++]] & 0xf0f0f0) >> 4;
                colLgt = colCur;
                colRgb1 = (2 * colNext  + 6 * colPrev1 + 8 * colLgt) & 0xf0f0f0;
                colRgb2 = (2 * colPrev1 + 6 * colNext  + 8 * colLgt) & 0xf0f0f0;

                noise = (rnd >> 29) * 0x10101;
                pDst1[dstIndex++] = colRgb1 + noise;
                pDst1[dstIndex++] = colRgb2 + noise;


                rnd *= 23;
                colPrev1 = colCur;
                colCur = colNext;
            }
        }
        pDst1 += dstPitch;
    }
}

static void copyPAL_1x1_16(FrameBuffer* frame, void* pDestination, int dstPitch, UInt16* rgbTable, UInt32 rnd)
{
    static UInt32 rndVal = 51;
    UInt16* pDst        = (UInt16*)pDestination;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    rndVal *= 13;
    rnd    *= rndVal;
    
    dstPitch /= (int)sizeof(UInt16);

    for (h = 0; h < height; h++) {
        UInt16* pSrc = frame->line[h].buffer;
        UInt16 colCur =  (rgbTable[pSrc[0]] & 0xe79c) >> 2;
        UInt16 colPrev = colCur;
        int dstIndex = 0;
        int w;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth * 2;
            for (w = 0; w < width;) {
                UInt16 colTmp1 = (rgbTable[pSrc[w++]] & 0xc718) >> 3;
                UInt16 colTmp2 = (rgbTable[pSrc[w++]] & 0xc718) >> 3;
                UInt16 colNext = (colTmp1 + colTmp2) & 0xe79c;
                UInt16 colRgb  = (colPrev + 2 * colCur + colNext) & 0xe79c;

                colPrev = colCur;
                colCur = colNext;

                pDst[dstIndex++] = colRgb + (UInt16)(rnd >> 31)  * 0x0821;
                rnd *= 23;
            }
        }
        else {
            int width = srcWidth;
            for (w = 0; w < width;) {
                UInt16 colNext = (rgbTable[pSrc[w++]] & 0xe79c) >> 2;
                UInt16 colRgb  = (colPrev + 2 * colCur + colNext) & 0xe79c;
                colPrev = colCur;
                colCur = colNext;

                pDst[dstIndex++] = colRgb + (UInt16)(rnd >> 31)  * 0x0821;
                rnd *= 23;
            }
        }
        pDst += dstPitch;
    }
}


static void copyPAL_1x1_32(FrameBuffer* frame, void* pDestination, int dstPitch, UInt32* rgbTable, UInt32 rnd)
{
    static UInt32 rndVal = 51;
    UInt32* pDst        = (UInt32*)pDestination;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    rndVal *= 13;
    rnd    *= rndVal;

    dstPitch /= (int)sizeof(UInt32);

    for (h = 0; h < height; h++) {
        UInt16* pSrc = frame->line[h].buffer;
        UInt32 colCur = (rgbTable[pSrc[0]] & 0xfcfcfc) >> 2;
        UInt32 colPrev = colCur;
        int dstIndex = 0;
        int w;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth * 2;
            for (w = 0; w < width; ) {
                UInt32 colTmp1 = (rgbTable[pSrc[w++]] & 0xf8f8f8) >> 3;
                UInt32 colTmp2 = (rgbTable[pSrc[w++]] & 0xf8f8f8) >> 3;
                UInt32 colNext = (colTmp1 + colTmp2) & 0xfcfcfc;
                UInt32 colRgb  = (colPrev + 2 * colCur + colNext) & 0xfcfcfc;

                colPrev = colCur;
                colCur = colNext;

                pDst[dstIndex++] = colRgb + (rnd >> 31)  * 0x10101;
                rnd *= 23;
            }
        }
        else {
            int width = srcWidth;
            for (w = 0; w < width;) {
                UInt32 colNext = (rgbTable[pSrc[w++]] & 0xfcfcfc) >> 2;
                UInt32 colRgb  = (colPrev + 2 * colCur + colNext) & 0xfcfcfc;
                colPrev = colCur;
                colCur = colNext;

                pDst[dstIndex++] = colRgb + (rnd >> 31)  * 0x10101;
                rnd *= 23;
            }
        }
        pDst += dstPitch;
    }
}

static void copyPAL_1x05_32(FrameBuffer* frame, void* pDestination, int dstPitch, UInt32* rgbTable, UInt32 rnd)
{
    static UInt32 rndVal = 51;
    UInt32* pDst        = (UInt32*)pDestination;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    rndVal *= 13;
    rnd    *= rndVal;

    dstPitch /= (int)sizeof(UInt32);

    for (h = 0; h < height; h += 2) {
        UInt16* pSrcA = frame->line[h + 0].buffer;
        UInt16* pSrcB = frame->line[h + 1].buffer;
        UInt32 colCur = (rgbTable[pSrcA[0]] & 0xfcfcfc) >> 2;
        UInt32 colPrev = colCur;
        int dstIndex = 0;
        int w;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth * 2;
            for (w = 0; w < width;) {
                UInt32 colTmp1A = (rgbTable[pSrcA[w  ]] & 0xf0f0f0) >> 4;
                UInt32 colTmp1B = (rgbTable[pSrcB[w++]] & 0xf0f0f0) >> 4;
                UInt32 colTmp2A = (rgbTable[pSrcA[w  ]] & 0xf0f0f0) >> 4;
                UInt32 colTmp2B = (rgbTable[pSrcB[w++]] & 0xf0f0f0) >> 4;
                UInt32 colNext = (colTmp1A + colTmp2A + colTmp1B + colTmp2B) & 0xf0f0f0;
                UInt32 colRgb  = (colPrev + 2 * colCur + colNext) & 0xfcfcfc;
                
                colPrev = colCur;
                colCur = colNext;

                pDst[dstIndex++] = colRgb + (rnd >> 31)  * 0x10101;
                rnd *= 23;
            }
        }
        else {
            int width = srcWidth;
            for (w = 0; w < width;) {
                UInt32 colTmpA = (rgbTable[pSrcA[w  ]] & 0xf8f8f8) >> 3;
                UInt32 colTmpB = (rgbTable[pSrcB[w++]] & 0xf8f8f8) >> 3;
                UInt32 colNext = (colTmpA + colTmpB) & 0xf8f8f8;
                UInt32 colRgb  = (colPrev + 2 * colCur + colNext) & 0xfcfcfc;
                
                colPrev = colCur;
                colCur = colNext;

                pDst[dstIndex++] = colRgb + (rnd >> 31)  * 0x10101;
                rnd *= 23;
            }
        }
        pDst += dstPitch;
    }
}


/*****************************************************************************
**
** Fast rendering routines
**
******************************************************************************
*/
static void copy_1x1_16(FrameBuffer* frame, void* pDestination, int dstPitch, UInt16* rgbTable)
{
    UInt16* pDst        = (UInt16*)pDestination;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    dstPitch /= (int)sizeof(UInt16);

    for (h = 0; h < height; h++) {
        UInt16* pOldDst = pDst;
        UInt16* pSrc = frame->line[h].buffer;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth / 4;
            while (width--) {
                pDst[0] = (((rgbTable[pSrc[0]] & 0xe79c) >> 1) + ((rgbTable[pSrc[1]] & 0xe79c) >> 1)) & 0xe79c;
                pDst[1] = (((rgbTable[pSrc[2]] & 0xe79c) >> 1) + ((rgbTable[pSrc[3]] & 0xe79c) >> 1)) & 0xe79c;
                pDst[2] = (((rgbTable[pSrc[4]] & 0xe79c) >> 1) + ((rgbTable[pSrc[5]] & 0xe79c) >> 1)) & 0xe79c;
                pDst[3] = (((rgbTable[pSrc[6]] & 0xe79c) >> 1) + ((rgbTable[pSrc[7]] & 0xe79c) >> 1)) & 0xe79c;
                pSrc += 8;
                pDst += 4;
            }
        }
        else {
            int width = srcWidth / 4;
            while (width--) {
                pDst[0] = rgbTable[pSrc[0]];
                pDst[1] = rgbTable[pSrc[1]];
                pDst[2] = rgbTable[pSrc[2]];
                pDst[3] = rgbTable[pSrc[3]];
                pSrc += 4;
                pDst += 4;
            }
        }
        pDst = pOldDst + dstPitch; 
    }
}


static void copy_1x1_32(FrameBuffer* frame, void* pDestination, int dstPitch, UInt32* rgbTable)
{
    UInt32* pDst        = (UInt32*)pDestination;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    dstPitch /= (int)sizeof(UInt32);

    for (h = 0; h < height; h++) {
        UInt32* pOldDst = pDst;
        UInt16* pSrc = frame->line[h].buffer;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth / 4;
            while (width--) {
                pDst[0] = (((rgbTable[pSrc[0]] & 0xfefefe) >> 1) + ((rgbTable[pSrc[1]] & 0xfefefe) >> 1)) & 0xfefefe;
                pDst[1] = (((rgbTable[pSrc[2]] & 0xfefefe) >> 1) + ((rgbTable[pSrc[3]] & 0xfefefe) >> 1)) & 0xfefefe;
                pDst[2] = (((rgbTable[pSrc[4]] & 0xfefefe) >> 1) + ((rgbTable[pSrc[5]] & 0xfefefe) >> 1)) & 0xfefefe;
                pDst[3] = (((rgbTable[pSrc[6]] & 0xfefefe) >> 1) + ((rgbTable[pSrc[7]] & 0xfefefe) >> 1)) & 0xfefefe;
                pSrc += 8;
                pDst += 4;
            }
        }
        else {
            int width = srcWidth / 4;
            while (width--) {
                pDst[0] = rgbTable[pSrc[0]];
                pDst[1] = rgbTable[pSrc[1]];
                pDst[2] = rgbTable[pSrc[2]];
                pDst[3] = rgbTable[pSrc[3]];
                pSrc += 4;
                pDst += 4;
            }
        }
        pDst = pOldDst + dstPitch; 
    }
}

static void copy_1x05_16(FrameBuffer* frame, void* pDestination, int dstPitch, UInt16* rgbTable)
{
    UInt16* pDst        = (UInt16*)pDestination;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    dstPitch /= (int)sizeof(UInt16);

    for (h = 0; h < height; h += 2) {
        UInt16* pOldDst = pDst;
        UInt16* pSrc1 = frame->line[h + 0].buffer;
        UInt16* pSrc2 = frame->line[h + 1].buffer;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth;
            while (width--) {
                UInt16 col0 = (((rgbTable[pSrc1[0]] & 0xe79c) >> 2) + ((rgbTable[pSrc1[1]] & 0xe79c) >> 2));
                UInt16 col1 = (((rgbTable[pSrc2[0]] & 0xe79c) >> 2) + ((rgbTable[pSrc2[1]] & 0xe79c) >> 2));
                
                *pDst++ = (col0 + col1) & 0xe79c;
                pSrc1 += 2;
                pSrc2 += 2;
            }
        }
        else {
            int width = srcWidth;
            while (width--) {
                *pDst++ = (((rgbTable[pSrc1[0]] & 0xe79c) >> 1) + ((rgbTable[pSrc2[0]] & 0xe79c) >> 1)) & 0xe79c;
            }
        }
        pDst = pOldDst + dstPitch; 
    }
}


static void copy_1x05_32(FrameBuffer* frame, void* pDestination, int dstPitch, UInt32* rgbTable)
{
    UInt32* pDst        = (UInt32*)pDestination;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    dstPitch /= (int)sizeof(UInt32);

    for (h = 0; h < height; h += 2) {
        UInt32* pOldDst = pDst;
        UInt16* pSrc1 = frame->line[h + 0].buffer;
        UInt16* pSrc2 = frame->line[h + 1].buffer;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth;
            while (width--) {
                UInt32 col0 = (((rgbTable[pSrc1[0]] & 0xfcfcfc) >> 2) + ((rgbTable[pSrc1[1]] & 0xfcfcfc) >> 2));
                UInt32 col1 = (((rgbTable[pSrc2[0]] & 0xfcfcfc) >> 2) + ((rgbTable[pSrc2[1]] & 0xfcfcfc) >> 2));
                
                *pDst++ = (col0 + col1) & 0xfcfcfc;
                pSrc1 += 2;
                pSrc2 += 2;
            }
        }
        else {
            int width = srcWidth;
            while (width--) {
                *pDst++ = (((rgbTable[pSrc1[0]] & 0xfefefe) >> 1) + ((rgbTable[pSrc2[0]] & 0xfefefe) >> 1)) & 0xfefefe;
                pSrc1++;
                pSrc2++;
            }
        }
        pDst = pOldDst + dstPitch; 
    }
}


static void copy_2x2_16(FrameBuffer* frame, void* pDestination, int dstPitch, UInt16* rgbTable)
{
    UInt16* pDst1       = (UInt16*)pDestination;
    UInt16* pDst2       = pDst1 + dstPitch / (int)sizeof(UInt16);
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    dstPitch /= (int)sizeof(UInt16);

    if (frame->interlace == INTERLACE_ODD) {
        pDst1 += dstPitch;
        pDst2 += dstPitch;
        height--;
    }

    for (h = 0; h < height; h++) {
        UInt16* pDst1old = pDst1;
        UInt16* pDst2old = pDst2;
        UInt16* pSrc = frame->line[h].buffer;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth / 4 * 2;
            while (width--) {
                UInt16 col1 = rgbTable[pSrc[0]];
                UInt16 col2 = rgbTable[pSrc[1]];
                UInt16 col3 = rgbTable[pSrc[2]];
                UInt16 col4 = rgbTable[pSrc[3]];
                pSrc  += 4;

                pDst1[0] = col1;
                pDst1[1] = col2;
                pDst1[2] = col3;
                pDst1[3] = col4;
                pDst1 += 4;

                pDst2[0] = col1;
                pDst2[1] = col2;
                pDst2[2] = col3;
                pDst2[3] = col4;
                pDst2 += 4;
            }
        }
        else {
            int width = srcWidth / 4;
            while (width--) {
                UInt16 col1 = rgbTable[pSrc[0]];
                UInt16 col2 = rgbTable[pSrc[1]];
                UInt16 col3 = rgbTable[pSrc[2]];
                UInt16 col4 = rgbTable[pSrc[3]];
                pSrc  += 4;

                pDst1[0] = col1;
                pDst1[1] = col1;
                pDst1[2] = col2;
                pDst1[3] = col2;
                pDst1[4] = col3;
                pDst1[5] = col3;
                pDst1[6] = col4;
                pDst1[7] = col4;
                pDst1 += 8;

                pDst2[0] = col1;
                pDst2[1] = col1;
                pDst2[2] = col2;
                pDst2[3] = col2;
                pDst2[4] = col3;
                pDst2[5] = col3;
                pDst2[6] = col4;
                pDst2[7] = col4;
                pDst2 += 8;
            }  
        }

        pDst1 = pDst1old + dstPitch * 2;
        pDst2 = pDst2old +  dstPitch * 2;
    }
}

#ifndef NO_ASM
/* 7500 units -> 4100 units */
static void copy_2x2_32_core1_SSE(UInt32* rgbTable, UInt16* pSrc, UInt32* pDst1, UInt32* pDst2, int width, int hint) {

#ifdef __GNUC__
    __asm__(
        "movl   $0,%%esi                \n\t"
        "movl   %4,%%ecx                \n\t"   // width
        "movl   %1,%%eax                \n\t"   // pSrc
        "movl   %2,%%ebx                \n\t"   // pDst1
        "movl   %0,%%edi                \n\t"   // rgbTable
"inner_loop1:                           \n\t"
        "movw   (%%eax),%%si            \n\t"
        "movd   (%%edi,%%esi,4),%%mm0   \n\t"
        "movw   2(%%eax),%%si           \n\t"
        "movd   (%%edi,%%esi,4),%%mm1   \n\t"
        "movw   6(%%eax),%%si           \n\t"
        "punpckldq %%mm1,%%mm0          \n\t"
        "movd   (%%edi,%%esi,4),%%mm3   \n\t"
        "movw   4(%%eax),%%si           \n\t"
        "movntq %%mm0,0(%%ebx)          \n\t"
        "addl   $8,%%eax                \n\t"
        "movd   (%%edi,%%esi,4),%%mm2   \n\t"
        "addl   $16,%%ebx               \n\t"
        "punpckldq %%mm3,%%mm2          \n\t"
        "decl   %%ecx                   \n\t"
        "movntq %%mm2,-8(%%ebx)         \n\t"
        "jnz    inner_loop1             \n\t"

        //#-- second line

        "movl   %4,%%ecx                \n\t"   // width
        "movl   %1,%%eax                \n\t"   // pSrc
        "movl   %5,%%ebx                \n\t"   // hint
        "movl   %3,%%edx                \n\t"   // pDst2
        "movl   %0,%%edi                \n\t"   // rgbTable
"inner_loop2:                           \n\t"
        "movw   (%%eax),%%si            \n\t"
        "movd   (%%edi,%%esi,4),%%mm0   \n\t"
        "movw   2(%%eax),%%si           \n\t"
        "movd   (%%edi,%%esi,4),%%mm1   \n\t"
        "movw   6(%%eax),%%si           \n\t"
        "punpckldq %%mm1,%%mm0          \n\t"
        "movd   (%%edi,%%esi,4),%%mm3   \n\t"
        "movw   4(%%eax),%%si           \n\t"
        "prefetcht0 (%%eax,%%ebx)       \n\t"
        "movntq %%mm0,0(%%edx)          \n\t"
        "addl   $8,%%eax                \n\t"
        "movd   (%%edi,%%esi,4),%%mm2   \n\t"
        "addl   $16,%%edx               \n\t"
        "punpckldq %%mm3,%%mm2          \n\t"
        "decl   %%ecx                   \n\t"
        "movntq %%mm2,-8(%%edx)         \n\t"
        "jnz    inner_loop2             \n\t"

        "emms                           \n\t"
        :
        : "m" (rgbTable), "m" (pSrc), "m" (pDst1), "m" (pDst2), "m" (width), "m" (hint)
        : "%eax", "%ebx", "%ecx", "%edx", "%edi", "%esi"
    );
#else
	__asm{
        mov     esi,0
		mov		ecx,width
		mov		eax,pSrc
		mov		ebx,pDst1
		mov		edi,rgbTable
inner_loop1:
		mov		si,[eax]
        movd	mm0,[edi+esi*4]
		mov		si,[eax+2]
		movd	mm1,[edi+esi*4]
		mov		si,[eax+6]
		punpckldq mm0,mm1
		movd	mm3,[edi+esi*4]
		mov		si,[eax+4]
		movntq	[ebx+0],mm0
		add		eax,8
		movd	mm2,[edi+esi*4]
		add		ebx,16
		punpckldq mm2,mm3
		dec		ecx
		movntq	[ebx-8],mm2
		jnz		inner_loop1

		;-- second line

		mov		ecx,width
		mov		eax,pSrc
		mov		ebx,hint
		mov		edx,pDst2
		mov		edi,rgbTable
inner_loop2:
		mov		si,[eax]
		movd	mm0,[edi+esi*4]
		mov		si,[eax+2]
		movd	mm1,[edi+esi*4]
		mov		si,[eax+6]
		punpckldq mm0,mm1
		movd	mm3,[edi+esi*4]
		mov		si,[eax+4]
		prefetcht0 [eax+ebx]
		movntq	[edx+0],mm0
		add		eax,8
		movd	mm2,[edi+esi*4]
		add		edx,16
		punpckldq mm2,mm3
		dec		ecx
		movntq	[edx-8],mm2
		jnz		inner_loop2

		emms 

	}
#endif
}
#endif

void copy_2x2_32_core1(UInt32* rgbTable, UInt16* pSrc, UInt32* pDst1, UInt32* pDst2, int width, int hint) 
{
    while (width--) {
        UInt32 col1 = rgbTable[pSrc[0]];
        UInt32 col2 = rgbTable[pSrc[1]];
        UInt32 col3 = rgbTable[pSrc[2]];
        UInt32 col4 = rgbTable[pSrc[3]];
        pSrc  += 4;

        pDst1[0] = col1;
        pDst1[1] = col2;
        pDst1[2] = col3;
        pDst1[3] = col4;
        pDst1 += 4;

        pDst2[0] = col1;
        pDst2[1] = col2;
        pDst2[2] = col3;
        pDst2[3] = col4;
        pDst2 += 4;
    }
}

#ifndef NO_ASM
/* 6000 units -> 2500 units */
void copy_2x2_32_core2_SSE(UInt32* rgbTable, UInt16* pSrc, UInt32* pDst1, UInt32* pDst2, int width, int hint) {

#ifdef __GNUC__
    __asm__(
        "movl   $0,%%esi                \n\t"
        "movl   %4,%%ecx                \n\t"   // width
        "movl   %1,%%eax                \n\t"   // pSrc
        "movl   %2,%%ebx                \n\t"   // pDst1
        "movl   %0,%%edi                \n\t"   // rgbTable
"inner_loop3:                           \n\t"
        "movw   (%%eax),%%si            \n\t"
        "movd   (%%edi,%%esi,4),%%mm0   \n\t"
        "movw   2(%%eax),%%si           \n\t"
        "punpckldq %%mm0,%%mm0          \n\t"
        "movd   (%%edi,%%esi,4),%%mm1   \n\t"
        "movntq %%mm0,0(%%ebx)          \n\t"

        "punpckldq %%mm1,%%mm1          \n\t"
        "movw   4(%%eax),%%si           \n\t"
        "movntq %%mm1,8(%%ebx)          \n\t"

        "movd   (%%edi,%%esi,4),%%mm0   \n\t"
        "punpckldq %%mm0,%%mm0          \n\t"
        "movw   6(%%eax),%%si           \n\t"
        "movntq %%mm0,16(%%ebx)         \n\t"

        "addl   $32,%%ebx               \n\t"
        "movd   (%%edi,%%esi,4),%%mm0   \n\t"
        "addl   $8,%%eax                \n\t"
        "punpckldq %%mm0,%%mm0          \n\t"
        "decl   %%ecx                   \n\t"
        "movntq %%mm0,24-32(%%ebx)      \n\t"

        "jnz    inner_loop3             \n\t"

        //#-- second line

        "movl   %4,%%ecx                \n\t"   // width
        "movl   %1,%%eax                \n\t"   // pSrc
        "movl   %5,%%edx                \n\t"   // hint
        "movl   %3,%%ebx                \n\t"   // pDst2
        "movl   %0,%%edi                \n\t"   // rgbTable
"inner_loop4:                           \n\t"
        "movw   (%%eax),%%si            \n\t"
        "movd   (%%edi,%%esi,4),%%mm0   \n\t"
        "punpckldq %%mm0,%%mm0          \n\t"
        "movw   2(%%eax),%%si           \n\t"
        "movntq %%mm0,0(%%ebx)          \n\t"

        "movd   (%%edi,%%esi,4),%%mm0   \n\t"
        "punpckldq %%mm0,%%mm0          \n\t"
        "movw   4(%%eax),%%si \n\t"
        "movntq %%mm0,8(%%ebx)          \n\t"

        "movd   (%%edi,%%esi,4),%%mm0   \n\t"
        "prefetcht0 (%%eax,%%edx)       \n\t"
        "punpckldq %%mm0,%%mm0          \n\t"
        "movw   6(%%eax),%%si           \n\t"
        "movntq %%mm0,16(%%ebx)         \n\t"

        "addl   $32,%%ebx               \n\t"
        "movd   (%%edi,%%esi,4),%%mm0   \n\t"
        "addl   $8,%%eax                \n\t"
        "punpckldq %%mm0,%%mm0          \n\t"
        "decl   %%ecx                   \n\t"
        "movntq %%mm0,24-32(%%ebx)      \n\t"

        "jnz    inner_loop4             \n\t"

        "emms                           \n\t"
        :
        : "m" (rgbTable), "m" (pSrc), "m" (pDst1), "m" (pDst2), "m" (width), "m" (hint)
        : "%eax", "%ebx", "%ecx", "%edx", "%edi", "%esi"
    );
#else
	__asm{
        mov     esi,0
		mov		ecx,width
		mov		eax,pSrc
		mov		ebx,pDst1
		mov		edi,rgbTable
inner_loop1:
		mov		si,[eax]
		movd	mm0,[edi+esi*4]
		mov		si,[eax+2]
		punpckldq mm0,mm0
		movd	mm1,[edi+esi*4]
		movntq	[ebx+0],mm0

		punpckldq mm1,mm1
		mov		si,[eax+4]
		movntq	[ebx+8],mm1

		movd	mm0,[edi+esi*4]
		punpckldq mm0,mm0
		mov		si,[eax+6]
		movntq	[ebx+16],mm0

		add		ebx,32
		movd	mm0,[edi+esi*4]
		add		eax,8
		punpckldq mm0,mm0
		dec		ecx
		movntq	[ebx+24-32],mm0

		jnz		inner_loop1

		;-- second line

		mov		ecx,width
		mov		eax,pSrc
		mov		edx,hint
		mov		ebx,pDst2
		mov		edi,rgbTable
inner_loop2:
		mov		si,[eax]
		movd	mm0,[edi+esi*4]
		punpckldq mm0,mm0
		mov		si,[eax+2]
		movntq	[ebx+0],mm0

		movd	mm0,[edi+esi*4]
		punpckldq mm0,mm0
		mov		si,[eax+4]
		movntq	[ebx+8],mm0

		movd	mm0,[edi+esi*4]
		prefetcht0 [eax+edx]
		punpckldq mm0,mm0
		mov		si,[eax+6]
		movntq	[ebx+16],mm0

		add		ebx,32
		movd	mm0,[edi+esi*4]
		add		eax,8
		punpckldq mm0,mm0
		dec		ecx
		movntq	[ebx+24-32],mm0

		jnz		inner_loop2

		emms 

	}
#endif

}
#endif

void copy_2x2_32_core2(UInt32* rgbTable, UInt16* pSrc, UInt32* pDst1, UInt32* pDst2, int width, int hint) {

	while (width--) {
        UInt32 col1 = rgbTable[pSrc[0]];
        UInt32 col2 = rgbTable[pSrc[1]];
        UInt32 col3 = rgbTable[pSrc[2]];
        UInt32 col4 = rgbTable[pSrc[3]];
        pSrc  += 4;

        pDst1[0] = col1;
        pDst1[1] = col1;
        pDst1[2] = col2;
        pDst1[3] = col2;
        pDst1[4] = col3;
        pDst1[5] = col3;
        pDst1[6] = col4;
        pDst1[7] = col4;
        pDst1 += 8;

        pDst2[0] = col1;
        pDst2[1] = col1;
        pDst2[2] = col2;
        pDst2[3] = col2;
        pDst2[4] = col3;
        pDst2[5] = col3;
        pDst2[6] = col4;
        pDst2[7] = col4;
        pDst2 += 8;
    }
}

static void copy_2x2_32(FrameBuffer* frame, void* pDestination, int dstPitch, UInt32* rgbTable)
{
    UInt32* pDst1       = (UInt32*)pDestination;
    UInt32* pDst2       = pDst1 + dstPitch / (int)sizeof(UInt32);
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;
    void (*core1) (UInt32*, UInt16*, UInt32*, UInt32* , int , int );
    void (*core2) (UInt32*, UInt16*, UInt32*, UInt32* , int , int );
#ifndef NO_ASM
	int hasSSE=0;
	const int SSEbit=1<<25;

#ifdef __GNUC__
    __asm__ (
        "movl   $1,%%eax            \n\t"
        "cpuid                      \n\t"
        "andl   $0x2000000,%%edx    \n\t"
        "movl   %%edx,%0            \n\t"
        : "=r" (hasSSE) : : "%eax", "%ebx", "%ecx", "%edx"
    );
#else
	__asm {
		mov eax,1
		cpuid
		and edx,SSEbit
		mov hasSSE,edx
	}
#endif
    
    hasSSE = 1;
	core1=hasSSE? copy_2x2_32_core1_SSE: copy_2x2_32_core1;
	core2=hasSSE? copy_2x2_32_core2_SSE: copy_2x2_32_core2;
#else
	core1=copy_2x2_32_core1;
	core2=copy_2x2_32_core2;
#endif

	/*rdtsc_start_timer(0);*/
    dstPitch /= (int)sizeof(UInt32);

    if (frame->interlace == INTERLACE_ODD) {
        pDst1 += dstPitch;
        pDst2 += dstPitch;
        height--;
    }

    for (h = 0; h < height; h++) {

        if (frame->line[h].doubleWidth) 
			core1(rgbTable,frame->line[h].buffer,pDst1,pDst2,srcWidth / 4 * 2,dstPitch * 2*4);
        else 
			core2(rgbTable,frame->line[h].buffer,pDst1,pDst2,srcWidth / 4,dstPitch * 2*4);

        pDst1 += dstPitch * 2;
        pDst2 += dstPitch * 2;
    }
	/*rdtsc_end_timer(0);*/
}

static void copy_2x1_16(FrameBuffer* frame, void* pDestination, int dstPitch, UInt16* rgbTable)
{
    UInt16* pDst1       = (UInt16*)pDestination;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    dstPitch /= (int)sizeof(UInt16);

    if (frame->interlace == INTERLACE_ODD) {
        pDst1 += dstPitch;
        height--;
    }

    for (h = 0; h < height; h++) {
        UInt16* pDst1old = pDst1;
        UInt16* pSrc = frame->line[h].buffer;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth / 4 * 2;
            while (width--) {
                pDst1[0] = rgbTable[pSrc[0]];
                pDst1[1] = rgbTable[pSrc[1]];
                pDst1[2] = rgbTable[pSrc[2]];
                pDst1[3] = rgbTable[pSrc[3]];
                pDst1 += 4;
                pSrc  += 4;
            }
        }
        else {
            int width = srcWidth / 4;
            while (width--) {
                UInt16 col1 = rgbTable[pSrc[0]];
                UInt16 col2 = rgbTable[pSrc[1]];
                UInt16 col3 = rgbTable[pSrc[2]];
                UInt16 col4 = rgbTable[pSrc[3]];
                pSrc  += 4;

                pDst1[0] = col1;
                pDst1[1] = col1;
                pDst1[2] = col2;
                pDst1[3] = col2;
                pDst1[4] = col3;
                pDst1[5] = col3;
                pDst1[6] = col4;
                pDst1[7] = col4;
                pDst1 += 8;
            }
        }

        pDst1 = pDst1old + dstPitch;
    }
}

static void copy_2x1_32(FrameBuffer* frame, void* pDestination, int dstPitch, UInt32* rgbTable)
{
    UInt32* pDst1       = (UInt32*)pDestination;
    int height          = frame->lines;
    int srcWidth        = frame->maxWidth;
    int h;

    dstPitch /= (int)sizeof(UInt32);

    if (frame->interlace == INTERLACE_ODD) {
        pDst1 += dstPitch;
        height--;
    }

    for (h = 0; h < height; h++) {
        UInt32* pDst1old = pDst1;
        UInt16* pSrc = frame->line[h].buffer;

        if (frame->line[h].doubleWidth) {
            int width = srcWidth / 4 * 2;
            while (width--) {
                pDst1[0] = rgbTable[pSrc[0]];
                pDst1[1] = rgbTable[pSrc[1]];
                pDst1[2] = rgbTable[pSrc[2]];
                pDst1[3] = rgbTable[pSrc[3]];
                pDst1 += 4;
                pSrc  += 4;
            }
        }
        else {
            int width = srcWidth / 4;
            while (width--) {
                UInt32 col1 = rgbTable[pSrc[0]];
                UInt32 col2 = rgbTable[pSrc[1]];
                UInt32 col3 = rgbTable[pSrc[2]];
                UInt32 col4 = rgbTable[pSrc[3]];
                pSrc  += 4;

                pDst1[0] = col1;
                pDst1[1] = col1;
                pDst1[2] = col2;
                pDst1[3] = col2;
                pDst1[4] = col3;
                pDst1[5] = col3;
                pDst1[6] = col4;
                pDst1[7] = col4;
                pDst1 += 8;
            }
        }

        pDst1 = pDst1old + dstPitch;
    }
}



static void hq2x_2x2_32(FrameBuffer* frame, void* pDestination, int dstPitch, UInt16* rgbTable)
{
	UInt16  ImgSrc[320 * 240];
    UInt16* pDst        = (UInt16*)ImgSrc;
    int srcHeight       = frame->lines;
    int srcWidth        = frame->maxWidth;
	int h;

    if (srcWidth == 0) {
        return;
    }  

    for (h = 0; h < srcHeight; h++) {
        UInt16* pSrc = frame->line[h].buffer;
        int width = srcWidth / 8;
        while (width--) {
            pDst[0] = rgbTable[pSrc[0]];
            pDst[1] = rgbTable[pSrc[1]];
            pDst[2] = rgbTable[pSrc[2]];
            pDst[3] = rgbTable[pSrc[3]];
            pDst[4] = rgbTable[pSrc[4]];
            pDst[5] = rgbTable[pSrc[5]];
            pDst[6] = rgbTable[pSrc[6]];
            pDst[7] = rgbTable[pSrc[7]];
            pSrc += 8;
            pDst += 8;
        }
    }

    hq2x_32(ImgSrc, pDestination, srcWidth, srcHeight, dstPitch);
}

static void hq3x_2x2_32(FrameBuffer* frame, void* pDestination, int dstPitch, UInt16* rgbTable)
{
	UInt16  ImgSrc[320 * 240];
    UInt16* pDst        = (UInt16*)ImgSrc;
    int srcHeight       = frame->lines;
    int srcWidth        = frame->maxWidth;
	int h;

    if (srcWidth == 0) {
        return;
    }  

    for (h = 0; h < srcHeight; h++) {
        UInt16* pSrc = frame->line[h].buffer;
        int width = srcWidth / 8;
        while (width--) {
            pDst[0] = rgbTable[pSrc[0]];
            pDst[1] = rgbTable[pSrc[1]];
            pDst[2] = rgbTable[pSrc[2]];
            pDst[3] = rgbTable[pSrc[3]];
            pDst[4] = rgbTable[pSrc[4]];
            pDst[5] = rgbTable[pSrc[5]];
            pDst[6] = rgbTable[pSrc[6]];
            pDst[7] = rgbTable[pSrc[7]];
            pSrc += 8;
            pDst += 8;
        }
    }

    hq3x_32(ImgSrc, pDestination, srcWidth, srcHeight, dstPitch);
}

static void scale2x_2x2_32(FrameBuffer* frame, void* pDestination, int dstPitch, UInt32* rgbTable)
{
	UInt32  ImgSrc[320 * 240];
    UInt32* pDst        = (UInt32*)ImgSrc;
    int srcHeight       = frame->lines;
    int srcWidth        = frame->maxWidth;
	int h;

    if (srcWidth == 0) {
        return;
    }

    for (h = 0; h < srcHeight; h++) {
        UInt16* pSrc = frame->line[h].buffer;
        int width = srcWidth / 8;
        while (width--) {
            pDst[0] = rgbTable[pSrc[0]];
            pDst[1] = rgbTable[pSrc[1]];
            pDst[2] = rgbTable[pSrc[2]];
            pDst[3] = rgbTable[pSrc[3]];
            pDst[4] = rgbTable[pSrc[4]];
            pDst[5] = rgbTable[pSrc[5]];
            pDst[6] = rgbTable[pSrc[6]];
            pDst[7] = rgbTable[pSrc[7]];
            pSrc += 8;
            pDst += 8;
        }
    }

    scale(2, pDestination, dstPitch, ImgSrc, srcWidth * sizeof(UInt32), sizeof(UInt32), srcWidth, srcHeight);
}

static void scale2x_2x2_16(FrameBuffer* frame, void* pDestination, int dstPitch, UInt16* rgbTable)
{
	UInt16  ImgSrc[320 * 240];
    UInt16* pDst        = (UInt16*)ImgSrc;
    int srcHeight       = frame->lines;
    int srcWidth        = frame->maxWidth;
	int h;

    if (srcWidth == 0) {
        return;
    }

    for (h = 0; h < srcHeight; h++) {
        UInt16* pSrc = frame->line[h].buffer;
        int width = srcWidth / 8;
        while (width--) {
            pDst[0] = rgbTable[pSrc[0]];
            pDst[1] = rgbTable[pSrc[1]];
            pDst[2] = rgbTable[pSrc[2]];
            pDst[3] = rgbTable[pSrc[3]];
            pDst[4] = rgbTable[pSrc[4]];
            pDst[5] = rgbTable[pSrc[5]];
            pDst[6] = rgbTable[pSrc[6]];
            pDst[7] = rgbTable[pSrc[7]];
            pSrc += 8;
            pDst += 8;
        }
    }

    scale(2, pDestination, dstPitch, ImgSrc, srcWidth * sizeof(UInt16), sizeof(UInt16), srcWidth, srcHeight);
}

/*****************************************************************************
**
** Public interface methods
**
******************************************************************************
*/
Video* videoCreate() 
{
    Video* pVideo = (Video*)calloc(1, sizeof(Video));

    pVideo->gamma       = 1;
    pVideo->saturation  = 1;
    pVideo->brightness  = 0;
    pVideo->contrast    = 1;
    pVideo->deInterlace = 0;
    pVideo->invertRGB   = 0;

    initRGBTable(pVideo);

    hq2x_init();
    hq3x_init();

    pVideo->palMode = VIDEO_PAL_FAST;
    pVideo->pRgbTable16 = pRgbTableColor16;
    pVideo->pRgbTable32 = pRgbTableColor32;

    return pVideo;
}

void videoDestroy(Video* pVideo) 
{
    free(pVideo);
}

void videoSetDeInterlace(Video* pVideo, int deInterlace)
{
    pVideo->deInterlace = deInterlace;
}

void videoSetBlendFrames(Video* pVideo, int blendFrames)
{
    frameBufferSetBlendFrames(blendFrames);
}

void videoSetColors(Video* pVideo, int saturation, int brightness, int contrast, int gamma)
{
    pVideo->gamma      = 1 + (MAX(0, MIN(200, gamma)) - 100) / 500.;
    pVideo->saturation = MAX(0, MIN(200, saturation)) / 100.;
    pVideo->brightness = MAX(0, MIN(200, brightness)) - 100.;
    pVideo->contrast   = MAX(0, MIN(200, contrast)) / 100.;

    initRGBTable(pVideo);
}

void videoSetColorMode(Video* pVideo, VideoColorMode colorMode) 
{
    switch (colorMode) {
    case VIDEO_GREEN:
        pVideo->pRgbTable16 = pRgbTableGreen16;
        pVideo->pRgbTable32 = pRgbTableGreen32;
        break;
    case VIDEO_BLACKWHITE:
        pVideo->pRgbTable16 = pRgbTableWhite16;
        pVideo->pRgbTable32 = pRgbTableWhite32;
        break;
    case VIDEO_AMBER:
        pVideo->pRgbTable16 = pRgbTableAmber16;
        pVideo->pRgbTable32 = pRgbTableAmber32;
        break;
    case VIDEO_COLOR:
    default:
        pVideo->pRgbTable16 = pRgbTableColor16;
        pVideo->pRgbTable32 = pRgbTableColor32;
        break;
    }
}

void videoSetPalMode(Video* pVideo, VideoPalMode palMode)
{
    pVideo->palMode = palMode;
}

void videoSetRgbMode(Video* pVideo, int inverted)
{
    int recalculate = pVideo->invertRGB != inverted;

    pVideo->invertRGB = inverted;

    if (recalculate) {
        initRGBTable(pVideo);
    }
}


void videoSetScanLines(Video* pVideo, int enable, int scanLinesPct)
{
    pVideo->scanLinesEnable = enable;
    pVideo->scanLinesPct    = scanLinesPct;
}

void videoSetColorSaturation(Video* pVideo, int enable, int width)
{
    pVideo->colorSaturationEnable = enable;
    pVideo->colorSaturationWidth  = width;
}

void videoUpdateAll(Video* video, Properties* properties) 
{
    videoSetColors(video, properties->video.saturation, properties->video.brightness, properties->video.contrast, properties->video.gamma);
    videoSetScanLines(video, properties->video.scanlinesEnable, properties->video.scanlinesPct);
    videoSetColorSaturation(video, properties->video.colorSaturationEnable, properties->video.colorSaturationWidth);
    videoSetDeInterlace(video, properties->video.deInterlace);

    switch (properties->video.monitorColor) {
    case P_VIDEO_COLOR:
        videoSetColorMode(video, VIDEO_COLOR);
        break;
    case P_VIDEO_BW:
        videoSetColorMode(video, VIDEO_BLACKWHITE);
        break;
    case P_VIDEO_GREEN:
        videoSetColorMode(video, VIDEO_GREEN);
        break;
    case P_VIDEO_AMBER:
        videoSetColorMode(video, VIDEO_AMBER);
        break;
    }

    switch (properties->video.monitorType) {
    case P_VIDEO_PALNONE:
        videoSetPalMode(video, VIDEO_PAL_FAST);
        break;
    case P_VIDEO_PALMON:
        videoSetPalMode(video, VIDEO_PAL_MONITOR);
        break;
    case P_VIDEO_PALYC:
        videoSetPalMode(video, VIDEO_PAL_SHARP);
        break;
    case P_VIDEO_PALNYC:
        videoSetPalMode(video, VIDEO_PAL_SHARP_NOISE);
        break;
    case P_VIDEO_PALCOMP:
        videoSetPalMode(video, VIDEO_PAL_BLUR);
        break;
    case P_VIDEO_PALNCOMP:
        videoSetPalMode(video, VIDEO_PAL_BLUR_NOISE);
        break;
	case P_VIDEO_PALSCALE2X:
		videoSetPalMode(video, VIDEO_PAL_SCALE2X);
		break;
	case P_VIDEO_PALHQ2X:
		videoSetPalMode(video, VIDEO_PAL_HQ2X);
		break;
    }
}


void colorSaturation_16(void* pBuffer, int width, int height, int pitch, int blur)
{
    // This can be made more efficient by using 32 bit arithmetic and calculate
    // two pixels at the time....
    UInt16* pBuf = (UInt16*)pBuffer;
    int w, h;

    pitch  /= (int)sizeof(UInt16);

    switch (blur) {
    case 0:
        break;

    case 1:
        for (h = 0; h < height; h++) {
            UInt16 p0 = pBuf[0] & 0xf7de;
            for (w = 1; w < width; w++) {
                UInt16 p1 = pBuf[w] & 0xf7de;
                pBuf[w] = (UInt16)((p0 & 0x07e0) | (((p0 + p1) / 2) & 0x001f) | (p1 & 0xf800));
                p0 = p1;
            }
            pBuf += pitch;
        }
        break;

    case 2:
        for (h = 0; h < height; h++) {
            UInt16 p0 = pBuf[0];
            UInt16 p1 = pBuf[1];
            for (w = 2; w < width; w++) {
                UInt16 p2 = pBuf[w];
                pBuf[w] = (UInt16)((p0 & 0x07e0) | (p1 & 0x001f) | (((p1 + p2) / 2) & 0xf800));
                p0 = p1;
                p1 = p2;
            }
            pBuf += pitch;
        }
        break;

    case 3:
        for (h = 0; h < height; h++) {
            UInt16 p0 = pBuf[0];
            UInt16 p1 = pBuf[1];
            for (w = 2; w < width; w++) {
                UInt16 p2 = pBuf[w];
                pBuf[w] = (UInt16)((p0 & 0x07e0) | (p1 & 0x001f) | (p2 & 0xf800));
                p0 = p1;
                p1 = p2;
            }
            pBuf += pitch;
        }
        break;

    case 4:
        for (h = 0; h < height; h++) {
            UInt16 p0 = pBuf[0] & 0xf7de;
            UInt16 p1 = pBuf[1] & 0xf7de;
            UInt16 p2 = pBuf[2] & 0xf7de;
            for (w = 3; w < width; w++) {
                UInt16 p3 = pBuf[w] & 0xf7de;;
                pBuf[w] = (UInt16)((((p0 + p1) / 2) & 0x07e0) | (p2 & 0x001f) | (p3 & 0xf800));
                p0 = p1;
                p1 = p2;
                p2 = p3;
            }
            pBuf += pitch;
        }
        break;
    }
}

void colorSaturation_32(void* pBuffer, int width, int height, int pitch, int blur)
{
    UInt32* pBuf = (UInt32*)pBuffer;
    int w, h;

    pitch  /= (int)sizeof(UInt32);

    switch (blur) {
    case 0:
        break;

    case 1:
        for (h = 0; h < height; h++) {
            UInt32 p0 = pBuf[0] & 0xfefefe;
            for (w = 1; w < width; w++) {
                UInt32 p1 = pBuf[w] & 0xfefefe;
                pBuf[w] = (p0 & 0x00ff00) | (((p0 + p1) / 2) & 0x0000ff) | (p1 & 0xff0000);
                p0 = p1;
            }
            pBuf += pitch;
        }
        break;

    case 2:
        for (h = 0; h < height; h++) {
            UInt32 p0 = pBuf[0];
            UInt32 p1 = pBuf[1];
            for (w = 2; w < width; w++) {
                UInt32 p2 = pBuf[w];
                pBuf[w] = (p0 & 0x00ff00) | (p1 & 0x0000ff) | (((p1 + p2) / 2) & 0xff0000);
                p0 = p1;
                p1 = p2;
            }
            pBuf += pitch;
        }
        break;

    case 3:
        for (h = 0; h < height; h++) {
            UInt32 p0 = pBuf[0];
            UInt32 p1 = pBuf[1];
            for (w = 2; w < width; w++) {
                UInt32 p2 = pBuf[w];
                pBuf[w] = (p0 & 0x00ff00) | (p1 & 0x0000ff) | (p2 & 0xff0000);
                p0 = p1;
                p1 = p2;
            }
            pBuf += pitch;
        }
        break;

    case 4:
        for (h = 0; h < height; h++) {
            UInt32 p0 = pBuf[0] & 0xfefefe;
            UInt32 p1 = pBuf[1] & 0xfefefe;
            UInt32 p2 = pBuf[2] & 0xfefefe;
            for (w = 3; w < width; w++) {
                UInt32 p3 = pBuf[w] & 0xfefefe;;
                pBuf[w] = (((p0 + p1) / 2) & 0x00ff00) | (p2 & 0x0000ff) | (p3 & 0xff0000);
                p0 = p1;
                p1 = p2;
                p2 = p3;
            }
            pBuf += pitch;
        }
        break;
    }
}

void scanLines_16(void* pBuffer, int width, int height, int pitch, int scanLinesPct)
{
    UInt32* pBuf = (UInt32*)pBuffer;
    int w, h;

    if (scanLinesPct == 100) {
        return;
    }

    pitch = pitch * 2 / (int)sizeof(UInt32);
    scanLinesPct = scanLinesPct * 32 / 100;
    height /= 2;
    width /= 2;

    if (scanLinesPct == 0) {
        for (h = 0; h < height; h++) {
            memset(pBuf, 0, width * sizeof(UInt32));
            pBuf += pitch;
        }
        return;
    }

    for (h = 0; h < height; h++) {
        for (w = 0; w < width; w++) {
            UInt32 pixel = pBuf[w];
            UInt32 a = (((pixel & 0x07e0f81f) * scanLinesPct) & 0xfc1f03e0) >> 5;
            UInt32 b = (((pixel >> 5) & 0x07c0f83f) * scanLinesPct) & 0xf81f07e0;
            pBuf[w] = a | b;
        }
        pBuf += pitch;
    }
}

void scanLines_32_core(UInt32* pBuf, int width, int scanLinesPct, int hint) {
	int w;
        for (w = 0; w < width; w++) {
            UInt32 pixel = pBuf[w];
            UInt32 a = (((pixel & 0xff00ff) * scanLinesPct) & 0xff00ff00) >> 8;
            UInt32 b  = (((pixel >> 8)& 0xff00ff) * scanLinesPct) & 0xff00ff00;
            pBuf[w] = a | b;
        }
}

#ifndef NO_ASM
void scanLines_32_core_SSE(UInt32* pBuf, int width, int scanLinesPct, int hint) {

#ifdef __GNUC__
    __asm__ (
        "movl   %1,%%ecx            \n\t"   // width
        "shrl   $2,%%ecx            \n\t"
        "movl   %0,%%eax            \n\t"   // pbuf
        "movl   %3,%%ebx            \n\t"   // hint
        "pxor   %%mm0,%%mm0         \n\t"
        "movd   %2,%%mm1            \n\t"   // scanLinesPct
        "punpcklwd %%mm1,%%mm1      \n\t"
        "punpckldq %%mm1,%%mm1      \n\t"
        "psllw  $8,%%mm1            \n\t"
"inner_loop5:                       \n\t"
        "movq   (%%eax),%%mm2       \n\t"
        "movq   8(%%eax),%%mm4      \n\t"
        "movq   %%mm2,%%mm3         \n\t"
        "punpcklbw %%mm0,%%mm2      \n\t"
        "movq   %%mm4,%%mm5         \n\t"
        "pmulhuw %%mm1,%%mm2        \n\t"
        "punpcklbw %%mm0,%%mm4      \n\t"
        "punpckhbw %%mm0,%%mm3      \n\t"
        "prefetcht1 (%%eax,%%ebx)   \n\t"
        "pmulhuw %%mm1,%%mm4        \n\t"
        "punpckhbw %%mm0,%%mm5      \n\t"
        "addl   $16,%%eax           \n\t"
        "pmulhuw %%mm1,%%mm3        \n\t"
        "pmulhuw %%mm1,%%mm5        \n\t"
        "packuswb %%mm3,%%mm2       \n\t"
        "packuswb %%mm5,%%mm4       \n\t"
        "movq   %%mm2,-16(%%eax)    \n\t"
        "decl   %%ecx               \n\t"
        "movq   %%mm4,-8(%%eax)     \n\t"

        "jnz    inner_loop5         \n\t"
        "emms                       \n\t"
        :
        : "m" (pBuf), "m" (width), "m" (scanLinesPct), "m" (hint)
        : "%eax", "%ebx", "%ecx"
	);
#else
	__asm {
		mov		ecx,width
		shr		ecx,2
		mov		eax,pBuf
		mov		ebx,hint
		pxor	mm0,mm0
		movd	mm1,scanLinesPct
		punpcklwd mm1,mm1
		punpckldq mm1,mm1
		psllw	mm1,8
inner_loop:
		movq	mm2,[eax]
		movq	mm4,[eax+8]
		movq	mm3,mm2
		punpcklbw mm2,mm0
		movq	mm5,mm4
		pmulhuw	mm2,mm1
		punpcklbw mm4,mm0
		punpckhbw mm3,mm0
		prefetcht1 [eax+ebx]
		pmulhuw	mm4,mm1
		punpckhbw mm5,mm0
		add		eax,16
		pmulhuw	mm3,mm1
		pmulhuw	mm5,mm1
		packuswb mm2,mm3
		packuswb mm4,mm5
		movq	[eax-16],mm2
		dec		ecx
		movq	[eax-8],mm4

		jnz		inner_loop
		emms
	}
#endif

}
#endif

/* 5500 units -> 2500 units */
void scanLines_32(void* pBuffer, int width, int height, int pitch, int scanLinesPct)
{
    UInt32* pBuf = (UInt32*)pBuffer;
    int h;
    void (*core) (UInt32* , int , int , int ) ;

#ifndef NO_ASM
	int hasSSE=0;
	const int SSEbit=1<<25;

#ifdef __GNUC__
    __asm__ (
        "movl   $1,%%eax            \n\t"
        "cpuid                      \n\t"
        "andl   $0x2000000,%%edx    \n\t"
        "movl   %%edx,%0            \n\t"
        : "=r" (hasSSE) : : "%eax", "%ebx", "%ecx", "%edx"
    );
#else
	__asm {
		mov eax,1
		cpuid
		and edx,SSEbit
		mov hasSSE,edx
	}
#endif

    hasSSE = 1;
	core=hasSSE? scanLines_32_core_SSE: scanLines_32_core;
	/*rdtsc_start_timer(0);*/
#else
	core=scanLines_32_core;
#endif

    if (scanLinesPct == 100) {
  	    /*rdtsc_end_timer(0);*/
        return;
    }

    pitch = pitch * 2 / (int)sizeof(UInt32);
    scanLinesPct = scanLinesPct * 255 / 100;
    height /= 2;

    if (scanLinesPct == 0) {
        for (h = 0; h < height; h++) {
            memset(pBuf, 0, width * sizeof(UInt32));
            pBuf += pitch;
        }
        /*rdtsc_end_timer(0);*/
		return;
    }

    for (h = 0; h < height; h++) {
		core (pBuf,width,scanLinesPct,pitch*4);
        pBuf += pitch;
    }
	/*rdtsc_end_timer(0);*/
}

static int videoRender240(Video* pVideo, FrameBuffer* frame, int bitDepth, int zoom, 
                          void* pDst, int dstOffset, int dstPitch, int canChangeZoom)
{
    pDst = (char*)pDst + zoom * dstOffset;

    switch (bitDepth) {
    case 16:
        switch (pVideo->palMode) {
        case VIDEO_PAL_FAST:
            if (zoom == 2) {
                if (pVideo->scanLinesEnable || pVideo->colorSaturationEnable || canChangeZoom == 0) {
                    copy_2x2_16(frame, pDst, dstPitch, pVideo->pRgbTable16);
                }
                else {
                    int h = frame->lines;
                    while (--h >= 0 && !frame->line[h].doubleWidth);
                    if (h) copy_2x2_16(frame, pDst, dstPitch, pVideo->pRgbTable16);
                    else {
                        copy_1x1_16(frame, pDst, dstPitch, pVideo->pRgbTable16);
                        zoom = 1;
                    }
                }
            }
            else copy_1x1_16(frame, pDst, dstPitch, pVideo->pRgbTable16);
            break;
        case VIDEO_PAL_MONITOR:
            if (zoom == 2) copyMonitorPAL_2x2_16(frame, pDst, dstPitch, pVideo->pRgbTable16, 0);
            else           copyPAL_1x1_16(frame, pDst, dstPitch, pVideo->pRgbTable16, 0);
            break;
        case VIDEO_PAL_SHARP:
            if (zoom == 2) copySharpPAL_2x2_16(frame, pDst, dstPitch, pVideo->pRgbTable16, 0);
            else           copyPAL_1x1_16(frame, pDst, dstPitch, pVideo->pRgbTable16, 0);
            break;
        case VIDEO_PAL_SHARP_NOISE:
            if (zoom == 2) copySharpPAL_2x2_16(frame, pDst, dstPitch, pVideo->pRgbTable16, 1);
            else           copyPAL_1x1_16(frame, pDst, dstPitch, pVideo->pRgbTable16, 1);
            break;
        case VIDEO_PAL_BLUR:
            if (zoom == 2) copyMonitorPAL_2x2_16(frame, pDst, dstPitch, pVideo->pRgbTable16, 0);
            else           copyPAL_1x1_16(frame, pDst, dstPitch, pVideo->pRgbTable16, 0);
            break;
        case VIDEO_PAL_BLUR_NOISE:
            if (zoom == 2) copyMonitorPAL_2x2_16(frame, pDst, dstPitch, pVideo->pRgbTable16, 1);
            else           copyPAL_1x1_16(frame, pDst, dstPitch, pVideo->pRgbTable16, 1);
            break;
		case VIDEO_PAL_HQ2X: // Can't do 16bit hq2x so just use scale2x instead
		case VIDEO_PAL_SCALE2X:
            if (zoom==2) {
                if (frame->line[0].doubleWidth == 0 && frame->interlace == INTERLACE_NONE) {
                    scale2x_2x2_16(frame, pDst, dstPitch, pVideo->pRgbTable16);
                }
                else {
                    copy_2x2_16(frame, pDst, dstPitch, pVideo->pRgbTable16);
                }
            }
            else {
                copy_1x1_16(frame, pDst, dstPitch, pVideo->pRgbTable16);
            }
            break;
        }
        break;
    case 32:
        switch (pVideo->palMode) {
        case VIDEO_PAL_FAST:
            if (zoom == 2) {
                if (pVideo->scanLinesEnable || pVideo->colorSaturationEnable || canChangeZoom == 0) {
                    copy_2x2_32(frame, pDst, dstPitch, pVideo->pRgbTable32);
                }
                else {
                    int h = frame->lines;
                    while (--h >= 0 && !frame->line[h].doubleWidth);
                    if (h) copy_2x2_32(frame, pDst, dstPitch, pVideo->pRgbTable32);
                    else {
                        copy_1x1_32(frame, pDst, dstPitch, pVideo->pRgbTable32);
                        zoom = 1;
                    }
                }
            }
            else           copy_1x1_32(frame, pDst, dstPitch, pVideo->pRgbTable32);
            break;
        case VIDEO_PAL_MONITOR:
            if (zoom == 2) copyMonitorPAL_2x2_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 0);
            else           copyPAL_1x1_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 0);
            break;
        case VIDEO_PAL_SHARP:
            if (zoom == 2) copySharpPAL_2x2_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 0);
            else           copy_1x1_32(frame, pDst, dstPitch, pVideo->pRgbTable32);
            break;
        case VIDEO_PAL_SHARP_NOISE:
            if (zoom == 2) copySharpPAL_2x2_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 1);
            else           copyPAL_1x1_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 1);
            break;
        case VIDEO_PAL_BLUR:
            if (zoom == 2) copyPAL_2x2_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 0);
            else           copyPAL_1x1_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 0);
            break;
        case VIDEO_PAL_BLUR_NOISE:
            if (zoom == 2) copyPAL_2x2_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 1);
            else           copyPAL_1x1_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 1);
            break;
		case VIDEO_PAL_SCALE2X:
            if (zoom==2) {
                if (frame->line[0].doubleWidth == 0 && frame->interlace == INTERLACE_NONE) {
                    scale2x_2x2_32(frame, pDst, dstPitch, pVideo->pRgbTable32);
                }
                else {
                    copy_2x2_32(frame, pDst, dstPitch, pVideo->pRgbTable32);
                }
            }
            else {
                copy_1x1_32(frame, pDst, dstPitch, pVideo->pRgbTable32);
            }
            break;
		case VIDEO_PAL_HQ2X:
            if (zoom==2) {
                if (frame->line[0].doubleWidth == 0 && frame->interlace == INTERLACE_NONE) {
                    if (canChangeZoom > 0) {
                        pDst = (char*)pDst + dstOffset;
                        hq3x_2x2_32(frame, pDst, dstPitch, pVideo->pRgbTable16);
                        zoom =3;
                    }
                    else {
                        hq2x_2x2_32(frame, pDst, dstPitch, pVideo->pRgbTable16);
                    }
                }
                else {
                    copy_2x2_32(frame, pDst, dstPitch, pVideo->pRgbTable32);
                }
            }
            else {
                copy_1x1_32(frame, pDst, dstPitch, pVideo->pRgbTable32);
            }
            break;
        }
        break;
    }
    return zoom;
}

static int videoRender480(Video* pVideo, FrameBuffer* frame, int bitDepth, int zoom, 
                          void* pDst, int dstOffset, int dstPitch, int canChangeZoom)
{
    pDst = (char*)pDst + zoom * dstOffset;

    switch (bitDepth) {
    case 16:
        switch (pVideo->palMode) {
        default:
        case VIDEO_PAL_FAST:
		case VIDEO_PAL_SCALE2X:
		case VIDEO_PAL_HQ2X:
            if (zoom == 2) copy_2x1_16(frame, pDst, dstPitch, pVideo->pRgbTable16);
            else           copy_1x05_16(frame, pDst, dstPitch, pVideo->pRgbTable16);
            break;
        case VIDEO_PAL_MONITOR:
            if (zoom == 2) copyMonitorPAL_2x1_16(frame, pDst, dstPitch, pVideo->pRgbTable16, 1);
            else           copy_1x05_16(frame, pDst, dstPitch, pVideo->pRgbTable16);
            break;
        case VIDEO_PAL_SHARP:
            if (zoom == 2) copySharpPAL_2x1_16(frame, pDst, dstPitch, pVideo->pRgbTable16, 0);
            else           copy_1x05_16(frame, pDst, dstPitch, pVideo->pRgbTable16);
            break;
        case VIDEO_PAL_SHARP_NOISE:
            if (zoom == 2) copySharpPAL_2x1_16(frame, pDst, dstPitch, pVideo->pRgbTable16, 1);
            else           copy_1x05_16(frame, pDst, dstPitch, pVideo->pRgbTable16);
            break;
        case VIDEO_PAL_BLUR:
            if (zoom == 2) copyMonitorPAL_2x1_16(frame, pDst, dstPitch, pVideo->pRgbTable16, 0);
            else           copy_1x05_16(frame, pDst, dstPitch, pVideo->pRgbTable16);
            break;
        case VIDEO_PAL_BLUR_NOISE:
            if (zoom == 2) copyMonitorPAL_2x1_16(frame, pDst, dstPitch, pVideo->pRgbTable16, 1);
            else           copy_1x05_16(frame, pDst, dstPitch, pVideo->pRgbTable16);
            break;
        }
        break;
    case 32:
        switch (pVideo->palMode) {
        default:
        case VIDEO_PAL_FAST:
		case VIDEO_PAL_SCALE2X:
		case VIDEO_PAL_HQ2X:
            if (zoom == 2) copy_2x1_32(frame, pDst, dstPitch, pVideo->pRgbTable32);
            else           copy_1x05_32(frame, pDst, dstPitch, pVideo->pRgbTable32);
            break;
        case VIDEO_PAL_MONITOR:
            if (zoom == 2) copyMonitorPAL_2x1_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 1);
            else           copyPAL_1x05_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 0);
            break;
        case VIDEO_PAL_SHARP:
            if (zoom == 2) copySharpPAL_2x1_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 0);
            else           copy_1x05_32(frame, pDst, dstPitch, pVideo->pRgbTable32);
            break;
        case VIDEO_PAL_SHARP_NOISE:
            if (zoom == 2) copySharpPAL_2x1_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 1);
            else           copyPAL_1x05_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 1);
            break;
        case VIDEO_PAL_BLUR:
            if (zoom == 2) copyPAL_2x1_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 0);
            else           copyPAL_1x05_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 0);
            break;
        case VIDEO_PAL_BLUR_NOISE:
            if (zoom == 2) copyPAL_2x1_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 1);
            else           copyPAL_1x05_32(frame, pDst, dstPitch, pVideo->pRgbTable32, 1);
            break;
        }
        break;
    }
    return zoom;
}

int videoRender(Video* pVideo, FrameBuffer* frame, int bitDepth, int zoom, 
                void* pDst, int dstOffset, int dstPitch, int canChangeZoom)
{
    if (frame == NULL) {
        return zoom;
    }

    if (frame->interlace != INTERLACE_NONE && pVideo->deInterlace) {
        frame = frameBufferDeinterlace(frame);
    }

    if (frame->lines <= 240) {
        zoom = videoRender240(pVideo, frame, bitDepth, zoom, pDst, dstOffset, dstPitch, canChangeZoom);
    }
    else {
        zoom = videoRender480(pVideo, frame, bitDepth, zoom, pDst, dstOffset, dstPitch, canChangeZoom);
    }

    switch (bitDepth) {
    case 16:
        if (pVideo->colorSaturationEnable) {
            colorSaturation_16(pDst, 320 * zoom, 240 * zoom, dstPitch, pVideo->colorSaturationWidth);
        }

        if (pVideo->scanLinesEnable) {
            scanLines_16(pDst, 320 * zoom, 240 * zoom, dstPitch, pVideo->scanLinesPct);
        }

        break;
    case 32:
        if (pVideo->colorSaturationEnable) {
            colorSaturation_32(pDst, 320 * zoom, 240 * zoom, dstPitch, pVideo->colorSaturationWidth);
        }

        if (pVideo->scanLinesEnable) {
            scanLines_32(pDst, 320 * zoom, 240 * zoom, dstPitch, pVideo->scanLinesPct);
        }
        break;
    }

    return zoom;
}
#endif
