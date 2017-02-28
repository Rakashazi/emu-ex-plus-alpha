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
#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
#include <emuframework/VController.hh>
#endif
#include <imagine/gui/AlertView.hh>
#include <imagine/util/assume.h>
#include <cmath>

static Gfx::Renderer renderer;
AppWindowData mainWin{}, extraWin{};
bool menuViewIsActive = true;
EmuView emuView{{mainWin.win, renderer}}, emuView2{{extraWin.win, renderer}};
EmuVideo emuVideo{renderer};
EmuVideoLayer emuVideoLayer{emuVideo};
EmuInputView emuInputView{{mainWin.win, renderer}};
AppWindowData *emuWin = &mainWin;
ViewStack viewStack{};
MsgPopup popup{};
BasicViewController modalViewController{};
static bool updateInputDevicesOnResume = false;
DelegateFunc<void ()> onUpdateInputDevices{};
#ifdef CONFIG_BLUETOOTH
BluetoothAdapter *bta{};
#endif
#ifdef __ANDROID__
std::unique_ptr<Base::UserActivityFaker> userActivityFaker{};
#endif
static OnMainMenuOptionChanged onMainMenuOptionChanged_{};
FS::PathString lastLoadPath{};
#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
SysVController vController{renderer};
uint pointerInputPlayer = 0;
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

static Gfx::PixmapTexture assetBuffImg[IG::size(assetFilename)];

static void updateProjection(AppWindowData &appWin, const Gfx::Viewport &viewport);
static Gfx::Viewport makeViewport(const Base::Window &win);
void mainInitWindowCommon(Base::Window &win);

Gfx::PixmapTexture &getAsset(Gfx::Renderer &r, AssetID assetID)
{
	assert(assetID < IG::size(assetFilename));
	auto &res = assetBuffImg[assetID];
	if(!res)
	{
		PngFile png;
		auto ec = png.loadAsset(assetFilename[assetID]);
		if(ec)
		{
			bug_exit("couldn't load %s", assetFilename[assetID]);
		}
		res.init(r, png);
	}
	return res;
}

Gfx::PixmapTexture *getCollectTextCloseAsset(Gfx::Renderer &r)
{
	return Config::envIsAndroid ? nullptr : &getAsset(r, ASSET_CLOSE);
}

void postDrawToEmuWindows()
{
	emuWin->win.postDraw();
}

static void drawEmuVideo(Gfx::Renderer &r)
{
	if(emuView.layer)
		emuView.draw();
	else if(emuView2.layer)
		emuView2.draw();
	popup.draw();
	r.setClipRect(false);
	r.presentDrawable(emuWin->drawable);
}

