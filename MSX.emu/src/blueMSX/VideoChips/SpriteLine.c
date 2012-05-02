/*****************************************************************************
** File:
**      SpriteLine.c
**
** Author:
**      Daniel Vik
**
** Copyright (C) 2003-2004 Daniel Vik
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
******************************************************************************
*/

#include "SpriteLine.h"
#include <string.h>

static int vramAddr;
#define MAP_VRAM(addr) (VRAM + ((vramAddr = addr, screenMode >= 7 && screenMode <= 8 ? (vramAddr >> 1 | ((vramAddr & 1) << 16)) : vramAddr) & vramMask))

typedef struct {
    int horizontalPos;
    int color;
    UInt16 pattern;
} SpriteAttribute;

static UInt8 status;
static int   showSprites;
static int   sprTabBase;
static int   sprGenBase;
static int   spritesBig;
static int   spritesDouble;
static int   screenMode;
static int   vramMask;
static UInt8* VRAM;
static UInt8 lineBuffer[2][384];
static UInt8* lineBufs[2] = { NULL, NULL };
static UInt8* lineBuf = NULL;

void spriteLineInit(UInt8* vramPtr) {
    status      = 0x1f;
    showSprites = 1;
    VRAM        = vramPtr;
    sprTabBase  = 0;
    sprGenBase  = 0;
}

void spriteLineSetAttributes(int tabBase, int genBase, int big, int dblSize) {
    sprTabBase    = tabBase;
    sprGenBase    = genBase;
    spritesBig    = big;
    spritesDouble = dblSize;
}

UInt8 spriteLineGetStatus() {
    UInt8 s = status;
    status &= 0x1f;
    return s;
}

void spriteLineShow(int show) {
    showSprites = show;
}

