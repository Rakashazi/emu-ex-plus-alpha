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

#define LOGTAG "VideoLayer"
#include <emuframework/EmuVideoLayer.hh>
#include <emuframework/EmuInputView.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/VController.hh>
#include "EmuOptions.hh"
#include <imagine/util/math/Point2D.hh>
#include <imagine/base/Window.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/Vec3.hh>
#include <imagine/glm/common.hpp>
#include <imagine/glm/gtc/color_space.hpp>
#include <imagine/logger/logger.h>
#include <algorithm>

namespace EmuEx
{

EmuVideoLayer::EmuVideoLayer(EmuVideo &video, float defaultAspectRatio):
	video{video},
	landscapeAspectRatio{defaultAspectRatio},
	portraitAspectRatio{defaultAspectRatio} {}

void EmuVideoLayer::place(IG::WindowRect viewRect, IG::WindowRect displayRect, EmuInputView *inputView, EmuSystem &sys)
{
	if(sys.hasContent() && video.size().x)
	{
		auto viewportAspectRatio = displayRect.xSize() / (float)displayRect.ySize();
		auto zoom = zoom_;
		auto contentSize = video.size();
		if(isSideways(rotation))
			std::swap(contentSize.x, contentSize.y);
		contentRect_ = {};
		if(zoom == optionImageZoomIntegerOnly || zoom == optionImageZoomIntegerOnlyY)
		{
			int x = contentSize.x, y = contentSize.y;

			// Halve pixel sizes if image has mixed low/high-res content so scaling is based on lower res,
			// this prevents jumping between two screen sizes in games like Seiken Densetsu 3 on SNES
			auto multiresVideoBaseSize = sys.multiresVideoBaseSize();
			if(multiresVideoBaseSize.x && x > multiresVideoBaseSize.x)
			{
				logMsg("halving X size for multires content");
				x /= 2;
			}
			if(multiresVideoBaseSize.y && y > multiresVideoBaseSize.y)
			{
				logMsg("halving Y size for multires content");
				y /= 2;
			}

			auto aR = x / float(y);

			// avoid overly wide images (SNES, etc.) or tall images (2600, etc.)
			if(aR >= 2.f)
			{
				logMsg("unscaled image too wide, doubling height to compensate");
				y *= 2;
				aR = x / float(y);
			}
			else if(aR < 0.8f)
			{
				logMsg("unscaled image too tall, doubling width to compensate");
				x *= 2;
				aR = x / float(y);
			}

			int scaleFactor;
			if(aR > viewportAspectRatio)
			{
				scaleFactor = std::max(1, displayRect.xSize() / x);
				logMsg("using x scale factor %d", scaleFactor);
			}
			else
			{
				scaleFactor = std::max(1, displayRect.ySize() / y);
				logMsg("using y scale factor %d", scaleFactor);
			}
			contentRect_.x2 = x * scaleFactor;
			contentRect_.y2 = y * scaleFactor;
		}
		if(zoom <= 100 || zoom == optionImageZoomIntegerOnlyY)
		{
			auto aR = evalAspectRatio(viewportAspectRatio < 1.f ? portraitAspectRatio : landscapeAspectRatio)
				* sys.videoAspectRatioScale();
			if(isSideways(rotation))
				aR = 1. / aR;
			if(zoom == optionImageZoomIntegerOnlyY)
			{
				// get width from previously calculated pixel height
				float width = contentRect_.ySize() * (float)aR;
				if(!aR)
				{
					width = displayRect.xSize();
				}
				contentRect_.x2 = width;
			}
			else
			{
				auto size = displayRect.size();
				if(aR)
				{
					size = IG::sizesWithRatioBestFit((float)aR, size.x, size.y);
				}
				contentRect_.x2 = size.x;
				contentRect_.y2 = size.y;
				if(zoom < 100)
				{
					auto scaler = zoom / 100.f;
					contentRect_.x2 *= scaler;
					contentRect_.y2 *= scaler;
				}
			}
		}
		if(viewportAspectRatio < 1.f && inputView)
		{
			contentRect_.setPos(viewRect.pos(CT2DO) + WPt{0, inputView->uiElementHeight() + portraitOffset}, CT2DO);
		}
		else
		{
			contentRect_.setPos(displayRect.center() + WPt{landscapeOffset, 0}, C2DO);
		}
		contentRect_.fitIn(displayRect);
		quad.write(0, {.bounds = contentRect_.as<int16_t>(), .textureSpan = texture, .rotation = rotation});
		logMsg("placed game rect, at pixels %d:%d:%d:%d",
			contentRect_.x, contentRect_.y, contentRect_.x2, contentRect_.y2);
	}
	placeOverlay();
}

void EmuVideoLayer::draw(Gfx::RendererCommands &cmds)
{
	if(!texture)
		return;
	using namespace IG::Gfx;
	bool srgbOutput = srgbColorSpace();
	auto c = srgbOutput ? brightnessSrgb : brightness;
	cmds.setColor({c.r, c.g, c.b});
	cmds.set(BlendMode::OFF);
	if(effects.size())
	{
		cmds.setDither(false);
		TextureSpan srcTex = video.image();
		for(auto &ePtr : effects)
		{
			auto &e = *ePtr;
			cmds.setProgram(e.program());
			cmds.setRenderTarget(e.renderTarget());
			cmds.clear();
			e.drawRenderTarget(cmds, srcTex);
			srcTex = e.renderTarget();
		}
		cmds.setDefaultRenderTarget();
		cmds.setDither(true);
		cmds.restoreViewport();
	}
	if(srgbOutput)
		cmds.setSrgbFramebufferWrite(true);
	cmds.basicEffect().drawSprite(cmds, quad, 0, texture);
	video.addFence(cmds);
	vidImgOverlay.draw(cmds, c);
	if(srgbOutput)
		cmds.setSrgbFramebufferWrite(false);
}

void EmuVideoLayer::setRendererTask(Gfx::RendererTask &task)
{
	quad = {task, {.size = 1}};
}

void EmuVideoLayer::setFormat(EmuSystem &sys, IG::PixelFormat videoFmt, IG::PixelFormat effectFmt, Gfx::ColorSpace colorSpace)
{
	if(colSpace != colorSpace)
	{
		vidImgOverlay.setEffect(video.renderer(), {}, colSpace);
	}
	colSpace = colorSpace;
	if(EmuSystem::canRenderRGBA8888 && colorSpace == Gfx::ColorSpace::SRGB)
	{
		videoFmt = IG::PIXEL_RGBA8888;
	}
	if(!video.setRenderPixelFormat(sys, videoFmt, videoColorSpace(videoFmt)))
	{
		setEffectFormat(effectFmt);
		updateConvertColorSpaceEffect();
		updateSprite();
		setOverlay(userOverlayEffectId);
	}
}

void EmuVideoLayer::setOverlay(ImageOverlayId id)
{
	userOverlayEffectId = id;
	vidImgOverlay.setEffect(video.renderer(), id, colSpace);
	placeOverlay();
}

void EmuVideoLayer::setOverlayIntensity(float intensity)
{
	vidImgOverlay.setIntensity(intensity);
}

void EmuVideoLayer::placeOverlay()
{
	vidImgOverlay.place(contentRect(), video.size(), rotation);
}

void EmuVideoLayer::setEffectFormat(IG::PixelFormat fmt)
{
	userEffect.setFormat(renderer(), fmt, colorSpace(), samplerConfig());
}

void EmuVideoLayer::setEffect(EmuSystem &sys, ImageEffectId effect, IG::PixelFormat fmt)
{
	if(userEffectId == effect)
		return;
	userEffectId = effect;
	if(effect == ImageEffectId::DIRECT)
	{
		userEffect = {};
		buildEffectChain();
		logMsg("deleted user effect");
		video.setRenderPixelFormat(sys, video.renderPixelFormat(), videoColorSpace(video.renderPixelFormat()));
		updateConvertColorSpaceEffect();
	}
	else
	{
		userEffect = {renderer(), effect, fmt, colorSpace(), samplerConfig(), video.size()};
		buildEffectChain();
		video.setRenderPixelFormat(sys, video.renderPixelFormat(), Gfx::ColorSpace::LINEAR);
	}
}

void EmuVideoLayer::setLinearFilter(bool on)
{
	useLinearFilter = on;
	if(effects.size())
		effects.back()->setSampler(samplerConfig());
	else
		video.setSampler(samplerConfig());
}

void EmuVideoLayer::setBrightness(Gfx::Vec3 b)
{
	brightness = b;
	brightnessSrgb = glm::convertSRGBToLinear(b);
}

void EmuVideoLayer::onVideoFormatChanged(IG::PixelFormat effectFmt)
{
	setEffectFormat(effectFmt);
	if(!updateConvertColorSpaceEffect())
	{
		updateEffectImageSize();
	}
	updateSprite();
	setOverlay(userOverlayEffectId);
}

void EmuVideoLayer::setRotation(IG::Rotation r)
{
	rotation = r;
	quad.write(0, {.bounds = contentRect_.as<int16_t>(), .textureSpan = texture, .rotation = rotation});
	placeOverlay();
}

float EmuVideoLayer::evalAspectRatio(float aR)
{
	if(aR == -1)
	{
		return video.size().ratio<float>();
	}
	return aR;
}

Gfx::Renderer &EmuVideoLayer::renderer()
{
	return video.renderer();
}

void EmuVideoLayer::updateEffectImageSize()
{
	auto &r = renderer();
	for(auto &e : effects)
	{
		e->setImageSize(r, video.size(), e == effects.back() ? samplerConfig() : Gfx::SamplerConfigs::noLinearNoMipClamp);
	}
}

void EmuVideoLayer::buildEffectChain()
{
	effects.clear();
	if(userEffect)
	{
		effects.emplace_back(&userEffect);
	}
	updateEffectImageSize();
	updateSprite();
	logOutputFormat();
}

bool EmuVideoLayer::updateConvertColorSpaceEffect()
{
	bool needsConversion = video.colorSpace() == Gfx::ColorSpace::LINEAR
		&& colorSpace() == Gfx::ColorSpace::SRGB
		&& userEffectId == ImageEffectId::DIRECT;
	if(needsConversion && !userEffect)
	{
		userEffect = {renderer(), ImageEffectId::DIRECT, IG::PIXEL_RGBA8888, Gfx::ColorSpace::SRGB, samplerConfig(), video.size()};
		logMsg("made sRGB conversion effect");
		buildEffectChain();
		return true;
	}
	else if(!needsConversion && userEffect && userEffectId == ImageEffectId::DIRECT)
	{
		userEffect = {};
		logMsg("deleted sRGB conversion effect");
		buildEffectChain();
		return true;
	}
	return false;
}

void EmuVideoLayer::updateSprite()
{
	if(effects.size())
	{
		texture = effects.back()->renderTarget();
		video.setSampler(Gfx::SamplerConfigs::noLinearNoMipClamp);
	}
	else
	{
		texture = video.image();
		video.setSampler(samplerConfig());
	}
}

void EmuVideoLayer::logOutputFormat()
{
	if constexpr(Config::DEBUG_BUILD)
	{
		IG::StaticString<255> str{"output format: main video:"};
		str += video.image().pixmapDesc().format.name();
		for(auto &ePtr : effects)
		{
			auto &e = *ePtr;
			str += " -> effect:";
			str += e.imageFormat().name();
		}
		logMsg("%s", str.data());
	}
}

Gfx::ColorSpace EmuVideoLayer::videoColorSpace(IG::PixelFormat videoFmt) const
{
	// if we want sRGB output and are rendering directly, set the video to sRGB if possible
	return colorSpace() == Gfx::ColorSpace::SRGB && userEffectId == ImageEffectId::DIRECT ?
			Gfx::Renderer::supportedColorSpace(videoFmt, colorSpace()) : Gfx::ColorSpace::LINEAR;
}

Gfx::TextureSamplerConfig EmuVideoLayer::samplerConfig() const { return EmuVideo::samplerConfigForLinearFilter(useLinearFilter); }

}
