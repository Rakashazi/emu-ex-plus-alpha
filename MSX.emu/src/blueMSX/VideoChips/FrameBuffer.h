/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/VideoChips/FrameBuffer.h,v $
**
** $Revision: 1.27 $
**
** $Date: 2009-07-18 14:35:59 $
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
#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include <Config/VideoChips.h>
#include "../Common/MsxTypes.h"

#ifdef WII
#define FB_MAX_LINE_WIDTH 544
#else
#define FB_MAX_LINE_WIDTH 640
#endif
#define FB_MAX_LINES      480

#ifndef NO_FRAMEBUFFER
typedef struct {
    int doubleWidth; // 1 when normal, 2 when 2 src pixels per dest pixel
    UInt16 buffer[FB_MAX_LINE_WIDTH];
} LineBuffer;
#endif

typedef enum { INTERLACE_NONE, INTERLACE_ODD, INTERLACE_EVEN } InterlaceMode;

#ifdef NO_FRAMEBUFFER
typedef void FrameBuffer;
#else
typedef struct {
    int age;           // Internal use
    InterlaceMode interlace;
    int maxWidth;
    int lines;         // Number of lines in frame buffer
    LineBuffer line[FB_MAX_LINES];
} FrameBuffer;
#endif

typedef struct FrameBufferData FrameBufferData;

typedef enum {
    MIXMODE_INTERNAL = 1,
    MIXMODE_BOTH     = 2,
    MIXMODE_EXTERNAL = 4,
    MIXMODE_NONE     = 8,
    MIXMODE_ALL      = MIXMODE_INTERNAL | MIXMODE_BOTH | MIXMODE_EXTERNAL | MIXMODE_NONE
} FrameBufferMixMode;

void frameBufferSetFrameCount(int frameCount);

FrameBuffer* frameBufferGetViewFrame();
FrameBuffer* frameBufferGetDrawFrame();
FrameBuffer* frameBufferFlipViewFrame(int mixFrames);
FrameBuffer* frameBufferFlipDrawFrame();

void frameBufferSetScanline(int scanline);
int frameBufferGetScanline(FrameBuffer* frameBuffer);

FrameBuffer* frameBufferGetWhiteNoiseFrame();
FrameBuffer* frameBufferDeinterlace(FrameBuffer* frameBuffer);
void frameBufferClearDeinterlace();

FrameBufferData* frameBufferDataCreate(int maxWidth, int maxHeight, int defaultHorizZoom);
void frameBufferDataDestroy(FrameBufferData* frameData);

void frameBufferSetActive(FrameBufferData* frameData);
void frameBufferSetMixMode(FrameBufferMixMode mode, FrameBufferMixMode mask);

FrameBufferData* frameBufferGetActive();

void frameBufferSetBlendFrames(int blendFrames);

#ifdef WII
#define BKMODE_TRANSPARENT 0x0020
#define videoGetColor(R, G, B) \
          ((((int)(R) >> 3) << 11) | (((int)(G) >> 3) << 6) | ((int)(B) >> 3))
#else
#if defined(VIDEO_COLOR_TYPE_RGB565)
#define BKMODE_TRANSPARENT 0x0000
#define videoGetColor(R, G, B) \
		((((int)(R) >> 3) << 11) | (((int)(G) >> 2) << 5) | ((int)(B) >> 3))
#elif defined(VIDEO_COLOR_TYPE_RGBA5551)
#define BKMODE_TRANSPARENT 0x0001
#define videoGetColor(R, G, B) \
		((((int)(R) >> 3) << 11) | (((int)(G) >> 3) << 6) | (((int)(B) >> 3) << 1))
#elif defined(VIDEO_COLOR_TYPE_RGBA8888)
#define BKMODE_TRANSPARENT 0x0000
#define videoGetColor(R, G, B) \
		(((int)(R) << 16) | ((int)(G) << 8) | ((int)(B) << 0))
#else // default is ARGB1555
#define BKMODE_TRANSPARENT 0x8000
#define videoGetColor(R, G, B) \
		((((int)(R) >> 3) << 10) | (((int)(G) >> 3) << 5) | ((int)(B) >> 3))
#endif // VIDEO_COLOR_TYPE

#endif // WII
#define videoGetTransparentColor() BKMODE_TRANSPARENT


#ifdef NO_FRAMEBUFFER
// User implementation
Pixel* frameBufferGetLine(FrameBuffer* frameBuffer, int y);
int    frameBufferGetDoubleWidth(FrameBuffer* frameBuffer, int y);
void   frameBufferSetDoubleWidth(FrameBuffer* frameBuffer, int y, int val);
void   frameBufferSetInterlace(FrameBuffer* frameBuffer, int val);
void   frameBufferSetLineCount(FrameBuffer* frameBuffer, int val);
int    frameBufferGetLineCount(FrameBuffer* frameBuffer);
int    frameBufferGetMaxWidth(FrameBuffer* frameBuffer);

#else

#define frameBufferGetLine(frameBuffer, y)              (frameBuffer->line[y].buffer)
#define frameBufferGetDoubleWidth(frameBuffer, y)       (frameBuffer->line[y].doubleWidth)
#define frameBufferSetDoubleWidth(frameBuffer, y, val)  frameBuffer->line[y].doubleWidth = val
#define frameBufferSetInterlace(frameBuffer, val)       frameBuffer->interlace = val
#define frameBufferSetLineCount(frameBuffer, val)       frameBuffer->lines     = val
#define frameBufferGetLineCount(frameBuffer)            frameBuffer->lines
#define frameBufferGetMaxWidth(frameBuffer)             frameBuffer->maxWidth
#endif

#endif