void updateAndDrawEmuVideo()
{
	emuVideo.updateImage();
	drawEmuVideo(renderer);
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

void setCPUNeedsLowLatency(bool needed)
{
	#ifdef __ANDROID__
	if(userActivityFaker)
	{
		if(needed)
			userActivityFaker->start();
		else
			userActivityFaker->stop();
	}
	#endif
}

static Base::Screen::OnFrameDelegate onFrameUpdate
{
	[](Base::Screen::FrameParams params)
	{
		commonUpdateInput();
		if(unlikely(fastForwardActive))
		{
			EmuSystem::runFrameOnDraw = true;
			postDrawToEmuWindows();
			iterateTimes((uint)optionFastForwardSpeed, i)
			{
				EmuSystem::runFrame(false, false, false);
			}
		}
		else
		{
			uint frames = EmuSystem::advanceFramesWithTime(params.timestamp());
			//logDMsg("%d frames elapsed (%fs)", frames, Base::frameTimeBaseToSecsDec(params.frameTimeDiff()));
			if(frames)
			{
				EmuSystem::runFrameOnDraw = true;
				postDrawToEmuWindows();
				const uint maxLateFrameSkip = 6;
				uint maxFrameSkip = optionSkipLateFrames ? maxLateFrameSkip : 0;
				#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
				if(!optionSkipLateFrames)
					maxFrameSkip = optionFrameInterval - 1;
				#endif
				assumeExpr(maxFrameSkip <= maxLateFrameSkip);
				if(frames > 1 && maxFrameSkip)
				{
					uint framesToSkip = frames - 1;
					framesToSkip = std::min(framesToSkip, maxFrameSkip);
					bool renderAudio = optionSound;
					iterateTimes(framesToSkip, i)
					{
						EmuSystem::runFrame(false, false, renderAudio);
					}
				}
			}
		}
		params.readdOnFrame();
	}
};

static void applyFrameRates()
{
	EmuSystem::setFrameTime(EmuSystem::VIDSYS_NATIVE_NTSC,
		optionFrameRate.val ? optionFrameRate.val : emuWin->win.screen()->frameTime());
	EmuSystem::setFrameTime(EmuSystem::VIDSYS_PAL,
		optionFrameRatePAL.val ? optionFrameRatePAL.val : emuWin->win.screen()->frameTime());
	EmuSystem::configFrameTime();
}

static void startEmulation()
{
	setCPUNeedsLowLatency(true);
	EmuSystem::start();
	emuWin->win.screen()->addOnFrameOnce(onFrameUpdate);
}

static void pauseEmulation()
{
	EmuSystem::pause();
	emuWin->win.screen()->removeOnFrame(onFrameUpdate);
	setCPUNeedsLowLatency(false);
}

void closeGame(bool allowAutosaveState)
{
	EmuSystem::closeGame();
	emuWin->win.screen()->removeOnFrame(onFrameUpdate);
	setCPUNeedsLowLatency(false);
}

static void drawEmuFrame(Gfx::Renderer &r)
{
	if(EmuSystem::runFrameOnDraw)
	{
		bool renderAudio = optionSound;
		EmuSystem::runFrame(true, true, renderAudio);
		EmuSystem::runFrameOnDraw = false;
	}
	else
	{
		drawEmuVideo(r);
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
					renderer.restoreBind();
					updateProjection(extraWin, makeViewport(win));
					emuView2.setViewRect(extraWin.viewport().bounds(), extraWin.projectionPlane);
					emuView2.place();
				}
				renderer.updateDrawableForSurfaceChange(extraWin.drawable, change);
			});

		winConf.setOnDraw(
			[](Base::Window &win, Base::Window::DrawParams params)
			{
				renderer.updateCurrentDrawable(extraWin.drawable, win, params, extraWin.viewport(), extraWin.projectionMat);
				renderer.clear();
				if(EmuSystem::isActive())
				{
					drawEmuFrame(renderer);
				}
				else
				{
					emuView2.draw();
					renderer.setClipRect(false);
					renderer.presentDrawable(extraWin.drawable);
				}
				renderer.finishPresentDrawable(extraWin.drawable);
			});

		winConf.setOnInputEvent(
			[](Base::Window &win, Input::Event e)
			{
				if(!e.isPointer())
				{
					renderer.restoreBind();
					handleInputEvent(win, e);
				}
			});

		winConf.setOnFocusChange(
			[](Base::Window &win, uint in)
			{
				renderer.restoreBind();
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
				renderer.setCurrentDrawable({});
				EmuSystem::resetFrameTime();
				logMsg("setting emu view on main window");
				emuWin = &mainWin;
				std::swap(emuView.layer, emuView2.layer);
				emuView.place();
				mainWin.win.postDraw();
				if(EmuSystem::isActive() && mainWin.win.screen() != extraWin.win.screen())
				{
					extraWin.win.screen()->removeOnFrame(onFrameUpdate);
					mainWin.win.screen()->addOnFrame(onFrameUpdate);
					applyFrameRates();
				}
			});

		renderer.initWindow(extraWin.win, winConf);
		extraWin.focused = true;
		logMsg("init extra window");
		emuWin = &extraWin;
		if(EmuSystem::isActive() && mainWin.win.screen() != extraWin.win.screen())
		{
			mainWin.win.screen()->removeOnFrame(onFrameUpdate);
			extraWin.win.screen()->addOnFrame(onFrameUpdate);
			applyFrameRates();
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
	#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
	emuWin->win.screen()->setFrameInterval(optionFrameInterval);
	#endif
	logMsg("running game");
	menuViewIsActive = 0;
	viewStack.navView()->showRightBtn(true);
	emuInputView.resetInput();
	//logMsg("touch control state: %d", touchControlsAreOn);
	renderer.setWindowValidOrientations(mainWin.win, optionGameOrientation);
	commonInitInput();
	popup.clear();
	Input::setKeyRepeat(false);
	EmuControls::setupVolKeysInGame();
	emuWin->win.screen()->setFrameRate(1. / EmuSystem::frameTime());
	startEmulation();
	mainWin.win.postDraw();
	if(extraWin.win)
		extraWin.win.postDraw();
	emuView.place();
	emuView2.place();
}

