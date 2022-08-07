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

#define CONV_COL(x) 0, x
alignas(2) constexpr uint8_t scanlinePixmapBuff[]{ CONV_COL(0x00), CONV_COL(0xff) };
#undef CONV_COL

#define CONV_COL(x) 31, x
alignas(8) constexpr uint8_t crtPixmapBuff[]
{
		CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00),
		CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff),
		CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00),
		CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff),
		CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00),
		CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff),
		CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00),
		CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff),
};
#undef CONV_COL

#define CONV_COL(r,g,b) IG::PIXEL_DESC_RGBA8888_NATIVE.build(r, g, b, 0xff)
alignas(8) constexpr uint32_t crtRgbPixmapBuff[]
{
		CONV_COL(0xcc,0,0x32), CONV_COL(0xff,0,0), CONV_COL(0xcb,0x33,0), CONV_COL(0x98,0x66,0), CONV_COL(0x65,0x99,0), CONV_COL(0x32,0xcc,0), CONV_COL(0,0xff,0), CONV_COL(0,0xcb,0x33), CONV_COL(0,0x98,0x66), CONV_COL(0,0x65,0x99), CONV_COL(0,0x32,0xcc), CONV_COL(0,0,0xff), CONV_COL(0x33,0,0xcb), CONV_COL(0x66,0,0x98), CONV_COL(0x99,0,0x65), CONV_COL(0xcb,0,0x33),
		CONV_COL(0,0x98,0x66), CONV_COL(0,0x65,0x99), CONV_COL(0,0x32,0xcc), CONV_COL(0,0,0xff), CONV_COL(0x33,0,0xcb), CONV_COL(0x66,0,0x98), CONV_COL(0x99,0,0x65), CONV_COL(0xcb,0,0x33), CONV_COL(0xcc,0,0x32), CONV_COL(0xff,0,0), CONV_COL(0xcb,0x33,0), CONV_COL(0x98,0x66,0), CONV_COL(0x65,0x99,0), CONV_COL(0x32,0xcc,0), CONV_COL(0,0xff,0), CONV_COL(0,0xcb,0x33),
};
#undef CONV_COL

void VideoImageOverlay::setEffect(Gfx::Renderer &r, ImageOverlayId id)
{
	if(overlayId == id)
		return;
	overlayId = id;
	auto pix = [&]() -> IG::MutablePixmapView
	{
		switch(id)
		{
			case ImageOverlayId::SCANLINES ... ImageOverlayId::SCANLINES_2:
				return {{{1, 2}, IG::PIXEL_IA88}, scanlinePixmapBuff};
			case ImageOverlayId::CRT:
				return {{{8, 8}, IG::PIXEL_IA88}, crtPixmapBuff};
			case ImageOverlayId::CRT_RGB ... ImageOverlayId::CRT_RGB_2:
				return {{{16, 2}, IG::PIXEL_RGBA8888}, crtRgbPixmapBuff};
		}
		return {};
	}();
	if(!pix) // turn off effect
	{
		spr = {};
		img = {};
		return;
	}
	Gfx::TextureConfig texConf{pix.desc(), &r.make(Gfx::CommonTextureSampler::NEAREST_MIP_REPEAT)};
	texConf.setWillGenerateMipmaps(true);
	img = r.makeTexture(texConf);
	img.write(0, pix, {});
	img.generateMipmaps();
	spr = {{}, img};
	spr.compileDefaultProgramOneShot(Gfx::EnvMode::MODULATE);
}

void VideoImageOverlay::setIntensity(float i)
{
	intensity = i;
}

void VideoImageOverlay::place(const Gfx::Sprite &disp, int lines, IG::Rotation r)
{
	if(!spr.image())
		return;
	using namespace IG::Gfx;
	//logMsg("placing overlay with %u lines in image", lines);
	spr.setPos(disp);
	auto width = lines * EmuSystem::aspectRatioInfos()[0].aspect.ratio<float>();
	spr.setImg([&]() -> TextureSpan
	{
		switch(overlayId)
		{
			case ImageOverlayId::SCANLINES:
				return {&img, {{}, {1.0f, (float)lines}}};
			case ImageOverlayId::SCANLINES_2:
				return {&img, {{}, {1.0f, lines*2.f}}};
			case ImageOverlayId::CRT:
				return {&img, {{}, {width/2.f, lines/2.f}}};
			case ImageOverlayId::CRT_RGB:
				return {&img, {{}, {width/2.f, (float)lines}}};
			case ImageOverlayId::CRT_RGB_2:
				return {&img, {{}, {width/2.f, lines*2.f}}};
		}
		bug_unreachable("invalid overlayId:%d", std::to_underlying(overlayId));
	}(), r);
}

void VideoImageOverlay::draw(Gfx::RendererCommands &cmds)
{
	if(!spr.image())
		return;
	using namespace IG::Gfx;
	cmds.set(CommonTextureSampler::NEAREST_MIP_REPEAT);
	cmds.setColor(1., 1., 1., intensity);
	cmds.set(BlendMode::ALPHA);
	spr.setCommonProgram(cmds, EnvMode::MODULATE);
	spr.draw(cmds);
}

}
