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
#include "private.hh"
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

EmuApp::EmuApp(IG::ApplicationInitParams initParams, IG::ApplicationContext &ctx):
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
	vibrationManager_{ctx}
{
	EmuSystem::onInit(ctx);
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
		auto img = pixmapReader.loadAsset(assetFilename[assetIdx]);
		if(!img)
		{
			logErr("couldn't load %s", assetFilename[assetIdx]);
			return res;
		}
		res = renderer.makePixmapTexture(img, &renderer.get(View::imageCommonTextureSampler));
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

void EmuApp::setCPUNeedsLowLatency(IG::ApplicationContext ctx, bool needed)
{
	#ifdef __ANDROID__
	if(optionSustainedPerformanceMode)
		ctx.setSustainedPerformanceMode(needed);
	#endif
}

static void suspendEmulation(EmuApp &app)
{
	if(!EmuSystem::gameIsRunning())
		return;
	app.saveAutoState();
	EmuSystem::saveBackupMem(app.appContext());
}

void EmuApp::exitGame(bool allowAutosaveState)
{
	// leave any sub menus that may depending on running game state
	popMenuToRoot();
	viewController().closeSystem(allowAutosaveState);
}

void EmuApp::applyOSNavStyle(IG::ApplicationContext ctx, bool inGame)
{
	auto flags = IG::SYS_UI_STYLE_NO_FLAGS;
	if(optionLowProfileOSNav > (inGame ? 0 : 1))
		flags |= IG::SYS_UI_STYLE_DIM_NAV;
	if(optionHideOSNav > (inGame ? 0 : 1))
		flags |= IG::SYS_UI_STYLE_HIDE_NAV;
	if(optionHideStatusBar > (inGame ? 0 : 1))
		flags |= IG::SYS_UI_STYLE_HIDE_STATUS;
	ctx.setSysUIStyle(flags);
}

IG::Audio::Manager &EmuApp::audioManager()
{
	return audioManager_;
}

IG::ApplicationContext EmuApp::appContext() const
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

static const char *parseCommandArgs(IG::CommandArgs arg)
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
	emuVideoLayer.setFormat(fmt, imageEffectPixelFormat(), windowDrawableConf.colorSpace);
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

void EmuApp::updateLegacySavePath(IG::ApplicationContext ctx, IG::CStringView path)
{
	auto oldSaveSubDirs = subDirectoryStrings(ctx, path);
	if(oldSaveSubDirs.empty())
	{
		logMsg("no legacy save folders in:%s", path.data());
		return;
	}
	flattenSubDirectories(ctx, std::move(oldSaveSubDirs), path);
}

void EmuApp::mainInitCommon(IG::ApplicationInitParams initParams, IG::ApplicationContext ctx)
{
	if(ctx.registerInstance(initParams))
	{
		ctx.exit();
		return;
	}
	ctx.setAcceptIPC(true);
	ctx.setOnInterProcessMessage(
		[this](IG::ApplicationContext, const char *path)
		{
			logMsg("got IPC path:%s", path);
			if(emuViewController)
				viewController().handleOpenFileCommand(path);
			else
				EmuSystem::setInitialLoadPath(path);
		});
	initOptions(ctx);
	auto appConfig = loadConfigFile(ctx);
	EmuSystem::onOptionsLoaded(ctx);
	updateLegacySavePathOnStoragePath(ctx);
	auto launchGame = parseCommandArgs(initParams.commandArgs());
	if(launchGame)
		EmuSystem::setInitialLoadPath(launchGame);
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
					ctx.addNotification(title, title, EmuSystem::contentDisplayName());
				}
			}
			emuAudio.close();
			audioManager().endSession();

			saveConfigFile(ctx);

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
			viewManager.setDefaultFace({renderer, fontManager.makeSystem(), IG::FontSettings{}});
			viewManager.setDefaultBoldFace({renderer, fontManager.makeBoldSystem(), IG::FontSettings{}});
			win.makeAppData<WindowData>();
			auto &winData = windowData(win);
			setupFont(viewManager, renderer, win);
			winData.projection = updateProjection(makeViewport(win));
			win.setAcceptDnd(true);
			renderer.setWindowValidOrientations(win, optionMenuOrientation);
			updateInputDevices(ctx);
			vController.configure(win, renderer, viewManager.defaultFace());
			vController.setMenuImage(asset(EmuApp::AssetID::MENU));
			vController.setFastForwardImage(asset(EmuApp::AssetID::FAST_FORWARD));
			if constexpr(Config::EmuFramework::VCONTROLS_GAMEPAD)
			{
				vController.setImg(asset(AssetID::GAMEPAD_OVERLAY));
			}
			if(EmuSystem::inputHasKeyboard)
			{
				vController.setKeyboardImage(asset(AssetID::KEYBOARD_OVERLAY));
			}
			ViewAttachParams viewAttach{viewManager, win, renderer.task()};
			emuViewController.emplace(viewAttach, vController, emuVideoLayer,
				emuSystemTask, emuAudio);
			if(!appConfig.rendererPresentationTime())
				emuViewController->setUsePresentationTime(false);
			emuVideo.setRendererTask(renderer.task());
			emuVideo.setTextureBufferMode((Gfx::TextureBufferMode)optionTextureBufferMode.val);
			emuVideo.setImageBuffers(optionVideoImageBuffers);
			emuVideoLayer.setLinearFilter(optionImgFilter); // init the texture sampler before setting format
			applyRenderPixelFormat();
			emuVideoLayer.setOverlayIntensity(optionOverlayEffectLevel/100.);
			emuVideoLayer.setEffect((ImageEffectId)optionImgEffect.val, imageEffectPixelFormat());

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

			onMainWindowCreated(viewAttach, ctx.defaultInputEvent());

			ctx.setOnInterProcessMessage(
				[this](IG::ApplicationContext, const char *path)
				{
					logMsg("got IPC path:%s", path);
					viewController().handleOpenFileCommand(path);
				});

			ctx.setOnScreenChange(
				[this](IG::ApplicationContext ctx, IG::Screen &screen, IG::ScreenChange change)
				{
					viewController().onScreenChange(ctx, screen, change);
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

			if(auto launchPathStr = EmuSystem::contentLocation();
				launchPathStr.size())
			{
				EmuSystem::setInitialLoadPath("");
				viewController().handleOpenFileCommand(launchPathStr);
			}

			win.show();
		});
}

