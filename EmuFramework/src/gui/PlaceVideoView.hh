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

#include <imagine/input/DragTracker.hh>
#include <imagine/gui/View.hh>
#include <imagine/gfx/GfxText.hh>
#include <imagine/gfx/Quads.hh>
#include <emuframework/EmuAppHelper.hh>

namespace EmuEx
{

class EmuVideoLayer;
class VController;

using namespace IG;

class PlaceVideoView final: public View, public EmuAppHelper
{
public:
	PlaceVideoView(ViewAttachParams, EmuVideoLayer &, VController &);
	~PlaceVideoView() final;
	void place() final;
	bool inputEvent(const Input::Event& e, ViewInputEventParams p = {}) final;
	void draw(Gfx::RendererCommands&__restrict__, ViewDrawParams p = {}) const final;
	void onShow() final;

private:
	EmuVideoLayer &layer;
	VController &vController;
	Gfx::Text exitText, resetText;
	Gfx::IQuads quads;
	WRect exitBounds{}, resetBounds{};
	Input::DragTrackerState dragState;
	int startPosOffset{};
	int posOffsetLimit{};

	void updateVideo(int offset);
};

}
