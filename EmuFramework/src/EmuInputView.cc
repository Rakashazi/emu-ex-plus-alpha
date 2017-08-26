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
#include <emuframework/EmuInput.hh>
#include <emuframework/VController.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuOptions.hh>
#include <imagine/gui/AlertView.hh>
#include <emuframework/FilePicker.hh>
#include "private.hh"

extern bool touchControlsAreOn;

void EmuInputView::draw()
{
	vController.draw(touchControlsAreOn && EmuSystem::touchControlsApplicable(), ffKeyPushed || ffToggleActive);
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
	fastForwardActive = ffKeyPushed || ffToggleActive;
}

void EmuInputView::inputEvent(Input::Event e)
{
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	if(e.isPointer())
	{
		auto &layoutPos = vControllerLayoutPos[mainWin.viewport().isPortrait() ? 1 : 0];
		if(e.state == Input::PUSHED && layoutPos[VCTRL_LAYOUT_MENU_IDX].state != 0 && vController.menuBound.overlaps({e.x, e.y}))
		{
			viewStack.top().clearSelection();
			EmuApp::restoreMenuFromGame();
			return;
		}
		else if(e.state == Input::PUSHED && layoutPos[VCTRL_LAYOUT_FF_IDX].state != 0 && vController.ffBound.overlaps({e.x, e.y}))
		{
			ffToggleActive ^= true;
			updateFastforward();
		}
		else if((touchControlsAreOn && EmuSystem::touchControlsApplicable())
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
		else if(!touchControlsAreOn && (uint)optionTouchCtrl == 2 && optionTouchCtrlShowOnTouch
			&& !vController.isInKeyboardMode()
			&& e.isTouch() && e.state == Input::PUSHED
			)
		{
			logMsg("turning on on-screen controls from touch input");
			touchControlsAreOn = 1;
			placeEmuViews();
		}
		#endif
		return;
	}
	#else
	if(e.isPointer())
	{
		if(e.state == Input::PUSHED)
		{
			viewStack.top()->clearSelection();
			restoreMenuFromGame();
		}
		return;
	}
	#endif
	if(e.isRelativePointer())
	{
		processRelPtr(e);
	}
	else
	{
		#if defined CONFIG_ENV_WEBOS && CONFIG_ENV_WEBOS_OS <= 2
		if(e.state == Input::PUSHED && e.button == Input::Keycode::ESCAPE)
		{
			restoreMenuFromGame();
			return;
		}
		#endif
		assert(e.device);
		const KeyMapping::ActionGroup &actionMap = keyMapping.inputDevActionTablePtr[e.device->idx][e.button];
		//logMsg("player %d input %s", player, Input::buttonName(e.map, e.button));
		iterateTimes(KeyMapping::maxKeyActions, i)
		{
			auto action = actionMap[i];
			if(action != 0)
			{
				using namespace EmuControls;
				action--;

				switch(action)
				{
					bcase guiKeyIdxFastForward:
					{
						ffKeyPushed = e.state == Input::PUSHED;
						updateFastforward();
						logMsg("fast-forward key state: %d", ffKeyPushed);
					}

					bcase guiKeyIdxLoadGame:
					if(e.state == Input::PUSHED)
					{
						logMsg("open load game menu from key event");
						EmuApp::restoreMenuFromGame();
						viewStack.popToRoot();
						auto &fPicker = *EmuFilePicker::makeForLoading(attachParams());
						viewStack.pushAndShow(fPicker, e, false);
						return;
					}

					bcase guiKeyIdxMenu:
					if(e.state == Input::PUSHED)
					{
						logMsg("open menu from key event");
						EmuApp::restoreMenuFromGame();
						return;
					}

					bcase guiKeyIdxSaveState:
					if(e.state == Input::PUSHED)
					{
						static auto doSaveState =
							[]()
							{
								if(auto err = EmuApp::saveStateWithSlot(EmuSystem::saveStateSlot);
									err)
								{
									popup.printf(4, true, "Save State: %s", err->what());
								}
								else
									popup.post("State Saved");
							};

						if(EmuSystem::shouldOverwriteExistingState())
						{
							doSaveState();
						}
						else
						{
							auto &ynAlertView = *new YesNoAlertView{attachParams(), "Really Overwrite State?"};
							ynAlertView.setOnYes(
								[](TextMenuItem &, View &view, Input::Event e)
								{
									view.dismiss();
									doSaveState();
									startGameFromMenu();
								});
							ynAlertView.setOnNo(
								[](TextMenuItem &, View &view, Input::Event e)
								{
									view.dismiss();
									startGameFromMenu();
								});
							modalViewController.pushAndShow(ynAlertView, e);
							EmuApp::restoreMenuFromGame();
						}
						return;
					}

					bcase guiKeyIdxLoadState:
					if(e.state == Input::PUSHED)
					{
						if(auto err = EmuApp::loadStateWithSlot(EmuSystem::saveStateSlot);
							err)
						{
							popup.printf(4, true, "Load State: %s", err->what());
						}
						return;
					}

					bcase guiKeyIdxDecStateSlot:
					if(e.state == Input::PUSHED)
					{
						EmuSystem::saveStateSlot--;
						if(EmuSystem::saveStateSlot < -1)
							EmuSystem::saveStateSlot = 9;
						popup.printf(1, 0, "State Slot: %s", stateNameStr(EmuSystem::saveStateSlot));
					}

					bcase guiKeyIdxIncStateSlot:
					if(e.state == Input::PUSHED)
					{
						auto prevSlot = EmuSystem::saveStateSlot;
						EmuSystem::saveStateSlot++;
						if(EmuSystem::saveStateSlot > 9)
							EmuSystem::saveStateSlot = -1;
						popup.printf(1, 0, "State Slot: %s", stateNameStr(EmuSystem::saveStateSlot));
					}

					bcase guiKeyIdxGameScreenshot:
					if(e.state == Input::PUSHED)
					{
						emuVideo.takeGameScreenshot();
						return;
					}

					bcase guiKeyIdxExit:
					if(e.state == Input::PUSHED)
					{
						logMsg("request exit from key event");
						auto &ynAlertView = *new YesNoAlertView{attachParams(), "Really Exit?"};
						ynAlertView.setOnYes(
							[](TextMenuItem &, View &view, Input::Event e)
							{
								Base::exit();
							});
						modalViewController.pushAndShow(ynAlertView, e);
						EmuApp::restoreMenuFromGame();
						return;
					}

					bdefault:
					{
						//logMsg("action %d, %d", emuKey, state);
						bool turbo;
						uint sysAction = EmuSystem::translateInputAction(action, turbo);
						//logMsg("action %d -> %d, pushed %d", action, sysAction, e.state == Input::PUSHED);
						if(turbo)
						{
							if(e.state == Input::PUSHED)
							{
								turboActions.addEvent(sysAction);
							}
							else
							{
								turboActions.removeEvent(sysAction);
							}
						}
						EmuSystem::handleInputAction(e.state, sysAction);
					}
				}
			}
			else
				break;
		}
	}
}
