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

#include <emuframework/EmuInputView.hh>
#include <emuframework/VController.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuVideoLayer.hh>
#include <emuframework/FilePicker.hh>
#include "../EmuOptions.hh"
#include "../privateInput.hh"
#include <imagine/gui/AlertView.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/util/variant.hh>
#include <format>

namespace EmuEx
{

EmuInputView::EmuInputView() {}

EmuInputView::EmuInputView(ViewAttachParams attach, VController &vCtrl, EmuVideoLayer &videoLayer):
	View(attach),
	vController{&vCtrl},
	videoLayer{&videoLayer} {}

void EmuInputView::draw(Gfx::RendererCommands &__restrict__ cmds)
{
	vController->draw(cmds, speedToggleActive);
}

void EmuInputView::place()
{
	vController->place();
}

void EmuInputView::resetInput()
{
	vController->resetInput();
	speedToggleActive = false;
}

bool EmuInputView::toggleAltSpeedMode(AltSpeedMode mode)
{
	return setAltSpeedMode(mode, speedToggleActive ^ true);
}

bool EmuInputView::setAltSpeedMode(AltSpeedMode mode, bool on)
{
	logMsg("alt speed state:%d", on);
	speedToggleActive = on;
	vController->updateAltSpeedModeInput(mode, on);
	updateRunSpeed(mode);
	return speedToggleActive;
}

void EmuInputView::updateRunSpeed(AltSpeedMode mode)
{
	app().setRunSpeed(speedToggleActive ? app().altSpeedAsDouble(mode) : 1.);
}

bool EmuInputView::inputEvent(const Input::Event &e)
{
	return visit(overloaded
	{
		[&](const Input::MotionEvent &motionEv)
		{
			if(!motionEv.isAbsolute())
				return false;
			return vController->pointerInputEvent(motionEv, videoLayer->contentRect());
		},
		[&](const Input::KeyEvent &keyEv)
		{
			if(vController->keyInput(keyEv))
				return true;
			auto &emuApp = app();
			auto &sys = emuApp.system();
			auto &devData = inputDevData(*keyEv.device());
			const auto &actionTable = devData.actionTable;
			if(!actionTable.size()) [[unlikely]]
				return false;
			assumeExpr(keyEv.device());
			const auto &actionGroup = actionTable[keyEv.mapKey()];
			bool isPushed = keyEv.pushed();
			bool isRepeated = keyEv.repeated();
			bool didAction = false;
			static constexpr bool printKeyEvent = false;
			if(printKeyEvent && !isRepeated)
			{
				app().postMessage(std::format("{} key: {} from device: {}",
					isPushed ? "pushed" : "released", keyEv.device()->keyName(keyEv.key()),
					keyEv.device()->name()));
			}
			for(auto action : actionGroup)
			{
				if(!action)
					break;
				didAction = true;
				if(isRepeated) // only consume the event
					break;
				action--; // action values are offset by 1 due to the null action value
				if(emuApp.handleKeyInput({action, keyEv.state(), keyEv.metaKeyBits()}, e))
					break;
			}
			return didAction
				|| keyEv.isGamepad() // consume all gamepad events
				|| devData.devConf.shouldConsumeUnboundKeys();
		}
	}, e);
}

void EmuInputView::setSystemGestureExclusion(bool on)
{
	if(on)
	{
		auto rectsSize = vController->deviceElements().size();
		WRect rects[rectsSize];
		for(int i = 0; const auto &e : vController->deviceElements())
		{
			rects[i++] = e.realBounds();
		}
		window().setSystemGestureExclusionRects({rects, rectsSize});
	}
	else
		window().setSystemGestureExclusionRects({});
}

int EmuInputView::uiElementHeight() const
{
	if(!vController)
		return 0;
	return View::navBarHeight(vController->face());
}

}