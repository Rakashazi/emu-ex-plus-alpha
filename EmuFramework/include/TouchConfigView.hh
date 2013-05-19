#pragma once

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

#include <util/gui/BaseMenuView.hh>
#include <MultiChoiceView.hh>
#include <EmuOptions.hh>
#include <EmuInput.hh>
#include <VController.hh>

class TouchConfigView : public BaseMenuView
{
public:
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
	MultiChoiceSelectMenuItem triggerPos;
	void triggerPosInit();
	MultiChoiceSelectMenuItem btnStagger;
	void btnStaggerInit();
	MultiChoiceSelectMenuItem dPadPos;
	void dPadPosInit();
	MultiChoiceSelectMenuItem faceBtnPos;
	void faceBtnPosInit();
	MultiChoiceSelectMenuItem centerBtnPos;
	void centerBtnPosInit();
	MultiChoiceSelectMenuItem menuPos;
	void menuPosInit();
	MultiChoiceSelectMenuItem ffPos;
	void ffPosInit();
	MultiChoiceSelectMenuItem dpi;
	void dpiInit();
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLLER_RESOLUTION_CHANGE
	BoolMenuItem imageResolution;
	#endif
	BoolMenuItem boundingBoxes;
	BoolMenuItem vibrate;
	BoolMenuItem showMenuIcon;
	BoolMenuItem showOnTouch;

	MenuItem *text[20] = {nullptr};

	void updatePositionVals();
	void refreshTouchConfigMenu()
	{
		updatePositionVals();
	}

public:
	TouchConfigView(const char *faceBtnName, const char *centerBtnName);

	void init(bool highlightFirst);
	void draw(Gfx::FrameTimeBase frameTime) override;
};
