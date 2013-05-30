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

#include <TouchConfigView.hh>
#include <EmuInput.hh>

void setupFont();

static const char *ctrlPosStr[] =
{
	"Top-Left", "Mid-Left", "Bottom-Left", "Top-Right", "Mid-Right", "Bottom-Right", "Top", "Bottom", "Off"
};

static int triggerOptionToVal(uint o)
{
	if(o == (uint)TRIGGERS_INLINE)
		return 0;
	else if(o == (uint)TRIGGERS_RIGHT)
		return 1;
	else if(o == (uint)TRIGGERS_LEFT)
		return 2;
	else
		return 3;
}

void TouchConfigView::triggerPosInit()
{
	static const char *str[] =
	{
		"Inline", "On Main", "On D-Pad", "Split"
	};
	int init = triggerOptionToVal(optionTouchCtrlTriggerBtnPos);
	triggerPos.init(str, init, sizeofArray(str), 0);
}

static int btnPosOptionToVal(_2DOrigin o)
{
	if(o == NULL2DO) return 8;
	else if(o == LT2DO) return 0;
	else if(o == LC2DO) return 1;
	else if(o == LB2DO) return 2;
	else if(o == RT2DO) return 3;
	else if(o == RC2DO) return 4;
	else if(o == RB2DO) return 5;
	else if(o == CT2DO) return 6;
	else if(o == CB2DO) return 7;
	else return 2;
}

static _2DOrigin btnPosValTo2DO(int val)
{
	if(val == 0) return LT2DO;
	else if(val == 1) return LC2DO;
	else if(val == 2) return LB2DO;
	else if(val == 3) return RT2DO;
	else if(val == 4) return RC2DO;
	else if(val == 5) return RB2DO;
	else if(val == 6) return CT2DO;
	else if(val == 7) return CB2DO;
	else return LB2DO;
}

void TouchConfigView::dPadPosInit()
{
	int init = btnPosOptionToVal(optionTouchCtrlDpadPos);
	dPadPos.init(ctrlPosStr, init, sizeofArray(ctrlPosStr), 0);
}

void TouchConfigView::faceBtnPosInit()
{
	int init = btnPosOptionToVal(optionTouchCtrlFaceBtnPos);
	faceBtnPos.init(ctrlPosStr, init, sizeofArray(ctrlPosStr), 0);
}

static int centerBtnOptionToVal(_2DOrigin o)
{
	if(o == LT2DO) return 0;
	else if(o == CT2DO) return 1;
	else if(o == RT2DO) return 2;
	else if(o == LB2DO) return 3;
	else if(o == CB2DO) return 4;
	else if(o == RB2DO) return 5;
	else return 4;
}

void TouchConfigView::centerBtnPosInit()
{
	static const char *str[] =
	{
		"Top-Left", "Top", "Top-Right", "Bottom-Left", "Bottom", "Bottom-Right"
	};
	int init = centerBtnOptionToVal(optionTouchCtrlCenterBtnPos);
	centerBtnPos.init(str, init, sizeofArray(str), 0);
}

void TouchConfigView::menuPosInit()
{
	int init = btnPosOptionToVal(optionTouchCtrlMenuPos);
	int vals = sizeofArray(ctrlPosStr);
	if(Config::envIsIOS)
		vals--; // prevent iOS port from disabling menu hot-spot
	menuPos.init(ctrlPosStr, init, vals, 0);
}

void TouchConfigView::ffPosInit()
{
	int init = btnPosOptionToVal(optionTouchCtrlFFPos);
	ffPos.init(ctrlPosStr, init, sizeofArray(ctrlPosStr), 0);
}

static uint alphaToMenuOption(int alpha)
{
	if(alpha == 0)
		return 0;
	else if(alpha == int(255 * .1))
		return 1;
	else if(alpha == int(255 * .25))
		return 2;
	else if(alpha == int(255 * .50))
		return 3;
	else if(alpha == int(255 * .65))
		return 4;
	else
		return 5;
}

