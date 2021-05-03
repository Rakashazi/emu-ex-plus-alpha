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

#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/util/string.h>
#include <imagine/logger/logger.h>

TextEntry::TextEntry(const char *initText, Gfx::Renderer &r, Gfx::GlyphTextureSet *face, const Gfx::ProjectionPlane &projP)
{
	string_copy(str, initText);
	t = {str.data(), face};
	t.compile(r, projP);
}

void TextEntry::setAcceptingInput(bool on)
{
	if(on)
	{
		logMsg("accepting input");
	}
	else
	{
		logMsg("stopped accepting input");
	}
	acceptingInput = on;
}

bool TextEntry::isAcceptingInput() const
{
	return acceptingInput;
}

bool TextEntry::inputEvent(View &parentView, Input::Event e)
{
	if(e.isPointer() && e.pushed() && b.overlaps(e.pos()))
	{
		setAcceptingInput(true);
		return true;
	}
	if(acceptingInput && e.pushed() && e.map() == Input::Map::SYSTEM)
	{
		bool updateText = false;

		if(e.mapKey() == Input::Keycode::BACK_SPACE)
		{
			int len = strlen(str.data());
			if(len > 0)
			{
				str[len-1] = '\0';
				updateText = true;
			}
		}
		else
		{
			auto keyStr = e.keyString(parentView.appContext());
			if(strlen(keyStr.data()))
			{
				if(!multiLine)
				{
					if(keyStr[0] == '\r' || keyStr[0] == '\n')
					{
						setAcceptingInput(false);
						return true;
					}
				}
				string_cat(str, keyStr.data());
				updateText = true;
			}
		}

		if(updateText)
		{
			{
				parentView.waitForDrawFinished();
				t.setString(str.data());
				t.compile(parentView.renderer(), projP);
			}
			parentView.postDraw();
		}
		return true;
	}
	return false;
}

void TextEntry::prepareDraw(Gfx::Renderer &r)
{
	t.makeGlyphs(r);
}

void TextEntry::draw(Gfx::RendererCommands &cmds)
{
	using namespace Gfx;
	cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
	t.draw(cmds, projP.unProjectRect(b).pos(LC2DO), LC2DO, projP);
}

void TextEntry::place(Gfx::Renderer &r)
{
	t.compile(r, projP);
}

void TextEntry::place(Gfx::Renderer &r, IG::WindowRect rect, const Gfx::ProjectionPlane &projP)
{
	this->projP = projP;
	b = rect;
	place(r);
}

const char *TextEntry::textStr() const
{
	return str.data();
}

IG::WindowRect TextEntry::bgRect() const
{
	return b;
}

CollectTextInputView::CollectTextInputView(ViewAttachParams attach, const char *msgText, const char *initialContent,
	Gfx::TextureSpan closeRes, OnTextDelegate onText, Gfx::GlyphTextureSet *face):
	View{attach},
	textField
	{
		attach.appContext(),
		[this](const char *str)
		{
			if(!str)
			{
				logMsg("text collection canceled by external source");
				dismiss();
				return;
			}
			if(onTextD(*this, str))
			{
				logMsg("text collection canceled by text delegate");
				dismiss();
			}
		},
		initialContent, msgText,
		face ? face->fontSettings().pixelHeight() : attach.viewManager().defaultFace().fontSettings().pixelHeight()
	},
	textEntry
	{
		initialContent,
		attach.renderer(),
		face ? face : &attach.viewManager().defaultFace(),
		projP
	},
	onTextD{onText}
{
	face = face ? face : &attach.viewManager().defaultFace();
	#ifndef CONFIG_BASE_ANDROID
	if(manager().needsBackControl() && closeRes)
	{
		cancelSpr = {{{-.5, -.5}, {.5, .5}}, closeRes};
		if(cancelSpr.compileDefaultProgram(Gfx::IMG_MODE_MODULATE))
			renderer().autoReleaseShaderCompiler();
	}
	#endif
	message = {msgText, face};
	[](auto &textEntry)
	{
		if constexpr(!Config::Input::SYSTEM_COLLECTS_TEXT)
		{
			textEntry.setAcceptingInput(true);
		}
	}(textEntry);
}

