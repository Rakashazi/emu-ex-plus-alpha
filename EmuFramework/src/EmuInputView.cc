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
#include "EmuOptions.hh"
#include <imagine/gui/AlertView.hh>
#include <emuframework/FilePicker.hh>
#include "private.hh"
#include "privateInput.hh"

EmuInputView::EmuInputView(ViewAttachParams attach, SysVController &vCtrl)
	: View(attach), vController{vCtrl}
{}

void EmuInputView::draw(Gfx::RendererCommands &cmds)
{
	cmds.loadTransform(projP.makeTranslate());
	vController.draw(cmds, touchControlsOn && EmuSystem::touchControlsApplicable(), ffKeyPushed || ffToggleActive);
}

void EmuInputView::place()
{
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	EmuControls::setupVControllerVars();
	vController.place();
	#endif
}

void EmuInputView::resetInput()
{
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	vController.resetInput();
	#endif
	ffKeyPushed = ffToggleActive = false;
}

void EmuInputView::updateFastforward()
{
	emuSystemTask.setFastForwardActive(ffKeyPushed || ffToggleActive);
}

bool EmuInputView::inputEvent(Input::Event e)
{
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	if(e.isPointer())
	{
		if(e.pushed() && vController.menuHitTest(e.pos()))
		{
			EmuApp::showLastViewFromSystem(attachParams(), e);
			return true;
		}
		else if(e.pushed() && vController.fastForwardHitTest(e.pos()))
		{
			ffToggleActive ^= true;
			updateFastforward();
		}
		else if((touchControlsOn && EmuSystem::touchControlsApplicable())
			|| vController.isInKeyboardMode())
		{
			vController.applyInput(e);
			EmuSystem::handlePointerInputEvent(e, emuVideoLayer.gameRect());
		}
		else if(EmuSystem::handlePointerInputEvent(e, emuVideoLayer.gameRect()))
		{
			//logMsg("game consumed pointer input event");
		}
		#ifdef CONFIG_VCONTROLS_GAMEPAD
		else if(!touchControlsOn && (uint)optionTouchCtrl == 2 && optionTouchCtrlShowOnTouch
			&& !vController.isInKeyboardMode()
			&& e.isTouch() && e.pushed()
			)
		{
			logMsg("turning on on-screen controls from touch input");
			touchControlsOn = true;
			emuViewController.placeEmuViews();
		}
		#endif
		return true;
	}
	#else
	if(e.isPointer())
	{
		if(e.state == Input::PUSHED)
		{
			EmuApp::showMenuViewFromSystem(attachParams(), e);
		}
		return true;
	}
	#endif
	if(e.isRelativePointer())
	{
		processRelPtr(e);
		return true;
	}
	else
	{
		#ifdef CONFIG_VCONTROLS_GAMEPAD
		if(vController.keyInput(e))
			return true;
		#endif
		if(unlikely(!keyMapping))
			return false;
		assumeExpr(e.device());
		const KeyMapping::ActionGroup &actionMap = keyMapping.inputDevActionTablePtr[e.device()->idx][e.mapKey()];
		//logMsg("player %d input %s", player, Input::buttonName(e.map, e.button));
		bool didAction = false;
		iterateTimes(KeyMapping::maxKeyActions, i)
		{
			auto action = actionMap[i];
			if(action != 0)
			{
				using namespace EmuControls;
				didAction = true;
				action--;

				switch(action)
				{
					bcase guiKeyIdxFastForward:
					{
						ffKeyPushed = e.pushed();
						updateFastforward();
						logMsg("fast-forward key state: %d", ffKeyPushed);
					}

					bcase guiKeyIdxLoadGame:
					if(e.pushed())
					{
						logMsg("show load game view from key event");
						emuViewController.popToRoot();
						pushAndShow(EmuFilePicker::makeForLoading(attachParams(), e), e, false);
						return true;
					}

					bcase guiKeyIdxMenu:
					if(e.pushed())
					{
						logMsg("show system actions view from key event");
						EmuApp::showSystemActionsViewFromSystem(attachParams(), e);
						return true;
					}

					bcase guiKeyIdxSaveState:
					if(e.pushed())
					{
						static auto doSaveState =
							[]()
							{
								if(auto err = EmuApp::saveStateWithSlot(EmuSystem::saveStateSlot);
									err)
								{
									EmuApp::printfMessage(4, true, "Save State: %s", err->what());
								}
								else
									EmuApp::postMessage("State Saved");
							};

						if(EmuSystem::shouldOverwriteExistingState())
						{
							EmuApp::syncEmulationThread();
							doSaveState();
						}
						else
						{
							auto ynAlertView = makeView<YesNoAlertView>("Really Overwrite State?");
							ynAlertView->setOnYes(
								[](TextMenuItem &, View &view, Input::Event e)
								{
									view.dismiss();
									doSaveState();
									emuViewController.showEmulation();
								});
							ynAlertView->setOnNo(
								[](TextMenuItem &, View &view, Input::Event e)
								{
									view.dismiss();
									emuViewController.showEmulation();
								});
							emuViewController.pushAndShowModal(std::move(ynAlertView), e, false);
						}
						return true;
					}

					bcase guiKeyIdxLoadState:
					if(e.pushed())
					{
						EmuApp::syncEmulationThread();
						if(auto err = EmuApp::loadStateWithSlot(EmuSystem::saveStateSlot);
							err)
						{
							EmuApp::printfMessage(4, true, "Load State: %s", err->what());
						}
						return true;
					}

					bcase guiKeyIdxDecStateSlot:
					if(e.pushed())
					{
						EmuSystem::saveStateSlot--;
						if(EmuSystem::saveStateSlot < -1)
							EmuSystem::saveStateSlot = 9;
						EmuApp::printfMessage(1, false, "State Slot: %s", stateNameStr(EmuSystem::saveStateSlot));
					}

					bcase guiKeyIdxIncStateSlot:
					if(e.pushed())
					{
						auto prevSlot = EmuSystem::saveStateSlot;
						EmuSystem::saveStateSlot++;
						if(EmuSystem::saveStateSlot > 9)
							EmuSystem::saveStateSlot = -1;
						EmuApp::printfMessage(1, false, "State Slot: %s", stateNameStr(EmuSystem::saveStateSlot));
					}

					bcase guiKeyIdxGameScreenshot:
					if(e.pushed())
					{
						emuVideo.takeGameScreenshot();
						return true;
					}

					bcase guiKeyIdxExit:
					if(e.pushed())
					{
						logMsg("show last view from key event");
						EmuApp::showLastViewFromSystem(attachParams(), e);
						return true;
					}

					bdefault:
					{
						//logMsg("action %d, %d", emuKey, state);
						bool turbo;
						uint sysAction = EmuSystem::translateInputAction(action, turbo);
						//logMsg("action %d -> %d, pushed %d", action, sysAction, e.state == Input::PUSHED);
						if(turbo)
						{
							if(e.pushed())
							{
								turboActions.addEvent(sysAction);
							}
							else
							{
								turboActions.removeEvent(sysAction);
							}
						}
						EmuSystem::handleInputAction(e.state(), sysAction);
					}
				}
			}
			else
				break;
		}
		return didAction;
	}
}

void EmuInputView::setTouchControlsOn(bool on)
{
	touchControlsOn = on;
}

bool EmuInputView::touchControlsAreOn() const
{
	return touchControlsOn;
}
