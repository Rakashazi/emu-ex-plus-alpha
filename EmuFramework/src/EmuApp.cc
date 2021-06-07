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
#include <emuframework/FileUtils.hh>
#include "private.hh"
#include "privateInput.hh"
#include "WindowData.hh"
#include "configFile.hh"
#include "EmuOptions.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gui/ToastView.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/data-type/image/PixmapReader.hh>
#include <imagine/util/utility.h>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/thread/Thread.hh>
#include <cmath>

EmuApp::EmuApp(Base::ApplicationInitParams initParams, Base::ApplicationContext &ctx, Gfx::Error &rendererErr):
	Application{initParams},
	renderer{ctx, rendererErr},
	audioManager_{ctx},
	emuAudio{audioManager_},
	emuVideoLayer{emuVideo},
	emuSystemTask{*this},
	vController{EmuSystem::inputFaceBtns},
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
	inputDevConf{},
	lastLoadPath{ctx.sharedStoragePath()},
	pixmapWriter{ctx}
{
	if(rendererErr)
	{
		ctx.exitWithErrorMessagePrintf(-1, "Error creating renderer: %s", rendererErr->what());
		return;
	}
	if(auto err = EmuSystem::onInit(ctx);
		err)
	{
		ctx.exitWithErrorMessagePrintf(-1, "%s", err->what());
		return;
	}
	mainInitCommon(initParams, ctx);
}

class ExitConfirmAlertView : public AlertView
{
public:
	ExitConfirmAlertView(ViewAttachParams attach, EmuViewController &emuViewController):
		AlertView
		{
			attach,
			"Really Exit? (Push Back/Escape again to confirm)",
			EmuSystem::gameIsRunning() ? 3u : 2u
		}
	{
		setItem(0, "Yes", [this](){ appContext().exit(); });
		setItem(1, "No", [](){});
		if(item.size() == 3)
		{
			setItem(2, "Close Menu",
				[&]()
				{
					if(EmuSystem::gameIsRunning())
					{
						emuViewController.showEmulation();
					}
				});
		}
	}

	bool inputEvent(Input::Event e) final
	{
		if(e.pushed() && e.isDefaultCancelButton())
		{
			if(!e.repeated())
			{
				appContext().exit();
			}
			return true;
		}
		return AlertView::inputEvent(e);
	}
};

#ifdef CONFIG_BLUETOOTH
BluetoothAdapter *bta{};
#endif
[[gnu::weak]] bool EmuApp::hasIcon = true;
[[gnu::weak]] bool EmuApp::autoSaveStateDefault = true;

static constexpr const char *assetFilename[]
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

static_assert(std::size(assetFilename) == (unsigned)EmuApp::AssetID::END);

Gfx::PixmapTexture &EmuApp::asset(AssetID assetID) const
{
	assumeExpr(assetID < AssetID::END);
	auto assetIdx = (unsigned)assetID;
	auto &res = assetBuffImg[assetIdx];
	if(!res)
	{
		IG::Data::PixmapReader pixReader{renderer.appContext()};
		if(auto ec = pixReader.loadAsset(assetFilename[assetIdx]);
			ec)
		{
			logErr("couldn't load %s", assetFilename[assetIdx]);
		}
		res = renderer.makePixmapTexture(pixReader, &renderer.get(View::imageCommonTextureSampler));
	}
	return res;
}

Gfx::PixmapTexture *EmuApp::collectTextCloseAsset() const
{
	return Config::envIsAndroid ? nullptr : &asset(AssetID::CLOSE);
}

EmuViewController &EmuApp::viewController()
{
	return *emuViewController;
}

void EmuApp::setCPUNeedsLowLatency(Base::ApplicationContext ctx, bool needed)
{
	#ifdef __ANDROID__
	if(optionSustainedPerformanceMode)
		ctx.setSustainedPerformanceMode(needed);
	#endif
}

static void suspendEmulation(EmuApp &app)
{
	app.saveAutoState();
	EmuSystem::saveBackupMem();
}

