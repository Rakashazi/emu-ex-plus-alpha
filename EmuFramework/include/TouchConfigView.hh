/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#pragma once

#include <util/gui/BaseMenuView.hh>
#include <MultiChoiceView.hh>
#include <EmuOptions.hh>
#include <VController.hh>

#ifdef INPUT_SUPPORTS_POINTER

void refreshTouchConfigMenu();
void resolveOnScreenCollisions(_2DOrigin *movedObj);
void setupVControllerPosition();
void setupVControllerVars();
void updateVControlImg();

static const char *ctrlPosStr[] =
{
	"Top-Left", "Mid-Left", "Bottom-Left", "Top-Right", "Mid-Right", "Bottom-Right", "Top", "Bottom", "Off"
};

class TouchConfigView : public BaseMenuView
{
	MultiChoiceSelectMenuItem alpha;
	void alphaInit();

	MultiChoiceSelectMenuItem size;
	void sizeInit();

	MultiChoiceSelectMenuItem deadzone;
	void deadzoneInit();

	MultiChoiceSelectMenuItem diagonalSensitivity;
	void diagonalSensitivityInit();

	MultiChoiceSelectMenuItem btnSpace;
	void btnSpaceInit();

	MultiChoiceSelectMenuItem btnExtraXSize;
	void btnExtraXSizeInit();

	MultiChoiceSelectMenuItem btnExtraYSizeMultiRow;
	void btnExtraYSizeMultiRowInit();

	MultiChoiceSelectMenuItem btnExtraYSize;
	void btnExtraYSizeInit();

	struct TriggerPosMenuItem : public MultiChoiceSelectMenuItem
	{
		constexpr TriggerPosMenuItem() { }
		int optionToVal(uint o)
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

		void init()
		{
			static const char *str[] =
			{
				"Inline", "On Main", "On D-Pad", "Split"
			};
			int init = optionToVal(optionTouchCtrlTriggerBtnPos);
			MultiChoiceSelectMenuItem::init("L/R Placement", str, init, sizeofArray(str), 0);
		}

		void updateVal()
		{
			MultiChoiceMenuItem::updateVal(optionToVal(optionTouchCtrlTriggerBtnPos));
		}

		void doSet(int val)
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
	} triggerPos;

	MultiChoiceSelectMenuItem btnStagger;
	void btnStaggerInit();

	struct MainBtnPosMenuItem : public MultiChoiceSelectMenuItem
	{
		constexpr MainBtnPosMenuItem() { }
		int optionToVal(_2DOrigin o)
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

		_2DOrigin valTo2DO(int val)
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
	};

	struct DPadPosMenuItem : public MainBtnPosMenuItem
	{
		constexpr DPadPosMenuItem() { }
		void updateVal()
		{
			MultiChoiceMenuItem::updateVal(optionToVal(optionTouchCtrlDpadPos));
		}

		void init()
		{
			int init = optionToVal(optionTouchCtrlDpadPos);
			MultiChoiceSelectMenuItem::init("D-Pad", ctrlPosStr, init, sizeofArray(ctrlPosStr), 0);
		}

		void doSet(int val)
		{
			if(val == 8)
				optionTouchCtrlDpadPos.val = NULL2DO;
			else
			{
				optionTouchCtrlDpadPos.val = valTo2DO(val);
				resolveOnScreenCollisions(&optionTouchCtrlDpadPos.val);
			}
			if(vController.hasTriggers() && optionTouchCtrlDpadPos == NULL2DO && (optionTouchCtrlTriggerBtnPos == (uint)TRIGGERS_LEFT || optionTouchCtrlTriggerBtnPos == (uint)TRIGGERS_SPLIT))
				optionTouchCtrlTriggerBtnPos.val = TRIGGERS_RIGHT;
			setupVControllerPosition();
			refreshTouchConfigMenu();
			vController.place();
		}
	} dPadPos;

	struct FaceBtnPosMenuItem : public MainBtnPosMenuItem
	{
		constexpr FaceBtnPosMenuItem() { }
		void updateVal()
		{
			MultiChoiceMenuItem::updateVal(optionToVal(optionTouchCtrlFaceBtnPos));
		}

		void init(const char *faceBtnName)
		{
			int init = optionToVal(optionTouchCtrlFaceBtnPos);
			MultiChoiceSelectMenuItem::init(faceBtnName, ctrlPosStr, init, sizeofArray(ctrlPosStr), 0);
		}

