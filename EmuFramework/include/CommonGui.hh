/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#pragma once

#include <gui/View.hh>
#include <gui/AlertView.hh>
#include "EmuSystem.hh"
#include "Recent.hh"
#include "Screenshot.hh"
#include <util/gui/ViewStack.hh>
#include <VideoImageOverlay.hh>
#include "EmuOptions.hh"
#include <EmuInput.hh>
#include "MsgPopup.hh"
#include "MultiChoiceView.hh"
#include "ConfigFile.hh"
#include "FilePicker.hh"
#include <InputManagerView.hh>
#include <EmuView.hh>
#include <TextEntry.hh>

#include <meta.h>

bool isMenuDismissKey(const Input::Event &e);
void startGameFromMenu();
bool touchControlsApplicable();
void loadGameCompleteFromFilePicker(uint result, const Input::Event &e);
EmuFilePicker fPicker;
extern const char *creditsViewStr;
CreditsView credits(creditsViewStr);
YesNoAlertView ynAlertView;
#ifdef INPUT_SUPPORTS_POINTER
Gfx::Sprite menuIcon;
#endif
MultiChoiceView multiChoiceView;
CollectTextInputView textInputView;
ViewStack viewStack;
WorkDirStack<1> workDirStack;
static bool updateInpueDevicesOnResume = 0;

void chdirFromFilePath(const char *path)
{
	FsSys::chdir(string_dirname(path));
}

void onCloseModalPopWorkDir(const Input::Event &e)
{
	View::removeModalView();
	workDirStack.pop();
}

void onLeftNavBtn(const Input::Event &e)
{
	viewStack.popAndShow();
}

void onRightNavBtn(const Input::Event &e)
{
	if(EmuSystem::gameIsRunning())
	{
		startGameFromMenu();
	}
}

BasicNavView viewNav(NavView::OnInputDelegate::create<&onLeftNavBtn>(), NavView::OnInputDelegate::create<&onRightNavBtn>());
static bool menuViewIsActive = 1;

static Rect2<int> emuMenuB, emuFFB;
MsgPopup popup;

EmuView emuView;

namespace Gfx
{
void onViewChange(Gfx::GfxViewState * = 0);
}

#if !defined(CONFIG_AUDIO_ALSA) && !defined(CONFIG_AUDIO_SDL) && !defined(CONFIG_AUDIO_PS3) && !defined CONFIG_AUDIO_COREAUDIO
	// use WIP direct buffer write API
	#define USE_NEW_AUDIO
#endif

// used on iOS to allow saves on incorrectly root-owned files/dirs
void fixFilePermissions(const char *path)
{
	if(FsSys::hasWriteAccess(path) == 0)
	{
		logMsg("%s lacks write permission, setting user as owner", path);
	}
	else
		return;

	if(!Base::setUIDEffective())
	{
		logErr("failed to set effective uid");
		return;
	}
	FsSys::chown(path, Base::realUID, Base::realUID);
	Base::setUIDReal();
}

//static int soundRateDelta = 0;
bool ffGuiKeyPush = 0, ffGuiTouch = 0;

static GC fontMM =
#if defined(CONFIG_BASE_IOS) || CONFIG_ENV_WEBOS_OS >= 3
	3.5
#elif defined(CONFIG_BASE_ANDROID) || defined(CONFIG_ENV_WEBOS)
	3.2
#elif defined(CONFIG_BASE_PS3)
	10
#else
	8
#endif
;

static GC largeFontMM =
#if defined(CONFIG_BASE_ANDROID)
	4.5
#elif defined(CONFIG_BASE_IOS) || defined(CONFIG_ENV_WEBOS)
	5
#elif defined(CONFIG_BASE_PS3)
	14
#else
	14
#endif
;