void EmuApp::exitGame(bool allowAutosaveState)
{
	// leave any sub menus that may depending on running game state
	popMenuToRoot();
	viewController().closeSystem(allowAutosaveState);
}

void EmuApp::applyOSNavStyle(Base::ApplicationContext ctx, bool inGame)
{
	auto flags = Base::SYS_UI_STYLE_NO_FLAGS;
	if(optionLowProfileOSNav > (inGame ? 0 : 1))
		flags |= Base::SYS_UI_STYLE_DIM_NAV;
	if(optionHideOSNav > (inGame ? 0 : 1))
		flags |= Base::SYS_UI_STYLE_HIDE_NAV;
	if(optionHideStatusBar > (inGame ? 0 : 1))
		flags |= Base::SYS_UI_STYLE_HIDE_STATUS;
	ctx.setSysUIStyle(flags);
}

IG::Audio::Manager &EmuApp::audioManager()
{
	return audioManager_;
}

Base::ApplicationContext EmuApp::appContext() const
{
	return renderer.appContext();
}

void EmuApp::showSystemActionsViewFromSystem(ViewAttachParams attach, Input::Event e)
{
	viewController().showSystemActionsView(attach, e);
}

void EmuApp::showLastViewFromSystem(ViewAttachParams attach, Input::Event e)
{
	if(optionSystemActionsIsDefaultMenu)
	{
		showSystemActionsViewFromSystem(attach, e);
	}
	else
	{
		viewController().showUI();
	}
}

void EmuApp::showExitAlert(ViewAttachParams attach, Input::Event e)
{
	viewController().pushAndShowModal(std::make_unique<ExitConfirmAlertView>(attach, *emuViewController), e, false);
}

static const char *parseCommandArgs(Base::CommandArgs arg)
{
	if(arg.c < 2)
	{
		return nullptr;
	}
	auto launchGame = arg.v[1];
	logMsg("starting game from command line: %s", launchGame);
	return launchGame;
}

bool EmuApp::setWindowDrawableConfig(Gfx::DrawableConfig conf)
{
	windowDrawableConf = conf;
	for(auto &w : appContext().windows())
	{
		if(!renderer.setDrawableConfig(*w, conf))
			return false;
	}
	emuVideoLayer.setSrgbColorSpaceOutput(conf.colorSpace == Gfx::ColorSpace::SRGB);
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
	if(renderPixelFmt)
		return renderPixelFmt;
	return {};
}

void EmuApp::applyRenderPixelFormat()
{
	if(!emuVideo.hasRendererTask())
		return;
	auto fmt = renderPixelFormat();
	if(!fmt)
		fmt = windowDrawableConfig().pixelFormat;
	if(!fmt)
		fmt = appContext().defaultWindowPixelFormat();
	emuVideo.setRenderPixelFormat(fmt);
	fmt = emuVideo.renderPixelFormat();
	logMsg("setting render pixel format:%s", fmt.name());
	EmuSystem::onVideoRenderFormatChange(emuVideo, fmt);
	renderSystemFramebuffer(emuVideo);
}

void EmuApp::renderSystemFramebuffer(EmuVideo &video)
{
	if(!EmuSystem::gameIsRunning())
	{
		video.clear();
		return;
	}
	logMsg("updating video with current framebuffer content");
	EmuSystem::renderFramebuffer(video);
}

static bool supportsVideoImageBuffersOption(const Gfx::Renderer &r)
{
	return r.supportsSyncFences() && r.maxSwapChainImages() > 2;
}

EmuAudio &EmuApp::audio()
{
	return emuAudio;
}

EmuVideo &EmuApp::video()
{
	return emuVideo;
}

