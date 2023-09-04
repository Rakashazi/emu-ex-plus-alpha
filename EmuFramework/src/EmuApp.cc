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
#include <emuframework/LoadProgressView.hh>
#include <emuframework/EmuVideoLayer.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/FilePicker.hh>
#include <emuframework/MainMenuView.hh>
#include <emuframework/SystemActionsView.hh>
#include <emuframework/SystemOptionView.hh>
#include <emuframework/GUIOptionView.hh>
#include <emuframework/AudioOptionView.hh>
#include <emuframework/VideoOptionView.hh>
#include <emuframework/FilePathOptionView.hh>
#include <emuframework/AppKeyCode.hh>
#include "gui/AutosaveSlotView.hh"
#include "InputDeviceData.hh"
#include "WindowData.hh"
#include "configFile.hh"
#include "EmuOptions.hh"
#include "pathUtils.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/fs/FS.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/io/IO.hh>
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
#include <imagine/bluetooth/BluetoothInputDevScanner.hh>
#include <cmath>

namespace EmuEx
{

constexpr SystemLogger log{"App"};
static EmuApp *gAppPtr{};
[[gnu::weak]] bool EmuApp::hasIcon = true;
[[gnu::weak]] bool EmuApp::needsGlobalInstance = false;
constexpr float pausedVideoBrightnessScale = .75f;

constexpr AssetDesc assetDesc[wise_enum::size<AssetID>]
{
	// arrow, accept, close, more
	{AssetFileID::ui, {{},       {.25, .25}}},
	{AssetFileID::ui, {{.25, 0}, {.5,  .25}}},
	{AssetFileID::ui, {{.5,  0}, {.75, .25}}},
	{AssetFileID::ui, {{.75, 0}, {1.,  .25}}},
	// fast, slow, speed, menu
	{AssetFileID::ui, {{0,   .25}, {.25, .5}}},
	{AssetFileID::ui, {{.25, .25}, {.5,  .5}}},
	{AssetFileID::ui, {{.5,  .25}, {.75, .5}}},
	{AssetFileID::ui, {{.75, .25}, {1.,  .5}}},
	// leftSwitch, rightSwitch, load, save
	{AssetFileID::ui, {{0,   .5}, {.25, .75}}},
	{AssetFileID::ui, {{.25, .5}, {.5,  .75}}},
	{AssetFileID::ui, {{.5,  .5}, {.75, .75}}},
	{AssetFileID::ui, {{.75, .5}, {1.,  .75}}},
	// display, screenshot, openFile
	{AssetFileID::ui, {{0,   .75}, {.25, 1.}}},
	{AssetFileID::ui, {{.25, .75}, {.5,  1.}}},
	{AssetFileID::ui, {{.5,  .75}, {.75, 1.}}},
	{AssetFileID::gamepadOverlay, {{}, {1.f, 1.f}}},
	{AssetFileID::keyboardOverlay, {{}, {1.f, 1.f}}},
};

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
	emuVideoLayer{emuVideo, defaultVideoAspectRatio()},
	emuSystemTask{*this},
	autosaveManager_{*this},
	inputManager{ctx},
	pixmapReader{ctx},
	pixmapWriter{ctx},
	vibrationManager_{ctx},
	perfHintManager{ctx.performanceHintManager()},
	optionFontSize{CFGKEY_FONT_Y_SIZE,
		Config::MACHINE_IS_PANDORA ? 6500 :
		(Config::envIsIOS || Config::envIsAndroid) ? 3000 :
		8000,
		false, optionIsValidWithMinMax<2000, 10000, uint16_t>},
	optionPauseUnfocused{CFGKEY_PAUSE_UNFOCUSED, 1,
		!(Config::envIsLinux || Config::envIsAndroid)},
	optionConfirmOverwriteState{CFGKEY_CONFIRM_OVERWRITE_STATE, 1},
	optionNotificationIcon{CFGKEY_NOTIFICATION_ICON, 1, !Config::envIsAndroid},
	optionTitleBar{CFGKEY_TITLE_BAR, 1, !CAN_HIDE_TITLE_BAR},
	optionSystemActionsIsDefaultMenu{CFGKEY_SYSTEM_ACTIONS_IS_DEFAULT_MENU, 1},
	optionIdleDisplayPowerSave{CFGKEY_IDLE_DISPLAY_POWER_SAVE, 0},
	optionLowProfileOSNav{CFGKEY_LOW_PROFILE_OS_NAV, 1, !Config::envIsAndroid},
	optionHideOSNav{CFGKEY_HIDE_OS_NAV, 0, !Config::envIsAndroid},
	optionHideStatusBar{CFGKEY_HIDE_STATUS_BAR, 1, !Config::envIsAndroid && !Config::envIsIOS},
	optionShowBundledGames{CFGKEY_SHOW_BUNDLED_GAMES, 1},
	optionShowBluetoothScan{CFGKEY_SHOW_BLUETOOTH_SCAN, 1},
	optionImgFilter{CFGKEY_GAME_IMG_FILTER, 1, 0},
	optionImgEffect{CFGKEY_IMAGE_EFFECT, 0, 0, optionIsValidWithMax<std::to_underlying(lastEnum<ImageEffectId>)>},
	optionImageEffectPixelFormat{CFGKEY_IMAGE_EFFECT_PIXEL_FORMAT, IG::PIXEL_NONE, 0, imageEffectPixelFormatIsValid},
	optionOverlayEffect{CFGKEY_OVERLAY_EFFECT, 0, 0, optionIsValidWithMax<std::to_underlying(lastEnum<ImageOverlayId>)>},
	optionOverlayEffectLevel{CFGKEY_OVERLAY_EFFECT_LEVEL, 75, 0, optionIsValidWithMax<100>},
	optionFrameInterval{CFGKEY_FRAME_INTERVAL, 1, false, optionIsValidWithMinMax<0, 4, uint8_t>},
	optionImageZoom(CFGKEY_IMAGE_ZOOM, 100, 0, optionImageZoomIsValid),
	optionViewportZoom(CFGKEY_VIEWPORT_ZOOM, 100, 0, optionIsValidWithMinMax<50, 100>),
	optionShowOnSecondScreen{CFGKEY_SHOW_ON_2ND_SCREEN, 0},
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
	onEvent = [this](ApplicationContext ctx, ApplicationEvent appEvent)
	{
		visit(overloaded
		{
			[&](InterProcessMessageEvent &e)
			{
				log.info("got IPC path:{}", e.filename);
				system().setInitialLoadPath(e.filename);
			},
			[](auto &) {}
		}, appEvent);
	};
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
		if(e.keyEvent() && e.keyEvent()->pushed(Input::DefaultKey::CANCEL))
		{
			if(!e.keyEvent()->repeated())
			{
				appContext().exit();
			}
			return true;
		}
		return AlertView::inputEvent(e);
	}
};

