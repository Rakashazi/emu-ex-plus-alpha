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
	turboModifierActive = false;
}

bool EmuInputView::toggleFastSlowMode()
{
	ffToggleActive ^= true;
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
			for(auto action : actionGroup)
			{
				if(!action)
					break;
				using namespace Controls;
				didAction = true;
				action--; // action values are offset by 1 due to the null action value
				switch(action)
				{
					case guiKeyIdxFastForward:
					{
						if(isRepeated)
							break;
						ffToggleActive = keyEv.pushed();
						updateRunSpeed();
						logMsg("fast-forward state:%d", ffToggleActive);
						break;
					}
					case guiKeyIdxLoadGame:
					{
						if(!isPushed)
							break;
						logMsg("show load game view from key event");
						emuApp.viewController().popToRoot();
						pushAndShow(EmuFilePicker::makeForLoading(attachParams(), e), e, false);
						return true;
					}
					case guiKeyIdxMenu:
					{
						if(!isPushed)
							break;
						logMsg("show system actions view from key event");
						emuApp.showSystemActionsViewFromSystem(attachParams(), e);
						return true;
					}
					case guiKeyIdxSaveState:
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
						if(emuApp.shouldOverwriteExistingState())
						{
							emuApp.syncEmulationThread();
							doSaveState(emuApp, emuApp.confirmOverwriteStateOption());
						}
						else
						{
							auto ynAlertView = makeView<YesNoAlertView>("Really Overwrite State?");
							ynAlertView->setOnYes(
								[this]()
								{
									doSaveState(app(), false);
									app().showEmulation();
								});
							ynAlertView->setOnNo(
								[this]()
								{
									app().showEmulation();
								});
							pushAndShowModal(std::move(ynAlertView), e);
						}
						return true;
					}
					case guiKeyIdxLoadState:
					{
						if(!isPushed)
							break;
						emuApp.syncEmulationThread();
						emuApp.loadStateWithSlot(sys.stateSlot());
						return true;
					}
					case guiKeyIdxDecStateSlot:
					{
						if(!isPushed)
							break;
						sys.decStateSlot();
						emuApp.postMessage(1, false, fmt::format("State Slot: {}", sys.stateSlotName()));
						return true;
					}
					case guiKeyIdxIncStateSlot:
					{
						if(!isPushed)
							break;
						sys.incStateSlot();
						emuApp.postMessage(1, false, fmt::format("State Slot: {}", sys.stateSlotName()));
						return true;
					}
					case guiKeyIdxGameScreenshot:
					{
						if(!isPushed)
							break;
						videoLayer->emuVideo().takeGameScreenshot();
						return true;
					}
					case guiKeyIdxToggleFastForward:
					{
						if(!isPushed || keyEv.repeated())
							break;
						toggleFastSlowMode();
						logMsg("fast-forward state:%d", ffToggleActive);
						break;
					}
					case guiKeyIdxLastView:
					{
						if(!isPushed)
							break;
						logMsg("show last view from key event");
						emuApp.showLastViewFromSystem(attachParams(), e);
						return true;
					}
					case guiKeyIdxTurboModifier:
					{
						if(isRepeated)
							break;
						turboModifierActive = isPushed;
						if(!isPushed)
							emuApp.removeTurboInputEvents();
						break;
					}
					case guiKeyIdxExitApp:
					{
						if(!isPushed)
							break;
						auto ynAlertView = makeView<YesNoAlertView>("Really Exit?");
						ynAlertView->setOnYes([this]() { appContext().exit(); });
						emuApp.viewController().pushAndShowModal(std::move(ynAlertView), e, false);
						break;
					}
					default:
					{
						if(isRepeated)
							break;
						InputActionFlagsMask flags{};
						if(turboModifierActive)
							flags |= InputActionFlagsMask::turbo;
						emuApp.handleSystemKeyInput({action, keyEv.state(), keyEv.metaKeyBits(), flags});
					}
				}
			}
			return didAction
				|| keyEv.isGamepad() // consume all gamepad events
				|| devData.devConf.shouldConsumeUnboundKeys();
		}
	}, e);
}

}