void EmuApp::mainInitCommon(Base::ApplicationInitParams initParams, Base::ApplicationContext ctx)
{
	if(ctx.registerInstance(initParams))
	{
		ctx.exit();
		return;
	}
	ctx.setAcceptIPC(true);
	ctx.setOnInterProcessMessage(
		[](Base::ApplicationContext, const char *path)
		{
			logMsg("got IPC path:%s", path);
			EmuSystem::setInitialLoadPath(path);
		});
	optionVControllerLayoutPos.setVController(vController);
	initOptions(ctx);
	auto appConfig = loadConfigFile(ctx);
	if(auto err = EmuSystem::onOptionsLoaded(ctx);
		err)
	{
		ctx.exitWithErrorMessagePrintf(-1, "%s", err->what());
		return;
	}
	auto launchGame = parseCommandArgs(initParams.commandArgs());
	if(launchGame)
		EmuSystem::setInitialLoadPath(launchGame);
	audioManager().setMusicVolumeControlHint();
	if(optionSoundRate > optionSoundRate.defaultVal)
		optionSoundRate.reset();
	emuAudio.setRate(optionSoundRate);
	emuAudio.setAddSoundBuffersOnUnderrun(optionAddSoundBuffersOnUnderrun);
	applyOSNavStyle(ctx, false);

	ctx.addOnResume(
		[this](Base::ApplicationContext ctx, bool focused)
		{
			audioManager().startSession();
			if(soundIsEnabled())
				emuAudio.open(audioOutputAPI());
			if(!keyMapping)
				keyMapping.buildAll(inputDevConf, ctx.inputDevices());
			return true;
		});

	ctx.addOnExit(
		[this](Base::ApplicationContext ctx, bool backgrounded)
		{
			if(backgrounded)
			{
				suspendEmulation(*this);
				if(optionNotificationIcon)
				{
					auto title = string_makePrintf<64>("%s was suspended", ctx.applicationName);
					ctx.addNotification(title.data(), title.data(), EmuSystem::fullGameName().data());
				}
			}
			emuAudio.close();
			audioManager().endSession();

			saveConfigFile(ctx);

			#ifdef CONFIG_BLUETOOTH
			if(bta && (!backgrounded || (backgrounded && !optionKeepBluetoothActive)))
				Bluetooth::closeBT(bta);
			#endif

			ctx.dispatchOnFreeCaches(false);
			keyMapping.free();

			#ifdef CONFIG_BASE_IOS
			//if(backgrounded)
			//	FsSys::remove("/private/var/mobile/Library/Caches/" CONFIG_APP_ID "/com.apple.opengl/shaders.maps");
			#endif

			return true;
		});

	if(ctx.usesPermission(Base::Permission::WRITE_EXT_STORAGE) &&
		!ctx.requestPermission(Base::Permission::WRITE_EXT_STORAGE))
	{
		logMsg("requested external storage write permissions");
	}

	#ifdef CONFIG_INPUT_ANDROID_MOGA
	if(optionMOGAInputSystem)
		ctx.initMogaInputSystem(false);
	#endif

	if(!renderer.supportsColorSpace())
		windowDrawableConf.colorSpace = {};
	emuVideo.setSrgbColorSpaceOutput(windowDrawableConf.colorSpace == Gfx::ColorSpace::SRGB);

	Base::WindowConfig winConf{};
	winConf.setTitle(ctx.applicationName);
	winConf.setFormat(windowDrawableConf.pixelFormat);
	ctx.makeWindow(winConf,
		[this, appConfig](Base::ApplicationContext ctx, Base::Window &win)
		{
			if(auto err = renderer.initMainTask(&win, windowDrawableConfig());
				err)
			{
				ctx.exitWithErrorMessagePrintf(-1, "Error creating renderer: %s", err->what());
				return;
			}
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
			vController.setRenderer(renderer);
			emuVideo.setRendererTask(renderer.task());
			emuVideo.setTextureBufferMode((Gfx::TextureBufferMode)optionTextureBufferMode.val);
			emuVideo.setImageBuffers(optionVideoImageBuffers);
			emuVideoLayer.setLinearFilter(optionImgFilter);
			emuVideoLayer.setOverlayIntensity(optionOverlayEffectLevel/100.);

			viewManager = {renderer};
			viewManager.setNeedsBackControl(appConfig.backNavigation());
			viewManager.setDefaultFace(Gfx::GlyphTextureSet::makeSystem(renderer, IG::FontSettings{}));
			viewManager.setDefaultBoldFace(Gfx::GlyphTextureSet::makeBoldSystem(renderer, IG::FontSettings{}));
			vController.setFace(viewManager.defaultFace());

			win.makeAppData<WindowData>();
			auto &winData = windowData(win);
			setupFont(viewManager, renderer, win);
			winData.projection = updateProjection(makeViewport(win));
			win.setAcceptDnd(true);
			renderer.setWindowValidOrientations(win, optionMenuOrientation);
			updateInputDevices(ctx);
			vController.setWindow(win);
			initVControls();
			ViewAttachParams viewAttach{viewManager, win, renderer.task()};
			emuViewController.emplace(viewAttach, vController, emuVideoLayer, emuSystemTask, emuAudio);
			applyRenderPixelFormat();

			#if defined CONFIG_BASE_ANDROID
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

			onMainWindowCreated(viewAttach, ctx.defaultInputEvent());

			ctx.setOnInterProcessMessage(
				[this](Base::ApplicationContext, const char *path)
				{
					logMsg("got IPC path:%s", path);
					viewController().handleOpenFileCommand(path);
				});

			ctx.setOnScreenChange(
				[this](Base::ApplicationContext ctx, Base::Screen &screen, Base::ScreenChange change)
				{
					viewController().onScreenChange(ctx, screen, change);
				});

			ctx.setOnInputDevicesEnumerated(
				[this, ctx]()
				{
					logMsg("input devs enumerated");
					updateInputDevices(ctx);
					viewController().setPhysicalControlsPresent(ctx.keyInputIsPresent());
					viewController().updateAutoOnScreenControlVisible();
				});

			ctx.setOnInputDeviceChange(
				[this, ctx](const Input::Device &dev, Input::DeviceChange change)
				{
					logMsg("got input dev change");

					updateInputDevices(ctx);
					viewController().setPhysicalControlsPresent(ctx.keyInputIsPresent());
					viewController().updateAutoOnScreenControlVisible();

					if(optionNotifyInputDeviceChange && (change.added() || change.removed()))
					{
						printfMessage(2, 0, "%s #%d %s", dev.name(), dev.enumId() + 1, change.added() ? "connected" : "disconnected");
					}
					else if(change.hadConnectError())
					{
						printfMessage(2, 1, "%s had a connection error", dev.name());
					}

					viewController().onInputDevicesChanged();
				});

			ctx.setOnFreeCaches(
				[this](Base::ApplicationContext, bool running)
				{
					viewManager.defaultFace().freeCaches();
					viewManager.defaultBoldFace().freeCaches();
					if(running)
						viewController().prepareDraw();
				});

			if(auto launchPathStr = EmuSystem::gamePathString();
				strlen(launchPathStr.data()))
			{
				EmuSystem::setInitialLoadPath("");
				viewController().handleOpenFileCommand(launchPathStr.data());
			}

			win.show();
		});
}

