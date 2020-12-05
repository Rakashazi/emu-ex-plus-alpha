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

class VController;
class EmuVideoLayer;

class EmuInputView : public View
{
public:
	EmuInputView();
	EmuInputView(ViewAttachParams attach, VController &vCtrl, EmuVideoLayer &videoLayer);
	void place() final;
	void draw(Gfx::RendererCommands &cmds) final;
	bool inputEvent(Input::Event e) final;
	void resetInput();
	void setTouchControlsOn(bool on);
	bool touchControlsAreOn() const;
	VController *activeVController() const { return vController; }

private:
	VController *vController;
	EmuVideoLayer *videoLayer;
	bool touchControlsOn = false;
	bool ffToggleActive = false;

	void updateFastforward();
};
