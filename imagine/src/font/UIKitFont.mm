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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/container/array.hh>
#include "../base/iphone/private.hh"
#import <CoreGraphics/CGBitmapContext.h>
#import <CoreGraphics/CGContext.h>
#import <UIKit/UIKit.h>
#include <cstdlib>

namespace IG
{

struct GlyphRenderData
{
	GlyphMetrics metrics{};
	void *pixData{};
	void *startOfCharInPixData{};
};

static CGColorRef textColor{};

static void renderTextIntoBuffer(NSString *str, void *buff, uint32_t xSize, uint32_t ySize,
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
	uint32_t cXFullSize = size.width;
	uint32_t cYFullSize = size.height;

	// render char into buffer
	uint32_t bufferSize = cXFullSize * cYFullSize;
	auto pixBuffer = (char*)std::calloc(1, bufferSize);
	renderTextIntoBuffer(str, pixBuffer, size.width, size.height,
		Base::grayColorSpace, textColor, fontSize.font());

	// measure real bounds
	auto pixView = IG::ArrayView2<char>{pixBuffer, cXFullSize};
	uint32_t minX = cXFullSize, maxX = 0, minY = cYFullSize, maxY = 0;
	iterateTimes(cYFullSize, y)
		iterateTimes(cXFullSize, x)
		{
			if(pixView[y][x])
			{
				if (x < minX) minX = x;
				if (x > maxX) maxX = x;
				if (y < minY) minY = y;
				if (y > maxY) maxY = y;
			}
		}
	//logMsg("min bounds %d:%d:%d:%d", minX, minY, maxX, maxY);
	auto cXOffset = minX;
	uint32_t cXSize = (maxX - minX) + 1;
	auto cYOffset = minY;
	uint32_t cYSize = (maxY - minY) + 1;
	auto startOfCharInPixBuffer = &pixView[cYOffset][cXOffset];

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
		std::free(pixBuffer);
		return {metrics, nullptr, nullptr};
	}
}

Font Font::makeSystem(Base::ApplicationContext)
{
	if(!textColor) [[unlikely]]
	{
		const CGFloat component[]{1., 1.};
		textColor = CGColorCreate(Base::grayColorSpace, component);
	}
	return {};
}

Font Font::makeBoldSystem(Base::ApplicationContext ctx)
{
	Font font = makeSystem(ctx);
	font.isBold = true;
	return font;
}

UIKitFont::UIKitFont(UIKitFont &&o)
{
	*this = std::move(o);
}

UIKitFont &UIKitFont::operator=(UIKitFont &&o)
{
	isBold = o.isBold;
	return *this;
}

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
		{(uint32_t)glyphData.metrics.xAdvance, IG::Pixmap::BYTE_UNITS}
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

UIKitFontSize::UIKitFontSize(void *font): font_{font} {}

UIKitFontSize::UIKitFontSize(UIKitFontSize &&o)
{
	*this = std::move(o);
}

UIKitFontSize &UIKitFontSize::operator=(UIKitFontSize &&o)
{
	deinit();
	font_ = std::exchange(o.font_, {});
	return *this;
}

UIKitFontSize::~UIKitFontSize()
{
	deinit();
}

void UIKitFontSize::deinit()
{
	if(!font_)
		return;
	CFRelease(font_);
}

UIKitGlyphImage::UIKitGlyphImage(IG::Pixmap pixmap, void *pixData):
	pixmap_{pixmap},
	pixData_{pixData}
{}

UIKitGlyphImage::UIKitGlyphImage(UIKitGlyphImage &&o)
{
	*this = std::move(o);
}

UIKitGlyphImage &UIKitGlyphImage::operator=(UIKitGlyphImage &&o)
{
	static_cast<GlyphImage*>(this)->unlock();
	pixmap_ = o.pixmap_;
	pixData_ = std::exchange(o.pixData_, {});
	return *this;
}

UIKitGlyphImage::~UIKitGlyphImage()
{
	static_cast<GlyphImage*>(this)->unlock();
}

void GlyphImage::unlock()
{
	if(pixData_)
	{
		std::free(pixData_);
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
