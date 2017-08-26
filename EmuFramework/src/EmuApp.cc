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
#include <emuframework/EmuInput.hh>
#include <emuframework/FilePicker.hh>
#include <emuframework/EmuView.hh>
#include <emuframework/EmuLoadProgressView.hh>
#include <emuframework/FileUtils.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/util/utility.h>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/base/Pipe.hh>
#include <imagine/thread/Thread.hh>
#include <cmath>
#include "private.hh"

class AutoStateConfirmAlertView : public YesNoAlertView
{
	std::array<char, 96> msg{};

public:
	AutoStateConfirmAlertView(ViewAttachParams attach, const char *dateStr, bool addToRecent):
		YesNoAlertView{attach, "", "Continue", "Restart Game"}
	{
		string_printf(msg, "Auto-save state exists from:\n%s", dateStr);
		setLabel(msg.data());
		setOnYes(
			[addToRecent](TextMenuItem &, View &view, Input::Event e)
			{
				view.dismiss();
				loadGameComplete(true, addToRecent);
			});
		setOnNo(
			[addToRecent](TextMenuItem &, View &view, Input::Event e)
			{
				view.dismiss();
				loadGameComplete(false, addToRecent);
			});
	}
};

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
Base::Screen::OnFrameDelegate onFrameUpdate{};
#ifdef CONFIG_BLUETOOTH
BluetoothAdapter *bta{};
#endif
#ifdef __ANDROID__
std::unique_ptr<Base::UserActivityFaker> userActivityFaker{};
#endif
static EmuApp::OnMainMenuOptionChanged onMainMenuOptionChanged_{};
FS::PathString lastLoadPath{};
#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
SysVController vController{renderer};
uint pointerInputPlayer = 0;
#endif
[[gnu::weak]] bool EmuApp::hasIcon = true;
[[gnu::weak]] bool EmuApp::autoSaveStateDefault = true;

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
void handleOpenFileCommand(const char *filename);

Gfx::PixmapTexture &getAsset(Gfx::Renderer &r, AssetID assetID)
{
	assumeExpr(assetID < IG::size(assetFilename));
	auto &res = assetBuffImg[assetID];
	if(!res)
	{
		PngFile png;
		if(auto ec = png.loadAsset(assetFilename[assetID]);
			ec)
		{
			logErr("couldn't load %s", assetFilename[assetID]);
		}
		res.init(r, png);
	}
	return res;
}

static Gfx::PixmapTexture *getCollectTextCloseAsset(Gfx::Renderer &r)
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

void EmuApp::updateAndDrawEmuVideo()
{
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
	EmuSystem::closeGame(allowAutosaveState);
	emuWin->win.screen()->removeOnFrame(onFrameUpdate);
	setCPUNeedsLowLatency(false);
}

void EmuApp::exitGame(bool allowAutosaveState)
{
	closeGame(allowAutosaveState);
	if(EmuSystem::isActive())
	{
		restoreMenuFromGame();
	}
	// leave any sub menus that may depending on running game state
	popMenuToRoot();
}

