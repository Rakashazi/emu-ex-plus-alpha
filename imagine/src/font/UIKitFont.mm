/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

static_assert(__has_feature(objc_arc), "This file requires ARC");
#define LOGTAG "UIKitFont"
#include <imagine/font/Font.hh>
#include <imagine/logger/logger.h>
#include <imagine/gfx/Gfx.hh>
#include <imagine/util/Mem2D.hh>
#include <imagine/mem/mem.h>
#include "../base/iphone/private.hh"
#import <CoreGraphics/CGBitmapContext.h>
#import <CoreGraphics/CGContext.h>
#import <UIKit/UIKit.h>

namespace IG
{

struct GlyphRenderData
{
	GlyphMetrics metrics{};
	void *pixData{};
	void *startOfCharInPixData{};
};

static CGColorRef textColor{};

static void renderTextIntoBuffer(NSString *str, void *buff, uint xSize, uint ySize,
		CGColorSpaceRef colorSpace, CGColorRef textColor, UIFont *font)
{
	auto context = CGBitmapContextCreate(buff, xSize, ySize, 8, xSize, colorSpace, kCGImageAlphaOnly);
	assert(context);
	CGContextSetBlendMode(context, kCGBlendModeCopy);
	CGContextSetTextDrawingMode(context, kCGTextFill);
	//CGContextSetStrokeColorWithColor(context, textColor);
	CGContextSetFillColorWithColor(context, textColor);
	CGContextTranslateCTM(context, 0.0f, ySize);
	CGContextScaleCTM(context, 1.0f, -1.0f);
	UIGraphicsPushContext(context);
	//[str drawInRect:CGRectMake(0, 0, xSize, ySize) withFont:font];
	[str drawAtPoint:CGPointMake(0, 0) withFont:font];
	UIGraphicsPopContext();
	CGContextRelease(context);
}

static GlyphRenderData makeGlyphRenderData(int idx, FontSize &fontSize, bool keepPixData, std::errc &ec)
{
	UniChar uniChar = idx;
	auto str = [[NSString alloc] initWithCharacters:&uniChar length:1];

	// measure max bounds
	CGSize size = [str sizeWithFont:fontSize.font()];
	if(!size.width || !size.height)
	{
		logMsg("invalid char 0x%X size %f:%f", idx, size.width, size.height);
		ec = std::errc::invalid_argument;
		return {};
	}
	ec = (std::errc)0;
	//logMsg("char %c size %f:%f", idx, size.width, size.height);
	uint cXFullSize = size.width;
	uint cYFullSize = size.height;

	// render char into buffer
	uint bufferSize = cXFullSize * cYFullSize;
	auto pixBuffer = (char*)mem_calloc(bufferSize);
	renderTextIntoBuffer(str, pixBuffer, size.width, size.height,
		Base::grayColorSpace, textColor, fontSize.font());

	// measure real bounds
	uint minX = cXFullSize, maxX = 0, minY = cYFullSize, maxY = 0;
	iterateTimes(cYFullSize, y)
		iterateTimes(cXFullSize, x)
		{
			if(pixBuffer[Mem2D<char>::arrOffsetRM(x, y, cXFullSize)])
			{
				if (x < minX) minX = x;
				if (x > maxX) maxX = x;
				if (y < minY) minY = y;
				if (y > maxY) maxY = y;
			}
		}
	//logMsg("min bounds %d:%d:%d:%d", minX, minY, maxX, maxY);
	auto cXOffset = minX;
	uint cXSize = (maxX - minX) + 1;
	auto cYOffset = minY;
	uint cYSize = (maxY - minY) + 1;
	auto startOfCharInPixBuffer = pixBuffer + Mem2D<char>::arrOffsetRM(cXOffset, cYOffset, cXFullSize);

	GlyphMetrics metrics;
	metrics.xSize = cXSize;
	metrics.ySize = cYSize;
	metrics.xOffset = cXOffset;
	metrics.yOffset = -cYOffset;
	metrics.xAdvance = cXFullSize;
	
	if(keepPixData)
	{
		return {metrics, pixBuffer, startOfCharInPixBuffer};
	}
	else
	{
		mem_free(pixBuffer);
		return {metrics, nullptr, nullptr};
	}
}

Font::Font() {}

Font Font::makeSystem()
{
	if(unlikely(!textColor))
	{
		const CGFloat component[]{1., 1.};
		textColor = CGColorCreate(Base::grayColorSpace, component);
	}
	return {};
}

Font Font::makeBoldSystem()
{
	Font font = makeSystem();
	font.isBold = true;
	return font;
}

Font::Font(Font &&o)
{
	swap(*this, o);
}

Font &Font::operator=(Font o)
{
	swap(*this, o);
	return *this;
}

void Font::swap(Font &a, Font &b)
{
	std::swap(a.isBold, b.isBold);
}

Font::~Font() {}

Font::operator bool() const
{
	return true;
}

Font::Glyph Font::glyph(int idx, FontSize &size, std::errc &ec)
{
	auto glyphData = makeGlyphRenderData(idx, size, true, ec);
	if((bool)ec)
	{
		return {};
	}
	IG::Pixmap pix
	{
		{
			{glyphData.metrics.xSize, glyphData.metrics.ySize},
			IG::PIXEL_FMT_A8
		},
		glyphData.startOfCharInPixData,
		{(uint)glyphData.metrics.xAdvance, IG::Pixmap::BYTE_UNITS}
	};
	return {{pix, glyphData.pixData}, glyphData.metrics};
}

GlyphMetrics Font::metrics(int idx, FontSize &size, std::errc &ec)
{
	auto glyphData = makeGlyphRenderData(idx, size, false, ec);
	if((bool)ec)
	{
		return {};
	}
	return glyphData.metrics;
}

FontSize Font::makeSize(FontSettings settings, std::errc &ec)
{
	if(settings.pixelHeight() <= 0)
	{
		ec = std::errc::invalid_argument;
		return {};
	}	
	ec = (std::errc)0;
	if(isBold)
		return {(void*)CFBridgingRetain([UIFont boldSystemFontOfSize:(CGFloat)settings.pixelHeight()])};
	else
		return {(void*)CFBridgingRetain([UIFont systemFontOfSize:(CGFloat)settings.pixelHeight()])};
}

UIKitFontSize::UIKitFontSize(UIKitFontSize &&o)
{
	swap(*this, o);
}

UIKitFontSize &UIKitFontSize::operator=(UIKitFontSize o)
{
	swap(*this, o);
	return *this;
}

UIKitFontSize::~UIKitFontSize()
{
	if(!font_)
		return;
	CFRelease(font_);
}

void UIKitFontSize::swap(UIKitFontSize &a, UIKitFontSize &b)
{
	std::swap(a.font_, b.font_);
}

GlyphImage::GlyphImage(GlyphImage &&o)
{
	swap(*this, o);
}

GlyphImage &GlyphImage::operator=(GlyphImage o)
{
	swap(*this, o);
	return *this;
}

void GlyphImage::swap(GlyphImage &a, GlyphImage &b)
{
	std::swap(a.pixmap_, b.pixmap_);
	std::swap(a.pixData_, b.pixData_);
}

void GlyphImage::unlock()
{
	if(pixData_)
	{
		mem_free(pixData_);
		pixData_ = {};
	}
	pixmap_ = {};
}

IG::Pixmap GlyphImage::pixmap()
{
	return pixmap_;
}

GlyphImage::operator bool() const
{
	return (bool)pixmap_;
}

}
