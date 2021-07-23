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
#include <imagine/util/algorithm.h>
#include <imagine/util/string.h>
#include <imagine/logger/logger.h>
#include <utility>

static constexpr bool CAN_TURN_OFF_MENU_BTN = !Config::envIsIOS;

static constexpr const char *ctrlStateStr[]
{
	"Off", "On", "Hidden"
};

static constexpr const char *touchCtrlSizeMenuName[10]
{
	"6.5",
	"7",
	"7.5",
	"8",
	"8.5",
	"9",
	"10",
	"12",
	"14",
	"15",
};

static constexpr unsigned touchCtrlSizeMenuVal[10]
{
	650,
	700,
	750,
	800,
	850,
	900,
	1000,
	1200,
	1400,
	1500,
};

static constexpr const char *touchDpadDeadzoneMenuName[3]
{
	"1", "1.35", "1.6"
};

static constexpr unsigned touchDpadDeadzoneMenuVal[3]
{
	100, 135, 160
};

static constexpr const char *touchDpadDiagonalSensitivityMenuName[5]
{
	"None", "Low", "M-Low","Med.", "High"
};

static constexpr unsigned touchDpadDiagonalSensitivityMenuVal[5]
{
	1000, 1500, 1750, 2000, 2500
};

static constexpr const char *touchCtrlBtnSpaceMenuName[4]
{
	"1", "2", "3", "4"
};

static constexpr unsigned touchCtrlBtnSpaceMenuVal[4]
{
	100, 200, 300, 400
};

static constexpr const char *touchCtrlExtraBtnSizeMenuName[4]
{
	"None", "Gap only", "10%", "25%"
};

static constexpr unsigned touchCtrlExtraBtnSizeMenuVal[4]
{
	0, 1, 200, 500
};

static constexpr const char *alphaMenuName[6]
{
	"0%", "10%", "25%", "50%", "65%", "75%"
};

static constexpr uint8_t alphaMenuVal[6]
{
	0, uint8_t(255. * .1), uint8_t(255. * .25), uint8_t(255. * .5), uint8_t(255. * .65), uint8_t(255. * .75)
};

static auto &layoutPosArr(VController &vController, Base::Window &win)
{
	return vController.layoutPosition()[win.isPortrait() ? 1 : 0];
}

class OnScreenInputPlaceView : public View, public EmuAppHelper<OnScreenInputPlaceView>
{
	struct DragState
	{
		constexpr DragState() {};
		int elem{-1};
		IG::WP startPos{};
	};
	Gfx::Text text{};
	VController &vController;
	IG::InterpolatorValue<float, IG::FrameTime, IG::InterpolatorType::LINEAR> textFade{};
	Base::Timer animationStartTimer{"OnScreenInputPlaceView::animationStartTimer"};
	Base::OnFrameDelegate animate{};
	IG::WindowRect exitBtnRect{};
	DragState drag[Config::Input::MAX_POINTERS]{};
	Input::DragTracker dragTracker{};

public:
	OnScreenInputPlaceView(ViewAttachParams attach, VController &vController);
	~OnScreenInputPlaceView();
	void place() final;
	bool inputEvent(Input::Event e) final;
	void draw(Gfx::RendererCommands &cmds) final;
};

OnScreenInputPlaceView::OnScreenInputPlaceView(ViewAttachParams attach, VController &vController):
	View(attach),
	text{"Click center to go back", &defaultFace()},
	vController{vController},
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
	for(auto &d : drag)
	{
		d.elem = -1;
	}

	auto exitBtnPos = viewRect().pos(C2DO);
	int exitBtnSize = window().widthSMMInPixels(10.);
	exitBtnRect = IG::makeWindowRectRel(exitBtnPos - IG::WP{exitBtnSize/2, exitBtnSize/2}, {exitBtnSize, exitBtnSize});
	text.compile(renderer(), projP);
}