UInt8* spritesLine(int line) {
    int bufIndex;
    UInt8 collisionBuf[384];
    UInt8* attrib;
    UInt8* attribTable[4];
    UInt8 patternMask;
    int idx;
    int size;
    int scale;
    int visibleCnt;
    int collision;

    bufIndex = line & 1;

    if (showSprites) {
        lineBufs[bufIndex] = NULL;
        return lineBufs[bufIndex ^ 1];
    }

    memset(collisionBuf, 0, sizeof(collisionBuf));

    attrib = &VRAM[sprTabBase & (-1 << 7)];
    size   = spritesDouble ? 16 : 8;
    scale  = spritesBig ? 2 : 1;
    line   = (line + VScroll) & 0xff;
    
	patternMask = spritesDouble ? 0xfc : 0xff;
    visibleCnt = 0;

    /* Find visible sprites on current line */
    for (idx = 0; idx < 32; idx++, attrib += 4) {
        int spriteLine = attrib[0];
        if (spriteLine == 208) {
            break;
        }
       
        spriteLine = ((line - spriteLine) & 0xff) / scale;
		if (spriteLine >= size) {
            continue;
        }
        
        if (visibleCnt == 4) {
			if (~status & 0xc0) {
				status = (status & 0xe0) | 0x40 | idx;
			}
            break;
        }

        attribTable[visibleCnt++] = attrib;
    }

    if (visibleCnt == 0) {
        lineBufs[bufIndex] = NULL;
        return lineBufs[bufIndex ^ 1];
    }

	if (~status & 0x40) {
		status = (status & 0xe0) | (idx < 32 ? idx : 31);
	}
    
    lineBuf = lineBuffer[bufIndex];
    memset(lineBuf, 0, 384);
    memset(collisionBuf, 0, 384);

    collision = 0;
    
    memset(lineBuf, 0, 384);

    while (visibleCnt--) {
        int    spriteLine;
        UInt8  color;
        UInt8* patternPtr;
        UInt8  pattern;
        UInt8* linePtr;
        UInt8* colPtr;

        attrib     = attribTable[visibleCnt];
        spriteLine = ((line - attrib[0]) & 0xff) / scale;

        colPtr     = collisionBuf + ((int)attrib[1] + 32 - ((attrib[3] >> 2) & 0x20));
        linePtr    = lineBuf + ((int)attrib[1] + 32 - ((attrib[3] >> 2) & 0x20));
        color      = attrib[3] & 0x0f;
        patternPtr = &VRAM[(sprGenBase & (-1 << 11)) + ((int)(attrib[2] & patternMask) << 3) + spriteLine];

        if (scale == 1) {
            pattern = patternPtr[0]; 
            if (pattern & 0x80) { linePtr[0] = color; collision |= colPtr[0]; colPtr[0] = 1; }
            if (pattern & 0x40) { linePtr[1] = color; collision |= colPtr[1]; colPtr[1] = 1; }
            if (pattern & 0x20) { linePtr[2] = color; collision |= colPtr[2]; colPtr[2] = 1; }
            if (pattern & 0x10) { linePtr[3] = color; collision |= colPtr[3]; colPtr[3] = 1; }
            if (pattern & 0x08) { linePtr[4] = color; collision |= colPtr[4]; colPtr[4] = 1; }
            if (pattern & 0x04) { linePtr[5] = color; collision |= colPtr[5]; colPtr[5] = 1; }
            if (pattern & 0x02) { linePtr[6] = color; collision |= colPtr[6]; colPtr[6] = 1; }
            if (pattern & 0x01) { linePtr[7] = color; collision |= colPtr[7]; colPtr[7] = 1; }

            if (spritesDouble) {
                pattern = patternPtr[16];

                if (pattern & 0x80) { linePtr[8]  = color; collision |= colPtr[8];  colPtr[8]  = 1; }
                if (pattern & 0x40) { linePtr[9]  = color; collision |= colPtr[9];  colPtr[9]  = 1; }
                if (pattern & 0x20) { linePtr[10] = color; collision |= colPtr[10]; colPtr[10] = 1; }
                if (pattern & 0x10) { linePtr[11] = color; collision |= colPtr[11]; colPtr[11] = 1; }
                if (pattern & 0x08) { linePtr[12] = color; collision |= colPtr[12]; colPtr[12] = 1; }
                if (pattern & 0x04) { linePtr[13] = color; collision |= colPtr[13]; colPtr[13] = 1; }
                if (pattern & 0x02) { linePtr[14] = color; collision |= colPtr[14]; colPtr[14] = 1; }
                if (pattern & 0x01) { linePtr[15] = color; collision |= colPtr[15]; colPtr[15] = 1; }
            }
        }
        else {
            pattern = patternPtr[0];
            if (pattern & 0x80) { linePtr[0]  = linePtr[1]  = color; collision |= colPtr[0];  colPtr[0]  = 1; collision |= colPtr[1];  colPtr[1]  = 1; }
            if (pattern & 0x40) { linePtr[2]  = linePtr[3]  = color; collision |= colPtr[2];  colPtr[2]  = 1; collision |= colPtr[3];  colPtr[3]  = 1; }
            if (pattern & 0x20) { linePtr[4]  = linePtr[5]  = color; collision |= colPtr[4];  colPtr[4]  = 1; collision |= colPtr[5];  colPtr[5]  = 1; }
            if (pattern & 0x10) { linePtr[6]  = linePtr[7]  = color; collision |= colPtr[6];  colPtr[6]  = 1; collision |= colPtr[7];  colPtr[7]  = 1; }
            if (pattern & 0x08) { linePtr[8]  = linePtr[9]  = color; collision |= colPtr[8];  colPtr[8]  = 1; collision |= colPtr[9];  colPtr[9]  = 1; }
            if (pattern & 0x04) { linePtr[10] = linePtr[11] = color; collision |= colPtr[10]; colPtr[10] = 1; collision |= colPtr[11]; colPtr[11] = 1; }
            if (pattern & 0x02) { linePtr[12] = linePtr[13] = color; collision |= colPtr[12]; colPtr[12] = 1; collision |= colPtr[13]; colPtr[13] = 1; }
            if (pattern & 0x01) { linePtr[14] = linePtr[15] = color; collision |= colPtr[14]; colPtr[14] = 1; collision |= colPtr[15]; colPtr[15] = 1; }

            if (spritesDouble) {
                pattern = patternPtr[16];

                if (pattern & 0x80) { linePtr[16] = linePtr[17] = color; collision |= colPtr[16]; colPtr[16] = 1; collision |= colPtr[17]; colPtr[17] = 1; }
                if (pattern & 0x40) { linePtr[18] = linePtr[19] = color; collision |= colPtr[18]; colPtr[18] = 1; collision |= colPtr[19]; colPtr[19] = 1; }
                if (pattern & 0x20) { linePtr[20] = linePtr[21] = color; collision |= colPtr[20]; colPtr[20] = 1; collision |= colPtr[21]; colPtr[21] = 1; }
                if (pattern & 0x10) { linePtr[22] = linePtr[23] = color; collision |= colPtr[22]; colPtr[22] = 1; collision |= colPtr[23]; colPtr[23] = 1; }
                if (pattern & 0x08) { linePtr[24] = linePtr[25] = color; collision |= colPtr[24]; colPtr[24] = 1; collision |= colPtr[25]; colPtr[25] = 1; }
                if (pattern & 0x04) { linePtr[26] = linePtr[27] = color; collision |= colPtr[26]; colPtr[26] = 1; collision |= colPtr[27]; colPtr[27] = 1; }
                if (pattern & 0x02) { linePtr[28] = linePtr[29] = color; collision |= colPtr[28]; colPtr[28] = 1; collision |= colPtr[29]; colPtr[29] = 1; }
                if (pattern & 0x01) { linePtr[30] = linePtr[31] = color; collision |= colPtr[30]; colPtr[30] = 1; collision |= colPtr[31]; colPtr[31] = 1; }
            }
        }
    }

    if (collision) {
        status |= 0x20;
    }

    lineBufs[bufIndex] = lineBuf + 32;
    return lineBufs[bufIndex ^ 1];
}

