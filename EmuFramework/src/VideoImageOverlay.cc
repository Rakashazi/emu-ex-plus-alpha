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

#define LOGTAG "VideoImageOverlay"
#include <emuframework/VideoImageOverlay.hh>
#include <emuframework/EmuSystem.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr uint32_t slCol(uint8_t a) { return IG::PIXEL_DESC_RGBA8888_NATIVE.build(0, 0, 0, a); }

constexpr uint32_t scanlinePixmapBuff[]
{
	slCol(0x00),
	slCol(0xff)
};

constexpr uint32_t lcdCol(uint8_t a) { return IG::PIXEL_DESC_RGBA8888_NATIVE.build(0, 0, 0, a); }

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
	return IG::PIXEL_DESC_RGBA8888_NATIVE.build(r, g, b, 0xff);
}

constexpr IP crtTexSize{4, 4};

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
			return {{{{1, 2}, PIXEL_RGBA8888}, scanlinePixmapBuff}, Gfx::WrapMode::REPEAT};
		case ImageOverlayId::LCD:
			return {{{{8, 8}, PIXEL_RGBA8888}, lcdPixmapBuff}, Gfx::WrapMode::MIRROR_REPEAT};
		case ImageOverlayId::CRT_MASK ... ImageOverlayId::CRT_MASK_2:
			return {{{crtTexSize, PIXEL_RGBA8888}, crtMaskPixmapBuff}, Gfx::WrapMode::REPEAT};
		case ImageOverlayId::CRT_GRILLE ... ImageOverlayId::CRT_GRILLE_2:
			return {{{crtTexSize, PIXEL_RGBA8888}, crtGrillePixmapBuff}, Gfx::WrapMode::REPEAT};
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
		spr = {};
		img = {};
		sampler = {};
		return;
	}
	multiplyBlend = isCrtOverlay(id);
	auto desc = overlayDesc(id);
	Gfx::TextureSamplerConfig samplerConf
	{
		.mipFilter = Gfx::MipFilter::NEAREST,
		.debugLabel = "VideoImageOverlay"
	};
	samplerConf.setWrapMode(desc.wrapMode);
	sampler = r.makeTextureSampler(samplerConf);
	Gfx::TextureConfig texConf{desc.pixView.desc(), &sampler};
	texConf.colorSpace = colorSpace;
	texConf.setWillGenerateMipmaps(true);
	img = r.makeTexture(texConf);
	img.write(0, desc.pixView, {}, Gfx::Texture::WRITE_FLAG_MAKE_MIPMAPS);
	spr = {{{0.f, 0.f}, {0.f, 0.f}}, img};
}

void VideoImageOverlay::setIntensity(float i)
{
	intensity = i;
}

void VideoImageOverlay::place(const Gfx::Sprite &disp, IG::WRect contentRect, WP videoPixels, IG::Rotation r)
{
	if(!spr.hasTexture() || videoPixels.y <= 1)
		return;
	using namespace IG::Gfx;
	//logMsg("placing overlay with %d lines in image", videoPixels.y);
	spr.setPos(disp);
	const float width2x = videoPixels.x * 2.f;
	const bool is240p = videoPixels.y <= 256;
	const float lines = is240p ? videoPixels.y : videoPixels.y * .5f;
	const float lines2x = is240p ? videoPixels.y * 2.f : videoPixels.y;
	const FP crtDots = {contentRect.xSize() / float(crtTexSize.x), contentRect.ySize() / float(crtTexSize.y * 2)};
	const FP crtDotsHalf = crtDots / FP{2.f, 1.f};
	spr.set([&]() -> TextureSpan
	{
		switch(overlayId)
		{
			case ImageOverlayId::SCANLINES:
				return {&img, {{}, {1.f, lines}}};
			case ImageOverlayId::SCANLINES_2:
				return {&img, {{}, {1.f, lines2x}}};
			case ImageOverlayId::LCD:
				return {&img, {{}, {width2x, lines2x}}};
			case ImageOverlayId::CRT_MASK:
			case ImageOverlayId::CRT_GRILLE:
				return {&img, {{}, crtDots}};
			case ImageOverlayId::CRT_MASK_2:
			case ImageOverlayId::CRT_GRILLE_2:
				return {&img, {{}, crtDotsHalf}};
		}
		bug_unreachable("invalid ImageOverlayId");
	}(), r);
}

void VideoImageOverlay::draw(Gfx::RendererCommands &cmds, float brightness)
{
	if(!spr.hasTexture())
		return;
	using namespace IG::Gfx;
	if(multiplyBlend)
	{
		brightness *= 2.f;
		cmds.setColor(brightness, brightness, brightness, intensity);
		cmds.setBlendFunc(BlendFunc::DST_COLOR, BlendFunc::SRC_ALPHA);
		cmds.setBlend(true);
	}
	else
	{
		cmds.setColor(brightness, brightness, brightness, intensity);
		cmds.set(BlendMode::ALPHA);
	}
	cmds.setTextureSampler(sampler);
	spr.draw(cmds, cmds.basicEffect());
}

}
