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
#include <MsgPopup.hh>
#include <gui/AlertView.hh>
#include <FilePicker.hh>
#include <Screenshot.hh>
#include <algorithm>

#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
extern SysVController vController;
#endif
extern bool touchControlsAreOn;
bool touchControlsApplicable();
extern MsgPopup popup;
extern ViewStack viewStack;
void restoreMenuFromGame();

void EmuView::placeEmu()
{
	if(EmuSystem::gameIsRunning())
	{
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

			GC gameAR = GC(gameX) / GC(gameY);

			// avoid overly wide images (SNES, etc.) or tall images (2600, etc.)
			if(gameAR >= 2)
			{
				logMsg("unscaled image too wide, doubling height to compensate");
				gameY *= 2;
				gameAR = GC(gameX) / GC(gameY);
			}
			else if(gameAR < 0.8)
			{
				logMsg("unscaled image too tall, doubling width to compensate");
				gameX *= 2;
				gameAR = GC(gameX) / GC(gameY);
			}

			if(gameAR > Gfx::proj.aspectRatio)
			{
				scaleFactor = std::max(1U, Gfx::viewPixelWidth() / gameX);
				logMsg("using x scale factor %d", scaleFactor);
			}
			else
			{
				scaleFactor = std::max(1U, Gfx::viewPixelHeight() / gameY);
				logMsg("using y scale factor %d", scaleFactor);
			}

			gameRect.x = 0;
			gameRect.y = 0;
			gameRect.x2 = gameX * scaleFactor;
			gameRect.y2 = gameY * scaleFactor;
			gameRect.setPos({(int)Gfx::viewPixelWidth()/2 - gameRect.x2/2, (int)Gfx::viewPixelHeight()/2 - gameRect.y2/2});
		}

		// compute the video rectangle in world coordinates for sub-pixel placement
		if((uint)optionImageZoom <= 100 || (uint)optionImageZoom == optionImageZoomIntegerOnlyY)
		{
			Rational aR(EmuSystem::aspectRatioX, EmuSystem::aspectRatioY);
			if(optionAspectRatio == 1U)
				aR = {1, 1};
			else if(optionAspectRatio == 2U)
				aR = {0, 1};

			if((uint)optionImageZoom == optionImageZoomIntegerOnlyY)
			{
				// get width from previously calculated pixel height
				GC width = Gfx::iYSize(gameRect.ySize()) * (GC)aR;
				if(!aR)
				{
					width = Gfx::proj.w;
				}
				gameRectG.x = -width/2.;
				gameRectG.x2 = width/2.;
			}
			else
			{
				IG::Point2D<GC> size { Gfx::proj.w, Gfx::proj.h };
				if(aR)
				{
					size = IG::sizesWithRatioBestFit((GC)aR, size.x, size.y);
				}
				gameRectG.x = -size.x/2.;
				gameRectG.x2 = size.x/2.;
				gameRectG.y = size.y/2.;
				gameRectG.y2 = -size.y/2.;
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
		GC yOffset = 0;
		int yOffsetPixels = 0;
		#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
		if(Gfx::proj.aspectRatio < 1. && touchControlsAreOn && touchControlsApplicable())
		{
			if(vController.gp.dp.origin.onBottom() && vController.gp.btnO.onBottom())
			{
				logMsg("moving game rect to top");
				if(vController.gp.cenBtnO.onTop())
				{
					yOffset = Gfx::iXSize(-vController.gp.centerBtnBound[0].ySize());
					yOffsetPixels = -vController.gp.centerBtnBound[0].ySize();
				}
				gameRectG.setYPos(Gfx::proj.rect.y, CT2DO);
				gameRect.setYPos(0, CT2DO);
			}
			else if(vController.gp.dp.origin.onTop() && vController.gp.btnO.onTop())
			{
				logMsg("moving game rect to bottom");
				if(vController.gp.cenBtnO.onBottom())
				{
					yOffset = Gfx::iXSize(vController.gp.centerBtnBound[0].ySize());
					yOffsetPixels = vController.gp.centerBtnBound[0].ySize();
				}
				gameRectG.setYPos(Gfx::proj.rect.y2, CB2DO);
				gameRect.setYPos(Gfx::viewPixelHeight(), CB2DO);
			}
		}
		#endif

		// apply sub-pixel zoom
		if(optionImageZoom.val < 100)
		{
			auto scaler = (GC(optionImageZoom.val) / 100.);
			gameRectG.x *= scaler;
			gameRectG.y *= scaler;
			gameRectG.x2 *= scaler;
			gameRectG.y2 *= scaler;
		}

		// apply y offset after zoom
		gameRectG += IG::Point2D<GC>{0, yOffset};
		gameRect += IG::Point2D<int>{0, yOffsetPixels};

		// assign final coordinates
		auto fromWorldSpaceRect = Gfx::projectRect2(gameRectG);
		auto fromPixelRect = Gfx::unProjectRect2(gameRect);
		if(getXCoordinateFromPixels)
		{
			gameRectG.x = fromPixelRect.x;
			gameRectG.x2 = fromPixelRect.x2;
		}
		else
		{
			gameRect.x = fromWorldSpaceRect.x;
			gameRect.x2 = fromWorldSpaceRect.x2;
		}
		if(getYCoordinateFromPixels)
		{
			gameRectG.y = fromPixelRect.y;
			gameRectG.y2 = fromPixelRect.y2;
		}
		else
		{
			gameRect.y = fromWorldSpaceRect.y;
			gameRect.y2 = fromWorldSpaceRect.y2;
		}

		disp.setPos(gameRectG.x, gameRectG.y2, gameRectG.x2, gameRectG.y);
		#if defined CONFIG_BASE_ANDROID && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE
		disp.screenX = gameView.xIPos(LB2DO);
		disp.screenY = Gfx::viewPixelHeight() - gameView.yIPos(LB2DO);
		disp.screenX2 = gameView.iXSize;
		disp.screenY2 = gameView.iYSize;
		#endif
		logMsg("placed game rect at pixels %d:%d:%d:%d, world %f:%f:%f:%f",
				gameRect.x, gameRect.y, gameRect.x2, gameRect.y2,
				(double)gameRectG.x, (double)gameRectG.y, (double)gameRectG.x2, (double)gameRectG.y2);
	}
	placeOverlay();
}

template <bool active>
void EmuView::drawContent()
{
	using namespace Gfx;
	disp.draw();
	vidImgOverlay.draw();
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	if(active && ((touchControlsAreOn && touchControlsApplicable())
		#ifdef CONFIG_VCONTROLLER_KEYBOARD
		|| vController.kbMode
		#endif
	))
	{
		vController.draw();
		if(optionShowMenuIcon)
		{
			setBlendMode(BLEND_MODE_INTENSITY);
			menuIcon.draw();
		}
	}
	#endif
	popup.draw();
}

template void EmuView::drawContent<0>();
template void EmuView::drawContent<1>();

void EmuView::draw(Gfx::FrameTimeBase frameTime)
{
	using namespace Gfx;
	if(likely(EmuSystem::isActive()))
	{
		resetTransforms();
		setBlendMode(0);
		setImgMode(IMG_MODE_REPLACE);
		/*setImgMode(IMG_MODE_MODULATE);
		setColor(COLOR_WHITE);*/
		#ifdef CONFIG_BASE_PS3
		setColor(1., 1., 1., 1.); // hack to work-around non-working GFX_IMG_MODE_REPLACE
		#endif
		Base::displayNeedsUpdate();

		runFrame(frameTime);
	}
	else if(EmuSystem::isStarted())
	{
		setBlendMode(0);
		setImgMode(IMG_MODE_MODULATE);
		setColor(.33, .33, .33, 1.);
		resetTransforms();
		drawContent<0>();
	}
}

void EmuView::runFrame(Gfx::FrameTimeBase frameTime)
{
	commonUpdateInput();
	bool renderAudio = optionSound;

	if(unlikely(ffGuiKeyPush || ffGuiTouch))
	{
		iterateTimes(4, i)
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
	//if(emuActive)
	{
		#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
			//if(touchControlsAreOn())
			{
				EmuControls::setupVControllerVars();
				vController.place();
			}
			if(optionTouchCtrlMenuPos != NULL2DO)
			{
				menuB = Gfx::relRectFromViewport(0, 0, Gfx::xSMMSizeToPixel(9), optionTouchCtrlMenuPos, optionTouchCtrlMenuPos);
			}
			if(optionTouchCtrlFFPos != NULL2DO)
			{
				fastForwardB = Gfx::relRectFromViewport(0, 0, Gfx::xSMMSizeToPixel(9), optionTouchCtrlFFPos, optionTouchCtrlFFPos);
			}
			using namespace Gfx;
			menuIcon.setPos(gXPos(menuB, LB2DO) + gXSize(menuB) / 4.0, gYPos(menuB, LB2DO) + gYSize(menuB) / 3.0,
					gXPos(menuB, RT2DO) - gXSize(menuB) / 4.0, gYPos(menuB, RT2DO) - gYSize(menuB) / 3.0);
		#endif
	}
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
		if(e.state == Input::PUSHED && optionTouchCtrlMenuPos != NULL2DO && menuB.overlaps(e.x, e.y))
		{
			viewStack.top()->clearSelection();
			restoreMenuFromGame();
			return;
		}
		else if(e.state == Input::PUSHED && optionTouchCtrlFFPos != NULL2DO && fastForwardB.overlaps(e.x, e.y))
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
						auto &fPicker = *menuAllocator.allocNew<EmuFilePicker>();
						fPicker.init(Input::keyInputIsPresent());
						viewStack.useNavView = 0;
						viewStack.pushAndShow(&fPicker, &menuAllocator);
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
						int ret = EmuSystem::saveState();
						if(ret != STATE_RESULT_OK)
						{
							popup.postError(stateResultToStr(ret));
						}
						else
							popup.post("State Saved");
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
						auto &ynAlertView = *allocModalView<YesNoAlertView>();
						ynAlertView.init("Really Exit?", Input::keyInputIsPresent());
						ynAlertView.onYes() =
							[](const Input::Event &e)
							{
								Base::exit();
							};
						View::addModalView(ynAlertView);
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
