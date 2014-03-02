#include <data-type/image/png/sys.hh>
#include "EmuApp.hh"
#include "EmuSystem.hh"
#include "EmuOptions.hh"
#include "FilePicker.hh"
#include "ConfigFile.hh"
#include <EmuView.hh>
#include <gui/AlertView.hh>
#include <cmath>

bool menuViewIsActive = true;
EmuNavView viewNav;
Base::Window mainWin, secondWin;
EmuView emuView(mainWin);
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
static Gfx::Viewport viewport;
static Gfx::Mat4 projectionMat;
static IG::Point2D<int> lastWindowSize;
static TimedInterpolator<int> viewportDelta[4];

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

static void updateProjection(const Gfx::Viewport &viewport);
static Gfx::Viewport makeViewport(const Base::Window &win);

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
		Base::mainWindow().setVideoInterval((int)optionFrameSkip + 1);
	logMsg("running game");
	menuViewIsActive = 0;
	viewNav.setRightBtnActive(1);
	//logMsg("touch control state: %d", touchControlsAreOn);
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	vController.resetInput();
	#endif
	Base::mainWindow().setValidOrientations(optionGameOrientation);
	Base::mainWindow().dispatchResize(); // TODO: only do this if needed
	commonInitInput();
	emuView.ffGuiKeyPush = emuView.ffGuiTouch = 0;

	popup.clear();
	Input::setKeyRepeat(false);
	EmuControls::setupVolKeysInGame();
	/*if(optionFrameSkip != 0 && soundRateDelta != 0)
	{
		logMsg("reset sound rate delta");
		soundRateDelta = 0;
		audio_setPcmRate(audio_pPCM.rate);
	}*/
	Base::mainScreen().setRefreshRate(EmuSystem::vidSysIsPAL() ? 50 : 60);
	EmuSystem::start();
	Base::mainWindow().postDraw();

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
		Base::mainWindow().setVideoInterval(1);
	Base::mainWindow().setValidOrientations(optionMenuOrientation);
	Base::mainWindow().dispatchResize(); // TODO: only do this if needed
	Input::setKeyRepeat(true);
	Input::setHandleVolumeKeys(false);
	if(!optionRememberLastMenu)
		viewStack.popToRoot();
	Base::mainScreen().setRefreshRate(Base::Screen::REFRESH_RATE_DEFAULT);
	Base::mainWindow().postDraw();
	viewStack.show();
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

void mainInitCommon(int argc, char** argv)
{
	Base::registerInstance(CONFIG_APP_ID, argc, argv);
	Base::setAcceptIPC(CONFIG_APP_ID, true);
	initOptions();
	EmuSystem::initOptions();
	parseCmdLineArgs(argc, argv);
	loadConfigFile();
	#ifdef USE_BEST_COLOR_MODE_OPTION
	Base::Window::setPixelBestColorHint(optionBestColorModeHint);
	#endif
	EmuSystem::onOptionsLoaded();
	doOrAbort(Audio::init());
	mainWin.init({0, 0}, {0, 0});
	Base::setIdleDisplayPowerSave(optionIdleDisplayPowerSave);
	applyOSNavStyle(false);
}

void mainInitWindowCommon(Base::Window &win, const Gfx::LGradientStopDesc *navViewGrad, uint navViewGradSize)
{
	lastWindowSize = win.size();
	viewport = makeViewport(win);
	Gfx::setViewport(win, viewport);
	updateProjection(viewport);
	#ifdef CONFIG_BASE_MULTI_WINDOW
	if(win != mainWin)
	{
		logMsg("init for 2nd window");
		win.setTitle("Test Window");
		win.show();
		return;
	}
	#endif

	auto compiled = Gfx::texAlphaProgram.compile();
	compiled |= Gfx::noTexProgram.compile();
	compiled |= View::compileGfxPrograms();
	if(compiled)
		Gfx::autoReleaseShaderCompiler();

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

	//logMsg("setting up view stack");
	viewNav.init(View::defaultFace, View::needsBackControl ? &getAsset(ASSET_ARROW) : nullptr,
			!Config::envIsPS3 ? &getAsset(ASSET_GAME_ICON) : nullptr, navViewGrad, navViewGradSize);
	viewNav.setRightBtnActive(false);
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
		modalViewController.pushAndShow(ynAlertView);
	}
	#endif

	win.dispatchResize();
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

