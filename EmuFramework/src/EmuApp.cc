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
EmuView emuView(mainWin.win);
ViewStack viewStack;
MsgPopup popup;
StackAllocator menuAllocator;
BasicViewController modalViewController;
uint8 modalViewStorage[2][1024] __attribute__((aligned)) {{0}};
uint modalViewStorageIdx = 0;
WorkDirStack<1> workDirStack;
static bool trackFPS = 0;
static TimeSys prevFrameTime;
static uint frameCount = 0;
const char *launchGame = nullptr;
static bool updateInputDevicesOnResume = false;
InputManagerView *imMenu = nullptr;
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

static void updateProjection(AppWindowData &appWin, const Gfx::Viewport &viewport, bool updateGUI);
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

void EmuNavView::draw(const Base::Window &win)
{
	using namespace Gfx;
	setBlendMode(0);
	noTexProgram.use(View::projP.makeTranslate());
	bg.draw();
	setColor(COLOR_WHITE);
	texAlphaProgram.use();
	text.draw(View::projP.alignToPixel(View::projP.unProjectRect(viewRect).pos(C2DO)), C2DO);
	if(leftSpr.image())
	{
		if(leftBtnActive)
		{
			setColor(COLOR_WHITE);
			setBlendMode(BLEND_MODE_ALPHA);
			auto trans = View::projP.makeTranslate(View::projP.unProjectRect(leftBtn).pos(C2DO));
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
			rightSpr.useDefaultProgram(IMG_MODE_MODULATE, View::projP.makeTranslate(View::projP.unProjectRect(rightBtn).pos(C2DO)));
			rightSpr.draw();
		}
	}
}

static void animateViewportStep(AppWindowData &appWin)
{
	auto &viewportDelta = appWin.viewportDelta;
	auto &viewport = appWin.viewport;
	auto &win = appWin.win;
	for(auto &d : viewportDelta)
	{
		d.update(1);
	}
	//logMsg("animated viewport: %d:%d:%d:%d",
	//	viewportDelta[0].now(), viewportDelta[1].now(), viewportDelta[2].now(), viewportDelta[3].now());
	auto v = Gfx::Viewport::makeFromWindow(win, {viewportDelta[0].now(), viewportDelta[1].now(), viewportDelta[2].now(), viewportDelta[3].now()});
	viewport = v;
	updateProjection(appWin, v, true);
	win.setAsDrawTarget();
	Gfx::setViewport(win, v);
	Gfx::setProjectionMatrix(appWin.projectionMat);
}

static Base::Screen::OnFrameDelegate animateViewport
{
	[](Base::Screen &screen, Base::FrameTimeBase frameTime)
	{
		auto &winData = mainWin;
		winData.win.setNeedsDraw(true);
		animateViewportStep(winData);
		placeElements(winData.viewport);
		if(!winData.viewportDelta[0].isComplete())
		{
			screen.addOnFrameDelegate(animateViewport);
			screen.postFrame();
		}
	}
};