Gfx::Projection updateProjection(Gfx::Viewport viewport)
{
	return {viewport, Gfx::Mat4::makePerspectiveFovRH(M_PI/4.0, viewport.realAspectRatio(), 1.0, 100.)};
}

Gfx::Viewport makeViewport(const Base::Window &win)
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

void EmuApp::dispatchOnMainMenuItemOptionChanged()
{
	onMainMenuOptionChanged_.callSafe();
}

void EmuApp::setOnMainMenuItemOptionChanged(OnMainMenuOptionChanged func)
{
	onMainMenuOptionChanged_ = func;
}

void launchSystem(EmuApp &app, bool tryAutoState, bool addToRecent)
{
	if(tryAutoState)
	{
		app.loadAutoState();
		if(!EmuSystem::gameIsRunning())
		{
			logErr("game was closed while trying to load auto-state");
			return;
		}
	}
	if(addToRecent)
		addRecentGame();
	app.viewController().showEmulation();
}

void onSelectFileFromPicker(EmuApp &app, const char* name, Input::Event e, EmuSystemCreateParams params, ViewAttachParams attachParams)
{
	app.createSystemWithMedia({}, name, "", e, params, attachParams,
		[&app](Input::Event e)
		{
			app.launchSystemWithResumePrompt(e, true);
		});
}

