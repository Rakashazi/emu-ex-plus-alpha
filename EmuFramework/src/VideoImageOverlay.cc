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

alignas(8) constexpr uint32_t scanlinePixmapBuff[]
{
	slCol(0x00),
	slCol(0xff)
};

constexpr uint32_t lcdCol(uint8_t a) { return IG::PIXEL_DESC_RGBA8888_NATIVE.build(0, 0, 0, a); }

alignas(8) constexpr uint32_t lcdPixmapBuff[]
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

alignas(8) constexpr uint32_t crtPixmapBuff[]
{
	crtCol(0xff,0,0), crtCol(0xff,0,0), crtCol(0,0xff,0), crtCol(0,0xff,0), crtCol(0,0,0xff), crtCol(0,0,0xff),
	crtCol(0,0xff,0), crtCol(0,0,0xff), crtCol(0,0,0xff), crtCol(0xff,0,0), crtCol(0xff,0,0), crtCol(0,0xff,0),
};

void VideoImageOverlay::setEffect(Gfx::Renderer &r, ImageOverlayId id, Gfx::ColorSpace colorSpace)
{
	if(overlayId == id)
		return;
	overlayId = id;
	auto pix = [&]() -> IG::MutablePixmapView
	{
		switch(id)
		{
			case ImageOverlayId::SCANLINES ... ImageOverlayId::SCANLINES_2:
				return {{{1, 2}, IG::PIXEL_RGBA8888}, scanlinePixmapBuff};
			case ImageOverlayId::LCD:
				return {{{8, 8}, IG::PIXEL_RGBA8888}, lcdPixmapBuff};
			case ImageOverlayId::CRT_RGB ... ImageOverlayId::CRT_RGB_2:
				return {{{6, 2}, IG::PIXEL_RGBA8888}, crtPixmapBuff};
		}
		return {};
	}();
	if(!pix) // turn off effect
	{
		spr = {};
		img = {};
		sampler = {};
		return;
	}
	Gfx::TextureSamplerConfig samplerConf
	{
		.mipFilter = Gfx::MipFilter::NEAREST,
		.debugLabel = "VideoImageOverlay"
	};
	if(id == ImageOverlayId::LCD)
		samplerConf.setWrapMode(Gfx::WrapMode::MIRROR_REPEAT);
	else
		samplerConf.setWrapMode(Gfx::WrapMode::REPEAT);
	sampler = r.makeTextureSampler(samplerConf);
	Gfx::TextureConfig texConf{pix.desc(), &sampler};
	texConf.colorSpace = colorSpace;
	texConf.setWillGenerateMipmaps(true);
	img = r.makeTexture(texConf);
	img.write(0, pix, {}, Gfx::Texture::WRITE_FLAG_MAKE_MIPMAPS);
	spr = {{{0.f, 0.f}, {0.f, 0.f}}, img};
	multiplyBlend = id == ImageOverlayId::CRT_RGB || id == ImageOverlayId::CRT_RGB_2;
}

void VideoImageOverlay::setIntensity(float i)
{
	intensity = i;
}

void VideoImageOverlay::place(const Gfx::Sprite &disp, int lines, IG::Rotation r)
{
	if(!spr.hasTexture() || lines <= 1)
		return;
	using namespace IG::Gfx;
	//logMsg("placing overlay with %u lines in image", lines);
	spr.setPos(disp);
	auto width = lines * EmuSystem::aspectRatioInfos()[0].aspect.ratio<float>();
	spr.set([&]() -> TextureSpan
	{
		switch(overlayId)
		{
			case ImageOverlayId::SCANLINES:
				return {&img, {{}, {1.f, (float)lines}}};
			case ImageOverlayId::SCANLINES_2:
				return {&img, {{}, {1.f, lines * 2.f}}};
			case ImageOverlayId::LCD:
				return {&img, {{}, {width * 2.f, lines * 2.f}}};
			case ImageOverlayId::CRT_RGB:
				return {&img, {{}, {width, (float)lines}}};
			case ImageOverlayId::CRT_RGB_2:
				return {&img, {{}, {width, lines * 2.f}}};
		}
		bug_unreachable("invalid overlayId:%d", std::to_underlying(overlayId));
	}(), r);
}

void VideoImageOverlay::draw(Gfx::RendererCommands &cmds, float brightness)
{
	if(!spr.hasTexture())
		return;
	using namespace IG::Gfx;
	cmds.setColor(brightness, brightness, brightness, intensity);
	if(multiplyBlend)
	{
		cmds.setBlendFunc(BlendFunc::DST_COLOR, BlendFunc::SRC_ALPHA);
		cmds.setBlend(true);
	}
	else
	{
		cmds.set(BlendMode::ALPHA);
	}
	cmds.setTextureSampler(sampler);
	spr.draw(cmds, cmds.basicEffect());
}

}
