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

#include <emuframework/EmuViewController.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuView.hh>
#include <emuframework/EmuVideoLayer.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/MainMenuView.hh>
#include <emuframework/EmuOptions.hh>
#include "../WindowData.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Screen.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/gui/ToastView.hh>
#include <imagine/fs/FS.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"EmuViewController"};

EmuViewController::EmuViewController(ViewAttachParams viewAttach,
	VController &vCtrl, EmuVideoLayer &videoLayer, EmuSystem &sys):
	emuView{viewAttach, &videoLayer, sys},
	inputView{viewAttach, vCtrl, videoLayer},
	popup{viewAttach},
	viewStack{viewAttach, app()}
{
	inputView.setController(this);
	auto &face = viewAttach.viewManager.defaultFace;
	{
		auto viewNav = std::make_unique<BasicNavView>
		(
			viewAttach,
			&face,
			app().asset(AssetID::arrow),
			app().asset(AssetID::display)
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
				app().showEmulation();
			});
		viewNav->showRightBtn(false);
		viewStack.setShowNavViewBackButton(viewAttach.viewManager.needsBackControl);
		app().onCustomizeNavView(*viewNav);
		viewStack.setNavView(std::move(viewNav));
	}
	viewStack.showNavView(app().showsTitleBar);
	emuView.setLayoutInputView(&inputView);
}

static bool shouldExitFromViewRootWithoutPrompt(const Input::KeyEvent &e)
{
	return e.map() == Input::Map::SYSTEM && (Config::envIsAndroid || Config::envIsLinux);
}

EmuMenuViewStack::EmuMenuViewStack(ViewAttachParams attach, EmuApp &app):
	ViewStack(attach), emuAppPtr{&app} {}

bool EmuMenuViewStack::inputEvent(const Input::Event& e)
{
	if(ViewStack::inputEvent(e))
	{
		return true;
	}
	if(e.keyEvent())
	{
		auto &keyEv = *e.keyEvent();
		bool hasEmuContent = app().system().hasContent();
		if(keyEv.pushed(Input::DefaultKey::CANCEL))
		{
			if(size() == 1)
			{
				//log.info("cancel button at view stack root");
				if(keyEv.repeated())
				{
					return true;
				}
				if(hasEmuContent || (!hasEmuContent && !shouldExitFromViewRootWithoutPrompt(keyEv)))
				{
					app().showExitAlert(top().attachParams(), e);
				}
				else
				{
					app().appContext().exit();
				}
			}
			else
			{
				popAndShow();
			}
			return true;
		}
		if(keyEv.pushed() && app().viewController().isMenuDismissKey(keyEv) && !hasModalView())
		{
			app().showEmulation();
			return true;
		}
	}
	return false;
}

