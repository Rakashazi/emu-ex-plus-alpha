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

#include <emuframework/EmuVideoLayer.hh>
#include <emuframework/EmuInputView.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/VController.hh>
#include <emuframework/Option.hh>
#include <emuframework/EmuOptions.hh>
#include <imagine/util/math/Point2D.hh>
#include <imagine/util/format.hh>
#include <imagine/base/Window.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/Vec3.hh>
#include <imagine/gfx/Mat4.hh>
#include <imagine/glm/gtc/color_space.hpp>
#include <imagine/logger/logger.h>
#include <algorithm>

namespace EmuEx
{

constexpr SystemLogger log{"VideoLayer"};

EmuVideoLayer::EmuVideoLayer(EmuVideo &video, float defaultAspectRatio):
	video{video},
	landscapeAspectRatio{defaultAspectRatio},
	portraitAspectRatio{defaultAspectRatio} {}

void EmuVideoLayer::place(IG::WindowRect viewRect, IG::WindowRect displayRect, EmuInputView *inputView, EmuSystem &sys)
{
	if(sys.hasContent() && video.size().x)
	{
		auto viewportAspectRatio = displayRect.xSize() / (float)displayRect.ySize();
		auto contentSize = video.size();
		if(isSideways(rotation))
			std::swap(contentSize.x, contentSize.y);
		contentRect_ = {};
		if(scale == optionContentScaleIntegerOnly || scale == optionContentScaleIntegerOnlyY)
		{
			int x = contentSize.x, y = contentSize.y;

			// Halve pixel sizes if image has mixed low/high-res content so scaling is based on lower res,
			// this prevents jumping between two screen sizes in games like Seiken Densetsu 3 on SNES
			auto multiresVideoBaseSize = sys.multiresVideoBaseSize();
			if(multiresVideoBaseSize.x && x > multiresVideoBaseSize.x)
			{
				log.info("halving X size for multires content");
				x /= 2;
			}
			if(multiresVideoBaseSize.y && y > multiresVideoBaseSize.y)
			{
				log.info("halving Y size for multires content");
				y /= 2;
			}

			auto aR = x / float(y);

			// avoid overly wide images (SNES, etc.) or tall images (2600, etc.)
			if(aR >= 2.f)
			{
				log.info("unscaled image too wide, doubling height to compensate");
				y *= 2;
				aR = x / float(y);
			}
			else if(aR < 0.8f)
			{
				log.info("unscaled image too tall, doubling width to compensate");
				x *= 2;
				aR = x / float(y);
			}

			int scaleFactor;
			if(aR > viewportAspectRatio)
			{
				scaleFactor = std::max(1, displayRect.xSize() / x);
				log.info("using x scale factor:{}", scaleFactor);
			}
			else
			{
				scaleFactor = std::max(1, displayRect.ySize() / y);
				log.info("using y scale factor:{}", scaleFactor);
			}
			contentRect_.x2 = x * scaleFactor;
			contentRect_.y2 = y * scaleFactor;
		}
		if(scale <= 200 || scale == optionContentScaleIntegerOnlyY)
		{
			auto aR = evalAspectRatio(viewportAspectRatio < 1.f ? portraitAspectRatio : landscapeAspectRatio)
				* sys.videoAspectRatioScale();
			if(isSideways(rotation))
				aR = 1. / aR;
			if(scale == optionContentScaleIntegerOnlyY)
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
				if(scale != 100)
				{
					auto scaler = scale / 100.f;
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
		//log.info("placed game rect at pixels {}:{}:{}:{}", contentRect_.x, contentRect_.y, contentRect_.x2, contentRect_.y2);
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
	auto &basicEffect = cmds.basicEffect();
	if(video.isOddField) // shift image by half a line to reduce interlace flicker
	{
		float fieldOffset = (contentRect_.size().y / float(video.size().y)) / 2.f;
		basicEffect.setModelView(cmds, Mat4::makeTranslate({0, fieldOffset, 0}));
	}
	basicEffect.drawSprite(cmds, quad, 0, texture);
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
		videoFmt = IG::PixelFmtRGBA8888;
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
	updateEffect(sys, fmt);
}

void EmuVideoLayer::updateEffect(EmuSystem &sys, IG::PixelFormat fmt)
{
	if(userEffectId == ImageEffectId::DIRECT)
	{
		userEffect = {};
		buildEffectChain();
		log.info("deleted user effect");
		video.setRenderPixelFormat(sys, video.renderPixelFormat(), videoColorSpace(video.renderPixelFormat()));
		updateConvertColorSpaceEffect();
	}
	else
	{
		userEffect = {renderer(), userEffectId, fmt, colorSpace(), samplerConfig(), video.size()};
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

void EmuVideoLayer::updateBrightness()
{
	brightness = brightnessUnscaled * brightnessScale;
	brightnessSrgb = glm::convertSRGBToLinear(brightness);
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
		userEffect = {renderer(), ImageEffectId::DIRECT, IG::PixelFmtRGBA8888, Gfx::ColorSpace::SRGB, samplerConfig(), video.size()};
		log.info("made sRGB conversion effect");
		buildEffectChain();
		return true;
	}
	else if(!needsConversion && userEffect && userEffectId == ImageEffectId::DIRECT)
	{
		userEffect = {};
		log.info("deleted sRGB conversion effect");
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
		log.info("{}", str);
	}
}

Gfx::ColorSpace EmuVideoLayer::videoColorSpace(IG::PixelFormat videoFmt) const
{
	// if we want sRGB output and are rendering directly, set the video to sRGB if possible
	return colorSpace() == Gfx::ColorSpace::SRGB && userEffectId == ImageEffectId::DIRECT ?
			Gfx::Renderer::supportedColorSpace(videoFmt, colorSpace()) : Gfx::ColorSpace::LINEAR;
}

Gfx::TextureSamplerConfig EmuVideoLayer::samplerConfig() const { return EmuVideo::samplerConfigForLinearFilter(useLinearFilter); }

static auto &channelBrightnessVal(ImageChannel ch, auto &brightnessUnscaled)
{
	switch(ch)
	{
		case ImageChannel::All: break;
		case ImageChannel::Red: return brightnessUnscaled.r;
		case ImageChannel::Green: return brightnessUnscaled.g;
		case ImageChannel::Blue: return brightnessUnscaled.b;
	}
	bug_unreachable("invalid ImageChannel");
}

float EmuVideoLayer::channelBrightness(ImageChannel ch) const
{
	return channelBrightnessVal(ch, brightnessUnscaled);
}

void EmuVideoLayer::setBrightness(float brightness, ImageChannel ch)
{
	if(ch == ImageChannel::All)
	{
		brightnessUnscaled.r = brightnessUnscaled.g = brightnessUnscaled.b = brightness;
	}
	else
	{
		channelBrightnessVal(ch, brightnessUnscaled) = brightness;
	}
	updateBrightness();
}

bool EmuVideoLayer::readConfig(MapIO &io, unsigned key)
{
	switch(key)
	{
		default: return false;
		case CFGKEY_CONTENT_SCALE: return readOptionValue(io, scale);
		case CFGKEY_VIDEO_BRIGHTNESS: return readOptionValue(io, brightnessUnscaled);
		case CFGKEY_GAME_IMG_FILTER: return readOptionValue(io, useLinearFilter);
		case CFGKEY_IMAGE_EFFECT: return readOptionValue(io, userEffectId, [](auto m){return m <= lastEnum<ImageEffectId>;});
		case CFGKEY_OVERLAY_EFFECT: return readOptionValue(io, userOverlayEffectId, [](auto m){return m <= lastEnum<ImageOverlayId>;});
		case CFGKEY_OVERLAY_EFFECT_LEVEL: return readOptionValue<int8_t>(io, [&](auto i){if(i >= 0 && i <= 100) setOverlayIntensity(i / 100.f); });
	}
}

void EmuVideoLayer::writeConfig(FileIO &io) const
{
	writeOptionValueIfNotDefault(io, scale);
	if(brightnessUnscaled != Gfx::Vec3{1.f, 1.f, 1.f})
		writeOptionValue(io, CFGKEY_VIDEO_BRIGHTNESS, brightnessUnscaled);
	writeOptionValueIfNotDefault(io, CFGKEY_GAME_IMG_FILTER, useLinearFilter, true);
	writeOptionValueIfNotDefault(io, CFGKEY_IMAGE_EFFECT, userEffectId, ImageEffectId{});
	writeOptionValueIfNotDefault(io, CFGKEY_OVERLAY_EFFECT, userOverlayEffectId, ImageOverlayId{});
	writeOptionValueIfNotDefault(io, CFGKEY_OVERLAY_EFFECT_LEVEL, int8_t(overlayIntensity() * 100.f), 75);
}

}