void CollectTextInputView::place()
{
	using namespace Gfx;
	auto &face = *message.face();
	#ifndef CONFIG_BASE_ANDROID
	if(cancelSpr.image())
	{
		cancelBtn.setPosRel(viewRect().pos(RT2DO), face.nominalHeight() * 1.75, RT2DO);
		cancelSpr.setPos(projP.unProjectRect(cancelBtn));
	}
	#endif
	message.setMaxLineSize(projP.width() * 0.95);
	message.compile(renderer(), projP);
	[&](auto &textEntry)
	{
		IG::WindowRect textRect;
		int xSize = viewRect().xSize() * 0.95;
		int ySize = face.nominalHeight() * (Config::envIsAndroid ? 2. : 1.5);
		if constexpr(!Config::Input::SYSTEM_COLLECTS_TEXT)
		{
			textRect.setPosRel(viewRect().pos(C2DO), {xSize, ySize}, C2DO);
			textEntry.place(renderer(), textRect, projP);
		}
		else
		{
			textRect.setPosRel(viewRect().pos(C2DO) - IG::WP{0, (int)viewRect().ySize()/4}, {xSize, ySize}, C2DO);
			textField.place(textRect);
		}
	}(textEntry);
}

bool CollectTextInputView::inputEvent(Input::Event e)
{
	if(e.state() == Input::Action::PUSHED)
	{
		if(e.isDefaultCancelButton() || (e.isPointer() && cancelBtn.overlaps(e.pos())))
		{
			dismiss();
			return true;
		}
	}
	return [&](auto &textEntry)
		{
			if constexpr(!Config::Input::SYSTEM_COLLECTS_TEXT)
			{
				bool acceptingInput = textEntry.isAcceptingInput();
				bool handled = textEntry.inputEvent(*this, e);
				if(!textEntry.isAcceptingInput() && acceptingInput)
				{
					logMsg("calling on-text delegate");
					if(onTextD.callCopy(*this, textEntry.textStr()))
					{
						textEntry.setAcceptingInput(1);
					}
				}
				return handled;
			}
			else
			{
				return false;
			}
		}(textEntry);
}

void CollectTextInputView::prepareDraw()
{
	message.makeGlyphs(renderer());
	[this](auto &textEntry)
	{
		if constexpr(!Config::Input::SYSTEM_COLLECTS_TEXT)
		{
			textEntry.prepareDraw(renderer());
		}
	}(textEntry);
}

void CollectTextInputView::draw(Gfx::RendererCommands &cmds)
{
	using namespace Gfx;
	#ifndef CONFIG_BASE_ANDROID
	if(cancelSpr.image())
	{
		cmds.set(ColorName::WHITE);
		cmds.setBlendMode(BLEND_MODE_ALPHA);
		cmds.set(imageCommonTextureSampler);
		cancelSpr.setCommonProgram(cmds, IMG_MODE_MODULATE, projP.makeTranslate());
		cancelSpr.draw(cmds);
	}
	#endif
	[&](auto &textEntry)
	{
		if constexpr(!Config::Input::SYSTEM_COLLECTS_TEXT)
		{
			cmds.setColor(0.25);
			cmds.setCommonProgram(CommonProgram::NO_TEX, projP.makeTranslate());
			GeomRect::draw(cmds, textEntry.bgRect(), projP);
			cmds.set(ColorName::WHITE);
			textEntry.draw(cmds);
			cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
			message.draw(cmds, 0, projP.unprojectY(textEntry.bgRect().pos(C2DO).y) + message.nominalHeight(), CB2DO, projP);
		}
		else
		{
			cmds.set(ColorName::WHITE);
			cmds.setCommonProgram(CommonProgram::TEX_ALPHA, projP.makeTranslate());
			message.draw(cmds, 0, projP.unprojectY(textField.windowRect().pos(C2DO).y) + message.nominalHeight(), CB2DO, projP);
		}
	}(textEntry);
}
