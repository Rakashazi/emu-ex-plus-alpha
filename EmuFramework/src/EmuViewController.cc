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
#include "EmuTiming.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Screen.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/gui/ToastView.hh>

class AutoStateConfirmAlertView : public YesNoAlertView, public EmuAppHelper<AutoStateConfirmAlertView>
{
public:
	AutoStateConfirmAlertView(ViewAttachParams attach, const char *dateStr, bool addToRecent):
		YesNoAlertView
		{
			attach,
			"",
			"Continue",
			"Restart Game",
			[this, addToRecent]()
			{
				launchSystem(app(), true, addToRecent);
			},
			[this, addToRecent]()
			{
				launchSystem(app(), false, addToRecent);
			}
		}
	{
		setLabel(string_makePrintf<96>("Auto-save state exists from:\n%s", dateStr).data());
	}
};

EmuViewController::EmuViewController() {}

EmuViewController::EmuViewController(ViewAttachParams viewAttach,
	VController &vCtrl, EmuVideoLayer &videoLayer, EmuSystemTask &systemTask,
	EmuAudio &emuAudio):
	emuView{viewAttach, &videoLayer},
	emuInputView{viewAttach, vCtrl, videoLayer},
	popup{viewAttach},
	rendererTask_{&viewAttach.rendererTask()},
	systemTask{&systemTask},
	emuAudioPtr{&emuAudio},
	appPtr{&EmuApp::get(viewAttach.appContext())},
	onExit
	{
		[this](Base::ApplicationContext ctx, bool backgrounded)
		{
			if(backgrounded)
			{
				showUI();
				if(optionShowOnSecondScreen && ctx.screens().size() > 1)
				{
					setEmuViewOnExtraWindow(false, *ctx.screens()[1]);
				}
				viewStack.top().onHide();
				ctx.addOnResume(
					[this](Base::ApplicationContext, bool focused)
					{
						configureSecondaryScreens();
						prepareDraw();
						if(showingEmulation && focused && EmuSystem::isPaused())
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
	if constexpr(Config::envIsAndroid)
	{
		emuInputView.setConsumeUnboundGamepadKeys(optionConsumeUnboundGamepadKeys);
	}
	auto &win = viewAttach.window();

	win.setOnInputEvent(
		[this](Base::Window &win, Input::Event e)
		{
			return inputEvent(e);
		});

	win.setOnFocusChange(
		[this](Base::Window &win, bool in)
		{
			windowData(win).focused = in;
			onFocusChange(in);
		});

	win.setOnDragDrop(
		[this](Base::Window &win, const char *filename)
		{
			logMsg("got DnD: %s", filename);
			handleOpenFileCommand(filename);
		});

	win.setOnSurfaceChange(
		[this](Base::Window &win, Base::Window::SurfaceChange change)
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
		[this](Base::Window &win, Base::Window::DrawParams params)
		{
			auto &winData = windowData(win);
			return rendererTask().draw(win, params, {}, winData.viewport(), winData.projection.matrix(),
				[this](Base::Window &win, Gfx::RendererCommands &cmds)
				{
					auto &winData = windowData(win);
					cmds.clear();
					drawMainWindow(win, cmds, winData.hasEmuView, winData.hasPopup);
				});
		});

	initViews(viewAttach);
	configureSecondaryScreens();
}

static bool shouldExitFromViewRootWithoutPrompt(Input::Event e)
{
	return e.map() == Input::Map::SYSTEM && (Config::envIsAndroid || Config::envIsLinux);
}

EmuMenuViewStack::EmuMenuViewStack(EmuViewController &emuViewController):
	ViewStack{}, emuViewControllerPtr{&emuViewController}
{}

bool EmuMenuViewStack::inputEvent(Input::Event e)
{
	if(ViewStack::inputEvent(e))
	{
		return true;
	}
	if(e.pushed() && e.isDefaultCancelButton())
	{
		if(size() == 1)
		{
			//logMsg("cancel button at view stack root");
			if(e.repeated())
			{
				return true;
			}
			if(EmuSystem::gameIsRunning() ||
					(!EmuSystem::gameIsRunning() && !shouldExitFromViewRootWithoutPrompt(e)))
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
	if(e.pushed() && viewController().isMenuDismissKey(e) && !hasModalView())
	{
		if(EmuSystem::gameIsRunning())
		{
			viewController().showEmulation();
		}
		return true;
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
		setWindowFrameClockSource(Base::Window::FrameTimeSource::RENDERER);
	}
	else
	{
		setWindowFrameClockSource(Base::Window::FrameTimeSource::SCREEN);
	}
	logMsg("timestamp source:%s", useRendererTime() ? "renderer" : "screen");
	onFrameUpdate = [this, &r = std::as_const(viewAttach.renderer())](IG::FrameParams params)
		{
			bool skipForward = false;
			bool fastForwarding = false;
			auto &audio = emuAudio();
			if(EmuSystem::shouldFastForward()) [[unlikely]]
			{
				// for skipping loading on disk-based computers
				fastForwarding = true;
				skipForward = true;
				EmuSystem::setSpeedMultiplier(audio, 8);
			}
			else if(targetFastForwardSpeed > 1) [[unlikely]]
			{
				fastForwarding = true;
				EmuSystem::setSpeedMultiplier(audio, targetFastForwardSpeed);
			}
			else
			{
				EmuSystem::setSpeedMultiplier(audio, 1);
			}
			auto frameInfo = EmuSystem::advanceFramesWithTime(params.timestamp());
			if(!frameInfo.advanced)
			{
				return true;
			}
			if(!optionSkipLateFrames && !fastForwarding)
			{
				frameInfo.advanced = currentFrameInterval();
			}
			constexpr unsigned maxFrameSkip = 8;
			uint32_t framesToEmulate = std::min(frameInfo.advanced, maxFrameSkip);
			EmuAudio *audioPtr = audio ? &audio : nullptr;
			systemTask->runFrame(&videoLayer().emuVideo(), audioPtr, framesToEmulate, skipForward);
			r.setPresentationTime(emuWindow(), params.presentTime());
			/*logMsg("frame present time:%.4f next display frame:%.4f",
				std::chrono::duration_cast<IG::FloatSeconds>(frameInfo.presentTime).count(),
				std::chrono::duration_cast<IG::FloatSeconds>(params.presentTime()).count());*/
			return false;
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
		viewNav->rotateLeftBtn = true;
		viewNav->setOnPushLeftBtn(
			[this](Input::Event)
			{
				viewStack.popAndShow();
			});
		viewNav->setOnPushRightBtn(
			[this](Input::Event)
			{
				if(EmuSystem::gameIsRunning())
				{
					showEmulation();
				}
			});
		viewNav->showRightBtn(false);
		viewStack.setShowNavViewBackButton(viewAttach.viewManager().needsBackControl());
		appPtr->onCustomizeNavView(*viewNav);
		viewStack.setNavView(std::move(viewNav));
	}
	viewStack.showNavView(optionTitleBar);
	emuView.setLayoutInputView(&inputView());
	placeElements();
	auto mainMenu = EmuApp::makeView(viewAttach, EmuApp::ViewID::MAIN_MENU);
	static_cast<EmuMainMenuView*>(mainMenu.get())->setAudioVideo(emuAudio(), videoLayer());
	pushAndShow(std::move(mainMenu));
	applyFrameRates(false);
	videoLayer().emuVideo().setOnFormatChanged(
		[this, &videoLayer = videoLayer()](EmuVideo &)
		{
			#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
			videoLayer.setEffect(optionImgEffect, optionImageEffectPixelFormatValue());
			#else
			videoLayer.resetImage();
			#endif
			videoLayer.setOverlay(optionOverlayEffect);
			if((unsigned)optionImageZoom > 100)
			{
				placeEmuViews();
			}
		});
	setPhysicalControlsPresent(viewAttach.appContext().keyInputIsPresent());
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	if((int)optionTouchCtrl == 2)
		updateAutoOnScreenControlVisible();
	else
		setOnScreenControls(optionTouchCtrl);
	#endif
}

void EmuViewController::pushAndShow(std::unique_ptr<View> v, Input::Event e, bool needsNavView, bool isModal)
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

bool EmuViewController::inputEvent(Input::Event e)
{
	if(e.isPointer())
	{
		//logMsg("Pointer %s @ %d,%d", e.actionToStr(e.state), e.x, e.y);
	}
	else
	{
		//logMsg("%s %s from %s", e.device->keyName(e.button), e.actionToStr(e.state), e.device->name());
	}
	if(showingEmulation)
	{
		return emuInputView.inputEvent(e);
	}
	return viewStack.inputEvent(e);
}

void EmuViewController::movePopupToWindow(Base::Window &win)
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

void EmuViewController::moveEmuViewToWindow(Base::Window &win)
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
	appContext().setIdleDisplayPowerSave(running ? (bool)optionIdleDisplayPowerSave : true);
	app().applyOSNavStyle(appContext(), running);
	appContext().setHintKeyRepeat(!running);
}

void EmuViewController::configureWindowForEmulation(Base::Window &win, bool running)
{
	#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
	win.screen()->setFrameInterval(optionFrameInterval);
	#endif
	emuView.renderer().setWindowValidOrientations(win, running ? optionGameOrientation : optionMenuOrientation);
	win.setIntendedFrameRate(running ? EmuSystem::frameRate() : 0.);
	movePopupToWindow(running ? emuView.window() : emuInputView.window());
}

void EmuViewController::showEmulation()
{
	if(showingEmulation)
		return;
	viewStack.top().onHide();
	showingEmulation = true;
	configureAppForEmulation(true);
	configureWindowForEmulation(emuView.window(), true);
	if(emuView.window() != emuInputView.window())
		emuInputView.postDraw();
	app().resetInput();
	popup.clear();
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

bool EmuViewController::showAutoStateConfirm(Input::Event e, bool addToRecent)
{
	if(!(optionConfirmAutoLoadState && optionAutoSaveState))
	{
		return false;
	}
	auto saveStr = EmuSystem::sprintStateFilename(-1);
	if(FS::exists(saveStr))
	{
		auto mTime = FS::status(saveStr).lastWriteTimeLocal();
		char dateStr[64]{};
		std::strftime(dateStr, sizeof(dateStr), strftimeFormat, &mTime);
		pushAndShowModal(std::make_unique<AutoStateConfirmAlertView>(viewStack.top().attachParams(), dateStr, addToRecent), e, false);
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

static bool hasExtraWindow(Base::ApplicationContext ctx)
{
	return ctx.windows().size() == 2;
}

static void dismissExtraWindow(Base::ApplicationContext ctx)
{
	if(!hasExtraWindow(ctx))
		return;
	ctx.windows()[1]->dismiss();
}

static bool extraWindowIsFocused(Base::ApplicationContext ctx)
{
	if(!hasExtraWindow(ctx))
		return false;
	return windowData(*ctx.windows()[1]).focused;
}

static Base::Screen *extraWindowScreen(Base::ApplicationContext ctx)
{
	if(!hasExtraWindow(ctx))
		return nullptr;
	return ctx.windows()[1]->screen();
}

void EmuViewController::setEmuViewOnExtraWindow(bool on, Base::Screen &screen)
{
	auto app = appContext();
	if(on && !hasExtraWindow(app))
	{
		logMsg("setting emu view on extra window");
		Base::WindowConfig winConf;
		winConf.setScreen(screen);
		winConf.setTitle(app.applicationName);
		auto extraWin = app.makeWindow(winConf,
			[this](Base::ApplicationContext, Base::Window &win)
			{
				emuView.renderer().attachWindow(win);
				win.makeAppData<WindowData>();
				auto &mainWinData = mainWindowData();
				auto &extraWinData = windowData(win);
				extraWinData.focused = true;
				if(EmuSystem::isActive())
				{
					systemTask->pause();
					moveOnFrame(mainWindow(), win);
					applyFrameRates();
				}
				extraWinData.projection = updateProjection(makeViewport(win));
				moveEmuViewToWindow(win);
				emuView.setLayoutInputView(nullptr);

				win.setOnSurfaceChange(
					[this](Base::Window &win, Base::Window::SurfaceChange change)
					{
						auto &winData = windowData(win);
						rendererTask().updateDrawableForSurfaceChange(win, change);
						if(change.resized())
						{
							logMsg("view resize for extra window");
							winData.projection = updateProjection(makeViewport(win));
							emuView.setViewRect(winData.viewport().bounds(), winData.projection.plane());
							emuView.place();
						}
					});

				win.setOnDraw(
					[this](Base::Window &win, Base::Window::DrawParams params)
					{
						auto &winData = windowData(win);
						return rendererTask().draw(win, params, {}, winData.viewport(), winData.projection.matrix(),
							[this, &winData](Base::Window &win, Gfx::RendererCommands &cmds)
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
					[this](Base::Window &win, Input::Event e)
					{
						if(EmuSystem::isActive() && e.isKey())
						{
							return emuInputView.inputEvent(e);
						}
						return false;
					});

				win.setOnFocusChange(
					[this](Base::Window &win, bool in)
					{
						windowData(win).focused = in;
						onFocusChange(in);
					});

				win.setOnDismissRequest(
					[](Base::Window &win)
					{
						win.dismiss();
					});

				win.setOnDismiss(
					[this](Base::Window &win)
					{
						EmuSystem::resetFrameTime();
						logMsg("setting emu view on main window");
						moveEmuViewToWindow(mainWindow());
						movePopupToWindow(mainWindow());
						emuView.setLayoutInputView(&inputView());
						placeEmuViews();
						mainWindow().postDraw();
						if(EmuSystem::isActive())
						{
							systemTask->pause();
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
	else if(!on && hasExtraWindow(app))
	{
		dismissExtraWindow(app);
	}
}

void EmuViewController::startViewportAnimation(Base::Window &win)
{
	auto &winData = windowData(win);
	auto oldViewport = winData.viewport();
	auto newViewport = makeViewport(win);
	winData.animatedViewport.start(win, oldViewport, newViewport);
	win.postDraw();
}

void EmuViewController::startMainViewportAnimation()
{
	startViewportAnimation(mainWindow());
}

void EmuViewController::updateWindowViewport(Base::Window &win, Base::Window::SurfaceChange change)
{
	auto &winData = windowData(win);
	if(change.surfaceResized())
	{
		winData.animatedViewport.finish();
		winData.projection = updateProjection(makeViewport(win));
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
	EmuSystem::setFrameTime(EmuSystem::VIDSYS_NATIVE_NTSC,
		optionFrameRate.val ? IG::FloatSeconds(optionFrameRate.val) : emuView.window().screen()->frameTime());
	EmuSystem::setFrameTime(EmuSystem::VIDSYS_PAL,
		optionFrameRatePAL.val ? IG::FloatSeconds(optionFrameRatePAL.val) : emuView.window().screen()->frameTime());
	if(updateFrameTime)
		EmuSystem::configFrameTime(optionSoundRate);
}

Base::OnFrameDelegate EmuViewController::makeOnFrameDelayed(uint8_t delay)
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
				if(EmuSystem::isActive())
				{
					emuWindow().setDrawEventPriority(1); // block UI from posting draws
					addOnFrame();
				}
			}
			return false;
		};
}

void EmuViewController::addOnFrameDelegate(Base::OnFrameDelegate onFrame)
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

void EmuViewController::moveOnFrame(Base::Window &from, Base::Window &to)
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
	systemTask->start();
	EmuSystem::start(*appPtr);
	videoLayer().setBrightness(1.f);
	addOnFrameDelayed();
}

void EmuViewController::pauseEmulation()
{
	app().setCPUNeedsLowLatency(appContext(), false);
	videoLayer().emuVideo().setOnFrameFinished([](EmuVideo &){});
	systemTask->pause();
	EmuSystem::pause(*appPtr);
	videoLayer().setBrightness(showingEmulation ? .75f : .25f);
	setFastForwardActive(false);
	emuWindow().setDrawEventPriority();
	removeOnFrame();
}

void EmuViewController::closeSystem(bool allowAutosaveState)
{
	showUI();
	systemTask->stop();
	EmuSystem::closeRuntimeSystem(*appPtr, allowAutosaveState);
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

Base::Screen *EmuViewController::emuWindowScreen() const
{
	return emuView.window().screen();
}

Base::Window &EmuViewController::emuWindow() const
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

void EmuViewController::pushAndShowModal(std::unique_ptr<View> v, Input::Event e, bool needsNavView)
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

void EmuViewController::drawMainWindow(Base::Window &win, Gfx::RendererCommands &cmds, bool hasEmuView, bool hasPopup)
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

void EmuViewController::showSystemActionsView(ViewAttachParams attach, Input::Event e)
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
	EmuSystem::prepareAudio(emuAudio());
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

void EmuViewController::onScreenChange(Base::ApplicationContext ctx, Base::Screen &screen, Base::ScreenChange change)
{
	if(change.added())
	{
		logMsg("screen added");
		if(optionShowOnSecondScreen && ctx.screens().size() > 1)
			setEmuViewOnExtraWindow(true, screen);
	}
	else if(change.removed())
	{
		logMsg("screen removed");
		if(hasExtraWindow(appContext()) && *extraWindowScreen(appContext()) == screen)
			setEmuViewOnExtraWindow(false, screen);
	}
}

void EmuViewController::handleOpenFileCommand(const char *path)
{
	auto type = FS::status(path).type();
	if(type == FS::file_type::directory)
	{
		logMsg("changing to dir %s from external command", path);
		showUI(false);
		popToRoot();
		appPtr->setMediaSearchPath(FS::makePathString(path));
		pushAndShow(
			EmuFilePicker::makeForLoading(viewStack.top().attachParams(), appContext().defaultInputEvent()),
			appContext().defaultInputEvent(),
			false);
		return;
	}
	if(type != FS::file_type::regular || (!appPtr->hasArchiveExtension(path) && !EmuSystem::defaultFsFilter(path)))
	{
		logMsg("unrecognized file type");
		return;
	}
	logMsg("opening file %s from external command", path);
	showUI();
	popToRoot();
	onSelectFileFromPicker(*appPtr, path, Input::Event{}, {}, viewStack.top().attachParams());
}

void EmuViewController::onFocusChange(bool in)
{
	if(showingEmulation)
	{
		if(in && EmuSystem::isPaused())
		{
			logMsg("resuming emulation due to window focus");
			#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
			emuInputView.activeVController()->resetInput();
			#endif
			startEmulation();
		}
		else if(optionPauseUnfocused && !EmuSystem::isPaused() && !allWindowsAreFocused())
		{
			logMsg("pausing emulation with all windows unfocused");
			pauseEmulation();
			postDrawToEmuWindows();
		}
	}
}

void EmuViewController::setOnScreenControls(bool on)
{
	emuInputView.setTouchControlsOn(on);
	placeEmuViews();
}

void EmuViewController::updateAutoOnScreenControlVisible()
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	if((unsigned)optionTouchCtrl == 2)
	{
		if(emuInputView.touchControlsAreOn() && physicalControlsPresent)
		{
			logMsg("auto-turning off on-screen controls");
			setOnScreenControls(0);
		}
		else if(!emuInputView.touchControlsAreOn() && !physicalControlsPresent)
		{
			logMsg("auto-turning on on-screen controls");
			setOnScreenControls(1);
		}
	}
	#endif
}

void EmuViewController::setPhysicalControlsPresent(bool present)
{
	if(present != physicalControlsPresent)
	{
		logMsg("Physical controls present:%s", present ? "y" : "n");
	}
	physicalControlsPresent = present;
}

WindowData &EmuViewController::mainWindowData() const
{
	return windowData(emuInputView.window());
}

Base::Window &EmuViewController::mainWindow() const
{
	return emuInputView.window();
}

void EmuViewController::setFastForwardActive(bool active)
{
	targetFastForwardSpeed = active ? optionFastForwardSpeed.val : 0;
	emuAudio().setAddSoundBuffersOnUnderrun(active ? optionAddSoundBuffersOnUnderrun.val : false);
	auto soundVolume = (active && !soundDuringFastForwardIsEnabled()) ? 0 : optionSoundVolume.val;
	emuAudio().setVolume(soundVolume);
}

void EmuViewController::setWindowFrameClockSource(Base::Window::FrameTimeSource src)
{
	winFrameTimeSrc = src;
}

bool EmuViewController::useRendererTime() const
{
	return winFrameTimeSrc == Base::Window::FrameTimeSource::RENDERER;
}

void EmuViewController::configureSecondaryScreens()
{
	if(optionShowOnSecondScreen && appContext().screens().size() > 1)
	{
		setEmuViewOnExtraWindow(true, *appContext().screens()[1]);
	}
}

Base::ApplicationContext EmuViewController::appContext() const
{
	return emuWindow().appContext();
}

bool EmuViewController::isMenuDismissKey(Input::Event e)
{
	using namespace Input;
	Key dismissKey = Keycode::MENU;
	Key dismissKey2 = Keycode::GAME_Y;
	if(Config::MACHINE_IS_PANDORA && e.device()->subtype() == Device::SUBTYPE_PANDORA_HANDHELD)
	{
		if(hasModalView()) // make sure not performing text input
			return false;
		dismissKey = Keycode::SPACE;
	}
	return e.key() == dismissKey || e.key() == dismissKey2;
}
