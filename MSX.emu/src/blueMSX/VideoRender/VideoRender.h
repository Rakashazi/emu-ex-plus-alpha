/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/VideoRender/VideoRender.h,v $
**
** $Revision: 1.18 $
**
** $Date: 2008-03-31 19:42:23 $
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
#ifndef VIDEO_RENDER_H
#define VIDEO_RENDER_H

#include "MsxTypes.h"
#include "FrameBuffer.h"

struct Properties;

typedef enum { 
    VIDEO_GREEN, 
    VIDEO_COLOR, 
    VIDEO_BLACKWHITE,
    VIDEO_AMBER
} VideoColorMode;

typedef enum { 
    VIDEO_PAL_FAST,
    VIDEO_PAL_MONITOR,
    VIDEO_PAL_SHARP,
    VIDEO_PAL_SHARP_NOISE,  
    VIDEO_PAL_BLUR, 
    VIDEO_PAL_BLUR_NOISE,
	VIDEO_PAL_SCALE2X,
    VIDEO_PAL_HQ2X,
} VideoPalMode;

typedef struct Video Video;

struct Video {
    UInt16* pRgbTable16;
    UInt32* pRgbTable32;
    VideoPalMode palMode;
    int scanLinesEnable;
    int scanLinesPct;
    int colorSaturationEnable;
    int colorSaturationWidth;
    DoubleT gamma;
    DoubleT saturation;
    DoubleT brightness;
    DoubleT contrast;
    int deInterlace;
    int invertRGB;
};

Video* videoCreate();

void videoDestroy(Video* video);

void videoSetDeInterlace(Video* video, int deInterlace);
void videoSetBlendFrames(Video* video, int blendFrames);

void videoSetColorMode(Video* video, VideoColorMode colorMode);
void videoSetRgbMode(Video* video, int inverted);

void videoSetPalMode(Video* video, VideoPalMode palMode);

int videoRender(Video* video, FrameBuffer* frameBuffer, int bitDepth, int zoom, void* pDst, int dstOffset, int dstPitch, int canChangeZoom);

void videoSetColors(Video* video, int saturation, int brightness, int contrast, int gamma);

void videoSetScanLines(Video* video, int enable, int scanLinesPct);
void videoSetColorSaturation(Video* video, int enable, int width);

void videoUpdateAll(Video* video, struct Properties* properties); 

#endif