#ifndef CONFIG_BASE_PS3

	#include "VController.hh"
	SysVController vController;

	void refreshTouchConfigMenu();
	void setupVControllerPosition()
	{
		vController.gp.dp.origin = optionTouchCtrlDpadPos;
		vController.gp.btnO = optionTouchCtrlFaceBtnPos;
		vController.gp.cenBtnO = optionTouchCtrlCenterBtnPos;
		vController.gp.triggerPos = optionTouchCtrlTriggerBtnPos;
	}

	static const _2DOrigin allCornersO[] = { RT2DO, RC2DO, RB2DO, CB2DO, LB2DO, LC2DO, LT2DO, CT2DO };
	static const _2DOrigin onlyTopBottomO[] = { RT2DO, RB2DO, CB2DO, LB2DO, LT2DO, CT2DO };
	template <size_t S, size_t S2>
	static _2DOrigin getFreeOnScreenSpace(const _2DOrigin(&occupiedCorner)[S], const _2DOrigin(&wantedCorner)[S2])
	{
		forEachInArray(wantedCorner, e)
		{
			if(!equalsAny(*e, occupiedCorner))
				return *e;
		}
		return NULL2DO; // no free corners
	}

	static bool onScreenObjectCanOverlap(_2DOrigin &a, _2DOrigin &b)
	{
		return (&a == &optionTouchCtrlCenterBtnPos.val || &b == &optionTouchCtrlCenterBtnPos.val) // one is the center btn. group, and
			&& (&a == &optionTouchCtrlFaceBtnPos.val || &b == &optionTouchCtrlFaceBtnPos.val
					|| &a == &optionTouchCtrlDpadPos.val || &b == &optionTouchCtrlDpadPos.val); // one is the dpad/face btn. group
	}

	void resolveOnScreenCollisions(_2DOrigin *movedObj = 0)
	{
		_2DOrigin *obj[] = { &optionTouchCtrlFaceBtnPos.val, &optionTouchCtrlDpadPos.val, &optionTouchCtrlCenterBtnPos.val, &optionTouchCtrlMenuPos.val, &optionTouchCtrlFFPos.val };
		iterateTimes(sizeofArray(obj), i)
		{
			if(movedObj == obj[i] || *obj[i] == NULL2DO) // don't move object that was just placed, and ignore objects that are off
			{
				//logMsg("skipped obj %d", (int)i);
				continue;
			}

			iterateTimes(sizeofArray(obj), j)
			{
				if(obj[i] != obj[j] && *obj[j] != NULL2DO && *obj[i] == *obj[j] && !onScreenObjectCanOverlap(*obj[i], *obj[j]))
				{
					_2DOrigin freeO;
					if(obj[i] == &optionTouchCtrlCenterBtnPos.val)
					{
						// Center btns. can only collide with menu/ff hot-spots
						const _2DOrigin occupied[] = { optionTouchCtrlMenuPos.val, optionTouchCtrlFFPos.val };
						freeO = getFreeOnScreenSpace(occupied, onlyTopBottomO);
					}
					else if(obj[i] == &optionTouchCtrlMenuPos.val || obj[i] == &optionTouchCtrlFFPos.val)
					{
						// Menu/ff hot-spots collide with everything
						const _2DOrigin occupied[] = { optionTouchCtrlMenuPos.val, optionTouchCtrlFFPos.val, optionTouchCtrlFaceBtnPos.val, optionTouchCtrlDpadPos.val, optionTouchCtrlCenterBtnPos.val, };
						freeO = getFreeOnScreenSpace(occupied, allCornersO);
					}
					else
					{
						// Main btns. collide with others of themselves and Menu/ff hot-spots
						const _2DOrigin occupied[] = { optionTouchCtrlMenuPos.val, optionTouchCtrlFFPos.val, optionTouchCtrlFaceBtnPos.val, optionTouchCtrlDpadPos.val };
						freeO = getFreeOnScreenSpace(occupied, allCornersO);
					}
					assert(freeO != NULL2DO);
					logMsg("objs %d & %d collide, moving first to %d,%d", (int)i, (int)j, freeO.x, freeO.y);
					*obj[i] = freeO;
					break;
				}
			}
		}
	}

	void updateVControlImg()
	{
		vController.setImg(ResourceImagePng::loadAsset((optionTouchCtrlImgRes == 128U) ? "overlays128.png" : "overlays64.png"));
		#ifdef CONFIG_VCONTROLLER_KEYBOARD
		vController.kb.setImg(ResourceImagePng::loadAsset("kbOverlay.png"));
		#endif
	}