Gfx::TextureSpan EmuApp::asset(AssetID assetID) const
{
	assumeExpr(to_underlying(assetID) < wise_enum::size<AssetID>);
	return asset(assetDesc[to_underlying(assetID)]);
}

Gfx::TextureSpan EmuApp::asset(AssetDesc desc) const
{
	auto &res = assetBuffImg[desc.fileIdx()];
	if(!res)
	{
		try
		{
			res = renderer.makeTexture(pixmapReader.loadAsset(desc.filename()), View::imageSamplerConfig);
		}
		catch(...)
		{
			log.error("error loading asset:{}", desc.filename());
		}
	}
	return {&res, desc.texBounds};
}

Gfx::TextureSpan EmuApp::collectTextCloseAsset() const
{
	return Config::envIsAndroid ? Gfx::TextureSpan{} : asset(AssetID::close);
}

EmuViewController &EmuApp::viewController() { return mainWindowData().viewController; }
const EmuViewController &EmuApp::viewController() const { return mainWindowData().viewController; }
const Screen &EmuApp::emuScreen() const { return *viewController().emuWindowScreen(); }
Window &EmuApp::emuWindow() { return viewController().emuWindow(); }

void EmuApp::setCPUNeedsLowLatency(IG::ApplicationContext ctx, bool needed)
{
	#ifdef __ANDROID__
	if(useNoopThread)
		ctx.setNoopThreadActive(needed);
	#endif
	if(useSustainedPerformanceMode)
		ctx.setSustainedPerformanceMode(needed);
	applyCPUAffinity(needed);
}

static void suspendEmulation(EmuApp &app)
{
	if(!app.system().hasContent())
		return;
	app.autosaveManager().save();
	app.system().flushBackupMemory(app);
}

void EmuApp::closeSystem()
{
	showUI();
	emuSystemTask.stop();
	system().closeRuntimeSystem(*this);
	autosaveManager_.resetSlot();
	viewController().onSystemClosed();
}

void EmuApp::closeSystemWithoutSave()
{
	autosaveManager_.resetSlot(noAutosaveName);
	closeSystem();
}

void EmuApp::applyOSNavStyle(IG::ApplicationContext ctx, bool inGame)
{
	SystemUIStyleFlags flags;
	if((int)optionLowProfileOSNav > (inGame ? 0 : 1))
		flags.dimNavigation = true;
	if((int)optionHideOSNav > (inGame ? 0 : 1))
		flags.hideNavigation = true;
	if((int)optionHideStatusBar > (inGame ? 0 : 1))
		flags.hideStatus = true;
	ctx.setSysUIStyle(flags);
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
	log.info("starting content from command line:{}", launchPath);
	return launchPath;
}

bool EmuApp::setWindowDrawableConfig(Gfx::DrawableConfig conf)
{
	windowDrawableConf = conf;
	auto ctx = appContext();
	for(auto &w : ctx.windows())
	{
		if(!renderer.setDrawableConfig(*w, conf))
			return false;
	}
	applyRenderPixelFormat();
	return true;
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
		log.info("Using RGB565 render format since emulated system can't render RGBA8888");
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
	log.info("updating video with current framebuffer content");
	system().renderFramebuffer(video);
}

static bool supportsVideoImageBuffersOption(const Gfx::Renderer &r)
{
	return r.supportsSyncFences() && r.maxSwapChainImages() > 2;
}

void EmuApp::startAudio()
{
	audio().start(system().frameTime());
}

