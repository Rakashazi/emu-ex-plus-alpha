/*  This file is part of 2600.emu.

	2600.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	2600.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with 2600.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <FrameBuffer.hxx>
#include <OSystem.hxx>
#include <stella/emucore/tia/TIA.hxx>
// TODO: Some Stella types collide with MacTypes.h
#define Debugger DebuggerMac
#include <emuframework/EmuApp.hh>
#undef Debugger
#include <imagine/logger/logger.h>

FrameBuffer::FrameBuffer(OSystem& osystem):
	appPtr{&osystem.app()}
{}

void FrameBuffer::showMessage(const string& message, int position, bool force, uInt32 color)
{
	appPtr->printfMessage(3, false, "%s", message.c_str());
}

void FrameBuffer::enablePhosphor(bool enable, int blend)
{
	myUsePhosphor = enable;
	if(blend >= 0)
	{
		myPhosphorPercent = std::max(blend, 1) / 100.0;
  	logMsg("phosphor blend:%d (%.2f%%)", blend, myPhosphorPercent);
	}
	if(enable)
  {
    for(Int16 c = 255; c >= 0; c--)
      for(Int16 p = 255; p >= 0; p--)
        myPhosphorPalette[c][p] = getPhosphor(c, p);
  }
	prevFramebuffer = {};
}

uint8_t FrameBuffer::getPhosphor(uInt8 c1, uInt8 c2) const
{
	// Use maximum of current and decayed previous values
	c2 = uInt8(c2 * myPhosphorPercent);
	if(c1 > c2)  return c1; // raise (assumed immediate)
	else         return c2; // decay
}

void FrameBuffer::setTIAPalette(const PaletteArray& palette)
{
	logMsg("setTIAPalette");
	iterateTimes(256, i)
	{
		uint8_t r = (palette[i] >> 16) & 0xff;
		uint8_t g = (palette[i] >> 8) & 0xff;
		uint8_t b = palette[i] & 0xff;
		tiaColorMap16[i] = IG::PIXEL_DESC_RGB565.build(r >> 3, g >> 2, b >> 3, 0);
		tiaColorMap32[i] = IG::PIXEL_DESC_BGRA8888.build((int)r, (int)g, (int)b, 0);
	}
}

uInt32 FrameBuffer::getRGBPhosphor(const uInt32 c, const uInt32 p) const
{
  #define TO_RGB(color, red, green, blue) \
    const uInt8 red = color >> 24; const uInt8 green = color >> 16; const uInt8 blue = color >> 8;

  TO_RGB(c, rc, gc, bc);
  TO_RGB(p, rp, gp, bp);

  // Mix current calculated frame with previous displayed frame
  const uInt8 rn = myPhosphorPalette[rc][rp];
  const uInt8 gn = myPhosphorPalette[gc][gp];
  const uInt8 bn = myPhosphorPalette[bc][bp];

  return IG::PIXEL_DESC_RGB565.build(rn >> 3, gn >> 2, bn >> 3, 0);
}

void FrameBuffer::render(IG::Pixmap pix, TIA &tia)
{
	assumeExpr(pix.w() == tia.width());
	assumeExpr(pix.h() == tia.height());
	IG::Pixmap framePix{{{(int)tia.width(), (int)tia.height()}, IG::PIXEL_I8}, tia.frameBuffer()};
	if(myUsePhosphor)
	{
		uint8_t* prevFrame = prevFramebuffer.data();
		pix.writeTransformed([this, &prevFrame](uint8_t p)
			{
				return getRGBPhosphor(tiaColorMap32[p], tiaColorMap32[*prevFrame++]);
			}, framePix);
		memcpy(prevFramebuffer.data(), tia.frameBuffer(), sizeof(prevFramebuffer));
	}
	else
	{
		pix.writeTransformed([this](uint8_t p){ return tiaColorMap16[p]; }, framePix);
	}
}
