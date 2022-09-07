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
#include <imagine/logger/logger.h>
#include <algorithm>

namespace EmuEx
{

EmuVideoLayer::EmuVideoLayer(EmuVideo &video):
	video{video} {}

void EmuVideoLayer::place(IG::WindowRect viewRect, IG::WindowRect displayRect, Gfx::ProjectionPlane projP, EmuInputView *inputView, EmuSystem &sys)
{
	if(sys.hasContent())
	{
		Gfx::GCRect contentGCRect{};
		auto viewportAspectRatio = displayRect.xSize() / (float)displayRect.ySize();
		auto zoom = zoom_;
		auto contentSize = video.size();
		if(isSideways(rotation))
			std::swap(contentSize.x, contentSize.y);
		// compute the video rectangle in pixel coordinates
		if((zoom == optionImageZoomIntegerOnly || zoom == optionImageZoomIntegerOnlyY)
			&& contentSize.x)
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

			contentRect_.x = 0;
			contentRect_.y = 0;
			contentRect_.x2 = x * scaleFactor;
			contentRect_.y2 = y * scaleFactor;
			contentRect_.setPos({(int)displayRect.xCenter() - contentRect_.x2/2, (int)displayRect.yCenter() - contentRect_.y2/2});
		}

		// compute the video rectangle in world coordinates for sub-pixel placement
		if(zoom <= 100 || zoom == optionImageZoomIntegerOnlyY)
		{
			auto aR = aspectRatio() * sys.videoAspectRatioScale();
			if(isSideways(rotation))
				aR = 1. / aR;
			if(zoom == optionImageZoomIntegerOnlyY)
			{
				// get width from previously calculated pixel height
				float width = projP.unprojectYSize(contentRect_.ySize()) * (float)aR;
				if(!aR)
				{
					width = projP.width();
				}
				contentGCRect.x = -width / 2.f;
				contentGCRect.x2 = width / 2.f;
			}
			else
			{
				auto size = projP.size();
				if(aR)
				{
					size = IG::sizesWithRatioBestFit((float)aR, size.x, size.y);
				}
				contentGCRect.x = -size.x / 2.f;
				contentGCRect.x2 = size.x / 2.f;
				contentGCRect.y = -size.y / 2.f;
				contentGCRect.y2 = size.y / 2.f;
			}
		}

		// determine whether to generate the final coordinates from pixels or world units
		bool getXCoordinateFromPixels = 0, getYCoordinateFromPixels = 0;
		if(zoom == optionImageZoomIntegerOnlyY)
		{
			getYCoordinateFromPixels = 1;
		}
		else if(zoom == optionImageZoomIntegerOnly)
		{
			getXCoordinateFromPixels = getYCoordinateFromPixels = 1;
		}

		// apply sub-pixel zoom
		if(zoom < 100)
		{
			auto scaler = zoom / 100.f;
			contentGCRect.x *= scaler;
			contentGCRect.y *= scaler;
			contentGCRect.x2 *= scaler;
			contentGCRect.y2 *= scaler;
		}

		// adjust position
		int layoutDirection = 0;
		#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
		if(inputView && viewportAspectRatio < 1. && !isSideways(rotation) &&
			inputView->activeVController()->gamepadIsActive())
		{
			auto &vController = *inputView->activeVController();
			auto padding = vController.bounds(3).ySize(); // adding menu button-sized padding
			auto paddingG = projP.unProjectRect(vController.bounds(3)).ySize();
			auto viewBoundsG = projP.unProjectRect(viewRect);
			auto &layoutPos = vController.layoutPosition()[inputView->window().isPortrait() ? 1 : 0];
			if(layoutPos[VCTRL_LAYOUT_DPAD_IDX].origin.onTop() && layoutPos[VCTRL_LAYOUT_FACE_BTN_GAMEPAD_IDX].origin.onTop())
			{
				layoutDirection = -1;
				contentGCRect.setYPos(viewBoundsG.y + paddingG, CB2DO);
				contentRect_.setYPos(viewRect.y2 - padding, CB2DO);
			}
			else if(!(layoutPos[VCTRL_LAYOUT_DPAD_IDX].origin.onBottom() && layoutPos[VCTRL_LAYOUT_FACE_BTN_GAMEPAD_IDX].origin.onTop())
				&& !(layoutPos[VCTRL_LAYOUT_DPAD_IDX].origin.onTop() && layoutPos[VCTRL_LAYOUT_FACE_BTN_GAMEPAD_IDX].origin.onBottom()))
			{
				// move controls to top if d-pad & face button aren't on opposite Y quadrants
				layoutDirection = 1;
				contentGCRect.setYPos(viewBoundsG.y2 - paddingG, CT2DO);
				contentRect_.setYPos(viewRect.y + padding, CT2DO);
			}
		}
		#endif

		// assign final coordinates
		auto fromWorldSpaceRect = projP.projectRect(contentGCRect);
		auto fromPixelRect = projP.unProjectRect(contentRect_);
		if(getXCoordinateFromPixels)
		{
			contentGCRect.x = fromPixelRect.x;
			contentGCRect.x2 = fromPixelRect.x2;
		}
		else
		{
			contentRect_.x = fromWorldSpaceRect.x;
			contentRect_.x2 = fromWorldSpaceRect.x2;
		}
		if(getYCoordinateFromPixels)
		{
			contentGCRect.y = fromPixelRect.y;
			contentGCRect.y2 = fromPixelRect.y2;
		}
		else
		{
			contentRect_.y = fromWorldSpaceRect.y;
			contentRect_.y2 = fromWorldSpaceRect.y2;
		}

		disp.setPos(contentGCRect);
		auto layoutStr = layoutDirection == 1 ? "top" : layoutDirection == -1 ? "bottom" : "center";
		logMsg("placed game rect (%s), at pixels %d:%d:%d:%d, world %f:%f:%f:%f",
				layoutStr, contentRect_.x, contentRect_.y, contentRect_.x2, contentRect_.y2,
				(double)contentGCRect.x, (double)contentGCRect.y, (double)contentGCRect.x2, (double)contentGCRect.y2);
	}
	placeOverlay();
}

void EmuVideoLayer::draw(Gfx::RendererCommands &cmds, const Gfx::ProjectionPlane &projP)
{
	using namespace IG::Gfx;
	bool srgbOutput = srgbColorSpace();
	auto c = srgbOutput ? brightnessSrgb : brightness;
	cmds.setColor(c, c, c);
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
	disp.draw(cmds, cmds.basicEffect());
	video.addFence(cmds);
	vidImgOverlay.draw(cmds, c);
	if(srgbOutput)
		cmds.setSrgbFramebufferWrite(false);
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
	vidImgOverlay.place(disp, contentRect(), video.size(), rotation);
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

void EmuVideoLayer::setBrightness(float b)
{
	brightness = b;
	brightnessSrgb = std::pow(b, 2.2f);
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
	disp.setUVBounds({{0.f, 0.f}, {1.f, 1.f}}, r);
	placeOverlay();
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
		disp.set(effects.back()->renderTarget(), rotation);
		video.setSampler(Gfx::SamplerConfigs::noLinearNoMipClamp);
	}
	else
	{
		disp.set(video.image(), rotation);
		video.setSampler(samplerConfig());
	}
}

void EmuVideoLayer::logOutputFormat()
{
	if constexpr(Config::DEBUG_BUILD)
	{
		IG::StaticString<255> str{"output format: main video:"};
		str += video.image().pixmapDesc().format().name();
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
