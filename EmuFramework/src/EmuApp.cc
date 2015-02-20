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

#include <imagine/data-type/image/sys.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/FilePicker.hh>
#include <emuframework/ConfigFile.hh>
#include <emuframework/EmuView.hh>
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
BluetoothAdapter *bta{};
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

static Gfx::PixmapTexture assetBuffImg[sizeofArray(assetFilename)];

static void updateProjection(AppWindowData &appWin, const Gfx::Viewport &viewport);
static Gfx::Viewport makeViewport(const Base::Window &win);
void mainInitWindowCommon(Base::Window &win);

Gfx::PixmapTexture &getAsset(AssetID assetID)
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

Gfx::PixmapTexture *getCollectTextCloseAsset()
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

void postDrawToEmuWindows()
{
	emuWin->win.postDraw();
}

static void drawEmuVideo()
{
	if(emuView.layer)
		emuView.draw();
	else if(emuView2.layer)
		emuView2.draw();
	popup.draw();
	Gfx::setClipRect(false);
	Gfx::presentWindow(emuWin->win);
}

void updateAndDrawEmuVideo()
{
	emuVideo.vidImg.write(0, emuVideo.vidPix, {}, emuVideo.vidPixAlign);
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
			TextureSampler::bindDefaultNearestMipClampSampler();
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
			TextureSampler::bindDefaultNearestMipClampSampler();
			rightSpr.useDefaultProgram(IMG_MODE_MODULATE, projP.makeTranslate(projP.unProjectRect(rightBtn).pos(C2DO)));
			rightSpr.draw();
		}
	}
}

void startViewportAnimation(AppWindowData &winData)
{
	auto oldViewport = winData.viewport();
	auto newViewport = makeViewport(winData.win);
	winData.animatedViewport.start(winData.win, oldViewport, newViewport);
}

static void updateWindowViewport(AppWindowData &winData, Base::Window::SurfaceChange change)
{
	if(change.surfaceResized())
	{
		winData.animatedViewport.cancel();
		updateProjection(winData, makeViewport(winData.win));
	}
	else if(change.contentRectResized())
	{
		startViewportAnimation(winData);
	}
	else if(change.customViewportResized())
	{
		updateProjection(winData, winData.animatedViewport.viewport());
	}
}

static Base::Screen::OnFrameDelegate frameUpdate =
	[](Base::Screen &screen, Base::Screen::FrameParams params)
	{
		commonUpdateInput();

		if(unlikely(fastForwardActive))
		{
			EmuSystem::runFrameOnDraw = true;
			iterateTimes((uint)optionFastForwardSpeed, i)
			{
				EmuSystem::runFrame(false, false, false);
			}
		}
		else
		{
			int framesToSkip = EmuSystem::setupFrameSkip(optionFrameSkip, params.frameTime());
			if(framesToSkip >= 0)
			{
				EmuSystem::runFrameOnDraw = true;
				bool renderAudio = optionSound;
				iterateTimes(framesToSkip, i)
				{
					EmuSystem::runFrame(false, false, renderAudio);
				}
			}
		}
		postDrawToEmuWindows();
		screen.postOnFrame(params.thisOnFrame());
	};

static void startEmulation()
{
	EmuSystem::start();
	emuWin->win.screen()->addOnFrame(frameUpdate);
}

static void pauseEmulation()
{
	EmuSystem::pause();
	emuWin->win.screen()->removeOnFrame(frameUpdate);
}

void closeGame(bool allowAutosaveState)
{
	EmuSystem::closeGame();
	emuWin->win.screen()->removeOnFrame(frameUpdate);
}

static void drawEmuFrame()
{
	if(EmuSystem::runFrameOnDraw)
	{
		bool renderAudio = optionSound;
		EmuSystem::runFrame(true, true, renderAudio);
		EmuSystem::runFrameOnDraw = false;
	}
	else
	{
		drawEmuVideo();
	}
}

static bool allWindowsAreFocused()
{
	return mainWin.focused && (!extraWin.win || extraWin.focused);
}

static void onFocusChange(uint in)
{
	if(!menuViewIsActive)
	{
		if(in && EmuSystem::isPaused())
		{
			logMsg("resuming emulation due to window focus");
			#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
			vController.resetInput();
			#endif
			startEmulation();
			postDrawToEmuWindows();
		}
		else if(optionPauseUnfocused && !EmuSystem::isPaused() && !allWindowsAreFocused())
		{
			logMsg("pausing emulation with all windows unfocused");
			pauseEmulation();
			postDrawToEmuWindows();
		}
	}
}

