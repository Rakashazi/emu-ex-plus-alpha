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
#include <imagine/util/mdspan.hh>
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

	constexpr explicit operator bool() const { return bool(metrics); }
};

static void renderTextIntoBuffer(NSString *str, void *buff, int xSize, int ySize,
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

static GlyphRenderData makeGlyphRenderData(int idx, FontSize &fontSize, CGColorSpaceRef grayColorSpace,
	CGColorRef textColor, bool keepPixData)
{
	UniChar uniChar = idx;
	auto str = [[NSString alloc] initWithCharacters:&uniChar length:1];

	// measure max bounds
	CGSize size = [str sizeWithFont:fontSize.font()];
	if(!size.width || !size.height)
	{
		logMsg("invalid char 0x%X size %f:%f", idx, size.width, size.height);
		return {};
	}
	//logMsg("char %c size %f:%f", idx, size.width, size.height);
	int cXFullSize = size.width;
	int cYFullSize = size.height;

	// render char into buffer
	auto bufferSize = cXFullSize * cYFullSize;
	auto pixBuffer = (char*)std::calloc(1, bufferSize);
	renderTextIntoBuffer(str, pixBuffer, size.width, size.height,
		grayColorSpace, textColor, fontSize.font());

	// measure real bounds
	auto pixView = mdspan{pixBuffer, cYFullSize, cXFullSize};
	int minX = cXFullSize, maxX = 0, minY = cYFullSize, maxY = 0;
	for(auto y : iotaCount(cYFullSize))
		for(auto x : iotaCount(cXFullSize))
		{
			if(pixView[y, x])
			{
				if (x < minX) minX = x;
				if (x > maxX) maxX = x;
				if (y < minY) minY = y;
				if (y > maxY) maxY = y;
			}
		}
	//logMsg("min bounds %d:%d:%d:%d", minX, minY, maxX, maxY);
	int16_t cXOffset = minX;
	int16_t cXSize = (maxX - minX) + 1;
	int16_t cYOffset = minY;
	int16_t cYSize = (maxY - minY) + 1;
	auto startOfCharInPixBuffer = &pixView[cYOffset, cXOffset];

	GlyphMetrics metrics;
	metrics.size = {cXSize, cYSize};
	metrics.offset = {cXOffset, int16_t(-cYOffset)};
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

UIKitFontFontManager::UIKitFontFontManager(ApplicationContext)
{
	grayColorSpace = CGColorSpaceCreateDeviceGray();
	const CGFloat component[]{1., 1.};
	textColor = CGColorCreate(grayColorSpace, component);
}

UIKitFontFontManager::~UIKitFontFontManager()
{
	CGColorRelease(textColor);
	CGColorSpaceRelease(grayColorSpace);
}

Font FontManager::makeSystem() const
{
	return {grayColorSpace, textColor};
}

Font FontManager::makeBoldSystem() const
{
	return {grayColorSpace, textColor, FontWeight::BOLD};
}

Font::operator bool() const
{
	return true;
}

Font::Glyph Font::glyph(int idx, FontSize &size)
{
	auto glyphData = makeGlyphRenderData(idx, size, grayColorSpace, textColor, true);
	if(!glyphData)
	{
		return {};
	}
	PixmapView pix
	{
		{
			glyphData.metrics.size.as<int>(),
			IG::PixelFmtA8
		},
		glyphData.startOfCharInPixData,
		{glyphData.metrics.xAdvance, PixmapView::Units::BYTE}
	};
	return {{pix, glyphData.pixData}, glyphData.metrics};
}

GlyphMetrics Font::metrics(int idx, FontSize &size)
{
	auto glyphData = makeGlyphRenderData(idx, size, grayColorSpace, textColor, false);
	if(!glyphData)
	{
		return {};
	}
	return glyphData.metrics;
}

FontSize Font::makeSize(FontSettings settings)
{
	if(settings.pixelHeight() <= 0)
	{
		return {};
	}	
	if(weight == FontWeight::BOLD)
		return {(void*)CFBridgingRetain([UIFont boldSystemFontOfSize:(CGFloat)settings.pixelHeight()])};
	else
		return {(void*)CFBridgingRetain([UIFont systemFontOfSize:(CGFloat)settings.pixelHeight()])};
}

UIKitFontSize::UIKitFontSize(UIKitFontSize &&o) noexcept
{
	*this = std::move(o);
}

UIKitFontSize &UIKitFontSize::operator=(UIKitFontSize &&o) noexcept
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

UIKitGlyphImage::UIKitGlyphImage(UIKitGlyphImage &&o) noexcept
{
	*this = std::move(o);
}

UIKitGlyphImage &UIKitGlyphImage::operator=(UIKitGlyphImage &&o) noexcept
{
	deinit();
	pixmap_ = o.pixmap_;
	pixData_ = std::exchange(o.pixData_, {});
	return *this;
}

UIKitGlyphImage::~UIKitGlyphImage()
{
	deinit();
}

void UIKitGlyphImage::deinit()
{
	if(pixData_)
	{
		std::free(std::exchange(pixData_, {}));
	}
}

PixmapView GlyphImage::pixmap()
{
	return pixmap_;
}

GlyphImage::operator bool() const
{
	return (bool)pixData_;
}

}
