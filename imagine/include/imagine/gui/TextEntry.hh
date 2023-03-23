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
#include <imagine/input/config.hh>
#include <imagine/input/TextField.hh>
#include <imagine/util/used.hh>
#include <imagine/util/string/StaticString.hh>
#include <array>

namespace IG
{

class TextEntry
{
public:
	TextEntry(const char *initText, Gfx::Renderer &r, Gfx::GlyphTextureSet *face);
	void setAcceptingInput(bool on);
	bool isAcceptingInput() const;
	bool inputEvent(View &parentView, const Input::Event &);
	void prepareDraw(Gfx::Renderer &r);
	void draw(Gfx::RendererCommands &__restrict__);
	void place(Gfx::Renderer &r);
	void place(Gfx::Renderer &r, WRect rect);
	const char *textStr() const;
	WRect bgRect() const;

protected:
	Gfx::Text t;
	WRect b;
	StaticString<128> str;
	bool acceptingInput{};
	bool multiLine{};
};

class CollectTextInputView : public View
{
public:
	// returning non-zero keeps text entry active on Android
	using OnTextDelegate = DelegateFunc<bool (CollectTextInputView &view, const char *str)>;

	CollectTextInputView(ViewAttachParams attach, CStringView msgText, CStringView initialContent,
		Gfx::TextureSpan closeRes, OnTextDelegate onText, Gfx::GlyphTextureSet *face = {});
	CollectTextInputView(ViewAttachParams attach, CStringView msgText,
		Gfx::TextureSpan closeRes, OnTextDelegate onText, Gfx::GlyphTextureSet *face = {}):
		CollectTextInputView(attach, msgText, "", closeRes, onText, face) {}
	void place() override;
	bool inputEvent(const Input::Event &) override;
	void prepareDraw() override;
	void draw(Gfx::RendererCommands &__restrict__) override;

protected:
	WRect cancelBtn;
	// TODO: cancel button doesn't work yet due to popup window not forwarding touch events to main window
	IG_UseMemberIf(!Config::envIsAndroid, Gfx::Sprite, cancelSpr);
	Gfx::Text message;
	[[no_unique_address]] Input::TextField textField;
	IG_UseMemberIf(!Config::Input::SYSTEM_COLLECTS_TEXT, TextEntry, textEntry);
	OnTextDelegate onTextD;
};

}