		void doSet(int val)
		{
			if(val == 8)
			{
				optionTouchCtrlFaceBtnPos.val = NULL2DO;
				return;
			}

			optionTouchCtrlFaceBtnPos.val = valTo2DO(val);
			resolveOnScreenCollisions(&optionTouchCtrlFaceBtnPos.val);
			setupVControllerPosition();
			refreshTouchConfigMenu();
			vController.place();
		}
	} faceBtnPos;

	struct CenterBtnPosMenuItem : public MultiChoiceSelectMenuItem
	{
		constexpr CenterBtnPosMenuItem() { }
		int optionToVal(_2DOrigin o)
		{
			if(o == LT2DO) return 0;
			else if(o == CT2DO) return 1;
			else if(o == RT2DO) return 2;
			else if(o == LB2DO) return 3;
			else if(o == CB2DO) return 4;
			else if(o == RB2DO) return 5;
			else return 4;
		}

		void updateVal()
		{
			MultiChoiceMenuItem::updateVal(optionToVal(optionTouchCtrlCenterBtnPos));
		}

		void init(const char *centerBtnName)
		{
			static const char *str[] =
			{
				"Top-Left", "Top", "Top-Right", "Bottom-Left", "Bottom", "Bottom-Right"
			};
			int init = optionToVal(optionTouchCtrlCenterBtnPos);
			MultiChoiceSelectMenuItem::init(centerBtnName, str, init, sizeofArray(str), 0);
		}

		void doSet(int val)
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

			resolveOnScreenCollisions(&optionTouchCtrlCenterBtnPos.val);
			setupVControllerPosition();
			refreshTouchConfigMenu();
			vController.place();
		}
	} centerBtnPos;

	struct MenuPosMenuItem : public MainBtnPosMenuItem
	{
		constexpr MenuPosMenuItem() { }
		void updateVal()
		{
			MultiChoiceMenuItem::updateVal(optionToVal(optionTouchCtrlMenuPos));
		}

		void init()
		{
			int init = optionToVal(optionTouchCtrlMenuPos);
			int vals = sizeofArray(ctrlPosStr);
			if(Config::envIsIOS)
				vals--; // prevent iOS port from disabling menu hot-spot
			MultiChoiceSelectMenuItem::init("Open Menu", ctrlPosStr, init, vals, 0);
		}

		void doSet(int val)
		{
			if(val == 8)
			{
				optionTouchCtrlMenuPos.val = NULL2DO;
				return;
			}

			optionTouchCtrlMenuPos.val = valTo2DO(val);
			resolveOnScreenCollisions(&optionTouchCtrlMenuPos.val);
			setupVControllerPosition();
			refreshTouchConfigMenu();
			vController.place();
		}
	} menuPos;

	struct FFPosMenuItem : public MainBtnPosMenuItem
	{
		constexpr FFPosMenuItem() { }
		void updateVal()
		{
			MultiChoiceMenuItem::updateVal(optionToVal(optionTouchCtrlFFPos));
		}

		void init()
		{
			int init = optionToVal(optionTouchCtrlFFPos);
			MultiChoiceSelectMenuItem::init("Fast-forward", ctrlPosStr, init, sizeofArray(ctrlPosStr), 0);
		}

		void doSet(int val)
		{
			if(val == 8)
			{
				optionTouchCtrlFFPos.val = NULL2DO;
				return;
			}

			optionTouchCtrlFFPos.val = valTo2DO(val);
			resolveOnScreenCollisions(&optionTouchCtrlFFPos.val);
			setupVControllerPosition();
			refreshTouchConfigMenu();
			vController.place();
		}
	} ffPos;

	BoolMenuItem imageResolution {"High Resolution"};

	BoolMenuItem boundingBoxes {"Show Bounding Boxes"};

	BoolMenuItem vibrate {"Vibration"};

	BoolMenuItem showMenuIcon {"Menu Icon"};

	MultiChoiceSelectMenuItem dpi {"DPI Override"};
	void dpiInit();
	static void dpiSet(MultiChoiceMenuItem &, int val);

	MenuItem *text[20] = {nullptr};
public:
	constexpr TouchConfigView(const char *faceBtnName, const char *centerBtnName) : BaseMenuView("On-screen Config"),
		faceBtnName(faceBtnName), centerBtnName(centerBtnName) { }
	const char *faceBtnName, *centerBtnName;

	void init(bool highlightFirst);
	void draw(Gfx::FrameTimeBase frameTime);
	void updatePositionVals();
};

#endif
