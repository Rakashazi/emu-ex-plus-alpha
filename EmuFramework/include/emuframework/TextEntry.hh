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

#include <imagine/gfx/GfxText.hh>
#include <imagine/gui/View.hh>

class TextEntry
{
public:
	IG::WindowRect b;
	Gfx::Text t;
	char str[128] {0};
	bool acceptingInput = 0;
	bool multiLine = 0;
	Gfx::ProjectionPlane projP;

	constexpr TextEntry() {}
	CallResult init(const char *initText, ResourceFace *face, const Gfx::ProjectionPlane &projP);
	void deinit();
	void setAcceptingInput(bool on);
	void inputEvent(const Input::Event &e);
	void draw();
	void place();
	void place(IG::WindowRect rect, const Gfx::ProjectionPlane &projP);
};

class CollectTextInputView : public View
{
public:
	IG::WindowRect rect;
	IG::WindowRect cancelBtn;
	#ifndef CONFIG_BASE_ANDROID // TODO: cancel button doesn't work yet due to popup window not forwarding touch events to main window
	Gfx::Sprite cancelSpr;
	#endif
	Gfx::Text message;
	#ifndef CONFIG_INPUT_SYSTEM_COLLECTS_TEXT
	TextEntry textEntry;
	#endif

	// returning non-zero keeps text entry active on Android
	typedef DelegateFunc<uint (CollectTextInputView &view, const char *str)> OnTextDelegate;
	OnTextDelegate onTextD;
	OnTextDelegate &onText() { return onTextD; }

	CollectTextInputView(Base::Window &win): View("Text Entry", win) {}
	void init(const char *msgText, const char *initialContent, Gfx::PixmapTexture *closeRes, ResourceFace *face = View::defaultFace);
	void init(const char *msgText, Gfx::PixmapTexture *closeRes, ResourceFace *face = View::defaultFace)
	{
		init(msgText, "", closeRes, face);
	}
	void deinit() override;
	IG::WindowRect &viewRect() override { return rect; }
	void place() override;
	void inputEvent(const Input::Event &e) override;
	void draw() override;
};
