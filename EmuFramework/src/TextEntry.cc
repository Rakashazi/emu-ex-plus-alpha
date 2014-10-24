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

#include <emuframework/TextEntry.hh>
#include <imagine/gui/GuiTable1D.hh>

void TextEntry::setAcceptingInput(bool on)
{
	if(on)
	{
		Input::setTranslateKeyboardEventsByModifiers(1);
		Input::showSoftInput();
		logMsg("accepting input");
	}
	else
	{
		Input::setTranslateKeyboardEventsByModifiers(0);
		Input::hideSoftInput();
		logMsg("stopped accepting input");
	}
	acceptingInput = on;
}

void TextEntry::inputEvent(const Input::Event &e)
{
	if(e.isPointer() && e.pushed() && b.overlaps({e.x, e.y}))
	{
		{
			setAcceptingInput(1);
		}
		return;
	}

	if(acceptingInput && e.pushed() && e.isKeyboard())
	{
		bool updateText = 0;

		if(!e.isKeyboard())
		{
			//setAcceptingInput(0);
			return;
		}
		#ifdef INPUT_SUPPORTS_KEYBOARD
		else if(e.button == Input::Keycode::BACK_SPACE)
		{
			int len = strlen(str);
			if(len > 0)
			{
				str[len-1] = '\0';
				updateText = 1;
			}
		}
		else if(Input::Keycode::isAsciiKey(e.button))
		{
			if(strlen(str) < 127)
			{
				uchar key = e.decodeAscii();
				//logMsg("got input %c", key);
				char keyStr[] = " ";
				if(!multiLine)
				{
					if(key == '\r' || key == '\n')
					{
						setAcceptingInput(0);
						return;
					}
				}
				keyStr[0] = key == '\r' ? '\n' : key;
				strcat(str, keyStr);
				updateText = 1;
			}
		}
		#endif

		if(updateText)
		{
			t.setString(str);
			t.compile(projP);
			Base::mainWindow().postDraw();
		}
	}
}

void TextEntry::draw()
{
	using namespace Gfx;
	texAlphaProgram.use();
	t.draw(projP.unProjectRect(b).pos(LC2DO) + GP{GuiTable1D::globalXIndent, 0}, LC2DO, projP);
}

void TextEntry::place()
{
	t.compile(projP);
}

void TextEntry::place(IG::WindowRect rect, const Gfx::ProjectionPlane &projP)
{
	var_selfs(projP);
	b = rect;
	place();
}

CallResult TextEntry::init(const char *initText, ResourceFace *face, const Gfx::ProjectionPlane &projP)
{
	string_copy(str, initText, sizeof(str));
	t.init(str, face);
	t.compile(projP);
	acceptingInput = 0;
	return OK;
}

void TextEntry::deinit()
{
	Input::setTranslateKeyboardEventsByModifiers(0);
	Input::hideSoftInput();
	t.deinit();
}

void CollectTextInputView::init(const char *msgText, const char *initialContent, Gfx::BufferImage *closeRes, ResourceFace *face)
{
	#ifndef CONFIG_BASE_ANDROID
	if(View::needsBackControl && closeRes)
	{
		cancelSpr.init(-.5, -.5, .5, .5, closeRes);
		if(cancelSpr.compileDefaultProgram(Gfx::IMG_MODE_MODULATE))
			Gfx::autoReleaseShaderCompiler();
	}
	#endif
	message.init(msgText, face);
	#ifndef CONFIG_INPUT_SYSTEM_CAN_COLLECT_TEXT
	textEntry.init(initialContent, face, projP);
	textEntry.setAcceptingInput(1);
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
		initialContent, msgText, face->settings.pixelHeight);
	#endif
}

void CollectTextInputView::deinit()
{
	#ifndef CONFIG_BASE_ANDROID
	cancelSpr.deinit();
	#endif
	message.deinit();
	#ifndef CONFIG_INPUT_SYSTEM_CAN_COLLECT_TEXT
	textEntry.deinit();
	#else
	Input::cancelSysTextInput();
	#endif
}

void CollectTextInputView::place()
{
	using namespace Gfx;
	#ifndef CONFIG_BASE_ANDROID
	if(cancelSpr.image())
	{
		cancelBtn.setPosRel(rect.pos(RT2DO), View::defaultFace->nominalHeight() * 1.75, RT2DO);
		cancelSpr.setPos(-projP.unprojectXSize(cancelBtn)/3., -projP.unprojectYSize(cancelBtn)/3., projP.unprojectXSize(cancelBtn)/3., projP.unprojectYSize(cancelBtn)/3.);
	}
	#endif
	message.maxLineSize = projP.w * 0.95;
	message.compile(projP);
	IG::WindowRect textRect;
	int xSize = rect.xSize() * 0.95;
	int ySize = View::defaultFace->nominalHeight()* (Config::envIsAndroid ? 2. : 1.5);
	#ifndef CONFIG_INPUT_SYSTEM_CAN_COLLECT_TEXT
	textRect.setPosRel({rect.xPos(C2DO), rect.yPos(C2DO)}, {xSize, ySize}, C2DO);
	textEntry.place(textRect, projP);
	#else
	textRect.setPosRel(rect.pos(C2DO) - IG::WP{0, (int)rect.ySize()/4}, {xSize, ySize}, C2DO);
	Input::placeSysTextInput(textRect);
	#endif
}

void CollectTextInputView::inputEvent(const Input::Event &e)
{
	if(e.state == Input::PUSHED)
	{
		if(e.isDefaultCancelButton() || (e.isPointer() && cancelBtn.overlaps({e.x, e.y})))
		{
			dismiss();
			return;
		}
	}
	#ifndef CONFIG_INPUT_SYSTEM_CAN_COLLECT_TEXT
	bool acceptingInput = textEntry.acceptingInput;
	textEntry.inputEvent(e);
	if(!textEntry.acceptingInput && acceptingInput)
	{
		logMsg("calling on-text delegate");
		if(onTextD(*this, textEntry.str))
		{
			textEntry.setAcceptingInput(1);
		}
	}
	#endif
}

void CollectTextInputView::draw()
{
	using namespace Gfx;
	#ifndef CONFIG_BASE_ANDROID
	if(cancelSpr.image())
	{
		setColor(COLOR_WHITE);
		setBlendMode(BLEND_MODE_ALPHA);
		cancelSpr.useDefaultProgram(IMG_MODE_MODULATE, projP.makeTranslate(projP.unProjectRect(cancelBtn).pos(C2DO)));
		cancelSpr.draw();
	}
	#endif
	#ifndef CONFIG_INPUT_SYSTEM_CAN_COLLECT_TEXT
	setColor(0.25);
	noTexProgram.use(projP.makeTranslate());
	GeomRect::draw(textEntry.b, projP);
	setColor(COLOR_WHITE);
	textEntry.draw();
	texAlphaProgram.use();
	message.draw(0, projP.unprojectY(textEntry.b.pos(C2DO).y) + message.nominalHeight, CB2DO, projP);
	#else
	setColor(COLOR_WHITE);
	texAlphaProgram.use(projP.makeTranslate());
	message.draw(0, projP.unprojectY(Input::sysTextInputRect().pos(C2DO).y) + message.nominalHeight, CB2DO, projP);
	#endif
}