void runBenchmarkOneShot(EmuApp &app, EmuVideo &emuVideo)
{
	logMsg("starting benchmark");
	IG::FloatSeconds time = EmuSystem::benchmark(emuVideo);
	app.viewController().closeSystem(false);
	logMsg("done in: %f", time.count());
	app.printfMessage(2, 0, "%.2f fps", double(180.)/time.count());
}

void EmuApp::showEmuation()
{
	if(EmuSystem::gameIsRunning())
	{
		viewController().showEmulation();
	}
}

void EmuApp::launchSystemWithResumePrompt(Input::Event e, bool addToRecent)
{
	if(!viewController().showAutoStateConfirm(e, addToRecent))
	{
		::launchSystem(*this, true, addToRecent);
	}
}

void EmuApp::launchSystem(Input::Event e, bool tryAutoState, bool addToRecent)
{
	::launchSystem(*this, tryAutoState, addToRecent);
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
	pushAndShowModalView(std::make_unique<CollectTextInputView>(attach, msgText, initialContent,
		collectTextCloseAsset(), onText), e);
}

void EmuApp::pushAndShowNewYesNoAlertView(ViewAttachParams attach, Input::Event e, const char *label,
	const char *choice1, const char *choice2, TextMenuItem::SelectDelegate onYes, TextMenuItem::SelectDelegate onNo)
{
	pushAndShowModalView(std::make_unique<YesNoAlertView>(attach, label, choice1, choice2, onYes, onNo), e);
}

void EmuApp::pushAndShowModalView(std::unique_ptr<View> v, Input::Event e)
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

void EmuApp::reloadGame(EmuSystemCreateParams params)
{
	if(!EmuSystem::gameIsRunning())
		return;
	viewController().popToSystemActionsMenu();
	FS::PathString gamePath;
	string_copy(gamePath, EmuSystem::fullGamePath());
	emuSystemTask.pause();
	EmuSystem::Error err{};
	EmuSystem::createWithMedia(viewController().appContext(), {}, gamePath.data(), "", err, params,
		[](int pos, int max, const char *label){ return true; });
	if(!err)
	{
		viewController().onSystemCreated();
		viewController().showEmulation();
	}
}

void EmuApp::promptSystemReloadDueToSetOption(ViewAttachParams attach, Input::Event e, EmuSystemCreateParams params)
{
	if(!EmuSystem::gameIsRunning())
		return;
	auto ynAlertView = std::make_unique<YesNoAlertView>(attach,
		"This option takes effect next time the system starts. Restart it now?");
	ynAlertView->setOnYes(
		[this, params]()
		{
			reloadGame(params);
		});
	viewController().pushAndShowModal(std::move(ynAlertView), e, false);
}

void EmuApp::printfMessage(unsigned secs, bool error, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	auto vaEnd = IG::scopeGuard([&](){ va_end(args); });
	viewController().popupMessageView().vprintf(secs, error, format, args);
}

void EmuApp::postMessage(const char *msg)
{
	postMessage(false, msg);
}

void EmuApp::postMessage(bool error, const char *msg)
{
	postMessage(3, error, msg);
}

void EmuApp::postMessage(unsigned secs, bool error, const char *msg)
{
	viewController().popupMessageView().post(msg, secs, error);
}

void EmuApp::postErrorMessage(const char *msg)
{
	postMessage(true, msg);
}

void EmuApp::postErrorMessage(unsigned secs, const char *msg)
{
	postMessage(secs, true, msg);
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
		printfMessage(2, !success, "%s%d",
			success ? "Wrote screenshot #" : "Error writing screenshot #", num);
	}
}

[[gnu::weak]] bool EmuApp::willCreateSystem(ViewAttachParams attach, Input::Event) { return true; }