#endif

void setupStatusBarInMenu()
{
	if(!optionHideStatusBar.isConst)
		Base::setStatusBarHidden(optionHideStatusBar > 1);
}

static void setupStatusBarInGame()
{
	if(!optionHideStatusBar.isConst)
		Base::setStatusBarHidden(optionHideStatusBar);
}

void onRemoveModalView()
{
	if(!menuViewIsActive)
	{
		startGameFromMenu();
	}
}

bool touchControlsAreOn = 0;



#ifdef INPUT_SUPPORTS_POINTER
void setupVControllerVars()
{
	vController.gp.btnSize = Gfx::xMMSize(int(optionTouchCtrlSize) / 100.);
	logMsg("set on-screen button size: %f, %d pixels", (double)vController.gp.btnSize, Gfx::toIXSize(vController.gp.btnSize));
	vController.gp.dp.deadzone = Gfx::xMMSizeToPixel(int(optionTouchDpadDeadzone) / 100.);
	vController.gp.dp.diagonalSensitivity = optionTouchDpadDiagonalSensitivity / 1000.;
	vController.gp.btnSpace = Gfx::xMMSize(int(optionTouchCtrlBtnSpace) / 100.);
	vController.gp.btnRowShift = 0;
	vController.gp.btnExtraXSize = optionTouchCtrlExtraXBtnSize / 1000.;
	vController.gp.btnExtraYSize = optionTouchCtrlExtraYBtnSize / 1000.;
	vController.gp.btnExtraYSizeMultiRow = optionTouchCtrlExtraYBtnSizeMultiRow / 1000.;
	switch((int)optionTouchCtrlBtnStagger)
	{
		case 0: vController.gp.btnStagger = vController.gp.btnSize * -.75; break;
		case 1: vController.gp.btnStagger = vController.gp.btnSize * -.5; break;
		case 2: vController.gp.btnStagger = 0; break;
		case 3: vController.gp.btnStagger = vController.gp.btnSize * .5; break;
		case 4: vController.gp.btnStagger = vController.gp.btnSize * .75; break;
		default:
			vController.gp.btnStagger = vController.gp.btnSize + vController.gp.btnSpace;
			vController.gp.btnRowShift = -(vController.gp.btnSize + vController.gp.btnSpace);
		break;
	}
	vController.setBoundingAreaVisible(optionTouchCtrlBoundingBoxes);
}
#endif

void setOnScreenControls(bool on)
{
	touchControlsAreOn = on;
	emuView.placeEmu();
}

void updateAutoOnScreenControlVisible()
{
	if((uint)optionTouchCtrl == 2)
	{
		if(touchControlsAreOn && physicalControlsPresent)
		{
			logMsg("auto-turning off on-screen controls");
			setOnScreenControls(0);
		}
		else if(!touchControlsAreOn && !physicalControlsPresent)
		{
			logMsg("auto-turning on on-screen controls");
			setOnScreenControls(1);
		}
	}
}

static void setupVolKeysInGame()
{
	using namespace EmuControls;
	#if defined(INPUT_SUPPORTS_KEYBOARD)
	iterateTimes(inputDevConfs, i)
	{
		if(!inputDevConf[i].enabled ||
			inputDevConf[i].dev->map() != Input::Event::MAP_KEYBOARD ||
			!inputDevConf[i].savedConf) // no default configs use volume keys
		{
			continue;
		}
		auto key = inputDevConf[i].keyConf().key();
		iterateTimes(MAX_KEY_CONFIG_KEYS, k)
		{
			if(Input::isVolumeKey(key[k]))
			{
				logMsg("key config has volume keys");
				Input::setHandleVolumeKeys(1);
				return;
			}
		}
	}
	#endif
}

