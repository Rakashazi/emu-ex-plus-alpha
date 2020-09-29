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
#include "EmuOptions.hh"
#include <emuframework/EmuView.hh>
#include <emuframework/EmuLoadProgressView.hh>
#include <emuframework/EmuVideoLayer.hh>
#include <emuframework/FileUtils.hh>
#include <imagine/base/Base.hh>
#include <imagine/base/platformExtras.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gui/ToastView.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/util/utility.h>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/thread/Thread.hh>
#include <cmath>
#include "private.hh"
#include "privateInput.hh"
#include "configFile.hh"
#include "EmuSystemTask.hh"

class ExitConfirmAlertView : public AlertView
{
public:
	ExitConfirmAlertView(ViewAttachParams attach):
		AlertView
		{
			attach,
			"Really Exit? (Push Back/Escape again to confirm)",
			EmuSystem::gameIsRunning() ? 3u : 2u
		}
	{
		setItem(0, "Yes", [](){ Base::exit(); });
		setItem(1, "No", [](){});
		if(item.size() == 3)
		{
			setItem(2, "Close Menu",
				[]()
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
			Base::exit();
			return true;
		}
		return AlertView::inputEvent(e);
	}
};

Gfx::Renderer renderer;
static Gfx::RendererTask rendererTask{renderer};
static AppWindowData mainWin{};
static EmuSystemTask emuSystemTask{};
EmuViewController emuViewController{mainWin, renderer, rendererTask, vController, emuVideoLayer, emuSystemTask};
EmuVideo emuVideo{rendererTask};
EmuVideoLayer emuVideoLayer{emuVideo};
EmuAudio emuAudio{};
DelegateFunc<void ()> onUpdateInputDevices{};
#ifdef CONFIG_BLUETOOTH
BluetoothAdapter *bta{};
#endif
static EmuApp::OnMainMenuOptionChanged onMainMenuOptionChanged_{};
FS::PathString lastLoadPath{};
#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
SysVController vController{renderer, mainWin, EmuSystem::inputFaceBtns};
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

static Gfx::PixmapTexture assetBuffImg[std::size(assetFilename)];

Gfx::PixmapTexture &getAsset(Gfx::Renderer &r, AssetID assetID)
{
	assumeExpr(assetID < std::size(assetFilename));
	auto &res = assetBuffImg[assetID];
	if(!res)
	{
		PngFile png;
		if(auto ec = png.loadAsset(assetFilename[assetID], appName());
			ec)
		{
			logErr("couldn't load %s", assetFilename[assetID]);
		}
		res = r.makePixmapTexture(png);
	}
	return res;
}

static Gfx::PixmapTexture *getCollectTextCloseAsset(Gfx::Renderer &r)
{
	return Config::envIsAndroid ? nullptr : &getAsset(r, ASSET_CLOSE);
}

void setCPUNeedsLowLatency(bool needed)
{
	#ifdef __ANDROID__
	if(optionSustainedPerformanceMode)
		Base::setSustainedPerformanceMode(needed);
	#endif
}

static void suspendEmulation()
{
	EmuApp::saveAutoState();
	EmuSystem::saveBackupMem();
}

void EmuApp::exitGame(bool allowAutosaveState)
{
	// leave any sub menus that may depending on running game state
	popMenuToRoot();
	emuViewController.closeSystem(allowAutosaveState);
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

void EmuApp::showSystemActionsViewFromSystem(ViewAttachParams attach, Input::Event e)
{
	emuViewController.showSystemActionsView(attach, e);
}

void EmuApp::showLastViewFromSystem(ViewAttachParams attach, Input::Event e)
{
	if(optionSystemActionsIsDefaultMenu)
	{
		showSystemActionsViewFromSystem(attach, e);
	}
	else
	{
		emuViewController.showUI();
	}
}

void EmuApp::showExitAlert(ViewAttachParams attach, Input::Event e)
{
	emuViewController.pushAndShowModal(std::make_unique<ExitConfirmAlertView>(attach), e, false);
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

IG::PixelFormat windowPixelFormat()
{
	#ifdef EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
	return (IG::PixelFormatID)optionWindowPixelFormat.val;
	#else
	return Base::Window::defaultPixelFormat();
	#endif
}

IG::PixelFormat EmuApp::defaultRenderPixelFormat()
{
	auto fmt = windowPixelFormat();
	if(fmt == IG::PIXEL_NONE)
	{
		fmt = Base::Window::defaultPixelFormat();
	}
	return fmt;
}

void mainInitCommon(int argc, char** argv)
{
	using namespace IG;
	Base::registerInstance(appID(), argc, argv);
	Base::setAcceptIPC(appID(), true);
	Base::setOnInterProcessMessage(
		[](const char *filename)
		{
			logMsg("got IPC: %s", filename);
			emuViewController.handleOpenFileCommand(filename);
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
	if(optionSoundRate > optionSoundRate.defaultVal)
		optionSoundRate.reset();
	emuAudio.setAddSoundBuffersOnUnderrun(optionAddSoundBuffersOnUnderrun);
	applyOSNavStyle(false);

	{
		Gfx::Error err{};
		auto threadMode = (Gfx::Renderer::ThreadMode)optionGPUMultiThreading.val;
		renderer = Gfx::Renderer::makeConfiguredRenderer(threadMode, windowPixelFormat(), err);
		if(err)
		{
			Base::exitWithErrorMessagePrintf(-1, "Error creating renderer: %s", err->what());
			return;
		}
		if(!renderer.supportsThreadMode())
			optionGPUMultiThreading.reset();
	}

	auto compiled = renderer.texAlphaProgram.compile(renderer);
	compiled |= renderer.noTexProgram.compile(renderer);
	compiled |= View::compileGfxPrograms(renderer);
	if(compiled)
		renderer.autoReleaseShaderCompiler();

	View::defaultFace = Gfx::GlyphTextureSet::makeSystem(renderer, IG::FontSettings{});
	View::defaultBoldFace = Gfx::GlyphTextureSet::makeBoldSystem(renderer, IG::FontSettings{});

	#ifdef CONFIG_INPUT_ANDROID_MOGA
	if(optionMOGAInputSystem)
		Input::initMOGA(false);
	#endif
	updateInputDevices();

	if(optionTextureBufferMode.val)
	{
		auto mode = (Gfx::TextureBufferMode)optionTextureBufferMode.val;
		if(renderer.makeValidTextureBufferMode(mode) != mode)
		{
			// reset to default if saved non-default mode isn't supported
			optionTextureBufferMode.reset();
		}
	}
	emuVideo.setTextureBufferMode((Gfx::TextureBufferMode)optionTextureBufferMode.val);
	emuVideoLayer.setLinearFilter(optionImgFilter);
	emuVideoLayer.setOverlayIntensity(optionOverlayEffectLevel/100.);

	Base::addOnResume(
		[](bool focused)
		{
			AudioManager::startSession();
			if(soundIsEnabled())
				emuAudio.open(audioOutputAPI());
			if(!keyMapping)
				keyMapping.buildAll();
			rendererTask.start();
			if(optionShowOnSecondScreen && Base::Screen::screens() > 1)
			{
				emuViewController.setEmuViewOnExtraWindow(true, *Base::Screen::screen(1));
			}
			return true;
		});

	Base::setOnFreeCaches(
		[]()
		{
			View::defaultFace.freeCaches();
			View::defaultBoldFace.freeCaches();
		});

	Base::addOnExit(
		[](bool backgrounded)
		{
			if(backgrounded)
			{
				emuViewController.showUI();
				suspendEmulation();
				if(optionShowOnSecondScreen && Base::Screen::screens() > 1)
				{
					emuViewController.setEmuViewOnExtraWindow(false, *Base::Screen::screen(1));
				}
				if(optionNotificationIcon)
				{
					auto title = string_makePrintf<64>("%s was suspended", appName());
					Base::addNotification(title.data(), title.data(), EmuSystem::fullGameName().data());
				}
			}
			else
			{
				emuViewController.closeSystem();
				emuVideo.deleteImage();
			}
			emuAudio.close();
			AudioManager::endSession();

			saveConfigFile();

			#ifdef CONFIG_BLUETOOTH
			if(bta && (!backgrounded || (backgrounded && !optionKeepBluetoothActive)))
				Bluetooth::closeBT(bta);
			#endif

			Base::dispatchOnFreeCaches();
			keyMapping.free();

			#ifdef CONFIG_BASE_IOS
			//if(backgrounded)
			//	FsSys::remove("/private/var/mobile/Library/Caches/" CONFIG_APP_ID "/com.apple.opengl/shaders.maps");
			#endif

			return true;
		});

	Base::Screen::setOnChange(
		[](Base::Screen &screen, Base::Screen::Change change)
		{
			emuViewController.onScreenChange(screen, change);
		});

	Input::setOnDevicesEnumerated(
		[]()
		{
			logMsg("input devs enumerated");
			updateInputDevices();
			emuViewController.updateAutoOnScreenControlVisible();
		});

	Input::setOnDeviceChange(
		[](const Input::Device &dev, Input::Device::Change change)
		{
			logMsg("got input dev change");

			updateInputDevices();
			emuViewController.updateAutoOnScreenControlVisible();

			if(optionNotifyInputDeviceChange && (change.added() || change.removed()))
			{
				EmuApp::printfMessage(2, 0, "%s #%d %s", dev.name(), dev.enumId() + 1, change.added() ? "connected" : "disconnected");
			}
			else if(change.hadConnectError())
			{
				EmuApp::printfMessage(2, 1, "%s had a connection error", dev.name());
			}

			emuViewController.onInputDevicesChanged();
		});

	if(Base::usesPermission(Base::Permission::WRITE_EXT_STORAGE) &&
		!Base::requestPermission(Base::Permission::WRITE_EXT_STORAGE))
	{
		logMsg("requested external storage write permissions");
	}
	Base::WindowConfig winConf = emuViewController.addWindowConfig({}, mainWin);
	winConf.setCustomData(&mainWin);
	renderer.initWindow(mainWin.win, winConf);
	auto &win = mainWin.win;
	setupFont(renderer, win);
	updateProjection(mainWin, makeViewport(win));
	win.setTitle(appName());
	win.setAcceptDnd(true);
	renderer.setWindowValidOrientations(win, optionMenuOrientation);
	initVControls(renderer);

	#if defined CONFIG_BASE_ANDROID
	if(!Base::apkSignatureIsConsistent())
	{
		auto ynAlertView = std::make_unique<YesNoAlertView>(ViewAttachParams{win, rendererTask}, "Warning: App has been modified by 3rd party, use at your own risk");
		ynAlertView->setOnNo(
			[]()
			{
				Base::exit();
			});
		emuViewController.pushAndShowModal(std::move(ynAlertView), Input::defaultEvent(), false);
	}
	#endif

	ViewAttachParams viewAttach{mainWin.win, rendererTask};
	emuViewController.initViews(viewAttach);
	win.show();
	win.postDraw();
	EmuApp::onMainWindowCreated(viewAttach, Input::defaultEvent());
	if(launchGame)
	{
		emuViewController.handleOpenFileCommand(launchGame);
	}
}

void updateProjection(AppWindowData &appWin, const Gfx::Viewport &viewport)
{
	appWin.projectionMat = Gfx::Mat4::makePerspectiveFovRH(M_PI/4.0, viewport.realAspectRatio(), 1.0, 100.);
	appWin.projectionPlane = Gfx::ProjectionPlane::makeWithMatrix(viewport, appWin.projectionMat);
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

void onMainMenuItemOptionChanged()
{
	onMainMenuOptionChanged_.callSafe();
}

void EmuApp::setOnMainMenuItemOptionChanged(OnMainMenuOptionChanged func)
{
	onMainMenuOptionChanged_ = func;
}

void launchSystem(bool tryAutoState, bool addToRecent)
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
	emuViewController.showEmulation();
}

void onSelectFileFromPicker(const char* name, Input::Event e)
{
	EmuApp::createSystemWithMedia({}, name, "", e,
		[](Input::Event e)
		{
			EmuApp::launchSystemWithResumePrompt(e, true);
		});
}

void runBenchmarkOneShot()
{
	logMsg("starting benchmark");
	IG::FloatSeconds time = EmuSystem::benchmark();
	emuViewController.closeSystem(false);
	logMsg("done in: %f", time.count());
	EmuApp::printfMessage(2, 0, "%.2f fps", double(180.)/time.count());
}

void EmuApp::showEmuation()
{
	if(EmuSystem::gameIsRunning())
	{
		emuViewController.showEmulation();
	}
}

void EmuApp::launchSystemWithResumePrompt(Input::Event e, bool addToRecent)
{
	if(!emuViewController.showAutoStateConfirm(e, addToRecent))
	{
		::launchSystem(true, addToRecent);
	}
}

void EmuApp::launchSystem(Input::Event e, bool tryAutoState, bool addToRecent)
{
	::launchSystem(tryAutoState, addToRecent);
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
		getCollectTextCloseAsset(attach.renderer()), onText), e);
}

void EmuApp::pushAndShowNewYesNoAlertView(ViewAttachParams attach, Input::Event e, const char *label,
	const char *choice1, const char *choice2, TextMenuItem::SelectDelegate onYes, TextMenuItem::SelectDelegate onNo)
{
	pushAndShowModalView(std::make_unique<YesNoAlertView>(attach, label, choice1, choice2, onYes, onNo), e);
}

void EmuApp::pushAndShowModalView(std::unique_ptr<View> v, Input::Event e)
{
	emuViewController.pushAndShowModal(std::move(v), e, false);
}

void EmuApp::popModalViews()
{
	emuViewController.popModalViews();
}

void EmuApp::popMenuToRoot()
{
	emuViewController.popToRoot();
}

void EmuApp::reloadGame()
{
	if(!EmuSystem::gameIsRunning())
		return;
	emuViewController.popToSystemActionsMenu();
	FS::PathString gamePath;
	string_copy(gamePath, EmuSystem::fullGamePath());
	emuSystemTask.pause();
	EmuSystem::Error err{};
	EmuSystem::createWithMedia({}, gamePath.data(), "", err, [](int pos, int max, const char *label){ return true; });
	if(!err)
	{
		EmuSystem::prepareAudioVideo();
		emuViewController.onSystemCreated();
		emuViewController.showEmulation();
	}
}

void EmuApp::promptSystemReloadDueToSetOption(ViewAttachParams attach, Input::Event e)
{
	if(!EmuSystem::gameIsRunning())
		return;
	auto ynAlertView = std::make_unique<YesNoAlertView>(attach,
		"This option takes effect next time the system starts. Restart it now?");
	ynAlertView->setOnYes(
		[]()
		{
			reloadGame();
		});
	emuViewController.pushAndShowModal(std::move(ynAlertView), e, false);
}

void EmuApp::printfMessage(uint secs, bool error, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	auto vaEnd = IG::scopeGuard([&](){ va_end(args); });
	emuViewController.popupMessageView().vprintf(secs, error, format, args);
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
	emuViewController.popupMessageView().post(msg, secs, error);
}

void EmuApp::postErrorMessage(const char *msg)
{
	postMessage(true, msg);
}

void EmuApp::postErrorMessage(uint secs, const char *msg)
{
	postMessage(secs, true, msg);
}

void EmuApp::unpostMessage()
{
	emuViewController.popupMessageView().clear();
}

void EmuApp::printScreenshotResult(int num, bool success)
{
	if(num == -1)
	{
		EmuApp::postErrorMessage("Too many screenshots");
	}
	else
	{
		EmuApp::printfMessage(2, !success, "%s%d",
			success ? "Wrote screenshot #" : "Error writing screenshot #", num);
	}
}

ViewAttachParams emuViewAttachParams()
{
	return {mainWin.win, rendererTask};
}

[[gnu::weak]] bool EmuApp::willCreateSystem(ViewAttachParams attach, Input::Event) { return true; }

void EmuApp::createSystemWithMedia(GenericIO io, const char *path, const char *name, Input::Event e, CreateSystemCompleteDelegate onComplete)
{
	if(!EmuApp::willCreateSystem(emuViewAttachParams(), e))
	{
		return;
	}
	emuViewController.closeSystem();
	auto loadProgressView = std::make_unique<EmuLoadProgressView>(emuViewAttachParams(), e, onComplete);
	auto &msgPort = loadProgressView->messagePort();
	pushAndShowModalView(std::move(loadProgressView), e);
	IG::makeDetachedThread(
		[io{std::move(io)}, pathStr{FS::makePathString(path)}, fileStr{FS::makeFileString(name)}, &msgPort]() mutable
		{
			logMsg("starting loader thread");
			EmuSystem::Error err;
			EmuSystem::createWithMedia(std::move(io), pathStr.data(), fileStr.data(), err,
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
	fixFilePermissions(path);
	syncEmulationThread();
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
	syncEmulationThread();
	logMsg("loading state %s", path);
	return EmuSystem::loadState(path);
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

void EmuApp::setMediaSearchPath(FS::PathString path)
{
	lastLoadPath = path;
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

void EmuApp::addTurboInputEvent(uint action)
{
	turboActions.addEvent(action);
}

void EmuApp::removeTurboInputEvent(uint action)
{
	turboActions.removeEvent(action);
}

FS::PathString EmuApp::assetPath()
{
	return Base::assetPath(appName());
}

FS::PathString EmuApp::libPath()
{
	return Base::libPath(appName());
}

FS::PathString EmuApp::supportPath()
{
	return Base::supportPath(appName());
}

AssetIO EmuApp::openAppAssetIO(const char *name, IO::AccessHint access)
{
	return FileUtils::openAppAsset(name, access, appName());
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
	EmuSystem::resetSessionOptions();
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
	if(!EmuSystem::resetSessionOptions())
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
						logMsg("skipping unknown key %u", (uint)key);
					}
				}
			}
		});
	EmuSystem::onSessionOptionsLoaded();
}

void EmuApp::syncEmulationThread()
{
	emuSystemTask.pause();
}

AppWindowData &appWindowData(const Base::Window &win)
{
	auto data = win.customData<AppWindowData>();
	assumeExpr(data);
	return *data;
}

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