void EmuApp::createSystemWithMedia(GenericIO io, const char *path, const char *name, Input::Event e, EmuSystemCreateParams params,
	ViewAttachParams attachParams, CreateSystemCompleteDelegate onComplete)
{
	if(!EmuApp::willCreateSystem(attachParams, e))
	{
		return;
	}
	viewController().closeSystem();
	auto loadProgressView = std::make_unique<EmuLoadProgressView>(attachParams, e, onComplete);
	auto &msgPort = loadProgressView->messagePort();
	pushAndShowModalView(std::move(loadProgressView), e);
	auto app = attachParams.window().appContext();
	IG::makeDetachedThread(
		[app, io{std::move(io)}, pathStr{FS::makePathString(path)}, fileStr{FS::makeFileString(name)}, &msgPort, params]() mutable
		{
			logMsg("starting loader thread");
			EmuSystem::Error err;
			EmuSystem::createWithMedia(app, std::move(io), pathStr.data(), fileStr.data(), err, params,
				[&msgPort](int pos, int max, const char *label)
				{
					int len = label ? strlen(label) : -1;
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
			if(err)
			{
				auto errStr = err->what();
				int len = strlen(errStr);
				assert(len);
				if(len > 1024)
				{
					logWarn("truncating long error size:%d", len);
					len = 1024;
				}
				msgPort.sendWithExtraData({EmuSystem::LoadProgress::FAILED, 0, 0, len}, errStr, len);
				logErr("loader thread failed");
				return;
			}
			msgPort.send({EmuSystem::LoadProgress::OK, 0, 0, 0});
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
	//fixFilePermissions(path);
	syncEmulationThread();
	logMsg("saving state %s", path);
	return EmuSystem::saveState(*this, path);
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
	//fixFilePermissions(path);
	syncEmulationThread();
	logMsg("loading state %s", path);
	return EmuSystem::loadState(*this, path);
}

EmuSystem::Error EmuApp::loadStateWithSlot(int slot)
{
	auto path = EmuSystem::sprintStateFilename(slot);
	return loadState(path.data());
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

void EmuApp::setMediaSearchPath(std::optional<FS::PathString> opt)
{
	if(!opt)
		return;
	lastLoadPath = *opt;
}

FS::PathString EmuApp::firmwareSearchPath()
{
	if(!strlen(optionFirmwarePath))
		return lastLoadPath;
	return hasArchiveExtension(optionFirmwarePath) ? FS::dirname(optionFirmwarePath) : FS::makePathString(optionFirmwarePath);
}

void EmuApp::setFirmwareSearchPath(const char *path)
{
	strncpy(optionFirmwarePath.val, path, sizeof(FS::PathString));
}

[[gnu::weak]] void EmuApp::onMainWindowCreated(ViewAttachParams, Input::Event) {}

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
	assert(EmuSystem::gameIsRunning());
	turboActions.update(this);
}

void EmuApp::resetInput()
{
	turboActions = {};
	viewController().setFastForwardActive(false);
}

FS::PathString EmuApp::assetPath(Base::ApplicationContext ctx)
{
	return ctx.assetPath();
}

FS::PathString EmuApp::libPath(Base::ApplicationContext ctx)
{
	return ctx.libPath();
}

FS::PathString EmuApp::supportPath(Base::ApplicationContext ctx)
{
	return ctx.supportPath();
}

AssetIO EmuApp::openAppAssetIO(Base::ApplicationContext ctx, const char *name, IO::AccessHint access)
{
	return ctx.openAsset(name, access);
}

static FS::PathString sessionConfigPath()
{
	return FS::makePathStringPrintf("%s/%s.config", EmuSystem::savePath(), EmuSystem::gameName().data());
}

bool EmuApp::hasSavedSessionOptions()
{
	return EmuSystem::sessionOptionsSet || FS::exists(sessionConfigPath());
}

void EmuApp::deleteSessionOptions()
{
	if(!hasSavedSessionOptions())
	{
		return;
	}
	EmuSystem::resetSessionOptions(*this);
	EmuSystem::sessionOptionsSet = false;
	FS::remove(sessionConfigPath());
}

void EmuApp::saveSessionOptions()
{
	if(!EmuSystem::sessionOptionsSet)
		return;
	auto configFilePath = sessionConfigPath();
	FileIO configFile;
	if(auto ec = configFile.create(configFilePath.data());
		ec)
	{
		logMsg("error creating session config file:%s", configFilePath.data());
		return;
	}
	writeConfigHeader(configFile);
	EmuSystem::writeSessionConfig(configFile);
	EmuSystem::sessionOptionsSet = false;
	if(configFile.size() == 1)
	{
		// delete file if only header was written
		configFile.close();
		FS::remove(configFilePath);
		logMsg("deleted empty session config file:%s", configFilePath.data());
	}
	else
	{
		logMsg("wrote session config file:%s", configFilePath.data());
	}
}

void EmuApp::loadSessionOptions()
{
	if(!EmuSystem::resetSessionOptions(*this))
		return;
	auto configFilePath = sessionConfigPath();
	FileIO configFile;
	if(auto ec = configFile.open(configFilePath.data(), IO::AccessHint::ALL);
		ec)
	{
		return;
	}
	readConfigKeys(configFile,
		[](uint16_t key, uint16_t size, IO &io)
		{
			switch(key)
			{
				default:
				{
					if(!EmuSystem::readSessionConfig(io, key, size))
					{
						logMsg("skipping unknown key %u", (unsigned)key);
					}
				}
			}
		});
	EmuSystem::onSessionOptionsLoaded(*this);
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

WindowData &windowData(const Base::Window &win)
{
	auto data = win.appData<WindowData>();
	assumeExpr(data);
	return *data;
}

VController &EmuApp::defaultVController()
{
	return vController;
}

void EmuApp::configFrameTime()
{
	EmuSystem::configFrameTime(emuAudio.format().rate);
}

void EmuApp::runFrames(EmuSystemTask *task, EmuVideo *video, EmuAudio *audio, int frames, bool skipForward)
{
	if(skipForward) [[unlikely]]
	{
		if(skipForwardFrames(task, frames - 1))
		{
			// don't write any audio while skip is in progress
			audio = nullptr;
		}
		else
		{
			// restore normal speed when skip ends
			EmuSystem::setSpeedMultiplier(*audio, 1);
		}
	}
	else
	{
		skipFrames(task, frames - 1, audio);
	}
	runTurboInputEvents();
	EmuSystem::runFrame(task, video, audio);
}

void EmuApp::skipFrames(EmuSystemTask *task, uint32_t frames, EmuAudio *audio)
{
	assert(EmuSystem::gameIsRunning());
	iterateTimes(frames, i)
	{
		runTurboInputEvents();
		EmuSystem::runFrame(task, nullptr, audio);
	}
}

bool EmuApp::skipForwardFrames(EmuSystemTask *task, uint32_t frames)
{
	iterateTimes(frames, i)
	{
		skipFrames(task, 1, nullptr);
		if(!EmuSystem::shouldFastForward())
		{
			logMsg("skip-forward ended early after %u frame(s)", i);
			return false;
		}
	}
	return true;
}

void EmuApp::buildKeyInputMapping()
{
	keyMapping.buildAll(inputDevConf, appContext().inputDevices());
}

const KeyMapping &EmuApp::keyInputMapping()
{
	return keyMapping;
}

std::vector<InputDeviceConfig> &EmuApp::inputDeviceConfigs()
{
	return inputDevConf;
}

bool EmuApp::writeScreenshot(IG::Pixmap pix, const char *path)
{
	return pixmapWriter.writeToFile(pix, path);
}

std::pair<int, FS::PathString> EmuApp::makeNextScreenshotFilename()
{
	constexpr int maxNum = 999;
	iterateTimes(maxNum, i)
	{
		auto str = FS::makePathStringPrintf("%s/%s.%.3d.png", EmuSystem::savePath(), EmuSystem::gameName().data(), i);
		if(!FS::exists(str))
		{
			logMsg("screenshot %d", i);
			return {i, str};
		}
	}
	logMsg("no screenshot filenames left");
	return {-1, {}};
}

EmuApp &EmuApp::get(Base::ApplicationContext ctx)
{
	return static_cast<EmuApp&>(ctx.application());
}

namespace Base
{

void ApplicationContext::onInit(ApplicationInitParams initParams)
{
	Gfx::Error rendererErr;
	initApplication<EmuApp>(initParams, *this, rendererErr);
}

}
