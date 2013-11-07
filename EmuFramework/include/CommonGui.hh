#pragma once

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

#include <data-type/image/png/sys.hh>
#include <gui/View.hh>
#include <gui/AlertView.hh>
#include "EmuSystem.hh"
#include "Recent.hh"
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
#include <MenuView.hh>
#include <FileUtils.hh>
#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
#include <VController.hh>
#include <TouchConfigView.hh>
#endif
#include <meta.h>

bool isMenuDismissKey(const Input::Event &e);
void startGameFromMenu();
bool touchControlsApplicable();
void loadGameCompleteFromFilePicker(uint result, const Input::Event &e);
extern ViewStack viewStack;
StackAllocator menuAllocator;
uint8 modalViewStorage[2][1024] __attribute__((aligned)) { {0} };
uint modalViewStorageIdx = 0;
WorkDirStack<1> workDirStack;
static bool updateInputDevicesOnResume = 0;
#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
extern SysVController vController;
#endif
InputManagerView *imMenu = nullptr;
EmuNavView viewNav;
static bool menuViewIsActive = 1;
MsgPopup popup;
Base::Window mainWin, secondWin;
EmuView emuView(mainWin);
#ifdef CONFIG_BLUETOOTH
BluetoothAdapter *bta = nullptr;
#endif

void EmuNavView::onLeftNavBtn(const Input::Event &e)
{
	viewStack.popAndShow();
};

void EmuNavView::onRightNavBtn(const Input::Event &e)
{
	if(EmuSystem::gameIsRunning())
	{
		startGameFromMenu();
	}
};

bool vControllerUseScaledCoordinates()
{
	#ifdef CONFIG_BASE_ANDROID
	return vController.useScaledCoordinates;
	#else
	return false;
	#endif
}

void setVControllerUseScaledCoordinates(bool on)
{
	#ifdef CONFIG_BASE_ANDROID
	vController.useScaledCoordinates = on;
	#endif
}

namespace Gfx
{
void onViewChange(Base::Window &win, Gfx::GfxViewState * = 0);
}

#if !defined(CONFIG_AUDIO_ALSA) && !defined(CONFIG_AUDIO_SDL) && !defined(CONFIG_AUDIO_PS3)
// use WIP direct buffer write API
#define USE_NEW_AUDIO
#endif

//static int soundRateDelta = 0;

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
	Base::setIdleDisplayPowerSave(false);
	setupStatusBarInGame();
	if(!optionFrameSkip.isConst && (uint)optionFrameSkip != EmuSystem::optionFrameSkipAuto)
		Base::mainWindow().setVideoInterval((int)optionFrameSkip + 1);
	logMsg("running game");
	menuViewIsActive = 0;
	viewNav.setRightBtnActive(1);
	//logMsg("touch control state: %d", touchControlsAreOn);
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	vController.resetInput();
	#endif
	// TODO: simplify this
	if(!Base::mainWindow().setValidOrientations(optionGameOrientation, 1))
		Gfx::onViewChange(Base::mainWindow());
	#ifndef CONFIG_GFX_SOFT_ORIENTATION
	Gfx::onViewChange(Base::mainWindow());
	#endif
	commonInitInput();
	emuView.ffGuiKeyPush = emuView.ffGuiTouch = 0;

	popup.clear();
	Input::setKeyRepeat(0);
	EmuControls::setupVolKeysInGame();
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
	Base::setRefreshRate(EmuSystem::vidSysIsPAL() ? 50 : 60);
	EmuSystem::start();
	Base::mainWindow().displayNeedsUpdate();

	if(trackFPS)
	{
		frameCount = 0;
		prevFrameTime.setTimeNow();
	}
}

void restoreMenuFromGame()
{
	menuViewIsActive = 1;
	Base::setIdleDisplayPowerSave(optionIdleDisplayPowerSave);
	setupStatusBarInMenu();
	EmuSystem::pause();
	if(!optionFrameSkip.isConst)
		Base::mainWindow().setVideoInterval(1);
	//logMsg("setting valid orientations");
	if(!Base::mainWindow().setValidOrientations(optionMenuOrientation, 1))
		Gfx::onViewChange(Base::mainWindow());
	Input::setKeyRepeat(1);
	Input::setHandleVolumeKeys(0);
	if(!optionRememberLastMenu)
		viewStack.popToRoot();
	Base::setRefreshRate(Base::REFRESH_RATE_DEFAULT);
	Base::mainWindow().displayNeedsUpdate();
	viewStack.show();
}

