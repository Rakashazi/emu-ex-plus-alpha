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

void EmuVideoLayer::place(const IG::WindowRect &viewportRect, const Gfx::ProjectionPlane &projP, EmuInputView *inputView, EmuSystem &sys)
{
	if(sys.hasContent())
	{
		float viewportAspectRatio = viewportRect.xSize()/(float)viewportRect.ySize();
		auto zoom = zoom_;
		// compute the video rectangle in pixel coordinates
		if((zoom == optionImageZoomIntegerOnly || zoom == optionImageZoomIntegerOnlyY)
			&& video.size().x)
		{
			int gameX = video.size().x, gameY = video.size().y;

			// Halve pixel sizes if image has mixed low/high-res content so scaling is based on lower res,
			// this prevents jumping between two screen sizes in games like Seiken Densetsu 3 on SNES
			auto multiresVideoBaseSize = sys.multiresVideoBaseSize();
			if(multiresVideoBaseSize.x && gameX > multiresVideoBaseSize.x)
			{
				logMsg("halving X size for multires content");
				gameX /= 2;
			}
			if(multiresVideoBaseSize.y && gameY > multiresVideoBaseSize.y)
			{
				logMsg("halving Y size for multires content");
				gameY /= 2;
			}

			auto gameAR = float(gameX) / float(gameY);

			// avoid overly wide images (SNES, etc.) or tall images (2600, etc.)
			if(gameAR >= 2)
			{
				logMsg("unscaled image too wide, doubling height to compensate");
				gameY *= 2;
				gameAR = float(gameX) / float(gameY);
			}
			else if(gameAR < 0.8)
			{
				logMsg("unscaled image too tall, doubling width to compensate");
				gameX *= 2;
				gameAR = float(gameX) / float(gameY);
			}

			int scaleFactor;
			if(gameAR > viewportAspectRatio)//Gfx::proj.aspectRatio)
			{
				scaleFactor = std::max(1, viewportRect.xSize() / gameX);
				logMsg("using x scale factor %d", scaleFactor);
			}
			else
			{
				scaleFactor = std::max(1, viewportRect.ySize() / gameY);
				logMsg("using y scale factor %d", scaleFactor);
			}

			gameRect_.x = 0;
			gameRect_.y = 0;
			gameRect_.x2 = gameX * scaleFactor;
			gameRect_.y2 = gameY * scaleFactor;
			gameRect_.setPos({(int)viewportRect.xCenter() - gameRect_.x2/2, (int)viewportRect.yCenter() - gameRect_.y2/2});
		}

		// compute the video rectangle in world coordinates for sub-pixel placement
		if(zoom <= 100 || zoom == optionImageZoomIntegerOnlyY)
		{
			auto aR = aspectRatio();
			if(sys.videoAspectRatioScale())
			{
				aR *= sys.videoAspectRatioScale();
			}
			if(zoom == optionImageZoomIntegerOnlyY)
			{
				// get width from previously calculated pixel height
				float width = projP.unprojectYSize(gameRect_.ySize()) * (float)aR;
				if(!aR)
				{
					width = projP.width();
				}
				gameRectG.x = -width/2.;
				gameRectG.x2 = width/2.;
			}
			else
			{
				Gfx::GP size = projP.size();
				if(aR)
				{
					size = IG::sizesWithRatioBestFit((float)aR, size.x, size.y);
				}
				gameRectG.x = -size.x/2.;
				gameRectG.x2 = size.x/2.;
				gameRectG.y = -size.y/2.;
				gameRectG.y2 = size.y/2.;
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
			gameRectG.x *= scaler;
			gameRectG.y *= scaler;
			gameRectG.x2 *= scaler;
			gameRectG.y2 *= scaler;
		}

		// adjust position
		int layoutDirection = 0;
		#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
		if(inputView && viewportAspectRatio < 1. && inputView->activeVController()->gamepadIsActive())
		{
			auto &vController = *inputView->activeVController();
			auto padding = vController.bounds(3).ySize(); // adding menu button-sized padding
			auto paddingG = projP.unProjectRect(vController.bounds(3)).ySize();
			auto &layoutPos = vController.layoutPosition()[inputView->window().isPortrait() ? 1 : 0];
			if(layoutPos[VCTRL_LAYOUT_DPAD_IDX].origin.onTop() && layoutPos[VCTRL_LAYOUT_FACE_BTN_GAMEPAD_IDX].origin.onTop())
			{
				layoutDirection = -1;
				gameRectG.setYPos(projP.bounds().y + paddingG, CB2DO);
				gameRect_.setYPos(viewportRect.y2 - padding, CB2DO);
			}
			else if(!(layoutPos[VCTRL_LAYOUT_DPAD_IDX].origin.onBottom() && layoutPos[VCTRL_LAYOUT_FACE_BTN_GAMEPAD_IDX].origin.onTop())
				&& !(layoutPos[VCTRL_LAYOUT_DPAD_IDX].origin.onTop() && layoutPos[VCTRL_LAYOUT_FACE_BTN_GAMEPAD_IDX].origin.onBottom()))
			{
				// move controls to top if d-pad & face button aren't on opposite Y quadrants
				layoutDirection = 1;
				gameRectG.setYPos(projP.bounds().y2 - paddingG, CT2DO);
				gameRect_.setYPos(viewportRect.y + padding, CT2DO);
			}
		}
		#endif

		// assign final coordinates
		auto fromWorldSpaceRect = projP.projectRect(gameRectG);
		auto fromPixelRect = projP.unProjectRect(gameRect_);
		if(getXCoordinateFromPixels)
		{
			gameRectG.x = fromPixelRect.x;
			gameRectG.x2 = fromPixelRect.x2;
		}
		else
		{
			gameRect_.x = fromWorldSpaceRect.x;
			gameRect_.x2 = fromWorldSpaceRect.x2;
		}
		if(getYCoordinateFromPixels)
		{
			gameRectG.y = fromPixelRect.y;
			gameRectG.y2 = fromPixelRect.y2;
		}
		else
		{
			gameRect_.y = fromWorldSpaceRect.y;
			gameRect_.y2 = fromWorldSpaceRect.y2;
		}

		disp.setPos(gameRectG);
		auto layoutStr = layoutDirection == 1 ? "top" : layoutDirection == -1 ? "bottom" : "center";
		logMsg("placed game rect (%s), at pixels %d:%d:%d:%d, world %f:%f:%f:%f",
				layoutStr, gameRect_.x, gameRect_.y, gameRect_.x2, gameRect_.y2,
				(double)gameRectG.x, (double)gameRectG.y, (double)gameRectG.x2, (double)gameRectG.y2);
	}
	placeOverlay();
}

void EmuVideoLayer::draw(Gfx::RendererCommands &cmds, const Gfx::ProjectionPlane &projP)
{
	using namespace IG::Gfx;
	bool replaceMode = true;
	if(brightness != 1.f)
	{
		auto c = srgbColorSpace() ? brightnessSrgb : brightness;
		cmds.setColor(c, c, c);
		replaceMode = false;
	}
	cmds.setBlendMode(0);
	if(effects.size())
	{
		auto prevViewport = cmds.viewport();
		cmds.setClipTest(false);
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
		cmds.setViewport(prevViewport);
	}
	disp.setCommonProgram(cmds, replaceMode ? IMG_MODE_REPLACE : IMG_MODE_MODULATE, projP.makeTranslate());
	bool srgbFrameBufferWrite = srgbColorSpace();
	cmds.setTextureSampler(*texSampler);
	if(srgbFrameBufferWrite)
		cmds.setSrgbFramebufferWrite(true);
	disp.draw(cmds);
	if(srgbFrameBufferWrite)
		cmds.setSrgbFramebufferWrite(false);
	video.addFence(cmds);
	vidImgOverlay.draw(cmds);
}

void EmuVideoLayer::setFormat(EmuSystem &sys, IG::PixelFormat videoFmt, IG::PixelFormat effectFmt, Gfx::ColorSpace colorSpace)
{
	colSpace = colorSpace;
	if(EmuSystem::canRenderRGBA8888 && colorSpace == Gfx::ColorSpace::SRGB)
	{
		videoFmt = IG::PIXEL_RGBA8888;
	}
	if(!video.setRenderPixelFormat(sys, videoFmt, videoColorSpace(videoFmt)))
	{
		setEffectFormat(effectFmt);
		updateConvertColorSpaceEffect();
	}
}

void EmuVideoLayer::setOverlay(int effect)
{
	userOverlayEffectId = effect;
	vidImgOverlay.setEffect(video.renderer(), effect);
	placeOverlay();
}

void EmuVideoLayer::setOverlayIntensity(float intensity)
{
	vidImgOverlay.setIntensity(intensity);
}

void EmuVideoLayer::placeOverlay()
{
	vidImgOverlay.place(disp, video.size().y);
}

void EmuVideoLayer::setEffectFormat(IG::PixelFormat fmt)
{
	userEffect.setFormat(renderer(), fmt, colorSpace(), *texSampler);
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
		userEffect = {renderer(), effect, fmt, colorSpace(), *texSampler, video.size()};
		buildEffectChain();
		video.setRenderPixelFormat(sys, video.renderPixelFormat(), Gfx::ColorSpace::LINEAR);
	}
}