static bool trackFPS = 0;
static TimeSys prevFrameTime;
static uint frameCount = 0;

void applyOSNavStyle()
{
	if(Base::hasHardwareNavButtons())
		return;
	uint flags = 0;
	if(optionLowProfileOSNav) flags|= Base::OS_NAV_STYLE_DIM;
	if(optionHideOSNav) flags|= Base::OS_NAV_STYLE_HIDDEN;
	Base::setOSNavigationStyle(flags);
}

void startGameFromMenu()
{
	applyOSNavStyle();
	Base::setIdleDisplayPowerSave(0);
	setupStatusBarInGame();
	if(!optionFrameSkip.isConst && (uint)optionFrameSkip != EmuSystem::optionFrameSkipAuto)
		Gfx::setVideoInterval((int)optionFrameSkip + 1);
	logMsg("running game");
	menuViewIsActive = 0;
	viewNav.setRightBtnActive(1);
	//logMsg("touch control state: %d", touchControlsAreOn);
	#ifdef INPUT_SUPPORTS_POINTER
	vController.resetInput();
	#endif
	// TODO: simplify this
	if(!Gfx::setValidOrientations(optionGameOrientation, 1))
		Gfx::onViewChange();
	#ifndef CONFIG_GFX_SOFT_ORIENTATION
	Gfx::onViewChange();
	#endif
	commonInitInput();
	ffGuiKeyPush = ffGuiTouch = 0;

	popup.clear();
	Input::setKeyRepeat(0);
	setupVolKeysInGame();
	/*if(optionFrameSkip == -1)
	{
		gfx_updateFrameTime();
	}*/
	/*if(optionFrameSkip != 0 && soundRateDelta != 0)
	{
		logMsg("reset sound rate delta");
		soundRateDelta = 0;
		audio_setPcmRate(audio_pPCM.rate);
	}*/

	EmuSystem::start();
	Base::displayNeedsUpdate();

	if(trackFPS)
	{
		frameCount = 0;
		prevFrameTime.setTimeNow();
	}
}

static void restoreMenuFromGame()
{
	menuViewIsActive = 1;
	Base::setIdleDisplayPowerSave(
	#ifdef CONFIG_BLUETOOTH
		Bluetooth::devsConnected() ? 0 :
	#endif
		(int)optionIdleDisplayPowerSave);
	//Base::setLowProfileNavigation(0);
	setupStatusBarInMenu();
	EmuSystem::pause();
	if(!optionFrameSkip.isConst)
		Gfx::setVideoInterval(1);
	//logMsg("setting valid orientations");
	if(!Gfx::setValidOrientations(optionMenuOrientation, 1))
		Gfx::onViewChange();
	Input::setKeyRepeat(1);
	Input::setHandleVolumeKeys(0);
	if(!optionRememberLastMenu)
		viewStack.popToRoot();
	Base::displayNeedsUpdate();
	viewStack.show();
}





void confirmExitAppAlert(const Input::Event &e)
{
	Base::exit();
}