void TouchConfigView::alphaInit()
{
	static const char *str[] =
	{
		"0%", "10%", "25%", "50%", "65%", "75%"
	};
	alpha.init(str, alphaToMenuOption(optionTouchCtrlAlpha), sizeofArray(str), 0);
}

void TouchConfigView::sizeInit()
{
	static const char *str[] =
	{
		"6.5mm", "7mm", "7.5mm", "8mm", "8.5mm", "9mm", "10mm", "12mm", "14mm"
	};
	int init = 0;
	if(optionTouchCtrlSize == 650U)
		init = 0;
	else if(optionTouchCtrlSize == 700U)
		init = 1;
	else if(optionTouchCtrlSize == 750U)
		init = 2;
	else if(optionTouchCtrlSize == 800U)
		init = 3;
	else if(optionTouchCtrlSize == 850U)
		init = 4;
	else if(optionTouchCtrlSize == 900U)
		init = 5;
	else if(optionTouchCtrlSize == 1000U)
		init = 6;
	else if(optionTouchCtrlSize == 1200U)
		init = 7;
	else if(optionTouchCtrlSize == 1400U)
		init = 8;
	size.init(str, init, sizeofArray(str), 0);
}

void TouchConfigView::deadzoneInit()
{
	static const char *str[] =
	{
		"1mm", "1.35mm", "1.6mm"
	};
	int init = 0;
	if(optionTouchDpadDeadzone == 100U)
		init = 0;
	else if(optionTouchDpadDeadzone == 135U)
		init = 1;
	else if(optionTouchDpadDeadzone == 160U)
		init = 2;
	deadzone.init(str, init, sizeofArray(str), 0);
}

void TouchConfigView::diagonalSensitivityInit()
{
	static const char *str[] =
	{
		"None", "Low", "M-Low","Med.", "High"
	};
	int init = 0;
	if(optionTouchDpadDiagonalSensitivity == 1500U)
		init = 1;
	else if(optionTouchDpadDiagonalSensitivity == 1750U)
		init = 2;
	else if(optionTouchDpadDiagonalSensitivity == 2000U)
		init = 3;
	else if(optionTouchDpadDiagonalSensitivity == 2500U)
		init = 4;
	diagonalSensitivity.init(str, init, sizeofArray(str), 0);
}

void TouchConfigView::btnSpaceInit()
{
	static const char *str[] =
	{
		"1mm", "2mm", "3mm", "4mm"
	};
	int init = 0;
	if(optionTouchCtrlBtnSpace == 100U)
		init = 0;
	else if(optionTouchCtrlBtnSpace == 200U)
		init = 1;
	else if(optionTouchCtrlBtnSpace == 300U)
		init = 2;
	else if(optionTouchCtrlBtnSpace == 400U)
		init = 3;
	btnSpace.init(str, init, sizeofArray(str), 0);
}

void TouchConfigView::btnExtraXSizeInit()
{
	static const char *str[] =
	{
		"None", "Gap only", "10%", "25%"
	};
	int init = 0;
	switch((int)optionTouchCtrlExtraXBtnSize)
	{
		bcase 1: init = 1;
		bcase 200: init = 2;
		bcase 500: init = 3;
	}
	btnExtraXSize.init(str, init, sizeofArray(str), 0);
}

void TouchConfigView::btnExtraYSizeMultiRowInit()
{
	static const char *str[] =
	{
		"None", "Gap only", "10%", "25%"
	};
	int init = 0;
	switch((int)optionTouchCtrlExtraYBtnSizeMultiRow)
	{
		bcase 1: init = 1;
		bcase 200: init = 2;
		bcase 500: init = 3;
	}
	btnExtraYSizeMultiRow.init((systemFaceBtns == 4 || (systemFaceBtns >= 6 && systemHasTriggerBtns)) ? "V Overlap" : "V Overlap (2 rows)", str, init, sizeofArray(str), 0);
}

