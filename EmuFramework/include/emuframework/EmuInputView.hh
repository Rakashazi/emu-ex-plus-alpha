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
#include <emuframework/EmuVideo.hh>

class EmuInputView : public View
{
public:
	bool ffKeyPushed = false, ffToggleActive = false;

private:
	IG::WindowRect rect{};

public:
	EmuInputView(ViewAttachParams attach): View(attach) {}
	IG::WindowRect &viewRect() override { return rect; }
	void place() override;
	void draw() override;
	bool inputEvent(Input::Event e) override;
	void onAddedToController(Input::Event e) override {}
	void resetInput();

private:
	void updateFastforward();
};