bool OnScreenInputPlaceView::inputEvent(Input::Event e)
{
	if(!e.isPointer())
	{
		if(e.pushed())
		{
			dismiss();
			return true;
		}
		return false;
	}
	if(e.pushed() && animationStartTimer.isArmed())
	{
		animationStartTimer.dispatchEarly();
	}
	auto &d = drag[e.deviceID()];
	dragTracker.inputEvent(e,
		[&](Input::DragTrackerState)
		{
			if(d.elem == -1)
			{
				iterateTimes(vController.numElements(), i)
				{
					if(vController.state(i) == VControllerState::OFF || !vController.bounds(i).contains(e.pos()))
						continue;
					for(auto &otherDrag : drag)
					{
						if(otherDrag.elem == (int)i)
							continue; // element already grabbed
					}
					d.elem = i;
					d.startPos = vController.bounds(d.elem).pos(C2DO);
					break;
				}
			}
		},
		[&](Input::DragTrackerState state, Input::DragTrackerState)
		{
			if(d.elem >= 0)
			{
				auto newPos = d.startPos + state.downPosDiff();
				waitForDrawFinished();
				vController.setPos(d.elem, newPos);
				auto layoutPos = vController.pixelToLayoutPos(vController.bounds(d.elem).pos(C2DO), vController.bounds(d.elem).size(), viewRect());
				//logMsg("set pos %d,%d from %d,%d", layoutPos.pos.x, layoutPos.pos.y, layoutPos.origin.xScaler(), layoutPos.origin.yScaler());
				auto &vCtrlLayoutPos = vController.layoutPosition()[window().isPortrait() ? 1 : 0];
				vCtrlLayoutPos[d.elem].origin = layoutPos.origin;
				vCtrlLayoutPos[d.elem].pos = layoutPos.pos;
				vController.setLayoutPositionChanged();
				app().viewController().placeEmuViews();
				postDraw();
			}
		},
		[&](Input::DragTrackerState state)
		{
			if(d.elem >= 0)
			{
				d.elem = -1;
			}
			else if(exitBtnRect.overlaps(state.pos()) && exitBtnRect.overlaps(state.downPos()))
			{
				dismiss();
			}
		});
	return true;
}

void OnScreenInputPlaceView::draw(Gfx::RendererCommands &cmds)
{
	using namespace Gfx;
	projP.resetTransforms(cmds);
	vController.draw(cmds, true, false, true, .75);
	cmds.setColor(.5, .5, .5);
	cmds.setCommonProgram(CommonProgram::NO_TEX, projP.makeTranslate());
	Gfx::GC lineSize = projP.unprojectYSize(1);
	GeomRect::draw(cmds, Gfx::GCRect{{-projP.wHalf(), -lineSize/(Gfx::GC)2.},
		{projP.wHalf(), lineSize/(Gfx::GC)2.}});
	lineSize = projP.unprojectYSize(1);
	GeomRect::draw(cmds, Gfx::GCRect{{-lineSize/(Gfx::GC)2., -projP.hHalf()},
		{lineSize/(Gfx::GC)2., projP.hHalf()}});

	if(textFade != 0.)
	{
		cmds.setColor(0., 0., 0., textFade/2.);
		GeomRect::draw(cmds, Gfx::makeGCRectRel({-text.width()/(Gfx::GC)2. - text.spaceWidth(), -text.height()/(Gfx::GC)2. - text.spaceWidth()},
			{text.width() + text.spaceWidth()*(Gfx::GC)2., text.height() + text.spaceWidth()*(Gfx::GC)2.}));
		cmds.setColor(1., 1., 1., textFade);
		cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
		text.draw(cmds, projP.unProjectRect(viewRect()).pos(C2DO), C2DO, projP);
	}
}

static void setPointerInputPlayer(VController &vController, unsigned val)
{
	vController.setInputPlayer(val);
}

void TouchConfigView::setSize(uint16_t val)
{
	vController.setButtonSize(val);
}

static void setDeadzone(VController &vController, unsigned val, Gfx::Renderer &r, Base::Window &win)
{
	vController.setDpadDeadzone(val);
}

static void setDiagonalSensitivity(VController &vController, unsigned val, Gfx::Renderer &r)
{
	vController.setDpadDiagonalSensitivity(val);
}

