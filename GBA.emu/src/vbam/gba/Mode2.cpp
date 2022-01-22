#include "GBA.h"
#include "GBAGfx.h"
#include "Globals.h"

#define BLDMOD ioMem.BLDMOD
#define COLEV ioMem.COLEV
#define COLY ioMem.COLY
#define VCOUNT ioMem.VCOUNT
#define MOSAIC ioMem.MOSAIC
#define DISPCNT ioMem.DISPCNT
#define layerEnable lcd.layerEnable
#define BG0CNT ioMem.BG0CNT
#define BG0HOFS ioMem.BG0HOFS
#define BG0VOFS ioMem.BG0VOFS
#define line0 lcd.line0
#define BG1CNT ioMem.BG1CNT
#define BG1HOFS ioMem.BG1HOFS
#define BG1VOFS ioMem.BG1VOFS
#define line1 lcd.line1
#define BG2CNT ioMem.BG2CNT
#define BG2HOFS ioMem.BG2HOFS
#define BG2VOFS ioMem.BG2VOFS
#define line2 lcd.line2
#define BG3CNT ioMem.BG3CNT
#define BG3HOFS ioMem.BG3HOFS
#define BG3VOFS ioMem.BG3VOFS
#define line3 lcd.line3
#define lineOBJ lcd.lineOBJ
#define lineOBJWin lcd.lineOBJWin
#define WIN0V ioMem.WIN0V
#define WIN1V ioMem.WIN1V
#define WININ ioMem.WININ
#define WINOUT ioMem.WINOUT
#define gfxBG2Changed lcd.gfxBG2Changed
#define gfxBG3Changed lcd.gfxBG3Changed
#define gfxLastVCOUNT lcd.gfxLastVCOUNT
#define gfxInWin0 lcd.gfxInWin0
#define gfxInWin1 lcd.gfxInWin1
#define gfxDrawTextScreen(BGCNT, BGHOFS, BGVOFS, line) gfxDrawTextScreen(lcd.vram, BGCNT, BGHOFS, BGVOFS, line, VCOUNT, MOSAIC, palette)
#define gfxDrawSprites(lineOBJ) gfxDrawSprites(lcd, lineOBJ, VCOUNT, MOSAIC, DISPCNT)
#define gfxDrawOBJWin(lineOBJWin) gfxDrawOBJWin(lcd, lineOBJWin, VCOUNT, DISPCNT)

void mode2RenderLine(MixColorType *lineMix, GBALCD &lcd, const GBAMem::IoMem &ioMem)
{
#ifdef GBALCD_TEMP_LINE_BUFFER
	uint32_t line2[240];
	uint32_t line3[240];
	uint32_t lineOBJ[240];
#endif
  const uint16_t *palette = (uint16_t *)lcd.paletteRAM;

  if (layerEnable & 0x0400) {
    int changed = gfxBG2Changed;
    if (gfxLastVCOUNT > VCOUNT)
      changed = 3;

    gfxDrawRotScreen(lcd.vram, BG2CNT, ioMem.BG2X_L, ioMem.BG2X_H, ioMem.BG2Y_L, ioMem.BG2Y_H,
                     ioMem.BG2PA, ioMem.BG2PB, ioMem.BG2PC, ioMem.BG2PD, lcd.gfxBG2X, lcd.gfxBG2Y,
                     changed, line2, VCOUNT, MOSAIC, palette);
  }

  if (layerEnable & 0x0800) {
    int changed = gfxBG3Changed;
    if (gfxLastVCOUNT > VCOUNT)
      changed = 3;

    gfxDrawRotScreen(lcd.vram, BG3CNT, ioMem.BG3X_L, ioMem.BG3X_H, ioMem.BG3Y_L, ioMem.BG3Y_H,
    		ioMem.BG3PA, ioMem.BG3PB, ioMem.BG3PC, ioMem.BG3PD, lcd.gfxBG3X, lcd.gfxBG3Y,
                     changed, line3, VCOUNT, MOSAIC, palette);
  }

  gfxDrawSprites(lineOBJ);

  uint32_t backdrop;
  if (customBackdropColor == -1) {
    backdrop = (READ16LE(&palette[0]) | 0x30000000);
  } else {
    backdrop = ((customBackdropColor & 0x7FFF) | 0x30000000);
  }

  for (int x = 0; x < 240; x++) {
    uint32_t color = backdrop;
    uint8_t top = 0x20;

    if ((uint8_t)(line2[x] >> 24) < (uint8_t)(color >> 24)) {
      color = line2[x];
      top = 0x04;
    }

    if ((uint8_t)(line3[x] >> 24) < (uint8_t)(color >> 24)) {
      color = line3[x];
      top = 0x08;
    }

    if ((uint8_t)(lineOBJ[x] >> 24) < (uint8_t)(color >> 24)) {
      color = lineOBJ[x];
      top = 0x10;
    }

    if ((top & 0x10) && (color & 0x00010000)) {
      // semi-transparent OBJ
      uint32_t back = backdrop;
      uint8_t top2 = 0x20;

      if ((uint8_t)(line2[x] >> 24) < (uint8_t)(back >> 24)) {
        back = line2[x];
        top2 = 0x04;
      }

      if ((uint8_t)(line3[x] >> 24) < (uint8_t)(back >> 24)) {
        back = line3[x];
        top2 = 0x08;
      }

      if (top2 & (BLDMOD >> 8))
        color = gfxAlphaBlend(color, back,
                              coeff[COLEV & 0x1F],
                              coeff[(COLEV >> 8) & 0x1F]);
      else {
        switch ((BLDMOD >> 6) & 3) {
        case 2:
          if (BLDMOD & top)
            color = gfxIncreaseBrightness(color, coeff[COLY & 0x1F]);
          break;
        case 3:
          if (BLDMOD & top)
            color = gfxDecreaseBrightness(color, coeff[COLY & 0x1F]);
          break;
        }
      }
    }

    lineMix[x] = color;
  }
  gfxBG2Changed = 0;
  gfxBG3Changed = 0;
  gfxLastVCOUNT = VCOUNT;
}