void setEmuViewOnExtraWindow(bool on)
{
	if(on && !extraWin.win)
	{
		logMsg("setting emu view on extra window");
		extraWin.win.init({0, 0}, {0, 0},
			[](Base::Window &win)
			{
				logMsg("init extra window");
				emuView.videoWin = &win;
				extraWin.viewport = makeViewport(win);
				updateProjection(extraWin, extraWin.viewport, false);
				win.setTitle("Test Window");
				win.show();
			});

		extraWin.win.setOnSurfaceChange(
			[](Base::Window &win, bool didResize)
			{
				if(didResize)
				{
					logMsg("view resize for extra window");
					extraWin.viewport = makeViewport(win);
					updateProjection(extraWin, extraWin.viewport, false);
				}
				Gfx::setViewport(extraWin.win, extraWin.viewport);
				Gfx::setProjectionMatrix(extraWin.projectionMat);
				emuView.place();
			});

		extraWin.win.setOnDraw(
			[](Base::Window &win, Base::FrameTimeBase frameTime)
			{
				Gfx::clear();
				if(EmuSystem::isActive() || EmuSystem::isStarted())
				{
					emuView.draw(frameTime, win);
				}
			});

		extraWin.win.setOnInputEvent(
			[](Base::Window &win, const Input::Event &e)
			{
				if(!e.isPointer())
					handleInputEvent(win, e);
			});
	}
	else if(!on && extraWin.win)
	{
		logMsg("setting emu view on main window");
		extraWin.win.deinit();
		emuView.videoWin = &mainWin.win;
		emuView.place();
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
		emuView.videoWin->setPresentInterval(optionFrameSkip + 1);
	logMsg("running game");
	menuViewIsActive = 0;
	viewNav.setRightBtnActive(1);
	//logMsg("touch control state: %d", touchControlsAreOn);
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	vController.resetInput();
	#endif
	Base::mainWindow().setValidOrientations(optionGameOrientation);
	commonInitInput();
	emuView.ffGuiKeyPush = emuView.ffGuiTouch = 0;
	popup.clear();
	Input::setKeyRepeat(false);
	EmuControls::setupVolKeysInGame();
	emuView.videoWin->screen().setRefreshRate(EmuSystem::vidSysIsPAL() ? 50 : 60);
	EmuSystem::start();
	mainWin.win.postDraw();
	if(extraWin.win)
		extraWin.win.postDraw();
	emuView.place();
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
		Base::mainWindow().setPresentInterval(1);
	Base::mainWindow().setValidOrientations(optionMenuOrientation);
	Input::setKeyRepeat(true);
	Input::setHandleVolumeKeys(false);
	if(!optionRememberLastMenu)
		viewStack.popToRoot();
	Base::mainScreen().setRefreshRate(Base::Screen::REFRESH_RATE_DEFAULT);
	mainWin.win.postDraw();
	if(extraWin.win)
		extraWin.win.postDraw();
	viewStack.show();
}

static void onFocusChange(Base::Window &win, uint in)
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
		emuView.videoWin->postDraw();
	}
}

