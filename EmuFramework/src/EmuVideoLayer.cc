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
#include <emuframework/VController.hh>
#include "EmuOptions.hh"
#include <imagine/util/math/Point2D.hh>
#include <imagine/base/Window.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/logger/logger.h>
#include <algorithm>

EmuVideoLayer::EmuVideoLayer(EmuVideo &video):
	video{video}
{}

void EmuVideoLayer::resetImage()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(vidImgEffect.renderTarget())
	{
		logMsg("drawing video via render target");
		disp.setImg(&vidImgEffect.renderTarget());
		vidImgEffect.setImageSize(video.renderer(), video.size(), *texSampler);
		video.setCompatTextureSampler(video.renderer().make(Gfx::CommonTextureSampler::NO_LINEAR_NO_MIP_CLAMP));
	}
	else
	#endif
	{
		logMsg("drawing video texture directly");
		disp.setImg(video.image());
		video.setCompatTextureSampler(*texSampler);
	}
	compileDefaultPrograms();
}

void EmuVideoLayer::place(const IG::WindowRect &viewportRect, const Gfx::ProjectionPlane &projP, EmuInputView *inputView)
{
	if(EmuSystem::gameIsRunning())
	{
		float viewportAspectRatio = viewportRect.xSize()/(float)viewportRect.ySize();
		// compute the video rectangle in pixel coordinates
		if(((unsigned)optionImageZoom == optionImageZoomIntegerOnly || (unsigned)optionImageZoom == optionImageZoomIntegerOnlyY)
			&& video.size().x)
		{
			unsigned gameX = video.size().x, gameY = video.size().y;

			// Halve pixel sizes if image has mixed low/high-res content so scaling is based on lower res,
			// this prevents jumping between two screen sizes in games like Seiken Densetsu 3 on SNES
			if(EmuSystem::multiresVideoBaseX() && gameX > EmuSystem::multiresVideoBaseX())
			{
				logMsg("halving X size for multires content");
				gameX /= 2;
			}
			if(EmuSystem::multiresVideoBaseY() && gameY > EmuSystem::multiresVideoBaseY())
			{
				logMsg("halving Y size for multires content");
				gameY /= 2;
			}

			auto gameAR = Gfx::GC(gameX) / Gfx::GC(gameY);

			// avoid overly wide images (SNES, etc.) or tall images (2600, etc.)
			if(gameAR >= 2)
			{
				logMsg("unscaled image too wide, doubling height to compensate");
				gameY *= 2;
				gameAR = Gfx::GC(gameX) / Gfx::GC(gameY);
			}
			else if(gameAR < 0.8)
			{
				logMsg("unscaled image too tall, doubling width to compensate");
				gameX *= 2;
				gameAR = Gfx::GC(gameX) / Gfx::GC(gameY);
			}

			unsigned scaleFactor;
			if(gameAR > viewportAspectRatio)//Gfx::proj.aspectRatio)
			{
				scaleFactor = std::max(1U, viewportRect.xSize() / gameX);
				logMsg("using x scale factor %d", scaleFactor);
			}
			else
			{
				scaleFactor = std::max(1U, viewportRect.ySize() / gameY);
				logMsg("using y scale factor %d", scaleFactor);
			}

			gameRect_.x = 0;
			gameRect_.y = 0;
			gameRect_.x2 = gameX * scaleFactor;
			gameRect_.y2 = gameY * scaleFactor;
			gameRect_.setPos({(int)viewportRect.xCenter() - gameRect_.x2/2, (int)viewportRect.yCenter() - gameRect_.y2/2});
		}

		// compute the video rectangle in world coordinates for sub-pixel placement
		if((unsigned)optionImageZoom <= 100 || (unsigned)optionImageZoom == optionImageZoomIntegerOnlyY)
		{
			auto aR = optionAspectRatio.val;

			if((unsigned)optionImageZoom == optionImageZoomIntegerOnlyY)
			{
				// get width from previously calculated pixel height
				Gfx::GC width = projP.unprojectYSize(gameRect_.ySize()) * (Gfx::GC)aR;
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
					size = IG::sizesWithRatioBestFit((Gfx::GC)aR, size.x, size.y);
				}
				gameRectG.x = -size.x/2.;
				gameRectG.x2 = size.x/2.;
				gameRectG.y = -size.y/2.;
				gameRectG.y2 = size.y/2.;
			}
		}

		// determine whether to generate the final coordinates from pixels or world units
		bool getXCoordinateFromPixels = 0, getYCoordinateFromPixels = 0;
		if((unsigned)optionImageZoom == optionImageZoomIntegerOnlyY)
		{
			getYCoordinateFromPixels = 1;
		}
		else if((unsigned)optionImageZoom == optionImageZoomIntegerOnly)
		{
			getXCoordinateFromPixels = getYCoordinateFromPixels = 1;
		}

		// apply sub-pixel zoom
		if(optionImageZoom.val < 100)
		{
			auto scaler = (Gfx::GC(optionImageZoom.val) / 100.);
			gameRectG.x *= scaler;
			gameRectG.y *= scaler;
			gameRectG.x2 *= scaler;
			gameRectG.y2 *= scaler;
		}

		// adjust position
		int layoutDirection = 0;
		#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
		if(inputView && viewportAspectRatio < 1. && inputView->touchControlsAreOn() && EmuSystem::touchControlsApplicable())
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
	placeEffect();
}

