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

#include <emuframework/EmuAppHelper.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/util/container/ArrayList.hh>
#include <emuframework/config.hh>

namespace EmuEx
{

using namespace IG;
class VController;
enum class VControllerState : uint8_t;
enum class VControllerVisibility : uint8_t;

class TouchConfigView final: public TableView, public EmuAppHelper<TouchConfigView>
{
public:
	TouchConfigView(ViewAttachParams attach, VController &vController, UTF16String faceBtnName, UTF16String centerBtnName);
	void place() final;
	void draw(Gfx::RendererCommands &__restrict__) final;

protected:
	VController *vControllerPtr{};
	TextMenuItem touchCtrlItem[3];
	MultiChoiceMenuItem touchCtrl;
	TextMenuItem pointerInputItem[5];
	MultiChoiceMenuItem pointerInput;
	TextMenuItem sizeItem[11];
	MultiChoiceMenuItem size;
	TextMenuItem deadzoneItem[3];
	MultiChoiceMenuItem deadzone;
	TextMenuItem diagonalSensitivityItem[5];
	MultiChoiceMenuItem diagonalSensitivity;
	TextMenuItem btnSpaceItem[4];
	MultiChoiceMenuItem btnSpace;
	TextMenuItem btnExtraXSizeItem[4];
	MultiChoiceMenuItem btnExtraXSize;
	TextMenuItem btnExtraYSizeItem[4];
	MultiChoiceMenuItem btnExtraYSize;
	BoolMenuItem triggerPos;
	TextMenuItem btnStaggerItem[6];
	MultiChoiceMenuItem btnStagger;
	TextMenuItem dPadStateItem[3];
	MultiChoiceMenuItem dPadState;
	TextMenuItem faceBtnStateItem[3];
	MultiChoiceMenuItem faceBtnState;
	TextMenuItem centerBtnStateItem[3];
	MultiChoiceMenuItem centerBtnState;
	BoolMenuItem boundingBoxes;
	BoolMenuItem vibrate;
	BoolMenuItem showOnTouch;
	TextMenuItem alphaItem[6];
	MultiChoiceMenuItem alpha;
	TextMenuItem btnPlace;
	TextMenuItem menuStateItem[3];
	MultiChoiceMenuItem menuState;
	TextMenuItem ffStateItem[3];
	MultiChoiceMenuItem ffState;
	IG_UseMemberIf(Config::DISPLAY_CUTOUT, BoolMenuItem, allowButtonsPastContentBounds);
	TextMenuItem resetControls;
	TextMenuItem resetAllControls;
	TextHeadingMenuItem btnTogglesHeading;
	TextHeadingMenuItem dpadtHeading;
	TextHeadingMenuItem faceBtnHeading;
	TextHeadingMenuItem otherHeading;
	StaticArrayList<MenuItem*, 32> item;

	VController &vController() { return *vControllerPtr; }
	void refreshTouchConfigMenu();
	TextMenuItem::SelectDelegate setVisibilityDel(VControllerVisibility);
	TextMenuItem::SelectDelegate setSizeDel();
	TextMenuItem::SelectDelegate setPointerInputPlayerDel(int val);
	TextMenuItem::SelectDelegate setDeadzoneDel();
	TextMenuItem::SelectDelegate setDiagonalSensitivityDel();
	TextMenuItem::SelectDelegate setButtonSpaceDel();
	TextMenuItem::SelectDelegate setButtonExtraXSizeDel();
	TextMenuItem::SelectDelegate setButtonExtraYSizeDel();
	TextMenuItem::SelectDelegate setButtonStaggerDel(int val);
	TextMenuItem::SelectDelegate setButtonStateDel(VControllerState, uint8_t btnIdx);
	TextMenuItem::SelectDelegate setAlphaDel();
};

}