void setEmuViewOnExtraWindow(bool on)
{
	if(on && !extraWin.win)
	{
		logMsg("setting emu view on extra window");
		Base::WindowConfig winConf;
		if(Base::Screen::screens() > 1)
		{
			winConf.setScreen(*Base::Screen::screen(1));
		}

		winConf.setOnSurfaceChange(
			[](Base::Window &win, Base::Window::SurfaceChange change)
			{
				if(change.resized())
				{
					logMsg("view resize for extra window");
					Gfx::bind();
					updateProjection(extraWin, makeViewport(win));
					emuView2.setViewRect(extraWin.viewport().bounds(), extraWin.projectionPlane);
					emuView2.place();
				}
			});

		winConf.setOnDraw(
			[](Base::Window &win, Base::Window::DrawParams params)
			{
				Gfx::updateCurrentWindow(win, params, extraWin.viewport(), extraWin.projectionMat);
				Gfx::clear();
				if(EmuSystem::isActive())
				{
					drawEmuFrame();
				}
				else
				{
					emuView2.draw();
					Gfx::setClipRect(false);
					Gfx::presentWindow(win);
				}
			});

		winConf.setOnInputEvent(
			[](Base::Window &win, const Input::Event &e)
			{
				if(!e.isPointer())
				{
					Gfx::bind();
					handleInputEvent(win, e);
				}
			});

		winConf.setOnFocusChange(
			[](Base::Window &win, uint in)
			{
				Gfx::bind();
				extraWin.focused = in;
				onFocusChange(in);
			});

		winConf.setOnDismissRequest(
			[](Base::Window &win)
			{
				win.dismiss();
			});

		winConf.setOnDismiss(
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

		Gfx::initWindow(extraWin.win, winConf);
		extraWin.focused = true;
		logMsg("init extra window");
		emuWin = &extraWin;
		if(EmuSystem::isActive() && mainWin.win.screen() != extraWin.win.screen())
		{
			mainWin.win.screen()->removeOnFrame(frameUpdate);
			extraWin.win.screen()->addOnFrame(frameUpdate);
			EmuSystem::resetFrameTime();
		}
		std::swap(emuView.layer, emuView2.layer);
		updateProjection(extraWin, makeViewport(extraWin.win));
		extraWin.win.setTitle(appName());
		extraWin.win.show();
		emuView.place();
		mainWin.win.postDraw();
	}
	else if(!on && extraWin.win)
	{
		if(EmuSystem::isActive() && mainWin.win.screen() != extraWin.win.screen())
		{
			extraWin.win.screen()->removeOnFrame(frameUpdate);
			mainWin.win.screen()->addOnFrame(frameUpdate);
			EmuSystem::resetFrameTime();
		}
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
		emuWin->win.screen()->setFrameInterval(optionFrameSkip + 1);
	logMsg("running game");
	menuViewIsActive = 0;
	viewNav.setRightBtnActive(1);
	emuInputView.resetInput();
	//logMsg("touch control state: %d", touchControlsAreOn);
	Gfx::setWindowValidOrientations(mainWin.win, optionGameOrientation);
	commonInitInput();
	popup.clear();
	Input::setKeyRepeat(false);
	EmuControls::setupVolKeysInGame();
	emuWin->win.screen()->setRefreshRate(EmuSystem::vidSysIsPAL() ? 50 : 60);
	startEmulation();
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
	pauseEmulation();
	if(!optionFrameSkip.isConst)
		mainWin.win.screen()->setFrameInterval(1);
	Gfx::setWindowValidOrientations(mainWin.win, optionMenuOrientation);
	Input::setKeyRepeat(true);
	Input::setHandleVolumeKeys(false);
	if(!optionRememberLastMenu)
		viewStack.popToRoot();
	mainWin.win.screen()->setRefreshRate(Base::Screen::REFRESH_RATE_DEFAULT);
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
			Gfx::bind();
			if(updateInputDevicesOnResume)
			{
				updateInputDevices();
				EmuControls::updateAutoOnScreenControlVisible();
				updateInputDevicesOnResume = false;
			}
			if(!menuViewIsActive && focused && EmuSystem::isPaused())
			{
				logMsg("resuming emulation due to app resume");
				#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
				vController.resetInput();
				#endif
				startEmulation();
				postDrawToEmuWindows();
			}
	});

	Base::setOnFreeCaches(
		[]()
		{
			Gfx::bind();
			if(View::defaultFace)
				View::defaultFace->freeCaches();
			if(View::defaultSmallFace)
				View::defaultSmallFace->freeCaches();
		});

	Base::setOnExit(
		[](bool backgrounded)
		{
			Audio::closePcm();
			Gfx::bind();
			if(backgrounded)
			{
				pauseEmulation();
				EmuSystem::saveAutoState();
				EmuSystem::saveBackupMem();
				Base::dispatchOnFreeCaches();
				if(optionNotificationIcon)
				{
					//auto title = CONFIG_APP_NAME " was suspended";
					auto title = string_makePrintf<64>("%s was suspended", appName());
					Base::addNotification(title.data(), title.data(), EmuSystem::fullGameName());
				}
			}
			else
			{
				closeGame();
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
		[](const Base::Screen &screen, Base::Screen::Change change)
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
				if(extraWin.win && *extraWin.win.screen() == screen)
					setEmuViewOnExtraWindow(false);
			}
		});

	Input::setOnDeviceChange(
		[](const Input::Device &dev, Input::Device::Change change)
		{
			logMsg("got input dev change");

			if(Base::appIsRunning())
			{
				Gfx::bind();
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
				updateInputDevicesOnResume = true;
			}
		});

	Base::registerInstance(appID(), argc, argv);
	Base::setAcceptIPC(appID(), true);
	Base::setOnInterProcessMessage(
		[](const char *filename)
		{
			logMsg("got IPC: %s", filename);
			Gfx::bind();
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

	#ifdef __ANDROID__
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
	// TODO: not used yet
	//View::defaultSmallFace = ResourceFace::create(View::defaultFace);

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

	Base::WindowConfig winConf;

	winConf.setOnInputEvent(
		[](Base::Window &win, const Input::Event &e)
		{
			Gfx::bind();
			handleInputEvent(win, e);
		});

	winConf.setOnFocusChange(
		[](Base::Window &win, uint in)
		{
			Gfx::bind();
			mainWin.focused = in;
			onFocusChange(in);
		});

	winConf.setOnDragDrop(
		[](Base::Window &win, const char *filename)
		{
			logMsg("got DnD: %s", filename);
			Gfx::bind();
			handleOpenFileCommand(filename);
		});

	winConf.setOnSurfaceChange(
		[](Base::Window &win, Base::Window::SurfaceChange change)
		{
			if(change.resized())
			{
				Gfx::bind();
				updateWindowViewport(mainWin, change);
				emuView.setViewRect(mainWin.viewport().bounds(), mainWin.projectionPlane);
				placeElements();
			}
		});

	winConf.setOnDraw(
		[](Base::Window &win, Base::Window::DrawParams params)
		{
			Gfx::updateCurrentWindow(win, params, mainWin.viewport(), mainWin.projectionMat);
			Gfx::clear();
			if(EmuSystem::isActive())
			{
				if(emuView.layer)
					drawEmuFrame();
				else
				{
					emuView.draw();
					Gfx::setClipRect(false);
					Gfx::presentWindow(win);
				}
			}
			else
			{
				emuView.draw();
				if(modalViewController.hasView())
					modalViewController.draw();
				else if(menuViewIsActive)
					viewStack.draw();
				popup.draw();
				Gfx::setClipRect(false);
				Gfx::presentWindow(win);
			}
		});

	Gfx::initWindow(mainWin.win, winConf);
	mainInitWindowCommon(mainWin.win);
	if(menuShownDel)
		menuShownDel(mainWin.win);

	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	// TODO: set bit depth based on source texture depth and allow user setting
	//emuVideoLayer.vidImgEffect.setBitDepth(optionBestColorModeHint ? 24 : 16);
	emuVideoLayer.setEffect(optionImgEffect);
	#endif

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

	win.setTitle(appName());

	setupFont();
	popup.init();
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	initVControls();
	EmuControls::updateVControlImg();
	vController.menuBtnSpr.init({}, getAsset(ASSET_MENU));
	vController.ffBtnSpr.init({}, getAsset(ASSET_FAST_FORWARD));
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
	Gfx::setWindowValidOrientations(win, optionMenuOrientation);
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
	TableView::setDefaultXIndent(mainWin.projectionPlane);
	popup.place(emuWin->projectionPlane);
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