namespace Base
{

void onExit(bool backgrounded)
{
	Audio::closePcm();
	EmuSystem::pause();
	if(backgrounded)
	{
		EmuSystem::saveAutoState();
		EmuSystem::saveBackupMem();
		if(optionNotificationIcon)
		{
			auto title = CONFIG_APP_NAME " was suspended";
			Base::addNotification(title, title, EmuSystem::gameName);
		}
	}
	else
	{
		EmuSystem::closeGame();
	}

	saveConfigFile();

	#ifdef CONFIG_BLUETOOTH
	if(bta && (!backgrounded || (backgrounded && !optionKeepBluetoothActive)))
		Bluetooth::closeBT(bta);
	#endif

	#ifdef CONFIG_BASE_IOS
	if(backgrounded)
		FsSys::remove("/private/var/mobile/Library/Caches/" CONFIG_APP_ID "/com.apple.opengl/shaders.maps");
	#endif
}

void onFocusChange(Base::Window &win, uint in)
{
	if(optionPauseUnfocused && !menuViewIsActive)
	{
		if(in)
		{
			#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
			vController.resetInput();
			#endif
			EmuSystem::start();
		}
		else
		{
			EmuSystem::pause();
		}
		Base::mainWindow().displayNeedsUpdate();
	}
}

static void handleOpenFileCommand(const char *filename)
{
	auto type = FsSys::fileType(filename);
	logMsg("%d %s", type, filename);
	if(type == Fs::TYPE_DIR)
	{
		logMsg("changing to dir %s from external command", filename);
		restoreMenuFromGame();
		FsSys::chdir(filename);
		viewStack.popToRoot();
		auto &fPicker = *menuAllocator.allocNew<EmuFilePicker>(Base::mainWindow());
		fPicker.init(Input::keyInputIsPresent(), false);
		viewStack.useNavView = 0;
		viewStack.pushAndShow(&fPicker, &menuAllocator);
		return;
	}
	if(type != Fs::TYPE_FILE || !EmuFilePicker::defaultFsFilter(filename, type))
	{
		logMsg("unrecognized file type");
		return;
	}
	FsSys::cPath dirnameTemp, basenameTemp;
	auto dir = string_dirname(filename, dirnameTemp);
	auto file = string_basename(filename, basenameTemp);
	FsSys::chdir(dir);
	logMsg("opening file %s in dir %s from external command", file, dir);
	restoreMenuFromGame();
	viewStack.popToRoot();
	if(View::modalView)
		View::removeModalView();
	GameFilePicker::onSelectFile(file, Input::Event{});
}

void onDragDrop(Base::Window &win, const char *filename)
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
	if(updateInputDevicesOnResume)
	{
		updateInputDevices();
		EmuControls::updateAutoOnScreenControlVisible();
		updateInputDevicesOnResume = 0;
	}

	if(optionPauseUnfocused)
		onFocusChange(Base::mainWindow(), focused); // let focus handler deal with resuming emulation
	else
	{
		if(!menuViewIsActive) // resume emulation
		{
			#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
			vController.resetInput();
			#endif
			EmuSystem::start();
			Base::mainWindow().displayNeedsUpdate();
		}
	}
}

}