static void drawEmuFrame(Gfx::Renderer &r)
{
	if(EmuSystem::runFrameOnDraw)
	{
		bool renderAudio = optionSound;
		EmuSystem::runFrame(emuVideo, true, true, renderAudio);
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

void EmuApp::restoreMenuFromGame()
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
	if(auto err = EmuSystem::onOptionsLoaded();
		err)
	{
		Base::exitWithErrorMessagePrintf(-1, "%s", err->what());
		return;
	}
	AudioManager::setMusicVolumeControlHint();
	AudioManager::startSession();
	Base::setIdleDisplayPowerSave(optionIdleDisplayPowerSave);
	applyOSNavStyle(false);

	{
		Gfx::Error err{};
		#ifdef EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
		renderer = Gfx::Renderer::makeConfiguredRenderer((IG::PixelFormatID)optionWindowPixelFormat.val, err);
		#else
		renderer = Gfx::Renderer::makeConfiguredRenderer(err);
		#endif
		if(err)
		{
			Base::exitWithErrorMessagePrintf(-1, "Error creating renderer: %s", err->what());
			return;
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
		EmuApp::onCustomizeNavView(*viewNav);
		viewStack.setNavView(std::move(viewNav));
	}

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
				EmuApp::saveAutoState();
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

	onFrameUpdate = [](Base::Screen::FrameParams params)
		{
			commonUpdateInput();
			if(unlikely(fastForwardActive))
			{
				EmuSystem::runFrameOnDraw = true;
				postDrawToEmuWindows();
				iterateTimes((uint)optionFastForwardSpeed, i)
				{
					EmuSystem::runFrame(emuVideo, false, false, false);
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
					constexpr uint maxLateFrameSkip = 6;
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
							EmuSystem::runFrame(emuVideo, false, false, renderAudio);
						}
					}
				}
			}
			params.readdOnFrame();
		};

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
	EmuApp::onMainWindowCreated({mainWin.win, renderer}, Input::defaultEvent());

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
		EmuApp::restoreMenuFromGame();
		viewStack.popToRoot();
		string_copy(lastLoadPath, path);
		auto &fPicker = *EmuFilePicker::makeForLoading({mainWin.win, renderer});
		viewStack.pushAndShow(fPicker, Input::defaultEvent(), false);
		return;
	}
	if(type != FS::file_type::regular || (!EmuApp::hasArchiveExtension(path) && !EmuSystem::defaultFsFilter(path)))
	{
		logMsg("unrecognized file type");
		return;
	}
	logMsg("opening file %s from external command", path);
	EmuApp::restoreMenuFromGame();
	viewStack.popToRoot();
	if(modalViewController.hasView())
		modalViewController.pop();
	onSelectFileFromPicker(viewStack.top().renderer(), path, Input::Event{});
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

void EmuApp::setOnMainMenuItemOptionChanged(OnMainMenuOptionChanged func)
{
	onMainMenuOptionChanged_ = func;
}

void loadGameComplete(bool tryAutoState, bool addToRecent)
{
	if(tryAutoState)
	{
		EmuApp::loadAutoState();
		if(!EmuSystem::gameIsRunning())
		{
			logErr("game was closed while trying to load auto-state");
			return;
		}
	}
	if(addToRecent)
		addRecentGame();
	startGameFromMenu();
}

bool showAutoStateConfirm(Gfx::Renderer &r, Input::Event e, bool addToRecent)
{
	if(!(optionConfirmAutoLoadState && optionAutoSaveState))
	{
		return 0;
	}
	auto saveStr = EmuSystem::sprintStateFilename(-1);
	if(FS::exists(saveStr))
	{
		auto mTime = FS::status(saveStr).lastWriteTimeLocal();
		char dateStr[64]{};
		std::strftime(dateStr, sizeof(dateStr), strftimeFormat, &mTime);
		auto &ynAlertView = *new AutoStateConfirmAlertView{{mainWin.win, r}, dateStr, addToRecent};
		modalViewController.pushAndShow(ynAlertView, e);
		return 1;
	}
	return 0;
}

void EmuApp::loadGameCompleteFromFilePicker(Gfx::Renderer &r, uint result, Input::Event e)
{
	if(!result)
		return;

	if(!showAutoStateConfirm(r, e, true))
	{
		loadGameComplete(1, 1);
	}
}

void onSelectFileFromPicker(Gfx::Renderer &r, const char* name, Input::Event e)
{
	EmuApp::createSystemWithMedia({}, name, "", e,
		[&r](uint result, Input::Event e)
		{
			EmuApp::loadGameCompleteFromFilePicker(r, result, e);
		});
}

void loadGameCompleteFromBenchmarkFilePicker(uint result, Input::Event e)
{
	if(result)
	{
		logMsg("starting benchmark");
		IG::Time time = EmuSystem::benchmark();
		EmuSystem::closeGame(0);
		logMsg("done in: %f", double(time));
		popup.printf(2, 0, "%.2f fps", double(180.)/double(time));
	}
}

bool EmuApp::hasArchiveExtension(const char *name)
{
	return string_hasDotExtension(name, "7z") ||
		string_hasDotExtension(name, "rar") ||
		string_hasDotExtension(name, "zip");
}

void EmuApp::pushAndShowNewCollectTextInputView(ViewAttachParams attach, Input::Event e, const char *msgText,
	const char *initialContent, CollectTextInputView::OnTextDelegate onText)
{
	auto textInputView = new CollectTextInputView{attach, msgText, initialContent,
		getCollectTextCloseAsset(attach.renderer), onText};
	pushAndShowModalView(*textInputView, e);
}

