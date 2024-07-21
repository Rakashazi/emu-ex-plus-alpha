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
#include <imagine/bluetooth/BluetoothInputDevice.hh>
#include <imagine/input/android/MogaManager.hh>
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
	// display, screenshot, openFile, rewind
	{AssetFileID::ui, {{0,   .75}, {.25, 1.}}},
	{AssetFileID::ui, {{.25, .75}, {.5,  1.}}},
	{AssetFileID::ui, {{.5,  .75}, {.75, 1.}}},
	{AssetFileID::ui, {{.75, .75}, {1.,  1.}}},
	{AssetFileID::gamepadOverlay, {{}, {1.f, 1.f}}},
	{AssetFileID::keyboardOverlay, {{}, {1.f, 1.f}}},
};

EmuApp::EmuApp(ApplicationInitParams initParams, ApplicationContext &ctx):
	Application{initParams},
	fontManager{ctx},
	renderer{ctx},
	audio{ctx},
	videoLayer{video, defaultVideoAspectRatio()},
	inputManager{ctx},
	vibrationManager{ctx},
	pixmapReader{ctx},
	pixmapWriter{ctx},
	perfHintManager{ctx.performanceHintManager()},
	layoutBehindSystemUI{ctx.hasTranslucentSysUI()},
	bluetoothAdapter{ctx}
{
	if(ctx.registerInstance(initParams))
	{
		ctx.exit();
		return;
	}
	if(needsGlobalInstance)
		gAppPtr = this;
	ctx.setAcceptIPC(true);
	onEvent = [this](ApplicationContext, const ApplicationEvent& appEvent)
	{
		appEvent.visit(overloaded
		{
			[&](const DocumentPickerEvent& e)
			{
				log.info("document picked with URI:{}", e.uri);
				system().setInitialLoadPath(e.uri);
			},
			[](auto &) {}
		});
	};
	initOptions(ctx);
}

class ExitConfirmAlertView : public AlertView, public EmuAppHelper
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
		item.emplace_back("Yes", attach, [this](){ appContext().exit(); });
		item.emplace_back("No", attach, [](){});
		if(hasEmuContent)
		{
			item.emplace_back("Close Menu", attach, [this](){ app().showEmulation(); });
		}
	}

	bool inputEvent(const Input::Event &e, ViewInputEventParams) final
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
IG::ToastView &EmuApp::toastView() { return viewController().popup; }
const Screen &EmuApp::emuScreen() const { return *viewController().emuWindowScreen(); }
Window &EmuApp::emuWindow() { return viewController().emuWindow(); }
const Window &EmuApp::emuWindow() const { return viewController().emuWindow(); }

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
	app.autosaveManager.save();
	app.system().flushBackupMemory(app);
}

void EmuApp::closeSystem()
{
	emuSystemTask.stop();
	showUI();
	system().closeRuntimeSystem(*this);
	autosaveManager.resetSlot();
	rewindManager.clear();
	viewController().onSystemClosed();
}

void EmuApp::closeSystemWithoutSave()
{
	autosaveManager.resetSlot(noAutosaveName);
	closeSystem();
}

void EmuApp::applyOSNavStyle(IG::ApplicationContext ctx, bool inEmu)
{
	SystemUIStyleFlags flags;
	if(lowProfileOSNav > (inEmu ? InEmuTristate::Off : InEmuTristate::InEmu))
		flags.dimNavigation = true;
	if(hidesOSNav > (inEmu ? InEmuTristate::Off : InEmuTristate::InEmu))
		flags.hideNavigation = true;
	if(hidesStatusBar > (inEmu ? InEmuTristate::Off : InEmuTristate::InEmu))
		flags.hideStatus = true;
	ctx.setSysUIStyle(flags);
}

void EmuApp::showSystemActionsViewFromSystem(ViewAttachParams attach, const Input::Event &e)
{
	viewController().showSystemActionsView(attach, e);
}