void EmuApp::updateLegacySavePath(IG::ApplicationContext ctx, CStringView path)
{
	auto oldSaveSubDirs = subDirectoryStrings(ctx, path);
	if(oldSaveSubDirs.empty())
	{
		log.info("no legacy save folders in:{}", path);
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

static SteadyClockTime targetFrameTime(const Screen &s)
{
	auto total = s.frameTime() - s.presentationDeadline();
	auto lowerBound = Milliseconds{1};
	if(total < lowerBound)
		total = lowerBound;
	return total;
}

void EmuApp::mainInitCommon(IG::ApplicationInitParams initParams, IG::ApplicationContext ctx)
{
	auto appConfig = loadConfigFile(ctx);
	system().onOptionsLoaded();
	loadSystemOptions();
	updateLegacySavePathOnStoragePath(ctx, system());
	if(auto launchGame = parseCommandArgs(initParams.commandArgs());
		launchGame)
		system().setInitialLoadPath(launchGame);
	audioManager().setMusicVolumeControlHint();
	if(!renderer.supportsColorSpace())
		windowDrawableConf.colorSpace = {};
	applyOSNavStyle(ctx, false);

	ctx.addOnResume(
		[this](IG::ApplicationContext ctx, bool focused)
		{
			audioManager().startSession();
			emuAudio.open();
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
					auto title = std::format("{} was suspended", ctx.applicationName);
					ctx.addNotification(title, title, system().contentDisplayName());
				}
			}
			emuAudio.close();
			audioManager().endSession();
			saveConfigFile(ctx);
			saveSystemOptions();
			#ifdef CONFIG_INPUT_BLUETOOTH
			if(bta && (!backgrounded || (backgrounded && !keepBluetoothActive)))
				closeBluetoothConnections();
			#endif
			onEvent(ctx, FreeCachesEvent{false});
			return true;
		});

	IG::WindowConfig winConf{ .title = ctx.applicationName };
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
			viewManager.defaultFace = {renderer, fontManager.makeSystem(), fontSettings(win)};
			viewManager.defaultBoldFace = {renderer, fontManager.makeBoldSystem(), fontSettings(win)};
			ViewAttachParams viewAttach{viewManager, win, renderer.task()};
			auto &vController = inputManager.vController;
			auto &winData = win.makeAppData<MainWindowData>(viewAttach, vController, emuVideoLayer, system());
			winData.updateWindowViewport(win, makeViewport(win), renderer);
			win.setAcceptDnd(true);
			renderer.setWindowValidOrientations(win, menuOrientation());
			inputManager.updateInputDevices(ctx);
			vController.configure(win, renderer, viewManager.defaultFace);
			if(EmuSystem::inputHasKeyboard)
			{
				vController.setKeyboardImage(asset(AssetID::keyboardOverlay));
			}
			auto &screen = *win.screen();
			if(!screen.supportsTimestamps() && (!Config::envIsLinux || screen.frameRate() < 100.))
			{
				windowFrameTimeSource = WindowFrameTimeSource::RENDERER;
			}
			else
			{
				windowFrameTimeSource = WindowFrameTimeSource::SCREEN;
			}
			log.info("timestamp source:{}", windowFrameTimeSource == WindowFrameTimeSource::RENDERER ? "renderer" : "screen");
			winData.viewController.placeElements();
			winData.viewController.pushAndShowMainMenu(viewAttach, emuVideoLayer, emuAudio);
			configureSecondaryScreens();
			emuVideo.setOnFormatChanged(
				[this, &viewController = winData.viewController](EmuVideo &)
				{
					emuVideoLayer.onVideoFormatChanged(videoEffectPixelFormat());
					viewController.placeEmuViews();
				});
			emuVideo.setRendererTask(renderer.task());
			emuVideo.setTextureBufferMode(system(), (Gfx::TextureBufferMode)optionTextureBufferMode.val);
			emuVideo.setImageBuffers(optionVideoImageBuffers);
			emuVideoLayer.setLinearFilter(optionImgFilter); // init the texture sampler before setting format
			applyRenderPixelFormat();
			emuVideoLayer.setOverlay((ImageOverlayId)optionOverlayEffect.val);
			emuVideoLayer.setOverlayIntensity(optionOverlayEffectLevel / 100.f);
			emuVideoLayer.setEffect(system(), (ImageEffectId)optionImgEffect.val, videoEffectPixelFormat());
			emuVideoLayer.setZoom(optionImageZoom);
			system().onFrameUpdate = [this, &viewController = winData.viewController](IG::FrameParams params)
				{
					bool skipForward = false;
					bool altSpeed = false;
					auto &audio = this->audio();
					auto &sys = system();
					auto &win = viewController.emuWindow();
					if(sys.shouldFastForward()) [[unlikely]]
					{
						// for skipping loading on disk-based computers
						altSpeed = true;
						skipForward = true;
						sys.setSpeedMultiplier(audio, 8.);
					}
					else
					{
						altSpeed = sys.targetSpeed != 1.;
						sys.setSpeedMultiplier(audio, sys.targetSpeed);
					}
					auto frameInfo = sys.advanceFramesWithTime(params.timestamp);
					if(!frameInfo.advanced)
					{
						if(enableBlankFrameInsertion)
						{
							viewController.drawBlankFrame = true;
							win.postDraw(1);
						}
						return true;
					}
					int interval = frameInterval();
					auto videoPtr = &this->video();
					if(frameInfo.advanced + savedAdvancedFrames < interval)
					{
						// running at a lower target fps, skip current frames
						savedAdvancedFrames += frameInfo.advanced;
						videoPtr = {};
					}
					else
					{
						savedAdvancedFrames = 0;
					}
					if(videoPtr)
					{
						if(win.isReady())
						{
							if(showFrameTimeStats)
								viewController.emuView.updateFrameTimeStats(frameTimeStats, params.timestamp);
							record(FrameTimeStatEvent::startOfFrame, params.timestamp);
							record(FrameTimeStatEvent::startOfEmulation);
						}
						else
						{
							//log.debug("previous async frame not ready yet");
							doIfUsed(frameTimeStats, [&](auto &stats) { stats.missedFrameCallbacks++; });
						}
						win.setDrawEventPriority(Window::drawEventPriorityLocked);
					}
					EmuAudio *audioPtr = audio ? &audio : nullptr;
					runTurboInputEvents();
					emuSystemTask.runFrame(videoPtr, audioPtr, frameInfo.advanced, skipForward, altSpeed);
					if(videoPtr)
					{
						if(usePresentationTime)
							viewController.presentTime = params.presentTime(interval);
						doIfUsed(frameStartTimePoint, [&](auto &tp)
						{
							if(!hasTime(tp))
								tp = params.timestamp;
						});
					}
					return true;
				};

			win.onEvent = [this](Window &win, WindowEvent winEvent)
			{
				return visit(overloaded
				{
					[&](Input::Event &e) { return viewController().inputEvent(e); },
					[&](DrawEvent &e)
					{
						record(FrameTimeStatEvent::startOfDraw);
						auto reportTime = scopeGuard([&]
						{
							if(viewController().isShowingEmulation())
								reportFrameWorkTime();
						});
						return viewController().drawMainWindow(win, e.params, renderer.task());
					},
					[&](WindowSurfaceChangeEvent &e)
					{
						if(e.change.resized())
						{
							viewController().updateMainWindowViewport(win, makeViewport(win), renderer.task());
						}
						renderer.task().updateDrawableForSurfaceChange(win, e.change);
						return true;
					},
					[&](DragDropEvent &e)
					{
						log.info("got DnD:{}", e.filename);
						handleOpenFileCommand(e.filename);
						return true;
					},
					[&](FocusChangeEvent &e)
					{
						windowData(win).focused = e.in;
						onFocusChange(e.in);
						return true;
					},
					[](auto &){ return false; }
				}, winEvent);
			};

			onMainWindowCreated(viewAttach, ctx.defaultInputEvent());

			onEvent = [this](ApplicationContext ctx, ApplicationEvent appEvent)
			{
				visit(overloaded
				{
					[&](InterProcessMessageEvent &e)
					{
						log.info("got IPC path:{}", e.filename);
						handleOpenFileCommand(e.filename);
					},
					[&](ScreenChangeEvent &e)
					{
						if(e.change == ScreenChange::added)
						{
							log.info("screen added");
							if(showOnSecondScreenOption() && ctx.screens().size() > 1)
								setEmuViewOnExtraWindow(true, e.screen);
						}
						else if(e.change == ScreenChange::removed)
						{
							log.info("screen removed");
							if(hasExtraWindow(appContext()) && *extraWindowScreen(appContext()) == e.screen)
								setEmuViewOnExtraWindow(false, e.screen);
						}
						else if(e.change == ScreenChange::frameRate && e.screen == emuScreen())
						{
							if(viewController().isShowingEmulation())
							{
								if(perfHintSession)
								{
									auto targetTime = targetFrameTime(e.screen);
									perfHintSession.updateTargetWorkTime(targetTime);
									log.info("updated performance hint session with target time:{}", targetTime);
								}
								syncEmulationThread();
								configFrameTime();
							}
						}
					},
					[&](Input::DevicesEnumeratedEvent &)
					{
						log.info("input devs enumerated");
						inputManager.updateInputDevices(ctx);
					},
					[&](Input::DeviceChangeEvent &e)
					{
						log.info("got input dev change");
						inputManager.updateInputDevices(ctx);
						if(notifyOnInputDeviceChange && (e.change == Input::DeviceChange::added || e.change == Input::DeviceChange::removed))
						{
							postMessage(2, 0, std::format("{} {}", inputDevData(e.device).displayName, e.change == Input::DeviceChange::added ? "connected" : "disconnected"));
						}
						else if(e.change == Input::DeviceChange::connectError)
						{
							postMessage(2, 1, std::format("{} had a connection error", e.device.name()));
						}
						viewController().onInputDevicesChanged();
					},
					[&](FreeCachesEvent &e)
					{
						viewManager.defaultFace.freeCaches();
						viewManager.defaultBoldFace.freeCaches();
						if(e.running)
							viewController().prepareDraw();
					},
					[](auto &) {}
				}, appEvent);
			};

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
									log.info("resuming emulation due to app resume");
									viewController().inputView.resetInput();
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
		WPt viewCenter{viewRect.xSize() / 2, viewRect.ySize() / 2};
		viewRect -= viewCenter;
		viewRect *= optionViewportZoom / 100.f;
		viewRect += viewCenter;
	}
	return win.viewport(viewRect);
}