void mode2RenderLineNoWindow(MixColorType *lineMix, GBALCD &lcd, const GBAMem::IoMem &ioMem)
{
#ifdef GBALCD_TEMP_LINE_BUFFER
	uint32_t line2[240];
	uint32_t line3[240];
	uint32_t lineOBJ[240];
#endif
  const uint16_t *palette = (uint16_t *)lcd.paletteRAM;

  if (layerEnable & 0x0400) {
    int changed = gfxBG2Changed;
    if (gfxLastVCOUNT > VCOUNT)
      changed = 3;

    gfxDrawRotScreen(lcd.vram, BG2CNT, ioMem.BG2X_L, ioMem.BG2X_H, ioMem.BG2Y_L, ioMem.BG2Y_H,
                     ioMem.BG2PA, ioMem.BG2PB, ioMem.BG2PC, ioMem.BG2PD, lcd.gfxBG2X, lcd.gfxBG2Y,
                     changed, line2, VCOUNT, MOSAIC, palette);
  }

  if (layerEnable & 0x0800) {
    int changed = gfxBG3Changed;
    if (gfxLastVCOUNT > VCOUNT)
      changed = 3;

    gfxDrawRotScreen(lcd.vram, BG3CNT, ioMem.BG3X_L, ioMem.BG3X_H, ioMem.BG3Y_L, ioMem.BG3Y_H,
    		ioMem.BG3PA, ioMem.BG3PB, ioMem.BG3PC, ioMem.BG3PD, lcd.gfxBG3X, lcd.gfxBG3Y,
                     changed, line3, VCOUNT, MOSAIC, palette);
  }

  gfxDrawSprites(lineOBJ);

  uint32_t backdrop;
  if (customBackdropColor == -1) {
    backdrop = (READ16LE(&palette[0]) | 0x30000000);
  } else {
    backdrop = ((customBackdropColor & 0x7FFF) | 0x30000000);
  }

  for (int x = 0; x < 240; x++) {
    uint32_t color = backdrop;
    uint8_t top = 0x20;


    if ((uint8_t)(line2[x] >> 24) < (uint8_t)(color >> 24)) {
      color = line2[x];
      top = 0x04;
    }

    if ((uint8_t)(line3[x] >> 24) < (uint8_t)(color >> 24)) {
      color = line3[x];
      top = 0x08;
    }

    if ((uint8_t)(lineOBJ[x] >> 24) < (uint8_t)(color >> 24)) {
      color = lineOBJ[x];
      top = 0x10;
    }

    if (!(color & 0x00010000)) {
      switch ((BLDMOD >> 6) & 3) {
      case 0:
        break;
      case 1: {
          if (top & BLDMOD) {
            uint32_t back = backdrop;
            uint8_t top2 = 0x20;

            if ((uint8_t)(line2[x] >> 24) < (uint8_t)(back >> 24)) {
              if (top != 0x04) {
                back = line2[x];
                top2 = 0x04;
              }
            }

            if ((uint8_t)(line3[x] >> 24) < (uint8_t)(back >> 24)) {
              if (top != 0x08) {
                back = line3[x];
                top2 = 0x08;
              }
            }

            if ((uint8_t)(lineOBJ[x] >> 24) < (uint8_t)(back >> 24)) {
              if (top != 0x10) {
                back = lineOBJ[x];
                top2 = 0x10;
              }
            }

            if (top2 & (BLDMOD >> 8))
              color = gfxAlphaBlend(color, back,
                                    coeff[COLEV & 0x1F],
                                    coeff[(COLEV >> 8) & 0x1F]);
          }
        } break;
      case 2:
        if (BLDMOD & top)
          color = gfxIncreaseBrightness(color, coeff[COLY & 0x1F]);
        break;
      case 3:
        if (BLDMOD & top)
          color = gfxDecreaseBrightness(color, coeff[COLY & 0x1F]);
        break;
      }
    } else {
      // semi-transparent OBJ
      uint32_t back = backdrop;
      uint8_t top2 = 0x20;

      if ((uint8_t)(line2[x] >> 24) < (uint8_t)(back >> 24)) {
        back = line2[x];
        top2 = 0x04;
      }

      if ((uint8_t)(line3[x] >> 24) < (uint8_t)(back >> 24)) {
        back = line3[x];
        top2 = 0x08;
      }

      if (top2 & (BLDMOD >> 8))
        color = gfxAlphaBlend(color, back,
                              coeff[COLEV & 0x1F],
                              coeff[(COLEV >> 8) & 0x1F]);
      else {
        switch ((BLDMOD >> 6) & 3) {
        case 2:
          if (BLDMOD & top)
            color = gfxIncreaseBrightness(color, coeff[COLY & 0x1F]);
          break;
        case 3:
          if (BLDMOD & top)
            color = gfxDecreaseBrightness(color, coeff[COLY & 0x1F]);
          break;
        }
      }
    }

    lineMix[x] = color;
  }
  gfxBG2Changed = 0;
  gfxBG3Changed = 0;
  gfxLastVCOUNT = VCOUNT;
}

