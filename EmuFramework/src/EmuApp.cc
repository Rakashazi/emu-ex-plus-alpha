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

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuView.hh>
#include <emuframework/EmuLoadProgressView.hh>
#include <emuframework/EmuVideoLayer.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/FilePicker.hh>
#include "privateInput.hh"
#include "WindowData.hh"
#include "configFile.hh"
#include "EmuOptions.hh"
#include "pathUtils.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/fs/FS.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gui/ToastView.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/data-type/image/PixmapSource.hh>
#include <imagine/util/utility.h>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>
#include <imagine/thread/Thread.hh>
#include <cmath>

namespace EmuEx
{

constexpr uint8_t OPTION_SOUND_ENABLED_FLAG = IG::bit(0);
constexpr uint8_t OPTION_SOUND_DURING_FAST_FORWARD_ENABLED_FLAG = IG::bit(1);
constexpr uint8_t OPTION_SOUND_DEFAULT_FLAGS = OPTION_SOUND_ENABLED_FLAG | OPTION_SOUND_DURING_FAST_FORWARD_ENABLED_FLAG;
static EmuApp *gAppPtr{};
[[gnu::weak]] bool EmuApp::hasIcon = true;
[[gnu::weak]] bool EmuApp::autoSaveStateDefault = true;
[[gnu::weak]] bool EmuApp::needsGlobalInstance = false;

constexpr const char *assetFilename[wise_enum::size<AssetID>]
{
	"navArrow.png",
	"x.png",
	"accept.png",
	"game.png",
	"menu.png",
	"fastForward.png",
	"overlays128.png",
	"kbOverlay.png",
};

constexpr bool isValidSoundRate(uint32_t rate)
{
	switch(rate)
	{
		case 22050:
		case 32000:
		case 44100:
		case 48000: return true;
	}
	return false;
}

constexpr bool optionOrientationIsValid(uint8_t val)
{
	return val == IG::VIEW_ROTATE_AUTO ||
		val == IG::VIEW_ROTATE_0 ||
		val == IG::VIEW_ROTATE_90 ||
		val == IG::VIEW_ROTATE_180 ||
		val == IG::VIEW_ROTATE_270;
}

constexpr bool optionAspectRatioIsValid(double val)
{
	return val == 0. || (val >= 0.1 && val <= 10.);
}

constexpr bool imageEffectPixelFormatIsValid(uint8_t val)
{
	switch(val)
	{
		case IG::PIXEL_RGB565:
		case IG::PIXEL_RGBA8888:
			return true;
	}
	return false;
}

constexpr bool optionFrameTimeIsValid(auto val)
{
	return !val || EmuSystem::frameTimeIsValid(VideoSystem::NATIVE_NTSC, IG::FloatSeconds(val));
}

constexpr bool optionFrameTimePALIsValid(auto val)
{
	return !val || EmuSystem::frameTimeIsValid(VideoSystem::PAL, IG::FloatSeconds(val));
}

constexpr bool optionImageZoomIsValid(uint8_t val)
{
	return val == optionImageZoomIntegerOnly || val == optionImageZoomIntegerOnlyY
		|| (val >= 10 && val <= 100);
}

EmuApp::EmuApp(ApplicationInitParams initParams, ApplicationContext &ctx):
	Application{initParams},
	fontManager{ctx},
	renderer{ctx},
	audioManager_{ctx},
	emuAudio{audioManager_},
	emuVideoLayer{emuVideo},
	emuSystemTask{*this},
	vController{ctx, (int)EmuSystem::inputFaceBtns, (int)EmuSystem::inputCenterBtns},
	autoSaveStateTimer
	{
		"EmuApp::autoSaveStateTimer",
		[this]()
		{
			logMsg("running auto-save state timer");
			syncEmulationThread();
			saveAutoState();
			return true;
		}
	},
	pixmapReader{ctx},
	pixmapWriter{ctx},
	vibrationManager_{ctx},
	optionAspectRatio{CFGKEY_GAME_ASPECT_RATIO, (double)EmuSystem::aspectRatioInfo[0], 0, optionAspectRatioIsValid},
	optionFrameRate{CFGKEY_FRAME_RATE, 0, 0, optionFrameTimeIsValid},
	optionFrameRatePAL{CFGKEY_FRAME_RATE_PAL, 1./50., !EmuSystem::hasPALVideoSystem, optionFrameTimePALIsValid},
	optionSoundRate{CFGKEY_SOUND_RATE, 48000, false, isValidSoundRate},
	optionFontSize{CFGKEY_FONT_Y_SIZE,
		Config::MACHINE_IS_PANDORA ? 6500 :
		(Config::envIsIOS || Config::envIsAndroid) ? 3000 :
		8000,
		false, optionIsValidWithMinMax<2000, 10000, uint16_t>},
	optionPauseUnfocused{CFGKEY_PAUSE_UNFOCUSED, 1,
		!(Config::envIsLinux || Config::envIsAndroid)},
	optionAutoSaveState{CFGKEY_AUTO_SAVE_STATE, 1},
	optionConfirmAutoLoadState{CFGKEY_CONFIRM_AUTO_LOAD_STATE, 1},
	optionConfirmOverwriteState{CFGKEY_CONFIRM_OVERWRITE_STATE, 1},
	optionFastForwardSpeed{CFGKEY_FAST_FORWARD_SPEED, 4, false, optionIsValidWithMinMax<2, 7>},
	optionSound{CFGKEY_SOUND, OPTION_SOUND_DEFAULT_FLAGS},
	optionSoundVolume{CFGKEY_SOUND_VOLUME,
		100, false, optionIsValidWithMinMax<0, 100, uint8_t>},
	optionSoundBuffers{CFGKEY_SOUND_BUFFERS,
		3, 0, optionIsValidWithMinMax<1, 7, uint8_t>},
	optionAddSoundBuffersOnUnderrun{CFGKEY_ADD_SOUND_BUFFERS_ON_UNDERRUN, 1, 0},
	optionAudioAPI{CFGKEY_AUDIO_API, 0},
	optionNotificationIcon{CFGKEY_NOTIFICATION_ICON, 1, !Config::envIsAndroid},
	optionTitleBar{CFGKEY_TITLE_BAR, 1, !CAN_HIDE_TITLE_BAR},
	optionSystemActionsIsDefaultMenu{CFGKEY_SYSTEM_ACTIONS_IS_DEFAULT_MENU, 1},
	optionIdleDisplayPowerSave{CFGKEY_IDLE_DISPLAY_POWER_SAVE, 0},
	optionLowProfileOSNav{CFGKEY_LOW_PROFILE_OS_NAV, 1, !Config::envIsAndroid},
	optionHideOSNav{CFGKEY_HIDE_OS_NAV, 0, !Config::envIsAndroid},
	optionHideStatusBar{CFGKEY_HIDE_STATUS_BAR, 1, !Config::envIsAndroid && !Config::envIsIOS},
	optionNotifyInputDeviceChange{CFGKEY_NOTIFY_INPUT_DEVICE_CHANGE, Config::Input::DEVICE_HOTSWAP, !Config::Input::DEVICE_HOTSWAP},
	optionEmuOrientation{CFGKEY_GAME_ORIENTATION,
		(Config::envIsAndroid || Config::envIsIOS) ? IG::VIEW_ROTATE_AUTO : IG::VIEW_ROTATE_0,
		false, optionOrientationIsValid},
	optionMenuOrientation{CFGKEY_MENU_ORIENTATION,
		(Config::envIsAndroid || Config::envIsIOS) ? IG::VIEW_ROTATE_AUTO : IG::VIEW_ROTATE_0,
		false, optionOrientationIsValid},
	optionShowBundledGames{CFGKEY_SHOW_BUNDLED_GAMES, 1},
	optionKeepBluetoothActive{CFGKEY_KEEP_BLUETOOTH_ACTIVE, 0},
	optionShowBluetoothScan{CFGKEY_SHOW_BLUETOOTH_SCAN, 1},
	optionSustainedPerformanceMode{CFGKEY_SUSTAINED_PERFORMANCE_MODE, 0},
	optionImgFilter{CFGKEY_GAME_IMG_FILTER, 1, 0},
	optionImgEffect{CFGKEY_IMAGE_EFFECT, 0, 0, optionIsValidWithMax<std::to_underlying(lastEnum<ImageEffectId>)>},
	optionImageEffectPixelFormat{CFGKEY_IMAGE_EFFECT_PIXEL_FORMAT, IG::PIXEL_NONE, 0, imageEffectPixelFormatIsValid},
	optionOverlayEffect{CFGKEY_OVERLAY_EFFECT, 0, 0, optionIsValidWithMax<std::to_underlying(lastEnum<ImageOverlayId>)>},
	optionOverlayEffectLevel{CFGKEY_OVERLAY_EFFECT_LEVEL, 25, 0, optionIsValidWithMax<100>},
	optionFrameInterval{CFGKEY_FRAME_INTERVAL,	1, !Config::envIsIOS, optionIsValidWithMinMax<1, 4, uint8_t>},
	optionSkipLateFrames{CFGKEY_SKIP_LATE_FRAMES, 1, 0},
	optionImageZoom(CFGKEY_IMAGE_ZOOM, 100, 0, optionImageZoomIsValid),
	optionViewportZoom(CFGKEY_VIEWPORT_ZOOM, 100, 0, optionIsValidWithMinMax<50, 100>),
	optionShowOnSecondScreen{CFGKEY_SHOW_ON_2ND_SCREEN, 1, 0},
	optionTextureBufferMode{CFGKEY_TEXTURE_BUFFER_MODE, 0},
	optionVideoImageBuffers{CFGKEY_VIDEO_IMAGE_BUFFERS, 0, 0, optionIsValidWithMax<2>},
	layoutBehindSystemUI{ctx.hasTranslucentSysUI()}
{
	if(ctx.registerInstance(initParams))
	{
		ctx.exit();
		return;
	}
	if(needsGlobalInstance)
		gAppPtr = this;
	ctx.setAcceptIPC(true);
	ctx.setOnInterProcessMessage(
		[this](IG::ApplicationContext ctx, const char *path)
		{
			logMsg("got IPC path:%s", path);
			if(ctx.mainWindow().appData<MainWindowData>())
				handleOpenFileCommand(path);
			else
				system().setInitialLoadPath(path);
		});
	initOptions(ctx);
}

class ExitConfirmAlertView : public AlertView, public EmuAppHelper<ExitConfirmAlertView>
{
public:
	ExitConfirmAlertView(ViewAttachParams attach, bool hasEmuContent):
		AlertView
		{
			attach,
			"Really Exit? (Push Back/Escape again to confirm)",
			hasEmuContent ? 3u : 2u
		}
	{
		setItem(0, "Yes", [this](){ appContext().exit(); });
		setItem(1, "No", [](){});
		if(item.size() == 3)
		{
			setItem(2, "Close Menu",
				[&]()
				{
					app().showEmulation();
				});
		}
	}