void restoreMenuFromGame()
{
	menuViewIsActive = 1;
	Base::setIdleDisplayPowerSave(optionIdleDisplayPowerSave);
	applyOSNavStyle(false);
	pauseEmulation();
	#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
	mainWin.win.screen()->setFrameInterval(1);
	#endif
	renderer.setWindowValidOrientations(mainWin.win, optionMenuOrientation);
	Input::setKeyRepeat(true);
	Input::setHandleVolumeKeys(false);
	if(!optionRememberLastMenu)
		viewStack.popToRoot();
	mainWin.win.screen()->setFrameRate(Base::Screen::DISPLAY_RATE_DEFAULT);
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

void mainInitCommon(int argc, char** argv)
{
	Base::setOnResume(
		[](bool focused)
		{
			AudioManager::startSession();
			renderer.restoreBind();
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
			renderer.restoreBind();
			if(View::defaultFace)
				View::defaultFace.freeCaches();
			if(View::defaultBoldFace)
				View::defaultBoldFace.freeCaches();
		});

	Base::setOnExit(
		[](bool backgrounded)
		{
			Audio::closePcm();
			AudioManager::endSession();
			renderer.restoreBind();
			if(backgrounded)
			{
				pauseEmulation();
				EmuSystem::saveAutoState();
				EmuSystem::saveBackupMem();
				Base::dispatchOnFreeCaches();
				if(optionNotificationIcon)
				{
					auto title = string_makePrintf<64>("%s was suspended", appName());
					Base::addNotification(title.data(), title.data(), EmuSystem::fullGameName().data());
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

			mainWin.drawable.freeCaches();
			extraWin.drawable.freeCaches();
			renderer.finish();

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
				renderer.restoreBind();
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
				if(viewStack.size() == 1) // update bluetooth items
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
			renderer.restoreBind();
			handleOpenFileCommand(filename);
		});
	initOptions();
	auto launchGame = parseCmdLineArgs(argc, argv);
	loadConfigFile();
	EmuSystem::onOptionsLoaded();
	AudioManager::setMusicVolumeControlHint();
	AudioManager::startSession();
	Base::setIdleDisplayPowerSave(optionIdleDisplayPowerSave);
	applyOSNavStyle(false);

	{
		std::system_error err{{}};
		#ifdef EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
		renderer = Gfx::Renderer::makeConfiguredRenderer((IG::PixelFormatID)optionWindowPixelFormat.val, err);
		#else
		renderer = Gfx::Renderer::makeConfiguredRenderer(err);
		#endif
		if(err.code())
		{
			bug_exit("error creating renderer: %s", err.what());
		}
	}

	auto compiled = renderer.texAlphaProgram.compile(renderer);
	compiled |= renderer.noTexProgram.compile(renderer);
	compiled |= View::compileGfxPrograms(renderer);
	if(compiled)
		renderer.autoReleaseShaderCompiler();
	if(!optionDitherImage.isConst)
	{
		renderer.setDither(optionDitherImage);
	}

	#ifdef __ANDROID__
	if((int8)optionProcessPriority != 0)
		Base::setProcessPriority(optionProcessPriority);

	if(Base::androidSDK() < 14 && optionAndroidTextureStorage == OPTION_ANDROID_TEXTURE_STORAGE_SURFACE_TEXTURE)
	{
		optionAndroidTextureStorage = OPTION_ANDROID_TEXTURE_STORAGE_AUTO;
	}
	if(!Gfx::Texture::setAndroidStorageImpl(renderer, makeAndroidStorageImpl(optionAndroidTextureStorage)))
	{
		// try auto if the stored setting didn't work
		optionAndroidTextureStorage = OPTION_ANDROID_TEXTURE_STORAGE_AUTO;
		Gfx::Texture::setAndroidStorageImpl(renderer, makeAndroidStorageImpl(optionAndroidTextureStorage));
	}
	#endif

	View::defaultFace = Gfx::GlyphTextureSet::makeSystem(renderer, IG::FontSettings{});
	View::defaultBoldFace = Gfx::GlyphTextureSet::makeBoldSystem(renderer, IG::FontSettings{});

	#ifdef CONFIG_INPUT_ANDROID_MOGA
	if(optionMOGAInputSystem)
		Input::initMOGA(false);
	#endif
	updateInputDevices();

	emuView.inputView = &emuInputView;
	emuView.layer = &emuVideoLayer;
	emuVideoLayer.init();
	emuVideoLayer.setLinearFilter(optionImgFilter);
	emuVideoLayer.vidImgOverlay.setEffect(renderer, optionOverlayEffect);
	emuVideoLayer.vidImgOverlay.intensity = optionOverlayEffectLevel/100.;
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	emuVideoLayer.vidImgEffect.setBitDepth(renderer, (IG::PixelFormatID)optionImageEffectPixelFormat.val == IG::PIXEL_RGBA8888 ? 32 : 16);
	#endif

	{
		auto viewNav = std::make_unique<BasicNavView>
		(
			renderer,
			&View::defaultFace,
			&getAsset(renderer, ASSET_ARROW),
			&getAsset(renderer, ASSET_GAME_ICON)
		);
		viewNav->rotateLeftBtn = true;
		viewNav->setOnPushLeftBtn(
			[](Input::Event)
			{
				viewStack.popAndShow();
			});
		viewNav->setOnPushRightBtn(
			[](Input::Event)
			{
				if(EmuSystem::gameIsRunning())
				{
					startGameFromMenu();
				}
			});
		viewNav->showRightBtn(false);
		viewStack.setShowNavViewBackButton(View::needsBackControl);
		EmuSystem::onCustomizeNavView(*viewNav);
		viewStack.setNavView(std::move(viewNav));
	}

	Base::WindowConfig winConf;

	winConf.setOnInputEvent(
		[](Base::Window &win, Input::Event e)
		{
			renderer.restoreBind();
			handleInputEvent(win, e);
		});

	winConf.setOnFocusChange(
		[](Base::Window &win, uint in)
		{
			renderer.restoreBind();
			mainWin.focused = in;
			onFocusChange(in);
		});

	winConf.setOnDragDrop(
		[](Base::Window &win, const char *filename)
		{
			logMsg("got DnD: %s", filename);
			renderer.restoreBind();
			handleOpenFileCommand(filename);
		});

	winConf.setOnSurfaceChange(
		[](Base::Window &win, Base::Window::SurfaceChange change)
		{
			if(change.resized())
			{
				renderer.restoreBind();
				updateWindowViewport(mainWin, change);
				emuView.setViewRect(mainWin.viewport().bounds(), mainWin.projectionPlane);
				placeElements();
			}
			renderer.updateDrawableForSurfaceChange(mainWin.drawable, change);
		});

	winConf.setOnDraw(
		[](Base::Window &win, Base::Window::DrawParams params)
		{
			renderer.updateCurrentDrawable(mainWin.drawable, win, params, mainWin.viewport(), mainWin.projectionMat);
			renderer.clear();
			if(EmuSystem::isActive())
			{
				if(emuView.layer)
					drawEmuFrame(renderer);
				else
				{
					emuView.draw();
					renderer.setClipRect(false);
					renderer.presentDrawable(mainWin.drawable);
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
				renderer.setClipRect(false);
				renderer.presentDrawable(mainWin.drawable);
			}
			renderer.finishPresentDrawable(mainWin.drawable);
		});

	if(Base::usesPermission(Base::Permission::WRITE_EXT_STORAGE) &&
		!Base::requestPermission(Base::Permission::WRITE_EXT_STORAGE))
	{
		logMsg("requested external storage write permissions");
	}
	renderer.initWindow(mainWin.win, winConf);
	mainInitWindowCommon(mainWin.win);
	EmuSystem::onMainWindowCreated(mainWin.win);

	if(optionShowOnSecondScreen && Base::Screen::screens() > 1)
	{
		setEmuViewOnExtraWindow(true);
	}

	applyFrameRates();

	if(launchGame)
	{
		handleOpenFileCommand(launchGame);
	}

	#ifdef __ANDROID__
	if(optionFakeUserActivity)
	{
		userActivityFaker = std::make_unique<Base::UserActivityFaker>();
	}
	#endif
}

void mainInitWindowCommon(Base::Window &win)
{
	updateProjection(mainWin, makeViewport(win));

	win.setTitle(appName());

	setupFont(renderer);
	popup.init(renderer);
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	initVControls(renderer);
	EmuControls::updateVControlImg();
	vController.menuBtnSpr.init({}, getAsset(renderer, ASSET_MENU));
	vController.ffBtnSpr.init({}, getAsset(renderer, ASSET_FAST_FORWARD));
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
	viewStack.showNavView(optionTitleBar);
	//logMsg("setting menu orientation");
	renderer.setWindowValidOrientations(win, optionMenuOrientation);
	win.setAcceptDnd(1);

	#if defined CONFIG_BASE_ANDROID
	if(!Base::apkSignatureIsConsistent())
	{
		auto &ynAlertView = *new YesNoAlertView{{win, renderer}, "Warning: App has been modified by 3rd party, use at your own risk"};
		ynAlertView.setOnNo(
			[](TextMenuItem &, View &view, Input::Event e)
			{
				Base::exit();
			});
		modalViewController.pushAndShow(ynAlertView, Input::defaultEvent());
	}
	#endif

	placeElements();
	auto mMenu = EmuSystem::makeView({win, renderer}, EmuSystem::ViewID::MAIN_MENU);
	viewStack.pushAndShow(*mMenu, Input::defaultEvent());

	win.show();
	win.postDraw();
}

void handleInputEvent(Base::Window &win, Input::Event e)
{
	if(e.isPointer())
	{
		//logMsg("Pointer %s @ %d,%d", e.actionToStr(e.state), e.x, e.y);
	}
	else
	{
		//logMsg("%s %s from %s", e.device->keyName(e.button), e.actionToStr(e.state), e.device->name());
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
			if(viewStack.size() == 1)
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

void handleOpenFileCommand(const char *path)
{
	auto type = FS::status(path).type();
	if(type == FS::file_type::directory)
	{
		logMsg("changing to dir %s from external command", path);
		restoreMenuFromGame();
		viewStack.popToRoot();
		string_copy(lastLoadPath, path);
		auto &fPicker = *EmuFilePicker::makeForLoading({mainWin.win, renderer});
		viewStack.pushAndShow(fPicker, Input::defaultEvent(), false);
		return;
	}
	if(type != FS::file_type::regular || (!hasArchiveExtension(path) && !EmuSystem::defaultFsFilter(path)))
	{
		logMsg("unrecognized file type");
		return;
	}
	logMsg("opening file %s from external command", path);
	restoreMenuFromGame();
	viewStack.popToRoot();
	if(modalViewController.hasView())
		modalViewController.pop();
	GameFilePicker::onSelectFile(viewStack.top().renderer(), path, Input::Event{});
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

void onMainMenuItemOptionChanged()
{
	onMainMenuOptionChanged_.callSafe();
}

void setOnMainMenuItemOptionChanged(OnMainMenuOptionChanged func)
{
	onMainMenuOptionChanged_ = func;
}

namespace Base
{

void onInit(int argc, char** argv)
{
	EmuSystem::onInit();
	mainInitCommon(argc, argv);
}

}
