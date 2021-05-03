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
#include <imagine/util/typeTraits.hh>
#include <array>

class TextEntry
{
public:
	TextEntry(const char *initText, Gfx::Renderer &r, Gfx::GlyphTextureSet *face, const Gfx::ProjectionPlane &projP);
	void setAcceptingInput(bool on);
	bool isAcceptingInput() const;
	bool inputEvent(View &parentView, Input::Event e);
	void prepareDraw(Gfx::Renderer &r);
	void draw(Gfx::RendererCommands &cmds);
	void place(Gfx::Renderer &r);
	void place(Gfx::Renderer &r, IG::WindowRect rect, const Gfx::ProjectionPlane &projP);
	const char *textStr() const;
	IG::WindowRect bgRect() const;

protected:
	Gfx::Text t{};
	Gfx::ProjectionPlane projP{};
	IG::WindowRect b{};
	std::array<char, 128> str{};
	bool acceptingInput{};
	bool multiLine{};
};

class CollectTextInputView : public View
{
public:
	// returning non-zero keeps text entry active on Android
	using OnTextDelegate = DelegateFunc<uint32_t (CollectTextInputView &view, const char *str)>;

	CollectTextInputView(ViewAttachParams attach, const char *msgText, const char *initialContent,
		Gfx::TextureSpan closeRes, OnTextDelegate onText, Gfx::GlyphTextureSet *face = {});
	CollectTextInputView(ViewAttachParams attach, const char *msgText,
		Gfx::TextureSpan closeRes, OnTextDelegate onText, Gfx::GlyphTextureSet *face = {}):
		CollectTextInputView(attach, msgText, "", closeRes, onText, face) {}
	void place() override;
	bool inputEvent(Input::Event e) override;
	void prepareDraw() override;
	void draw(Gfx::RendererCommands &cmds) override;

protected:
	IG::WindowRect cancelBtn{};
	// TODO: cancel button doesn't work yet due to popup window not forwarding touch events to main window
	IG_enableMemberIf(!Config::envIsAndroid, Gfx::Sprite, cancelSpr){};
	Gfx::Text message{};
	[[no_unique_address]] Input::TextField textField;
	IG_enableMemberIf(!Config::Input::SYSTEM_COLLECTS_TEXT, TextEntry, textEntry);
	OnTextDelegate onTextD{};
};