namespace Base
{

void onExit(bool backgrounded)
{
	EmuSystem::pause();
	if(backgrounded)
	{
		EmuSystem::saveAutoState();
		EmuSystem::saveBackupMem();
		if(optionNotificationIcon)
		{
			char title[48];
			string_printf(title, "%s was suspended", CONFIG_APP_NAME);
			Base::addNotification(title, title, EmuSystem::gameName);
		}
	}
	else
	{
		EmuSystem::closeGame();
	}

	saveConfigFile();

	#ifdef CONFIG_BLUETOOTH
		if(!backgrounded || (backgrounded && !optionKeepBluetoothActive))
			Bluetooth::closeBT();
	#endif

	#ifdef CONFIG_BASE_IOS
		if(backgrounded)
			unlink("/private/var/mobile/Library/Caches/" CONFIG_APP_ID "/com.apple.opengl/shaders.maps");
	#endif
}

void onFocusChange(uint in)
{
	if(optionPauseUnfocused && !menuViewIsActive)
	{
		if(in)
		{
			#ifdef INPUT_SUPPORTS_POINTER
			vController.resetInput();
			#endif
			EmuSystem::start();
		}
		else
		{
			EmuSystem::pause();
		}
		Base::displayNeedsUpdate();
	}
}

static void handleOpenFileCommand(const char *filename)
{
	auto type = FsSys::fileType(filename);
	if(type == Fs::TYPE_DIR)
	{
		logMsg("changing to dir %s from external command", filename);
		restoreMenuFromGame();
		FsSys::chdir(filename);
		viewStack.popToRoot();
		fPicker.init(Input::keyInputIsPresent());
		viewStack.useNavView = 0;
		viewStack.pushAndShow(&fPicker);
		return;
	}
	if(type != Fs::TYPE_FILE)
		return;
	if(!EmuFilePicker::defaultFsFilter(filename, type))
		return;
	FsSys::cPath dir, file;
	dirName(filename, dir);
	baseName(filename, file);
	FsSys::chdir(dir);
	logMsg("opening file %s in dir %s from external command", file, dir);
	restoreMenuFromGame();
	GameFilePicker::onSelectFile(file, Input::Event{});
}

void onDragDrop(const char *filename)
{
	logMsg("got DnD: %s", filename);
	handleOpenFileCommand(filename);
}

void onInterProcessMessage(const char *filename)
{
	logMsg("got IPC: %s", filename);
	handleOpenFileCommand(filename);
}

void onResume(bool focused)
{
	if(updateInpueDevicesOnResume)
	{
		updateInputDevices();
		updateAutoOnScreenControlVisible();
		updateInpueDevicesOnResume = 0;
	}

	if(optionPauseUnfocused)
		onFocusChange(focused); // let focus handler deal with resuming emulation
	else
	{
		if(!menuViewIsActive) // resume emulation
		{
			#ifdef INPUT_SUPPORTS_POINTER
			vController.resetInput();
			#endif
			EmuSystem::start();
			Base::displayNeedsUpdate();
		}
	}
}

}

void takeGameScreenshot()
{
	FsSys::cPath path;
	int screenshotNum = sprintScreenshotFilename(path);
	if(screenshotNum == -1)
	{
		popup.postError("Too many screenshots");
	}
	else
	{
		if(!writeScreenshot(emuView.vidPix, path))
		{
			popup.printf(2, 1, "Error writing screenshot #%d", screenshotNum);
		}
		else
		{
			popup.printf(2, 0, "Wrote screenshot #%d", screenshotNum);
		}
	}
}

void EmuView::place()
{
	placeEmu();
	//if(emuActive)
	{
		#ifdef INPUT_SUPPORTS_POINTER
			//if(touchControlsAreOn())
			{
				setupVControllerVars();
				vController.place();
			}
			if(optionTouchCtrlMenuPos != NULL2DO)
			{
				emuMenuB = Gfx::relRectFromViewport(0, 0, Gfx::xSMMSizeToPixel(9), optionTouchCtrlMenuPos, optionTouchCtrlMenuPos);
			}
			if(optionTouchCtrlFFPos != NULL2DO)
			{
				emuFFB = Gfx::relRectFromViewport(0, 0, Gfx::xSMMSizeToPixel(9), optionTouchCtrlFFPos, optionTouchCtrlFFPos);
			}
			using namespace Gfx;
			menuIcon.setPos(gXPos(emuMenuB, LB2DO) + gXSize(emuMenuB) / 4.0, gYPos(emuMenuB, LB2DO) + gYSize(emuMenuB) / 3.0,
					gXPos(emuMenuB, RT2DO) - gXSize(emuMenuB) / 4.0, gYPos(emuMenuB, RT2DO) - gYSize(emuMenuB) / 3.0);
		#endif
	}
}