void EmuApp::showLastViewFromSystem(ViewAttachParams attach, const Input::Event &e)
{
	if(systemActionsIsDefaultMenu)
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
	windowDrawableConfig = conf;
	auto ctx = appContext();
	for(auto &w : ctx.windows())
	{
		if(!renderer.setDrawableConfig(*w, conf))
			return false;
	}
	applyRenderPixelFormat();
	return true;
}

IG::PixelFormat EmuApp::windowPixelFormat() const
{
	auto fmt = windowDrawableConfig.pixelFormat.value();
	if(fmt)
		return fmt;
	return appContext().defaultWindowPixelFormat();
}

void EmuApp::setRenderPixelFormat(IG::PixelFormat fmt)
{
	renderPixelFormat = fmt;
	applyRenderPixelFormat();
}

void EmuApp::applyRenderPixelFormat()
{
	if(!video.hasRendererTask())
		return;
	auto fmt = renderPixelFormat.value();
	if(!fmt)
		fmt = windowPixelFormat();
	if(!EmuSystem::canRenderRGBA8888 && fmt != IG::PixelFmtRGB565)
	{
		log.info("Using RGB565 render format since emulated system can't render RGBA8888");
		fmt = IG::PixelFmtRGB565;
	}
	videoLayer.setFormat(system(), fmt, videoEffectPixelFormat(), windowDrawableConfig.colorSpace);
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

void EmuApp::startAudio()
{
	audio.start(system().frameTime());
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
	loadConfigFile(ctx);
	system().onOptionsLoaded();
	loadSystemOptions();
	updateLegacySavePathOnStoragePath(ctx, system());
	system().setInitialLoadPath(parseCommandArgs(initParams.commandArgs()));
	audio.manager.setMusicVolumeControlHint();
	if(!renderer.supportsColorSpace())
		windowDrawableConfig.colorSpace = {};
	applyOSNavStyle(ctx, false);

	ctx.addOnResume(
		[this](IG::ApplicationContext, [[maybe_unused]] bool focused)
		{
			audio.manager.startSession();
			audio.open();
			return true;
		});

	ctx.addOnExit(
		[this](IG::ApplicationContext ctx, bool backgrounded)
		{
			if(backgrounded)
			{
				suspendEmulation(*this);
				if(showsNotificationIcon)
				{
					auto title = std::format("{} was suspended", ctx.applicationName);
					ctx.addNotification(title, title, system().contentDisplayName());
				}
			}
			audio.close();
			audio.manager.endSession();
			saveConfigFile(ctx);
			saveSystemOptions();
			if(!backgrounded || (backgrounded && !keepBluetoothActive))
				closeBluetoothConnections();
			onEvent(ctx, FreeCachesEvent{false});
			return true;
		});

	IG::WindowConfig winConf{ .title = ctx.applicationName };
	winConf.setFormat(windowDrawableConfig.pixelFormat);
	ctx.makeWindow(winConf,
		[this](IG::ApplicationContext ctx, IG::Window &win)
		{
			renderer.initMainTask(&win, windowDrawableConfig);
			textureBufferMode = renderer.validateTextureBufferMode(textureBufferMode);
			viewManager.defaultFace = {renderer, fontManager.makeSystem(), fontSettings(win)};
			viewManager.defaultBoldFace = {renderer, fontManager.makeBoldSystem(), fontSettings(win)};
			ViewAttachParams viewAttach{viewManager, win, renderer.task()};
			auto &vController = inputManager.vController;
			auto &winData = win.makeAppData<MainWindowData>(viewAttach, vController, videoLayer, system());
			winData.updateWindowViewport(win, makeViewport(win), renderer);
			win.setAcceptDnd(true);
			renderer.setWindowValidOrientations(win, menuOrientation);
			inputManager.updateInputDevices(ctx);
			vController.configure(win, renderer, viewManager.defaultFace);
			if(EmuSystem::inputHasKeyboard)
			{
				vController.setKeyboardImage(asset(AssetID::keyboardOverlay));
			}
			winData.viewController.placeElements();
			winData.viewController.pushAndShow(makeView(viewAttach, ViewID::MAIN_MENU));
			configureSecondaryScreens();
			video.onFormatChanged =  [this, &viewController = winData.viewController](EmuVideo&)
			{
				videoLayer.onVideoFormatChanged(videoEffectPixelFormat());
				viewController.placeEmuViews();
			};
			video.setRendererTask(renderer.task());
			video.setTextureBufferMode(system(), textureBufferMode);
			videoLayer.setRendererTask(renderer.task());
			applyRenderPixelFormat();
			videoLayer.updateEffect(system(), videoEffectPixelFormat());
			system().onFrameUpdate = [this](FrameParams params)
			{
				bool renderingFrame = advanceFrames(params, &emuSystemTask);
				if(params.isFromRenderer() && !renderingFrame)
				{
					renderingFrame = true;
					emuSystemTask.window().drawNow();
				}
				if(renderingFrame)
				{
					record(FrameTimeStatEvent::waitForPresent);
					framePresentedSem.acquire();
					reportFrameWorkTime();
					record(FrameTimeStatEvent::endOfFrame);
				}
				return true;
			};

			win.onEvent = [this](Window& win, const WindowEvent& winEvent)
			{
				return winEvent.visit(overloaded
				{
					[&](const Input::Event& e) { return viewController().inputEvent(e); },
					[&](const DrawEvent& e)
					{
						return viewController().drawMainWindow(win, e.params, renderer.task());
					},
					[&](const WindowSurfaceChangeEvent& e)
					{
						if(e.change.resized())
						{
							viewController().updateMainWindowViewport(win, makeViewport(win), renderer.task());
						}
						renderer.task().updateDrawableForSurfaceChange(win, e.change);
						return true;
					},
					[&](const DragDropEvent& e)
					{
						log.info("got DnD:{}", e.filename);
						handleOpenFileCommand(e.filename);
						return true;
					},
					[&](const FocusChangeEvent& e)
					{
						windowData(win).focused = e.in;
						onFocusChange(e.in);
						return true;
					},
					[](auto&){ return false; }
				});
			};

			onMainWindowCreated(viewAttach, ctx.defaultInputEvent());

			onEvent = [this](ApplicationContext ctx, const ApplicationEvent& appEvent)
			{
				appEvent.visit(overloaded
				{
					[&](const DocumentPickerEvent& e)
					{
						log.info("document picked with URI:{}", e.uri);
						if(!viewController().isShowingEmulation() && viewController().top().onDocumentPicked(e))
							return;
						handleOpenFileCommand(e.uri);
					},
					[&](const ScreenChangeEvent &e)
					{
						if(e.change == ScreenChange::added)
						{
							log.info("screen added");
							if(showOnSecondScreen && ctx.screens().size() > 1)
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
								auto suspendCtx = suspendEmulationThread();
								configFrameTime();
							}
						}
					},
					[&](const Input::DevicesEnumeratedEvent &)
					{
						log.info("input devs enumerated");
						inputManager.updateInputDevices(ctx);
					},
					[&](const Input::DeviceChangeEvent &e)
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
					[&](const FreeCachesEvent &e)
					{
						viewManager.defaultFace.freeCaches();
						viewManager.defaultBoldFace.freeCaches();
						if(e.running)
							viewController().prepareDraw();
					},
					[](auto &) {}
				});
			};

			ctx.addOnExit(
				[this](IG::ApplicationContext ctx, bool backgrounded)
				{
					if(backgrounded)
					{
						showUI();
						if(showOnSecondScreen && ctx.screens().size() > 1)
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

bool EmuApp::advanceFrames(FrameParams frameParams, EmuSystemTask *taskPtr)
{
	assert(hasTime(frameParams.timestamp));
	auto &sys = system();
	auto &viewCtrl = viewController();
	auto &win = viewCtrl.emuWindow();
	auto *audioPtr = audio ? &audio : nullptr;
	auto frameInfo = sys.timing.advanceFrames(frameParams);
	int interval = frameInterval;
	if(presentationTimeMode == PresentationTimeMode::full ||
		(presentationTimeMode == PresentationTimeMode::basic && interval > 1))
	{
		viewCtrl.presentTime = frameParams.presentTime(interval);
	}
	else
	{
		viewCtrl.presentTime = {};
	}
	if(sys.shouldFastForward()) [[unlikely]]
	{
		// for skipping loading on disk-based computers
		if(skipForwardFrames({taskPtr}, 20))
		{
			// don't write any audio while skip is in progress
			audioPtr = nullptr;
		}
		frameInfo.advanced = 1;
	}
	if(!frameInfo.advanced)
	{
		if(enableBlankFrameInsertion)
		{
			viewCtrl.drawBlankFrame = true;
			win.drawNow();
			return true;
		}
		return false;
	}
	bool allowFrameSkip = interval || sys.frameTimeMultiplier != 1.;
	if(frameInfo.advanced + savedAdvancedFrames < interval)
	{
		// running at a lower target fps
		savedAdvancedFrames += frameInfo.advanced;
	}
	else
	{
		savedAdvancedFrames = 0;
		if(!allowFrameSkip)
		{
			frameInfo.advanced = 1;
		}
		if(frameInfo.advanced > 1)
		{
			doIfUsed(frameTimeStats, [&](auto &stats) { stats.missedFrameCallbacks+= frameInfo.advanced - 1; });
		}
	}
	assumeExpr(frameInfo.advanced > 0);
	// cap advanced frames if we're falling behind
	if(frameInfo.frameTimeDiff > Milliseconds{70})
		frameInfo.advanced = std::min(frameInfo.advanced, 4);
	EmuVideo *videoPtr = savedAdvancedFrames ? nullptr : &video;
	if(videoPtr)
	{
		if(showFrameTimeStats)
		{
			viewCtrl.emuView.updateFrameTimeStats(frameTimeStats, frameParams.timestamp);
		}
		record(FrameTimeStatEvent::startOfFrame, frameParams.timestamp);
		record(FrameTimeStatEvent::startOfEmulation);
	}
	//log.debug("running {} frame(s), skip:{}", frameInfo.advanced, !videoPtr);
	runFrames({taskPtr}, videoPtr, audioPtr, frameInfo.advanced);
	inputManager.turboActions.update(*this);
	return videoPtr;
}

IG::Viewport EmuApp::makeViewport(const IG::Window &win) const
{
	return win.viewport(layoutBehindSystemUI ? win.bounds() : win.contentBounds());
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
	if(autosaveManager.autosaveLaunchMode == AutosaveLaunchMode::Ask)
	{
		autosaveManager.resetSlot(noAutosaveName);
		viewController().pushAndShow(EmuApp::makeView(attachParams(), EmuApp::ViewID::SYSTEM_ACTIONS), e);
		viewController().pushAndShow(std::make_unique<AutosaveSlotView>(attachParams()), e);
	}
	else
	{
		auto loadMode = autosaveManager.autosaveLaunchMode == AutosaveLaunchMode::LoadNoState ? LoadAutosaveMode::NoState : LoadAutosaveMode::Normal;
		if(autosaveManager.autosaveLaunchMode == AutosaveLaunchMode::NoSave)
			autosaveManager.resetSlot(noAutosaveName);
		static auto finishLaunch = [](EmuApp &app, LoadAutosaveMode mode)
		{
			app.autosaveManager.load(mode);
			if(!app.system().hasContent())
			{
				log.error("system was closed while trying to load autosave");
				return;
			}
			app.showEmulation();
		};
		auto stateIsOlderThanBackupMemory = [&]
		{
			auto stateTime = autosaveManager.stateTime();
			return hasTime(stateTime) && (autosaveManager.backupMemoryTime() - stateTime) > Seconds{1};
		};
		if(system().usesBackupMemory() && loadMode == LoadAutosaveMode::Normal &&
			!autosaveManager.saveOnlyBackupMemory && stateIsOlderThanBackupMemory())
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
	if(appContext().fileUriType(path) == FS::file_type::directory)
	{
		log.info("changing to dir {} from external command", path);
		showUI(false);
		viewController().popToRoot();
		contentSearchPath = path;
		viewController().pushAndShow(
			FilePicker::forLoading(attachParams(), appContext().defaultInputEvent()),
			appContext().defaultInputEvent(),
			false);
	}
	else
	{
		log.info("opening file {} from external command", path);
		showUI();
		viewController().popToRoot();
		onSelectFileFromPicker({}, path, name, Input::KeyEvent{}, {}, attachParams());
	}
}

void EmuApp::runBenchmarkOneShot(EmuVideo &video)
{
	log.info("starting benchmark");
	auto time = system().benchmark(video);
	autosaveManager.resetSlot(noAutosaveName);
	closeSystem();
	auto timeSecs = duration_cast<FloatSeconds>(time);
	log.info("done in:{}", timeSecs);
	postMessage(2, 0, std::format("{:.2f} fps", 180. / timeSecs.count()));
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
	videoLayer.setBrightnessScale(1.f);
	video.onFrameFinished = [&, &viewController = viewController()](EmuVideo&)
	{
		auto &win = viewController.emuWindow();
		win.drawNow();
	};
	frameTimeStats = {};
	system().start(*this);
	emuSystemTask.start(emuWindow());
	setCPUNeedsLowLatency(appContext(), true);
}

void EmuApp::showUI(bool updateTopView)
{
	if(!viewController().isShowingEmulation())
		return;
	pauseEmulation();
	configureAppForEmulation(false);
	videoLayer.setBrightnessScale(menuVideoBrightnessScale);
	viewController().showMenuView(updateTopView);
}

void EmuApp::pauseEmulation()
{
	emuSystemTask.stop();
	setCPUNeedsLowLatency(appContext(), false);
	video.onFrameFinished = [](EmuVideo&){};
	system().pause(*this);
	setRunSpeed(1.);
	videoLayer.setBrightnessScale(pausedVideoBrightnessScale);
}

bool EmuApp::hasArchiveExtension(std::string_view name)
{
	return FS::hasArchiveExtension(name);
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
	pauseEmulation();
	viewController().popToSystemActionsMenu();
	auto ctx = appContext();
	try
	{
		system().createWithMedia({}, system().contentLocation(),
			ctx.fileUriDisplayName(system().contentLocation()), params,
			[](int, int, const char*){ return true; });
		onSystemCreated();
		if(autosaveManager.slotName() != noAutosaveName)
			system().loadBackupMemory(*this);
	}
	catch(...)
	{
		log.error("Error reloading system");
		system().clearGamePaths();
	}
}

void EmuApp::onSystemCreated()
{
	updateVideoContentRotation();
	if(!rewindManager.reset(system().stateSize()))
	{
		postErrorMessage(4, "Not enough memory for rewind states");
	}
	viewController().onSystemCreated();
}

void EmuApp::promptSystemReloadDueToSetOption(ViewAttachParams attach, const Input::Event &e, EmuSystemCreateParams params)
{
	if(!system().hasContent())
		return;
	viewController().pushAndShowModal(std::make_unique<YesNoAlertView>(attach,
		"This option takes effect next time the system starts. Restart it now?",
		YesNoAlertView::Delegates
		{ .onYes = [this, params]
			{
				reloadSystem(params);
				showEmulation();
				return false;
			}
		}), e, false);
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
	auto slotName = autosaveManager.slotName();
	if(slotName.size() && slotName != noAutosaveName)
		return system().contentLocalSaveDirectory(slotName, name);
	else
		return system().contentSavePath(name);
}

FS::PathString EmuApp::contentSaveFilePath(std::string_view ext) const
{
	auto slotName = autosaveManager.slotName();
	if(slotName.size() && slotName != noAutosaveName)
		return system().contentLocalSaveDirectory(slotName, FS::FileString{"auto"}.append(ext));
	else
		return system().contentSaveFilePath(ext);
}

void EmuApp::setupStaticBackupMemoryFile(FileIO &io, std::string_view ext, size_t size, uint8_t initValue) const
{
	if(io)
		return;
	io = system().openStaticBackupMemoryFile(system().contentSaveFilePath(ext), size, initValue);
	if(!io) [[unlikely]]
		throw std::runtime_error(std::format("Error opening {}, please verify save path has write access", system().contentNameExt(ext)));
}

void EmuApp::readState(std::span<uint8_t> buff)
{
	auto suspendCtx = suspendEmulationThread();
	system().readState(*this, buff);
	system().clearInputBuffers(viewController().inputView);
	autosaveManager.resetTimer();
}

size_t EmuApp::writeState(std::span<uint8_t> buff, SaveStateFlags flags)
{
	auto suspendCtx = suspendEmulationThread();
	return system().writeState(buff, flags);
}

DynArray<uint8_t> EmuApp::saveState()
{
	auto suspendCtx = suspendEmulationThread();
	return system().saveState();
}

bool EmuApp::saveState(CStringView path, bool notify)
{
	if(!system().hasContent())
	{
		postErrorMessage("System not running");
		return false;
	}
	log.info("saving state {}", path);
	auto suspendCtx = suspendEmulationThread();
	try
	{
		system().saveState(path);
		if(notify)
			postMessage("State Saved");
		return true;
	}
	catch(std::exception &err)
	{
		postErrorMessage(4, std::format("Can't save state:\n{}", err.what()));
		return false;
	}
}

bool EmuApp::saveStateWithSlot(int slot, bool notify)
{
	return saveState(system().statePath(slot), notify);
}

bool EmuApp::loadState(CStringView path)
{
	if(!system().hasContent()) [[unlikely]]
	{
		postErrorMessage("System not running");
		return false;
	}
	log.info("loading state {}", path);
	auto suspendCtx = suspendEmulationThread();
	try
	{
		system().loadState(*this, path);
		autosaveManager.resetTimer();
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

FS::PathString EmuApp::inContentSearchPath(std::string_view name) const
{
	return FS::uriString(contentSearchPath, name);
}

FS::PathString EmuApp::validSearchPath(const FS::PathString &path) const
{
	if(path.empty())
		return contentSearchPath;
	return hasArchiveExtension(path) ? FS::dirnameUri(path) : path;
}

[[gnu::weak]] void EmuApp::onMainWindowCreated(ViewAttachParams, const Input::Event &) {}

[[gnu::weak]] void EmuApp::onCustomizeNavView(EmuApp::NavView &) {}

[[gnu::weak]] std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams, EmuApp::ViewID)
{
	return nullptr;
}

std::unique_ptr<YesNoAlertView> EmuApp::makeCloseContentView()
{
	return std::make_unique<YesNoAlertView>(attachParams(), "Really close current content?",
		YesNoAlertView::Delegates
		{
			.onYes = [this]
			{
				closeSystem(); // pops any System Actions views in the stack
				viewController().popModalViews();
				return false;
			}
		});
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
	auto suspendCtx = suspendEmulationThread();
	system().frameTimeMultiplier = 1. / speed;
	audio.setSpeedMultiplier(speed);
	configFrameTime();
}

FS::PathString EmuApp::sessionConfigPath()
{
	return system().contentSaveFilePath(".config");
}

bool EmuApp::hasSavedSessionOptions()
{
	return system().sessionOptionsAreSet() || appContext().fileUriExists(sessionConfigPath());
}

void EmuApp::resetSessionOptions()
{
	inputManager.resetSessionOptions(appContext());
	system().resetSessionOptions(*this);
}

void EmuApp::deleteSessionOptions()
{
	if(!hasSavedSessionOptions())
	{
		return;
	}
	resetSessionOptions();
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
		inputManager.writeSessionConfig(configFile);
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
	resetSessionOptions();
	auto ctx = appContext();
	if(readConfigKeys(FileUtils::bufferFromUri(ctx, sessionConfigPath(), {.test = true}),
		[this, ctx](auto key, auto &io) -> bool
		{
			if(inputManager.readSessionConfig(ctx, io, key))
				return true;
			if(system().readConfig(ConfigType::SESSION, io, key))
				return true;
			log.info("skipping unknown key {}", key);
			return false;
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
		[this](uint16_t key, auto &io)
		{
			if(!system().readConfig(ConfigType::CORE, io, key))
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

EmuSystemTask::SuspendContext EmuApp::suspendEmulationThread() { return emuSystemTask.suspend(); }

FrameTimeConfig EmuApp::configFrameTime()
{
	std::array<FrameRate, 1> overrideRate{overrideScreenFrameRate};
	auto supportedRates = overrideScreenFrameRate ? std::span<const FrameRate>{overrideRate.data(), 1} : emuScreen().supportedFrameRates();
	auto frameTimeConfig = outputTimingManager.frameTimeConfig(system(), supportedRates);
	system().configFrameTime(audio.format().rate, frameTimeConfig.time);
	system().timing.exactFrameDivisor = 0;
	if(frameTimeConfig.refreshMultiplier > 0 &&
		(allowBlankFrameInsertion || effectiveFrameTimeSource() == FrameTimeSource::Renderer))
	{
		system().timing.exactFrameDivisor = std::round(emuScreen().frameRate() / frameTimeConfig.rate);
		log.info("using exact frame divisor:{}", system().timing.exactFrameDivisor);
	}
	return frameTimeConfig;
}

void EmuApp::runFrames(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio, int frames)
{
	skipFrames(taskCtx, frames - 1, audio);
	system().runFrame(taskCtx, video, audio);
	system().updateBackupMemoryCounter();
}

void EmuApp::skipFrames(EmuSystemTaskContext taskCtx, int frames, EmuAudio *audio)
{
	assert(system().hasContent());
	for([[maybe_unused]] auto i : iotaCount(frames))
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

void EmuApp::notifyWindowPresented()
{
	framePresentedSem.release();
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
	if(!fontSize.set(size))
		return false;
	applyFontSize(viewController().emuWindow());
	viewController().placeElements();
	return true;
}

void EmuApp::configureAppForEmulation(bool running)
{
	appContext().setIdleDisplayPowerSave(running ? idleDisplayPowerSave.value() : true);
	applyOSNavStyle(appContext(), running);
	appContext().setHintKeyRepeat(!running);
}

void EmuApp::setIntendedFrameRate(Window &win, FrameTimeConfig config)
{
	enableBlankFrameInsertion = false;
	if(allowBlankFrameInsertion && config.refreshMultiplier > 1 && frameInterval <= 1)
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
		else if(pauseUnfocused && !system().isPaused() && !allWindowsAreFocused())
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
		winConf.setFormat(windowDrawableConfig.pixelFormat);
		auto extraWin = ctx.makeWindow(winConf,
			[this](IG::ApplicationContext, IG::Window &win)
			{
				renderer.attachWindow(win, windowDrawableConfig);
				auto &extraWinData = win.makeAppData<WindowData>();
				extraWinData.hasPopup = false;
				extraWinData.focused = true;
				auto suspendCtx = emuSystemTask.setWindow(win);
				if(system().isActive())
				{
					setIntendedFrameRate(win, configFrameTime());
				}
				extraWinData.updateWindowViewport(win, makeViewport(win), renderer);
				viewController().moveEmuViewToWindow(win);

				win.onEvent = [this](Window& win, const WindowEvent& winEvent)
				{
					return winEvent.visit(overloaded
					{
						[&](const Input::Event& e) { return viewController().extraWindowInputEvent(e); },
						[&](const DrawEvent& e)
						{
							return viewController().drawExtraWindow(win, e.params, renderer.task());
						},
						[&](const WindowSurfaceChangeEvent& e)
						{
							if(e.change.resized())
							{
								viewController().updateExtraWindowViewport(win, makeViewport(win), renderer.task());
							}
							renderer.task().updateDrawableForSurfaceChange(win, e.change);
							return true;
						},
						[&](const DragDropEvent& e)
						{
							log.info("got DnD:{}", e.filename);
							handleOpenFileCommand(e.filename);
							return true;
						},
						[&](const FocusChangeEvent& e)
						{
							windowData(win).focused = e.in;
							onFocusChange(e.in);
							return true;
						},
						[&](const DismissRequestEvent&)
						{
							win.dismiss();
							return true;
						},
						[&](const DismissEvent&)
						{
							auto suspendCtx = emuSystemTask.setWindow(mainWindow());
							system().resetFrameTime();
							log.info("setting emu view on main window");
							viewController().moveEmuViewToWindow(appContext().mainWindow());
							viewController().movePopupToWindow(appContext().mainWindow());
							viewController().placeEmuViews();
							if(system().isActive())
							{
								setIntendedFrameRate(mainWindow(), configFrameTime());
							}
							suspendCtx.resume();
							mainWindow().postDraw();
							return true;
						},
						[](auto&){ return false; }
					});
				};

				win.show();
				viewController().placeEmuViews();
				suspendCtx.resume();
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
	if(showOnSecondScreen && appContext().screens().size() > 1)
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
		if(params.isFromRenderer() || video.image())
		{
			emuSystemTask.window().drawNow();
		}
		if(delay)
		{
			addOnFrameDelegate(onFrameDelayed(delay - 1));
		}
		else
		{
			if(system().isActive())
			{
				addOnFrame();
			}
		}
		return false;
	};
}

void EmuApp::addOnFrameDelegate(IG::OnFrameDelegate onFrame)
{
	viewController().emuWindow().addOnFrame(onFrame, frameTimeSource);
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
	viewController().emuWindow().removeOnFrame(system().onFrameUpdate, frameTimeSource);
}

bool EmuApp::setAltSpeed(AltSpeedMode mode, int16_t speed)
{
	if(mode == AltSpeedMode::slow)
		return slowModeSpeed.set(speed);
	else
		return fastModeSpeed.set(speed);
}

void EmuApp::applyCPUAffinity(bool active)
{
	if(cpuAffinityMode.value() == CPUAffinityMode::Any)
		return;
	auto frameThreadGroup = std::vector{emuSystemTask.threadId(), renderer.task().threadId()};
	system().addThreadGroupIds(frameThreadGroup);
	if(cpuAffinityMode.value() == CPUAffinityMode::Auto && perfHintManager)
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
		(cpuAffinityMode.value() == CPUAffinityMode::Auto ? appContext().performanceCPUMask() : cpuAffinityMask.value()) : 0;
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
		case ViewID::VIDEO_OPTIONS: return std::make_unique<VideoOptionView>(attach, videoLayer);
		case ViewID::AUDIO_OPTIONS: return std::make_unique<AudioOptionView>(attach, audio);
		case ViewID::SYSTEM_OPTIONS: return std::make_unique<SystemOptionView>(attach);
		case ViewID::FILE_PATH_OPTIONS: return std::make_unique<FilePathOptionView>(attach);
		case ViewID::GUI_OPTIONS: return std::make_unique<GUIOptionView>(attach);
		default: bug_unreachable("Tried to make non-existing view ID:%d", (int)id);
	}
}

void EmuApp::closeBluetoothConnections()
{
	Bluetooth::closeBT(bluetoothAdapter);
}

void EmuApp::reportFrameWorkTime()
{
	auto lastFrameTimestamp = system().timing.lastFrameTimestamp();
	if(perfHintSession && hasTime(lastFrameTimestamp))
		perfHintSession.reportActualWorkTime(SteadyClock::now() - lastFrameTimestamp);
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

void pushAndShowModalView(std::unique_ptr<View> v, const Input::Event &e)
{
	v->appContext().applicationAs<EmuApp>().viewController().pushAndShowModal(std::move(v), e, false);
}

void pushAndShowNewYesNoAlertView(ViewAttachParams attach, const Input::Event &e, const char *label,
	const char *choice1, const char *choice2, TextMenuItem::SelectDelegate onYes, TextMenuItem::SelectDelegate onNo)
{
	attach.appContext().applicationAs<EmuApp>().pushAndShowModalView(std::make_unique<YesNoAlertView>(attach, label, choice1, choice2, YesNoAlertView::Delegates{onYes, onNo}), e);
}

Gfx::TextureSpan collectTextCloseAsset(ApplicationContext ctx)
{
	return ctx.applicationAs<const EmuApp>().collectTextCloseAsset();
}

void postErrorMessage(ApplicationContext ctx, std::string_view s)
{
	ctx.applicationAs<EmuApp>().postErrorMessage(s);
}

}