static void setButtonSpace(VController &vController, unsigned val)
{
	vController.setButtonSpacing(val);
}

static void setButtonExtraXSize(VController &vController, unsigned val)
{
	vController.setButtonXPadding(val);
}

static void setButtonExtraYSize(VController &vController, unsigned val)
{
	vController.setButtonYPadding(val);
}

static void setButtonStagger(VController &vController, unsigned val)
{
	vController.setButtonStagger(val);
}

static void setButtonState(VController &vController, VControllerState state, unsigned btnIdx, Base::Window &win)
{
	vController.layoutPosition()[win.isPortrait() ? 1 : 0][btnIdx].state = state;
	vController.setLayoutPositionChanged();
	vController.place();
}

static void setAlpha(VController &vController, uint8_t val)
{
	vController.setButtonAlpha(val);
}

void TouchConfigView::draw(Gfx::RendererCommands &cmds)
{
	projP.resetTransforms(cmds);
	vController.draw(cmds, true, false, true, .75);
	TableView::draw(cmds);
}

void TouchConfigView::place()
{
	refreshTouchConfigMenu();
	TableView::place();
}

void TouchConfigView::refreshTouchConfigMenu()
{
	auto &layoutPos = layoutPosArr(vController, window());
	alpha.setSelected(IG::findIndex(alphaMenuVal, vController.buttonAlpha(), 3), *this);
	ffState.setSelected((int)layoutPos[4].state, *this);
	menuState.setSelected((int)layoutPos[3].state - (CAN_TURN_OFF_MENU_BTN ? 0 : 1), *this);
	touchCtrl.setSelected((int)vController.gamepadControlsVisibility(), *this);
	if(EmuSystem::maxPlayers > 1)
		pointerInput.setSelected((int)vController.inputPlayer(), *this);
	size.setSelected(IG::findIndex(touchCtrlSizeMenuVal, (unsigned)vController.buttonSize(), std::size(sizeItem) - 1), *this);
	dPadState.setSelected((int)layoutPos[0].state, *this);
	faceBtnState.setSelected((int)layoutPos[2].state, *this);
	centerBtnState.setSelected((int)layoutPos[1].state, *this);
	if(vController.hasTriggers())
	{
		triggerPos.setBoolValue(vController.triggersInline());
	}
	deadzone.setSelected(IG::findIndex(touchDpadDeadzoneMenuVal, vController.dpadDeadzone(), 0), *this);
	diagonalSensitivity.setSelected(IG::findIndex(touchDpadDiagonalSensitivityMenuVal, vController.dpadDiagonalSensitivity(), 0), *this);
	btnSpace.setSelected(IG::findIndex(touchCtrlBtnSpaceMenuVal, vController.buttonSpacing(), 0), *this);
	btnExtraXSize.setSelected(IG::findIndex(touchCtrlExtraBtnSizeMenuVal, vController.buttonXPadding(), 0), *this);
	btnExtraYSize.setSelected(IG::findIndex(touchCtrlExtraBtnSizeMenuVal, vController.buttonYPadding(), 0), *this);
	btnStagger.setSelected(vController.buttonStagger(), *this);
	boundingBoxes.setBoolValue(vController.boundingAreaVisible(), *this);
	if(app().vibrationManager().hasVibrator())
	{
		vibrate.setBoolValue(vController.vibrateOnTouchInput(), *this);
	}
	showOnTouch.setBoolValue(vController.showOnTouchInput(), *this);
}

