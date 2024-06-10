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
#include <imagine/util/memory/DynArray.hh>
#include <emuframework/config.hh>
#include <vector>

namespace EmuEx
{

using namespace IG;
class VController;
enum class VControllerState : uint8_t;
enum class VControllerVisibility : uint8_t;

class TouchConfigView final: public TableView, public EmuAppHelper
{
public:
	TouchConfigView(ViewAttachParams attach, VController &vController);
	void place() final;
	void draw(Gfx::RendererCommands&__restrict__, ViewDrawParams p = {}) const final;
	void reloadItems();
	void onShow() final;

protected:
	VController &vController;
	TextMenuItem touchCtrlItem[3];
	MultiChoiceMenuItem touchCtrl;
	DynArray<TextMenuItem> playerItems;
	MultiChoiceMenuItem player;
	TextMenuItem sizeItem[11];
	MultiChoiceMenuItem size;
	BoolMenuItem vibrate;
	BoolMenuItem showOnTouch;
	BoolMenuItem highlightPushedButtons;
	TextMenuItem alphaItem[6];
	MultiChoiceMenuItem alpha;
	TextMenuItem btnPlace;
	TextMenuItem placeVideo;
	TextMenuItem addButton;
	ConditionalMember<Config::DISPLAY_CUTOUT, BoolMenuItem> allowButtonsPastContentBounds;
	TextMenuItem resetEmuPositions;
	TextMenuItem resetEmuGroups;
	TextMenuItem resetUIPositions;
	TextMenuItem resetUIGroups;
	TextHeadingMenuItem devButtonsHeading;
	TextHeadingMenuItem uiButtonsHeading;
	TextHeadingMenuItem otherHeading;
	std::vector<TextMenuItem> elementItems;
	std::vector<MenuItem*> item;

	void refreshTouchConfigMenu();
};

}
