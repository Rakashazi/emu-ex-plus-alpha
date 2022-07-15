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
#include <imagine/gui/ViewManager.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/GlyphTextureSet.hh>
#include <imagine/util/variant.hh>
#include <imagine/logger/logger.h>

namespace IG
{

TextEntry::TextEntry(const char *initText, Gfx::Renderer &r, Gfx::GlyphTextureSet *face, const Gfx::ProjectionPlane &projP):
	t{initText, face},
	str{initText}
{
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

bool TextEntry::inputEvent(View &parentView, const Input::Event &e)
{
	return visit(overloaded
	{
		[&](const Input::MotionEvent &motionEv)
		{
			if(motionEv.isAbsolute() && motionEv.pushed() && b.overlaps(motionEv.pos()))
			{
				setAcceptingInput(true);
				return true;
			}
			return false;
		},
		[&](const Input::KeyEvent &keyEv)
		{
			if(acceptingInput && keyEv.pushed() && keyEv.isKeyboard())
			{
				bool updateText = false;

				if(keyEv.mapKey() == Input::Keycode::BACK_SPACE)
				{
					auto len = str.size();
					if(len > 0)
					{
						str.pop_back();
						updateText = true;
					}
				}
				else
				{
					auto keyStr = keyEv.keyString(parentView.appContext());
					if(keyStr.size())
					{
						if(!multiLine)
						{
							if(keyStr[0] == '\r' || keyStr[0] == '\n')
							{
								setAcceptingInput(false);
								return true;
							}
						}
						str.append(keyStr);
						updateText = true;
					}
				}

				if(updateText)
				{
					{
						parentView.waitForDrawFinished();
						t.setString(str);
						t.compile(parentView.renderer(), projP);
					}
					parentView.postDraw();
				}
				return true;
			}
			return false;
		}
	}, e.asVariant());
}

void TextEntry::prepareDraw(Gfx::Renderer &r)
{
	t.makeGlyphs(r);
}

void TextEntry::draw(Gfx::RendererCommands &cmds)
{
	using namespace IG::Gfx;
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

CollectTextInputView::CollectTextInputView(ViewAttachParams attach, IG::CStringView msgText, IG::CStringView initialContent,
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
	IG::doIfUsed(cancelSpr,
		[&](auto &cancelSpr)
		{
			if(manager().needsBackControl() && closeRes)
			{
				cancelSpr = {{{-.5, -.5}, {.5, .5}}, closeRes};
				if(cancelSpr.compileDefaultProgram(Gfx::EnvMode::MODULATE))
					renderer().autoReleaseShaderCompiler();
			}
		});
	message = {msgText, face};
	IG::doIfUsed(textEntry,
		[](auto &textEntry)
		{
			textEntry.setAcceptingInput(true);
		});
}

void CollectTextInputView::place()
{
	using namespace IG::Gfx;
	auto &face = *message.face();
	IG::doIfUsed(cancelSpr,
		[&](auto &cancelSpr)
		{
			if(cancelSpr.image())
			{
				cancelBtn.setPosRel(viewRect().pos(RT2DO), face.nominalHeight() * 1.75, RT2DO);
				cancelSpr.setPos(projP.unProjectRect(cancelBtn));
			}
		});
	message.setMaxLineSize(projP.width() * 0.95);
	message.compile(renderer(), projP);
	IG::WindowRect textRect;
	int xSize = viewRect().xSize() * 0.95;
	int ySize = face.nominalHeight() * (Config::envIsAndroid ? 2. : 1.5);
	IG::doIfUsedOr(textEntry,
		[&](auto &textEntry)
		{
			textRect.setPosRel(viewRect().pos(C2DO), {xSize, ySize}, C2DO);
			textEntry.place(renderer(), textRect, projP);
		},
		[&]()
		{
			textRect.setPosRel(viewRect().pos(C2DO) - IG::WP{0, (int)viewRect().ySize()/4}, {xSize, ySize}, C2DO);
			textField.place(textRect);
		});
}

bool CollectTextInputView::inputEvent(const Input::Event &e)
{
	if(visit(overloaded
		{
			[&](const Input::MotionEvent &e) { return cancelBtn.overlaps(e.pos()); },
			[&](const Input::KeyEvent &e)	{ return e.pushed(Input::DefaultKey::CANCEL); }
		}, e.asVariant()))
	{
		dismiss();
		return true;
	}
	return IG::doIfUsedOr(textEntry,
		[&](auto &textEntry)
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
		},
		[]()
		{
			return false;
		});
}

void CollectTextInputView::prepareDraw()
{
	message.makeGlyphs(renderer());
	IG::doIfUsed(textEntry,
		[this](auto &textEntry)
		{
			textEntry.prepareDraw(renderer());
		});
}

void CollectTextInputView::draw(Gfx::RendererCommands &cmds)
{
	using namespace IG::Gfx;
	IG::doIfUsed(cancelSpr,
		[&](auto &cancelSpr)
		{
			if(cancelSpr.image())
			{
				cmds.set(ColorName::WHITE);
				cmds.set(BlendMode::ALPHA);
				cmds.set(imageCommonTextureSampler);
				cancelSpr.setCommonProgram(cmds, EnvMode::MODULATE, projP.makeTranslate());
				cancelSpr.draw(cmds);
			}
		});
	IG::doIfUsedOr(textEntry,
		[&](auto &textEntry)
		{
			cmds.setColor(0.25);
			cmds.setCommonProgram(CommonProgram::NO_TEX, projP.makeTranslate());
			GeomRect::draw(cmds, textEntry.bgRect(), projP);
			cmds.set(ColorName::WHITE);
			textEntry.draw(cmds);
			cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
			message.draw(cmds, 0, projP.unprojectY(textEntry.bgRect().pos(C2DO).y) + message.nominalHeight(), CB2DO, projP);
		},
		[&]()
		{
			cmds.set(ColorName::WHITE);
			cmds.setCommonProgram(CommonProgram::TEX_ALPHA, projP.makeTranslate());
			message.draw(cmds, 0, projP.unprojectY(textField.windowRect().pos(C2DO).y) + message.nominalHeight(), CB2DO, projP);
		});
}

}