void EmuApp::pushAndShowNewYesNoAlertView(ViewAttachParams attach, Input::Event e, const char *label,
	const char *choice1, const char *choice2, TextMenuItem::SelectDelegate onYes, TextMenuItem::SelectDelegate onNo)
{
	auto alertView = new YesNoAlertView{attach, label, choice1, choice2, onYes, onNo};
	pushAndShowModalView(*alertView, e);
}

void EmuApp::pushAndShowModalView(View &v, Input::Event e)
{
	modalViewController.pushAndShow(v, e);
}

void EmuApp::popModalViews()
{
	if(modalViewController.hasView())
		modalViewController.pop();
}

void EmuApp::popMenuToRoot()
{
	viewStack.popToRoot();
}

void EmuApp::reloadGame()
{
	if(!EmuSystem::gameIsRunning())
		return;
	FS::PathString gamePath;
	string_copy(gamePath, EmuSystem::fullGamePath());
	EmuSystem::Error err{};
	EmuSystem::createWithMedia({}, gamePath.data(), "", err, [](int pos, int max, const char *label){ return true; });
	if(!err)
	{
		EmuSystem::prepareAudioVideo();
		startGameFromMenu();
	}
}

void EmuApp::printfMessage(uint secs, bool error, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	auto vaEnd = IG::scopeGuard([&](){ va_end(args); });
	popup.vprintf(secs, error, format, args);
}

void EmuApp::postMessage(const char *msg)
{
	postMessage(false, msg);
}

void EmuApp::postMessage(bool error, const char *msg)
{
	postMessage(3, error, msg);
}

void EmuApp::postMessage(uint secs, bool error, const char *msg)
{
	popup.post(msg, secs, error);
}

void EmuApp::unpostMessage()
{
	popup.clear();
}

ViewAttachParams emuViewAttachParams()
{
	return {mainWin.win, emuVideo.r};
}

[[gnu::weak]] bool EmuApp::willCreateSystem(ViewAttachParams attach, Input::Event) { return true; }

void EmuApp::createSystemWithMedia(GenericIO io, const char *path, const char *name, Input::Event e, CreateSystemCompleteDelegate onComplete)
{
	popModalViews();
	if(!EmuApp::willCreateSystem(emuViewAttachParams(), e))
	{
		return;
	}
	auto loadProgressView = new EmuLoadProgressView{emuViewAttachParams(), e, onComplete};
	pushAndShowModalView(*loadProgressView, e);
	loadProgressView->pipe.init({},
		[loadProgressView](Base::Pipe &pipe)
		{
			while(pipe.hasData())
			{
				EmuSystem::LoadProgressMessage msg{};
				pipe.read(&msg, sizeof(msg));
				switch(msg.progress)
				{
					case EmuSystem::LoadProgress::FAILED:
					{
						assumeExpr(msg.intArg3 > 0);
						uint len = msg.intArg3;
						char errorStr[len + 1];
						pipe.read(errorStr, len);
						errorStr[len] = 0;
						pipe.deinit();
						popModalViews();
						popup.postError(errorStr, 4);
						return 0;
					}
					case EmuSystem::LoadProgress::OK:
					{
						pipe.deinit();
						auto onComplete = loadProgressView->onComplete;
						auto originalEvent = loadProgressView->originalEvent;
						popModalViews();
						EmuSystem::prepareAudioVideo();
						onComplete(1, originalEvent);
						return 0;
					}
					case EmuSystem::LoadProgress::UPDATE:
					{
						loadProgressView->setPos(msg.intArg);
						loadProgressView->setMax(msg.intArg2);
						assumeExpr(msg.intArg3 >= -1);
						switch(msg.intArg3)
						{
							bcase -1: // no string
							{}
							bcase 0: // default string
							{
								loadProgressView->setLabel("Loading...");
							}
							bdefault: // custom string
							{
								uint len = msg.intArg3;
								char labelStr[len + 1];
								pipe.read(labelStr, len);
								labelStr[len] = 0;
								loadProgressView->setLabel(labelStr);
								logMsg("set custom string:%s", labelStr);
							}
						}
						loadProgressView->place();
						loadProgressView->postDraw();
						return 1;
					}
					default:
					{
						logWarn("Unknown LoadProgressMessage value:%d", (int)msg.progress);
						return 1;
					}
				}
			}
			return 1;
		});
	auto pathStr = FS::makePathString(path);
	auto fileStr = FS::makeFileString(name);
	auto ioPtr = io.release();
	IG::makeDetachedThread(
		[ioPtr, pathStr, fileStr, loadProgressView]()
		{
			logMsg("starting loader thread");
			GenericIO io{std::unique_ptr<IO>(ioPtr)};
			EmuSystem::Error err;
			EmuSystem::createWithMedia(std::move(io), pathStr.data(), fileStr.data(), err,
				[loadProgressView](int pos, int max, const char *label)
				{
					int len = label ? strlen(label) : -1;
					EmuSystem::LoadProgressMessage msg{EmuSystem::LoadProgress::UPDATE, pos, max, len};
					loadProgressView->pipe.write(&msg, sizeof(msg));
					if(len > 0)
					{
						loadProgressView->pipe.write(label, len);
					}
					return true;
				});
			if(err)
			{
				auto errStr = err->what();
				int len = strlen(errStr);
				assert(len);
				EmuSystem::LoadProgressMessage msg{EmuSystem::LoadProgress::FAILED, 0, 0, len};
				loadProgressView->pipe.write(&msg, sizeof(msg));
				loadProgressView->pipe.write(errStr, len);
				logErr("loader thread failed");
				return;
			}
			EmuSystem::LoadProgressMessage msg{EmuSystem::LoadProgress::OK, 0, 0, 0};
			loadProgressView->pipe.write(&msg, sizeof(msg));
			logMsg("loader thread finished");
		});
}