UInt8* colorSpritesLine(int line, int solidColor) {
    int bufIndex;
    UInt8 collisionBuf[384];
    SpriteAttribute attribTable[8];
    UInt8 patternMask;
    int   attribOffset;
    int   sprite;
    int   size;
    int   scale;
    int   visibleCnt;
    int   collision;
    int   idx;
    static UInt8 ccColorMask;
    static UInt8 ccColorCheckMask;

    bufIndex = line & 1;

    if (line == 0) {
        // This is an not 100% correct optimization. CC sprites should be shown only when
        // they collide with a non CC sprite. However very few games/demos uses this and
        // it is safe to disable the CC sprites if no non CC sprites are visible.
        ccColorMask = ccColorCheckMask;
        ccColorCheckMask = 0xf0;
    }

    if (showSprites) {
        lineBufs[bufIndex] = NULL;
        return lineBufs[bufIndex ^ 1];
    }

    solidColor = solidColor ? 1 : 0;
    attribOffset = sprTabBase & 0x1fe00;
    size         = spritesDouble ? 16 : 8;
    scale        = spritesBig ? 2 : 1;
	patternMask  = spritesDouble ? 0xfc : 0xff;
    visibleCnt   = 0;
    collision    = 0;
    line         = (line + VScroll) & 0xff;

    /* Find visible sprites on current line */
    for (sprite = 0; sprite < 32; sprite++, attribOffset += 4) {
        int spriteLine;
        int offset;
        int color;

        spriteLine = *MAP_VRAM(attribOffset);
        if (spriteLine == 216) {
            break;
        }
       
        spriteLine = ((line - spriteLine) & 0xff) / scale;
		if (spriteLine >= size) {
            continue;
        }

        if (visibleCnt == 8) {
			if (~status & 0xc0) {
				status = (status & 0xe0) | 0x40 | sprite;
			}
            break;
        }

        offset = (sprGenBase & 0x1f800) + ((int)(*MAP_VRAM(attribOffset + 2) & patternMask) << 3) + spriteLine;
        color  = *MAP_VRAM(sprTabBase & ((-1 << 10) | (sprite * 16 + spriteLine)));

        if (color & 0x40) {
            color &= ccColorMask;
        }
        else if ((color & 0x0f) || solidColor) {
            ccColorCheckMask = 0xff;
        }

        attribTable[visibleCnt].color           = color;
        attribTable[visibleCnt].horizontalPos   = (int)*MAP_VRAM(attribOffset + 1) + 24 - ((attribTable[visibleCnt].color >> 2) & 0x20);
        attribTable[visibleCnt].pattern         = *MAP_VRAM(offset);
        if (spritesDouble) {
            attribTable[visibleCnt].pattern = (attribTable[visibleCnt].pattern << 8) | *MAP_VRAM(offset + 16);
            attribTable[visibleCnt].horizontalPos += 8;
        }
        visibleCnt++;
    }

    if (visibleCnt == 0) {
        lineBufs[bufIndex] = NULL;
        return lineBufs[bufIndex ^ 1];
    }

    if (~status & 0x40) {
		status = (status & 0xe0) | (sprite < 32 ? sprite : 31);
	}
    
    lineBuf = lineBuffer[bufIndex];
    memset(lineBuf, 0, 384);
    memset(collisionBuf, 0, 384);

    /* Draw the visible sprites */
    for (idx = visibleCnt - 1; idx >= 0; idx--) {
        SpriteAttribute* attrib = &attribTable[idx];
        UInt8* linePtr;
        UInt8* colPtr;
        UInt16 pattern;
        UInt8 color;
        int offset;
        int idx2;

        color = ((attrib->color & 0x0f) << 1) | solidColor;
        if (color == 0) {
            continue;
        }

        colPtr  = collisionBuf + attrib->horizontalPos;
        linePtr = lineBuf + attrib->horizontalPos;
        pattern = attrib->pattern;
        offset  = scale * 15;

        if (attrib->color & 0x60) {
            if (scale == 2) {
                while (pattern) {
                    if (pattern & 1) {
                        linePtr[offset] = color;
                        linePtr[offset + 1] = color;
                    }
                    offset -= 2;
                    pattern >>= 1;
                }
            }
            else {
                while (pattern) {
                    if (pattern & 1) {
                        linePtr[offset] = color;
                    }
                    offset--;
                    pattern >>= 1;
                }
            }
        }
        else {
            if (scale == 2) {
                while (pattern) {
                    if (pattern & 1) {
                        linePtr[offset] = color;
                        linePtr[offset + 1] = color;
                        collision |= colPtr[offset]; 
                        colPtr[offset] = 1;
                        collision |= colPtr[offset + 1]; 
                        colPtr[offset + 1] = 1;
                    }
                    offset -= 2;
                    pattern >>= 1;
                }
            }
            else {
                while (pattern) {
                    if (pattern & 1) {
                        linePtr[offset] = color;
                        collision |= colPtr[offset]; 
                        colPtr[offset] = 1;
                    }
                    offset--;
                    pattern >>= 1;
                }
            }
        }

        /* Skip CC sprites for now */
        if (attrib->color & 0x40) {
            continue;
        }

        /* Draw CC sprites */
        for (idx2 = idx + 1; idx2 < visibleCnt; idx2++) {
            SpriteAttribute* attrib = &attribTable[idx2];

            if (!(attrib->color & 0x40)) {
                break;
            }
            
            color   = ((attrib->color & 0x0f) << 1) | solidColor;
            colPtr  = collisionBuf + attrib->horizontalPos;
            linePtr = lineBuf + attrib->horizontalPos;
            pattern = attrib->pattern;
            offset  = scale * 15;
            
            if (scale == 2) {
                while (pattern) {
                    if (pattern & 1) {
                        linePtr[offset] |= color;
                        linePtr[offset + 1] |= color;
                    }
                    offset -= 2;
                    pattern >>= 1;
                }
            }
            else {
                while (pattern) {
                    if (pattern & 1) {
                        linePtr[offset] |= color;
                    }
                    offset--;
                    pattern >>= 1;
                }
            }
        }
    }

    if (collision) {
        status |= 0x20;
    }

    lineBufs[bufIndex] = lineBuf + 32;
    return lineBufs[bufIndex ^ 1];
}
