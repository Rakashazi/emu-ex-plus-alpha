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

#include <meta.h>
#include <imagine/data-type/image/sys.hh>
#include "EmuApp.hh"
#include "EmuSystem.hh"
#include "EmuOptions.hh"
#include "FilePicker.hh"
#include "ConfigFile.hh"
#include <EmuView.hh>
#include <imagine/gui/AlertView.hh>
#include <cmath>

AppWindowData mainWin, extraWin;
bool menuViewIsActive = true;
EmuNavView viewNav;
EmuView emuView{mainWin.win}, emuView2{extraWin.win};
EmuVideo emuVideo;
EmuVideoLayer emuVideoLayer{emuVideo};
EmuInputView emuInputView{mainWin.win};
AppWindowData *emuWin = &mainWin;
ViewStack viewStack;
MsgPopup popup;
BasicViewController modalViewController;
WorkDirStack<1> workDirStack;
static bool trackFPS = 0;
static TimeSys prevFrameTime;
static uint frameCount = 0;
static bool updateInputDevicesOnResume = false;
DelegateFunc<void ()> onUpdateInputDevices;
#ifdef CONFIG_BLUETOOTH
BluetoothAdapter *bta = nullptr;
#endif

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

static void updateProjection(AppWindowData &appWin, const Gfx::Viewport &viewport);
static Gfx::Viewport makeViewport(const Base::Window &win);
void mainInitWindowCommon(Base::Window &win);

Gfx::BufferImage &getAsset(AssetID assetID)
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

Gfx::BufferImage *getCollectTextCloseAsset()
{
	return Config::envIsAndroid ? nullptr : &getAsset(ASSET_CLOSE);
}

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

static void postDrawToEmuWindows()
{
	emuWin->win.postDraw();
}

static void drawEmuVideo()
{
	if(emuView.layer)
		emuView.draw(0);
	else if(emuView2.layer)
		emuView2.draw(0);
	Gfx::setClipRect(false);
	Gfx::presentWindow(emuWin->win);
}

void updateAndDrawEmuVideo()
{
	emuVideo.vidImg.write(emuVideo.vidPix, emuVideo.vidPixAlign);
	drawEmuVideo();
}

void EmuNavView::draw(const Base::Window &win, const Gfx::ProjectionPlane &projP)
{
	using namespace Gfx;
	setBlendMode(0);
	noTexProgram.use(projP.makeTranslate());
	bg.draw();
	setColor(COLOR_WHITE);
	texAlphaProgram.use();
	text.draw(projP.alignToPixel(projP.unProjectRect(viewRect).pos(C2DO)), C2DO, projP);
	if(leftSpr.image())
	{
		if(leftBtnActive)
		{
			setColor(COLOR_WHITE);
			setBlendMode(BLEND_MODE_ALPHA);
			auto trans = projP.makeTranslate(projP.unProjectRect(leftBtn).pos(C2DO));
			trans = trans.rollRotate(angleFromDegree(90));
			leftSpr.useDefaultProgram(IMG_MODE_MODULATE, trans);
			leftSpr.draw();
		}
	}
	if(rightSpr.image())
	{
		if(rightBtnActive)
		{
			setColor(COLOR_WHITE);
			setBlendMode(BLEND_MODE_ALPHA);
			rightSpr.useDefaultProgram(IMG_MODE_MODULATE, projP.makeTranslate(projP.unProjectRect(rightBtn).pos(C2DO)));
			rightSpr.draw();
		}
	}
}

static void animateViewportStep(AppWindowData &appWin)
{
	auto &viewportDelta = appWin.viewportDelta;
	auto &win = appWin.win;
	for(auto &d : viewportDelta)
	{
		d.update(1);
	}
	//logMsg("animated viewport: %d:%d:%d:%d",
	//	viewportDelta[0].now(), viewportDelta[1].now(), viewportDelta[2].now(), viewportDelta[3].now());
	auto v = Gfx::Viewport::makeFromWindow(win, {viewportDelta[0].now(), viewportDelta[1].now(), viewportDelta[2].now(), viewportDelta[3].now()});
	updateProjection(appWin, v);
}

