/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/VideoImageOverlay.hh>
#include <emuframework/EmuSystem.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/Vec3.hh>

namespace EmuEx
{

constexpr uint32_t slCol(uint8_t a) { return IG::PixelDescRGBA8888Native.build(0, 0, 0, a); }

constexpr uint32_t scanlinePixmapBuff[]
{
	slCol(0x00),
	slCol(0xff)
};

constexpr uint32_t lcdCol(uint8_t a) { return IG::PixelDescRGBA8888Native.build(0, 0, 0, a); }

constexpr uint32_t lcdPixmapBuff[]
{
	lcdCol(0xe6), lcdCol(0xe6), lcdCol(0xe6), lcdCol(0xe6), lcdCol(0xe6), lcdCol(0xe6), lcdCol(0xe6), lcdCol(0xe6),
	lcdCol(0xe6), lcdCol(0xe6), lcdCol(0x6f), lcdCol(0x6f), lcdCol(0x6f), lcdCol(0x6f), lcdCol(0x6f), lcdCol(0x6f),
	lcdCol(0xe6), lcdCol(0x6f), lcdCol(0x33), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00),
	lcdCol(0xe6), lcdCol(0x6f), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00),
	lcdCol(0xe6), lcdCol(0x6f), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00),
	lcdCol(0xe6), lcdCol(0x6f), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00),
	lcdCol(0xe6), lcdCol(0x6f), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00),
	lcdCol(0xe6), lcdCol(0x6f), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00), lcdCol(0x00),
};

constexpr uint32_t crtCol(uint8_t r, uint8_t g, uint8_t b)
{
	return IG::PixelDescRGBA8888Native.build(r, g, b, 0xff);
}

constexpr WSize crtTexSize{4, 4};

constexpr uint32_t crtMaskPixmapBuff[crtTexSize.x * crtTexSize.y]
{
	crtCol(0xff,0,0), crtCol(0,0xff,0), crtCol(0,0xff,0), crtCol(0,0,0xff),
	crtCol(0,0xff,0), crtCol(0,0,0xff), crtCol(0xff,0,0), crtCol(0,0xff,0),
	crtCol(0,0,0),    crtCol(0,0,0),    crtCol(0,0,0),    crtCol(0,0,0),
	crtCol(0,0,0),    crtCol(0,0,0),    crtCol(0,0,0),    crtCol(0,0,0),
};

constexpr uint32_t crtGrillePixmapBuff[crtTexSize.x * crtTexSize.y]
{
	crtCol(0xff,0,0), crtCol(0,0xff,0), crtCol(0,0xff,0), crtCol(0,0,0xff),
	crtCol(0xff,0,0), crtCol(0,0xff,0), crtCol(0,0xff,0), crtCol(0,0,0xff),
	crtCol(0,0,0),    crtCol(0,0,0),    crtCol(0,0,0),    crtCol(0,0,0),
	crtCol(0,0,0),    crtCol(0,0,0),    crtCol(0,0,0),    crtCol(0,0,0),
};

struct OverlayDesc
{
	PixmapView pixView;
	Gfx::WrapMode wrapMode;
};

constexpr OverlayDesc overlayDesc(ImageOverlayId id)
{
	switch(id)
	{
		case ImageOverlayId::SCANLINES ... ImageOverlayId::SCANLINES_2:
			return {{{{1, 2}, PixelFmtRGBA8888}, scanlinePixmapBuff}, Gfx::WrapMode::REPEAT};
		case ImageOverlayId::LCD:
			return {{{{8, 8}, PixelFmtRGBA8888}, lcdPixmapBuff}, Gfx::WrapMode::MIRROR_REPEAT};
		case ImageOverlayId::CRT_MASK ... ImageOverlayId::CRT_MASK_2:
			return {{{crtTexSize, PixelFmtRGBA8888}, crtMaskPixmapBuff}, Gfx::WrapMode::REPEAT};
		case ImageOverlayId::CRT_GRILLE ... ImageOverlayId::CRT_GRILLE_2:
			return {{{crtTexSize, PixelFmtRGBA8888}, crtGrillePixmapBuff}, Gfx::WrapMode::REPEAT};
	}
	bug_unreachable("invalid ImageOverlayId");
}

