//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <cmath>

#include "Font.hxx"
#include "Rect.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurface::FBSurface()
  : myPixels(nullptr),
    myPitch(0)
{
  // NOTE: myPixels and myPitch MUST be set in child classes that inherit
  // from this class

  // Set default attributes
  myAttributes.smoothing = false;
  myAttributes.blending = false;
  myAttributes.blendalpha = 100;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::readPixels(uInt8* buffer, uInt32 pitch, const GUI::Rect& rect) const
{
  uInt8* src = reinterpret_cast<uInt8*>(myPixels + rect.y() * myPitch + rect.x());

  if(rect.empty())
    memcpy(buffer, src, width() * height() * 4);
  else
  {
    uInt32 w = std::min(rect.width(), width());
    uInt32 h = std::min(rect.height(), height());

    // Copy 'height' lines of width 'pitch' (in bytes for both)
    uInt8* dst = buffer;
    while(h--)
    {
      memcpy(dst, src, w * 4);
      src += myPitch * 4;
      dst += pitch * 4;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::pixel(uInt32 x, uInt32 y, uInt32 color)
{
  uInt32* buffer = myPixels + y * myPitch + x;

  *buffer = uInt32(myPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::line(uInt32 x, uInt32 y, uInt32 x2, uInt32 y2, uInt32 color)
{
  // draw line using Bresenham algorithm
  Int32 dx = (x2 - x);
  Int32 dy = (y2 - y);

  if(abs(dx) >= abs(dy))
  {
    // x is major axis
    if(dx < 0)
    {
      uInt32 tx = x; x = x2; x2 = tx;
      uInt32 ty = y; y = y2; y2 = ty;
      dx = -dx;
      dy = -dy;
    }
    Int32 yd = dy > 0 ? 1 : -1;
    dy = abs(dy);
    Int32 err = dx / 2;
    // now draw the line
    for(; x <= x2; ++x)
    {
      pixel(x, y, color);
      err -= dy;
      if(err < 0)
      {
        err += dx;
        y += yd;
      }
    }
  }
  else
  {
    // y is major axis
    if(dy < 0)
    {
      uInt32 tx = x; x = x2; x2 = tx;
      uInt32 ty = y; y = y2; y2 = ty;
      dx = -dx;
      dy = -dy;
    }
    Int32 xd = dx > 0 ? 1 : -1;
    dx = abs(dx);
    Int32 err = dy / 2;
    // now draw the line
    for(; y <= y2; ++y)
    {
      pixel(x, y, color);
      err -= dx;
      if(err < 0)
      {
        err += dy;
        x += xd;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::hLine(uInt32 x, uInt32 y, uInt32 x2, uInt32 color)
{
  uInt32* buffer = myPixels + y * myPitch + x;
  while(x++ <= x2)
    *buffer++ = uInt32(myPalette[color]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::vLine(uInt32 x, uInt32 y, uInt32 y2, uInt32 color)
{
  uInt32* buffer = static_cast<uInt32*>(myPixels + y * myPitch + x);
  while(y++ <= y2)
  {
    *buffer = uInt32(myPalette[color]);
    buffer += myPitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, uInt32 color)
{
  while(h--)
    hLine(x, y+h, x+w-1, color);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::drawChar(const GUI::Font& font, uInt8 chr,
                         uInt32 tx, uInt32 ty, uInt32 color, uInt32 shadowColor)
{
  if(shadowColor != 0)
  {
    drawChar(font, chr, tx + 1, ty + 0, shadowColor);
    drawChar(font, chr, tx + 0, ty + 1, shadowColor);
    drawChar(font, chr, tx + 1, ty + 1, shadowColor);
  }

  const FontDesc& desc = font.desc();

  // If this character is not included in the font, use the default char.
  if(chr < desc.firstchar || chr >= desc.firstchar + desc.size)
  {
    if (chr == ' ') return;
    chr = desc.defaultchar;
  }
  chr -= desc.firstchar;

  // Get the bounding box of the character
  int bbw, bbh, bbx, bby;
  if(!desc.bbx)
  {
    bbw = desc.fbbw;
    bbh = desc.fbbh;
    bbx = desc.fbbx;
    bby = desc.fbby;
  }
  else
  {
    bbw = desc.bbx[chr].w;
    bbh = desc.bbx[chr].h;
    bbx = desc.bbx[chr].x;
    bby = desc.bbx[chr].y;
  }

  const uInt16* tmp = desc.bits + (desc.offset ? desc.offset[chr] : (chr * desc.fbbh));
  uInt32* buffer = myPixels + (ty + desc.ascent - bby - bbh) * myPitch + tx + bbx;

  for(int y = 0; y < bbh; y++)
  {
    const uInt16 ptr = *tmp++;
    uInt16 mask = 0x8000;

    for(int x = 0; x < bbw; x++, mask >>= 1)
      if(ptr & mask)
        buffer[x] = uInt32(myPalette[color]);

    buffer += myPitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::drawBitmap(uInt32* bitmap, uInt32 tx, uInt32 ty,
                           uInt32 color, uInt32 h)
{
  drawBitmap(bitmap, tx, ty, color, h, h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::drawBitmap(uInt32* bitmap, uInt32 tx, uInt32 ty,
                           uInt32 color, uInt32 w, uInt32 h)
{
  uInt32* buffer = myPixels + ty * myPitch + tx;

  for(uInt32 y = 0; y < h; ++y)
  {
    uInt32 mask = 1 << (w - 1);
    for(uInt32 x = 0; x < w; ++x, mask >>= 1)
      if(bitmap[y] & mask)
        buffer[x] = uInt32(myPalette[color]);

    buffer += myPitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::drawPixels(uInt32* data, uInt32 tx, uInt32 ty, uInt32 numpixels)
{
  uInt32* buffer = myPixels + ty * myPitch + tx;

  for(uInt32 i = 0; i < numpixels; ++i)
    *buffer++ = data[i];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::box(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                    uInt32 colorA, uInt32 colorB)
{
  hLine(x + 1, y,     x + w - 2, colorA);
  hLine(x,     y + 1, x + w - 1, colorA);
  vLine(x,     y + 1, y + h - 2, colorA);
  vLine(x + 1, y,     y + h - 1, colorA);

  hLine(x + 1,     y + h - 2, x + w - 1, colorB);
  hLine(x + 1,     y + h - 1, x + w - 2, colorB);
  vLine(x + w - 1, y + 1,     y + h - 2, colorB);
  vLine(x + w - 2, y + 1,     y + h - 1, colorB);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::frameRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                          uInt32 color, FrameStyle style)
{
  switch(style)
  {
    case FrameStyle::Solid:
      hLine(x,         y,         x + w - 1, color);
      hLine(x,         y + h - 1, x + w - 1, color);
      vLine(x,         y,         y + h - 1, color);
      vLine(x + w - 1, y,         y + h - 1, color);
      break;

    case FrameStyle::Dashed:
      uInt32 i, skip, lwidth = 1;

#ifndef FLAT_UI
      for(i = x, skip = 1; i < x+w-1; i=i+lwidth+1, ++skip)
      {
        if(skip % 2)
        {
          hLine(i, y,         i + lwidth, color);
          hLine(i, y + h - 1, i + lwidth, color);
        }
      }
      for(i = y, skip = 1; i < y+h-1; i=i+lwidth+1, ++skip)
      {
        if(skip % 2)
        {
          vLine(x,         i, i + lwidth, color);
          vLine(x + w - 1, i, i + lwidth, color);
        }
      }
#else
      for(i = x; i < x + w; i += 2)
      {
        hLine(i, y, i, color);
        hLine(i, y + h - 1, i, color);
      }
      for(i = y; i < y + h; i += 2)
      {
        vLine(x, i, i, color);
        vLine(x + w - 1, i, i, color);
      }
#endif
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::drawString(const GUI::Font& font, const string& s,
                           int x, int y, int w,
                           uInt32 color, TextAlign align,
                           int deltax, bool useEllipsis, uInt32 shadowColor)
{
  const string ELLIPSIS = "\x1d"; // "..."
  const int leftX = x, rightX = x + w;
  uInt32 i;
  int width = font.getStringWidth(s);
  string str;

  if(useEllipsis && width > w)
  {
    // String is too wide. So we shorten it "intelligently", by replacing
    // parts of it by an ellipsis ("..."). There are three possibilities
    // for this: replace the start, the end, or the middle of the string.
    // What is best really depends on the context; but most applications
    // replace the end. So we use that too.
    int w2 = font.getStringWidth(ELLIPSIS);

    // SLOW algorithm to find the acceptable length. But it is good enough for now.
    for(i = 0; i < s.size(); ++i)
    {
      int charWidth = font.getCharWidth(s[i]);
      if(w2 + charWidth > w)
        break;

      w2 += charWidth;
      str += s[i];
    }
    str += ELLIPSIS;

    width = font.getStringWidth(str);
  }
  else
    str = s;

  if(align == TextAlign::Center)
    x = x + (w - width - 1)/2;
  else if(align == TextAlign::Right)
    x = x + w - width;

  x += deltax;
  for(i = 0; i < str.size(); ++i)
  {
    w = font.getCharWidth(str[i]);
    if(x+w > rightX)
      break;
    if(x >= leftX)
      drawChar(font, str[i], x, y, color, shadowColor);

    x += w;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32* FBSurface::myPalette = nullptr;