void EmuApp::saveAutoState()
{
	if(optionAutoSaveState)
	{
		auto saveStr = EmuSystem::sprintStateFilename(-1);
		//logMsg("saving autosave-state %s", saveStr.data());
		saveState(saveStr.data());
	}
}

bool EmuApp::loadAutoState()
{
	if(optionAutoSaveState)
	{
		if(auto err = EmuApp::loadStateWithSlot(-1);
			!err)
		{
			logMsg("loaded autosave-state");
			return 1;
		}
	}
	return 0;
}

EmuSystem::Error EmuApp::saveState(const char *path)
{
	if(!EmuSystem::gameIsRunning())
	{
		return EmuSystem::makeError("System not running");
	}
	fixFilePermissions(path);
	logMsg("saving state %s", path);
	return EmuSystem::saveState(path);
}

EmuSystem::Error EmuApp::saveStateWithSlot(int slot)
{
	auto path = EmuSystem::sprintStateFilename(slot);
	return saveState(path.data());
}

EmuSystem::Error EmuApp::loadState(const char *path)
{
	if(!EmuSystem::gameIsRunning())
	{
		return EmuSystem::makeError("System not running");
	}
	if(!FS::exists(path))
	{
		return EmuSystem::makeError("File doesn't exist");
	}
	fixFilePermissions(path);
	logMsg("loading state %s", path);
	return EmuSystem::loadState(path);
}

EmuSystem::Error EmuApp::loadStateWithSlot(int slot)
{
	auto path = EmuSystem::sprintStateFilename(slot);
	return loadState(path.data());
}

void EmuApp::setDefaultVControlsButtonSize(int size)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	optionTouchCtrlSize.initDefault(size);
	#endif
}

void EmuApp::setDefaultVControlsButtonSpacing(int spacing)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	optionTouchCtrlBtnSpace.initDefault(spacing);
	#endif
}

void EmuApp::setDefaultVControlsButtonStagger(int stagger)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	optionTouchCtrlBtnStagger.initDefault(stagger);
	#endif
}

FS::PathString EmuApp::mediaSearchPath()
{
	return lastLoadPath;
}

[[gnu::weak]] void EmuApp::onMainWindowCreated(ViewAttachParams, Input::Event) {}

[[gnu::weak]] void EmuApp::onCustomizeNavView(EmuApp::NavView &) {}

namespace Base
{

void onInit(int argc, char** argv)
{
	if(auto err = EmuSystem::onInit();
		err)
	{
		Base::exitWithErrorMessagePrintf(-1, "%s", err->what());
		return;
	}
	mainInitCommon(argc, argv);
}

}
