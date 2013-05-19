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

#include <TextEntry.hh>
#include <gui/GuiTable1D/GuiTable1D.hh>

Gfx::BufferImage *getXAsset();

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
	if(e.isPointer() && e.pushed() && b.overlaps(e.x, e.y))
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
		else if(Input::isAsciiKey(e.button))
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
			t.compile();
			Base::displayNeedsUpdate();
		}
	}
}

void TextEntry::draw()
{
	using namespace Gfx;
	t.draw(gXPos(b, LC2DO) + GuiTable1D::globalXIndent, gYPos(b, LC2DO), LC2DO);
}

void TextEntry::place()
{
	t.compile();
}

void TextEntry::place(Rect2<int> rect)
{
	b = rect;
	place();
}

CallResult TextEntry::init(const char *initText, ResourceFace *face)
{
	string_copy(str, initText, sizeof(str));
	t.init(str, face);
	t.compile();
	acceptingInput = 0;
	return OK;
}

void TextEntry::deinit()
{
	Input::setTranslateKeyboardEventsByModifiers(0);
	Input::hideSoftInput();
	t.deinit();
}

void CollectTextInputView::init(const char *msgText, const char *initialContent, ResourceFace *face)
{
	#ifndef CONFIG_BASE_ANDROID
	if(View::needsBackControl)
	{
		auto res = getXAsset();
		cancelSpr.init(-.5, -.5, .5, .5, res);
	}
	#endif
	message.init(msgText, face);
	#ifndef CONFIG_INPUT_SYSTEM_CAN_COLLECT_TEXT
	textEntry.init(initialContent, face);
	textEntry.setAcceptingInput(1);
	#else
	Input::startSysTextInput(
		[this](const char *str)
		{
			if(!str)
			{
				logMsg("text collection canceled by external source");
				removeModalView();
				return;
			}
			if(onTextD(str))
			{
				logMsg("text collection canceled by text delegate");
				removeModalView();
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
		cancelSpr.setPos(-Gfx::gXSize(cancelBtn)/3., -Gfx::gYSize(cancelBtn)/3., Gfx::gXSize(cancelBtn)/3., Gfx::gYSize(cancelBtn)/3.);
	}
	#endif
	message.maxLineSize = Gfx::proj.w * 0.95;
	message.compile();
	Rect2<int> textRect;
	int xSize = rect.xSize() * 0.95;
	int ySize = View::defaultFace->nominalHeight()* (Config::envIsAndroid ? 2. : 1.5);
	#ifndef CONFIG_INPUT_SYSTEM_CAN_COLLECT_TEXT
	textRect.setPosRel(rect.xPos(C2DO), rect.yPos(C2DO), xSize, ySize, C2DO);
	textEntry.place(textRect);
	#else
	//a.setPos(gXPos(rect, C2DO), gYPos(rect, C2DO) + proj.h/4., C2DO, C2DO);
	textRect.setPosRel(rect.xPos(C2DO), rect.yPos(C2DO) - Gfx::viewPixelHeight()/4, xSize, ySize, C2DO);
	Input::placeSysTextInput(textRect);
	#endif
}

void CollectTextInputView::inputEvent(const Input::Event &e)
{
	if(e.state == Input::PUSHED)
	{
		if(e.isDefaultCancelButton() || (e.isPointer() && cancelBtn.overlaps(e.x, e.y)))
		{
			removeModalView();
			return;
		}
	}
	#ifndef CONFIG_INPUT_SYSTEM_CAN_COLLECT_TEXT
	bool acceptingInput = textEntry.acceptingInput;
	textEntry.inputEvent(e);
	if(!textEntry.acceptingInput && acceptingInput)
	{
		logMsg("calling on-text delegate");
		if(onTextD(textEntry.str))
		{
			textEntry.setAcceptingInput(1);
		}
	}
	#endif
}

void CollectTextInputView::draw(Gfx::FrameTimeBase frameTime)
{
	using namespace Gfx;
	#ifndef CONFIG_BASE_ANDROID
	if(cancelSpr.image())
	{
		setColor(COLOR_WHITE);
		setBlendMode(BLEND_MODE_INTENSITY);
		loadTranslate(gXPos(cancelBtn, C2DO), gYPos(cancelBtn, C2DO));
		cancelSpr.draw();
	}
	#endif
	#ifndef CONFIG_INPUT_SYSTEM_CAN_COLLECT_TEXT
	setColor(0.25);
	shadeMod();
	resetTransforms();
	GeomRect::draw(textEntry.b);
	setColor(COLOR_WHITE);
	textEntry.draw();
	message.draw(0, gYPos(textEntry.b, C2DO) + message.nominalHeight, CB2DO, C2DO);
	#else
	setColor(COLOR_WHITE);
	resetTransforms();
	message.draw(0, gYPos(Input::sysTextInputRect(), C2DO) + message.nominalHeight, CB2DO, C2DO);
	#endif
}
