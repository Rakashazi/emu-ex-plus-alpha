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

#define LOGTAG "EmuViewController"
#include <emuframework/EmuViewController.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppHelper.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuView.hh>
#include <emuframework/EmuVideoLayer.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuMainMenuView.hh>
#include <emuframework/FilePicker.hh>
#include "EmuOptions.hh"
#include "private.hh"
#include "WindowData.hh"
#include "configFile.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Screen.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/gui/ToastView.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>

namespace EmuEx
{

class AutoStateConfirmAlertView : public YesNoAlertView, public EmuAppHelper<AutoStateConfirmAlertView>
{
public:
	AutoStateConfirmAlertView(ViewAttachParams attach, std::string_view dateStr):
		YesNoAlertView
		{
			attach,
			"",
			"Continue",
			"Restart Game",
			[this]()
			{
				launchSystem(app(), true);
			},
			[this]()
			{
				launchSystem(app(), false);
			}
		}
	{
		setLabel(fmt::format("Auto-save state exists from:\n{}", dateStr));
	}
};

EmuViewController::EmuViewController() {}

EmuViewController::EmuViewController(ViewAttachParams viewAttach,
	VController &vCtrl, EmuVideoLayer &videoLayer, EmuSystemTask &systemTask,
	EmuAudio &emuAudio, EmuSystem &sys):
	emuView{viewAttach, &videoLayer, sys},
	emuInputView{viewAttach, vCtrl, videoLayer},
	popup{viewAttach},
	rendererTask_{&viewAttach.rendererTask()},
	systemTaskPtr{&systemTask},
	emuAudioPtr{&emuAudio},
	appPtr{&EmuApp::get(viewAttach.appContext())},
	onExit
	{
		[this](IG::ApplicationContext ctx, bool backgrounded)
		{
			if(backgrounded)
			{
				showUI();
				if(app().showOnSecondScreenOption() && ctx.screens().size() > 1)
				{
					setEmuViewOnExtraWindow(false, *ctx.screens()[1]);
				}
				viewStack.top().onHide();
				ctx.addOnResume(
					[this](IG::ApplicationContext, bool focused)
					{
						configureSecondaryScreens();
						prepareDraw();
						if(showingEmulation && focused && system().isPaused())
						{
							logMsg("resuming emulation due to app resume");
							#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
							emuInputView.activeVController()->resetInput();
							#endif
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
		}, viewAttach.appContext(), -10
	}
{
	emuInputView.setController(this);
	auto &win = viewAttach.window();

	win.setOnInputEvent(
		[this](IG::Window &win, const Input::Event &e)
		{
			return inputEvent(e);
		});

	win.setOnFocusChange(
		[this](IG::Window &win, bool in)
		{
			windowData(win).focused = in;
			onFocusChange(in);
		});

	win.setOnDragDrop(
		[this](IG::Window &win, const char *filename)
		{
			logMsg("got DnD: %s", filename);
			handleOpenFileCommand(filename);
		});

	win.setOnSurfaceChange(
		[this](IG::Window &win, IG::Window::SurfaceChange change)
		{
			auto &winData = windowData(win);
			rendererTask().updateDrawableForSurfaceChange(win, change);
			if(change.resized())
			{
				updateWindowViewport(win, change);
				if(winData.hasEmuView)
				{
					emuView.setViewRect(winData.viewport().bounds(), winData.projection.plane());
				}
				emuInputView.setViewRect(winData.viewport().bounds(), winData.projection.plane());
				placeElements();
			}
		});

	win.setOnDraw(
		[this](IG::Window &win, IG::Window::DrawParams params)
		{
			auto &winData = windowData(win);
			return rendererTask().draw(win, params, {}, winData.viewport(), winData.projection.matrix(),
				[this](IG::Window &win, Gfx::RendererCommands &cmds)
				{
					auto &winData = windowData(win);
					cmds.clear();
					drawMainWindow(win, cmds, winData.hasEmuView, winData.hasPopup);
				});
		});

	initViews(viewAttach);
	configureSecondaryScreens();
}

static bool shouldExitFromViewRootWithoutPrompt(const Input::KeyEvent &e)
{
	return e.map() == Input::Map::SYSTEM && (Config::envIsAndroid || Config::envIsLinux);
}

EmuMenuViewStack::EmuMenuViewStack(EmuViewController &emuViewController):
	ViewStack{}, emuViewControllerPtr{&emuViewController}
{}

bool EmuMenuViewStack::inputEvent(const Input::Event &e)
{
	if(ViewStack::inputEvent(e))
	{
		return true;
	}
	if(e.keyEvent())
	{
		auto &keyEv = e.asKeyEvent();
		bool hasEmuContent = viewController().app().system().hasContent();
		if(keyEv.pushed(Input::DefaultKey::CANCEL))
		{
			if(size() == 1)
			{
				//logMsg("cancel button at view stack root");
				if(keyEv.repeated())
				{
					return true;
				}
				if(hasEmuContent || (!hasEmuContent && !shouldExitFromViewRootWithoutPrompt(keyEv)))
				{
					viewController().app().showExitAlert(top().attachParams(), e);
				}
				else
				{
					viewController().appContext().exit();
				}
			}
			else
			{
				popAndShow();
			}
			return true;
		}
		if(keyEv.pushed() && viewController().isMenuDismissKey(keyEv) && !hasModalView())
		{
			viewController().showEmulation();
			return true;
		}
	}
	return false;
}

void EmuViewController::initViews(ViewAttachParams viewAttach)
{
	auto &winData = windowData(viewAttach.window());
	auto &face = viewAttach.viewManager().defaultFace();
	auto &screen = *viewAttach.window().screen();
	winData.hasEmuView = true;
	winData.hasPopup = true;
	if(!screen.supportsTimestamps() && (!Config::envIsLinux || screen.frameRate() < 100.))
	{
		setWindowFrameClockSource(IG::Window::FrameTimeSource::RENDERER);
	}
	else
	{
		setWindowFrameClockSource(IG::Window::FrameTimeSource::SCREEN);
	}
	logMsg("timestamp source:%s", useRendererTime() ? "renderer" : "screen");
	onFrameUpdate = [this, &r = std::as_const(viewAttach.renderer())](IG::FrameParams params)
		{
			bool skipForward = false;
			bool fastForwarding = false;
			auto &audio = emuAudio();
			auto &sys = system();
			if(sys.shouldFastForward()) [[unlikely]]
			{
				// for skipping loading on disk-based computers
				fastForwarding = true;
				skipForward = true;
				sys.setSpeedMultiplier(audio, 8);
			}
			else if(targetFastForwardSpeed > 1) [[unlikely]]
			{
				fastForwarding = true;
				sys.setSpeedMultiplier(audio, targetFastForwardSpeed);
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
			if(!app().shouldSkipLateFrames() && !fastForwarding)
			{
				frameInfo.advanced = app().frameInterval();
			}
			constexpr unsigned maxFrameSkip = 8;
			uint32_t framesToEmulate = std::min(frameInfo.advanced, maxFrameSkip);
			EmuAudio *audioPtr = audio ? &audio : nullptr;
			/*logMsg("frame present time:%.4f next display frame:%.4f",
				std::chrono::duration_cast<IG::FloatSeconds>(frameInfo.presentTime).count(),
				std::chrono::duration_cast<IG::FloatSeconds>(params.presentTime()).count());*/
			auto &video = videoLayer().emuVideo();
			if(framesToEmulate == 1)
			{
				// run common 1-frame case synced until the video frame is ready for more consistent timing
				emuTask().runFrame(&video, audioPtr, 1, false, true);
				if(emuTask().resetVideoFormatChanged())
				{
					video.dispatchFormatChanged();
				}
				emuWindow().setNeedsDraw(true);
				if(usePresentationTime())
					r.setPresentationTime(emuWindow(), params.presentTime());
				return true;
			}
			else
			{
				// run multiple frames async and let main loop collect additional input events
				emuTask().runFrame(&video, audioPtr, framesToEmulate, skipForward, false);
				if(usePresentationTime())
					r.setPresentationTime(emuWindow(), params.presentTime());
				return false;
			}
		};

	popup.setFace(face);
	{
		auto viewNav = std::make_unique<BasicNavView>
		(
			viewAttach,
			&face,
			&app().asset(EmuApp::AssetID::ARROW),
			&app().asset(EmuApp::AssetID::GAME_ICON)
		);
		viewNav->setRotateLeftButton(true);
		viewNav->setOnPushLeftBtn(
			[this](const Input::Event &)
			{
				viewStack.popAndShow();
			});
		viewNav->setOnPushRightBtn(
			[this](const Input::Event &)
			{
				showEmulation();
			});
		viewNav->showRightBtn(false);
		viewStack.setShowNavViewBackButton(viewAttach.viewManager().needsBackControl());
		appPtr->onCustomizeNavView(*viewNav);
		viewStack.setNavView(std::move(viewNav));
	}
	viewStack.showNavView(app().showsTitleBar());
	emuView.setLayoutInputView(&inputView());
	placeElements();
	auto mainMenu = EmuApp::makeView(viewAttach, EmuApp::ViewID::MAIN_MENU);
	static_cast<EmuMainMenuView*>(mainMenu.get())->setAudioVideo(emuAudio(), videoLayer());
	pushAndShow(std::move(mainMenu));
	applyFrameRates(false);
	videoLayer().emuVideo().setOnFormatChanged(
		[this, &videoLayer = videoLayer()](EmuVideo &)
		{
			videoLayer.onVideoFormatChanged(app().videoEffectPixelFormat());
			if(videoLayer.zoom() > 100)
			{
				placeEmuViews();
			}
		});
}

void EmuViewController::pushAndShow(std::unique_ptr<View> v, const Input::Event &e, bool needsNavView, bool isModal)
{
	showUI(false);
	viewStack.pushAndShow(std::move(v), e, needsNavView, isModal);
}

void EmuViewController::pop()
{
	viewStack.pop();
}

void EmuViewController::popTo(View &v)
{
	viewStack.popTo(v);
}

void EmuViewController::dismissView(View &v, bool refreshLayout)
{
	viewStack.dismissView(v, showingEmulation ? false : refreshLayout);
}

void EmuViewController::dismissView(int idx, bool refreshLayout)
{
	viewStack.dismissView(idx, showingEmulation ? false : refreshLayout);
}

bool EmuViewController::inputEvent(const Input::Event &e)
{
	if(showingEmulation)
	{
		return emuInputView.inputEvent(e);
	}
	return viewStack.inputEvent(e);
}

void EmuViewController::movePopupToWindow(IG::Window &win)
{
	auto &origWin = popup.window();
	if(origWin == win)
		return;
	auto &origWinData = windowData(origWin);
	origWinData.hasPopup = false;
	auto &winData = windowData(win);
	winData.hasPopup = true;
	popup.setWindow(&win);
}

void EmuViewController::moveEmuViewToWindow(IG::Window &win)
{
	auto &origWin = emuView.window();
	if(origWin == win)
		return;
	if(showingEmulation)
	{
		win.setDrawEventPriority(origWin.setDrawEventPriority());
	}
	auto &origWinData = windowData(origWin);
	origWinData.hasEmuView = false;
	auto &winData = windowData(win);
	winData.hasEmuView = true;
	emuView.setWindow(&win);
	emuView.setViewRect(winData.viewport().bounds(), winData.projection.plane());
}

void EmuViewController::configureAppForEmulation(bool running)
{
	appContext().setIdleDisplayPowerSave(running ? app().idleDisplayPowerSave() : true);
	app().applyOSNavStyle(appContext(), running);
	appContext().setHintKeyRepeat(!running);
}

void EmuViewController::configureWindowForEmulation(IG::Window &win, bool running)
{
	if constexpr(Config::SCREEN_FRAME_INTERVAL)
		win.screen()->setFrameInterval(app().frameInterval());
	emuView.renderer().setWindowValidOrientations(win, running ? app().emuOrientation() : app().menuOrientation());
	win.setIntendedFrameRate(running ? system().frameRate() : 0.);
	movePopupToWindow(running ? emuView.window() : emuInputView.window());
}

void EmuViewController::showEmulation()
{
	if(showingEmulation || !app().system().hasContent())
		return;
	viewStack.top().onHide();
	showingEmulation = true;
	configureAppForEmulation(true);
	configureWindowForEmulation(emuView.window(), true);
	if(emuView.window() != emuInputView.window())
		emuInputView.postDraw();
	app().resetInput();
	emuInputView.resetInput();
	startEmulation();
	placeEmuViews();
}

void EmuViewController::showUI(bool updateTopView)
{
	if(!showingEmulation)
		return;
	showingEmulation = false;
	pauseEmulation();
	configureAppForEmulation(false);
	configureWindowForEmulation(emuView.window(), false);
	emuView.postDraw();
	if(updateTopView)
	{
		viewStack.show();
		viewStack.top().postDraw();
	}
}

bool EmuViewController::showAutoStateConfirm(const Input::Event &e)
{
	auto ctx = appContext();
	auto saveStr = system().statePath(-1);
	auto modTimeStr = ctx.fileUriFormatLastWriteTimeLocal(saveStr);
	if(modTimeStr.size())
	{
		pushAndShowModal(std::make_unique<AutoStateConfirmAlertView>(viewStack.top().attachParams(),
			modTimeStr), e, false);
		return true;
	}
	return false;
}

void EmuViewController::placeEmuViews()
{
	emuView.place();
	emuInputView.place();
}

void EmuViewController::placeElements()
{
	//logMsg("placing app elements");
	{
		auto &winData = windowData(popup.window());
		popup.setViewRect(winData.viewport().bounds(), winData.projection.plane());
		popup.place();
	}
	auto &winData = mainWindowData();
	emuView.manager().setTableXIndentToDefault(inputView().window(), winData.projection.plane());
	placeEmuViews();
	viewStack.place(winData.viewport().bounds(), winData.projection.plane());
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

void EmuViewController::setEmuViewOnExtraWindow(bool on, IG::Screen &screen)
{
	auto ctx = appContext();
	if(on && !hasExtraWindow(ctx))
	{
		logMsg("setting emu view on extra window");
		IG::WindowConfig winConf;
		winConf.setScreen(screen);
		winConf.setTitle(ctx.applicationName);
		winConf.setFormat(app().windowDrawableConfig().pixelFormat);
		auto extraWin = ctx.makeWindow(winConf,
			[this](IG::ApplicationContext, IG::Window &win)
			{
				emuView.renderer().attachWindow(win, app().windowDrawableConfig());
				win.makeAppData<WindowData>();
				auto &mainWinData = mainWindowData();
				auto &extraWinData = windowData(win);
				extraWinData.focused = true;
				if(system().isActive())
				{
					emuTask().pause();
					moveOnFrame(mainWindow(), win);
					applyFrameRates();
				}
				extraWinData.projection = updateProjection(app().makeViewport(win));
				moveEmuViewToWindow(win);
				emuView.setLayoutInputView(nullptr);

				win.setOnSurfaceChange(
					[this](IG::Window &win, IG::Window::SurfaceChange change)
					{
						auto &winData = windowData(win);
						rendererTask().updateDrawableForSurfaceChange(win, change);
						if(change.resized())
						{
							logMsg("view resize for extra window");
							winData.projection = updateProjection(app().makeViewport(win));
							emuView.setViewRect(winData.viewport().bounds(), winData.projection.plane());
							emuView.place();
						}
					});

				win.setOnDraw(
					[this](IG::Window &win, IG::Window::DrawParams params)
					{
						auto &winData = windowData(win);
						return rendererTask().draw(win, params, {}, winData.viewport(), winData.projection.matrix(),
							[this, &winData](IG::Window &win, Gfx::RendererCommands &cmds)
							{
								cmds.clear();
								emuView.draw(cmds);
								if(winData.hasPopup)
								{
									popup.draw(cmds);
								}
								cmds.present();
							});
					});

				win.setOnInputEvent(
					[this](IG::Window &win, const Input::Event &e)
					{
						if(system().isActive() && e.keyEvent())
						{
							return emuInputView.inputEvent(e);
						}
						return false;
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
						moveEmuViewToWindow(mainWindow());
						movePopupToWindow(mainWindow());
						emuView.setLayoutInputView(&inputView());
						placeEmuViews();
						mainWindow().postDraw();
						if(system().isActive())
						{
							emuTask().pause();
							moveOnFrame(win, mainWindow());
							applyFrameRates();
						}
					});

				win.show();
				placeEmuViews();
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

void EmuViewController::startViewportAnimation(IG::Window &win)
{
	auto &winData = windowData(win);
	auto oldViewport = winData.viewport();
	auto newViewport = app().makeViewport(win);
	winData.animatedViewport.start(win, oldViewport, newViewport);
	win.postDraw();
}

void EmuViewController::startMainViewportAnimation()
{
	startViewportAnimation(mainWindow());
}

void EmuViewController::updateWindowViewport(IG::Window &win, IG::Window::SurfaceChange change)
{
	auto &winData = windowData(win);
	if(change.surfaceResized())
	{
		winData.animatedViewport.finish();
		winData.projection = updateProjection(app().makeViewport(win));
	}
	else if(change.contentRectResized())
	{
		startViewportAnimation(win);
	}
	else if(change.customViewportResized())
	{
		winData.projection = updateProjection(winData.animatedViewport.viewport());
	}
}

void EmuViewController::updateEmuAudioStats(unsigned underruns, unsigned overruns, unsigned callbacks, double avgCallbackFrames, unsigned frames)
{
	emuView.updateAudioStats(underruns, overruns, callbacks, avgCallbackFrames, frames);
}

void EmuViewController::clearEmuAudioStats()
{
	emuView.clearAudioStats();
}

bool EmuViewController::allWindowsAreFocused() const
{
	return mainWindowData().focused && (!hasExtraWindow(appContext()) || extraWindowIsFocused(appContext()));
}

void EmuViewController::applyFrameRates(bool updateFrameTime)
{
	auto screenFrameTime = emuView.window().screen()->frameTime();
	system().setFrameTime(VideoSystem::NATIVE_NTSC, app().frameTime(VideoSystem::NATIVE_NTSC, screenFrameTime));
	system().setFrameTime(VideoSystem::PAL, app().frameTime(VideoSystem::PAL, screenFrameTime));
	if(updateFrameTime)
		system().configFrameTime(app().soundRate());
}

IG::OnFrameDelegate EmuViewController::makeOnFrameDelayed(uint8_t delay)
{
	return
		[this, delay](IG::FrameParams params)
		{
			if(delay)
			{
				addOnFrameDelegate(makeOnFrameDelayed(delay - 1));
			}
			else
			{
				if(system().isActive())
				{
					emuWindow().setDrawEventPriority(1); // block UI from posting draws
					addOnFrame();
				}
			}
			return false;
		};
}

void EmuViewController::addOnFrameDelegate(IG::OnFrameDelegate onFrame)
{
	emuWindow().addOnFrame(onFrame, winFrameTimeSrc);
}

void EmuViewController::addOnFrameDelayed()
{
	// delay before adding onFrame handler to let timestamps stabilize
	auto delay = emuWindowScreen()->frameRate() / 4;
	//logMsg("delaying onFrame handler by %d frames", onFrameHandlerDelay);
	addOnFrameDelegate(makeOnFrameDelayed(delay));
}

void EmuViewController::addOnFrame()
{
	addOnFrameDelegate(onFrameUpdate);
}

void EmuViewController::removeOnFrame()
{
	emuWindow().removeOnFrame(onFrameUpdate, winFrameTimeSrc);
}

void EmuViewController::moveOnFrame(IG::Window &from, IG::Window &to)
{
	from.removeOnFrame(onFrameUpdate, winFrameTimeSrc);
	to.addOnFrame(onFrameUpdate, winFrameTimeSrc);
}

void EmuViewController::startEmulation()
{
	videoLayer().emuVideo().setOnFrameFinished(
		[this](EmuVideo &)
		{
			addOnFrame();
			emuWindow().drawNow();
		});
	app().setCPUNeedsLowLatency(appContext(), true);
	emuTask().start();
	system().start(*appPtr);
	videoLayer().setBrightness(1.f);
	addOnFrameDelayed();
}

void EmuViewController::pauseEmulation()
{
	app().setCPUNeedsLowLatency(appContext(), false);
	videoLayer().emuVideo().setOnFrameFinished([](EmuVideo &){});
	emuTask().pause();
	system().pause(*appPtr);
	videoLayer().setBrightness(showingEmulation ? .75f : .25f);
	setFastForwardSpeed(0);
	emuWindow().setDrawEventPriority();
	removeOnFrame();
}

void EmuViewController::closeSystem(bool allowAutosaveState)
{
	showUI();
	emuTask().stop();
	system().closeRuntimeSystem(*appPtr, allowAutosaveState);
	viewStack.navView()->showRightBtn(false);
	if(int idx = viewStack.viewIdx("System Actions");
		idx > 0)
	{
		// pop to menu below System Actions
		viewStack.popTo(idx - 1);
	}
}

void EmuViewController::popToSystemActionsMenu()
{
	viewStack.popTo(viewStack.viewIdx("System Actions"));
}

void EmuViewController::postDrawToEmuWindows()
{
	emuView.window().postDraw();
}

IG::Screen *EmuViewController::emuWindowScreen() const
{
	return emuView.window().screen();
}

IG::Window &EmuViewController::emuWindow() const
{
	return emuView.window();
}

WindowData &EmuViewController::emuWindowData()
{
	return windowData(emuView.window());
}

Gfx::RendererTask &EmuViewController::rendererTask() const
{
	return *rendererTask_;
}

void EmuViewController::pushAndShowModal(std::unique_ptr<View> v, const Input::Event &e, bool needsNavView)
{
	pushAndShow(std::move(v), e, needsNavView, true);
}

void EmuViewController::pushAndShowModal(std::unique_ptr<View> v, bool needsNavView)
{
	auto e = v->appContext().defaultInputEvent();
	pushAndShowModal(std::move(v), e, needsNavView);
}

bool EmuViewController::hasModalView() const
{
	return viewStack.hasModalView();
}

void EmuViewController::popModalViews()
{
	viewStack.popModalViews();
}

void EmuViewController::prepareDraw()
{
	popup.prepareDraw();
	emuView.prepareDraw();
	viewStack.prepareDraw();
}

void EmuViewController::drawMainWindow(IG::Window &win, Gfx::RendererCommands &cmds, bool hasEmuView, bool hasPopup)
{
	if(showingEmulation)
	{
		if(hasEmuView)
		{
			emuView.draw(cmds);
		}
		emuInputView.draw(cmds);
		if(hasPopup)
			popup.draw(cmds);
	}
	else
	{
		if(hasEmuView)
		{
			emuView.draw(cmds);
		}
		viewStack.draw(cmds);
		popup.draw(cmds);
	}
	cmds.present();
}

void EmuViewController::popToRoot()
{
	viewStack.popToRoot();
}

void EmuViewController::showNavView(bool show)
{
	viewStack.showNavView(show);
}

void EmuViewController::setShowNavViewBackButton(bool show)
{
	viewStack.setShowNavViewBackButton(show);
}

void EmuViewController::showSystemActionsView(ViewAttachParams attach, const Input::Event &e)
{
	showUI();
	if(!viewStack.contains("System Actions"))
	{
		viewStack.pushAndShow(EmuApp::makeView(attach, EmuApp::ViewID::SYSTEM_ACTIONS), e);
	}
}

void EmuViewController::onInputDevicesChanged()
{
	#ifdef CONFIG_BLUETOOTH
	if(viewStack.size() == 1) // update bluetooth items
		viewStack.top().onShow();
	#endif
}

void EmuViewController::onSystemCreated()
{
	app().prepareAudio();
	viewStack.navView()->showRightBtn(true);
}

EmuInputView &EmuViewController::inputView()
{
	return emuInputView;
}

ToastView &EmuViewController::popupMessageView()
{
	return popup;
}

EmuVideoLayer &EmuViewController::videoLayer() const
{
	return *emuView.videoLayer();
}

EmuAudio &EmuViewController::emuAudio() const
{
	return *emuAudioPtr;
}

void EmuViewController::onScreenChange(IG::ApplicationContext ctx, IG::Screen &screen, IG::ScreenChange change)
{
	if(change.added())
	{
		logMsg("screen added");
		if(app().showOnSecondScreenOption() && ctx.screens().size() > 1)
			setEmuViewOnExtraWindow(true, screen);
	}
	else if(change.removed())
	{
		logMsg("screen removed");
		if(hasExtraWindow(appContext()) && *extraWindowScreen(appContext()) == screen)
			setEmuViewOnExtraWindow(false, screen);
	}
}

void EmuViewController::handleOpenFileCommand(IG::CStringView path)
{
	auto name = appContext().fileUriDisplayName(path);
	if(name.empty())
	{
		appPtr->postErrorMessage(fmt::format("Can't access path name for:\n{}", path));
		return;
	}
	if(!IG::isUri(path) && FS::status(path).type() == FS::file_type::directory)
	{
		logMsg("changing to dir %s from external command", path.data());
		showUI(false);
		popToRoot();
		appPtr->setContentSearchPath(path);
		pushAndShow(
			EmuFilePicker::makeForLoading(viewStack.top().attachParams(), appContext().defaultInputEvent()),
			appContext().defaultInputEvent(),
			false);
		return;
	}
	logMsg("opening file %s from external command", path.data());
	showUI();
	popToRoot();
	onSelectFileFromPicker(*appPtr, {}, path, name, Input::KeyEvent{}, {}, viewStack.top().attachParams());
}

void EmuViewController::onFocusChange(bool in)
{
	if(showingEmulation)
	{
		if(in && system().isPaused())
		{
			logMsg("resuming emulation due to window focus");
			#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
			emuInputView.activeVController()->resetInput();
			#endif
			startEmulation();
		}
		else if(app().pauseUnfocusedOption() && !system().isPaused() && !allWindowsAreFocused())
		{
			logMsg("pausing emulation with all windows unfocused");
			pauseEmulation();
			postDrawToEmuWindows();
		}
	}
}

WindowData &EmuViewController::mainWindowData() const
{
	return windowData(emuInputView.window());
}

IG::Window &EmuViewController::mainWindow() const
{
	return emuInputView.window();
}

void EmuViewController::setFastForwardSpeed(int speed)
{
	bool active = speed > 1;
	targetFastForwardSpeed = speed;
	auto &emuApp = app();
	emuAudio().setAddSoundBuffersOnUnderrun(active ? emuApp.addSoundBuffersOnUnderrun() : false);
	auto soundVolume = (active && !emuApp.soundDuringFastForwardIsEnabled()) ? 0 : emuApp.soundVolume();
	emuAudio().setVolume(soundVolume);
}

void EmuViewController::setWindowFrameClockSource(IG::Window::FrameTimeSource src)
{
	winFrameTimeSrc = src;
}

bool EmuViewController::useRendererTime() const
{
	return winFrameTimeSrc == IG::Window::FrameTimeSource::RENDERER;
}

void EmuViewController::configureSecondaryScreens()
{
	if(app().showOnSecondScreenOption() && appContext().screens().size() > 1)
	{
		setEmuViewOnExtraWindow(true, *appContext().screens()[1]);
	}
}

IG::ApplicationContext EmuViewController::appContext() const
{
	return emuWindow().appContext();
}

bool EmuViewController::isMenuDismissKey(const Input::KeyEvent &e)
{
	using namespace IG::Input;
	Key dismissKey = Keycode::MENU;
	Key dismissKey2 = Keycode::GAME_Y;
	if(Config::MACHINE_IS_PANDORA && e.device()->subtype() == Device::Subtype::PANDORA_HANDHELD)
	{
		if(hasModalView()) // make sure not performing text input
			return false;
		dismissKey = Keycode::SPACE;
	}
	return e.key() == dismissKey || e.key() == dismissKey2;
}

void EmuViewController::writeConfig(IO &io)
{
	if(IG::used(usePresentationTime_) && !usePresentationTime_)
		writeOptionValue(io, CFGKEY_RENDERER_PRESENTATION_TIME, false);
}

}