void WindowData::updateWindowViewport(const IG::Window &win, IG::Viewport viewport, const IG::Gfx::Renderer &r)
{
	windowRect = viewport.bounds();
	contentRect = viewport.bounds().intersection(win.contentBounds());
	projM = Gfx::Mat4::makePerspectiveFovRH(M_PI/4.0, viewport.realAspectRatio(), .1f, 100.f)
		.projectionPlane(viewport, .5f, r.projectionRollAngle(win));
}

void EmuApp::launchSystem(const Input::Event &e)
{
	if(autosaveManager_.autosaveLaunchMode == AutosaveLaunchMode::Ask)
	{
		autosaveManager_.resetSlot(noAutosaveName);
		viewController().pushAndShow(EmuApp::makeView(attachParams(), EmuApp::ViewID::SYSTEM_ACTIONS), e);
		viewController().pushAndShow(std::make_unique<AutosaveSlotView>(attachParams()), e);
	}
	else
	{
		auto loadMode = autosaveManager_.autosaveLaunchMode == AutosaveLaunchMode::LoadNoState ? LoadAutosaveMode::NoState : LoadAutosaveMode::Normal;
		if(autosaveManager_.autosaveLaunchMode == AutosaveLaunchMode::NoSave)
			autosaveManager_.resetSlot(noAutosaveName);
		static auto finishLaunch = [](EmuApp &app, LoadAutosaveMode mode)
		{
			app.autosaveManager_.load(mode);
			if(!app.system().hasContent())
			{
				log.error("system was closed while trying to load autosave");
				return;
			}
			app.showEmulation();
		};
		auto stateIsOlderThanBackupMemory = [&]
		{
			auto stateTime = autosaveManager_.stateTime();
			return hasTime(stateTime) && stateTime < autosaveManager_.backupMemoryTime();
		};
		if(system().usesBackupMemory() && loadMode == LoadAutosaveMode::Normal &&
			!autosaveManager_.saveOnlyBackupMemory && stateIsOlderThanBackupMemory())
		{
			viewController().pushAndShowModal(std::make_unique<YesNoAlertView>(attachParams(),
				"Autosave state timestamp is older than the contents of backup memory, really load it even though progress may be lost?",
				YesNoAlertView::Delegates
				{
					.onYes = [this]{ finishLaunch(*this, LoadAutosaveMode::Normal); },
					.onNo = [this]{ finishLaunch(*this, LoadAutosaveMode::NoState); }
				}), e, false);
		}
		else
		{
			finishLaunch(*this, loadMode);
		}
	}
}

void EmuApp::onSelectFileFromPicker(IO io, CStringView path, std::string_view displayName,
	const Input::Event &e, EmuSystemCreateParams params, ViewAttachParams attachParams)
{
	createSystemWithMedia(std::move(io), path, displayName, e, params, attachParams,
		[this](const Input::Event &e)
		{
			recentContent.add(system());
			launchSystem(e);
		});
}

void EmuApp::handleOpenFileCommand(CStringView path)
{
	auto name = appContext().fileUriDisplayName(path);
	if(name.empty())
	{
		postErrorMessage(std::format("Can't access path name for:\n{}", path));
		return;
	}
	if(!IG::isUri(path) && FS::status(path).type() == FS::file_type::directory)
	{
		log.info("changing to dir {} from external command", path);
		showUI(false);
		viewController().popToRoot();
		setContentSearchPath(path);
		viewController().pushAndShow(
			FilePicker::forLoading(attachParams(), appContext().defaultInputEvent()),
			appContext().defaultInputEvent(),
			false);
		return;
	}
	log.info("opening file {} from external command", path);
	showUI();
	viewController().popToRoot();
	onSelectFileFromPicker({}, path, name, Input::KeyEvent{}, {}, attachParams());
}

void EmuApp::runBenchmarkOneShot(EmuVideo &emuVideo)
{
	log.info("starting benchmark");
	auto time = system().benchmark(emuVideo);
	autosaveManager_.resetSlot(noAutosaveName);
	closeSystem();
	log.info("done in:{}", duration_cast<FloatSeconds>(time));
	postMessage(2, 0, std::format("{:.2f} fps", 180. / time.count()));
}

void EmuApp::showEmulation()
{
	if(viewController().isShowingEmulation() || !system().hasContent())
		return;
	configureAppForEmulation(true);
	resetInput();
	inputManager.vController.applySavedButtonAlpha();
	viewController().showEmulationView(configFrameTime());
	startEmulation();
}