TouchConfigView::TouchConfigView(ViewAttachParams attach, VController &vCtrl, const char *faceBtnName, const char *centerBtnName):
	TableView{"On-screen Input Setup", attach, item},
	vController{vCtrl},
	touchCtrlItem
	{
		{"Off", &defaultFace(), [this]() { vController.setGamepadControlsVisibility(VControllerVisibility::OFF); }},
		{"On", &defaultFace(), [this]() { vController.setGamepadControlsVisibility(VControllerVisibility::ON); }},
		{"Auto", &defaultFace(), [this]() { vController.setGamepadControlsVisibility(VControllerVisibility::AUTO); }}
	},
	touchCtrl
	{
		"Use Virtual Gamepad", &defaultFace(),
		(int)vController.gamepadControlsVisibility(),
		touchCtrlItem
	},
	pointerInputItem
	{
		{"1", &defaultFace(), [this](){ setPointerInputPlayer(vController, 0); }},
		{"2", &defaultFace(), [this](){ setPointerInputPlayer(vController, 1); }},
		{"3", &defaultFace(), [this](){ setPointerInputPlayer(vController, 2); }},
		{"4", &defaultFace(), [this](){ setPointerInputPlayer(vController, 3); }},
		{"5", &defaultFace(), [this](){ setPointerInputPlayer(vController, 4); }}
	},
	pointerInput
	{
		"Virtual Gamepad Player", &defaultFace(),
		(int)vCtrl.inputPlayer(),
		[this](const MultiChoiceMenuItem &) -> int
		{
			return EmuSystem::maxPlayers;
		},
		[this](const MultiChoiceMenuItem &, unsigned idx) -> TextMenuItem&
		{
			return pointerInputItem[idx];
		}
	},
	sizeItem
	{
		{touchCtrlSizeMenuName[0], &defaultFace(), [this](){ setSize(touchCtrlSizeMenuVal[0]); }},
		{touchCtrlSizeMenuName[1], &defaultFace(), [this](){ setSize(touchCtrlSizeMenuVal[1]); }},
		{touchCtrlSizeMenuName[2], &defaultFace(), [this](){ setSize(touchCtrlSizeMenuVal[2]); }},
		{touchCtrlSizeMenuName[3], &defaultFace(), [this](){ setSize(touchCtrlSizeMenuVal[3]); }},
		{touchCtrlSizeMenuName[4], &defaultFace(), [this](){ setSize(touchCtrlSizeMenuVal[4]); }},
		{touchCtrlSizeMenuName[5], &defaultFace(), [this](){ setSize(touchCtrlSizeMenuVal[5]); }},
		{touchCtrlSizeMenuName[6], &defaultFace(), [this](){ setSize(touchCtrlSizeMenuVal[6]); }},
		{touchCtrlSizeMenuName[7], &defaultFace(), [this](){ setSize(touchCtrlSizeMenuVal[7]); }},
		{touchCtrlSizeMenuName[8], &defaultFace(), [this](){ setSize(touchCtrlSizeMenuVal[8]); }},
		{touchCtrlSizeMenuName[9], &defaultFace(), [this](){ setSize(touchCtrlSizeMenuVal[9]); }},
		{"Custom Value", &defaultFace(),
			[this](Input::Event e)
			{
				app().pushAndShowNewCollectValueInputView<double>(attachParams(), e, "Input 3.0 to 15.0", "",
					[this](EmuApp &app, auto val)
					{
						int scaledIntVal = val * 100.0;
						if(vController.setButtonSize(scaledIntVal))
						{
							size.setSelected(std::size(sizeItem) - 1, *this);
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
			}
		},
	},
	size
	{
		"Button Size", &defaultFace(),
		[this](uint32_t idx, Gfx::Text &t)
		{
			t.setString(string_makePrintf<6>("%.2f", vController.buttonSize() / 100.).data());
			return true;
		},
		IG::findIndex(touchCtrlSizeMenuVal, (unsigned)vController.buttonSize(), std::size(sizeItem) - 1),
		sizeItem
	},
	deadzoneItem
	{
		{touchDpadDeadzoneMenuName[0], &defaultFace(), [this](){ setDeadzone(vController, touchDpadDeadzoneMenuVal[0], renderer(), window()); }},
		{touchDpadDeadzoneMenuName[1], &defaultFace(), [this](){ setDeadzone(vController, touchDpadDeadzoneMenuVal[1], renderer(), window()); }},
		{touchDpadDeadzoneMenuName[2], &defaultFace(), [this](){ setDeadzone(vController, touchDpadDeadzoneMenuVal[2], renderer(), window()); }},
	},
	deadzone
	{
		"Deadzone", &defaultFace(),
		IG::findIndex(touchDpadDeadzoneMenuVal, vController.dpadDeadzone(), 0),
		deadzoneItem
	},
	diagonalSensitivityItem
	{
		{touchDpadDiagonalSensitivityMenuName[0], &defaultFace(), [this](){ setDiagonalSensitivity(vController, touchDpadDiagonalSensitivityMenuVal[0], renderer()); }},
		{touchDpadDiagonalSensitivityMenuName[1], &defaultFace(), [this](){ setDiagonalSensitivity(vController, touchDpadDiagonalSensitivityMenuVal[1], renderer()); }},
		{touchDpadDiagonalSensitivityMenuName[2], &defaultFace(), [this](){ setDiagonalSensitivity(vController, touchDpadDiagonalSensitivityMenuVal[2], renderer()); }},
		{touchDpadDiagonalSensitivityMenuName[3], &defaultFace(), [this](){ setDiagonalSensitivity(vController, touchDpadDiagonalSensitivityMenuVal[3], renderer()); }},
		{touchDpadDiagonalSensitivityMenuName[4], &defaultFace(), [this](){ setDiagonalSensitivity(vController, touchDpadDiagonalSensitivityMenuVal[4], renderer()); }},
	},
	diagonalSensitivity
	{
		"Diagonal Sensitivity", &defaultFace(),
		IG::findIndex(touchDpadDiagonalSensitivityMenuVal, vController.dpadDiagonalSensitivity(), 0),
		diagonalSensitivityItem
	},
	btnSpaceItem
	{
		{touchCtrlBtnSpaceMenuName[0], &defaultFace(), [this](){ setButtonSpace(vController, touchCtrlBtnSpaceMenuVal[0]); }},
		{touchCtrlBtnSpaceMenuName[1], &defaultFace(), [this](){ setButtonSpace(vController, touchCtrlBtnSpaceMenuVal[1]); }},
		{touchCtrlBtnSpaceMenuName[2], &defaultFace(), [this](){ setButtonSpace(vController, touchCtrlBtnSpaceMenuVal[2]); }},
		{touchCtrlBtnSpaceMenuName[3], &defaultFace(), [this](){ setButtonSpace(vController, touchCtrlBtnSpaceMenuVal[3]); }},
	},
	btnSpace
	{
		"Spacing", &defaultFace(),
		IG::findIndex(touchCtrlBtnSpaceMenuVal, vController.buttonSpacing(), 0),
		btnSpaceItem
	},
	btnExtraXSizeItem
	{
		{touchCtrlExtraBtnSizeMenuName[0], &defaultFace(), [this](){ setButtonExtraXSize(vController, touchCtrlExtraBtnSizeMenuVal[0]); }},
		{touchCtrlExtraBtnSizeMenuName[1], &defaultFace(), [this](){ setButtonExtraXSize(vController, touchCtrlExtraBtnSizeMenuVal[1]); }},
		{touchCtrlExtraBtnSizeMenuName[2], &defaultFace(), [this](){ setButtonExtraXSize(vController, touchCtrlExtraBtnSizeMenuVal[2]); }},
		{touchCtrlExtraBtnSizeMenuName[3], &defaultFace(), [this](){ setButtonExtraXSize(vController, touchCtrlExtraBtnSizeMenuVal[3]); }},
	},
	btnExtraXSize
	{
		"H Overlap", &defaultFace(),
		IG::findIndex(touchCtrlExtraBtnSizeMenuVal, vController.buttonXPadding(), 0),
		btnExtraXSizeItem
	},
	btnExtraYSizeItem
	{
		{touchCtrlExtraBtnSizeMenuName[0], &defaultFace(), [this](){ setButtonExtraYSize(vController, touchCtrlExtraBtnSizeMenuVal[0]); }},
		{touchCtrlExtraBtnSizeMenuName[1], &defaultFace(), [this](){ setButtonExtraYSize(vController, touchCtrlExtraBtnSizeMenuVal[1]); }},
		{touchCtrlExtraBtnSizeMenuName[2], &defaultFace(), [this](){ setButtonExtraYSize(vController, touchCtrlExtraBtnSizeMenuVal[2]); }},
		{touchCtrlExtraBtnSizeMenuName[3], &defaultFace(), [this](){ setButtonExtraYSize(vController, touchCtrlExtraBtnSizeMenuVal[3]); }},
	},
	btnExtraYSize
	{
		"V Overlap", &defaultFace(),
		IG::findIndex(touchCtrlExtraBtnSizeMenuVal, vController.buttonYPadding(), 0),
		btnExtraYSizeItem
	},
	triggerPos
	{
		"Inline L/R", &defaultFace(),
		vController.triggersInline(),
		[this](BoolMenuItem &item, Input::Event e)
		{
			vController.setTriggersInline(item.flipBoolValue(*this));
		}
	},
	btnStaggerItem
	{
		{"-0.75x V", &defaultFace(), [this](){ setButtonStagger(vController, 0); }},
		{"-0.5x V", &defaultFace(), [this](){ setButtonStagger(vController, 1); }},
		{"0", &defaultFace(), [this](){ setButtonStagger(vController, 2); }},
		{"0.5x V", &defaultFace(), [this](){ setButtonStagger(vController, 3); }},
		{"0.75x V", &defaultFace(), [this](){ setButtonStagger(vController, 4); }},
		{"1x H&V", &defaultFace(), [this](){ setButtonStagger(vController, 5); }},
	},
	btnStagger
	{
		"Stagger", &defaultFace(),
		vController.buttonStagger(),
		btnStaggerItem
	},
	dPadStateItem
	{
		{ctrlStateStr[0], &defaultFace(), [this](){ setButtonState(vController, VControllerState::OFF, 0, window()); }},
		{ctrlStateStr[1], &defaultFace(), [this](){ setButtonState(vController, VControllerState::SHOWN, 0, window()); }},
		{ctrlStateStr[2], &defaultFace(), [this](){ setButtonState(vController, VControllerState::HIDDEN, 0, window()); }},
	},
	dPadState
	{
		"D-Pad", &defaultFace(),
		(int)layoutPosArr(vController, window())[0].state,
		dPadStateItem
	},
	faceBtnStateItem
	{
		{ctrlStateStr[0], &defaultFace(), [this](){ setButtonState(vController, VControllerState::OFF, 2, window()); }},
		{ctrlStateStr[1], &defaultFace(), [this](){ setButtonState(vController, VControllerState::SHOWN, 2, window()); }},
		{ctrlStateStr[2], &defaultFace(), [this](){ setButtonState(vController, VControllerState::HIDDEN, 2, window()); }},
	},
	faceBtnState
	{
		faceBtnName, &defaultFace(),
		(int)layoutPosArr(vController, window())[2].state,
		faceBtnStateItem
	},
	centerBtnStateItem
	{
		{ctrlStateStr[0], &defaultFace(), [this](){ setButtonState(vController, VControllerState::OFF, 1, window()); }},
		{ctrlStateStr[1], &defaultFace(), [this](){ setButtonState(vController, VControllerState::SHOWN, 1, window()); }},
		{ctrlStateStr[2], &defaultFace(), [this](){ setButtonState(vController, VControllerState::HIDDEN, 1, window()); }},
	},
	centerBtnState
	{
		centerBtnName, &defaultFace(),
		(int)layoutPosArr(vController, window())[1].state,
		centerBtnStateItem
	},
	boundingBoxes
	{
		"Show Bounding Boxes", &defaultFace(),
		vController.boundingAreaVisible(),
		[this](BoolMenuItem &item, Input::Event e)
		{
			vController.setBoundingAreaVisible(item.flipBoolValue(*this));
			postDraw();
		}
	},
	vibrate
	{
		"Vibration", &defaultFace(),
		vController.vibrateOnTouchInput(),
		[this](BoolMenuItem &item, Input::Event e)
		{
			vController.setVibrateOnTouchInput(item.flipBoolValue(*this));
		}
	},
	showOnTouch
	{
		"Show Gamepad If Screen Touched", &defaultFace(),
		vController.showOnTouchInput(),
		[this](BoolMenuItem &item, Input::Event e)
		{
			vController.setShowOnTouchInput(item.flipBoolValue(*this));
		}
	},
	alphaItem
	{
		{alphaMenuName[0], &defaultFace(), [this](){ setAlpha(vController, alphaMenuVal[0]); }},
		{alphaMenuName[1], &defaultFace(), [this](){ setAlpha(vController, alphaMenuVal[1]); }},
		{alphaMenuName[2], &defaultFace(), [this](){ setAlpha(vController, alphaMenuVal[2]); }},
		{alphaMenuName[3], &defaultFace(), [this](){ setAlpha(vController, alphaMenuVal[3]); }},
		{alphaMenuName[4], &defaultFace(), [this](){ setAlpha(vController, alphaMenuVal[4]); }},
		{alphaMenuName[5], &defaultFace(), [this](){ setAlpha(vController, alphaMenuVal[5]); }},
	},
	alpha
	{
		"Blend Amount", &defaultFace(),
		IG::findIndex(alphaMenuVal, vController.buttonAlpha(), 3),
		alphaItem
	},
	btnPlace
	{
		"Set Button Positions", &defaultFace(),
		[this](Input::Event e)
		{
			auto onScreenInputPlace = makeView<OnScreenInputPlaceView>(vController);
			pushAndShowModal(std::move(onScreenInputPlace), e);
		}
	},
	menuStateItem
	{
		{ctrlStateStr[0], &defaultFace(), [this](){ setButtonState(vController, VControllerState::OFF, 3, window()); }},
		{ctrlStateStr[1], &defaultFace(), [this](){ setButtonState(vController, VControllerState::SHOWN, 3, window()); }},
		{ctrlStateStr[2], &defaultFace(), [this](){ setButtonState(vController, VControllerState::HIDDEN, 3, window()); }},
	},
	menuState
	{
		"Open Menu Button", &defaultFace(),
		(int)layoutPosArr(vController, window())[3].state,
		[](const MultiChoiceMenuItem &) -> int
		{
			return CAN_TURN_OFF_MENU_BTN ? 3 : 2; // iOS port doesn't use "off" value
		},
		[this](const MultiChoiceMenuItem &, unsigned idx) -> TextMenuItem&
		{
			return menuStateItem[CAN_TURN_OFF_MENU_BTN ? idx : idx + 1];
		}
	},
	ffStateItem
	{
		{ctrlStateStr[0], &defaultFace(), [this](){ setButtonState(vController, VControllerState::OFF, 4, window()); }},
		{ctrlStateStr[1], &defaultFace(), [this](){ setButtonState(vController, VControllerState::SHOWN, 4, window()); }},
		{ctrlStateStr[2], &defaultFace(), [this](){ setButtonState(vController, VControllerState::HIDDEN, 4, window()); }},
	},
	ffState
	{
		"Fast-forward Button", &defaultFace(),
		(int)layoutPosArr(vController, window())[4].state,
		ffStateItem
	},
	resetControls
	{
		"Reset Position & Spacing Options", &defaultFace(),
		[this](Input::Event e)
		{
			auto ynAlertView = makeView<YesNoAlertView>("Reset buttons to default positions & spacing?");
			ynAlertView->setOnYes(
				[this]()
				{
					vController.resetOptions();
					vController.place();
					refreshTouchConfigMenu();
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	},
	resetAllControls
	{
		"Reset All Options", &defaultFace(),
		[this](Input::Event e)
		{
			auto ynAlertView = makeView<YesNoAlertView>("Reset all on-screen control options to default?");
			ynAlertView->setOnYes(
				[this]()
				{
					vController.resetAllOptions();
					vController.place();
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
	item.emplace_back(&btnTogglesHeading);
	auto &layoutPos = layoutPosArr(vController, window());
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
	if(vController.hasTriggers())
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
