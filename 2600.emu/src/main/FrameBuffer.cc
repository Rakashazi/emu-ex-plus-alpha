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
	appPtr{&osystem.app()}, myPaletteHandler{osystem}
{
	myPaletteHandler.loadConfig(osystem.settings());
}

void FrameBuffer::showTextMessage(const string& message, MessagePosition, bool) const
{
	appPtr->postMessage(3, false, message.c_str());
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
	auto desc32 = format == IG::PixelFmtBGRA8888 ? IG::PixelDescBGRA8888Native : IG::PixelDescRGBA8888Native;
	for(auto i : IG::iotaCount(256))
	{
		uint8_t r = (palette[i] >> 16) & 0xff;
		uint8_t g = (palette[i] >> 8) & 0xff;
		uint8_t b = palette[i] & 0xff;
		tiaColorMap16[i] = IG::PixelDescRGB565.build(r >> 3, g >> 2, b >> 3, 0);
		tiaColorMap32[i] = desc32.build((int)r, (int)g, (int)b, 0);
	}
}

void FrameBuffer::setPixelFormat(IG::PixelFormatId fmt)
{
	format = fmt;
}

IG::PixelFormatId FrameBuffer::pixelFormat() const
{
	return format;
}

std::array<uInt8, 3> FrameBuffer::getRGBPhosphorTriple(uInt32 c, uInt32 p) const
{
	auto [rc, gc, bc, ac] = IG::PixelDescRGBA8888Native.rgba(c);
	auto [rp, gp, bp, ap] = IG::PixelDescRGBA8888Native.rgba(p);

  // Mix current calculated frame with previous displayed frame
  const uInt8 rn = myPhosphorPalette[rc][rp];
  const uInt8 gn = myPhosphorPalette[gc][gp];
  const uInt8 bn = myPhosphorPalette[bc][bp];
  return {rn, gn, bn};
}

uInt16 FrameBuffer::getRGBPhosphor16(const uInt32 c, const uInt32 p) const
{
  auto [rn, gn, bn] = getRGBPhosphorTriple(c, p);
  return IG::PixelDescRGB565.build(rn >> 3, gn >> 2, bn >> 3, 0);
}

uInt32 FrameBuffer::getRGBPhosphor32(const uInt32 c, const uInt32 p) const
{
  auto [rn, gn, bn] = getRGBPhosphorTriple(c, p);
  return IG::PixelDescRGBA8888Native.build(rn, gn, bn, (uInt8)0);
}

template <int outputBits>
void FrameBuffer::renderOutput(IG::MutablePixmapView pix, TIA &tia)
{
	IG::PixmapView framePix{{{(int)tia.width(), (int)tia.height()}, IG::PixelFmtI8}, tia.frameBuffer()};
	assumeExpr(pix.size() == framePix.size());
	assumeExpr(pix.format().bytesPerPixel() == outputBits / 8);
	assumeExpr(framePix.format().bytesPerPixel() == 1);
	if(myUsePhosphor)
	{
		uint8_t* prevFrame = prevFramebuffer.data();
		pix.writeTransformed([this, &prevFrame](uint8_t p)
			{
				if constexpr(outputBits == 16)
				{
					return getRGBPhosphor16(tiaColorMap32[p], tiaColorMap32[*prevFrame++]);
				}
				else
				{
					return getRGBPhosphor32(tiaColorMap32[p], tiaColorMap32[*prevFrame++]);
				}
			}, framePix);
		memcpy(prevFramebuffer.data(), tia.frameBuffer(), sizeof(prevFramebuffer));
	}
	else
	{
		pix.writeTransformed([this](uint8_t p)
			{
				if constexpr(outputBits == 16)
				{
					return tiaColorMap16[p];
				}
				else
				{
					return tiaColorMap32[p];
				}
			}, framePix);
	}
}

void FrameBuffer::render(IG::MutablePixmapView pix, TIA &tia)
{
	if(format == IG::PixelFmtRGB565)
	{
		renderOutput<16>(pix, tia);
	}
	else
	{
		renderOutput<32>(pix, tia);
	}
}
