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

#include <imagine/gui/View.hh>
#include <imagine/gfx/GfxText.hh>
#include <imagine/gfx/Quads.hh>
#include <imagine/input/DragTracker.hh>
#include <emuframework/EmuAppHelper.hh>

namespace EmuEx
{

class VController;
class VControllerElement;

using namespace IG;

class PlaceVControlsView final: public View, public EmuAppHelper
{
public:
	PlaceVControlsView(ViewAttachParams attach, VController &vController);
	~PlaceVControlsView() final;
	void place() final;
	bool inputEvent(const Input::Event&, ViewInputEventParams p = {}) final;
	void draw(Gfx::RendererCommands &__restrict__ cmds, ViewDrawParams p = {}) const final;
	void onShow() final;

private:
	struct DragData
	{
		VControllerElement *elem{};
		WPt startPos{};
	};
	Gfx::Text exitText;
	Gfx::Text snapText;
	VController &vController;
	WRect exitBtnRect{};
	WRect snapBtnRect{};
	Input::DragTracker<DragData> dragTracker;
	size_t snapPxIdx{};
	Gfx::QuadIndexArray<uint16_t> gridIdxs;
	Gfx::IQuads quads;
};

}
