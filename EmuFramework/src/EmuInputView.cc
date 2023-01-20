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
#include "EmuOptions.hh"
#include "privateInput.hh"
#include <imagine/gui/AlertView.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/util/format.hh>
#include <imagine/util/variant.hh>

namespace EmuEx
{

EmuInputView::EmuInputView() {}

EmuInputView::EmuInputView(ViewAttachParams attach, VController &vCtrl, EmuVideoLayer &videoLayer):
	View(attach),
	vController{&vCtrl},
	videoLayer{&videoLayer} {}

void EmuInputView::draw(Gfx::RendererCommands &__restrict__ cmds)
{
	vController->draw(cmds, ffToggleActive);
}

void EmuInputView::place()
{
	vController->place();
}

void EmuInputView::resetInput()
{
	vController->resetInput();
	ffToggleActive = false;
}

bool EmuInputView::toggleFastSlowMode()
{
	return setFastSlowMode(ffToggleActive ^ true);
}

bool EmuInputView::setFastSlowMode(bool on)
{
	ffToggleActive = on;
	logMsg("fast-forward state:%d", ffToggleActive);
	updateRunSpeed();
	return ffToggleActive;
}

void EmuInputView::updateRunSpeed()
{
	app().setRunSpeed(ffToggleActive ? app().fastSlowModeSpeedAsDouble() : 1.);
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
				app().postMessage(fmt::format("{} key: {} from device: {}",
					isPushed ? "pushed" : "released", keyEv.device()->keyName(keyEv.key()),
					keyEv.device()->name()));
			}
			if(!isRepeated)
			{
				for(auto action : actionGroup)
				{
					if(!action)
						break;
					using namespace Controls;
					didAction = true;
					action--; // action values are offset by 1 due to the null action value
					if(emuApp.handleKeyInput({action, keyEv.state(), keyEv.metaKeyBits()}, e))
						break;
				}
			}
			return didAction
				|| keyEv.isGamepad() // consume all gamepad events
				|| devData.devConf.shouldConsumeUnboundKeys();
		}
	}, e);
}

}