	bool inputEvent(const Input::Event &e) final
	{
		if(e.keyEvent() && e.asKeyEvent().pushed(Input::DefaultKey::CANCEL))
		{
			if(!e.asKeyEvent().repeated())
			{
				appContext().exit();
			}
			return true;
		}
		return AlertView::inputEvent(e);
	}
};

Gfx::PixmapTexture &EmuApp::asset(AssetID assetID) const
{
	auto assetIdx = std::to_underlying(assetID);
	assumeExpr(assetIdx < wise_enum::size<AssetID>);
	auto &res = assetBuffImg[assetIdx];
	if(!res)
	{
		try
		{
			res = renderer.makePixmapTexture(pixmapReader.loadAsset(assetFilename[assetIdx]),
				&renderer.get(View::imageCommonTextureSampler));
		}
		catch(...)
		{
			logErr("error loading asset:%s", assetFilename[assetIdx]);
		}
	}
	return res;
}

Gfx::PixmapTexture *EmuApp::collectTextCloseAsset() const
{
	return Config::envIsAndroid ? nullptr : &asset(AssetID::CLOSE);
}

EmuViewController &EmuApp::viewController()
{
	return mainWindowData().viewController;
}

void EmuApp::setCPUNeedsLowLatency(IG::ApplicationContext ctx, bool needed)
{
	#ifdef __ANDROID__
	if(optionSustainedPerformanceMode)
		ctx.setSustainedPerformanceMode(needed);
	#endif
}

static void suspendEmulation(EmuApp &app)
{
	if(!app.system().hasContent())
		return;
	app.saveAutoState();
	app.system().flushBackupMemory();
}

void EmuApp::closeSystem(bool allowAutosaveState)
{
	showUI();
	emuSystemTask.stop();
	system().closeRuntimeSystem(*this, allowAutosaveState);
	viewController().onSystemClosed();
}

void EmuApp::applyOSNavStyle(IG::ApplicationContext ctx, bool inGame)
{
	auto flags = IG::SYS_UI_STYLE_NO_FLAGS;
	if((int)optionLowProfileOSNav > (inGame ? 0 : 1))
		flags |= IG::SYS_UI_STYLE_DIM_NAV;
	if((int)optionHideOSNav > (inGame ? 0 : 1))
		flags |= IG::SYS_UI_STYLE_HIDE_NAV;
	if((int)optionHideStatusBar > (inGame ? 0 : 1))
		flags |= IG::SYS_UI_STYLE_HIDE_STATUS;
	ctx.setSysUIStyle(flags);
}

IG::Audio::Manager &EmuApp::audioManager()
{
	return audioManager_;
}

IG::ApplicationContext EmuApp::appContext() const
{
	return system().appContext();
}

void EmuApp::showSystemActionsViewFromSystem(ViewAttachParams attach, const Input::Event &e)
{
	viewController().showSystemActionsView(attach, e);
}

void EmuApp::showLastViewFromSystem(ViewAttachParams attach, const Input::Event &e)
{
	if(optionSystemActionsIsDefaultMenu)
	{
		showSystemActionsViewFromSystem(attach, e);
	}
	else
	{
		showUI();
	}
}

void EmuApp::showExitAlert(ViewAttachParams attach, const Input::Event &e)
{
	viewController().pushAndShowModal(std::make_unique<ExitConfirmAlertView>(
		attach, system().hasContent()), e, false);
}

static const char *parseCommandArgs(IG::CommandArgs arg)
{
	if(arg.c < 2)
	{
		return nullptr;
	}
	auto launchPath = arg.v[1];
	logMsg("starting content from command line:%s", launchPath);
	return launchPath;
}

bool EmuApp::setWindowDrawableConfig(Gfx::DrawableConfig conf)
{
	windowDrawableConf = conf;
	for(auto &w : appContext().windows())
	{
		if(!renderer.setDrawableConfig(*w, conf))
			return false;
	}
	applyRenderPixelFormat();
	return true;
}

Gfx::DrawableConfig EmuApp::windowDrawableConfig() const
{
	return windowDrawableConf;
}

std::optional<IG::PixelFormat> EmuApp::windowDrawablePixelFormatOption() const
{
	if(windowDrawableConf.pixelFormat)
		return windowDrawableConf.pixelFormat;
	return {};
}

std::optional<Gfx::ColorSpace> EmuApp::windowDrawableColorSpaceOption() const
{
	if(windowDrawableConf.colorSpace != Gfx::ColorSpace{})
		return windowDrawableConf.colorSpace;
	return {};
}

IG::PixelFormat EmuApp::windowPixelFormat() const
{
	auto fmt = windowDrawableConfig().pixelFormat;
	if(fmt)
		return fmt;
	return appContext().defaultWindowPixelFormat();
}

void EmuApp::setRenderPixelFormat(std::optional<IG::PixelFormat> fmtOpt)
{
	if(!fmtOpt)
		return;
	renderPixelFmt = *fmtOpt;
	applyRenderPixelFormat();
}

IG::PixelFormat EmuApp::renderPixelFormat() const
{
	return renderPixelFmt;
}

std::optional<IG::PixelFormat> EmuApp::renderPixelFormatOption() const
{
	if(EmuSystem::canRenderRGBA8888 && renderPixelFmt)
		return renderPixelFmt;
	return {};
}

void EmuApp::applyRenderPixelFormat()
{
	if(!emuVideo.hasRendererTask())
		return;
	auto fmt = renderPixelFormat();
	if(!fmt)
		fmt = windowPixelFormat();
	if(!EmuSystem::canRenderRGBA8888 && fmt != IG::PIXEL_RGB565)
	{
		logMsg("Using RGB565 render format since emulated system can't render RGBA8888");
		fmt = IG::PIXEL_RGB565;
	}
	emuVideoLayer.setFormat(system(), fmt, videoEffectPixelFormat(), windowDrawableConf.colorSpace);
}

void EmuApp::renderSystemFramebuffer(EmuVideo &video)
{
	if(!system().hasContent())
	{
		video.clear();
		return;
	}
	logMsg("updating video with current framebuffer content");
	system().renderFramebuffer(video);
}

static bool supportsVideoImageBuffersOption(const Gfx::Renderer &r)
{
	return r.supportsSyncFences() && r.maxSwapChainImages() > 2;
}

static IG::Microseconds makeWantedAudioLatencyUSecs(uint8_t buffers, IG::FloatSeconds frameTime)
{
	return buffers * std::chrono::duration_cast<IG::Microseconds>(frameTime);
}

void EmuApp::prepareAudio()
{
	system().configAudioPlayback(audio(), optionSoundRate);
}

void EmuApp::startAudio()
{
	audio().start(makeWantedAudioLatencyUSecs(optionSoundBuffers, system().frameTime()),
		makeWantedAudioLatencyUSecs(1, system().frameTime()));
}

EmuAudio &EmuApp::audio()
{
	return emuAudio;
}

EmuVideo &EmuApp::video()
{
	return emuVideo;
}

void EmuApp::updateLegacySavePath(IG::ApplicationContext ctx, IG::CStringView path)
{
	auto oldSaveSubDirs = subDirectoryStrings(ctx, path);
	if(oldSaveSubDirs.empty())
	{
		logMsg("no legacy save folders in:%s", path.data());
		return;
	}
	flattenSubDirectories(ctx, oldSaveSubDirs, path);
}

static bool hasExtraWindow(IG::ApplicationContext ctx)
{
	return ctx.windows().size() == 2;
}

static void dismissExtraWindow(IG::ApplicationContext ctx)
{
	if(!hasExtraWindow(ctx))
		return;
	ctx.windows()[1]->dismiss();
}

static bool extraWindowIsFocused(IG::ApplicationContext ctx)
{
	if(!hasExtraWindow(ctx))
		return false;
	return windowData(*ctx.windows()[1]).focused;
}

static IG::Screen *extraWindowScreen(IG::ApplicationContext ctx)
{
	if(!hasExtraWindow(ctx))
		return nullptr;
	return ctx.windows()[1]->screen();
}

void EmuApp::mainInitCommon(IG::ApplicationInitParams initParams, IG::ApplicationContext ctx)
{
	auto appConfig = loadConfigFile(ctx);
	system().onOptionsLoaded();
	loadSystemOptions();
	updateLegacySavePathOnStoragePath(ctx, system());
	auto launchGame = parseCommandArgs(initParams.commandArgs());
	if(launchGame)
		system().setInitialLoadPath(launchGame);
	audioManager().setMusicVolumeControlHint();
	if(optionSoundRate > optionSoundRate.defaultVal)
		optionSoundRate.reset();
	emuAudio.setRate(optionSoundRate);
	emuAudio.setAddSoundBuffersOnUnderrun(optionAddSoundBuffersOnUnderrun);
	if(!renderer.supportsColorSpace())
		windowDrawableConf.colorSpace = {};
	applyOSNavStyle(ctx, false);

	ctx.addOnResume(
		[this](IG::ApplicationContext ctx, bool focused)
		{
			audioManager().startSession();
			if(soundIsEnabled())
				emuAudio.open(audioOutputAPI());
			return true;
		});

	ctx.addOnExit(
		[this](IG::ApplicationContext ctx, bool backgrounded)
		{
			if(backgrounded)
			{
				suspendEmulation(*this);
				if(optionNotificationIcon)
				{
					auto title = fmt::format("{} was suspended", ctx.applicationName);
					ctx.addNotification(title, title, system().contentDisplayName());
				}
			}
			emuAudio.close();
			audioManager().endSession();

			saveConfigFile(ctx);
			saveSystemOptions();

			#ifdef CONFIG_BLUETOOTH
			if(bta && (!backgrounded || (backgrounded && !optionKeepBluetoothActive)))
				closeBluetoothConnections();
			#endif

			ctx.dispatchOnFreeCaches(false);

			#ifdef CONFIG_BASE_IOS
			//if(backgrounded)
			//	FsSys::remove("/private/var/mobile/Library/Caches/" CONFIG_APP_ID "/com.apple.opengl/shaders.maps");
			#endif

			return true;
		});

	IG::WindowConfig winConf{};
	winConf.setTitle(ctx.applicationName);
	winConf.setFormat(windowDrawableConf.pixelFormat);
	ctx.makeWindow(winConf,
		[this, appConfig](IG::ApplicationContext ctx, IG::Window &win)
		{
			renderer.initMainTask(&win, windowDrawableConfig());
			if(!supportsVideoImageBuffersOption(renderer))
				optionVideoImageBuffers.resetToConst();
			if(optionTextureBufferMode.val)
			{
				auto mode = (Gfx::TextureBufferMode)optionTextureBufferMode.val;
				if(renderer.makeValidTextureBufferMode(mode) != mode)
				{
					// reset to default if saved non-default mode isn't supported
					optionTextureBufferMode.reset();
				}
			}
			viewManager = {renderer};
			viewManager.setNeedsBackControl(appConfig.backNavigation());
			viewManager.setDefaultFace({renderer, fontManager.makeSystem(), fontSettings(win)});
			viewManager.setDefaultBoldFace({renderer, fontManager.makeBoldSystem(), fontSettings(win)});
			ViewAttachParams viewAttach{viewManager, win, renderer.task()};
			auto &winData = win.makeAppData<MainWindowData>(viewAttach, vController, emuVideoLayer, system());
			winData.updateWindowViewport(win, makeViewport(win));
			win.setAcceptDnd(true);
			renderer.setWindowValidOrientations(win, optionMenuOrientation);
			updateInputDevices(ctx);
			vController.configure(win, renderer, viewManager.defaultFace());
			vController.setMenuImage(asset(AssetID::MENU));
			vController.setFastForwardImage(asset(AssetID::FAST_FORWARD));
			if constexpr(VCONTROLS_GAMEPAD)
			{
				vController.setImg(asset(AssetID::GAMEPAD_OVERLAY));
			}
			if(EmuSystem::inputHasKeyboard)
			{
				vController.setKeyboardImage(asset(AssetID::KEYBOARD_OVERLAY));
			}
			auto &screen = *win.screen();
			if(!screen.supportsTimestamps() && (!Config::envIsLinux || screen.frameRate() < 100.))
			{
				setWindowFrameClockSource(IG::Window::FrameTimeSource::RENDERER);
			}
			else
			{
				setWindowFrameClockSource(IG::Window::FrameTimeSource::SCREEN);
			}
			logMsg("timestamp source:%s", windowFrameClockSource() == IG::Window::FrameTimeSource::RENDERER ? "renderer" : "screen");
			winData.viewController.placeElements();
			winData.viewController.pushAndShowMainMenu(viewAttach, emuVideoLayer, emuAudio);
			configureSecondaryScreens();
			applyFrameRates(false);
			emuVideo.setOnFormatChanged(
				[this, &viewController = winData.viewController](EmuVideo &)
				{
					emuVideoLayer.onVideoFormatChanged(videoEffectPixelFormat());
					if(emuVideoLayer.zoom() > 100)
					{
						viewController.placeEmuViews();
					}
				});
			emuVideo.setRendererTask(renderer.task());
			emuVideo.setTextureBufferMode(system(), (Gfx::TextureBufferMode)optionTextureBufferMode.val);
			emuVideo.setImageBuffers(optionVideoImageBuffers);
			emuVideoLayer.setLinearFilter(optionImgFilter); // init the texture sampler before setting format
			applyRenderPixelFormat();
			emuVideoLayer.setOverlay((ImageOverlayId)optionOverlayEffect.val);
			emuVideoLayer.setOverlayIntensity(optionOverlayEffectLevel/100.);
			emuVideoLayer.setEffect(system(), (ImageEffectId)optionImgEffect.val, videoEffectPixelFormat());
			emuVideoLayer.setAspectRatio(optionAspectRatio);
			emuVideoLayer.setZoom(optionImageZoom);
			system().onFrameUpdate = [this, &viewController = winData.viewController](IG::FrameParams params)
				{
					bool skipForward = false;
					bool fastForwarding = false;
					auto &audio = this->audio();
					auto &sys = system();
					if(sys.shouldFastForward()) [[unlikely]]
					{
						// for skipping loading on disk-based computers
						fastForwarding = true;
						skipForward = true;
						sys.setSpeedMultiplier(audio, 8);
					}
					else if(sys.targetFastForwardSpeed > 1) [[unlikely]]
					{
						fastForwarding = true;
						sys.setSpeedMultiplier(audio, sys.targetFastForwardSpeed);
					}
					else
					{
						sys.setSpeedMultiplier(audio, 1);
					}
					auto frameInfo = sys.advanceFramesWithTime(params.timestamp());
					if(!frameInfo.advanced)
					{
						return true;
					}
					if(!shouldSkipLateFrames() && !fastForwarding)
					{
						frameInfo.advanced = frameInterval();
					}
					constexpr int maxFrameSkip = 8;
					auto framesToEmulate = std::min(frameInfo.advanced, maxFrameSkip);
					EmuAudio *audioPtr = audio ? &audio : nullptr;
					/*logMsg("frame present time:%.4f next display frame:%.4f",
						std::chrono::duration_cast<IG::FloatSeconds>(frameInfo.presentTime).count(),
						std::chrono::duration_cast<IG::FloatSeconds>(params.presentTime()).count());*/
					auto &video = this->video();
					if(framesToEmulate == 1)
					{
						// run common 1-frame case synced until the video frame is ready for more consistent timing
						emuSystemTask.runFrame(&video, audioPtr, 1, false, true);
						if(emuSystemTask.resetVideoFormatChanged())
						{
							video.dispatchFormatChanged();
						}
						viewController.emuWindow().setNeedsDraw(true);
						if(usePresentationTime())
							renderer.setPresentationTime(viewController.emuWindow(), params.presentTime());
						return true;
					}
					else
					{
						// run multiple frames async and let main loop collect additional input events
						emuSystemTask.runFrame(&video, audioPtr, framesToEmulate, skipForward, false);
						if(usePresentationTime())
							renderer.setPresentationTime(viewController.emuWindow(), params.presentTime());
						return false;
					}
				};

			#if defined __ANDROID__
			if(!ctx.apkSignatureIsConsistent())
			{
				auto ynAlertView = std::make_unique<YesNoAlertView>(viewAttach, "Warning: App has been modified by 3rd party, use at your own risk");
				ynAlertView->setOnNo(
					[](View &v)
					{
						v.appContext().exit();
					});
				viewController().pushAndShowModal(std::move(ynAlertView), false);
			}
			#endif

			win.setOnInputEvent(
				[this](IG::Window &win, const Input::Event &e)
				{
					return viewController().inputEvent(e);
				});

			win.setOnSurfaceChange(
				[this](IG::Window &win, IG::Window::SurfaceChange change)
				{
					if(change.resized())
					{
						viewController().updateMainWindowViewport(win, makeViewport(win), renderer.task());
					}
					renderer.task().updateDrawableForSurfaceChange(win, change);
				});

			win.setOnDraw(
				[this](IG::Window &win, IG::Window::DrawParams params)
				{
					return viewController().drawMainWindow(win, params, renderer.task());
				});

			win.setOnDragDrop(
				[this](IG::Window &win, const char *filename)
				{
					logMsg("got DnD: %s", filename);
					handleOpenFileCommand(filename);
				});

			win.setOnFocusChange(
				[this](IG::Window &win, bool in)
				{
					windowData(win).focused = in;
					onFocusChange(in);
				});

			onMainWindowCreated(viewAttach, ctx.defaultInputEvent());

			ctx.setOnInterProcessMessage(
				[this](IG::ApplicationContext, const char *path)
				{
					logMsg("got IPC path:%s", path);
					handleOpenFileCommand(path);
				});

			ctx.setOnScreenChange(
				[this](IG::ApplicationContext ctx, IG::Screen &screen, IG::ScreenChange change)
				{
					if(change.added())
					{
						logMsg("screen added");
						if(showOnSecondScreenOption() && ctx.screens().size() > 1)
							setEmuViewOnExtraWindow(true, screen);
					}
					else if(change.removed())
					{
						logMsg("screen removed");
						if(hasExtraWindow(appContext()) && *extraWindowScreen(appContext()) == screen)
							setEmuViewOnExtraWindow(false, screen);
					}
				});

			ctx.setOnInputDevicesEnumerated(
				[this, ctx]()
				{
					logMsg("input devs enumerated");
					updateInputDevices(ctx);
				});

			ctx.setOnInputDeviceChange(
				[this, ctx](const Input::Device &dev, Input::DeviceChange change)
				{
					logMsg("got input dev change");

					updateInputDevices(ctx);

					if(optionNotifyInputDeviceChange && (change.added() || change.removed()))
					{
						postMessage(2, 0, fmt::format("{} {}", inputDevData(dev).displayName, change.added() ? "connected" : "disconnected"));
					}
					else if(change.hadConnectError())
					{
						postMessage(2, 1, fmt::format("{} had a connection error", dev.name()));
					}

					viewController().onInputDevicesChanged();
				});

			ctx.setOnFreeCaches(
				[this](IG::ApplicationContext, bool running)
				{
					viewManager.defaultFace().freeCaches();
					viewManager.defaultBoldFace().freeCaches();
					if(running)
						viewController().prepareDraw();
				});

			ctx.addOnExit(
				[this](IG::ApplicationContext ctx, bool backgrounded)
				{
					if(backgrounded)
					{
						showUI();
						if(showOnSecondScreenOption() && ctx.screens().size() > 1)
						{
							setEmuViewOnExtraWindow(false, *ctx.screens()[1]);
						}
						viewController().onHide();
						ctx.addOnResume(
							[this](IG::ApplicationContext, bool focused)
							{
								configureSecondaryScreens();
								viewController().prepareDraw();
								if(viewController().isShowingEmulation() && focused && system().isPaused())
								{
									logMsg("resuming emulation due to app resume");
									viewController().inputView().resetInput();
									startEmulation();
								}
								return false;
							}, 10);
					}
					else
					{
						closeSystem();
					}
					return true;
				}, -10);

			if(auto launchPathStr = system().contentLocation();
				launchPathStr.size())
			{
				system().setInitialLoadPath("");
				handleOpenFileCommand(launchPathStr);
			}

			win.show();
		});
}

IG::Viewport EmuApp::makeViewport(const IG::Window &win) const
{
	IG::WindowRect viewRect = layoutBehindSystemUI ? win.bounds() : win.contentBounds();
	if((int)optionViewportZoom != 100)
	{
		IG::WP viewCenter{viewRect.xSize() / 2, viewRect.ySize() / 2};
		viewRect -= viewCenter;
		viewRect *= optionViewportZoom/100.;
		viewRect += viewCenter;
	}
	return win.viewport(viewRect);
}

void WindowData::updateWindowViewport(const IG::Window &win, IG::Viewport viewport)
{
	projection = {viewport, Gfx::Mat4::makePerspectiveFovRH(M_PI/4.0, viewport.realAspectRatio(), 1.0, 100.)};
	contentRect = viewport.bounds().intersection(win.contentBounds());
}

void EmuApp::dispatchOnMainMenuItemOptionChanged()
{
	onMainMenuOptionChanged_.callSafe();
}

void EmuApp::setOnMainMenuItemOptionChanged(OnMainMenuOptionChanged func)
{
	onMainMenuOptionChanged_ = func;
}

void EmuApp::launchSystem(bool tryAutoState)
{
	if(tryAutoState)
	{
		loadAutoState();
		if(!system().hasContent())
		{
			logErr("system was closed while trying to load auto-state");
			return;
		}
	}
	if(!hasWriteAccessToDir(system().contentSaveDirectory()))
	{
		postErrorMessage(8, "Save folder inaccessible, please set it in Options➔File Paths➔Saves");
	}
	showEmulation();
}

void EmuApp::onSelectFileFromPicker(GenericIO io, IG::CStringView path, std::string_view displayName,
	const Input::Event &e, EmuSystemCreateParams params, ViewAttachParams attachParams)
{
	createSystemWithMedia(std::move(io), path, displayName, e, params, attachParams,
		[this](const Input::Event &e)
		{
			addCurrentContentToRecent();
			launchSystemWithResumePrompt(e);
		});
}

void EmuApp::handleOpenFileCommand(IG::CStringView path)
{
	auto name = appContext().fileUriDisplayName(path);
	if(name.empty())
	{
		postErrorMessage(fmt::format("Can't access path name for:\n{}", path));
		return;
	}
	if(!IG::isUri(path) && FS::status(path).type() == FS::file_type::directory)
	{
		logMsg("changing to dir %s from external command", path.data());
		showUI(false);
		viewController().popToRoot();
		setContentSearchPath(path);
		viewController().pushAndShow(
			EmuFilePicker::makeForLoading(attachParams(), appContext().defaultInputEvent()),
			appContext().defaultInputEvent(),
			false);
		return;
	}
	logMsg("opening file %s from external command", path.data());
	showUI();
	viewController().popToRoot();
	onSelectFileFromPicker({}, path, name, Input::KeyEvent{}, {}, attachParams());
}

void EmuApp::runBenchmarkOneShot(EmuVideo &emuVideo)
{
	logMsg("starting benchmark");
	IG::FloatSeconds time = system().benchmark(emuVideo);
	closeSystem(false);
	logMsg("done in: %f", time.count());
	postMessage(2, 0, fmt::format("{:.2f} fps", double(180.)/time.count()));
}

void EmuApp::showEmulation()
{
	if(viewController().isShowingEmulation() || !system().hasContent())
		return;
	configureAppForEmulation(true);
	resetInput();
	viewController().showEmulationView();
	startEmulation();
}

void EmuApp::startEmulation()
{
	if(!viewController().isShowingEmulation())
		return;
	emuVideoLayer.setBrightness(1.f);
	video().setOnFrameFinished(
		[this](EmuVideo &)
		{
			addOnFrame();
			viewController().emuWindow().drawNow();
		});
	setCPUNeedsLowLatency(appContext(), true);
	emuSystemTask.start();
	system().start(*this);
	addOnFrameDelayed();
}

void EmuApp::showUI(bool updateTopView)
{
	if(!viewController().isShowingEmulation())
		return;
	pauseEmulation();
	configureAppForEmulation(false);
	viewController().showMenuView(updateTopView);
}

void EmuApp::pauseEmulation()
{
	setCPUNeedsLowLatency(appContext(), false);
	video().setOnFrameFinished([](EmuVideo &){});
	emuSystemTask.pause();
	system().pause(*this);
	setFastForwardSpeed(0);
	emuVideoLayer.setBrightness(.75f);
	viewController().emuWindow().setDrawEventPriority();
	removeOnFrame();
}

void EmuApp::launchSystemWithResumePrompt(const Input::Event &e)
{
	if(optionAutoSaveState && optionConfirmAutoLoadState)
	{
		if(!viewController().showAutoStateConfirm(e))
		{
			// state doesn't exist
			launchSystem(false);
		}
	}
	else
	{
		launchSystem(optionAutoSaveState);
	}
}

void EmuApp::launchSystem(const Input::Event &e, bool tryAutoState)
{
	launchSystem(tryAutoState);
}

bool EmuApp::hasArchiveExtension(std::string_view name)
{
	return FS::hasArchiveExtension(name);
}

void EmuApp::pushAndShowNewCollectTextInputView(ViewAttachParams attach, const Input::Event &e, const char *msgText,
	const char *initialContent, CollectTextInputView::OnTextDelegate onText)
{
	pushAndShowModalView(std::make_unique<CollectTextInputView>(attach, msgText, initialContent,
		collectTextCloseAsset(), onText), e);
}

void EmuApp::pushAndShowNewYesNoAlertView(ViewAttachParams attach, const Input::Event &e, const char *label,
	const char *choice1, const char *choice2, TextMenuItem::SelectDelegate onYes, TextMenuItem::SelectDelegate onNo)
{
	pushAndShowModalView(std::make_unique<YesNoAlertView>(attach, label, choice1, choice2, onYes, onNo), e);
}

void EmuApp::pushAndShowModalView(std::unique_ptr<View> v, const Input::Event &e)
{
	viewController().pushAndShowModal(std::move(v), e, false);
}

void EmuApp::pushAndShowModalView(std::unique_ptr<View> v)
{
	auto e = v->appContext().defaultInputEvent();
	pushAndShowModalView(std::move(v), e);
}

void EmuApp::popModalViews()
{
	viewController().popModalViews();
}

void EmuApp::popMenuToRoot()
{
	viewController().popToRoot();
}

void EmuApp::reloadSystem(EmuSystemCreateParams params)
{
	if(!system().hasContent())
		return;
	viewController().popToSystemActionsMenu();
	emuSystemTask.pause();
	auto ctx = appContext();
	try
	{
		system().createWithMedia({}, system().contentLocation(),
			ctx.fileUriDisplayName(system().contentLocation()), params,
			[](int pos, int max, const char *label){ return true; });
		onSystemCreated();
		showEmulation();
	}
	catch(...)
	{
		logErr("Error reloading system");
		system().clearGamePaths();
	}
}

void EmuApp::onSystemCreated()
{
	prepareAudio();
	viewController().onSystemCreated();
}

void EmuApp::promptSystemReloadDueToSetOption(ViewAttachParams attach, const Input::Event &e, EmuSystemCreateParams params)
{
	if(!system().hasContent())
		return;
	auto ynAlertView = std::make_unique<YesNoAlertView>(attach,
		"This option takes effect next time the system starts. Restart it now?");
	ynAlertView->setOnYes(
		[this, params]()
		{
			reloadSystem(params);
		});
	viewController().pushAndShowModal(std::move(ynAlertView), e, false);
}

void EmuApp::unpostMessage()
{
	viewController().popupMessageView().clear();
}

void EmuApp::printScreenshotResult(int num, bool success)
{
	if(num == -1)
	{
		postErrorMessage("Too many screenshots");
	}
	else
	{
		postMessage(2, !success, fmt::format("{}{}",
			success ? "Wrote screenshot #" : "Error writing screenshot #", num));
	}
}

void EmuApp::createSystemWithMedia(GenericIO io, IG::CStringView path, std::string_view displayName,
	const Input::Event &e, EmuSystemCreateParams params, ViewAttachParams attachParams,
	CreateSystemCompleteDelegate onComplete)
{
	assert(strlen(path));
	if(!EmuApp::hasArchiveExtension(displayName) && !EmuSystem::defaultFsFilter(displayName))
	{
		postErrorMessage("File doesn't have a valid extension");
		return;
	}
	if(!EmuApp::willCreateSystem(attachParams, e))
	{
		return;
	}
	closeSystem();
	auto loadProgressView = std::make_unique<EmuLoadProgressView>(attachParams, e, onComplete);
	auto &msgPort = loadProgressView->messagePort();
	pushAndShowModalView(std::move(loadProgressView), e);
	auto ctx = attachParams.window().appContext();
	auto &sys = system();
	IG::makeDetachedThread(
		[&sys, ctx, io{std::move(io)}, pathStr = FS::PathString{path}, nameStr = FS::FileString{displayName}, &msgPort, params]() mutable
		{
			logMsg("starting loader thread");
			try
			{
				sys.createWithMedia(std::move(io), pathStr, nameStr, params,
					[&msgPort](int pos, int max, const char *label)
					{
						int len = label ? std::string_view{label}.size() : -1;
						auto msg = EmuSystem::LoadProgressMessage{EmuSystem::LoadProgress::UPDATE, pos, max, len};
						if(len > 0)
						{
							msgPort.sendWithExtraData(msg, label, len);
						}
						else
						{
							msgPort.send(msg);
						}
						return true;
					});
				msgPort.send({EmuSystem::LoadProgress::OK, 0, 0, 0});
				logMsg("loader thread finished");
			}
			catch(std::exception &err)
			{
				sys.clearGamePaths();
				std::string_view errStr{err.what()};
				int len = errStr.size();
				assert(len);
				if(len > 1024)
				{
					logWarn("truncating long error size:%d", len);
					len = 1024;
				}
				msgPort.sendWithExtraData({EmuSystem::LoadProgress::FAILED, 0, 0, len}, errStr.data(), len);
				logErr("loader thread failed");
				return;
			}
		});
}

void EmuApp::saveAutoState()
{
	if(optionAutoSaveState)
	{
		//logMsg("saving autosave-state");
		saveState(system().statePath(-1));
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

bool EmuApp::saveState(IG::CStringView path)
{
	if(!system().hasContent())
	{
		postErrorMessage("System not running");
		return false;
	}
	syncEmulationThread();
	logMsg("saving state %s", path.data());
	try
	{
		system().saveState(path);
		return true;
	}
	catch(std::exception &err)
	{
		postErrorMessage(4, fmt::format("Can't save state:\n{}", err.what()));
		return false;
	}
}

bool EmuApp::saveStateWithSlot(int slot)
{
	return saveState(system().statePath(slot));
}

bool EmuApp::loadState(IG::CStringView path)
{
	if(!system().hasContent()) [[unlikely]]
	{
		postErrorMessage("System not running");
		return false;
	}
	logMsg("loading state %s", path.data());
	syncEmulationThread();
	try
	{
		system().loadState(*this, path);
		return true;
	}
	catch(std::exception &err)
	{
		postErrorMessage(4, fmt::format("Can't load state:\n{}", err.what()));
		return false;
	}
}

bool EmuApp::loadStateWithSlot(int slot)
{
	return loadState(system().statePath(slot));
}

void EmuApp::setDefaultVControlsButtonSpacing(int spacing)
{
	vController.setDefaultButtonSpacing(spacing);
}

void EmuApp::setDefaultVControlsButtonStagger(int stagger)
{
	vController.setDefaultButtonStagger(stagger);
}

FS::PathString EmuApp::contentSearchPath() const
{
	return contentSearchPath_;
}

FS::PathString EmuApp::contentSearchPath(std::string_view name) const
{
	return FS::uriString(contentSearchPath_, name);
}

void EmuApp::setContentSearchPath(std::string_view path)
{
	contentSearchPath_ = path;
}

FS::PathString EmuApp::firmwareSearchPath() const
{
	auto ctx = appContext();
	auto firmwarePath = system().firmwarePath();
	if(firmwarePath.empty() || !ctx.fileUriExists(firmwarePath))
		return contentSearchPath();
	return hasArchiveExtension(firmwarePath) ? FS::dirnameUri(firmwarePath) : firmwarePath;
}

void EmuApp::setFirmwareSearchPath(std::string_view path)
{
	system().setFirmwarePath(path);
}

[[gnu::weak]] void EmuApp::onMainWindowCreated(ViewAttachParams, const Input::Event &) {}

[[gnu::weak]] void EmuApp::onCustomizeNavView(EmuApp::NavView &) {}

[[gnu::weak]] std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, EmuApp::ViewID id)
{
	return nullptr;
}

void EmuApp::addTurboInputEvent(unsigned action)
{
	turboActions.addEvent(action);
}

void EmuApp::removeTurboInputEvent(unsigned action)
{
	turboActions.removeEvent(action);
}

void EmuApp::runTurboInputEvents()
{
	assert(system().hasContent());
	turboActions.update(*this);
}

void EmuApp::resetInput()
{
	turboActions = {};
	setFastForwardSpeed(0);
}

void EmuApp::setFastForwardSpeed(int speed)
{
	bool active = speed > 1;
	system().targetFastForwardSpeed = speed;
	emuAudio.setAddSoundBuffersOnUnderrun(active ? addSoundBuffersOnUnderrun() : false);
	auto vol = (active && !soundDuringFastForwardIsEnabled()) ? 0 : soundVolume();
	emuAudio.setVolume(vol);
}

FS::PathString EmuApp::sessionConfigPath()
{
	return system().contentSaveFilePath(".config");
}

bool EmuApp::hasSavedSessionOptions()
{
	return system().sessionOptionsAreSet() || appContext().fileUriExists(sessionConfigPath());
}

void EmuApp::deleteSessionOptions()
{
	if(!hasSavedSessionOptions())
	{
		return;
	}
	system().resetSessionOptions(*this);
	system().resetSessionOptionsSet();
	appContext().removeFileUri(sessionConfigPath());
}

void EmuApp::saveSessionOptions()
{
	if(!system().sessionOptionsAreSet())
		return;
	auto configFilePath = sessionConfigPath();
	try
	{
		auto ctx = appContext();
		auto configFile = ctx.openFileUri(configFilePath, IO::OPEN_NEW);
		writeConfigHeader(configFile);
		system().writeConfig(ConfigType::SESSION, configFile);
		system().resetSessionOptionsSet();
		if(configFile.size() == 1)
		{
			// delete file if only header was written
			configFile = {};
			ctx.removeFileUri(configFilePath);
			logMsg("deleted empty session config file:%s", configFilePath.data());
		}
		else
		{
			logMsg("wrote session config file:%s", configFilePath.data());
		}
	}
	catch(...)
	{
		logMsg("error creating session config file:%s", configFilePath.data());
	}
}

void EmuApp::loadSessionOptions()
{
	if(!system().resetSessionOptions(*this))
		return;
	if(readConfigKeys(FileUtils::bufferFromUri(appContext(), sessionConfigPath(), IO::TEST_BIT),
		[this](uint16_t key, uint16_t size, IO &io)
		{
			switch(key)
			{
				default:
				{
					if(!system().readConfig(ConfigType::SESSION, io, key, size))
					{
						logMsg("skipping unknown key %u", (unsigned)key);
					}
				}
			}
		}))
	{
		system().onSessionOptionsLoaded(*this);
	}
}

void EmuApp::loadSystemOptions()
{
	auto configName = system().configName();
	if(configName.empty())
		return;
	readConfigKeys(FileUtils::bufferFromPath(FS::pathString(appContext().supportPath(), configName), IO::TEST_BIT),
		[this](uint16_t key, uint16_t size, IO &io)
		{
			if(!system().readConfig(ConfigType::CORE, io, key, size))
			{
				logMsg("skipping unknown system config key:%u", (unsigned)key);
			}
		});
}

void EmuApp::saveSystemOptions()
{
	auto configName = system().configName();
	if(configName.empty())
		return;
	try
	{
		auto configFilePath = FS::pathString(appContext().supportPath(), configName);
		auto configFile = FileIO{configFilePath, IO::OPEN_NEW};
		saveSystemOptions(configFile);
		if(configFile.size() == 1)
		{
			// delete file if only header was written
			configFile = {};
			FS::remove(configFilePath);
			logMsg("deleted empty system config file");
		}
	}
	catch(...)
	{
		logErr("error writing system config file");
	}
}

void EmuApp::saveSystemOptions(IO &configFile)
{
	writeConfigHeader(configFile);
	system().writeConfig(ConfigType::CORE, configFile);
}

void EmuApp::syncEmulationThread()
{
	emuSystemTask.pause();
}

void EmuApp::cancelAutoSaveStateTimer()
{
	autoSaveStateTimer.cancel();
}

void EmuApp::startAutoSaveStateTimer()
{
	if(optionAutoSaveState > 1)
	{
		IG::Minutes mins{optionAutoSaveState.val};
		autoSaveStateTimer.run(mins, mins);
	}
}

VController &EmuApp::defaultVController()
{
	return vController;
}

void EmuApp::configFrameTime()
{
	system().configFrameTime(emuAudio.format().rate);
}

void EmuApp::runFrames(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio, int frames, bool skipForward)
{
	if(skipForward) [[unlikely]]
	{
		if(skipForwardFrames(taskCtx, frames - 1))
		{
			// don't write any audio while skip is in progress
			audio = nullptr;
		}
		else
		{
			// restore normal speed when skip ends
			system().setSpeedMultiplier(*audio, 1);
		}
	}
	else
	{
		skipFrames(taskCtx, frames - 1, audio);
	}
	runTurboInputEvents();
	system().runFrame(taskCtx, video, audio);
	system().updateBackupMemoryCounter();
}

void EmuApp::skipFrames(EmuSystemTaskContext taskCtx, int frames, EmuAudio *audio)
{
	assert(system().hasContent());
	for(auto i : iotaCount(frames))
	{
		runTurboInputEvents();
		system().runFrame(taskCtx, nullptr, audio);
	}
}

bool EmuApp::skipForwardFrames(EmuSystemTaskContext taskCtx, int frames)
{
	for(auto i : iotaCount(frames))
	{
		skipFrames(taskCtx, 1, nullptr);
		if(!system().shouldFastForward())
		{
			logMsg("skip-forward ended early after %u frame(s)", i);
			return false;
		}
	}
	return true;
}

bool EmuApp::writeScreenshot(IG::PixmapView pix, IG::CStringView path)
{
	return pixmapWriter.writeToFile(pix, path);
}

std::pair<int, FS::PathString> EmuApp::makeNextScreenshotFilename()
{
	static constexpr int maxNum = 999;
	auto ctx = appContext();
	auto basePath = system().contentSavePath(system().contentName());
	for(auto i : iotaCount(maxNum))
	{
		auto str = IG::format<FS::PathString>("{}.{:03d}.png", basePath, i);
		if(!ctx.fileUriExists(str))
		{
			logMsg("screenshot %d", i);
			return {i, str};
		}
	}
	logMsg("no screenshot filenames left");
	return {-1, {}};
}

bool EmuApp::mogaManagerIsActive() const
{
	return (bool)mogaManagerPtr;
}

void EmuApp::setMogaManagerActive(bool on, bool notify)
{
	IG::doIfUsed(mogaManagerPtr,
		[&](auto &mogaManagerPtr)
		{
			if(on)
				mogaManagerPtr = std::make_unique<Input::MogaManager>(appContext(), notify);
			else
				mogaManagerPtr.reset();
		});
}

std::span<const KeyCategory> EmuApp::inputControlCategories() const
{
	return {Controls::category, Controls::categories};
}

ViewAttachParams EmuApp::attachParams()
{
	return viewController().inputView().attachParams();
}

void EmuApp::addRecentContent(std::string_view fullPath, std::string_view name)
{
	if(fullPath.empty())
		return;
	logMsg("adding %s @ %s to recent list, current size: %zu", name.data(), fullPath.data(), recentContentList.size());
	RecentContentInfo recent{FS::PathString{fullPath}, FS::FileString{name}};
	IG::eraseFirst(recentContentList, recent); // remove existing entry so it's added to the front
	if(recentContentList.isFull()) // list full
		recentContentList.pop_back();
	recentContentList.insert(recentContentList.begin(), recent);

	/*logMsg("list contents:");
	for(auto &e : recentContentList)
	{
		logMsg("path: %s name: %s", e.path.data(), e.name.data());
	}*/
}

void EmuApp::addCurrentContentToRecent()
{
	addRecentContent(system().contentLocation(), system().contentDisplayName());
}

void EmuApp::setSoundRate(int rate)
{
	assert(rate <= (int)optionSoundRate.defaultVal);
	if(!rate)
		rate = optionSoundRate.defaultVal;
	optionSoundRate = rate;
	system().configAudioPlayback(audio(), rate);
}

bool EmuApp::setSoundVolume(int vol)
{
	if(!optionSoundVolume.isValidVal(vol))
		return false;
	optionSoundVolume = vol;
	audio().setVolume(vol);
	return true;
}

void EmuApp::setSoundBuffers(int buffers)
{
	optionSoundBuffers = buffers;
}

bool EmuApp::soundIsEnabled() const
{
	return optionSound & OPTION_SOUND_ENABLED_FLAG;
}

void EmuApp::setSoundEnabled(bool on)
{
	optionSound = IG::setOrClearBits(optionSound.val, OPTION_SOUND_ENABLED_FLAG, on);
	if(on)
		audio().open(audioOutputAPI());
	else
		audio().close();
}

void EmuApp::setAddSoundBuffersOnUnderrun(bool on)
{
	optionAddSoundBuffersOnUnderrun = on;
	audio().setAddSoundBuffersOnUnderrun(on);
}

bool EmuApp::soundDuringFastForwardIsEnabled() const
{
	return optionSound & OPTION_SOUND_DURING_FAST_FORWARD_ENABLED_FLAG;
}

void EmuApp::setSoundDuringFastForwardEnabled(bool on)
{
	optionSound = IG::setOrClearBits(optionSound.val, OPTION_SOUND_DURING_FAST_FORWARD_ENABLED_FLAG, on);
}

void EmuApp::setAudioOutputAPI(IG::Audio::Api api)
{
	optionAudioAPI = (uint8_t)api;
	audio().open(api);
}

IG::Audio::Api EmuApp::audioOutputAPI() const
{
	if(IG::used(optionAudioAPI))
		return (IG::Audio::Api)(uint8_t)optionAudioAPI;
	else
		return IG::Audio::Api::DEFAULT;
}

bool EmuApp::setFontSize(int size)
{
	if(!optionFontSize.isValidVal(size))
		return false;
	optionFontSize = size;
	applyFontSize(viewController().emuWindow());
	viewController().placeElements();
	return true;
}

int EmuApp::fontSize() const
{
	return optionFontSize;
}

void EmuApp::configureAppForEmulation(bool running)
{
	appContext().setIdleDisplayPowerSave(running ? idleDisplayPowerSave() : true);
	applyOSNavStyle(appContext(), running);
	appContext().setHintKeyRepeat(!running);
}

void EmuApp::applyFrameRates(bool updateFrameTime)
{
	auto screenFrameTime = viewController().emuWindow().screen()->frameTime();
	system().setFrameTime(VideoSystem::NATIVE_NTSC, frameTime(VideoSystem::NATIVE_NTSC, screenFrameTime));
	system().setFrameTime(VideoSystem::PAL, frameTime(VideoSystem::PAL, screenFrameTime));
	if(updateFrameTime)
		system().configFrameTime(soundRate());
}

void EmuApp::onFocusChange(bool in)
{
	if(viewController().isShowingEmulation())
	{
		if(in && system().isPaused())
		{
			logMsg("resuming emulation due to window focus");
			viewController().inputView().resetInput();
			startEmulation();
		}
		else if(pauseUnfocusedOption() && !system().isPaused() && !allWindowsAreFocused())
		{
			logMsg("pausing emulation with all windows unfocused");
			pauseEmulation();
			viewController().postDrawToEmuWindows();
		}
	}
}

bool EmuApp::allWindowsAreFocused() const
{
	return windowData(appContext().mainWindow()).focused && (!hasExtraWindow(appContext()) || extraWindowIsFocused(appContext()));
}

void EmuApp::setEmuViewOnExtraWindow(bool on, IG::Screen &screen)
{
	auto ctx = appContext();
	if(on && !hasExtraWindow(ctx))
	{
		logMsg("setting emu view on extra window");
		IG::WindowConfig winConf;
		winConf.setScreen(screen);
		winConf.setTitle(ctx.applicationName);
		winConf.setFormat(windowDrawableConfig().pixelFormat);
		auto extraWin = ctx.makeWindow(winConf,
			[this](IG::ApplicationContext ctx, IG::Window &win)
			{
				renderer.attachWindow(win, windowDrawableConfig());
				auto &mainWinData = windowData(ctx.mainWindow());
				auto &extraWinData = win.makeAppData<WindowData>();
				extraWinData.hasPopup = false;
				extraWinData.focused = true;
				if(system().isActive())
				{
					emuSystemTask.pause();
					win.moveOnFrame(ctx.mainWindow(), system().onFrameUpdate, windowFrameClockSource());
					applyFrameRates();
				}
				extraWinData.updateWindowViewport(win, makeViewport(win));
				viewController().moveEmuViewToWindow(win);

				win.setOnSurfaceChange(
					[this](IG::Window &win, IG::Window::SurfaceChange change)
					{
						if(change.resized())
						{
							viewController().updateExtraWindowViewport(win, makeViewport(win), renderer.task());
						}
						renderer.task().updateDrawableForSurfaceChange(win, change);
					});

				win.setOnDraw(
					[this](IG::Window &win, IG::Window::DrawParams params)
					{
						return viewController().drawExtraWindow(win, params, renderer.task());
					});

				win.setOnInputEvent(
					[this](IG::Window &win, const Input::Event &e)
					{
						return viewController().extraWindowInputEvent(e);
					});

				win.setOnFocusChange(
					[this](IG::Window &win, bool in)
					{
						windowData(win).focused = in;
						onFocusChange(in);
					});

				win.setOnDismissRequest(
					[](IG::Window &win)
					{
						win.dismiss();
					});

				win.setOnDismiss(
					[this](IG::Window &win)
					{
						system().resetFrameTime();
						logMsg("setting emu view on main window");
						viewController().moveEmuViewToWindow(appContext().mainWindow());
						viewController().movePopupToWindow(appContext().mainWindow());
						viewController().placeEmuViews();
						mainWindow().postDraw();
						if(system().isActive())
						{
							emuSystemTask.pause();
							mainWindow().moveOnFrame(win, system().onFrameUpdate, windowFrameClockSource());
							applyFrameRates();
						}
					});

				win.show();
				viewController().placeEmuViews();
				mainWindow().postDraw();
			});
		if(!extraWin)
		{
			logErr("error creating extra window");
			return;
		}
	}
	else if(!on && hasExtraWindow(ctx))
	{
		dismissExtraWindow(ctx);
	}
}

void EmuApp::configureSecondaryScreens()
{
	if(showOnSecondScreenOption() && appContext().screens().size() > 1)
	{
		setEmuViewOnExtraWindow(true, *appContext().screens()[1]);
	}
}

IG::OnFrameDelegate EmuApp::onFrameDelayed(int8_t delay)
{
	return [this, delay](IG::FrameParams params)
	{
		if(delay)
		{
			addOnFrameDelegate(onFrameDelayed(delay - 1));
		}
		else
		{
			if(system().isActive())
			{
				viewController().emuWindow().setDrawEventPriority(1); // block UI from posting draws
				addOnFrame();
			}
		}
		return false;
	};
}

void EmuApp::addOnFrameDelegate(IG::OnFrameDelegate onFrame)
{
	viewController().emuWindow().addOnFrame(onFrame, windowFrameClockSource());
}

void EmuApp::addOnFrameDelayed()
{
	// delay before adding onFrame handler to let timestamps stabilize
	auto delay = viewController().emuWindowScreen()->frameRate() / 4;
	//logMsg("delaying onFrame handler by %d frames", onFrameHandlerDelay);
	addOnFrameDelegate(onFrameDelayed(delay));
}

void EmuApp::addOnFrame()
{
	addOnFrameDelegate(system().onFrameUpdate);
}

void EmuApp::removeOnFrame()
{
	viewController().emuWindow().removeOnFrame(system().onFrameUpdate, windowFrameClockSource());
}

MainWindowData &EmuApp::mainWindowData() const
{
	return EmuEx::mainWindowData(appContext().mainWindow());
}

EmuApp &EmuApp::get(IG::ApplicationContext ctx)
{
	return static_cast<EmuApp&>(ctx.application());
}

EmuApp &gApp() { return *gAppPtr; }

IG::ApplicationContext gAppContext() { return gApp().appContext(); }

}