void startViewportAnimation(AppWindowData &winData)
{
	auto newViewport = makeViewport(winData.win);
	auto oldViewport = winData.viewport();
	auto oldViewportAR = (oldViewport.width() && oldViewport.height()) ? oldViewport.aspectRatio() : 0;
	if(newViewport != oldViewport)
	{
		logMsg("animating from viewport %d:%d:%d:%d to %d:%d:%d:%d",
			oldViewport.bounds().x, oldViewport.bounds().y, oldViewport.bounds().x2, oldViewport.bounds().y2,
			newViewport.bounds().x, newViewport.bounds().y, newViewport.bounds().x2, newViewport.bounds().y2);
		auto &viewportDelta = winData.viewportDelta;
		auto type = INTERPOLATOR_TYPE_EASEINOUTQUAD;
		int time = 10;
		viewportDelta[0].set(oldViewport.bounds().x, newViewport.bounds().x, type, time);
		viewportDelta[1].set(oldViewport.bounds().y, newViewport.bounds().y, type, time);
		viewportDelta[2].set(oldViewport.bounds().x2, newViewport.bounds().x2, type, time);
		viewportDelta[3].set(oldViewport.bounds().y2, newViewport.bounds().y2, type, time);
		animateViewportStep(winData);
		if(!winData.viewportDelta[0].isComplete())
		{
			winData.win.setNeedsCustomViewportResize(true);
			winData.win.postDraw();
		}
	}
}

static void updateWindowViewport(AppWindowData &winData, Base::Window::SurfaceChange change)
{
	if(change.surfaceResized())
	{
		updateProjection(winData, makeViewport(winData.win));
	}
	else if(change.contentRectResized())
	{
		startViewportAnimation(winData);
	}
	else if(change.customViewportResized())
	{
		animateViewportStep(winData);
		if(!winData.viewportDelta[0].isComplete())
		{
			//logMsg("continuing viewport animation");
			winData.win.setNeedsCustomViewportResize(true);
			winData.win.postDraw();
		}
	}
}

static void runEmuFrame(Base::FrameTimeBase frameTime, bool fastForward)
{
	commonUpdateInput();
	bool renderAudio = optionSound;

	if(unlikely(fastForward))
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
			drawEmuVideo();
			return;
		}
	}

	EmuSystem::runFrame(1, 1, renderAudio);
}

static void onFocusChange(uint in)
{
	if(!menuViewIsActive)
	{
		if(in && EmuSystem::isStarted())
		{
			logMsg("resuming emulation due to window focus");
			#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
			vController.resetInput();
			#endif
			EmuSystem::start();
			postDrawToEmuWindows();
		}
		else if(optionPauseUnfocused && !mainWin.focused && (!extraWin.win || !extraWin.focused))
		{
			logMsg("pausing emulation with all windows unfocused");
			EmuSystem::pause();
			postDrawToEmuWindows();
		}
	}
}