void EmuApp::startEmulation()
{
	if(!viewController().isShowingEmulation())
		return;
	emuVideoLayer.setBrightness(videoBrightnessRGB);
	video().setOnFrameFinished(
		[&, &viewController = viewController()](EmuVideo &)
		{
			auto &win = viewController.emuWindow();
			record(FrameTimeStatEvent::aboutToPostDraw);
			win.setDrawEventPriority(1);
			win.postDraw(1);
		});
	frameTimeStats = {};
	emuSystemTask.start();
	setCPUNeedsLowLatency(appContext(), true);
	system().start(*this);
	addOnFrameDelayed();
}

void EmuApp::showUI(bool updateTopView)
{
	if(!viewController().isShowingEmulation())
		return;
	pauseEmulation();
	configureAppForEmulation(false);
	emuVideoLayer.setBrightness(videoBrightnessRGB * menuVideoBrightnessScale);
	viewController().showMenuView(updateTopView);
}

void EmuApp::pauseEmulation()
{
	setCPUNeedsLowLatency(appContext(), false);
	emuSystemTask.pause();
	video().setOnFrameFinished([](EmuVideo &){});
	system().pause(*this);
	setRunSpeed(1.);
	emuVideoLayer.setBrightness(videoBrightnessRGB * pausedVideoBrightnessScale);
	viewController().emuWindow().setDrawEventPriority();
	removeOnFrame();
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
	pushAndShowModalView(std::make_unique<YesNoAlertView>(attach, label, choice1, choice2, YesNoAlertView::Delegates{onYes, onNo}), e);
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
		if(autosaveManager_.slotName() != noAutosaveName)
			system().loadBackupMemory(*this);
		showEmulation();
	}
	catch(...)
	{
		log.error("Error reloading system");
		system().clearGamePaths();
	}
}

void EmuApp::onSystemCreated()
{
	updateContentRotation();
	viewController().onSystemCreated();
}

void EmuApp::promptSystemReloadDueToSetOption(ViewAttachParams attach, const Input::Event &e, EmuSystemCreateParams params)
{
	if(!system().hasContent())
		return;
	viewController().pushAndShowModal(std::make_unique<YesNoAlertView>(attach,
		"This option takes effect next time the system starts. Restart it now?",
		YesNoAlertView::Delegates{ .onYes = [this, params] { reloadSystem(params); } }), e, false);
}

void EmuApp::unpostMessage()
{
	viewController().popup.clear();
}

void EmuApp::printScreenshotResult(bool success)
{
	postMessage(3, !success, std::format("{}{}",
		success ? "Wrote screenshot at " : "Error writing screenshot at ",
		appContext().formatDateAndTime(WallClock::now())));
}

void EmuApp::createSystemWithMedia(IO io, CStringView path, std::string_view displayName,
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
	auto loadProgressView = std::make_unique<LoadProgressView>(attachParams, e, onComplete);
	auto &msgPort = loadProgressView->messagePort();
	pushAndShowModalView(std::move(loadProgressView), e);
	auto ctx = attachParams.appContext();
	IG::makeDetachedThread(
		[this, io{std::move(io)}, pathStr = FS::PathString{path}, nameStr = FS::FileString{displayName}, &msgPort, params]() mutable
		{
			log.info("starting loader thread");
			try
			{
				system().createWithMedia(std::move(io), pathStr, nameStr, params,
					[&msgPort](int pos, int max, const char *label)
					{
						int len = label ? std::string_view{label}.size() : -1;
						auto msg = EmuSystem::LoadProgressMessage{EmuSystem::LoadProgress::UPDATE, pos, max, len};
						msgPort.sendWithExtraData(msg, std::span{label, len > 0 ? size_t(len) : 0});
						return true;
					});
				msgPort.send({EmuSystem::LoadProgress::OK, 0, 0, 0});
				log.info("loader thread finished");
			}
			catch(std::exception &err)
			{
				system().clearGamePaths();
				std::string_view errStr{err.what()};
				auto len = errStr.size();
				if(len > 1024)
				{
					log.warn("truncating long error size:{}", len);
					len = 1024;
				}
				msgPort.sendWithExtraData({EmuSystem::LoadProgress::FAILED, 0, 0, int(len)}, std::span{errStr.data(), len});
				log.error("loader thread failed");
				return;
			}
		});
}

FS::PathString EmuApp::contentSavePath(std::string_view name) const
{
	auto slotName = autosaveManager_.slotName();
	if(slotName.size() && slotName != noAutosaveName)
		return system().contentLocalSaveDirectory(slotName, name);
	else
		return system().contentSavePath(name);
}

FS::PathString EmuApp::contentSaveFilePath(std::string_view ext) const
{
	auto slotName = autosaveManager_.slotName();
	if(slotName.size() && slotName != noAutosaveName)
		return system().contentLocalSaveDirectory(slotName, FS::FileString{"auto"}.append(ext));
	else
		return system().contentSaveFilePath(ext);
}

bool EmuApp::saveState(CStringView path)
{
	if(!system().hasContent())
	{
		postErrorMessage("System not running");
		return false;
	}
	syncEmulationThread();
	log.info("saving state {}", path);
	try
	{
		system().saveState(path);
		return true;
	}
	catch(std::exception &err)
	{
		postErrorMessage(4, std::format("Can't save state:\n{}", err.what()));
		return false;
	}
}

bool EmuApp::saveStateWithSlot(int slot)
{
	return saveState(system().statePath(slot));
}

bool EmuApp::loadState(CStringView path)
{
	if(!system().hasContent()) [[unlikely]]
	{
		postErrorMessage("System not running");
		return false;
	}
	log.info("loading state {}", path);
	syncEmulationThread();
	try
	{
		system().loadState(*this, path);
		autosaveManager_.resetTimer();
		return true;
	}
	catch(std::exception &err)
	{
		if(system().hasContent() && !hasWriteAccessToDir(system().contentSaveDirectory()))
			postErrorMessage(8, "Save folder inaccessible, please set it in Options➔File Paths➔Saves");
		else
			postErrorMessage(4, std::format("Can't load state:\n{}", err.what()));
		return false;
	}
}

bool EmuApp::loadStateWithSlot(int slot)
{
	assert(slot != -1);
	return loadState(system().statePath(slot));
}

FS::PathString EmuApp::contentSearchPath(std::string_view name) const
{
	return FS::uriString(contentSearchPath_, name);
}

void EmuApp::setContentSearchPath(std::string_view path)
{
	contentSearchPath_ = path;
}

FS::PathString EmuApp::validSearchPath(const FS::PathString &path) const
{
	auto ctx = appContext();
	if(path.empty())
		return contentSearchPath();
	return hasArchiveExtension(path) ? FS::dirnameUri(path) : path;
}

