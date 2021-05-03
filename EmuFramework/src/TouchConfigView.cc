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
#include "EmuOptions.hh"
#include "privateInput.hh"
#include <imagine/gui/AlertView.hh>
#include <imagine/base/Timer.hh>
#include <imagine/input/DragTracker.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/util/Interpolator.hh>
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

static constexpr const char *touchCtrlExtraXBtnSizeMenuName[4]
{
	"None", "Gap only", "10%", "25%"
};

static constexpr unsigned touchCtrlExtraXBtnSizeMenuVal[4]
{
	0, 1, 200, 500
};

static constexpr const char *touchCtrlExtraYBtnSizeMenuName[4]
{
	"None", "Gap only", "25%", "50%"
};

static constexpr unsigned touchCtrlExtraYBtnSizeMenuVal[4]
{
	0, 1, 500, 1000
};

static constexpr const char *alphaMenuName[6]
{
	"0%", "10%", "25%", "50%", "65%", "75%"
};

static constexpr unsigned alphaMenuVal[6]
{
	0, int(255 * .1), int(255 * .25), int(255 * .5), int(255 * .65), int(255 * .75)
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
		IG::Point2D<int> startPos{};
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
			postDraw();
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
					if(vController.state(i) != 0 && vController.bounds(i).contains(e.pos()))
					{
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
			}
		},
		[&](Input::DragTrackerState state, Input::DragTrackerState)
		{
			if(d.elem >= 0)
			{
				auto newPos = d.startPos + state.downPosDiff();
				waitForDrawFinished();
				vController.setPos(d.elem, newPos);
				auto layoutPos = vControllerPixelToLayoutPos(vController.bounds(d.elem).pos(C2DO), vController.bounds(d.elem).size(), viewRect());
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

template <class T, class T2, size_t S>
static int findIdxInArray(T (&arr)[S], const T2 &val)
{
	for(const auto &e : arr)
	{
		if(val == e)
		{
			return &e - arr;
		}
	}
	return -1;
}

template <class T, class T2, size_t S>
static int findIdxInArrayOrDefault(T (&arr)[S], const T2 &val, int defaultIdx)
{
	int idx = findIdxInArray(arr, val);
	if(idx == -1)
		return defaultIdx;
	return idx;
}

#ifdef CONFIG_VCONTROLS_GAMEPAD
static void setPointerInputPlayer(VController &vController, unsigned val)
{
	vController.setInputPlayer(val);
}

void TouchConfigView::setSize(uint16_t val)
{
	optionTouchCtrlSize = val;
	EmuControls::setupVControllerVars(vController);
	vController.place();
}

static void setDeadzone(VController &vController, unsigned val, Gfx::Renderer &r, Base::Window &win)
{
	optionTouchDpadDeadzone = val;
	vController.gamePad().dPad().setDeadzone(r, vController.xMMSizeToPixel(win, int(optionTouchDpadDeadzone) / 100.), vController.windowData());
}

static void setDiagonalSensitivity(VController &vController, unsigned val, Gfx::Renderer &r)
{
	optionTouchDpadDiagonalSensitivity = val;
	vController.gamePad().dPad().setDiagonalSensitivity(r, optionTouchDpadDiagonalSensitivity / 1000., vController.windowData());
}

static void setButtonSpace(VController &vController, unsigned val)
{
	optionTouchCtrlBtnSpace = val;
	EmuControls::setupVControllerVars(vController);
	vController.place();
}

static void setButtonExtraXSize(VController &vController, unsigned val)
{
	optionTouchCtrlExtraXBtnSize = val;
	EmuControls::setupVControllerVars(vController);
	vController.place();
}

static void setButtonExtraYSizeMultiRow(VController &vController, unsigned val)
{
	optionTouchCtrlExtraYBtnSizeMultiRow = val;
	EmuControls::setupVControllerVars(vController);
	vController.place();
}

static void setButtonExtraYSize(VController &vController, unsigned val)
{
	optionTouchCtrlExtraYBtnSize = val;
	EmuControls::setupVControllerVars(vController);
	vController.place();
}

static void setButtonStagger(VController &vController, unsigned val)
{
	optionTouchCtrlBtnStagger = val;
	EmuControls::setupVControllerVars(vController);
	vController.place();
}
#endif

static void setButtonState(VController &vController, unsigned state, unsigned btnIdx, Base::Window &win)
{
	vController.layoutPosition()[win.isPortrait() ? 1 : 0][btnIdx].state = state;
	vController.setLayoutPositionChanged();
	EmuControls::setupVControllerVars(vController);
}

static void setAlpha(VController &vController, unsigned val)
{
	optionTouchCtrlAlpha = val;
	vController.setAlpha((int)optionTouchCtrlAlpha / 255.0);
}

void TouchConfigView::draw(Gfx::RendererCommands &cmds)
{
	using namespace Gfx;
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
	alpha.setSelected(findIdxInArrayOrDefault(alphaMenuVal, optionTouchCtrlAlpha.val, 3), *this);
	ffState.setSelected(layoutPos[4].state, *this);
	menuState.setSelected(layoutPos[3].state - (CAN_TURN_OFF_MENU_BTN ? 0 : 1), *this);
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	touchCtrl.setSelected((int)optionTouchCtrl, *this);
	if(EmuSystem::maxPlayers > 1)
		pointerInput.setSelected((int)vController.inputPlayer(), *this);
	size.setSelected(findIdxInArrayOrDefault(touchCtrlSizeMenuVal, (unsigned)optionTouchCtrlSize, std::size(sizeItem) - 1), *this);
	dPadState.setSelected(layoutPos[0].state, *this);
	faceBtnState.setSelected(layoutPos[2].state, *this);
	centerBtnState.setSelected(layoutPos[1].state, *this);
	if(vController.hasTriggers())
	{
		lTriggerState.setSelected(layoutPos[5].state, *this);
		rTriggerState.setSelected(layoutPos[6].state, *this);
	}
	deadzone.setSelected(findIdxInArray(touchDpadDeadzoneMenuVal, (unsigned)optionTouchDpadDeadzone), *this);
	diagonalSensitivity.setSelected(findIdxInArray(touchDpadDiagonalSensitivityMenuVal, (unsigned)optionTouchDpadDiagonalSensitivity), *this);
	btnSpace.setSelected(findIdxInArray(touchCtrlBtnSpaceMenuVal, (unsigned)optionTouchCtrlBtnSpace), *this);
	btnExtraXSize.setSelected(findIdxInArray(touchCtrlExtraXBtnSizeMenuVal, (unsigned)optionTouchCtrlExtraXBtnSize), *this);
	if(EmuSystem::inputFaceBtns < 4 || (EmuSystem::inputFaceBtns == 6 && !EmuSystem::inputHasTriggerBtns))
	{
		btnExtraYSize.setSelected(findIdxInArray(touchCtrlExtraYBtnSizeMenuVal, (unsigned)optionTouchCtrlExtraYBtnSize), *this);
	}
	if(EmuSystem::inputFaceBtns >= 4)
	{
		btnExtraYSizeMultiRow.setSelected(findIdxInArray(touchCtrlExtraXBtnSizeMenuVal, (unsigned)optionTouchCtrlExtraYBtnSizeMultiRow), *this);
	}
	btnStagger.setSelected(optionTouchCtrlBtnStagger, *this);
	boundingBoxes.setBoolValue((int)optionTouchCtrlBoundingBoxes, *this);
	if(!optionVibrateOnPush.isConst)
	{
		vibrate.setBoolValue((int)optionVibrateOnPush, *this);
	}
	showOnTouch.setBoolValue((int)optionTouchCtrlShowOnTouch, *this);
	[this](auto &useScaledCoordinates)
	{
		if constexpr(Config::EmuFramework::USE_SCALED_COORDINATES)
		{
			useScaledCoordinates.setBoolValue(vController.usesScaledCoordinates(), *this);
		}
	}(useScaledCoordinates);
	#endif
}

TouchConfigView::TouchConfigView(ViewAttachParams attach, VController &vCtrl, const char *faceBtnName, const char *centerBtnName):
	TableView{"On-screen Input Setup", attach, item},
	vController{vCtrl},
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	touchCtrlItem
	{
		{"Off", &defaultFace(), [this]() { optionTouchCtrl = 0; app().viewController().setOnScreenControls(0); }},
		{"On", &defaultFace(), [this]() { optionTouchCtrl = 1; app().viewController().setOnScreenControls(1); }},
		{"Auto", &defaultFace(), [this]() { optionTouchCtrl = 2; app().viewController().updateAutoOnScreenControlVisible(); }}
	},
	touchCtrl
	{
		"Use Virtual Gamepad", &defaultFace(),
		optionTouchCtrl,
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
						if(optionTouchCtrlSize.isValidVal(scaledIntVal))
						{
							setSize(scaledIntVal);
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
			t.setString(string_makePrintf<6>("%.2f", optionTouchCtrlSize / 100.).data());
			return true;
		},
		findIdxInArrayOrDefault(touchCtrlSizeMenuVal, (unsigned)optionTouchCtrlSize, std::size(sizeItem) - 1),
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
		findIdxInArrayOrDefault(touchDpadDeadzoneMenuVal, (unsigned)optionTouchDpadDeadzone, 0),
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
		findIdxInArrayOrDefault(touchDpadDiagonalSensitivityMenuVal, (unsigned)optionTouchDpadDiagonalSensitivity, 0),
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
		findIdxInArrayOrDefault(touchCtrlBtnSpaceMenuVal, (unsigned)optionTouchCtrlBtnSpace, 0),
		btnSpaceItem
	},
	btnExtraXSizeItem
	{
		{touchCtrlExtraXBtnSizeMenuName[0], &defaultFace(), [this](){ setButtonExtraXSize(vController, touchCtrlExtraXBtnSizeMenuVal[0]); }},
		{touchCtrlExtraXBtnSizeMenuName[1], &defaultFace(), [this](){ setButtonExtraXSize(vController, touchCtrlExtraXBtnSizeMenuVal[1]); }},
		{touchCtrlExtraXBtnSizeMenuName[2], &defaultFace(), [this](){ setButtonExtraXSize(vController, touchCtrlExtraXBtnSizeMenuVal[2]); }},
		{touchCtrlExtraXBtnSizeMenuName[3], &defaultFace(), [this](){ setButtonExtraXSize(vController, touchCtrlExtraXBtnSizeMenuVal[3]); }},
	},
	btnExtraXSize
	{
		"H Overlap", &defaultFace(),
		findIdxInArrayOrDefault(touchCtrlExtraXBtnSizeMenuVal, (unsigned)optionTouchCtrlExtraXBtnSize, 0),
		btnExtraXSizeItem
	},
	btnExtraYSizeMultiRowItem
	{
		// uses same values as X counter-part
		{touchCtrlExtraXBtnSizeMenuName[0], &defaultFace(), [this](){ setButtonExtraYSizeMultiRow(vController, touchCtrlExtraXBtnSizeMenuVal[0]); }},
		{touchCtrlExtraXBtnSizeMenuName[1], &defaultFace(), [this](){ setButtonExtraYSizeMultiRow(vController, touchCtrlExtraXBtnSizeMenuVal[1]); }},
		{touchCtrlExtraXBtnSizeMenuName[2], &defaultFace(), [this](){ setButtonExtraYSizeMultiRow(vController, touchCtrlExtraXBtnSizeMenuVal[2]); }},
		{touchCtrlExtraXBtnSizeMenuName[3], &defaultFace(), [this](){ setButtonExtraYSizeMultiRow(vController, touchCtrlExtraXBtnSizeMenuVal[3]); }},
	},
	btnExtraYSizeMultiRow
	{
		(EmuSystem::inputFaceBtns == 4 || (EmuSystem::inputFaceBtns >= 6 && EmuSystem::inputHasTriggerBtns))
			? "V Overlap" : "V Overlap (2 rows)", &defaultFace(),
		findIdxInArrayOrDefault(touchCtrlExtraXBtnSizeMenuVal, (unsigned)optionTouchCtrlExtraYBtnSizeMultiRow, 0),
		btnExtraYSizeMultiRowItem
	},
	btnExtraYSizeItem
	{
		{touchCtrlExtraYBtnSizeMenuName[0], &defaultFace(), [this](){ setButtonExtraYSize(vController, touchCtrlExtraYBtnSizeMenuVal[0]); }},
		{touchCtrlExtraYBtnSizeMenuName[1], &defaultFace(), [this](){ setButtonExtraYSize(vController, touchCtrlExtraYBtnSizeMenuVal[1]); }},
		{touchCtrlExtraYBtnSizeMenuName[2], &defaultFace(), [this](){ setButtonExtraYSize(vController, touchCtrlExtraYBtnSizeMenuVal[2]); }},
		{touchCtrlExtraYBtnSizeMenuName[3], &defaultFace(), [this](){ setButtonExtraYSize(vController, touchCtrlExtraYBtnSizeMenuVal[3]); }},
	},
	btnExtraYSize
	{
		"V Overlap", &defaultFace(),
		findIdxInArrayOrDefault(touchCtrlExtraYBtnSizeMenuVal, (unsigned)optionTouchCtrlExtraYBtnSize, 0),
		btnExtraYSizeItem
	},
	triggerPos
	{
		"Inline L/R", &defaultFace(),
		(bool)optionTouchCtrlTriggerBtnPos,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionTouchCtrlTriggerBtnPos = item.flipBoolValue(*this);
			EmuControls::setupVControllerVars(vController);
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
		(int)optionTouchCtrlBtnStagger,
		btnStaggerItem
	},
	dPadStateItem
	{
		{ctrlStateStr[0], &defaultFace(), [this](){ setButtonState(vController, 0, 0, window()); }},
		{ctrlStateStr[1], &defaultFace(), [this](){ setButtonState(vController, 1, 0, window()); }},
		{ctrlStateStr[2], &defaultFace(), [this](){ setButtonState(vController, 2, 0, window()); }},
	},
	dPadState
	{
		"D-Pad", &defaultFace(),
		(int)layoutPosArr(vController, window())[0].state,
		dPadStateItem
	},
	faceBtnStateItem
	{
		{ctrlStateStr[0], &defaultFace(), [this](){ setButtonState(vController, 0, 2, window()); }},
		{ctrlStateStr[1], &defaultFace(), [this](){ setButtonState(vController, 1, 2, window()); }},
		{ctrlStateStr[2], &defaultFace(), [this](){ setButtonState(vController, 2, 2, window()); }},
	},
	faceBtnState
	{
		faceBtnName, &defaultFace(),
		(int)layoutPosArr(vController, window())[2].state,
		faceBtnStateItem
	},
	centerBtnStateItem
	{
		{ctrlStateStr[0], &defaultFace(), [this](){ setButtonState(vController, 0, 1, window()); }},
		{ctrlStateStr[1], &defaultFace(), [this](){ setButtonState(vController, 1, 1, window()); }},
		{ctrlStateStr[2], &defaultFace(), [this](){ setButtonState(vController, 2, 1, window()); }},
	},
	centerBtnState
	{
		centerBtnName, &defaultFace(),
		(int)layoutPosArr(vController, window())[1].state,
		centerBtnStateItem
	},
	lTriggerStateItem
	{
		{ctrlStateStr[0], &defaultFace(), [this](){ setButtonState(vController, 0, 5, window()); }},
		{ctrlStateStr[1], &defaultFace(), [this](){ setButtonState(vController, 1, 5, window()); }},
		{ctrlStateStr[2], &defaultFace(), [this](){ setButtonState(vController, 2, 5, window()); }},
	},
	lTriggerState
	{
		"L", &defaultFace(),
		(int)layoutPosArr(vController, window())[5].state,
		lTriggerStateItem
	},
	rTriggerStateItem
	{
		{ctrlStateStr[0], &defaultFace(), [this](){ setButtonState(vController, 0, 6, window()); }},
		{ctrlStateStr[1], &defaultFace(), [this](){ setButtonState(vController, 1, 6, window()); }},
		{ctrlStateStr[2], &defaultFace(), [this](){ setButtonState(vController, 2, 6, window()); }},
	},
	rTriggerState
	{
		"R", &defaultFace(),
		(int)layoutPosArr(vController, window())[6].state,
		rTriggerStateItem
	},
	boundingBoxes
	{
		"Show Bounding Boxes", &defaultFace(),
		(bool)optionTouchCtrlBoundingBoxes,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionTouchCtrlBoundingBoxes = item.flipBoolValue(*this);
			EmuControls::setupVControllerVars(vController);
			postDraw();
		}
	},
	vibrate
	{
		"Vibration", &defaultFace(),
		(bool)optionVibrateOnPush,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionVibrateOnPush = item.flipBoolValue(*this);
		}
	},
		#ifdef CONFIG_BASE_ANDROID
		useScaledCoordinates
		{
			"Size Units", &defaultFace(),
			vCtrl.usesScaledCoordinates(),
			"Physical (Millimeters)", "Scaled Points",
			[this](BoolMenuItem &item, Input::Event e)
			{
				vController.setUsesScaledCoordinates(item.flipBoolValue(*this));
				EmuControls::setupVControllerVars(vController);
				vController.place();
			}
		},
		#endif
	showOnTouch
	{
		"Show Gamepad If Screen Touched", &defaultFace(),
		(bool)optionTouchCtrlShowOnTouch,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionTouchCtrlShowOnTouch = item.flipBoolValue(*this);
		}
	},
	#endif // CONFIG_VCONTROLS_GAMEPAD
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
		findIdxInArrayOrDefault(alphaMenuVal, optionTouchCtrlAlpha.val, 3),
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
		{ctrlStateStr[0], &defaultFace(), [this](){ setButtonState(vController, 0, 3, window()); }},
		{ctrlStateStr[1], &defaultFace(), [this](){ setButtonState(vController, 1, 3, window()); }},
		{ctrlStateStr[2], &defaultFace(), [this](){ setButtonState(vController, 2, 3, window()); }},
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
		{ctrlStateStr[0], &defaultFace(), [this](){ setButtonState(vController, 0, 4, window()); }},
		{ctrlStateStr[1], &defaultFace(), [this](){ setButtonState(vController, 1, 4, window()); }},
		{ctrlStateStr[2], &defaultFace(), [this](){ setButtonState(vController, 2, 4, window()); }},
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
					resetVControllerOptions(vController);
					EmuControls::setupVControllerVars(vController);
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
					resetAllVControllerOptions(vController, app().viewController());
					EmuControls::setupVControllerVars(vController);
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
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	item.emplace_back(&touchCtrl);
	if(EmuSystem::maxPlayers > 1)
	{
		item.emplace_back(&pointerInput);
	}
	item.emplace_back(&size);
	#endif
	item.emplace_back(&btnPlace);
	item.emplace_back(&btnTogglesHeading);
	auto &layoutPos = layoutPosArr(vController, window());
	{
		if(!CAN_TURN_OFF_MENU_BTN) // prevent iOS port from disabling menu control
		{
			if(layoutPos[3].state == 0)
				layoutPos[3].state = 1;
		}
		item.emplace_back(&menuState);
	}
	item.emplace_back(&ffState);
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	item.emplace_back(&dPadState);
	item.emplace_back(&faceBtnState);
	item.emplace_back(&centerBtnState);
	if(vController.hasTriggers())
	{
		item.emplace_back(&lTriggerState);
		item.emplace_back(&rTriggerState);
		item.emplace_back(&triggerPos);
	}
	item.emplace_back(&dpadtHeading);
	item.emplace_back(&deadzone);
	item.emplace_back(&diagonalSensitivity);
	item.emplace_back(&faceBtnHeading);
	item.emplace_back(&btnSpace);
	item.emplace_back(&btnStagger);
	item.emplace_back(&btnExtraXSize);
	if(EmuSystem::inputFaceBtns < 4 || (EmuSystem::inputFaceBtns == 6 && !EmuSystem::inputHasTriggerBtns))
	{
		item.emplace_back(&btnExtraYSize);
	}
	if(EmuSystem::inputFaceBtns >= 4)
	{
		item.emplace_back(&btnExtraYSizeMultiRow);
	}
	item.emplace_back(&otherHeading);
	item.emplace_back(&boundingBoxes);
	if(!optionVibrateOnPush.isConst)
	{
		item.emplace_back(&vibrate);
	}
	item.emplace_back(&showOnTouch);
	[this](auto &useScaledCoordinates)
	{
		if constexpr(Config::EmuFramework::USE_SCALED_COORDINATES)
		{
			item.emplace_back(&useScaledCoordinates);
		}
	}(useScaledCoordinates);
	#else
	item.emplace_back(&otherHeading);
	#endif // CONFIG_VCONTROLS_GAMEPAD
	item.emplace_back(&alpha);
	item.emplace_back(&resetControls);
	item.emplace_back(&resetAllControls);
}