void setEmuViewOnExtraWindow(bool on)
{
	if(on && !extraWin.win)
	{
		logMsg("setting emu view on extra window");
		EmuSystem::resetFrameTime();
		auto winConf = Gfx::makeWindowConfig();
		if(Base::Screen::screens() > 1)
		{
			winConf.setScreen(*Base::Screen::screen(1));
		}
		extraWin.win.init(winConf);
		extraWin.focused = true;
		logMsg("init extra window");
		emuWin = &extraWin;
		std::swap(emuView.layer, emuView2.layer);
		updateProjection(extraWin, makeViewport(extraWin.win));
		extraWin.win.setTitle(CONFIG_APP_NAME);
		extraWin.win.show();
		extraWin.win.postDraw();
		emuView.place();
		mainWin.win.postDraw();

		extraWin.win.setOnSurfaceChange(
			[](Base::Window &win, Base::Window::SurfaceChange change)
			{
				if(change.resized())
				{
					logMsg("view resize for extra window");
					updateProjection(extraWin, makeViewport(win));
					emuView2.setViewRect(extraWin.viewport().bounds(), extraWin.projectionPlane);
					emuView2.place();
				}
			});

		extraWin.win.setOnDraw(
			[](Base::Window &win, Base::Window::DrawParams params)
			{
				if(Gfx::setCurrentWindow(&win) || params.wasResized())
				{
					Gfx::setViewport(extraWin.viewport());
					Gfx::setProjectionMatrix(extraWin.projectionMat);
				}
				Gfx::clear();
				if(EmuSystem::isActive())
				{
					win.postDraw();
					runEmuFrame(params.frameTime(), fastForwardActive);
				}
				else
				{
					emuView2.draw(params.frameTime());
					Gfx::setClipRect(false);
					Gfx::presentWindow(win);
				}
			});

		extraWin.win.setOnInputEvent(
			[](Base::Window &win, const Input::Event &e)
			{
				if(!e.isPointer())
					handleInputEvent(win, e);
			});

		extraWin.win.setOnFocusChange(
			[](Base::Window &win, uint in)
			{
				extraWin.focused = in;
				onFocusChange(in);
			});

		extraWin.win.setOnDismissRequest(
			[](Base::Window &win)
			{
				win.dismiss();
			});

		extraWin.win.setOnDismiss(
			[](Base::Window &win)
			{
				Gfx::setCurrentWindow(nullptr);
				EmuSystem::resetFrameTime();
				logMsg("setting emu view on main window");
				emuWin = &mainWin;
				std::swap(emuView.layer, emuView2.layer);
				emuView.place();
				mainWin.win.postDraw();
			});
	}
	else if(!on && extraWin.win)
	{
		extraWin.win.dismiss();
	}
}

void applyOSNavStyle(bool inGame)
{
	auto flags = Base::SYS_UI_STYLE_NO_FLAGS;
	if(optionLowProfileOSNav > (inGame ? 0 : 1))
		flags |= Base::SYS_UI_STYLE_DIM_NAV;
	if(optionHideOSNav > (inGame ? 0 : 1))
		flags |= Base::SYS_UI_STYLE_HIDE_NAV;
	if(optionHideStatusBar > (inGame ? 0 : 1))
		flags |= Base::SYS_UI_STYLE_HIDE_STATUS;
	Base::setSysUIStyle(flags);
}

void startGameFromMenu()
{
	Base::setIdleDisplayPowerSave(false);
	applyOSNavStyle(true);
	if(!optionFrameSkip.isConst && (uint)optionFrameSkip != EmuSystem::optionFrameSkipAuto)
		emuWin->win.screen().setFrameInterval(optionFrameSkip + 1);
	logMsg("running game");
	menuViewIsActive = 0;
	viewNav.setRightBtnActive(1);
	emuInputView.resetInput();
	//logMsg("touch control state: %d", touchControlsAreOn);
	mainWin.win.setValidOrientations(optionGameOrientation);
	commonInitInput();
	popup.clear();
	Input::setKeyRepeat(false);
	EmuControls::setupVolKeysInGame();
	emuWin->win.screen().setRefreshRate(EmuSystem::vidSysIsPAL() ? 50 : 60);
	EmuSystem::start();
	mainWin.win.postDraw();
	if(extraWin.win)
		extraWin.win.postDraw();
	emuView.place();
	emuView2.place();
	if(trackFPS)
	{
		frameCount = 0;
		prevFrameTime = TimeSys::now();
	}
}

void restoreMenuFromGame()
{
	menuViewIsActive = 1;
	Base::setIdleDisplayPowerSave(optionIdleDisplayPowerSave);
	applyOSNavStyle(false);
	EmuSystem::pause();
	if(!optionFrameSkip.isConst)
		mainWin.win.screen().setFrameInterval(1);
	mainWin.win.setValidOrientations(optionMenuOrientation);
	Input::setKeyRepeat(true);
	Input::setHandleVolumeKeys(false);
	if(!optionRememberLastMenu)
		viewStack.popToRoot();
	mainWin.win.screen().setRefreshRate(Base::Screen::REFRESH_RATE_DEFAULT);
	mainWin.win.postDraw();
	if(extraWin.win)
		extraWin.win.postDraw();
	viewStack.show();
}

static const char *parseCmdLineArgs(int argc, char** argv)
{
	if(argc < 2)
	{
		return nullptr;
	}
	auto launchGame = argv[1];
	logMsg("starting game from command line: %s", launchGame);
	return launchGame;
}