void mode2RenderLineAll(MixColorType *lineMix, GBALCD &lcd, const GBAMem::IoMem &ioMem)
{
#ifdef GBALCD_TEMP_LINE_BUFFER
	uint32_t line2[240];
	uint32_t line3[240];
	uint32_t lineOBJ[240];
#endif
  const uint16_t *palette = (uint16_t *)lcd.paletteRAM;

  bool inWindow0 = false;
  bool inWindow1 = false;

  if (layerEnable & 0x2000) {
    uint8_t v0 = WIN0V >> 8;
    uint8_t v1 = WIN0V & 255;
    inWindow0 = ((v0 == v1) && (v0 >= 0xe8));
    if (v1 >= v0)
      inWindow0 |= (VCOUNT >= v0 && VCOUNT < v1);
    else
      inWindow0 |= (VCOUNT >= v0 || VCOUNT < v1);
  }
  if (layerEnable & 0x4000) {
    uint8_t v0 = WIN1V >> 8;
    uint8_t v1 = WIN1V & 255;
    inWindow1 = ((v0 == v1) && (v0 >= 0xe8));
    if (v1 >= v0)
      inWindow1 |= (VCOUNT >= v0 && VCOUNT < v1);
    else
      inWindow1 |= (VCOUNT >= v0 || VCOUNT < v1);
  }

  if (layerEnable & 0x0400) {
    int changed = gfxBG2Changed;
    if (gfxLastVCOUNT > VCOUNT)
      changed = 3;

    gfxDrawRotScreen(lcd.vram, BG2CNT, ioMem.BG2X_L, ioMem.BG2X_H, ioMem.BG2Y_L, ioMem.BG2Y_H,
                     ioMem.BG2PA, ioMem.BG2PB, ioMem.BG2PC, ioMem.BG2PD, lcd.gfxBG2X, lcd.gfxBG2Y,
                     changed, line2, VCOUNT, MOSAIC, palette);
  }

  if (layerEnable & 0x0800) {
    int changed = gfxBG3Changed;
    if (gfxLastVCOUNT > VCOUNT)
      changed = 3;

    gfxDrawRotScreen(lcd.vram, BG3CNT, ioMem.BG3X_L, ioMem.BG3X_H, ioMem.BG3Y_L, ioMem.BG3Y_H,
    		ioMem.BG3PA, ioMem.BG3PB, ioMem.BG3PC, ioMem.BG3PD, lcd.gfxBG3X, lcd.gfxBG3Y,
                     changed, line3, VCOUNT, MOSAIC, palette);
  }

  gfxDrawSprites(lineOBJ);
  gfxDrawOBJWin(lineOBJWin);

  uint32_t backdrop;
  if (customBackdropColor == -1) {
    backdrop = (READ16LE(&palette[0]) | 0x30000000);
  } else {
    backdrop = ((customBackdropColor & 0x7FFF) | 0x30000000);
  }

  uint8_t inWin0Mask = WININ & 0xFF;
  uint8_t inWin1Mask = WININ >> 8;
  uint8_t outMask = WINOUT & 0xFF;

  for (int x = 0; x < 240; x++) {
    uint32_t color = backdrop;
    uint8_t top = 0x20;
    uint8_t mask = outMask;

    if (!(lineOBJWin[x] & 0x80000000)) {
      mask = WINOUT >> 8;
    }

    if (inWindow1) {
      if (gfxInWin1[x])
        mask = inWin1Mask;
    }

    if (inWindow0) {
      if (gfxInWin0[x]) {
        mask = inWin0Mask;
      }
    }

    if (line2[x] < color && (mask & 4)) {
      color = line2[x];
      top = 0x04;
    }

    if ((uint8_t)(line3[x] >> 24) < (uint8_t)(color >> 24) && (mask & 8)) {
      color = line3[x];
      top = 0x08;
    }

    if ((uint8_t)(lineOBJ[x] >> 24) < (uint8_t)(color >> 24) && (mask & 16)) {
      color = lineOBJ[x];
      top = 0x10;
    }

    if (color & 0x00010000) {
      // semi-transparent OBJ
      uint32_t back = backdrop;
      uint8_t top2 = 0x20;

      if ((mask & 4) && line2[x] < back) {
        back = line2[x];
        top2 = 0x04;
      }

      if ((mask & 8) && (uint8_t)(line3[x] >> 24) < (uint8_t)(back >> 24)) {
        back = line3[x];
        top2 = 0x08;
      }

      if (top2 & (BLDMOD >> 8))
        color = gfxAlphaBlend(color, back,
                              coeff[COLEV & 0x1F],
                              coeff[(COLEV >> 8) & 0x1F]);
      else {
        switch ((BLDMOD >> 6) & 3) {
        case 2:
          if (BLDMOD & top)
            color = gfxIncreaseBrightness(color, coeff[COLY & 0x1F]);
          break;
        case 3:
          if (BLDMOD & top)
            color = gfxDecreaseBrightness(color, coeff[COLY & 0x1F]);
          break;
        }
      }
    } else if (mask & 32) {
      // special FX on the window
      switch ((BLDMOD >> 6) & 3) {
      case 0:
        break;
      case 1: {
          if (top & BLDMOD) {
            uint32_t back = backdrop;
            uint8_t top2 = 0x20;

            if ((mask & 4) && line2[x] < back) {
              if (top != 0x04) {
                back = line2[x];
                top2 = 0x04;
              }
            }

            if ((mask & 8) && (uint8_t)(line3[x] >> 24) < (uint8_t)(back >> 24)) {
              if (top != 0x08) {
                back = line3[x];
                top2 = 0x08;
              }
            }

            if ((mask & 16) && (uint8_t)(lineOBJ[x] >> 24) < (uint8_t)(back >> 24)) {
              if (top != 0x10) {
                back = lineOBJ[x];
                top2 = 0x10;
              }
            }

            if (top2 & (BLDMOD >> 8))
              color = gfxAlphaBlend(color, back,
                                    coeff[COLEV & 0x1F],
                                    coeff[(COLEV >> 8) & 0x1F]);
          }
        } break;
      case 2:
        if (BLDMOD & top)
          color = gfxIncreaseBrightness(color, coeff[COLY & 0x1F]);
        break;
      case 3:
        if (BLDMOD & top)
          color = gfxDecreaseBrightness(color, coeff[COLY & 0x1F]);
        break;
      }
    }

    lineMix[x] = color;
  }
  gfxBG2Changed = 0;
  gfxBG3Changed = 0;
  gfxLastVCOUNT = VCOUNT;
}
