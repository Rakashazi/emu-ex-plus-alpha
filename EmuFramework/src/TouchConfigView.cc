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
#include <emuframework/EmuInput.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/base/Timer.hh>

static const char *ctrlStateStr[] =
{
	"Off", "On", "Hidden"
};

class OnScreenInputPlaceView : public View
{
	IG::WindowRect viewFrame;
	struct DragState
	{
		constexpr DragState() {};
		int elem = -1;
		IG::Point2D<int> startPos;
	};
	Gfx::Text text;
	TimedInterpolator<float> textFade;
	Base::Timer animationStartTimer;
	Base::Screen::OnFrameDelegate animate;
	IG::WindowRect exitBtnRect;
	DragState drag[Config::Input::MAX_POINTERS];

public:
	OnScreenInputPlaceView(Base::Window &win): View(win) {}
	IG::WindowRect &viewRect() override { return viewFrame; }
	void init();
	void deinit() override;
	void place() override;
	void inputEvent(const Input::Event &e) override;
	void draw() override;
};

void OnScreenInputPlaceView::init()
{
	applyOSNavStyle(true);
	text.init("Click center to go back", View::defaultFace);
	textFade.set(1.);
	animate =
		[this](Base::Screen &screen, Base::Screen::FrameParams param)
		{
			window().setNeedsDraw(true);
			//logMsg("updating fade");
			if(textFade.update(1))
			{
				screen.postOnFrame(param.thisOnFrame());
			}
		};
	animationStartTimer.callbackAfterSec(
		[this]()
		{
			logMsg("starting fade");
			postDraw();
			textFade.set(1., 0., INTERPOLATOR_TYPE_LINEAR, 25);
			screen()->postOnFrame(animate);
		}, 2);
}

void OnScreenInputPlaceView::deinit()
{
	applyOSNavStyle(false);
	animationStartTimer.deinit();
	screen()->removeOnFrame(animate);
	text.deinit();
}

void OnScreenInputPlaceView::place()
{
	for(auto &d : drag)
	{
		d.elem = -1;
	}

	auto &win = Base::mainWindow();
	auto exitBtnPos = mainWin.viewport().bounds().pos(C2DO);
	int exitBtnSize = win.widthSMMInPixels(10.);
	exitBtnRect = IG::makeWindowRectRel(exitBtnPos - IG::WP{exitBtnSize/2, exitBtnSize/2}, {exitBtnSize, exitBtnSize});
	text.compile(projP);
}

