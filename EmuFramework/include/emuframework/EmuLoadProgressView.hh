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

#include <emuframework/EmuApp.hh>
#include <imagine/base/Pipe.hh>
#include <array>

class EmuLoadProgressView : public View
{
private:
	Gfx::Text text{"Loading...", &View::defaultFace};
	IG::WindowRect rect;
	IG::WindowRect &viewRect() override { return rect; }
	uint pos = 0, max = 0;
	std::array<char, 128> str{};

public:
	EmuLoadProgressView(ViewAttachParams attach, Input::Event e, EmuApp::CreateSystemCompleteDelegate onComplete):
		View{attach}, originalEvent{e}, onComplete{onComplete} {}

	void setMax(uint val);
	void setPos(uint val);
	void setLabel(const char *str);
	void place() override;
	void inputEvent(Input::Event e) override {}
	void draw() override;
	void onAddedToController(Input::Event e) override {}

	// load context vars
	Base::Pipe pipe;
	Input::Event originalEvent{};
	EmuApp::CreateSystemCompleteDelegate onComplete{};
};