void TouchConfigView::btnExtraYSizeInit()
{
	static const char *str[] =
	{
		"None", "Gap only", "25%", "50%"
	};
	int init = 0;
	switch((int)optionTouchCtrlExtraYBtnSize)
	{
		bcase 1: init = 1;
		bcase 500: init = 2;
		bcase 1000: init = 3;
	}
	btnExtraYSize.init(str, init, sizeofArray(str), 0);
}

void TouchConfigView::btnStaggerInit()
{
	static const char *str[] =
	{
			"-0.75x V", "-0.5x V", "0", "0.5x V", "0.75x V", "1x H&V"
	};
	assert(optionTouchCtrlBtnStagger < sizeofArray(str));
	int init = optionTouchCtrlBtnStagger;
	btnStagger.init(str, init, sizeofArray(str), 0);
}

void TouchConfigView::init(bool highlightFirst)
{
	uint i = 0;
	//timeout.init(); text[i++] = &timeout;
	alphaInit(); text[i++] = &alpha;
	sizeInit(); text[i++] = &size;
	deadzoneInit(); text[i++] = &deadzone;
	diagonalSensitivityInit(); text[i++] = &diagonalSensitivity;
	menuPosInit(); text[i++] = &menuPos;
	showMenuIcon.init(optionShowMenuIcon); text[i++] = &showMenuIcon;
	ffPosInit(); text[i++] = &ffPos;
	dPadPosInit(); text[i++] = &dPadPos;
	faceBtnPosInit(); text[i++] = &faceBtnPos;
	centerBtnPosInit(); text[i++] = &centerBtnPos;
	if(vController.hasTriggers())
	{
		triggerPosInit(); text[i++] = &triggerPos;
	}
	btnSpaceInit(); text[i++] = &btnSpace;
	btnStaggerInit(); text[i++] = &btnStagger;
	btnExtraXSizeInit(); text[i++] = &btnExtraXSize;
	if(systemFaceBtns < 4 || (systemFaceBtns == 6 && !systemHasTriggerBtns))
	{
		btnExtraYSizeInit(); text[i++] = &btnExtraYSize;
	}
	if(systemFaceBtns >= 4)
	{
		btnExtraYSizeMultiRowInit(); text[i++] = &btnExtraYSizeMultiRow;
	}
	boundingBoxes.init(optionTouchCtrlBoundingBoxes); text[i++] = &boundingBoxes;
	if(!optionVibrateOnPush.isConst)
	{
		vibrate.init(optionVibrateOnPush); text[i++] = &vibrate;
	}
	showOnTouch.init(optionTouchCtrlShowOnTouch); text[i++] = &showOnTouch;
	if(!optionDPI.isConst) { dpiInit(); text[i++] = &dpi; }
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLLER_RESOLUTION_CHANGE
	if(!optionTouchCtrlImgRes.isConst)
	{
		imageResolution.init(optionTouchCtrlImgRes == 128U ? 1 : 0); text[i++] = &imageResolution;
	}
	#endif
	assert(i <= sizeofArray(text));
	BaseMenuView::init(text, i, highlightFirst);
}

void TouchConfigView::draw(Gfx::FrameTimeBase frameTime)
{
	using namespace Gfx;
	resetTransforms();
	vController.draw(.5);
	BaseMenuView::draw(frameTime);
}

void TouchConfigView::updatePositionVals()
{
	dPadPos.updateVal(btnPosOptionToVal(optionTouchCtrlDpadPos));
	faceBtnPos.updateVal(btnPosOptionToVal(optionTouchCtrlFaceBtnPos));
	centerBtnPos.updateVal(centerBtnOptionToVal(optionTouchCtrlCenterBtnPos));
	ffPos.updateVal(btnPosOptionToVal(optionTouchCtrlFFPos));
	menuPos.updateVal(btnPosOptionToVal(optionTouchCtrlMenuPos));
	if(vController.hasTriggers())
	{
		triggerPos.updateVal(triggerOptionToVal(optionTouchCtrlTriggerBtnPos));
	}
}