void OnScreenInputPlaceView::inputEvent(const Input::Event &e)
{
	if(!e.isPointer() && e.state == Input::PUSHED)
	{
		dismiss();
		return;
	}

	if(e.isPointer())
	{
		if(e.pushed() && !textFade.duration())
		{
			animationStartTimer.deinit();
			logMsg("starting fade");
			textFade.set(1., 0., INTERPOLATOR_TYPE_LINEAR, 20);
			screen()->postOnFrame(animate);
		}

		auto &d = drag[e.devId];
		if(e.pushed() && d.elem == -1)
		{
			iterateTimes(vController.numElements(), i)
			{
				if(vController.state(i) != 0 && vController.bounds(i).contains({e.x, e.y}))
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
		else if(d.elem >= 0)
		{
			if(e.moved())
			{
				auto newPos = d.startPos + Input::dragState(e.devId)->dragOffset();
				vController.setPos(d.elem, newPos);
				auto layoutPos = vControllerPixelToLayoutPos(vController.bounds(d.elem).pos(C2DO), vController.bounds(d.elem).size());
				//logMsg("set pos %d,%d from %d,%d", layoutPos.pos.x, layoutPos.pos.y, layoutPos.origin.xScaler(), layoutPos.origin.yScaler());
				auto &vCtrlLayoutPos = vControllerLayoutPos[mainWin.viewport().isPortrait() ? 1 : 0];
				vCtrlLayoutPos[d.elem].origin = layoutPos.origin;
				vCtrlLayoutPos[d.elem].pos = layoutPos.pos;
				vControllerLayoutPosChanged = true;
				placeEmuViews();
				postDraw();
			}
			else if(e.released())
			{
				d.elem = -1;
			}
		}
		else if(e.released() && exitBtnRect.overlaps({e.x, e.y}) && exitBtnRect.overlaps(Input::dragState(e.devId)->pushPos()))
		{
			dismiss();
			return;
		}
	}
}

void OnScreenInputPlaceView::draw()
{
	using namespace Gfx;
	projP.resetTransforms();
	vController.draw(true, false, true, .75);
	setColor(.5, .5, .5);
	noTexProgram.use(projP.makeTranslate());
	Gfx::GC lineSize = projP.unprojectYSize(1);
	GeomRect::draw(Gfx::GCRect{-projP.wHalf(), -lineSize/(Gfx::GC)2.,
		projP.wHalf(), lineSize/(Gfx::GC)2.});
	lineSize = projP.unprojectYSize(1);
	GeomRect::draw(Gfx::GCRect{-lineSize/(Gfx::GC)2., -projP.hHalf(),
		lineSize/(Gfx::GC)2., projP.hHalf()});

	if(textFade.now() != 0.)
	{
		setColor(0., 0., 0., textFade.now()/2.);
		GeomRect::draw(Gfx::makeGCRectRel({-text.xSize/(Gfx::GC)2. - text.spaceSize, -text.ySize/(Gfx::GC)2. - text.spaceSize},
			{text.xSize + text.spaceSize*(Gfx::GC)2., text.ySize + text.spaceSize*(Gfx::GC)2.}));
		setColor(1., 1., 1., textFade.now());
		texAlphaProgram.use();
		text.draw(projP.unProjectRect(viewFrame).pos(C2DO), C2DO, projP);
	}
}

static const uint touchCtrlSizeMenuVals[] =
{
	650, 700, 750, 800, 850, 900, 1000, 1200, 1400, 1500, 1600
};

static const uint touchDpadDeadzoneMenuVals[] =
{
	100, 135, 160
};

static const uint touchDpadDiagonalSensitivityMenuVals[] =
{
	1000, 1500, 1750, 2000, 2500
};

static const uint touchCtrlBtnSpaceMenuVals[] =
{
	100, 200, 300, 400
};

static const uint touchCtrlExtraXBtnSizeMenuVals[] =
{
	0, 1, 200, 500
};

static const uint touchCtrlExtraYBtnSizeMenuVals[] =
{
	0, 1, 500, 1000
};

static const uint dpiMenuVals[] =
{
	0, 96, 120, 130, 160, 220, 240, 265, 320
};

static const uint alphaMenuVals[] =
{
	0, int(255 * .1), int(255 * .25), int(255 * .5), int(255 * .65), int(255 * .75)
};

template <class T, class T2, size_t S>
static int findIdxInArray(T (&arr)[S], const T2 &val)
{
	forEachInArray(arr, e)
	{
		if(val == *e)
		{
			return e_i;
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
	#ifdef CONFIG_ENV_WEBOS
	static void touchCtrlInit(BoolMenuItem &touchCtrl)
	{
		touchCtrl.init(int(optionTouchCtrl));
		// TODO
	}
	#else
	static void touchCtrlInit(MultiChoiceSelectMenuItem &touchCtrl)
	{
		static const char *str[] =
		{
			"Off", "On", "Auto"
		};
		touchCtrl.init(str, int(optionTouchCtrl));
	}
	#endif
#endif

void TouchConfigView::init(bool highlightFirst)
{
	uint i = 0;
	//timeout.init(); text[i++] = &timeout;
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	touchCtrlInit(touchCtrl); text[i++] = &touchCtrl;
	if(EmuSystem::maxPlayers > 1)
	{
		static const char *str[] = { "1", "2", "3", "4", "5" };
		pointerInput.init(str, pointerInputPlayer, EmuSystem::maxPlayers); text[i++] = &pointerInput;
	}
	if(EmuSystem::hasInputOptions())
	{
		systemOptions.init(); text[i++] = &systemOptions;
	}
	{
		static const char *str[] =
		{
			"6.5", "7", "7.5", "8", "8.5", "9",
			"10", "12", "14", "15", "16"
		};
		size.init(str, findIdxInArrayOrDefault(touchCtrlSizeMenuVals, (uint)optionTouchCtrlSize, 0)); text[i++] = &size;
	}
	#endif
	btnPlace.init(); text[i++] = &btnPlace;
	btnTogglesHeading.init(); text[i++] = &btnTogglesHeading;
	auto &layoutPos = vControllerLayoutPos[mainWin.viewport().isPortrait() ? 1 : 0];
	{
		if(Config::envIsIOS) // prevent iOS port from disabling menu control
		{
			if(layoutPos[3].state == 0)
				layoutPos[3].state = 1;
			menuState.init(&ctrlStateStr[1], layoutPos[3].state-1, sizeofArray(ctrlStateStr)-1);
		}
		else
			menuState.init(ctrlStateStr, layoutPos[3].state);
		text[i++] = &menuState;
	}
	ffState.init(ctrlStateStr, layoutPos[4].state); text[i++] = &ffState;
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	dPadState.init(ctrlStateStr, layoutPos[0].state); text[i++] = &dPadState;
	faceBtnState.init(ctrlStateStr, layoutPos[2].state); text[i++] = &faceBtnState;
	centerBtnState.init(ctrlStateStr, layoutPos[1].state); text[i++] = &centerBtnState;
	if(vController.hasTriggers())
	{
		lTriggerState.init(ctrlStateStr, layoutPos[5].state); text[i++] = &lTriggerState;
		rTriggerState.init(ctrlStateStr, layoutPos[6].state); text[i++] = &rTriggerState;
		triggerPos.init(optionTouchCtrlTriggerBtnPos); text[i++] = &triggerPos;
	}
	#endif
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	dpadtHeading.init(); text[i++] = &dpadtHeading;
	{
		static const char *str[] = { "1", "1.35", "1.6" };
		int init = findIdxInArrayOrDefault(touchDpadDeadzoneMenuVals, (uint)optionTouchDpadDeadzone, 0);
		deadzone.init(str, init); text[i++] = &deadzone;
	}
	{
		static const char *str[] = { "None", "Low", "M-Low","Med.", "High" };
		int init = findIdxInArrayOrDefault(touchDpadDiagonalSensitivityMenuVals, (uint)optionTouchDpadDiagonalSensitivity, 0);
		diagonalSensitivity.init(str, init); text[i++] = &diagonalSensitivity;
	}
	faceBtnHeading.init(); text[i++] = &faceBtnHeading;
	{
		static const char *str[] = { "1", "2", "3", "4" };
		int init = findIdxInArrayOrDefault(touchCtrlBtnSpaceMenuVals, (uint)optionTouchCtrlBtnSpace, 0);
		btnSpace.init(str, init); text[i++] = &btnSpace;
	}
	{
		static const char *str[] = { "-0.75x V", "-0.5x V", "0", "0.5x V", "0.75x V", "1x H&V" };
		assert(optionTouchCtrlBtnStagger < sizeofArray(str));
		int init = optionTouchCtrlBtnStagger;
		btnStagger.init(str, init); text[i++] = &btnStagger;
	}
	{
		static const char *str[] = { "None", "Gap only", "10%", "25%" };
		int init = findIdxInArrayOrDefault(touchCtrlExtraXBtnSizeMenuVals, (uint)optionTouchCtrlExtraXBtnSize, 0);
		btnExtraXSize.init(str, init); text[i++] = &btnExtraXSize;
	}
	if(EmuSystem::inputFaceBtns < 4 || (EmuSystem::inputFaceBtns == 6 && !EmuSystem::inputHasTriggerBtns))
	{
		static const char *str[] = { "None", "Gap only", "25%", "50%" };
		int init = findIdxInArrayOrDefault(touchCtrlExtraYBtnSizeMenuVals, (uint)optionTouchCtrlExtraYBtnSize, 0);
		btnExtraYSize.init(str, init); text[i++] = &btnExtraYSize;
	}
	if(EmuSystem::inputFaceBtns >= 4)
	{
		static const char *str[] = { "None", "Gap only", "10%", "25%" };
		// uses same values as X counter-part
		int init = findIdxInArrayOrDefault(touchCtrlExtraXBtnSizeMenuVals, (uint)optionTouchCtrlExtraYBtnSizeMultiRow, 0);
		btnExtraYSizeMultiRow.init((EmuSystem::inputFaceBtns == 4 || (EmuSystem::inputFaceBtns >= 6 && EmuSystem::inputHasTriggerBtns)) ? "V Overlap" : "V Overlap (2 rows)", str, init, sizeofArray(str));
		text[i++] = &btnExtraYSizeMultiRow;
	}
	otherHeading.init(); text[i++] = &otherHeading;
	boundingBoxes.init(optionTouchCtrlBoundingBoxes); text[i++] = &boundingBoxes;
	if(!optionVibrateOnPush.isConst)
	{
		vibrate.init(optionVibrateOnPush); text[i++] = &vibrate;
	}
	showOnTouch.init(optionTouchCtrlShowOnTouch); text[i++] = &showOnTouch;
		#ifdef CONFIG_BASE_ANDROID
		useScaledCoordinates.init(optionTouchCtrlScaledCoordinates); text[i++] = &useScaledCoordinates;
		#endif
	#else
	otherHeading.init(); text[i++] = &otherHeading;
	#endif // CONFIG_VCONTROLS_GAMEPAD
	{
		static const char *str[] = { "0%", "10%", "25%", "50%", "65%", "75%" };
		alpha.init(str, findIdxInArrayOrDefault(alphaMenuVals, optionTouchCtrlAlpha.val, 3)); text[i++] = &alpha;
	}
	resetControls.init(); text[i++] = &resetControls;
	resetAllControls.init(); text[i++] = &resetAllControls;
	assert(i <= sizeofArray(text));
	TableView::init(text, i, highlightFirst);
}

void TouchConfigView::draw()
{
	using namespace Gfx;
	projP.resetTransforms();
	vController.draw(true, false, true, .75);
	TableView::draw();
}

void TouchConfigView::place()
{
	refreshTouchConfigMenu();
	TableView::place();
}

void TouchConfigView::refreshTouchConfigMenu()
{
	auto &layoutPos = vControllerLayoutPos[mainWin.viewport().isPortrait() ? 1 : 0];
	alpha.updateVal(findIdxInArrayOrDefault(alphaMenuVals, optionTouchCtrlAlpha.val, 3), *this);
	ffState.updateVal(layoutPos[4].state, *this);
	menuState.updateVal(layoutPos[3].state - (Config::envIsIOS ? 1 : 0), *this);
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	touchCtrl.updateVal((int)optionTouchCtrl, *this);
	if(EmuSystem::maxPlayers > 1)
		pointerInput.updateVal((int)pointerInputPlayer, *this);
	size.updateVal(findIdxInArrayOrDefault(touchCtrlSizeMenuVals, (uint)optionTouchCtrlSize, 0), *this);
	dPadState.updateVal(layoutPos[0].state, *this);
	faceBtnState.updateVal(layoutPos[2].state, *this);
	centerBtnState.updateVal(layoutPos[1].state, *this);
	if(vController.hasTriggers())
	{
		lTriggerState.updateVal(layoutPos[5].state, *this);
		rTriggerState.updateVal(layoutPos[6].state, *this);
	}
	deadzone.updateVal(findIdxInArray(touchDpadDeadzoneMenuVals, (uint)optionTouchDpadDeadzone), *this);
	diagonalSensitivity.updateVal(findIdxInArray(touchDpadDiagonalSensitivityMenuVals, (uint)optionTouchDpadDiagonalSensitivity), *this);
	btnSpace.updateVal(findIdxInArray(touchCtrlBtnSpaceMenuVals, (uint)optionTouchCtrlBtnSpace), *this);
	btnExtraXSize.updateVal(findIdxInArray(touchCtrlExtraXBtnSizeMenuVals, (uint)optionTouchCtrlExtraXBtnSize), *this);
	if(EmuSystem::inputFaceBtns < 4 || (EmuSystem::inputFaceBtns == 6 && !EmuSystem::inputHasTriggerBtns))
	{
		btnExtraYSize.updateVal(findIdxInArray(touchCtrlExtraYBtnSizeMenuVals, (uint)optionTouchCtrlExtraYBtnSize), *this);
	}
	if(EmuSystem::inputFaceBtns >= 4)
	{
		btnExtraYSizeMultiRow.updateVal(findIdxInArray(touchCtrlExtraXBtnSizeMenuVals, (uint)optionTouchCtrlExtraYBtnSizeMultiRow), *this);
	}
	btnStagger.updateVal(optionTouchCtrlBtnStagger, *this);
	boundingBoxes.set((int)optionTouchCtrlBoundingBoxes, *this);
	if(!optionVibrateOnPush.isConst)
	{
		vibrate.set((int)optionVibrateOnPush, *this);
	}
	showOnTouch.set((int)optionTouchCtrlShowOnTouch, *this);
		#ifdef CONFIG_BASE_ANDROID
		useScaledCoordinates.set(optionTouchCtrlScaledCoordinates, *this);
		#endif
	#endif
}

TouchConfigView::TouchConfigView(Base::Window &win, const char *faceBtnName, const char *centerBtnName):
	TableView{"On-screen Input Setup", win},
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	touchCtrl
	{
		"Use Virtual Gamepad",
		#ifdef CONFIG_ENV_WEBOS
		[](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle();
			optionTouchCtrl = item.on;
			setOnScreenControls(item.on);
		}
		#else
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionTouchCtrl = val;
			if(val == 2)
				EmuControls::updateAutoOnScreenControlVisible();
			else
				EmuControls::setOnScreenControls(val);
		}
		#endif
	},
	pointerInput
	{
		"Virtual Gamepad Player",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			pointerInputPlayer = val;
			vController.updateMapping(pointerInputPlayer);
		}
	},
	size
	{
		"Button Size",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionTouchCtrlSize = touchCtrlSizeMenuVals[val];
			EmuControls::setupVControllerVars();
			vController.place();
		}
	},
	deadzone
	{
		"Deadzone",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionTouchDpadDeadzone = touchDpadDeadzoneMenuVals[val];
			vController.gp.dp.setDeadzone(vController.xMMSizeToPixel(Base::mainWindow(), int(optionTouchDpadDeadzone) / 100.));
		}
	},
	diagonalSensitivity
	{
		"Diagonal Sensitivity",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionTouchDpadDiagonalSensitivity = touchDpadDiagonalSensitivityMenuVals[val];
			vController.gp.dp.setDiagonalSensitivity(optionTouchDpadDiagonalSensitivity / 1000.);
		}
	},
	btnSpace
	{
		"Spacing",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionTouchCtrlBtnSpace = touchCtrlBtnSpaceMenuVals[val];
			EmuControls::setupVControllerVars();
			vController.place();
		}
	},
	btnExtraXSize
	{
		"H Overlap",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionTouchCtrlExtraXBtnSize = touchCtrlExtraXBtnSizeMenuVals[val];
			EmuControls::setupVControllerVars();
			vController.place();
		}
	},
	btnExtraYSizeMultiRow
	{
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionTouchCtrlExtraYBtnSizeMultiRow = touchCtrlExtraXBtnSizeMenuVals[val];
			EmuControls::setupVControllerVars();
			vController.place();
		}
	},
	btnExtraYSize
	{
		"V Overlap",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionTouchCtrlExtraYBtnSize = touchCtrlExtraYBtnSizeMenuVals[val];
			EmuControls::setupVControllerVars();
			vController.place();
		}
	},
	triggerPos
	{
		"Inline L/R",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			optionTouchCtrlTriggerBtnPos = item.on;
			EmuControls::setupVControllerVars();
		}
	},
	btnStagger
	{
		"Stagger",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionTouchCtrlBtnStagger = val;
			EmuControls::setupVControllerVars();
			vController.place();
		}
	},
	dPadState
	{
		"D-Pad",
		[this](MultiChoiceMenuItem &item, View &, int val)
		{
			vControllerLayoutPos[mainWin.viewport().isPortrait() ? 1 : 0][0].state = val;
			vControllerLayoutPosChanged = true;
			EmuControls::setupVControllerVars();
		}
	},
	faceBtnState
	{
		faceBtnName,
		[this](MultiChoiceMenuItem &item, View &, int val)
		{
			vControllerLayoutPos[mainWin.viewport().isPortrait() ? 1 : 0][2].state = val;
			vControllerLayoutPosChanged = true;
			EmuControls::setupVControllerVars();
		}
	},
	centerBtnState
	{
		centerBtnName,
		[this](MultiChoiceMenuItem &item, View &, int val)
		{
			vControllerLayoutPos[mainWin.viewport().isPortrait() ? 1 : 0][1].state = val;
			vControllerLayoutPosChanged = true;
			EmuControls::setupVControllerVars();
		}
	},
	lTriggerState
	{
		"L",
		[this](MultiChoiceMenuItem &item, View &, int val)
		{
			vControllerLayoutPos[mainWin.viewport().isPortrait() ? 1 : 0][5].state = val;
			vControllerLayoutPosChanged = true;
			EmuControls::setupVControllerVars();
		}
	},
	rTriggerState
	{
		"R",
		[this](MultiChoiceMenuItem &item, View &, int val)
		{
			vControllerLayoutPos[mainWin.viewport().isPortrait() ? 1 : 0][6].state = val;
			vControllerLayoutPosChanged = true;
			EmuControls::setupVControllerVars();
		}
	},
	boundingBoxes
	{
		"Show Bounding Boxes",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			optionTouchCtrlBoundingBoxes = item.on;
			EmuControls::setupVControllerVars();
			postDraw();
		}
	},
	vibrate
	{
		"Vibration",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			optionVibrateOnPush = item.on;
		}
	},
		#ifdef CONFIG_BASE_ANDROID
		useScaledCoordinates
		{
			"Size Units", "Physical (Millimeters)", "Scaled Points",
			[this](BoolMenuItem &item, View &, const Input::Event &e)
			{
				item.toggle(*this);
				optionTouchCtrlScaledCoordinates = item.on;
				EmuControls::setupVControllerVars();
				vController.place();
			}
		},
		#endif
	showOnTouch
	{
		"Show Gamepad If Screen Touched",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			optionTouchCtrlShowOnTouch = item.on;
		}
	},
	#endif // CONFIG_VCONTROLS_GAMEPAD
	alpha
	{
		"Blend Amount",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionTouchCtrlAlpha = alphaMenuVals[val];
			vController.alpha = (int)optionTouchCtrlAlpha / 255.0;
		}
	},
	btnPlace
	{
		"Set Button Positions",
		[this](TextMenuItem &, View &, const Input::Event &e)
		{
			auto &onScreenInputPlace = *new OnScreenInputPlaceView{window()};
			onScreenInputPlace.init();
			modalViewController.pushAndShow(onScreenInputPlace);
		}
	},
	menuState
	{
		"Open Menu Button",
		[this](MultiChoiceMenuItem &item, View &, int val)
		{
			if(Config::envIsIOS) // iOS port doesn't use "off" value
			{
				val++;
			}
			vControllerLayoutPos[mainWin.viewport().isPortrait() ? 1 : 0][3].state = val;
			vControllerLayoutPosChanged = true;
			EmuControls::setupVControllerVars();
		}
	},
	ffState
	{
		"Fast-forward Button",
		[this](MultiChoiceMenuItem &item, View &, int val)
		{
			vControllerLayoutPos[mainWin.viewport().isPortrait() ? 1 : 0][4].state = val;
			vControllerLayoutPosChanged = true;
			EmuControls::setupVControllerVars();
		}
	},
	resetControls
	{
		"Reset Position & Spacing Options",
		[this](TextMenuItem &, View &, const Input::Event &e)
		{
			auto &ynAlertView = *new YesNoAlertView{window()};
			ynAlertView.init("Reset buttons to default positions & spacing?", !e.isPointer());
			ynAlertView.onYes() =
				[this](const Input::Event &e)
				{
					resetVControllerOptions();
					EmuControls::setupVControllerVars();
					refreshTouchConfigMenu();
				};
			modalViewController.pushAndShow(ynAlertView);
		}
	},
	resetAllControls
	{
		"Reset All Options",
		[this](TextMenuItem &, View &, const Input::Event &e)
		{
			auto &ynAlertView = *new YesNoAlertView{window()};
			ynAlertView.init("Reset all on-screen control options to default?", !e.isPointer());
			ynAlertView.onYes() =
				[this](const Input::Event &e)
				{
					resetAllVControllerOptions();
					EmuControls::setupVControllerVars();
					refreshTouchConfigMenu();
				};
			modalViewController.pushAndShow(ynAlertView);
		}
	},
	systemOptions
	{
		"Emulated System Options",
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			auto &optView = *makeOptionCategoryMenu(window(), e, 2);
			pushAndShow(optView);
		}
	},
	btnTogglesHeading
	{
		"Individual Button Toggles"
	},
	dpadtHeading
	{
		"D-Pad Options"
	},
	faceBtnHeading
	{
		"Face Button Layout"
	},
	otherHeading
	{
		"Other Options"
	}
{}
