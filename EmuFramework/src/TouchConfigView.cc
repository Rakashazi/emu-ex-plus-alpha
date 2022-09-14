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

#include <emuframework/TouchConfigView.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/base/Timer.hh>
#include <imagine/input/DragTracker.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/util/Interpolator.hh>
#include <imagine/util/format.hh>
#include <imagine/util/variant.hh>
#include <imagine/logger/logger.h>
#include <utility>

namespace EmuEx
{

constexpr bool CAN_TURN_OFF_MENU_BTN = !Config::envIsIOS;

constexpr const char *ctrlStateStr[]
{
	"Off", "On", "Hidden"
};

constexpr const char *touchCtrlExtraBtnSizeMenuName[4]
{
	"None", "Gap only", "10%", "25%"
};

constexpr int touchCtrlExtraBtnSizeMenuVal[4]
{
	0, 1, 200, 500
};

static auto &layoutPosArr(VController &vController, const IG::Window &win)
{
	return vController.layoutPosition()[win.isPortrait() ? 1 : 0];
}

class OnScreenInputPlaceView final: public View, public EmuAppHelper<OnScreenInputPlaceView>
{
public:
	OnScreenInputPlaceView(ViewAttachParams attach, VController &vController);
	~OnScreenInputPlaceView() final;
	void place() final;
	bool inputEvent(const Input::Event &e) final;
	void draw(Gfx::RendererCommands &__restrict__ cmds) final;

private:
	struct DragData
	{
		int elem{-1};
		IG::WP startPos{};
	};
	Gfx::Text text{};
	VController *vControllerPtr;
	IG::InterpolatorValue<float, IG::FrameTime, IG::InterpolatorType::LINEAR> textFade{};
	IG::Timer animationStartTimer{"OnScreenInputPlaceView::animationStartTimer"};
	IG::OnFrameDelegate animate{};
	IG::WindowRect exitBtnRect{};
	Input::DragTracker<DragData> dragTracker{};