void TouchConfigView::dpiInit()
{
	static const char *str[] = { "Auto", "96", "120", "130", "160", "220", "240", "265", "320" };
	uint init = 0;
	switch(optionDPI)
	{
		bcase 96: init = 1;
		bcase 120: init = 2;
		bcase 130: init = 3;
		bcase 160: init = 4;
		bcase 220: init = 5;
		bcase 240: init = 6;
		bcase 265: init = 7;
		bcase 320: init = 8;
	}
	assert(init < sizeofArray(str));
	dpi.init(str, init, sizeofArray(str));
}

TouchConfigView::TouchConfigView(const char *faceBtnName, const char *centerBtnName):
	BaseMenuView("On-screen Config"),
	alpha
	{
		"Blend Amount",
		[](MultiChoiceMenuItem &, int val)
		{
			if(val == 0)
			{
				optionTouchCtrlAlpha = 0;
			}
			else if(val == 1)
			{
				optionTouchCtrlAlpha = 255 * .1;
			}
			else if(val == 2)
			{
				optionTouchCtrlAlpha = 255 * .25;
			}
			else if(val == 3)
			{
				optionTouchCtrlAlpha = 255 * .5;
			}
			else if(val == 4)
			{
				optionTouchCtrlAlpha = 255 * .65;
			}
			else
			{
				optionTouchCtrlAlpha = 255 * .75;
			}
			vController.alpha = (int)optionTouchCtrlAlpha / 255.0;
		}
	},
	size
	{
		"Button Size",
		[](MultiChoiceMenuItem &, int val)
		{
			if(val == 0)
				optionTouchCtrlSize.val = 650;
			else if(val == 1)
				optionTouchCtrlSize.val = 700;
			else if(val == 2)
				optionTouchCtrlSize.val = 750;
			else if(val == 3)
				optionTouchCtrlSize.val = 800;
			else if(val == 4)
				optionTouchCtrlSize.val = 850;
			else if(val == 5)
				optionTouchCtrlSize.val = 900;
			else if(val == 6)
				optionTouchCtrlSize.val = 1000;
			else if(val == 7)
				optionTouchCtrlSize.val = 1200;
			else if(val == 8)
				optionTouchCtrlSize.val = 1400;
			EmuControls::setupVControllerVars();
			vController.place();
		}
	},
	deadzone
	{
		"D-Pad Deadzone",
		[](MultiChoiceMenuItem &, int val)
		{
			if(val == 0)
				optionTouchDpadDeadzone.val = 100;
			else if(val == 1)
				optionTouchDpadDeadzone.val = 135;
			else if(val == 2)
				optionTouchDpadDeadzone.val = 160;
			EmuControls::setupVControllerVars();
			vController.place();
		}
	},
	diagonalSensitivity
	{
		"Diagonal Sensitivity",
		[](MultiChoiceMenuItem &, int val)
		{
			if(val == 0)
				optionTouchDpadDiagonalSensitivity.val = 1000;
			else if(val == 1)
				optionTouchDpadDiagonalSensitivity.val = 1500;
			else if(val == 2)
				optionTouchDpadDiagonalSensitivity.val = 1750;
			else if(val == 3)
				optionTouchDpadDiagonalSensitivity.val = 2000;
			else if(val == 4)
				optionTouchDpadDiagonalSensitivity.val = 2500;
			EmuControls::setupVControllerVars();
			vController.place();
		}
	},
	btnSpace
	{
		"Button Spacing",
		[](MultiChoiceMenuItem &, int val)
		{
			if(val == 0)
				optionTouchCtrlBtnSpace.val = 100;
			else if(val == 1)
				optionTouchCtrlBtnSpace.val = 200;
			else if(val == 2)
				optionTouchCtrlBtnSpace.val = 300;
			else if(val == 3)
				optionTouchCtrlBtnSpace.val = 400;
			EmuControls::setupVControllerVars();
			vController.place();
		}
	},
	btnExtraXSize
	{
		"H Overlap",
		[](MultiChoiceMenuItem &, int val)
		{
			switch(val)
			{
				bdefault: optionTouchCtrlExtraXBtnSize = 0;
				bcase 1: optionTouchCtrlExtraXBtnSize = 1;
				bcase 2: optionTouchCtrlExtraXBtnSize = 200;
				bcase 3: optionTouchCtrlExtraXBtnSize = 500;
			}
			EmuControls::setupVControllerVars();
			vController.place();
		}
	},
	btnExtraYSizeMultiRow
	{
		[](MultiChoiceMenuItem &, int val)
		{
			switch(val)
			{
				bdefault: optionTouchCtrlExtraYBtnSizeMultiRow = 0;
				bcase 1: optionTouchCtrlExtraYBtnSizeMultiRow = 1;
				bcase 2: optionTouchCtrlExtraYBtnSizeMultiRow = 200;
				bcase 3: optionTouchCtrlExtraYBtnSizeMultiRow = 500;
			}
			EmuControls::setupVControllerVars();
			vController.place();
		}
	},
	btnExtraYSize
	{
		"V Overlap",
		[](MultiChoiceMenuItem &, int val)
		{
			switch(val)
			{
				bdefault: optionTouchCtrlExtraYBtnSize = 0;
				bcase 1: optionTouchCtrlExtraYBtnSize = 1;
				bcase 2: optionTouchCtrlExtraYBtnSize = 500;
				bcase 3: optionTouchCtrlExtraYBtnSize = 1000;
			}
			EmuControls::setupVControllerVars();
			vController.place();
		}
	},
	triggerPos
	{
		"L/R Placement",
		[this](MultiChoiceMenuItem &item, int val)
		{
			if(val == 0)
				optionTouchCtrlTriggerBtnPos.val = TRIGGERS_INLINE;
			else if(val == 1)
				optionTouchCtrlTriggerBtnPos.val = TRIGGERS_RIGHT;
			else if(val == 2)
				optionTouchCtrlTriggerBtnPos.val = TRIGGERS_LEFT;
			else
				optionTouchCtrlTriggerBtnPos.val = TRIGGERS_SPLIT;
			if(vController.hasTriggers() && optionTouchCtrlDpadPos == NULL2DO && (optionTouchCtrlTriggerBtnPos == (uint)TRIGGERS_LEFT || optionTouchCtrlTriggerBtnPos == (uint)TRIGGERS_SPLIT))
				optionTouchCtrlTriggerBtnPos.val = TRIGGERS_RIGHT;
			vController.gp.triggerPos = optionTouchCtrlTriggerBtnPos;
			refreshTouchConfigMenu();
			vController.place();
		}
	},
	btnStagger
	{
		"Button Stagger",
		[](MultiChoiceMenuItem &, int val)
		{
			optionTouchCtrlBtnStagger.val = val;
			EmuControls::setupVControllerVars();
			vController.place();
		}
	},
	dPadPos
	{
		"D-Pad",
		[this](MultiChoiceMenuItem &item, int val)
		{
			if(val == 8)
				optionTouchCtrlDpadPos.val = NULL2DO;
			else
			{
				optionTouchCtrlDpadPos.val = btnPosValTo2DO(val);
				EmuControls::resolveOnScreenCollisions(&optionTouchCtrlDpadPos.val);
			}
			if(vController.hasTriggers() && optionTouchCtrlDpadPos == NULL2DO && (optionTouchCtrlTriggerBtnPos == (uint)TRIGGERS_LEFT || optionTouchCtrlTriggerBtnPos == (uint)TRIGGERS_SPLIT))
				optionTouchCtrlTriggerBtnPos.val = TRIGGERS_RIGHT;
			EmuControls::setupVControllerPosition();
			refreshTouchConfigMenu();
			vController.place();
		}
	},
	faceBtnPos
	{
		faceBtnName,
		[this](MultiChoiceMenuItem &item, int val)
		{
			if(val == 8)
			{
				optionTouchCtrlFaceBtnPos.val = NULL2DO;
				return;
			}

			optionTouchCtrlFaceBtnPos.val = btnPosValTo2DO(val);
			EmuControls::resolveOnScreenCollisions(&optionTouchCtrlFaceBtnPos.val);
			EmuControls::setupVControllerPosition();
			refreshTouchConfigMenu();
			vController.place();
		}
	},
	centerBtnPos
	{
		centerBtnName,
		[this](MultiChoiceMenuItem &item, int val)
		{
			if(val == 0)
				optionTouchCtrlCenterBtnPos.val = LT2DO;
			else if(val == 1)
				optionTouchCtrlCenterBtnPos.val = CT2DO;
			else if(val == 2)
				optionTouchCtrlCenterBtnPos.val = RT2DO;
			else if(val == 3)
				optionTouchCtrlCenterBtnPos.val = LB2DO;
			else if(val == 4)
				optionTouchCtrlCenterBtnPos.val = CB2DO;
			else if(val == 5)
				optionTouchCtrlCenterBtnPos.val = RB2DO;

			EmuControls::resolveOnScreenCollisions(&optionTouchCtrlCenterBtnPos.val);
			EmuControls::setupVControllerPosition();
			refreshTouchConfigMenu();
			vController.place();
		}
	},
	menuPos
	{
		"Open Menu",
		[this](MultiChoiceMenuItem &item, int val)
		{
			if(val == 8)
			{
				optionTouchCtrlMenuPos.val = NULL2DO;
				return;
			}

			optionTouchCtrlMenuPos.val = btnPosValTo2DO(val);
			EmuControls::resolveOnScreenCollisions(&optionTouchCtrlMenuPos.val);
			EmuControls::setupVControllerPosition();
			refreshTouchConfigMenu();
			vController.place();
		}
	},
	ffPos
	{
		"Fast-forward",
		[this](MultiChoiceMenuItem &item, int val)
		{
			if(val == 8)
			{
				optionTouchCtrlFFPos.val = NULL2DO;
				return;
			}

			optionTouchCtrlFFPos.val = btnPosValTo2DO(val);
			EmuControls::resolveOnScreenCollisions(&optionTouchCtrlFFPos.val);
			EmuControls::setupVControllerPosition();
			refreshTouchConfigMenu();
			vController.place();
		}
	},
	dpi
	{
		"DPI Override",
		[](MultiChoiceMenuItem &, int val)
		{
			switch(val)
			{
				bdefault: optionDPI.val = 0;
				bcase 1: optionDPI.val = 96;
				bcase 2: optionDPI.val = 120;
				bcase 3: optionDPI.val = 130;
				bcase 4: optionDPI.val = 160;
				bcase 5: optionDPI.val = 220;
				bcase 6: optionDPI.val = 240;
				bcase 7: optionDPI.val = 265;
				bcase 8: optionDPI.val = 320;
			}
			Base::setDPI(optionDPI);
			logMsg("set DPI: %d", (int)optionDPI);
			setupFont();
			Gfx::onViewChange(nullptr);
		}
	},
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLLER_RESOLUTION_CHANGE
	imageResolution
	{
		"High Resolution",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			uint newRes = item.on ? 128 : 64;
			if(optionTouchCtrlImgRes != newRes)
			{
				optionTouchCtrlImgRes = newRes;
				EmuControls::updateVControlImg();
				vController.place();
			}
		}
	},
	#endif
	boundingBoxes
	{
		"Show Bounding Boxes",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionTouchCtrlBoundingBoxes = item.on;
			EmuControls::setupVControllerVars();
			Base::displayNeedsUpdate();
		}
	},
	vibrate
	{
		"Vibration",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionVibrateOnPush = item.on;
		}
	},
	showMenuIcon
	{
		"Menu Icon",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionShowMenuIcon = item.on;
		}
	},
	showOnTouch
	{
		"Show Controls If Screen Touched",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionTouchCtrlShowOnTouch = item.on;
		}
	}
{}
