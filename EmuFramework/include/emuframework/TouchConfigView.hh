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

#include <imagine/gui/TableView.hh>
#include <emuframework/MultiChoiceView.hh>
#include <emuframework/EmuOptions.hh>

class TouchConfigView : public TableView
{
public:
	#ifdef CONFIG_VCONTROLS_GAMEPAD
		#ifdef CONFIG_ENV_WEBOS
		BoolMenuItem touchCtrl;
		#else
		MultiChoiceSelectMenuItem touchCtrl;
		#endif
	MultiChoiceSelectMenuItem pointerInput;
	MultiChoiceSelectMenuItem size;
	MultiChoiceSelectMenuItem deadzone;
	MultiChoiceSelectMenuItem diagonalSensitivity;
	MultiChoiceSelectMenuItem btnSpace;
	MultiChoiceSelectMenuItem btnExtraXSize;
	MultiChoiceSelectMenuItem btnExtraYSizeMultiRow;
	MultiChoiceSelectMenuItem btnExtraYSize;
	BoolMenuItem triggerPos;
	MultiChoiceSelectMenuItem btnStagger;
	MultiChoiceSelectMenuItem dPadState;
	MultiChoiceSelectMenuItem faceBtnState;
	MultiChoiceSelectMenuItem centerBtnState;
	MultiChoiceSelectMenuItem lTriggerState;
	MultiChoiceSelectMenuItem rTriggerState;
		#ifdef CONFIG_EMUFRAMEWORK_VCONTROLLER_RESOLUTION_CHANGE
		BoolMenuItem imageResolution;
		#endif
	BoolMenuItem boundingBoxes;
	BoolMenuItem vibrate;
		#ifdef CONFIG_BASE_ANDROID
		BoolMenuItem useScaledCoordinates;
		#endif
	BoolMenuItem showOnTouch;
	#endif
	MultiChoiceSelectMenuItem alpha;
	TextMenuItem btnPlace;
	MultiChoiceSelectMenuItem menuState;
	MultiChoiceSelectMenuItem ffState;
	TextMenuItem resetControls;
	TextMenuItem resetAllControls;
	TextMenuItem systemOptions;
	TextHeadingMenuItem btnTogglesHeading;
	TextHeadingMenuItem dpadtHeading;
	TextHeadingMenuItem faceBtnHeading;
	TextHeadingMenuItem otherHeading;

	MenuItem *text[32]{};

	void refreshTouchConfigMenu();

public:
	TouchConfigView(Base::Window &win, const char *faceBtnName, const char *centerBtnName);
	void init(bool highlightFirst);
	void place() override;
	void draw() override;
};
