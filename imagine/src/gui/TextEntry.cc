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
#include <imagine/logger/logger.h>
#include <imagine/gui/TableView.hh>

void TextEntry::setAcceptingInput(bool on)
{
	if(on)
	{
		Input::showSoftInput();
		logMsg("accepting input");
	}
	else
	{
		Input::hideSoftInput();
		logMsg("stopped accepting input");
	}
	acceptingInput = on;
}

bool TextEntry::inputEvent(Gfx::Renderer &r, Input::Event e)
{
	if(e.isPointer() && e.pushed() && b.overlaps(e.pos()))
	{
		setAcceptingInput(true);
		return true;
	}
	if(acceptingInput && e.pushed() && e.map() == e.MAP_SYSTEM)
	{
		bool updateText = false;

		if(e.mapKey() == Input::Keycode::BACK_SPACE)
		{
			int len = strlen(str);
			if(len > 0)
			{
				str[len-1] = '\0';
				updateText = true;
			}
		}
		else
		{
			auto keyStr = e.keyString();
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
			t.setString(str);
			t.compile(r, projP);
			Base::mainWindow().postDraw();
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
	t.draw(cmds, projP.unProjectRect(b).pos(LC2DO) + GP{TableView::globalXIndent, 0}, LC2DO, projP);
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

TextEntry::TextEntry(const char *initText, Gfx::Renderer &r, Gfx::GlyphTextureSet *face, const Gfx::ProjectionPlane &projP)
{
	string_copy(str, initText);
	t = {str, face};
	t.compile(r, projP);
}

CollectTextInputView::CollectTextInputView(ViewAttachParams attach, const char *msgText, const char *initialContent,
	Gfx::PixmapTexture *closeRes, OnTextDelegate onText, Gfx::GlyphTextureSet *face):
	View{attach},
	#ifndef CONFIG_INPUT_SYSTEM_COLLECTS_TEXT
	textEntry{initialContent, attach.renderer, face, projP},
	#endif
	onTextD{onText}
{
	#ifndef CONFIG_BASE_ANDROID
	if(View::needsBackControl && closeRes)
	{
		cancelSpr.init({-.5, -.5, .5, .5}, *closeRes);
		if(cancelSpr.compileDefaultProgram(Gfx::IMG_MODE_MODULATE))
			renderer().autoReleaseShaderCompiler();
	}
	#endif
	message = {msgText, face};
	#ifndef CONFIG_INPUT_SYSTEM_COLLECTS_TEXT
	textEntry.setAcceptingInput(true);
	#else
	Input::startSysTextInput(
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
		initialContent, msgText, face->settings.pixelHeight());
	#endif
}

CollectTextInputView::~CollectTextInputView()
{
	#ifdef CONFIG_INPUT_SYSTEM_COLLECTS_TEXT
	Input::cancelSysTextInput();
	#endif
}

void CollectTextInputView::place()
{
	using namespace Gfx;
	#ifndef CONFIG_BASE_ANDROID
	if(cancelSpr.image())
	{
		cancelBtn.setPosRel(rect.pos(RT2DO), View::defaultFace.nominalHeight() * 1.75, RT2DO);
		cancelSpr.setPos(-projP.unprojectXSize(cancelBtn)/3., -projP.unprojectYSize(cancelBtn)/3., projP.unprojectXSize(cancelBtn)/3., projP.unprojectYSize(cancelBtn)/3.);
	}
	#endif
	message.maxLineSize = projP.w * 0.95;
	message.compile(renderer(), projP);
	IG::WindowRect textRect;
	int xSize = rect.xSize() * 0.95;
	int ySize = View::defaultFace.nominalHeight()* (Config::envIsAndroid ? 2. : 1.5);
	#ifndef CONFIG_INPUT_SYSTEM_COLLECTS_TEXT
	textRect.setPosRel({rect.xPos(C2DO), rect.yPos(C2DO)}, {xSize, ySize}, C2DO);
	textEntry.place(renderer(), textRect, projP);
	#else
	textRect.setPosRel(rect.pos(C2DO) - IG::WP{0, (int)rect.ySize()/4}, {xSize, ySize}, C2DO);
	Input::placeSysTextInput(textRect);
	#endif
}

bool CollectTextInputView::inputEvent(Input::Event e)
{
	if(e.state() == Input::PUSHED)
	{
		if(e.isDefaultCancelButton() || (e.isPointer() && cancelBtn.overlaps(e.pos())))
		{
			dismiss();
			return true;
		}
	}
	#ifndef CONFIG_INPUT_SYSTEM_COLLECTS_TEXT
	bool acceptingInput = textEntry.acceptingInput;
	bool handled = textEntry.inputEvent(renderer(), e);
	if(!textEntry.acceptingInput && acceptingInput)
	{
		logMsg("calling on-text delegate");
		if(onTextD.callCopy(*this, textEntry.str))
		{
			textEntry.setAcceptingInput(1);
		}
	}
	return handled;
	#endif
	return false;
}

void CollectTextInputView::prepareDraw()
{
	message.makeGlyphs(renderer());
	#ifndef CONFIG_INPUT_SYSTEM_COLLECTS_TEXT
	textEntry.prepareDraw(renderer());
	#endif
}

void CollectTextInputView::draw(Gfx::RendererCommands &cmds)
{
	using namespace Gfx;
	#ifndef CONFIG_BASE_ANDROID
	if(cancelSpr.image())
	{
		cmds.setColor(COLOR_WHITE);
		cmds.setBlendMode(BLEND_MODE_ALPHA);
		cmds.setCommonTextureSampler(CommonTextureSampler::NEAREST_MIP_CLAMP);
		cancelSpr.setCommonProgram(cmds, IMG_MODE_MODULATE, projP.makeTranslate(projP.unProjectRect(cancelBtn).pos(C2DO)));
		cancelSpr.draw(cmds);
	}
	#endif
	#ifndef CONFIG_INPUT_SYSTEM_COLLECTS_TEXT
	cmds.setColor(0.25);
	cmds.setCommonProgram(CommonProgram::NO_TEX, projP.makeTranslate());
	GeomRect::draw(cmds, textEntry.b, projP);
	cmds.setColor(COLOR_WHITE);
	textEntry.draw(cmds);
	cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
	message.draw(cmds, 0, projP.unprojectY(textEntry.b.pos(C2DO).y) + message.nominalHeight, CB2DO, projP);
	#else
	cmds.setColor(COLOR_WHITE);
	cmds.setCommonProgram(CommonProgram::TEX_ALPHA, projP.makeTranslate());
	message.draw(cmds, 0, projP.unprojectY(Input::sysTextInputRect().pos(C2DO).y) + message.nominalHeight, CB2DO, projP);
	#endif
}