void EmuVideoLayer::draw(Gfx::RendererCommands &cmds, const Gfx::ProjectionPlane &projP)
{
	if(!EmuSystem::isStarted())
		return;
	using namespace Gfx;
	bool replaceMode = true;
	if(brightness != 1.f)
	{
		auto c = video.isSrgbFormat() ? brightnessSrgb : brightness;
		cmds.setColor(c, c, c);
		replaceMode = false;
	}
	cmds.setBlendMode(0);
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(vidImgEffect.program())
	{
		auto prevViewport = cmds.viewport();
		cmds.setClipTest(false);
		cmds.setProgram(vidImgEffect.program());
		cmds.setRenderTarget(vidImgEffect.renderTarget());
		cmds.setDither(false);
		cmds.clear();
		vidImgEffect.drawRenderTarget(cmds, video.image());
		cmds.setDefaultRenderTarget();
		cmds.setDither(true);
		cmds.setViewport(prevViewport);
		disp.setCommonProgram(cmds, replaceMode ? IMG_MODE_REPLACE : IMG_MODE_MODULATE, projP.makeTranslate());
	}
	else
	#endif
	{
		disp.setCommonProgram(cmds, replaceMode ? IMG_MODE_REPLACE : IMG_MODE_MODULATE, projP.makeTranslate());
	}
	bool srgbFrameBufferWrite = video.isSrgbFormat();
	cmds.setTextureSampler(*texSampler);
	if(srgbFrameBufferWrite)
		cmds.setSrgbFramebufferWrite(true);
	disp.draw(cmds);
	if(srgbFrameBufferWrite)
		cmds.setSrgbFramebufferWrite(false);
	video.addFence(cmds);
	vidImgOverlay.draw(cmds);
}

void EmuVideoLayer::setOverlay(unsigned effect)
{
	vidImgOverlay.setEffect(video.renderer(), effect);
	placeOverlay();
}

void EmuVideoLayer::setOverlayIntensity(Gfx::GC intensity)
{
	vidImgOverlay.setIntensity(intensity);
}

void EmuVideoLayer::placeOverlay()
{
	vidImgOverlay.place(disp, video.size().y);
}

static unsigned effectFormatToBits(IG::PixelFormatID format, EmuVideo &video)
{
	if(format == IG::PIXEL_NONE)
	{
		format = video.image().pixmapDesc().format().id();
	}
	return format == IG::PIXEL_RGB565 ? 16 : 32;
}

void EmuVideoLayer::setEffectFormat(IG::PixelFormatID fmt)
{
	vidImgEffect.setBitDepth(video.renderer(), effectFormatToBits(fmt, video), *texSampler);
}

void EmuVideoLayer::placeEffect()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	vidImgEffect.setImageSize(video.renderer(), video.size(), *texSampler);
	#endif
}

void EmuVideoLayer::compileDefaultPrograms()
{
	disp.compileDefaultProgramOneShot(Gfx::IMG_MODE_REPLACE);
	disp.compileDefaultProgramOneShot(Gfx::IMG_MODE_MODULATE);
}

void EmuVideoLayer::setEffect(unsigned effect, IG::PixelFormatID fmt)
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	assert(video.image());
	vidImgEffect.setEffect(video.renderer(), effect, effectFormatToBits(fmt, video), video.isExternalTexture(), *texSampler);
	placeEffect();
	resetImage();
	#endif
}

void EmuVideoLayer::setLinearFilter(bool on)
{
	texSampler = &video.renderer().make(on ? Gfx::CommonTextureSampler::NO_MIP_CLAMP : Gfx::CommonTextureSampler::NO_LINEAR_NO_MIP_CLAMP);
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(vidImgEffect.renderTarget())
	{
		vidImgEffect.setCompatTextureSampler(*texSampler);
	}
	else
	#endif
	{
		video.setCompatTextureSampler(*texSampler);
	}
}

void EmuVideoLayer::setSrgbColorSpaceOutput(bool on)
{
	video.setSrgbColorSpaceOutput(on);
	video.resetImage();
}

void EmuVideoLayer::setBrightness(float b)
{
	brightness = b;
	brightnessSrgb = std::pow(b, 2.2f);
}

void EmuVideoLayer::setTextureBufferMode(Gfx::TextureBufferMode mode)
{
	if(video.setTextureBufferMode(mode))
	{
		// texture may switch to external format so
		// force effect shaders to re-compile
		auto params = vidImgEffect.effectParams();
		setEffect(0, IG::PIXEL_NONE);
		video.resetImage();
		#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
		setEffect(params.effectID, params.formatID);
		#endif
	}
}

void EmuVideoLayer::setImageBuffers(unsigned num)
{
	if(video.setImageBuffers(num))
	{
		video.resetImage();
	}
}

unsigned EmuVideoLayer::imageBuffers() const
{
	return video.imageBuffers();
}