[[gnu::weak]] void EmuApp::onMainWindowCreated(ViewAttachParams, const Input::Event &) {}

[[gnu::weak]] void EmuApp::onCustomizeNavView(EmuApp::NavView &) {}

[[gnu::weak]] std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, EmuApp::ViewID id)
{
	return nullptr;
}

bool EmuApp::handleKeyInput(KeyInfo keyInfo, const Input::Event &srcEvent)
{
	if(!keyInfo.flags.appCode)
	{
		handleSystemKeyInput(keyInfo, srcEvent.state(), srcEvent.metaKeyBits());
	}
	else
	{
		for(auto c : keyInfo.codes)
		{
			if(handleAppActionKeyInput({c, keyInfo.flags, srcEvent.state(), srcEvent.metaKeyBits()}, srcEvent))
				return true;
		}
	}
	return false;
}

bool EmuApp::handleAppActionKeyInput(InputAction action, const Input::Event &srcEvent)
{
	bool isPushed = action.state == Input::Action::PUSHED;
	assert(action.flags.appCode);
	using enum AppKeyCode;
	switch(AppKeyCode(action.code))
	{
		case fastForward:
		{
			viewController().inputView.setAltSpeedMode(AltSpeedMode::fast, isPushed);
			break;
		}
		case openContent:
		{
			if(!isPushed)
				break;
			log.info("show load game view from key event");
			viewController().popToRoot();
			viewController().pushAndShow(FilePicker::forLoading(attachParams(), srcEvent), srcEvent, false);
			return true;
		}
		case openSystemActions:
		{
			if(!isPushed)
				break;
			log.info("show system actions view from key event");
			showSystemActionsViewFromSystem(attachParams(), srcEvent);
			return true;
		}
		case saveState:
		{
			if(!isPushed)
				break;
			static auto doSaveState = [](EmuApp &app, bool notify)
			{
				if(app.saveStateWithSlot(app.system().stateSlot()) && notify)
				{
					app.postMessage("State Saved");
				}
			};
			if(shouldOverwriteExistingState())
			{
				syncEmulationThread();
				doSaveState(*this, confirmOverwriteStateOption());
			}
			else
			{
				viewController().pushAndShowModal(std::make_unique<YesNoAlertView>(attachParams(), "Really Overwrite State?",
					YesNoAlertView::Delegates
					{
						.onYes = [this]
						{
							doSaveState(*this, false);
							showEmulation();
						},
						.onNo = [this]{ showEmulation(); }
					}), srcEvent, false);
			}
			return true;
		}
		case loadState:
		{
			if(!isPushed)
				break;
			syncEmulationThread();
			loadStateWithSlot(system().stateSlot());
			return true;
		}
		case decStateSlot:
		{
			if(!isPushed)
				break;
			system().decStateSlot();
			postMessage(1, false, std::format("State Slot: {}", system().stateSlotName()));
			return true;
		}
		case incStateSlot:
		{
			if(!isPushed)
				break;
			system().incStateSlot();
			postMessage(1, false, std::format("State Slot: {}", system().stateSlotName()));
			return true;
		}
		case takeScreenshot:
		{
			if(!isPushed)
				break;
			video().takeGameScreenshot();
			return true;
		}
		case toggleFastForward:
		{
			if(!isPushed)
				break;
			viewController().inputView.toggleAltSpeedMode(AltSpeedMode::fast);
			break;
		}
		case openMenu:
		{
			if(!isPushed)
				break;
			log.info("show last view from key event");
			showLastViewFromSystem(attachParams(), srcEvent);
			return true;
		}
		case turboModifier:
		{
			inputManager.turboModifierActive = isPushed;
			if(!isPushed)
				inputManager.turboActions = {};
			break;
		}
		case exitApp:
		{
			if(!isPushed)
				break;
			viewController().pushAndShowModal(std::make_unique<YesNoAlertView>(attachParams(), "Really Exit?",
				YesNoAlertView::Delegates{.onYes = [this]{ appContext().exit(); }}), srcEvent, false);
			break;
		}
		case slowMotion:
		{
			viewController().inputView.setAltSpeedMode(AltSpeedMode::slow, isPushed);
			break;
		}
		case toggleSlowMotion:
		{
			if(!isPushed)
				break;
			viewController().inputView.toggleAltSpeedMode(AltSpeedMode::slow);
			break;
		}
	}
	return false;
}

void EmuApp::handleSystemKeyInput(KeyInfo keyInfo, Input::Action act, uint32_t metaState)
{
	if(inputManager.turboModifierActive && std::ranges::all_of(keyInfo.codes, allowsTurboModifier))
		keyInfo.flags.turbo = 1;
	if(keyInfo.flags.toggle)
	{
		inputManager.toggleInput.updateEvent(*this, keyInfo, act);
	}
	else if(keyInfo.flags.turbo)
	{
		inputManager.turboActions.updateEvent(*this, keyInfo, act);
	}
	else
	{
		for(auto code : keyInfo.codes)
		{
			system().handleInputAction(this, {code, keyInfo.flags, act, metaState});
		}
		defaultVController().updateSystemKeys(keyInfo, act == Input::Action::PUSHED);
	}
}

void EmuApp::runTurboInputEvents()
{
	assert(system().hasContent());
	inputManager.turboActions.update(*this);
}

void EmuApp::resetInput()
{
	inputManager.turboModifierActive = false;
	inputManager.turboActions = {};
	setRunSpeed(1.);
}

void EmuApp::setRunSpeed(double speed)
{
	assumeExpr(speed > 0.);
	bool altSpeedActive = speed != 1.;
	system().targetSpeed = speed;
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
		auto configFile = ctx.openFileUri(configFilePath, OpenFlags::newFile());
		writeConfigHeader(configFile);
		system().writeConfig(ConfigType::SESSION, configFile);
		system().resetSessionOptionsSet();
		if(configFile.size() == 1)
		{
			// delete file if only header was written
			configFile = {};
			ctx.removeFileUri(configFilePath);
			log.info("deleted empty session config file:{}", configFilePath);
		}
		else
		{
			log.info("wrote session config file:{}", configFilePath);
		}
	}
	catch(...)
	{
		log.info("error creating session config file:{}", configFilePath);
	}
}