static void parseCmdLineArgs(int argc, char** argv)
{
	if(argc < 2)
	{
		return;
	}
	launchGame = argv[1];
	logMsg("starting game from command line: %s", launchGame);
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
					emuView.videoWin->postDraw();
				}
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
				if(screen.screens() > 1)
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
					Base::mainWindow().postDraw();
				}
				else if(change.hadConnectError())
				{
					popup.printf(2, 1, "%s had a connection error", dev.name());
					Base::mainWindow().postDraw();
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
	EmuSystem::initOptions();
	parseCmdLineArgs(argc, argv);
	loadConfigFile();
	#ifdef EMU_FRAMEWORK_BEST_COLOR_MODE_OPTION
	Base::Window::setPixelBestColorHint(optionBestColorModeHint);
	#endif
	EmuSystem::onOptionsLoaded();
	Base::setIdleDisplayPowerSave(optionIdleDisplayPowerSave);
	applyOSNavStyle(false);
	Audio::init();
	Gfx::init();

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

	emuView.disp.init();
	#if defined CONFIG_BASE_ANDROID && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE
	emuView.disp.flags = Gfx::Sprite::HINT_NO_MATRIX_TRANSFORM;
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	emuView.vidImgEffect.setEffect(optionImgEffect);
	#endif
	emuView.vidImgOverlay.setEffect(optionOverlayEffect);
	emuView.vidImgOverlay.intensity = optionOverlayEffectLevel/100.;

	viewNav.init(View::defaultFace, View::needsBackControl ? &getAsset(ASSET_ARROW) : nullptr,
			!Config::envIsPS3 ? &getAsset(ASSET_GAME_ICON) : nullptr, navViewGrad, navViewGradSize);
	viewNav.setRightBtnActive(false);

	if(menuShownDel)
	{
		mainWin.win.init({0, 0}, {0, 0},
			[&menuShownDel](Base::Window &win)
			{
				mainInitWindowCommon(win);
				menuShownDel(win);
			});
	}
	else
	{
		mainWin.win.init({0, 0}, {0, 0},
			[](Base::Window &win)
			{
				mainInitWindowCommon(win);
			});
	}

	mainWin.win.setOnInputEvent(
		[](Base::Window &win, const Input::Event &e)
		{
			handleInputEvent(win, e);
		});

	mainWin.win.setOnFocusChange(
		[](Base::Window &win, uint in)
		{
			onFocusChange(win, in);
		});

	mainWin.win.setOnDragDrop(
		[](Base::Window &win, const char *filename)
		{
			logMsg("got DnD: %s", filename);
			handleOpenFileCommand(filename);
		});

	mainWin.win.setOnSurfaceChange(
		[](Base::Window &win, bool didResize)
		{
			auto &viewport = mainWin.viewport;
			if(didResize)
			{
				logMsg("view resize");
				auto &viewportDelta = mainWin.viewportDelta;
				auto &lastWindowSize = mainWin.lastWindowSize;
				auto oldViewport = viewport;
				auto oldViewportAR = (oldViewport.width() && oldViewport.height()) ? oldViewport.aspectRatio() : 0;
				auto newViewport = makeViewport(win);
				if(newViewport != oldViewport && lastWindowSize == win.size() && win.shouldAnimateContentBoundsChange())
				{
					logMsg("viewport changed with same window size");
					auto type = INTERPOLATOR_TYPE_EASEINOUTQUAD;
					int time = 10;
					viewportDelta[0].set(oldViewport.bounds().x, newViewport.bounds().x, type, time);
					viewportDelta[1].set(oldViewport.bounds().y, newViewport.bounds().y, type, time);
					viewportDelta[2].set(oldViewport.bounds().x2, newViewport.bounds().x2, type, time);
					viewportDelta[3].set(oldViewport.bounds().y2, newViewport.bounds().y2, type, time);
					if(!win.screen().containsOnFrameDelegate(animateViewport))
						win.screen().addOnFrameDelegate(animateViewport);
					win.screen().postFrame();
					animateViewportStep(mainWin);
				}
				else
				{
					win.screen().removeOnFrameDelegate(animateViewport);
					viewport = newViewport;
					if(oldViewportAR != newViewport.aspectRatio())
					{
						updateProjection(mainWin, newViewport, true);
					}
				}
				lastWindowSize = win.size();
				logMsg("done resize");
			}
			Gfx::setViewport(win, viewport);
			Gfx::setProjectionMatrix(mainWin.projectionMat);
			if(didResize)
			{
				placeElements(mainWin.viewport);
			}
		});

	mainWin.win.setOnDraw(
		[](Base::Window &win, Base::FrameTimeBase frameTime)
		{
			Gfx::clear();
			emuView.draw(frameTime, win);
			if(likely(EmuSystem::isActive()))
			{
				if(trackFPS)
				{
					if(frameCount == 119)
					{
						auto now = TimeSys::now();
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

			if(modalViewController.hasView())
				modalViewController.draw(frameTime);
			else if(menuViewIsActive)
				viewStack.draw(frameTime);
			popup.draw();
			Gfx::setClipRect(false);
		});

	if(Base::Screen::screens() > 1)
	{
		setEmuViewOnExtraWindow(true);
	}
}

void mainInitWindowCommon(Base::Window &win)
{
	mainWin.lastWindowSize = win.size();
	mainWin.viewport = makeViewport(win);
	updateProjection(mainWin, mainWin.viewport, true);

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
		auto &ynAlertView = *allocModalView<YesNoAlertView>(win);
		ynAlertView.init("Warning: App has been modified by 3rd party, use at your own risk", 0);
		ynAlertView.onNo() =
			[](const Input::Event &e)
			{
				Base::exit();
			};
		modalViewController.pushAndShow(ynAlertView);
	}
	#endif

	placeElements(mainWin.viewport);
	initMainMenu(win);
	auto &mMenu = mainMenu();
	viewStack.pushAndShow(mMenu);

	win.show();

	if(launchGame)
	{
		FsSys::chdir(Base::appPath);
		handleOpenFileCommand(launchGame);
	}
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
		auto &fPicker = *menuAllocator.allocNew<EmuFilePicker>(Base::mainWindow());
		fPicker.init(Input::keyInputIsPresent(), false);
		viewStack.useNavView = 0;
		viewStack.pushAndShow(fPicker, &menuAllocator);
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
	if(modalViewController.hasView())
		modalViewController.pop();
	GameFilePicker::onSelectFile(file, Input::Event{});
}

void placeElements(const Gfx::Viewport &viewport)
{
	GuiTable1D::setDefaultXIndent();
	popup.place();
	emuView.place();
	viewStack.place(viewport.bounds());
	modalViewController.place(viewport.bounds());
}

static void updateProjection(AppWindowData &appWin, const Gfx::Viewport &viewport, bool updateGUI)
{
	appWin.projectionMat = Gfx::Mat4::makePerspectiveFovRH(M_PI/4.0, viewport.realAspectRatio(), 1.0, 100.);
	if(updateGUI)
		View::projP = Gfx::ProjectionPlane::makeWithMatrix(viewport, appWin.projectionMat);
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