void EmuVideoLayer::setLinearFilter(bool on)
{
	texSampler = &renderer().make(on ? Gfx::CommonTextureSampler::NO_MIP_CLAMP : Gfx::CommonTextureSampler::NO_LINEAR_NO_MIP_CLAMP);
	if(effects.size())
		effects.back()->setCompatTextureSampler(*texSampler);
	else
		video.setCompatTextureSampler(*texSampler);
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
		updateSprite();
	}
	setOverlay(userOverlayEffectId);
}

Gfx::Renderer &EmuVideoLayer::renderer()
{
	return video.renderer();
}

void EmuVideoLayer::updateEffectImageSize()
{
	auto &r = renderer();
	auto &noLinearSampler = r.make(Gfx::CommonTextureSampler::NO_LINEAR_NO_MIP_CLAMP);
	for(auto &e : effects)
	{
		e->setImageSize(r, video.size(), e == effects.back() ? *texSampler : noLinearSampler);
	}
}

void EmuVideoLayer::buildEffectChain()
{
	auto &r = renderer();
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
		userEffect = {renderer(), ImageEffectId::DIRECT, IG::PIXEL_RGBA8888, Gfx::ColorSpace::SRGB, *texSampler, video.size()};
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
		disp.setImg(effects.back()->renderTarget());
		video.setCompatTextureSampler(renderer().make(Gfx::CommonTextureSampler::NO_LINEAR_NO_MIP_CLAMP));
	}
	else
	{
		disp.setImg(video.image());
		video.setCompatTextureSampler(*texSampler);
	}
	disp.compileDefaultProgramOneShot(Gfx::IMG_MODE_REPLACE);
	disp.compileDefaultProgramOneShot(Gfx::IMG_MODE_MODULATE);
}

void EmuVideoLayer::logOutputFormat()
{
	if constexpr(Config::DEBUG_BUILD)
	{
		IG::StaticString<255> str{"output format: main video:"};
		str += video.image().usedPixmapDesc().format().name();
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

}