void EmuViewController::pushAndShow(std::unique_ptr<View> v, const Input::Event &e, bool needsNavView, bool isModal)
{
	app().showUI(false);
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

bool EmuViewController::inputEvent(const Input::Event& e)
{
	if(showingEmulation)
	{
		return inputView.inputEvent(e);
	}
	return viewStack.inputEvent(e);
}

bool EmuViewController::extraWindowInputEvent(const Input::Event &e)
{
	if(showingEmulation && e.keyEvent())
	{
		return inputView.inputEvent(e);
	}
	return false;
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
	winData.applyViewRect(emuView);
	if(win == appContext().mainWindow())
		emuView.setLayoutInputView(&inputView);
	else
		emuView.setLayoutInputView(nullptr);
}

void EmuViewController::configureWindowForEmulation(IG::Window &win, FrameTimeConfig frameTimeConfig, bool running)
{
	emuView.renderer().setWindowValidOrientations(win, running ? app().emuOrientation.value() : app().menuOrientation.value());
	emuView.renderer().task().setPresentMode(win, running ? Gfx::PresentMode(app().effectivePresentMode()) : Gfx::PresentMode::Auto);
	if(running)
		app().setIntendedFrameRate(win, frameTimeConfig);
	else
		win.setIntendedFrameRate(0);
	movePopupToWindow(running ? emuView.window() : inputView.window());
}

void EmuViewController::showEmulationView(FrameTimeConfig frameTimeConfig)
{
	if(showingEmulation)
		return;
	viewStack.top().onHide();
	showingEmulation = true;
	emuView.window().configureFrameTimeSource(app().frameTimeSource);
	configureWindowForEmulation(emuView.window(), frameTimeConfig, true);
	if(emuView.window() != inputView.window())
		inputView.postDraw();
	inputView.resetInput();
	placeEmuViews();
	inputView.setSystemGestureExclusion(true);
}

void EmuViewController::showMenuView(bool updateTopView)
{
	if(!showingEmulation)
		return;
	showingEmulation = false;
	emuView.window().configureFrameTimeSource(FrameTimeSource::Unset);
	presentTime = {};
	inputView.setSystemGestureExclusion(false);
	configureWindowForEmulation(emuView.window(), {}, false);
	emuView.postDraw();
	if(updateTopView)
	{
		viewStack.show();
		viewStack.top().postDraw();
	}
}

void EmuViewController::placeEmuViews()
{
	emuView.place();
	inputView.place();
}

void EmuViewController::placeElements()
{
	//log.info("placing app elements");
	{
		auto &winData = windowData(popup.window());
		winData.applyViewRect(popup);
		popup.place();
	}
	auto &winData = app().mainWindowData();
	emuView.manager().setTableXIndentToDefault(appContext().mainWindow());
	placeEmuViews();
	WRect contentBounds = winData.contentBounds();
	WRect windowBounds = winData.windowBounds();
	if(app().menuScale != 100)
	{
		float scaler = app().menuScale / 100.f;
		contentBounds *= scaler;
		contentBounds.setPos(winData.contentBounds().pos(C2DO), C2DO);
		windowBounds = contentBounds;
	}
	viewStack.place(contentBounds, windowBounds);
}

void EmuViewController::updateMainWindowViewport(IG::Window &win, IG::Viewport viewport, Gfx::RendererTask &task)
{
	auto &winData = windowData(win);
	task.setDefaultViewport(win, viewport);
	winData.updateWindowViewport(win, viewport, task.renderer());
	if(winData.hasEmuView)
	{
		winData.applyViewRect(emuView);
	}
	winData.applyViewRect(inputView);
	placeElements();
}

void EmuViewController::updateExtraWindowViewport(IG::Window &win, IG::Viewport viewport, Gfx::RendererTask &task)
{
	log.info("view resize for extra window");
	task.setDefaultViewport(win, viewport);
	auto &winData = windowData(win);
	winData.updateWindowViewport(win, viewport, task.renderer());
	winData.applyViewRect(emuView);
	emuView.place();
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

static Gfx::DrawAsyncMode drawAsyncMode(bool showingEmulation)
{
	return showingEmulation ? Gfx::DrawAsyncMode::FULL : Gfx::DrawAsyncMode::AUTO;
}

bool EmuViewController::drawMainWindow(IG::Window &win, IG::WindowDrawParams params, Gfx::RendererTask &task)
{
	return task.draw(win, params, {.asyncMode = drawAsyncMode(showingEmulation)},
		[this, isBlankFrame = std::exchange(drawBlankFrame, {})](IG::Window &win, Gfx::RendererCommands &cmds)
	{
		auto &winData = windowData(win);
		cmds.basicEffect().setModelViewProjection(cmds, Gfx::Mat4::ident(), winData.projM);
		if(showingEmulation)
		{
			if(winData.hasEmuView && !isBlankFrame)
			{
				emuView.draw(cmds);
			}
			inputView.draw(cmds);
			if(app().showFrameTimeStats)
				emuView.drawframeTimeStatsText(cmds);
			if(winData.hasPopup)
				popup.draw(cmds);
			cmds.present(presentTime);
			app().notifyWindowPresented();
		}
		else
		{
			if(winData.hasEmuView)
			{
				emuView.draw(cmds);
			}
			viewStack.draw(cmds);
			popup.draw(cmds);
			cmds.present();
		}
		cmds.clear();
	});
}

bool EmuViewController::drawExtraWindow(IG::Window &win, IG::WindowDrawParams params, Gfx::RendererTask &task)
{
	return task.draw(win, params, {.asyncMode = drawAsyncMode(showingEmulation)},
		[this](IG::Window &win, Gfx::RendererCommands &cmds)
	{
		auto &winData = windowData(win);
		cmds.basicEffect().setModelViewProjection(cmds, Gfx::Mat4::ident(), winData.projM);
		emuView.draw(cmds);
		if(winData.hasPopup)
		{
			popup.draw(cmds);
		}
		cmds.present(presentTime);
		app().notifyWindowPresented();
		cmds.clear();
	});
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
	app().showUI();
	if(!viewStack.contains("System Actions"))
	{
		viewStack.pushAndShow(app().makeView(attach, EmuApp::ViewID::SYSTEM_ACTIONS), e);
	}
}

void EmuViewController::onInputDevicesChanged()
{
	if(viewStack.size() == 1) // update bluetooth items
		viewStack.top().onShow();
}

void EmuViewController::onSystemCreated()
{
	viewStack.navView()->showRightBtn(true);
}

void EmuViewController::onSystemClosed()
{
	viewStack.navView()->showRightBtn(false);
	if(int idx = viewStack.viewIdx("System Actions");
		idx > 0)
	{
		// pop to menu below System Actions
		viewStack.popTo(idx - 1);
	}
}

MainMenuView &EmuViewController::mainMenu()
{
	return static_cast<MainMenuView&>(viewStack.viewAtIdx(0));
}

EmuVideoLayer &EmuViewController::videoLayer() const
{
	return *emuView.videoLayer();
}

IG::ApplicationContext EmuViewController::appContext() const
{
	return emuWindow().appContext();
}

bool EmuViewController::isMenuDismissKey(const Input::KeyEvent &e) const
{
	using namespace IG::Input;
	std::array dismissKeys{Keycode::MENU, Keycode::GAME_Y, Keycode::GAME_MODE};
	if(Config::MACHINE_IS_PANDORA && e.device()->subtype() == Device::Subtype::PANDORA_HANDHELD)
	{
		if(hasModalView()) // make sure not performing text input
			return false;
		dismissKeys[0] = Keycode::SPACE;
	}
	return std::ranges::any_of(dismissKeys, [&](auto &k){return e.key() == k;});
}

void EmuViewController::onHide()
{
	viewStack.top().onHide();
}

}