void EmuApp::loadSessionOptions()
{
	if(!system().resetSessionOptions(*this))
		return;
	if(readConfigKeys(FileUtils::bufferFromUri(appContext(), sessionConfigPath(), {.test = true}),
		[this](uint16_t key, uint16_t size, auto &io)
		{
			switch(key)
			{
				default:
				{
					if(!system().readConfig(ConfigType::SESSION, io, key, size))
					{
						log.info("skipping unknown key {}", key);
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
	readConfigKeys(FileUtils::bufferFromPath(FS::pathString(appContext().supportPath(), configName), {.test = true}),
		[this](uint16_t key, uint16_t size, auto &io)
		{
			if(!system().readConfig(ConfigType::CORE, io, key, size))
			{
				log.info("skipping unknown system config key:{}", key);
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
		auto configFile = FileIO{configFilePath, OpenFlags::newFile()};
		saveSystemOptions(configFile);
		if(configFile.size() == 1)
		{
			// delete file if only header was written
			configFile = {};
			FS::remove(configFilePath);
			log.info("deleted empty system config file");
		}
	}
	catch(...)
	{
		log.error("error writing system config file");
	}
}

void EmuApp::saveSystemOptions(FileIO &configFile)
{
	writeConfigHeader(configFile);
	system().writeConfig(ConfigType::CORE, configFile);
}

void EmuApp::syncEmulationThread()
{
	emuSystemTask.pause();
}

FrameTimeConfig EmuApp::configFrameTime()
{
	auto supportedRates = overrideScreenFrameRate ? std::span<const FrameRate>{&overrideScreenFrameRate, 1} : emuScreen().supportedFrameRates();
	auto frameTimeConfig = outputTimingManager.frameTimeConfig(system(), supportedRates);
	system().configFrameTime(emuAudio.format().rate, frameTimeConfig.time);
	return frameTimeConfig;
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
			system().setSpeedMultiplier(*audio, 1.);
		}
	}
	else
	{
		skipFrames(taskCtx, frames - 1, audio);
	}
	system().runFrame(taskCtx, video, audio);
	system().updateBackupMemoryCounter();
}

void EmuApp::skipFrames(EmuSystemTaskContext taskCtx, int frames, EmuAudio *audio)
{
	assert(system().hasContent());
	for(auto i : iotaCount(frames))
	{
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
			log.info("skip-forward ended early after {} frame(s)", i);
			return false;
		}
	}
	return true;
}

bool EmuApp::writeScreenshot(IG::PixmapView pix, CStringView path)
{
	return pixmapWriter.writeToFile(pix, path);
}

FS::PathString EmuApp::makeNextScreenshotFilename()
{
	static constexpr std::string_view subDirName = "screenshots";
	auto &sys = system();
	auto userPath = sys.userPath(userScreenshotPath);
	sys.createContentLocalDirectory(userPath, subDirName);
	return sys.contentLocalDirectory(userPath, subDirName,
		appContext().formatDateAndTimeAsFilename(WallClock::now()).append(".png"));
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

ViewAttachParams EmuApp::attachParams()
{
	return viewController().inputView.attachParams();
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

void EmuApp::setIntendedFrameRate(Window &win, FrameTimeConfig config)
{
	enableBlankFrameInsertion = false;
	if(allowBlankFrameInsertion && config.refreshMultiplier > 1 && frameInterval() <= 1)
	{
		enableBlankFrameInsertion = true;
		if(!overrideScreenFrameRate)
		{
			config.rate *= config.refreshMultiplier;
			log.info("Multiplied intended frame rate to:{:g}", config.rate);
		}
	}
	return win.setIntendedFrameRate(overrideScreenFrameRate ? FrameRate(overrideScreenFrameRate) : config.rate);
}

void EmuApp::onFocusChange(bool in)
{
	if(viewController().isShowingEmulation())
	{
		if(in && system().isPaused())
		{
			log.info("resuming emulation due to window focus");
			viewController().inputView.resetInput();
			startEmulation();
		}
		else if(pauseUnfocusedOption() && !system().isPaused() && !allWindowsAreFocused())
		{
			log.info("pausing emulation with all windows unfocused");
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
		log.info("setting emu view on extra window");
		IG::WindowConfig winConf{ .title = ctx.applicationName };
		winConf.setScreen(screen);
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
					win.moveOnFrame(ctx.mainWindow(), system().onFrameUpdate, windowFrameTimeSource);
					setIntendedFrameRate(win, configFrameTime());
				}
				extraWinData.updateWindowViewport(win, makeViewport(win), renderer);
				viewController().moveEmuViewToWindow(win);

				win.onEvent = [this](Window &win, WindowEvent winEvent)
				{
					return visit(overloaded
					{
						[&](Input::Event &e) { return viewController().extraWindowInputEvent(e); },
						[&](DrawEvent &e)
						{
							auto reportTime = scopeGuard([&]
							{
								if(viewController().isShowingEmulation())
									reportFrameWorkTime();
							});
							return viewController().drawExtraWindow(win, e.params, renderer.task());
						},
						[&](WindowSurfaceChangeEvent &e)
						{
							if(e.change.resized())
							{
								viewController().updateExtraWindowViewport(win, makeViewport(win), renderer.task());
							}
							renderer.task().updateDrawableForSurfaceChange(win, e.change);
							return true;
						},
						[&](DragDropEvent &e)
						{
							log.info("got DnD:{}", e.filename);
							handleOpenFileCommand(e.filename);
							return true;
						},
						[&](FocusChangeEvent &e)
						{
							windowData(win).focused = e.in;
							onFocusChange(e.in);
							return true;
						},
						[&](DismissRequestEvent &e)
						{
							win.dismiss();
							return true;
						},
						[&](DismissEvent &e)
						{
							system().resetFrameTime();
							log.info("setting emu view on main window");
							viewController().moveEmuViewToWindow(appContext().mainWindow());
							viewController().movePopupToWindow(appContext().mainWindow());
							viewController().placeEmuViews();
							mainWindow().postDraw();
							if(system().isActive())
							{
								emuSystemTask.pause();
								mainWindow().moveOnFrame(win, system().onFrameUpdate, windowFrameTimeSource);
								setIntendedFrameRate(mainWindow(), configFrameTime());
							}
							return true;
						},
						[](auto &){ return false; }
					}, winEvent);
				};

				win.show();
				viewController().placeEmuViews();
				mainWindow().postDraw();
			});
		if(!extraWin)
		{
			log.error("error creating extra window");
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

void EmuApp::record(FrameTimeStatEvent event, SteadyClockTimePoint t)
{
	doIfUsed(frameTimeStats, [&](auto &frameTimeStats)
	{
		if(!showFrameTimeStats || !viewController().isShowingEmulation())
			return;
		(&frameTimeStats.startOfFrame)[to_underlying(event)] = hasTime(t) ? t : SteadyClock::now();
	});
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
	viewController().emuWindow().addOnFrame(onFrame, windowFrameTimeSource);
}

void EmuApp::addOnFrameDelayed()
{
	// delay before adding onFrame handler to let timestamps stabilize
	auto delay = viewController().emuWindowScreen()->frameRate() / 4;
	//log.info("delaying onFrame handler by {} frames", onFrameHandlerDelay);
	addOnFrameDelegate(onFrameDelayed(delay));
}

void EmuApp::addOnFrame()
{
	addOnFrameDelegate(system().onFrameUpdate);
	savedAdvancedFrames = 0;
}

void EmuApp::removeOnFrame()
{
	viewController().emuWindow().removeOnFrame(system().onFrameUpdate, windowFrameTimeSource);
}

static auto &videoBrightnessVal(ImageChannel ch, auto &videoBrightnessRGB)
{
	switch(ch)
	{
		case ImageChannel::All: break;
		case ImageChannel::Red: return videoBrightnessRGB.r;
		case ImageChannel::Green: return videoBrightnessRGB.g;
		case ImageChannel::Blue: return videoBrightnessRGB.b;
	}
	bug_unreachable("invalid ImageChannel");
}

float EmuApp::videoBrightness(ImageChannel ch) const
{
	return videoBrightnessVal(ch, videoBrightnessRGB);
}

void EmuApp::setVideoBrightness(float brightness, ImageChannel ch)
{
	if(ch == ImageChannel::All)
	{
		videoBrightnessRGB.r = videoBrightnessRGB.g = videoBrightnessRGB.b = brightness;
	}
	else
	{
		videoBrightnessVal(ch, videoBrightnessRGB) = brightness;
	}
	emuVideoLayer.setBrightness(videoBrightnessRGB * menuVideoBrightnessScale);
}

bool isValidFastSpeed(int16_t speed) { return speed <= int(maxRunSpeed * 100.) && speed > 100; }

bool isValidSlowSpeed(int16_t speed) { return speed >= int(minRunSpeed * 100.) && speed < 100; }

static bool isValidAltSpeed(AltSpeedMode mode, int16_t speed)
{
	return mode == AltSpeedMode::slow ? isValidSlowSpeed(speed) : isValidFastSpeed(speed);
}

bool EmuApp::setAltSpeed(AltSpeedMode mode, int16_t speed)
{
	if(!isValidAltSpeed(mode, speed))
		return false;
	altSpeedRef(mode) = speed;
	return true;
}

void EmuApp::applyCPUAffinity(bool active)
{
	if(cpuAffinityMode == CPUAffinityMode::Any)
		return;
	auto frameThreadGroup = std::array{emuSystemTask.threadId(), renderer.task().threadId()};
	if(cpuAffinityMode == CPUAffinityMode::Auto && perfHintManager)
	{
		if(active)
		{
			auto targetTime = targetFrameTime(emuScreen());
			perfHintSession = perfHintManager.session(frameThreadGroup, targetTime);
			if(perfHintSession)
				log.info("made performance hint session with target time:{} ({} - {})",
					targetTime, emuScreen().frameTime(), emuScreen().presentationDeadline());
			else
				log.error("error making performance hint session");
		}
		else
		{
			perfHintSession = {};
			log.info("closed performance hint session");
		}
		return;
	}
	auto mask = active ?
		(cpuAffinityMode == CPUAffinityMode::Auto ? appContext().performanceCPUMask() : CPUMask(cpuAffinityMask)) : 0;
	log.info("applying CPU affinity mask {:X}", mask);
	setThreadCPUAffinityMask(frameThreadGroup, mask);
}

void EmuApp::setCPUAffinity(int cpuNumber, bool on)
{
	doIfUsed(cpuAffinityMask, [&](auto &cpuAffinityMask)
	{
		cpuAffinityMask = setOrClearBits(cpuAffinityMask, bit(cpuNumber), on);
	});
}

bool EmuApp::cpuAffinity(int cpuNumber) const
{
	return doIfUsed(cpuAffinityMask, [&](auto &cpuAffinityMask) { return cpuAffinityMask & bit(cpuNumber); }, false);
}

std::unique_ptr<View> EmuApp::makeView(ViewAttachParams attach, ViewID id)
{
	auto view = makeCustomView(attach, id);
	if(view)
		return view;
	switch(id)
	{
		case ViewID::MAIN_MENU: return std::make_unique<MainMenuView>(attach);
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<SystemActionsView>(attach);
		case ViewID::VIDEO_OPTIONS: return std::make_unique<VideoOptionView>(attach);
		case ViewID::AUDIO_OPTIONS: return std::make_unique<AudioOptionView>(attach);
		case ViewID::SYSTEM_OPTIONS: return std::make_unique<SystemOptionView>(attach);
		case ViewID::FILE_PATH_OPTIONS: return std::make_unique<FilePathOptionView>(attach);
		case ViewID::GUI_OPTIONS: return std::make_unique<GUIOptionView>(attach);
		default: bug_unreachable("Tried to make non-existing view ID:%d", (int)id);
	}
}

BluetoothAdapter *EmuApp::bluetoothAdapter()
{
	if(bta)
	{
		return bta;
	}
	log.info("initializing Bluetooth");
	bta = BluetoothAdapter::defaultAdapter(appContext());
	return bta;
}

void EmuApp::closeBluetoothConnections()
{
	Bluetooth::closeBT(std::exchange(bta, {}));
}

void EmuApp::reportFrameWorkTime()
{
	doIfUsed(frameStartTimePoint, [&](auto &tp)
	{
		if(perfHintSession && hasTime(tp))
			perfHintSession.reportActualWorkTime(SteadyClock::now() - std::exchange(tp, {}));
	});
}

MainWindowData &EmuApp::mainWindowData() const
{
	return EmuEx::mainWindowData(appContext().mainWindow());
}

EmuApp &EmuApp::get(IG::ApplicationContext ctx)
{
	return ctx.applicationAs<EmuApp>();
}

EmuApp &gApp() { return *gAppPtr; }

IG::ApplicationContext gAppContext() { return gApp().appContext(); }

}