void EmuView::inputEvent(const Input::Event &e)
{
	#ifdef INPUT_SUPPORTS_POINTER
	if(e.isPointer())
	{
		if(e.state == Input::PUSHED && optionTouchCtrlMenuPos != NULL2DO && emuMenuB.overlaps(e.x, e.y))
		{
			viewStack.top()->clearSelection();
			restoreMenuFromGame();
			return;
		}
		else if(e.state == Input::PUSHED && optionTouchCtrlFFPos != NULL2DO && emuFFB.overlaps(e.x, e.y))
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
		else if(!touchControlsAreOn && (uint)optionTouchCtrl == 2
			#ifdef CONFIG_VCONTROLLER_KEYBOARD
			&& !vController.kbMode
			#endif
			&& e.isTouch() && e.state == Input::PUSHED
			)
		{
			logMsg("turning on on-screen controls from touch input");
			touchControlsAreOn = 1;
			emuView.placeEmu();
		}
	}
	else if(e.isRelativePointer())
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
						fPicker.init(Input::keyInputIsPresent());
						viewStack.useNavView = 0;
						viewStack.pushAndShow(&fPicker);
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
						ynAlertView.init("Really Exit?", Input::keyInputIsPresent());
						ynAlertView.onYes().bind<&confirmExitAppAlert>();
						ynAlertView.placeRect(Gfx::viewportRect());
						modalView = &ynAlertView;
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

namespace Gfx
{
void onDraw(Gfx::FrameTimeBase frameTime)
{
	emuView.draw(frameTime);
	if(likely(EmuSystem::isActive()))
	{
		if(trackFPS)
		{
			if(frameCount == 119)
			{
				TimeSys now;
				now.setTimeNow();
				float total = now - prevFrameTime;
				prevFrameTime = now;
				logMsg("%f fps", double(120./total));
				frameCount = 0;
			}
			else
				frameCount++;
		}
		return;
	}

	if(View::modalView)
		View::modalView->draw(frameTime);
	else if(menuViewIsActive)
		viewStack.draw(frameTime);
	popup.draw();
}
}

namespace Input
{

void onInputDevChange(const DeviceChange &change)
{
	logMsg("got input dev change");

	if(Base::appIsRunning())
	{
		updateInputDevices();
		updateAutoOnScreenControlVisible();

		if(change.added() || change.removed())
		{
			if(change.map == Input::Event::MAP_KEYBOARD || change.map == Input::Event::MAP_ICADE)
			{
				#ifdef INPUT_HAS_SYSTEM_DEVICE_HOTSWAP
				if(optionNotifyInputDeviceChange)
				#endif
				{
					popup.post("Input devices have changed", 2, 0);
					Base::displayNeedsUpdate();
				}
			}
			else
			{
				popup.printf(2, 0, "%s %d %s", Input::Event::mapName(change.map), change.devId + 1, change.added() ? "connected" : "disconnected");
				Base::displayNeedsUpdate();
			}
		}

		#ifdef CONFIG_BLUETOOTH
			if(viewStack.size == 1) // update bluetooth items
				viewStack.top()->onShow();
		#endif
	}
	else
	{
		logMsg("delaying input device changes until app resumes");
		updateInpueDevicesOnResume = 1;
	}

	if(menuViewIsActive)
		Base::setIdleDisplayPowerSave(
		#ifdef CONFIG_BLUETOOTH
			Bluetooth::devsConnected() ? 0 :
		#endif
			(int)optionIdleDisplayPowerSave);
}

}

static void handleInputEvent(const Input::Event &e)
{
	if(e.isPointer())
	{
		//logMsg("Pointer %s @ %d,%d", Input::eventActionToStr(e.state), e.x, e.y);
	}
	else
	{
		//logMsg("%s %s from %s", e.device->keyName(e.button), Input::eventActionToStr(e.state), e.device->name());
	}
	if(likely(EmuSystem::isActive()))
	{
		emuView.inputEvent(e);
	}
	else if(View::modalView)
		View::modalView->inputEvent(e);
	else if(menuViewIsActive)
	{
		if(e.state == Input::PUSHED && e.isDefaultCancelButton())
		{
			if(viewStack.size == 1)
			{
				//logMsg("cancel button at view stack root");
				if(EmuSystem::gameIsRunning())
				{
					startGameFromMenu();
				}
				else if(e.map == Input::Event::MAP_KEYBOARD && (Config::envIsAndroid || Config::envIsLinux))
					Base::exit();
			}
			else viewStack.popAndShow();
		}
		if(e.state == Input::PUSHED && isMenuDismissKey(e))
		{
			if(EmuSystem::gameIsRunning())
			{
				startGameFromMenu();
			}
		}
		else viewStack.inputEvent(e);
	}
}

void setupFont()
{
	logMsg("setting up font, large: %d", (int)optionLargeFonts);
	View::defaultFace->applySettings(FontSettings(Gfx::ySMMSizeToPixel(optionLargeFonts ? largeFontMM : fontMM)));
	//View::defaultFace->applySettings(FontSettings(33));
}

namespace Gfx
{
void onViewChange(GfxViewState *)
{
	logMsg("view change");
	GuiTable1D::setDefaultXIndent();
	popup.place();
	emuView.place();
	viewStack.place(Gfx::viewportRect());
	if(View::modalView)
		View::modalView->placeRect(Gfx::viewportRect());
	logMsg("done view change");
}
}

ResourceImage *getArrowAsset()
{
	static ResourceImage *res = 0;
	if(!res)
	{
		res = ResourceImagePng::loadAsset("padButton.png");
		res->ref();
	}
	return res;
}

ResourceImage *getXAsset()
{
	static ResourceImage *res = 0;
	if(!res)
	{
		res = ResourceImagePng::loadAsset("xButton.png");
		res->ref();
	}
	return res;
}

static void mainInitCommon()
{
	initOptions();
	EmuSystem::initOptions();

	#ifdef CONFIG_BLUETOOTH
	assert(EmuSystem::maxPlayers <= 5);
	Bluetooth::maxGamepadsPerType = EmuSystem::maxPlayers;
	#endif

	loadConfigFile();

	#ifdef USE_BEST_COLOR_MODE_OPTION
		Base::setWindowPixelBestColorHint(optionBestColorModeHint);
	#endif
}

#ifndef CONFIG_BASE_PS3
#include "TouchConfigView.hh"
TouchConfigView tcMenu(touchConfigFaceBtnName, touchConfigCenterBtnName);
#endif

#include "CommonViewControl.hh"

#include <MenuView.hh>

#include <main/EmuMenuViews.hh>
static SystemOptionView oCategoryMenu;
static SystemMenuView mMenu;

template <size_t NAV_GRAD_SIZE>
static void mainInitWindowCommon(const Gfx::LGradientStopDesc (&navViewGrad)[NAV_GRAD_SIZE])
{
	Gfx::setClear(1);
	if(!optionDitherImage.isConst)
	{
		Gfx::setDither(optionDitherImage);
	}

	#if defined CONFIG_BASE_ANDROID && CONFIG_ENV_ANDROID_MINSDK >= 9
		if((int8)optionProcessPriority != 0)
			Base::setProcessPriority(optionProcessPriority);

		optionSurfaceTexture.defaultVal = Gfx::supportsAndroidSurfaceTextureWhitelisted();
		if(!Gfx::supportsAndroidSurfaceTexture())
		{
			optionSurfaceTexture = 0;
			optionSurfaceTexture.isConst = 1;
		}
		else if(optionSurfaceTexture == OPTION_SURFACE_TEXTURE_UNSET)
		{
			optionSurfaceTexture = Gfx::useAndroidSurfaceTexture();
		}
		else
		{
			logMsg("using surface texture setting from config file");
			Gfx::setUseAndroidSurfaceTexture(optionSurfaceTexture);
		}
		// optionSurfaceTexture is treated as a boolean value after this point
	#endif

	#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
		optionDirectTexture.defaultVal = Gfx::supportsAndroidDirectTextureWhitelisted();
		if(!Gfx::supportsAndroidDirectTexture())
		{
			optionDirectTexture = 0;
			optionDirectTexture.isConst = 1;
		}
		else if(optionDirectTexture == OPTION_DIRECT_TEXTURE_UNSET)
		{
			optionDirectTexture = Gfx::useAndroidDirectTexture();
		}
		else
		{
			logMsg("using direct texture setting from config file");
			Gfx::setUseAndroidDirectTexture(optionDirectTexture);
		}
		// optionDirectTexture is treated as a boolean value after this point
	#endif

	#ifdef CONFIG_BASE_ANDROID
		if(!optionTouchCtrlImgRes.isConst)
			optionTouchCtrlImgRes.initDefault((Gfx::viewPixelWidth() * Gfx::viewPixelHeight() > 380000) ? 128 : 64);
	#endif

	View::defaultFace = ResourceFace::loadSystem();
	assert(View::defaultFace);

	updateInputDevices();
	if((int)optionTouchCtrl == 2)
		updateAutoOnScreenControlVisible();
	else
		setOnScreenControls(optionTouchCtrl);
	#ifdef INPUT_SUPPORTS_POINTER
	vController.updateMapping(0);
	#endif
	EmuSystem::configAudioRate();
	Base::setIdleDisplayPowerSave(optionIdleDisplayPowerSave);
	applyOSNavStyle();
	setupStatusBarInMenu();

	emuView.disp.init();
	#if defined CONFIG_BASE_ANDROID && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE
	emuView.disp.flags = Gfx::Sprite::HINT_NO_MATRIX_TRANSFORM;
	#endif
	emuView.vidImgOverlay.setEffect(optionOverlayEffect);
	emuView.vidImgOverlay.intensity = optionOverlayEffectLevel/100.;

	if(optionDPI != 0U)
		Base::setDPI(optionDPI);
	setupFont();
	popup.init();
	#ifdef INPUT_SUPPORTS_POINTER
	vController.init((int)optionTouchCtrlAlpha / 255.0, Gfx::xMMSize(int(optionTouchCtrlSize) / 100.));
	updateVControlImg();
	resolveOnScreenCollisions();
	setupVControllerPosition();
	menuIcon.init(getArrowAsset());
	#endif

	View::removeModalViewDelegate().bind<&onRemoveModalView>();
	//logMsg("setting up view stack");
	viewNav.init(View::defaultFace, View::needsBackControl ? getArrowAsset() : nullptr,
			!Config::envIsPS3 ? getArrowAsset() : nullptr, navViewGrad, sizeofArray(navViewGrad));
	viewNav.setRightBtnActive(0);
	viewStack.init();
	if(optionTitleBar)
	{
		//logMsg("title bar on");
		viewStack.setNavView(&viewNav);
	}

	//logMsg("setting menu orientation");
	// set orientation last since it can trigger onViewChange()
	Gfx::setValidOrientations(optionMenuOrientation, 1);
	Base::setAcceptDnd(1);

	#if defined CONFIG_BASE_ANDROID && CONFIG_ENV_ANDROID_MINSDK >= 9
	if(!Base::apkSignatureIsConsistent())
	{
		ynAlertView.init("Warning: App has been modified by 3rd party, use at your own risk", 0);
		ynAlertView.onNo().bind<&confirmExitAppAlert>();
		ynAlertView.placeRect(Gfx::viewportRect());
		View::modalView = &ynAlertView;
		Base::displayNeedsUpdate();
	}
	#endif

	mMenu.init(Input::keyInputIsPresent());
	viewStack.push(&mMenu);
	Gfx::onViewChange();
	mMenu.show();

	Base::displayNeedsUpdate();
}

void OptionCategoryView::onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i)
{
	oCategoryMenu.init(i, !e.isPointer());
	viewStack.pushAndShow(&oCategoryMenu);
}

OptionCategoryView oMenu;

StateSlotView ssMenu;

RecentGameView rMenu;

InputManagerView imMenu;
InputManagerDeviceView imdMenu;
