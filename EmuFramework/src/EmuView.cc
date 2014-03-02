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

#include <EmuView.hh>
#include <EmuInput.hh>
#include <VController.hh>
#include <EmuApp.hh>
#include <gui/AlertView.hh>
#include <FilePicker.hh>
#include <Screenshot.hh>
#include <algorithm>

extern bool touchControlsAreOn;
bool touchControlsApplicable();

void EmuView::placeEmu()
{
	if(EmuSystem::gameIsRunning())
	{
		const auto &viewportRect = Gfx::viewport().bounds();
		// compute the video rectangle in pixel coordinates
		if((uint)optionImageZoom == optionImageZoomIntegerOnly || (uint)optionImageZoom == optionImageZoomIntegerOnlyY)
		{
			uint scaleFactor;
			uint gameX = vidPix.x, gameY = vidPix.y;

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

			if(gameAR > Gfx::viewport().aspectRatio())//Gfx::proj.aspectRatio)
			{
				scaleFactor = std::max(1U, Gfx::viewport().width() / gameX);
				logMsg("using x scale factor %d", scaleFactor);
			}
			else
			{
				scaleFactor = std::max(1U, Gfx::viewport().height() / gameY);
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
		if(/*Gfx::proj.aspectRatio*/Gfx::viewport().aspectRatio() < 1. && touchControlsAreOn && touchControlsApplicable())
		{
			auto &layoutPos = vControllerLayoutPos[Gfx::viewport().isPortrait() ? 1 : 0];
			if(layoutPos[VCTRL_LAYOUT_DPAD_IDX].origin.onBottom() && layoutPos[VCTRL_LAYOUT_FACE_BTN_GAMEPAD_IDX].origin.onBottom())
			{
				logMsg("moving game rect to top");
				gameRectG.setYPos(projP.bounds().y2, CT2DO);
				gameRect_.setYPos(viewportRect.y, CT2DO);
			}
			else if(layoutPos[VCTRL_LAYOUT_DPAD_IDX].origin.onTop() && layoutPos[VCTRL_LAYOUT_FACE_BTN_GAMEPAD_IDX].origin.onTop())
			{
				logMsg("moving game rect to bottom");
				gameRectG.setYPos(projP.bounds().y, CB2DO);
				gameRect_.setYPos(viewportRect.y2, CB2DO);
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
		#if defined CONFIG_BASE_ANDROID && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE
		disp.screenX = gameView.xIPos(LB2DO);
		disp.screenY = Gfx::viewPixelHeight() - gameView.yIPos(LB2DO);
		disp.screenX2 = gameView.iXSize;
		disp.screenY2 = gameView.iYSize;
		#endif
		logMsg("placed game rect at pixels %d:%d:%d:%d, world %f:%f:%f:%f",
				gameRect_.x, gameRect_.y, gameRect_.x2, gameRect_.y2,
				(double)gameRectG.x, (double)gameRectG.y, (double)gameRectG.x2, (double)gameRectG.y2);
	}
	placeOverlay();
	placeEffect();
}

template <bool active>
void EmuView::drawContent()
{
	using namespace Gfx;
	setBlendMode(0);
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(vidImgEffect.hasProgram())
	{
		setProgram(vidImgEffect.program(active ? IMG_MODE_REPLACE : IMG_MODE_MODULATE), projP.makeTranslate());
	}
	else
	#endif
	{
		disp.useDefaultProgram(active ? IMG_MODE_REPLACE : IMG_MODE_MODULATE, projP.makeTranslate());
	}
	disp.draw();
	vidImgOverlay.draw();
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	/*if(active && ((touchControlsAreOn && touchControlsApplicable())
		#ifdef CONFIG_VCONTROLLER_KEYBOARD
		|| vController.kbMode
		#endif
	))*/
	if(active)
	{
		vController.draw(touchControlsAreOn && touchControlsApplicable(), ffGuiKeyPush || ffGuiTouch);
	}
	#endif
	popup.draw();
}

template void EmuView::drawContent<0>();
template void EmuView::drawContent<1>();

void EmuView::draw(Base::FrameTimeBase frameTime)
{
	using namespace Gfx;
	if(likely(EmuSystem::isActive()))
	{
		postDraw();
		#ifdef CONFIG_BASE_MULTI_WINDOW
		extern Base::Window secondWin;
		if(secondWin)
			secondWin.postDraw();
		#endif
		runFrame(frameTime);
	}
	else if(EmuSystem::isStarted())
	{
		setColor(.25, .25, .25);
		drawContent<0>();
	}
}

void EmuView::runFrame(Base::FrameTimeBase frameTime)
{
	commonUpdateInput();
	bool renderAudio = optionSound;

	if(unlikely(ffGuiKeyPush || ffGuiTouch))
	{
		iterateTimes((uint)optionFastForwardSpeed, i)
		{
			EmuSystem::runFrame(0, 0, 0);
		}
	}
	else
	{
		int framesToSkip = EmuSystem::setupFrameSkip(optionFrameSkip, frameTime);
		if(framesToSkip > 0)
		{
			iterateTimes(framesToSkip, i)
			{
				EmuSystem::runFrame(0, 0, renderAudio);
			}
		}
		else if(framesToSkip == -1)
		{
			drawContent<1>();
			return;
		}
	}

	EmuSystem::runFrame(1, 1, renderAudio);
}

void EmuView::place()
{
	placeEmu();
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	EmuControls::setupVControllerVars();
	vController.place();
	#endif
}

void EmuView::takeGameScreenshot()
{
	FsSys::cPath path;
	int screenshotNum = sprintScreenshotFilename(path);
	if(screenshotNum == -1)
	{
		popup.postError("Too many screenshots");
	}
	else
	{
		if(!writeScreenshot(vidPix, path))
		{
			popup.printf(2, 1, "Error writing screenshot #%d", screenshotNum);
		}
		else
		{
			popup.printf(2, 0, "Wrote screenshot #%d", screenshotNum);
		}
	}
}

void EmuView::inputEvent(const Input::Event &e)
{
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	if(e.isPointer())
	{
		auto &layoutPos = vControllerLayoutPos[Gfx::viewport().isPortrait() ? 1 : 0];
		if(e.state == Input::PUSHED && layoutPos[VCTRL_LAYOUT_MENU_IDX].state != 0 && vController.menuBound.overlaps({e.x, e.y}))
		{
			viewStack.top().clearSelection();
			restoreMenuFromGame();
			return;
		}
		else if(e.state == Input::PUSHED && layoutPos[VCTRL_LAYOUT_FF_IDX].state != 0 && vController.ffBound.overlaps({e.x, e.y}))
		{
			toggle(ffGuiTouch);
		}
		else if((touchControlsAreOn && touchControlsApplicable())
			#ifdef CONFIG_VCONTROLLER_KEYBOARD
			|| vController.kbMode
			#endif
			)
		{
			vController.applyInput(e);
		}
		#ifdef CONFIG_VCONTROLS_GAMEPAD
		else if(!touchControlsAreOn && (uint)optionTouchCtrl == 2 && optionTouchCtrlShowOnTouch
			#ifdef CONFIG_VCONTROLLER_KEYBOARD
			&& !vController.kbMode
			#endif
			&& e.isTouch() && e.state == Input::PUSHED
			)
		{
			logMsg("turning on on-screen controls from touch input");
			touchControlsAreOn = 1;
			placeEmu();
		}
		#endif
	}
	else
	#elif defined INPUT_SUPPORTS_POINTER
	if(e.isPointer())
	{
		if(e.state == Input::PUSHED)
		{
			viewStack.top()->clearSelection();
			restoreMenuFromGame();
			return;
		}
	}
	else
	#endif
	#ifdef INPUT_SUPPORTS_RELATIVE_POINTER
	if(e.isRelativePointer())
	{
		processRelPtr(e);
	}
	else
	#endif
	{
		#if defined CONFIG_ENV_WEBOS && CONFIG_ENV_WEBOS_OS <= 2
		if(e.state == Input::PUSHED && e.button == Input::Keycode::ESCAPE)
		{
			restoreMenuFromGame();
			return;
		}
		#endif
		assert(e.device);
		const KeyMapping::ActionGroup &actionMap = keyMapping.inputDevActionTablePtr[e.device->idx][e.button];
		//logMsg("player %d input %s", player, Input::buttonName(e.map, e.button));
		iterateTimes(KeyMapping::maxKeyActions, i)
		{
			auto action = actionMap[i];
			if(action != 0)
			{
				using namespace EmuControls;
				action--;

				switch(action)
				{
					bcase guiKeyIdxFastForward:
					{
						ffGuiKeyPush = e.state == Input::PUSHED;
						logMsg("fast-forward key state: %d", ffGuiKeyPush);
					}

					bcase guiKeyIdxLoadGame:
					if(e.state == Input::PUSHED)
					{
						logMsg("open load game menu from key event");
						restoreMenuFromGame();
						viewStack.popToRoot();
						auto &fPicker = *menuAllocator.allocNew<EmuFilePicker>(window());
						fPicker.init(Input::keyInputIsPresent(), false);
						viewStack.useNavView = 0;
						viewStack.pushAndShow(fPicker, &menuAllocator);
						return;
					}

					bcase guiKeyIdxMenu:
					if(e.state == Input::PUSHED)
					{
						logMsg("open menu from key event");
						restoreMenuFromGame();
						return;
					}

					bcase guiKeyIdxSaveState:
					if(e.state == Input::PUSHED)
					{
						static auto doSaveState =
							[]()
							{
								int ret = EmuSystem::saveState();
								if(ret != STATE_RESULT_OK)
									popup.postError(stateResultToStr(ret));
								else
									popup.post("State Saved");
							};

						if(EmuSystem::shouldOverwriteExistingState())
						{
							doSaveState();
						}
						else
						{
							auto &ynAlertView = *allocModalView<YesNoAlertView>(window());
							ynAlertView.init("Really Overwrite State?", !e.isPointer());
							ynAlertView.onYes() =
								[](const Input::Event &e)
								{
									doSaveState();
									startGameFromMenu();
								};
							ynAlertView.onNo() =
								[](const Input::Event &e)
								{
									startGameFromMenu();
								};
							modalViewController.pushAndShow(ynAlertView);
							restoreMenuFromGame();
						}
						return;
					}

					bcase guiKeyIdxLoadState:
					if(e.state == Input::PUSHED)
					{
						int ret = EmuSystem::loadState();
						if(ret != STATE_RESULT_OK && ret != STATE_RESULT_OTHER_ERROR)
						{
							popup.postError(stateResultToStr(ret));
						}
						return;
					}

					bcase guiKeyIdxDecStateSlot:
					if(e.state == Input::PUSHED)
					{
						EmuSystem::saveStateSlot--;
						if(EmuSystem::saveStateSlot < -1)
							EmuSystem::saveStateSlot = 9;
						popup.printf(1, 0, "State Slot: %s", stateNameStr(EmuSystem::saveStateSlot));
					}

					bcase guiKeyIdxIncStateSlot:
					if(e.state == Input::PUSHED)
					{
						auto prevSlot = EmuSystem::saveStateSlot;
						EmuSystem::saveStateSlot++;
						if(EmuSystem::saveStateSlot > 9)
							EmuSystem::saveStateSlot = -1;
						popup.printf(1, 0, "State Slot: %s", stateNameStr(EmuSystem::saveStateSlot));
					}

					bcase guiKeyIdxGameScreenshot:
					if(e.state == Input::PUSHED)
					{
						takeGameScreenshot();
						return;
					}

					bcase guiKeyIdxExit:
					if(e.state == Input::PUSHED)
					{
						logMsg("request exit from key event");
						auto &ynAlertView = *allocModalView<YesNoAlertView>(window());
						ynAlertView.init("Really Exit?", Input::keyInputIsPresent());
						ynAlertView.onYes() =
							[](const Input::Event &e)
							{
								Base::exit();
							};
						modalViewController.pushAndShow(ynAlertView);
						restoreMenuFromGame();
						return;
					}

					bdefault:
					{
						//logMsg("action %d, %d", emuKey, state);
						bool turbo;
						uint sysAction = EmuSystem::translateInputAction(action, turbo);
						//logMsg("action %d -> %d, pushed %d", action, sysAction, e.state == Input::PUSHED);
						if(turbo)
						{
							if(e.state == Input::PUSHED)
							{
								turboActions.addEvent(sysAction);
							}
							else
							{
								turboActions.removeEvent(sysAction);
							}
						}
						EmuSystem::handleInputAction(e.state, sysAction);
					}
				}
			}
			else
				break;
		}
	}
}

void EmuView::placeOverlay()
{
	vidImgOverlay.place(disp, vidPix.y);
}

void EmuView::placeEffect()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	vidImgEffect.place(vidPix);
	if(vidImg)
	{
		vidImg.setFilter(vidImgEffect.effect() ? Gfx::BufferImage::NEAREST : (uint)optionImgFilter);
	}
	#endif
}

void EmuView::updateAndDrawContent()
{
	vidImg.write(vidPix, vidPixAlign);
	drawContent<1>();
}

void EmuView::initPixmap(char *pixBuff, const PixelFormatDesc *format, uint x, uint y, uint pitch)
{
	new(&vidPix) IG::Pixmap(*format);
	if(!pitch)
		vidPix.init(pixBuff, x, y);
	else
		vidPix.init2(pixBuff, x, y, pitch);
	var_selfs(pixBuff);
}

void EmuView::compileDefaultPrograms()
{
	auto compiled = disp.compileDefaultProgram(Gfx::IMG_MODE_REPLACE);
	compiled |= disp.compileDefaultProgram(Gfx::IMG_MODE_MODULATE);
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(vidImgEffect.effect() && !vidImgEffect.hasProgram())
	{
		vidImgEffect.compile(*disp.image());
		compiled = true;
	}
	#endif
	if(compiled)
		Gfx::autoReleaseShaderCompiler();
}

void EmuView::reinitImage()
{
	vidImg.init(vidPix, 0, optionImgFilter);
	disp.setImg(&vidImg);
	compileDefaultPrograms();
}

void EmuView::resizeImage(uint x, uint y, uint pitch)
{
	resizeImage(0, 0, x, y, x, y, pitch);
}

void EmuView::resizeImage(uint xO, uint yO, uint x, uint y, uint totalX, uint totalY, uint pitch)
{
	IG::Pixmap basePix(vidPix.format);
	if(pitch)
		basePix.init2(pixBuff, totalX, totalY, pitch);
	else
		basePix.init(pixBuff, totalX, totalY);
	vidPix.initSubPixmap(basePix, xO, yO, x, y);
	vidImg.init(vidPix, 0, optionImgFilter);
	vidPixAlign = vidImg.bestAlignment(vidPix);
	logMsg("using %d:%d:%d:%d region of %d,%d pixmap for EmuView, aligned to min %d bytes", xO, yO, x, y, totalX, totalY, vidPixAlign);
	disp.setImg(&vidImg);
	if((uint)optionImageZoom > 100)
		placeEmu();
}

void EmuView::initImage(bool force, uint x, uint y, uint pitch)
{
	if(force || !disp.image() || vidPix.x != x || vidPix.y != y)
	{
		resizeImage(x, y, pitch);
		compileDefaultPrograms();
	}
}

void EmuView::initImage(bool force, uint xO, uint yO, uint x, uint y, uint totalX, uint totalY, uint pitch)
{
	if(force || !disp.image() || vidPix.x != x || vidPix.y != y)
	{
		resizeImage(xO, yO, x, y, totalX, totalY, pitch);
		compileDefaultPrograms();
	}
}