constexpr bool isCrtOverlay(ImageOverlayId id)
{
	return id >= ImageOverlayId::CRT_MASK && id <= ImageOverlayId::CRT_GRILLE_2;
}

void VideoImageOverlay::setEffect(Gfx::Renderer &r, ImageOverlayId id, Gfx::ColorSpace colorSpace)
{
	if(overlayId == id)
		return;
	overlayId = id;
	if(!to_underlying(id)) // turn off effect
	{
		quad = {};
		texture = {};
		return;
	}
	quad = {r.mainTask, {.size = 1}};
	multiplyBlend = isCrtOverlay(id);
	auto desc = overlayDesc(id);
	Gfx::TextureSamplerConfig samplerConf{ .mipFilter = Gfx::MipFilter::NEAREST };
	samplerConf.setWrapMode(desc.wrapMode);
	Gfx::TextureConfig texConf{desc.pixView.desc(), samplerConf};
	texConf.colorSpace = colorSpace;
	texConf.setWillGenerateMipmaps(true);
	texture = r.makeTexture(texConf);
	texture.write(0, desc.pixView, {}, {.makeMipmaps = true});
}

void VideoImageOverlay::setIntensity(float i)
{
	intensity = i;
}

void VideoImageOverlay::place(WRect contentRect, WSize videoPixels, Rotation r)
{
	if(!texture || videoPixels.y <= 1)
		return;
	using namespace IG::Gfx;
	const float width2x = videoPixels.x * 2.f;
	const bool is240p = videoPixels.y <= 256;
	const float lines = is240p ? videoPixels.y : videoPixels.y * .5f;
	const float lines2x = is240p ? videoPixels.y * 2.f : videoPixels.y;
	const F2Size crtDots = {contentRect.xSize() / float(crtTexSize.x), contentRect.ySize() / float(crtTexSize.y * 2)};
	const F2Size crtDotsHalf = crtDots / F2Size{2.f, 1.f};
	auto tex = [&] -> TextureSpan
	{
		switch(overlayId)
		{
			case ImageOverlayId::SCANLINES:
				return {&texture, {{}, {1.f, lines}}};
			case ImageOverlayId::SCANLINES_2:
				return {&texture, {{}, {1.f, lines2x}}};
			case ImageOverlayId::LCD:
				return {&texture, {{}, {width2x, lines2x}}};
			case ImageOverlayId::CRT_MASK:
			case ImageOverlayId::CRT_GRILLE:
				return {&texture, {{}, crtDots}};
			case ImageOverlayId::CRT_MASK_2:
			case ImageOverlayId::CRT_GRILLE_2:
				return {&texture, {{}, crtDotsHalf}};
		}
		bug_unreachable("invalid ImageOverlayId");
	}();
	quad.write(0, {.bounds = contentRect.as<int16_t>(), .textureSpan = tex, .rotation = r});
}

void VideoImageOverlay::draw(Gfx::RendererCommands &cmds, Gfx::Vec3 brightness)
{
	if(!texture)
		return;
	using namespace IG::Gfx;
	if(multiplyBlend)
	{
		brightness *= 2.f;
		cmds.setColor({brightness.r, brightness.g, brightness.b, intensity});
		cmds.setBlendFunc(BlendFunc::DST_COLOR, BlendFunc::SRC_ALPHA);
		cmds.setBlend(true);
	}
	else
	{
		cmds.setColor({brightness.r, brightness.g, brightness.b, intensity});
		cmds.set(BlendMode::PREMULT_ALPHA);
	}
	cmds.basicEffect().drawSprite(cmds, quad, 0, texture);
}

}