namespace Base
{

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
		Base::mainWindow().postDraw();
	}
}

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
	if(backgrounded)
		FsSys::remove("/private/var/mobile/Library/Caches/" CONFIG_APP_ID "/com.apple.opengl/shaders.maps");
	#endif
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
			Base::mainWindow().postDraw();
		}
	}
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
}

}

static void placeElements(const Gfx::Viewport &viewport)
{
	GuiTable1D::setDefaultXIndent();
	popup.place();
	emuView.place();
	viewStack.place(viewport.bounds());
	modalViewController.place(viewport.bounds());
}

static void animateViewportStep(const Base::Window &win)
{
	for(auto &d : viewportDelta)
	{
		d.update(1);
	}
	logMsg("animated viewport: %d:%d:%d:%d",
		viewportDelta[0].now(), viewportDelta[1].now(), viewportDelta[2].now(), viewportDelta[3].now());
	auto v = Gfx::Viewport::makeFromWindow(win, {viewportDelta[0].now(), viewportDelta[1].now(), viewportDelta[2].now(), viewportDelta[3].now()});
	viewport = v;
	Gfx::setViewport(win, v);
	updateProjection(v);
	placeElements(v);
}

static Base::Screen::OnFrameDelegate animateViewport
{
	[](Base::Screen &screen, Base::FrameTimeBase frameTime)
	{
		auto &win = Base::mainWindow();
		win.setNeedsDraw(true);
		animateViewportStep(win);
		if(!viewportDelta[0].isComplete())
		{
			screen.addOnFrameDelegate(animateViewport);
			screen.postFrame();
		}
	}
};

static void updateProjection(const Gfx::Viewport &viewport)
{
	projectionMat = Gfx::Mat4::makePerspectiveFovRH(M_PI/4.0, viewport.realAspectRatio(), 1.0, 100.);
	Gfx::setProjectionMatrix(projectionMat);
	View::projP = Gfx::ProjectionPlane::makeWithMatrix(viewport, projectionMat);
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
		return viewport.makeFromWindow(win, viewRect);
	}
	else
		return viewport.makeFromWindow(win);
}

namespace Base
{

void onViewChange(Base::Window &win)
{
	logMsg("view change");
	#ifdef CONFIG_BASE_MULTI_WINDOW
	if(win != mainWin)
	{
		bug_exit("TODO");
		logMsg("resize for 2nd window");
		//applyViewport(win);
		return;
	}
	#endif
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
		if(!mainScreen().containsOnFrameDelegate(animateViewport))
			mainScreen().addOnFrameDelegate(animateViewport);
		mainScreen().postFrame();
		animateViewportStep(win);
	}
	else
	{
		mainScreen().removeOnFrameDelegate(animateViewport);
		viewport = newViewport;
		Gfx::setViewport(win, newViewport);
		if(oldViewportAR != newViewport.aspectRatio())
		{
			updateProjection(newViewport);
		}
		placeElements(newViewport);
	}
	lastWindowSize = win.size();
	logMsg("done view change");
}

void onSetAsDrawTarget(Base::Window &win)
{
	Gfx::setViewport(win, viewport);
	Gfx::setProjectionMatrix(projectionMat);
}

void onDraw(Base::Window &win, FrameTimeBase frameTime)
{
	#ifdef CONFIG_BASE_MULTI_WINDOW
	if(win != mainWin)
	{
		if(EmuSystem::isActive())
		{
			win.postDraw();
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
}

}
