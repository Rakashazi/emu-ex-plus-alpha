#pragma once

/*  This file is part of Imagine.

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
#include <imagine/gfx/GfxSprite.hh>
#include <imagine/gui/View.hh>

class TextEntry
{
public:
	Gfx::Text t{};
	Gfx::ProjectionPlane projP;
	IG::WindowRect b{};
	char str[128]{};
	bool acceptingInput = false;
	bool multiLine = false;

	TextEntry(const char *initText, Gfx::Renderer &r, Gfx::GlyphTextureSet *face, const Gfx::ProjectionPlane &projP);
	void setAcceptingInput(bool on);
	bool inputEvent(View &parentView, Input::Event e);
	void prepareDraw(Gfx::Renderer &r);
	void draw(Gfx::RendererCommands &cmds);
	void place(Gfx::Renderer &r);
	void place(Gfx::Renderer &r, IG::WindowRect rect, const Gfx::ProjectionPlane &projP);
};

class CollectTextInputView : public View
{
public:
	// returning non-zero keeps text entry active on Android
	using OnTextDelegate = DelegateFunc<uint32_t (CollectTextInputView &view, const char *str)>;

	CollectTextInputView(ViewAttachParams attach, const char *msgText, const char *initialContent,
		Gfx::TextureSpan closeRes, OnTextDelegate onText, Gfx::GlyphTextureSet *face = &View::defaultFace);
	CollectTextInputView(ViewAttachParams attach, const char *msgText,
		Gfx::TextureSpan closeRes, OnTextDelegate onText, Gfx::GlyphTextureSet *face = &View::defaultFace):
		CollectTextInputView(attach, msgText, "", closeRes, onText, face) {}
	~CollectTextInputView() override;
	void place() override;
	bool inputEvent(Input::Event e) override;
	void prepareDraw() override;
	void draw(Gfx::RendererCommands &cmds) override;

protected:
	IG::WindowRect cancelBtn{};
	#ifndef CONFIG_BASE_ANDROID // TODO: cancel button doesn't work yet due to popup window not forwarding touch events to main window
	Gfx::Sprite cancelSpr{};
	#endif
	Gfx::Text message{};
	#ifndef CONFIG_INPUT_SYSTEM_COLLECTS_TEXT
	TextEntry textEntry;
	#endif
	OnTextDelegate onTextD{};
};