void mainInitCommon(int argc, char** argv, const Gfx::LGradientStopDesc *navViewGrad, uint navViewGradSize, MenuShownDelegate menuShownDel)
{
	Base::setOnResume(
		[](bool focused)
		{
			if(updateInputDevicesOnResume)
			{
				updateInputDevices();
				EmuControls::updateAutoOnScreenControlVisible();
				updateInputDevicesOnResume = 0;
			}
	});

	Base::setOnFreeCaches(
		[]()
		{
			if(View::defaultFace)
				View::defaultFace->freeCaches();
			if(View::defaultSmallFace)
				View::defaultSmallFace->freeCaches();
		});

	Base::setOnExit(
		[](bool backgrounded)
		{
			Audio::closePcm();
			EmuSystem::pause();
			if(backgrounded)
			{
				EmuSystem::saveAutoState();
				EmuSystem::saveBackupMem();
				Base::dispatchOnFreeCaches();
				if(optionNotificationIcon)
				{
					auto title = CONFIG_APP_NAME " was suspended";
					Base::addNotification(title, title, EmuSystem::fullGameName());
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
			//if(backgrounded)
			//	FsSys::remove("/private/var/mobile/Library/Caches/" CONFIG_APP_ID "/com.apple.opengl/shaders.maps");
			#endif
		});

	Base::Screen::setOnChange(
		[](const Base::Screen &screen, const Base::Screen::Change &change)
		{
			if(change.added())
			{
				logMsg("screen added");
				if(optionShowOnSecondScreen && screen.screens() > 1)
					setEmuViewOnExtraWindow(true);
			}
			else if(change.removed())
			{
				logMsg("screen removed");
				if(extraWin.win && extraWin.win.screen() == screen)
					setEmuViewOnExtraWindow(false);
			}
		});

	Input::setOnDeviceChange(
		[](const Input::Device &dev, const Input::Device::Change &change)
		{
			logMsg("got input dev change");

			if(Base::appIsRunning())
			{
				updateInputDevices();
				EmuControls::updateAutoOnScreenControlVisible();

				if(optionNotifyInputDeviceChange && (change.added() || change.removed()))
				{
					popup.printf(2, 0, "%s #%d %s", dev.name(), dev.enumId() + 1, change.added() ? "connected" : "disconnected");
					mainWin.win.postDraw();
				}
				else if(change.hadConnectError())
				{
					popup.printf(2, 1, "%s had a connection error", dev.name());
					mainWin.win.postDraw();
				}

				#ifdef CONFIG_BLUETOOTH
				if(viewStack.size == 1) // update bluetooth items
					viewStack.top().onShow();
				#endif
			}
			else
			{
				logMsg("delaying input device changes until app resumes");
				updateInputDevicesOnResume = 1;
			}
		});

	Base::registerInstance(CONFIG_APP_ID, argc, argv);
	Base::setAcceptIPC(CONFIG_APP_ID, true);
	Base::setOnInterProcessMessage(
		[](const char *filename)
		{
			logMsg("got IPC: %s", filename);
			handleOpenFileCommand(filename);
		});
	initOptions();
	auto launchGame = parseCmdLineArgs(argc, argv);
	loadConfigFile();
	EmuSystem::onOptionsLoaded();
	Base::setIdleDisplayPowerSave(optionIdleDisplayPowerSave);
	applyOSNavStyle(false);
	Audio::init();

	#ifdef EMU_FRAMEWORK_BEST_COLOR_MODE_OPTION
	Gfx::init(optionBestColorModeHint ? 24 : 16);
	#else
	Gfx::init();
	#endif

	auto compiled = Gfx::texAlphaProgram.compile();
	compiled |= Gfx::noTexProgram.compile();
	compiled |= View::compileGfxPrograms();
	if(compiled)
		Gfx::autoReleaseShaderCompiler();
	if(!optionDitherImage.isConst)
	{
		Gfx::setDither(optionDitherImage);
	}

	#if defined CONFIG_BASE_ANDROID
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

	View::defaultFace = ResourceFace::loadSystem();
	assert(View::defaultFace);
	View::defaultSmallFace = ResourceFace::create(View::defaultFace);

	#ifdef CONFIG_INPUT_ANDROID_MOGA
	if(optionMOGAInputSystem)
		Input::initMOGA(false);
	#endif
	updateInputDevices();
	EmuSystem::audioFramesPerVideoFrame = optionSoundRate / (EmuSystem::vidSysIsPAL() ? 50 : 60);
	EmuSystem::configAudioRate();

	emuView.inputView = &emuInputView;
	emuView.layer = &emuVideoLayer;
	emuVideoLayer.init();
	emuVideoLayer.setLinearFilter(optionImgFilter);
	emuVideoLayer.vidImgOverlay.setEffect(optionOverlayEffect);
	emuVideoLayer.vidImgOverlay.intensity = optionOverlayEffectLevel/100.;

	viewNav.init(View::defaultFace, View::needsBackControl ? &getAsset(ASSET_ARROW) : nullptr,
			!Config::envIsPS3 ? &getAsset(ASSET_GAME_ICON) : nullptr, navViewGrad, navViewGradSize);
	viewNav.setRightBtnActive(false);

	auto winConf = Gfx::makeWindowConfig();
	if(menuShownDel)
	{
		mainWin.win.init(winConf);
		mainInitWindowCommon(mainWin.win);
		menuShownDel(mainWin.win);
	}
	else
	{
		mainWin.win.init(winConf);
		mainInitWindowCommon(mainWin.win);
	}

	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	emuVideoLayer.vidImgEffect.setBitDepth(optionBestColorModeHint ? 24 : 16);
	emuVideoLayer.setEffect(optionImgEffect);
	#endif

	mainWin.win.setOnInputEvent(
		[](Base::Window &win, const Input::Event &e)
		{
			handleInputEvent(win, e);
		});

	mainWin.win.setOnFocusChange(
		[](Base::Window &win, uint in)
		{
			mainWin.focused = in;
			onFocusChange(in);
		});

	mainWin.win.setOnDragDrop(
		[](Base::Window &win, const char *filename)
		{
			logMsg("got DnD: %s", filename);
			handleOpenFileCommand(filename);
		});

	mainWin.win.setOnSurfaceChange(
		[](Base::Window &win, Base::Window::SurfaceChange change)
		{
			if(change.resized())
			{
				updateWindowViewport(mainWin, change);
				emuView.setViewRect(mainWin.viewport().bounds(), mainWin.projectionPlane);
				placeElements();
			}
		});

	mainWin.win.setOnDraw(
		[](Base::Window &win, Base::Window::DrawParams params)
		{
			auto frameTime = params.frameTime();
			if(Gfx::setCurrentWindow(&win) || params.wasResized())
			{
				Gfx::setViewport(mainWin.viewport());
				Gfx::setProjectionMatrix(mainWin.projectionMat);
			}
			Gfx::clear();
			if(EmuSystem::isActive() && emuView.layer)
			{
				win.postDraw();
				runEmuFrame(frameTime, fastForwardActive);
			}
			else
			{
				emuView.draw(frameTime);
				if(modalViewController.hasView())
					modalViewController.draw(frameTime);
				else if(menuViewIsActive)
					viewStack.draw(frameTime);
				popup.draw();
				Gfx::setClipRect(false);
				Gfx::presentWindow(win);
			}
		});

	if(optionShowOnSecondScreen && Base::Screen::screens() > 1)
	{
		setEmuViewOnExtraWindow(true);
	}

	if(launchGame)
	{
		handleOpenFileCommand(launchGame);
	}
}

void mainInitWindowCommon(Base::Window &win)
{
	updateProjection(mainWin, makeViewport(win));

	win.setTitle(CONFIG_APP_NAME);

	setupFont();
	popup.init();
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	initVControls();
	EmuControls::updateVControlImg();
	vController.menuBtnSpr.init(&getAsset(ASSET_MENU));
	vController.ffBtnSpr.init(&getAsset(ASSET_FAST_FORWARD));
	#endif

	//logMsg("setting up view stack");
	modalViewController.init(win);
	modalViewController.onRemoveView() =
		[]()
		{
			if(!menuViewIsActive)
			{
				startGameFromMenu();
			}
		};
	viewStack.init(win);
	if(optionTitleBar)
	{
		//logMsg("title bar on");
		viewStack.setNavView(&viewNav);
	}
	//logMsg("setting menu orientation");
	win.setValidOrientations(optionMenuOrientation, false);
	win.setAcceptDnd(1);

	#if defined CONFIG_BASE_ANDROID
	if(!Base::apkSignatureIsConsistent())
	{
		auto &ynAlertView = *new YesNoAlertView{win};
		ynAlertView.init("Warning: App has been modified by 3rd party, use at your own risk", 0);
		ynAlertView.onNo() =
			[](const Input::Event &e)
			{
				Base::exit();
			};
		modalViewController.pushAndShow(ynAlertView);
	}
	#endif

	placeElements();
	initMainMenu(win);
	auto &mMenu = mainMenu();
	viewStack.pushAndShow(mMenu);

	win.show();
	win.postDraw();
}

void handleInputEvent(Base::Window &win, const Input::Event &e)
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
	else if(modalViewController.hasView())
		modalViewController.inputEvent(e);
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
				else if(e.map == Input::Event::MAP_SYSTEM && (Config::envIsAndroid || Config::envIsLinux))
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

void handleOpenFileCommand(const char *filename)
{
	auto type = FsSys::fileType(filename);
	logMsg("%d %s", type, filename);
	if(type == Fs::TYPE_DIR)
	{
		logMsg("changing to dir %s from external command", filename);
		restoreMenuFromGame();
		FsSys::chdir(filename);
		viewStack.popToRoot();
		auto &fPicker = *new EmuFilePicker{mainWin.win};
		fPicker.init(Input::keyInputIsPresent(), false);
		viewStack.pushAndShow(fPicker, false);
		return;
	}
	if(type != Fs::TYPE_FILE || !EmuFilePicker::defaultFsFilter(filename, type))
	{
		logMsg("unrecognized file type");
		return;
	}
	FsSys::PathString dirnameTemp, basenameTemp;
	auto dir = string_dirname(filename, dirnameTemp);
	auto file = string_basename(filename, basenameTemp);
	FsSys::chdir(dir);
	logMsg("opening file %s in dir %s from external command", file, dir);
	restoreMenuFromGame();
	viewStack.popToRoot();
	if(modalViewController.hasView())
		modalViewController.pop();
	GameFilePicker::onSelectFile(file, Input::Event{});
}

void placeEmuViews()
{
	emuView.place();
	emuView2.place();
}

void placeElements()
{
	logMsg("placing app elements");
	GuiTable1D::setDefaultXIndent(mainWin.projectionPlane);
	popup.place(mainWin.projectionPlane);
	placeEmuViews();
	viewStack.place(mainWin.viewport().bounds(), mainWin.projectionPlane);
	modalViewController.place(mainWin.viewport().bounds(), mainWin.projectionPlane);
}

static void updateProjection(AppWindowData &appWin, const Gfx::Viewport &viewport)
{
	appWin.projectionMat = Gfx::Mat4::makePerspectiveFovRH(M_PI/4.0, viewport.realAspectRatio(), 1.0, 100.);
	appWin.projectionPlane = Gfx::ProjectionPlane::makeWithMatrix(viewport, appWin.projectionMat);
}

static Gfx::Viewport makeViewport(const Base::Window &win)
{
	if((int)optionViewportZoom != 100)
	{
		auto viewRect = win.contentBounds();
		IG::Point2D<int> viewCenter {(int)viewRect.xSize()/2, (int)viewRect.ySize()/2};
		viewRect -= viewCenter;
		viewRect.x = viewRect.x * optionViewportZoom/100.;
		viewRect.y = viewRect.y * optionViewportZoom/100.;
		viewRect.x2 = viewRect.x2 * optionViewportZoom/100.;
		viewRect.y2 = viewRect.y2 * optionViewportZoom/100.;
		viewRect += viewCenter;
		return Gfx::Viewport::makeFromWindow(win, viewRect);
	}
	else
		return Gfx::Viewport::makeFromWindow(win);
}
