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

#include <gfx/GfxText.hh>
#include <gui/View.hh>

class TextEntry
{
public:
	constexpr TextEntry() { }
	Rect2<int> b;
	Gfx::Text t;
	char str[128] = {0};
	bool acceptingInput = 0;
	bool multiLine = 0;

	CallResult init(const char *initText, ResourceFace *face);
	void deinit();
	void setAcceptingInput(bool on);
	void inputEvent(const Input::Event &e);
	void draw();
	void place();
	void place(Rect2<int> rect);
};

class CollectTextInputView : public View
{
public:
	constexpr CollectTextInputView(): View("Text Entry") { }

	Rect2<int> cancelBtn;
	#ifndef CONFIG_BASE_ANDROID // TODO: cancel button doesn't work yet due to popup window not forwarding touch events to main window
	Gfx::Sprite cancelSpr;
	#endif
	Gfx::Text message;
	#ifndef CONFIG_INPUT_SYSTEM_CAN_COLLECT_TEXT
	TextEntry textEntry;
	#endif

	// returning non-zero keeps text entry active on Android
	typedef DelegateFunc<uint (const char *str)> OnTextDelegate;
	OnTextDelegate onTextD;
	OnTextDelegate &onText() { return onTextD; }

	Rect2<int> rect;
	Rect2<int> &viewRect() override { return rect; }

	void init(const char *msgText, const char *initialContent = "",  ResourceFace *face = View::defaultFace);
	void deinit() override;
	void place() override;
	void inputEvent(const Input::Event &e) override;
	void draw(Gfx::FrameTimeBase frameTime) override;
};
