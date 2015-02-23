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
#include <emuframework/EmuInput.hh>
#include <emuframework/VController.hh>
#include <emuframework/EmuApp.hh>
#include <algorithm>

extern bool touchControlsAreOn;
bool touchControlsApplicable();

void EmuVideoLayer::init()
{
	disp.init({});
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	vidImgEffect.setImageSize({video.vidPix.x, video.vidPix.y});
	#endif
}

void EmuVideoLayer::resetImage()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(vidImgEffect.renderTarget())
	{
		logMsg("drawing video via render target");
		disp.setImg(&vidImgEffect.renderTarget().texture());
	}
	else
	#endif
	{
		logMsg("drawing video texture directly");
		disp.setImg(video.vidImg);
	}
	compileDefaultPrograms();
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	vidImgEffect.setImageSize({video.vidPix.x, video.vidPix.y});
	#endif
	setLinearFilter(useLinearFilter);
}

void EmuVideoLayer::place(const IG::WindowRect &viewportRect, const Gfx::ProjectionPlane &projP, bool onScreenControlsOverlay)
{
	if(EmuSystem::gameIsRunning())
	{
		float viewportAspectRatio = viewportRect.xSize()/(float)viewportRect.ySize();
		// compute the video rectangle in pixel coordinates
		if((uint)optionImageZoom == optionImageZoomIntegerOnly || (uint)optionImageZoom == optionImageZoomIntegerOnlyY)
		{
			uint scaleFactor;
			uint gameX = video.vidPix.x, gameY = video.vidPix.y;

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
		if((uint)optionImageZoom <= 100 || (uint)optionImageZoom == optionImageZoomIntegerOnlyY)
		{
			auto aR = optionAspectRatio.val;

			if((uint)optionImageZoom == optionImageZoomIntegerOnlyY)
			{
				// get width from previously calculated pixel height
				Gfx::GC width = projP.unprojectYSize(gameRect_.ySize()) * aR.ratio<Gfx::GC>();
				if(!aR.x)
				{
					width = projP.w;
				}
				gameRectG.x = -width/2.;
				gameRectG.x2 = width/2.;
			}
			else
			{
				Gfx::GP size { projP.w, projP.h };
				if(aR.x)
				{
					size = IG::sizesWithRatioBestFit(aR.ratio<Gfx::GC>(), size.x, size.y);
				}
				gameRectG.x = -size.x/2.;
				gameRectG.x2 = size.x/2.;
				gameRectG.y = -size.y/2.;
				gameRectG.y2 = size.y/2.;
			}
		}

		// determine whether to generate the final coordinates from pixels or world units
		bool getXCoordinateFromPixels = 0, getYCoordinateFromPixels = 0;
		if((uint)optionImageZoom == optionImageZoomIntegerOnlyY)
		{
			getYCoordinateFromPixels = 1;
		}
		else if((uint)optionImageZoom == optionImageZoomIntegerOnly)
		{
			getXCoordinateFromPixels = getYCoordinateFromPixels = 1;
		}

		// adjust position
		Gfx::GC yOffset = 0;
		int yOffsetPixels = 0;
		#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
		if(onScreenControlsOverlay && viewportAspectRatio < 1. && touchControlsAreOn && touchControlsApplicable())
		{
			auto &layoutPos = vControllerLayoutPos[mainWin.viewport().isPortrait() ? 1 : 0];
			if(layoutPos[VCTRL_LAYOUT_DPAD_IDX].origin.onTop() && layoutPos[VCTRL_LAYOUT_FACE_BTN_GAMEPAD_IDX].origin.onTop())
			{
				logMsg("moving game rect to bottom");
				gameRectG.setYPos(projP.bounds().y, CB2DO);
				gameRect_.setYPos(viewportRect.y2, CB2DO);
			}
			else if(!(layoutPos[VCTRL_LAYOUT_DPAD_IDX].origin.onBottom() && layoutPos[VCTRL_LAYOUT_FACE_BTN_GAMEPAD_IDX].origin.onTop())
				&& !(layoutPos[VCTRL_LAYOUT_DPAD_IDX].origin.onTop() && layoutPos[VCTRL_LAYOUT_FACE_BTN_GAMEPAD_IDX].origin.onBottom()))
			{
				// move controls to top if d-pad & face button aren't on opposite Y quadrants
				logMsg("moving game rect to top");
				gameRectG.setYPos(projP.bounds().y2, CT2DO);
				gameRect_.setYPos(viewportRect.y, CT2DO);
			}

		}
		#endif

		// apply sub-pixel zoom
		if(optionImageZoom.val < 100)
		{
			auto scaler = (Gfx::GC(optionImageZoom.val) / 100.);
			gameRectG.x *= scaler;
			gameRectG.y *= scaler;
			gameRectG.x2 *= scaler;
			gameRectG.y2 *= scaler;
		}

		// apply y offset after zoom
		gameRectG += IG::Point2D<Gfx::GC>{0, yOffset};
		gameRect_ += IG::Point2D<int>{0, yOffsetPixels};

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
		logMsg("placed game rect at pixels %d:%d:%d:%d, world %f:%f:%f:%f",
				gameRect_.x, gameRect_.y, gameRect_.x2, gameRect_.y2,
				(double)gameRectG.x, (double)gameRectG.y, (double)gameRectG.x2, (double)gameRectG.y2);
	}
	placeOverlay();
	placeEffect();
}

void EmuVideoLayer::draw(const Gfx::ProjectionPlane &projP)
{
	using namespace Gfx;
	if(EmuSystem::isStarted())
	{
		bool videoActive = true;
		if(unlikely(!EmuSystem::isActive()))
		{
			setColor(.25, .25, .25);
			videoActive = false;
		}

		setBlendMode(0);
		#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
		if(vidImgEffect.program())
		{
			auto prevViewport = Gfx::viewport();
			setClipRect(false);
			setProgram(vidImgEffect.program());
			vidImgEffect.renderTarget().setCurrent();
			Gfx::clear();
			vidImgEffect.drawRenderTarget(video.vidImg);
			RenderTarget::setDefaultCurrent();
			Gfx::setViewport(prevViewport);
			disp.useDefaultProgram(videoActive ? IMG_MODE_REPLACE : IMG_MODE_MODULATE, projP.makeTranslate());
		}
		else
		#endif
		{
			disp.useDefaultProgram(videoActive ? IMG_MODE_REPLACE : IMG_MODE_MODULATE, projP.makeTranslate());
		}
		if(useLinearFilter)
			Gfx::TextureSampler::bindDefaultNoMipClampSampler();
		else
			Gfx::TextureSampler::bindDefaultNoLinearNoMipClampSampler();
		disp.draw();
		vidImgOverlay.draw();
	}
}

void EmuVideoLayer::placeOverlay()
{
	vidImgOverlay.place(disp, video.vidPix.y);
}

void EmuVideoLayer::placeEffect()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	vidImgEffect.setImageSize({video.vidPix.x, video.vidPix.y});
	#endif
}

void EmuVideoLayer::compileDefaultPrograms()
{
	auto compiled = disp.compileDefaultProgram(Gfx::IMG_MODE_REPLACE);
	compiled |= disp.compileDefaultProgram(Gfx::IMG_MODE_MODULATE);
	if(compiled)
		Gfx::autoReleaseShaderCompiler();
}

void EmuVideoLayer::setEffect(uint effect)
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	vidImgEffect.setEffect(effect, video.isExternalTexture());
	placeEffect();
	if(video.vidImg)
		resetImage();
	#endif
}

void EmuVideoLayer::setLinearFilter(bool on)
{
	useLinearFilter = on;
	if(useLinearFilter)
		Gfx::TextureSampler::initDefaultNoMipClampSampler();
	else
		Gfx::TextureSampler::initDefaultNoLinearNoMipClampSampler();
}