Gfx::Projection updateProjection(Gfx::Viewport viewport)
{
	return {viewport, Gfx::Mat4::makePerspectiveFovRH(M_PI/4.0, viewport.realAspectRatio(), 1.0, 100.)};
}

Gfx::Viewport makeViewport(const IG::Window &win)
{
	if((int)optionViewportZoom != 100)
	{
		auto viewRect = win.contentBounds();
		IG::WP viewCenter {(int)viewRect.xSize()/2, (int)viewRect.ySize()/2};
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

void launchSystem(EmuApp &app, bool tryAutoState)
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
	app.viewController().showEmulation();
}

void onSelectFileFromPicker(EmuApp &app, GenericIO io, IG::CStringView path, std::string_view displayName,
	Input::Event e, EmuSystemCreateParams params, ViewAttachParams attachParams)
{
	app.createSystemWithMedia(std::move(io), path, displayName, e, params, attachParams,
		[&app, path](Input::Event e)
		{
			app.addCurrentContentToRecent();
			app.launchSystemWithResumePrompt(e);
		});
}

void runBenchmarkOneShot(EmuApp &app, EmuVideo &emuVideo)
{
	logMsg("starting benchmark");
	IG::FloatSeconds time = EmuSystem::benchmark(emuVideo);
	app.viewController().closeSystem(false);
	logMsg("done in: %f", time.count());
	app.postMessage(2, 0, fmt::format("{:.2f} fps", double(180.)/time.count()));
}

void EmuApp::showEmuation()
{
	if(EmuSystem::gameIsRunning())
	{
		viewController().showEmulation();
	}
}

void EmuApp::launchSystemWithResumePrompt(Input::Event e)
{
	if(optionAutoSaveState && optionConfirmAutoLoadState)
	{
		if(!viewController().showAutoStateConfirm(e))
		{
			// state doesn't exist
			EmuEx::launchSystem(*this, false);
		}
	}
	else
	{
		EmuEx::launchSystem(*this, optionAutoSaveState);
	}
}

void EmuApp::launchSystem(Input::Event e, bool tryAutoState)
{
	EmuEx::launchSystem(*this, tryAutoState);
}

bool EmuApp::hasArchiveExtension(std::string_view name)
{
	return FS::hasArchiveExtension(name);
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
	emuSystemTask.pause();
	auto ctx = appContext();
	try
	{
		EmuSystem::createWithMedia(ctx, {}, EmuSystem::contentLocation(),
			ctx.fileUriDisplayName(EmuSystem::contentLocation()), params,
			[](int pos, int max, const char *label){ return true; });
		viewController().onSystemCreated();
		viewController().showEmulation();
	}
	catch(...)
	{
		logErr("Error reloading game");
		EmuSystem::clearGamePaths();
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

[[gnu::weak]] bool EmuApp::willCreateSystem(ViewAttachParams attach, Input::Event) { return true; }

void EmuApp::createSystemWithMedia(GenericIO io, IG::CStringView path, std::string_view displayName,
	Input::Event e, EmuSystemCreateParams params, ViewAttachParams attachParams,
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
	viewController().closeSystem();
	auto loadProgressView = std::make_unique<EmuLoadProgressView>(attachParams, e, onComplete);
	auto &msgPort = loadProgressView->messagePort();
	pushAndShowModalView(std::move(loadProgressView), e);
	auto app = attachParams.window().appContext();
	IG::makeDetachedThread(
		[app, io{std::move(io)}, pathStr = FS::PathString{path}, nameStr = FS::FileString{displayName}, &msgPort, params]() mutable
		{
			logMsg("starting loader thread");
			try
			{
				EmuSystem::createWithMedia(app, std::move(io), pathStr, nameStr, params,
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
				EmuSystem::clearGamePaths();
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
		saveState(EmuSystem::statePath(appContext(), -1));
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
	if(!EmuSystem::gameIsRunning())
	{
		postErrorMessage("System not running");
		return false;
	}
	syncEmulationThread();
	logMsg("saving state %s", path.data());
	try
	{
		EmuSystem::saveState(appContext(), path);
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
	return saveState(EmuSystem::statePath(appContext(), slot));
}

bool EmuApp::loadState(IG::CStringView path)
{
	if(!EmuSystem::gameIsRunning()) [[unlikely]]
	{
		postErrorMessage("System not running");
		return false;
	}
	logMsg("loading state %s", path.data());
	syncEmulationThread();
	try
	{
		EmuSystem::loadState(*this, path);
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
	return loadState(EmuSystem::statePath(appContext(), slot));
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
	auto firmwarePath = EmuSystem::firmwarePath();
	if(firmwarePath.empty() || !ctx.fileUriExists(firmwarePath))
		return contentSearchPath();
	return hasArchiveExtension(firmwarePath) ? FS::dirnameUri(firmwarePath) : firmwarePath;
}

void EmuApp::setFirmwareSearchPath(std::string_view path)
{
	EmuSystem::setFirmwarePath(path);
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

FS::PathString EmuApp::sessionConfigPath()
{
	return EmuSystem::contentSaveFilePath(appContext(), ".config");
}

bool EmuApp::hasSavedSessionOptions()
{
	return EmuSystem::sessionOptionsSet || appContext().fileUriExists(sessionConfigPath());
}

void EmuApp::deleteSessionOptions()
{
	if(!hasSavedSessionOptions())
	{
		return;
	}
	EmuSystem::resetSessionOptions(*this);
	EmuSystem::sessionOptionsSet = false;
	appContext().removeFileUri(sessionConfigPath());
}

void EmuApp::saveSessionOptions()
{
	if(!EmuSystem::sessionOptionsSet)
		return;
	auto configFilePath = sessionConfigPath();
	try
	{
		auto ctx = appContext();
		auto configFile = ctx.openFileUri(configFilePath, IO::OPEN_CREATE);
		writeConfigHeader(configFile);
		EmuSystem::writeSessionConfig(configFile);
		EmuSystem::sessionOptionsSet = false;
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
	if(!EmuSystem::resetSessionOptions(*this))
		return;
	auto configFilePath = sessionConfigPath();
	auto configBuff = FileUtils::bufferFromUri(appContext(), configFilePath, IO::OPEN_TEST);
	if(!configBuff)
		return;
	readConfigKeys(std::move(configBuff),
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

WindowData &windowData(const IG::Window &win)
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
			EmuSystem::setSpeedMultiplier(*audio, 1);
		}
	}
	else
	{
		skipFrames(taskCtx, frames - 1, audio);
	}
	runTurboInputEvents();
	EmuSystem::runFrame(taskCtx, video, audio);
}

void EmuApp::skipFrames(EmuSystemTaskContext taskCtx, uint32_t frames, EmuAudio *audio)
{
	assert(EmuSystem::gameIsRunning());
	iterateTimes(frames, i)
	{
		runTurboInputEvents();
		EmuSystem::runFrame(taskCtx, nullptr, audio);
	}
}

bool EmuApp::skipForwardFrames(EmuSystemTaskContext taskCtx, uint32_t frames)
{
	iterateTimes(frames, i)
	{
		skipFrames(taskCtx, 1, nullptr);
		if(!EmuSystem::shouldFastForward())
		{
			logMsg("skip-forward ended early after %u frame(s)", i);
			return false;
		}
	}
	return true;
}

bool EmuApp::writeScreenshot(IG::Pixmap pix, IG::CStringView path)
{
	return pixmapWriter.writeToFile(pix, path);
}

std::pair<int, FS::PathString> EmuApp::makeNextScreenshotFilename()
{
	static constexpr int maxNum = 999;
	auto ctx = appContext();
	auto basePath = EmuSystem::contentSavePath(ctx, EmuSystem::contentName());
	iterateTimes(maxNum, i)
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
	return emuViewController->inputView().attachParams();
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
	addRecentContent(EmuSystem::contentLocation(), EmuSystem::contentDisplayName());
}

void EmuApp::setSoundRate(uint32_t rate)
{
	optionSoundRate = rate;
	EmuSystem::configAudioPlayback(audio(), rate);
}

void EmuApp::setFontSize(int size)
{
	optionFontSize = size;
	setupFont(viewManager, renderer, viewController().emuWindow());
	viewController().placeElements();
}

EmuApp &EmuApp::get(IG::ApplicationContext ctx)
{
	return static_cast<EmuApp&>(ctx.application());
}

}

namespace IG
{

void ApplicationContext::onInit(ApplicationInitParams initParams)
{
	initApplication<EmuEx::EmuApp>(initParams, *this);
}

}
