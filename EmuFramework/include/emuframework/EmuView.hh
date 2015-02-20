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
#include <emuframework/EmuOptions.hh>
#include <emuframework/EmuVideoLayer.hh>
#include <emuframework/EmuInputView.hh>

class EmuView : public View
{
public:
	EmuVideoLayer *layer{};
	EmuInputView *inputView{};

private:
	IG::WindowRect rect;

public:
	EmuView(Base::Window &win): View(win) {}
	void deinit() override {}
	IG::WindowRect &viewRect() override { return rect; }
	void place() override;
	void draw() override;
	void inputEvent(const Input::Event &e) override;
};