namespace Gfx
{
void onDraw(Base::Window &win, Gfx::FrameTimeBase frameTime)
{
	#ifdef CONFIG_BASE_MULTI_WINDOW
	if(win != mainWin)
	{
		if(EmuSystem::isActive())
		{
			win.displayNeedsUpdate();
			resetTransforms();
			setBlendMode(0);
			setImgMode(IMG_MODE_REPLACE);
			emuView.drawContent<0>();
		}
		return;
	}
	#endif
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

void onInputDevChange(const Device &dev, const Device::Change &change)
{
	logMsg("got input dev change");

	if(Base::appIsRunning())
	{
		updateInputDevices();
		EmuControls::updateAutoOnScreenControlVisible();

		if(optionNotifyInputDeviceChange && (change.added() || change.removed()))
		{
			popup.printf(2, 0, "%s #%d %s", dev.name(), dev.enumId() + 1, change.added() ? "connected" : "disconnected");
			Base::mainWindow().displayNeedsUpdate();
		}

		#ifdef CONFIG_BLUETOOTH
		if(viewStack.size == 1) // update bluetooth items
			viewStack.top()->onShow();
		#endif
	}
	else
	{
		logMsg("delaying input device changes until app resumes");
		updateInputDevicesOnResume = 1;
	}
}

}

static void handleInputEvent(Base::Window &win, const Input::Event &e)
{
	#ifdef CONFIG_BASE_MULTI_WINDOW
	if(win != mainWin)
	{
		logMsg("input for 2nd window");
		return;
	}
	#endif

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
				{
					Base::exit();
					return;
				}
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
	float size = optionFontSize / 1000.;
	logMsg("setting up font size %f", (double)size);
	View::defaultFace->applySettings(FontSettings(Base::mainWindow().ySMMSizeToPixel(size)));
}

namespace Gfx
{
void onViewChange(Base::Window &win, GfxViewState *)
{
	#ifdef CONFIG_BASE_MULTI_WINDOW
	if(win != mainWin)
	{
		logMsg("resize for 2nd window");
		return;
	}
	#endif

	logMsg("view change");
	if((int)optionViewportZoom != 100)
	{
		auto viewRect = win.untransformedViewBounds();
		IG::Point2D<int> viewCenter {(int)win.w/2, (int)win.h/2};
		viewRect -= viewCenter;
		viewRect.x = viewRect.x * optionViewportZoom/100.;
		viewRect.y = viewRect.y * optionViewportZoom/100.;
		viewRect.x2 = viewRect.x2 * optionViewportZoom/100.;
		viewRect.y2 = viewRect.y2 * optionViewportZoom/100.;
		viewRect += viewCenter;
		win.adjustViewport(viewRect);
	}
	GuiTable1D::setDefaultXIndent();
	popup.place();
	emuView.place();
	viewStack.place(win.viewBounds());
	if(View::modalView)
		View::modalView->placeRect(win.viewBounds());
	logMsg("done view change");
}
}

static const char *assetFilename[] =
{
	"navArrow.png",
	"x.png",
	"accept.png",
	"game.png",
	"menu.png",
	"fastForward.png"
};

static Gfx::BufferImage assetBuffImg[sizeofArray(assetFilename)];

Gfx::BufferImage &getAsset(uint assetID)
{
	assert(assetID < sizeofArray(assetFilename));
	auto &res = assetBuffImg[assetID];
	if(!res)
	{
		PngFile png;
		if(png.loadAsset(assetFilename[assetID]) != OK)
		{
			bug_exit("couldn't load %s", assetFilename[assetID]);
		}
		res.init(png);
	}
	return res;
}

static const char *launchGame = nullptr;

static void parseCmdLineArgs(int argc, char** argv)
{
	if(argc < 2)
	{
		return;
	}
	launchGame = argv[1];
	logMsg("starting game from command line: %s", launchGame);
}

static void mainInitCommon(int argc, char** argv)
{
	doOrAbort(Audio::init());
	initOptions();
	EmuSystem::initOptions();
	parseCmdLineArgs(argc, argv);
	loadConfigFile();
	#ifdef USE_BEST_COLOR_MODE_OPTION
	Base::Window::setPixelBestColorHint(optionBestColorModeHint);
	#endif
	EmuSystem::onOptionsLoaded();
	mainWin.init({0, 0}, {0, 0});
	Base::setIdleDisplayPowerSave(optionIdleDisplayPowerSave);
	applyOSNavStyle();
	setupStatusBarInMenu();
}

#include <main/EmuMenuViews.hh>
static SystemMenuView mMenu(mainWin);
ViewStack viewStack(mMenu);

template <size_t NAV_GRAD_SIZE>
static void mainInitWindowCommon(Base::Window &win, const Gfx::LGradientStopDesc (&navViewGrad)[NAV_GRAD_SIZE])
{
	#ifdef CONFIG_BASE_MULTI_WINDOW
	if(win != mainWin)
	{
		logMsg("init for 2nd window");
		win.setTitle("Test Window");
		win.show();
		return;
	}
	#endif

	win.setTitle(CONFIG_APP_NAME);
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

	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLLER_RESOLUTION_CHANGE
	if(!optionTouchCtrlImgRes.isConst)
		optionTouchCtrlImgRes.initDefault((Gfx::viewPixelWidth() * Gfx::viewPixelHeight() > 380000) ? 128 : 64);
	#endif

	View::defaultFace = ResourceFace::loadSystem();
	assert(View::defaultFace);

	updateInputDevices();
	EmuSystem::audioFramesPerVideoFrame = optionSoundRate / (EmuSystem::vidSysIsPAL() ? 50 : 60);
	EmuSystem::configAudioRate();

	emuView.disp.init();
	#if defined CONFIG_BASE_ANDROID && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE
	emuView.disp.flags = Gfx::Sprite::HINT_NO_MATRIX_TRANSFORM;
	#endif
	emuView.vidImgOverlay.setEffect(optionOverlayEffect);
	emuView.vidImgOverlay.intensity = optionOverlayEffectLevel/100.;

	#ifdef CONFIG_BASE_ANDROID
	if(optionDPI != 0U)
		win.setDPI(optionDPI);
	#endif
	setupFont();
	popup.init();
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	initVControls();
	EmuControls::updateVControlImg();
	vController.menuBtnSpr.init(&getAsset(ASSET_MENU));
	vController.ffBtnSpr.init(&getAsset(ASSET_FAST_FORWARD));
	#endif

	View::onRemoveModalView() =
		[]()
		{
			if(!menuViewIsActive)
			{
				startGameFromMenu();
			}
		};
	//logMsg("setting up view stack");
	viewNav.init(View::defaultFace, View::needsBackControl ? &getAsset(ASSET_ARROW) : nullptr,
			!Config::envIsPS3 ? &getAsset(ASSET_GAME_ICON) : nullptr, navViewGrad, sizeofArray(navViewGrad));
	viewNav.setRightBtnActive(0);
	viewStack.init(win);
	if(optionTitleBar)
	{
		//logMsg("title bar on");
		viewStack.setNavView(&viewNav);
	}
	mMenu.init(Input::keyInputIsPresent());
	//logMsg("setting menu orientation");
	// set orientation last since it can trigger onViewChange()
	win.setValidOrientations(optionMenuOrientation, 1);
	win.setAcceptDnd(1);

	#if defined CONFIG_BASE_ANDROID && CONFIG_ENV_ANDROID_MINSDK >= 9
	if(!Base::apkSignatureIsConsistent())
	{
		auto &ynAlertView = *allocModalView<YesNoAlertView>(win);
		ynAlertView.init("Warning: App has been modified by 3rd party, use at your own risk", 0);
		ynAlertView.onNo() =
			[](const Input::Event &e)
			{
				Base::exit();
			};
		View::addModalView(ynAlertView);
	}
	#endif

	Gfx::onViewChange(win);
	mMenu.show();

	win.show();

	if(launchGame)
	{
		FsSys::chdir(Base::appPath);
		Base::handleOpenFileCommand(launchGame);
	}
}

void OptionCategoryView::init(bool highlightFirst)
{
	//logMsg("running option category init");
	uint i = 0;
	forEachInArray(subConfig, e)
	{
		e->init(); item[i++] = e;
		e->onSelect() =
		[this, e_i](TextMenuItem &, const Input::Event &e)
		{
			auto &oCategoryMenu = *menuAllocator.allocNew<SystemOptionView>(window());
			oCategoryMenu.init(e_i, !e.isPointer());
			viewStack.pushAndShow(&oCategoryMenu, &menuAllocator);
		};
	}
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}