	VController &vController() { return *vControllerPtr; }
};

OnScreenInputPlaceView::OnScreenInputPlaceView(ViewAttachParams attach, VController &vController):
	View(attach),
	text{"Click center to go back", &defaultFace()},
	vControllerPtr{&vController},
	animate
	{
		[this](IG::FrameParams params)
		{
			window().setNeedsDraw(true);
			//logMsg("updating fade");
			return textFade.update(params.timestamp());
		}
	}
{
	app().applyOSNavStyle(appContext(), true);
	textFade = {1.};
	animationStartTimer.runIn(IG::Seconds{2}, {},
		[this]()
		{
			logMsg("starting fade");
			textFade = {1., 0., {}, IG::steadyClockTimestamp(), IG::Milliseconds{400}};
			window().addOnFrame(animate);
		});
}

OnScreenInputPlaceView::~OnScreenInputPlaceView()
{
	app().applyOSNavStyle(appContext(), false);
	window().removeOnFrame(animate);
}

void OnScreenInputPlaceView::place()
{
	dragTracker.reset();
	auto exitBtnPos = viewRect().pos(C2DO);
	int exitBtnSize = window().widthMMInPixels(10.);
	exitBtnRect = IG::makeWindowRectRel(exitBtnPos - IG::WP{exitBtnSize/2, exitBtnSize/2}, {exitBtnSize, exitBtnSize});
	text.compile(renderer(), projP);
}

bool OnScreenInputPlaceView::inputEvent(const Input::Event &e)
{
	return visit(overloaded
	{
		[&](const Input::KeyEvent &e)
		{
			if(e.pushed())
			{
				dismiss();
				return true;
			}
			return false;
		},
		[&](const Input::MotionEvent &e)
		{
			if(e.pushed() && animationStartTimer.isArmed())
			{
				animationStartTimer.dispatchEarly();
			}
			dragTracker.inputEvent(e,
				[&](Input::DragTrackerState, DragData &d)
				{
					if(d.elem == -1)
					{
						for(auto i : iotaCount(vController().numElements()))
						{
							if(vController().state(i) == VControllerState::OFF || !vController().bounds(i).contains(e.pos()))
								continue;
							for(const auto &state : dragTracker.stateList())
							{
								if(state.data.elem == (int)i)
									continue; // element already grabbed
							}
							d.elem = i;
							d.startPos = vController().bounds(d.elem).pos(C2DO);
							break;
						}
					}
				},
				[&](Input::DragTrackerState state, Input::DragTrackerState, DragData &d)
				{
					if(d.elem >= 0)
					{
						auto newPos = d.startPos + state.downPosDiff();
						waitForDrawFinished();
						vController().setPos(d.elem, newPos);
						auto layoutPos = vController().pixelToLayoutPos(vController().bounds(d.elem).pos(C2DO), vController().bounds(d.elem).size(), viewRect());
						//logMsg("set pos %d,%d from %d,%d", layoutPos.pos.x, layoutPos.pos.y, layoutPos.origin.xScaler(), layoutPos.origin.yScaler());
						auto &vCtrlLayoutPos = vController().layoutPosition()[window().isPortrait() ? 1 : 0];
						vCtrlLayoutPos[d.elem].origin = layoutPos.origin;
						vCtrlLayoutPos[d.elem].pos = layoutPos.pos;
						vController().setLayoutPositionChanged();
						app().viewController().placeEmuViews();
						postDraw();
					}
				},
				[&](Input::DragTrackerState state, DragData &d)
				{
					if(d.elem == -1 && exitBtnRect.overlaps(state.pos()) && exitBtnRect.overlaps(state.downPos()))
					{
						dismiss();
					}
				});
			return true;
		}
	}, e);
}

void OnScreenInputPlaceView::draw(Gfx::RendererCommands &__restrict__ cmds)
{
	using namespace IG::Gfx;
	vController().draw(cmds, false, true, .75);
	cmds.setColor(.5, .5, .5);
	auto &basicEffect = cmds.basicEffect();
	basicEffect.disableTexture(cmds);
	float lineSize = projP.unprojectYSize(1);
	GeomRect::draw(cmds, Gfx::GCRect{{-projP.wHalf(), -lineSize/2.f},
		{projP.wHalf(), lineSize/2.f}});
	lineSize = projP.unprojectYSize(1);
	GeomRect::draw(cmds, Gfx::GCRect{{-lineSize/2.f, -projP.hHalf()},
		{lineSize/2.f, projP.hHalf()}});

	if(textFade != 0.)
	{
		cmds.setColor(0., 0., 0., textFade/2.);
		GeomRect::draw(cmds, Gfx::GCRect::makeRel({-text.width()/2.f - text.spaceWidth(), -text.height()/2.f - text.spaceWidth()},
			{text.width() + text.spaceWidth()*2.f, text.height() + text.spaceWidth()*2.f}));
		cmds.setColor(1., 1., 1., textFade);
		basicEffect.enableAlphaTexture(cmds);
		text.draw(cmds, projP.unProjectRect(viewRect()).pos(C2DO), C2DO, projP);
	}
}

TextMenuItem::SelectDelegate TouchConfigView::setVisibilityDel(VControllerVisibility val)
{
	return [this, val](){ vController().setGamepadControlsVisibility(val); };
}

TextMenuItem::SelectDelegate TouchConfigView::setPointerInputPlayerDel(int val)
{
	return [this, val](){ vController().setInputPlayer(val); };
}

TextMenuItem::SelectDelegate TouchConfigView::setSizeDel()
{
	return [this](TextMenuItem &item){ vController().setButtonSize(item.id()); };
}

TextMenuItem::SelectDelegate TouchConfigView::setDeadzoneDel()
{
	return [this](TextMenuItem &item){ vController().setDpadDeadzone(item.id()); };
}

TextMenuItem::SelectDelegate TouchConfigView::setDiagonalSensitivityDel()
{
	return [this](TextMenuItem &item){ vController().setDpadDiagonalSensitivity(item.id()); };
}

TextMenuItem::SelectDelegate TouchConfigView::setButtonSpaceDel()
{
	return [this](TextMenuItem &item){ vController().setButtonSpacing(item.id()); };
}

TextMenuItem::SelectDelegate TouchConfigView::setButtonExtraXSizeDel()
{
	return [this](TextMenuItem &item){ vController().setButtonXPadding(item.id()); };
}

TextMenuItem::SelectDelegate TouchConfigView::setButtonExtraYSizeDel()
{
	return [this](TextMenuItem &item){ vController().setButtonYPadding(item.id()); };
}

TextMenuItem::SelectDelegate TouchConfigView::setButtonStaggerDel(int val)
{
	return [this, val]{ vController().setButtonStagger(val); };
}

TextMenuItem::SelectDelegate TouchConfigView::setButtonStateDel(VControllerState state, uint8_t btnIdx)
{
	return [this, state, btnIdx]
	{
		vController().layoutPosition()[window().isPortrait() ? 1 : 0][btnIdx].state = state;
		vController().setLayoutPositionChanged();
		vController().place();
	};
}

TextMenuItem::SelectDelegate TouchConfigView::setAlphaDel()
{
	return [this](TextMenuItem &item){ vController().setButtonAlpha(item.id()); };
}

void TouchConfigView::draw(Gfx::RendererCommands &__restrict__ cmds)
{
	vController().draw(cmds, false, true, .75);
	TableView::draw(cmds);
}

void TouchConfigView::place()
{
	refreshTouchConfigMenu();
	TableView::place();
}

void TouchConfigView::refreshTouchConfigMenu()
{
	auto &layoutPos = layoutPosArr(vController(), window());
	alpha.setSelected((MenuItem::Id)vController().buttonAlpha(), *this);
	ffState.setSelected((int)layoutPos[4].state, *this);
	menuState.setSelected((int)layoutPos[3].state - (CAN_TURN_OFF_MENU_BTN ? 0 : 1), *this);
	touchCtrl.setSelected((int)vController().gamepadControlsVisibility(), *this);
	if(EmuSystem::maxPlayers > 1)
		pointerInput.setSelected((int)vController().inputPlayer(), *this);
	size.setSelected((MenuItem::Id)vController().buttonSize(), *this);
	dPadState.setSelected((int)layoutPos[0].state, *this);
	faceBtnState.setSelected((int)layoutPos[2].state, *this);
	centerBtnState.setSelected((int)layoutPos[1].state, *this);
	if(vController().hasTriggers())
	{
		triggerPos.setBoolValue(vController().triggersInline());
	}
	deadzone.setSelected((MenuItem::Id)vController().dpadDeadzone(), *this);
	diagonalSensitivity.setSelected((MenuItem::Id)vController().dpadDiagonalSensitivity(), *this);
	btnSpace.setSelected((MenuItem::Id)vController().buttonSpacing(), *this);
	btnExtraXSize.setSelected((MenuItem::Id)vController().buttonXPadding(), *this);
	btnExtraYSize.setSelected((MenuItem::Id)vController().buttonYPadding(), *this);
	btnStagger.setSelected(vController().buttonStagger(), *this);
	boundingBoxes.setBoolValue(vController().boundingAreaVisible(), *this);
	if(app().vibrationManager().hasVibrator())
	{
		vibrate.setBoolValue(vController().vibrateOnTouchInput(), *this);
	}
	showOnTouch.setBoolValue(vController().showOnTouchInput(), *this);
}

TouchConfigView::TouchConfigView(ViewAttachParams attach, VController &vCtrl,
	UTF16String faceBtnName, UTF16String centerBtnName):
	TableView{"On-screen Input Setup", attach, item},
	vControllerPtr{&vCtrl},
	touchCtrlItem
	{
		{"Off",  &defaultFace(), setVisibilityDel(VControllerVisibility::OFF)},
		{"On",   &defaultFace(), setVisibilityDel(VControllerVisibility::ON)},
		{"Auto", &defaultFace(), setVisibilityDel(VControllerVisibility::AUTO)}
	},
	touchCtrl
	{
		"Use Virtual Gamepad", &defaultFace(),
		(int)vCtrl.gamepadControlsVisibility(),
		touchCtrlItem
	},
	pointerInputItem
	{
		{"1", &defaultFace(), setPointerInputPlayerDel(0)},
		{"2", &defaultFace(), setPointerInputPlayerDel(1)},
		{"3", &defaultFace(), setPointerInputPlayerDel(2)},
		{"4", &defaultFace(), setPointerInputPlayerDel(3)},
		{"5", &defaultFace(), setPointerInputPlayerDel(4)},
	},
	pointerInput
	{
		"Virtual Gamepad Player", &defaultFace(),
		(int)vCtrl.inputPlayer(),
		[this](const MultiChoiceMenuItem &)
		{
			return EmuSystem::maxPlayers;
		},
		[this](const MultiChoiceMenuItem &, size_t idx) -> TextMenuItem&
		{
			return pointerInputItem[idx];
		}
	},
	sizeItem
	{
		{"6.5", &defaultFace(), setSizeDel(), 650},
		{"7",   &defaultFace(), setSizeDel(), 700},
		{"7.5", &defaultFace(), setSizeDel(), 750},
		{"8",   &defaultFace(), setSizeDel(), 800},
		{"8.5", &defaultFace(), setSizeDel(), 850},
		{"9",   &defaultFace(), setSizeDel(), 900},
		{"10",  &defaultFace(), setSizeDel(), 1000},
		{"12",  &defaultFace(), setSizeDel(), 1200},
		{"14",  &defaultFace(), setSizeDel(), 1400},
		{"15",  &defaultFace(), setSizeDel(), 1500},
		{"Custom Value", &defaultFace(),
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueInputView<double>(attachParams(), e, "Input 3.0 to 15.0", "",
					[this](EmuApp &app, auto val)
					{
						int scaledIntVal = val * 100.0;
						if(vController().setButtonSize(scaledIntVal))
						{
							size.setSelected((MenuItem::Id)scaledIntVal, *this);
							dismissPrevious();
							return true;
						}
						else
						{
							app.postErrorMessage("Value not in range");
							return false;
						}
					});
				return false;
			}, MenuItem::DEFAULT_ID
		},
	},
	size
	{
		"Button Size", &defaultFace(),
		[this](auto idx, Gfx::Text &t)
		{
			t.resetString(fmt::format("{:.2f}", vController().buttonSize() / 100.));
			return true;
		},
		(MenuItem::Id)vController().buttonSize(),
		sizeItem
	},
	deadzoneItem
	{
		{"1",    &defaultFace(), setDeadzoneDel(), 100},
		{"1.35", &defaultFace(), setDeadzoneDel(), 135},
		{"1.6",  &defaultFace(), setDeadzoneDel(), 160},
	},
	deadzone
	{
		"Deadzone", &defaultFace(),
		(MenuItem::Id)vController().dpadDeadzone(),
		deadzoneItem
	},
	diagonalSensitivityItem
	{
		{"None",  &defaultFace(), setDiagonalSensitivityDel(), 1000},
		{"Low",   &defaultFace(), setDiagonalSensitivityDel(), 1500},
		{"M-Low", &defaultFace(), setDiagonalSensitivityDel(), 1750},
		{"Med.",  &defaultFace(), setDiagonalSensitivityDel(), 2000},
		{"High",  &defaultFace(), setDiagonalSensitivityDel(), 2500},
	},
	diagonalSensitivity
	{
		"Diagonal Sensitivity", &defaultFace(),
		(MenuItem::Id)vController().dpadDiagonalSensitivity(),
		diagonalSensitivityItem
	},
	btnSpaceItem
	{
		{"1", &defaultFace(), setButtonSpaceDel(), 100},
		{"2", &defaultFace(), setButtonSpaceDel(), 200},
		{"3", &defaultFace(), setButtonSpaceDel(), 300},
		{"4", &defaultFace(), setButtonSpaceDel(), 400},
	},
	btnSpace
	{
		"Spacing", &defaultFace(),
		(MenuItem::Id)vController().buttonSpacing(),
		btnSpaceItem
	},
	btnExtraXSizeItem
	{
		{touchCtrlExtraBtnSizeMenuName[0], &defaultFace(), setButtonExtraXSizeDel(), touchCtrlExtraBtnSizeMenuVal[0]},
		{touchCtrlExtraBtnSizeMenuName[1], &defaultFace(), setButtonExtraXSizeDel(), touchCtrlExtraBtnSizeMenuVal[1]},
		{touchCtrlExtraBtnSizeMenuName[2], &defaultFace(), setButtonExtraXSizeDel(), touchCtrlExtraBtnSizeMenuVal[2]},
		{touchCtrlExtraBtnSizeMenuName[3], &defaultFace(), setButtonExtraXSizeDel(), touchCtrlExtraBtnSizeMenuVal[3]},
	},
	btnExtraXSize
	{
		"H Overlap", &defaultFace(),
		(MenuItem::Id)vController().buttonXPadding(),
		btnExtraXSizeItem
	},
	btnExtraYSizeItem
	{
		{touchCtrlExtraBtnSizeMenuName[0], &defaultFace(), setButtonExtraYSizeDel(), touchCtrlExtraBtnSizeMenuVal[0]},
		{touchCtrlExtraBtnSizeMenuName[1], &defaultFace(), setButtonExtraYSizeDel(), touchCtrlExtraBtnSizeMenuVal[1]},
		{touchCtrlExtraBtnSizeMenuName[2], &defaultFace(), setButtonExtraYSizeDel(), touchCtrlExtraBtnSizeMenuVal[2]},
		{touchCtrlExtraBtnSizeMenuName[3], &defaultFace(), setButtonExtraYSizeDel(), touchCtrlExtraBtnSizeMenuVal[3]},
	},
	btnExtraYSize
	{
		"V Overlap", &defaultFace(),
		(MenuItem::Id)vController().buttonYPadding(),
		btnExtraYSizeItem
	},
	triggerPos
	{
		"Inline L/R", &defaultFace(),
		vController().triggersInline(),
		[this](BoolMenuItem &item)
		{
			vController().setTriggersInline(item.flipBoolValue(*this));
		}
	},
	btnStaggerItem
	{
		{"-0.75x V", &defaultFace(), setButtonStaggerDel(0)},
		{"-0.5x V",  &defaultFace(), setButtonStaggerDel(1)},
		{"0",        &defaultFace(), setButtonStaggerDel(2)},
		{"0.5x V",   &defaultFace(), setButtonStaggerDel(3)},
		{"0.75x V",  &defaultFace(), setButtonStaggerDel(4)},
		{"1x H&V",   &defaultFace(), setButtonStaggerDel(5)},
	},
	btnStagger
	{
		"Stagger", &defaultFace(),
		vController().buttonStagger(),
		btnStaggerItem
	},
	dPadStateItem
	{
		{ctrlStateStr[0], &defaultFace(), setButtonStateDel(VControllerState::OFF, 0)},
		{ctrlStateStr[1], &defaultFace(), setButtonStateDel(VControllerState::SHOWN, 0)},
		{ctrlStateStr[2], &defaultFace(), setButtonStateDel(VControllerState::HIDDEN, 0)},
	},
	dPadState
	{
		"D-Pad", &defaultFace(),
		(int)layoutPosArr(vCtrl, window())[0].state,
		dPadStateItem
	},
	faceBtnStateItem
	{
		{ctrlStateStr[0], &defaultFace(), setButtonStateDel(VControllerState::OFF, 2)},
		{ctrlStateStr[1], &defaultFace(), setButtonStateDel(VControllerState::SHOWN, 2)},
		{ctrlStateStr[2], &defaultFace(), setButtonStateDel(VControllerState::HIDDEN, 2)},
	},
	faceBtnState
	{
		std::move(faceBtnName), &defaultFace(),
		(int)layoutPosArr(vCtrl, window())[2].state,
		faceBtnStateItem
	},
	centerBtnStateItem
	{
		{ctrlStateStr[0], &defaultFace(), setButtonStateDel(VControllerState::OFF, 1)},
		{ctrlStateStr[1], &defaultFace(), setButtonStateDel(VControllerState::SHOWN, 1)},
		{ctrlStateStr[2], &defaultFace(), setButtonStateDel(VControllerState::HIDDEN, 1)},
	},
	centerBtnState
	{
		std::move(centerBtnName), &defaultFace(),
		(int)layoutPosArr(vCtrl, window())[1].state,
		centerBtnStateItem
	},
	boundingBoxes
	{
		"Show Bounding Boxes", &defaultFace(),
		vController().boundingAreaVisible(),
		[this](BoolMenuItem &item)
		{
			vController().setBoundingAreaVisible(item.flipBoolValue(*this));
			postDraw();
		}
	},
	vibrate
	{
		"Vibration", &defaultFace(),
		vController().vibrateOnTouchInput(),
		[this](BoolMenuItem &item)
		{
			vController().setVibrateOnTouchInput(app(), item.flipBoolValue(*this));
		}
	},
	showOnTouch
	{
		"Show Gamepad If Screen Touched", &defaultFace(),
		vController().showOnTouchInput(),
		[this](BoolMenuItem &item)
		{
			vController().setShowOnTouchInput(item.flipBoolValue(*this));
		}
	},
	alphaItem
	{
		{"0%",  &defaultFace(), setAlphaDel(), 0},
		{"10%", &defaultFace(), setAlphaDel(), int(255. * .1)},
		{"25%", &defaultFace(), setAlphaDel(), int(255. * .25)},
		{"50%", &defaultFace(), setAlphaDel(), int(255. * .5)},
		{"65%", &defaultFace(), setAlphaDel(), int(255. * .65)},
		{"75%", &defaultFace(), setAlphaDel(), int(255. * .75)},
	},
	alpha
	{
		"Blend Amount", &defaultFace(),
		(MenuItem::Id)vController().buttonAlpha(),
		alphaItem
	},
	btnPlace
	{
		"Set Button Positions", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShowModal(makeView<OnScreenInputPlaceView>(vController()), e);
		}
	},
	menuStateItem
	{
		{ctrlStateStr[0], &defaultFace(), setButtonStateDel(VControllerState::OFF, 3)},
		{ctrlStateStr[1], &defaultFace(), setButtonStateDel(VControllerState::SHOWN, 3)},
		{ctrlStateStr[2], &defaultFace(), setButtonStateDel(VControllerState::HIDDEN, 3)},
	},
	menuState
	{
		"Open Menu Button", &defaultFace(),
		(int)layoutPosArr(vCtrl, window())[3].state,
		[](const MultiChoiceMenuItem &)
		{
			return CAN_TURN_OFF_MENU_BTN ? 3 : 2; // iOS port doesn't use "off" value
		},
		[this](const MultiChoiceMenuItem &, size_t idx) -> TextMenuItem&
		{
			return menuStateItem[CAN_TURN_OFF_MENU_BTN ? idx : idx + 1];
		}
	},
	ffStateItem
	{
		{ctrlStateStr[0], &defaultFace(), setButtonStateDel(VControllerState::OFF, 4)},
		{ctrlStateStr[1], &defaultFace(), setButtonStateDel(VControllerState::SHOWN, 4)},
		{ctrlStateStr[2], &defaultFace(), setButtonStateDel(VControllerState::HIDDEN, 4)},
	},
	ffState
	{
		"Fast/Slow Mode Button", &defaultFace(),
		(int)layoutPosArr(vCtrl, window())[4].state,
		ffStateItem
	},
	allowButtonsPastContentBounds
	{
		"Allow Buttons In Display Cutout Area", &defaultFace(),
		vController().allowButtonsPastContentBounds(),
		[this](BoolMenuItem &item)
		{
			vController().setAllowButtonsPastContentBounds(item.flipBoolValue(*this));
			vController().place();
		}
	},
	resetControls
	{
		"Reset Position & Spacing Options", &defaultFace(),
		[this](const Input::Event &e)
		{
			auto ynAlertView = makeView<YesNoAlertView>("Reset buttons to default positions & spacing?");
			ynAlertView->setOnYes(
				[this]()
				{
					vController().resetOptions();
					vController().place();
					refreshTouchConfigMenu();
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	},
	resetAllControls
	{
		"Reset All Options", &defaultFace(),
		[this](const Input::Event &e)
		{
			auto ynAlertView = makeView<YesNoAlertView>("Reset all on-screen control options to default?");
			ynAlertView->setOnYes(
				[this]()
				{
					vController().resetAllOptions();
					vController().place();
					refreshTouchConfigMenu();
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	},
	btnTogglesHeading
	{
		"Individual Button Toggles", &defaultBoldFace()
	},
	dpadtHeading
	{
		"D-Pad Options", &defaultBoldFace()
	},
	faceBtnHeading
	{
		"Face Button Layout", &defaultBoldFace()
	},
	otherHeading
	{
		"Other Options", &defaultBoldFace()
	}
{
	item.emplace_back(&touchCtrl);
	if(EmuSystem::maxPlayers > 1)
	{
		item.emplace_back(&pointerInput);
	}
	item.emplace_back(&size);
	item.emplace_back(&btnPlace);
	if(used(allowButtonsPastContentBounds) && appContext().hasDisplayCutout())
	{
		item.emplace_back(&allowButtonsPastContentBounds);
	}
	item.emplace_back(&btnTogglesHeading);
	auto &layoutPos = layoutPosArr(vCtrl, window());
	{
		if(!CAN_TURN_OFF_MENU_BTN) // prevent iOS port from disabling menu control
		{
			if(layoutPos[3].state == VControllerState::OFF)
				layoutPos[3].state = VControllerState::SHOWN;
		}
		item.emplace_back(&menuState);
	}
	item.emplace_back(&ffState);
	item.emplace_back(&dPadState);
	item.emplace_back(&faceBtnState);
	item.emplace_back(&centerBtnState);
	if(vController().hasTriggers())
	{
		item.emplace_back(&triggerPos);
	}
	item.emplace_back(&dpadtHeading);
	item.emplace_back(&deadzone);
	item.emplace_back(&diagonalSensitivity);
	item.emplace_back(&faceBtnHeading);
	item.emplace_back(&btnSpace);
	item.emplace_back(&btnStagger);
	item.emplace_back(&btnExtraXSize);
	item.emplace_back(&btnExtraYSize);
	item.emplace_back(&otherHeading);
	item.emplace_back(&boundingBoxes);
	if(app().vibrationManager().hasVibrator())
	{
		item.emplace_back(&vibrate);
	}
	item.emplace_back(&showOnTouch);
	item.emplace_back(&alpha);
	item.emplace_back(&resetControls);
	item.emplace_back(&resetAllControls);
}

}
