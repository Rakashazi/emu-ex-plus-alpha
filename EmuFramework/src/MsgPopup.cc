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

#define thisModuleName "msgPopup"
#include <MsgPopup.hh>
#include <gui/View.hh>

using namespace Base;

void MsgPopup::init()
{
	//logMsg("init MsgPopup");
	text.init(View::defaultFace);
	text.maxLines = 6;
}

void MsgPopup::clear()
{
	if(text.str)
	{
		cancelCallback(callbackRef);
		callbackRef = nullptr;
	}
	text.str = 0;
}

void MsgPopup::place()
{
	text.maxLineSize = Gfx::proj.w;
	if(text.str)
		text.compile();
}

void MsgPopup::unpost()
{
	logMsg("unposting");
	callbackRef = nullptr;
	text.str = 0;
	Base::displayNeedsUpdate();
}

void MsgPopup::post(const char *msg, int secs, bool error)
{
	logMsg("%s", msg);
	text.setString(msg);
	text.compile();
	this->error = error;
	cancelCallback(callbackRef);
	callbackRef = callbackAfterDelaySec([this](){unpost();}, secs);
}

void MsgPopup::postError(const char *msg, int secs)
{
	post(msg, secs, 1);
}

void MsgPopup::draw()
{
	using namespace Gfx;
	if(text.str)
	{
		resetTransforms();
		setBlendMode(BLEND_MODE_ALPHA);
		if(error)
			setColor(1., 0, 0, .7);
		else
			setColor(0, 0, 1., .7);
		Rect2<GC> rect(-Gfx::proj.wHalf(), -Gfx::proj.hHalf(),
				Gfx::proj.wHalf(), -Gfx::proj.hHalf() + (text.ySize * 1.5));
		#if CONFIG_ENV_WEBOS_OS >= 3
		if(Input::softInputIsActive())
		{
			// Show messages on top on WebOS 3.x since there's no way to know how large the on-screen keyboard is
			rect.y = Gfx::proj.hHalf - (text.ySize * 1.5);
			rect.y2 = Gfx::proj.hHalf;
		}
		#endif
		GeomRect::draw(rect);
		setColor(1., 1., 1., 1.);
		text.draw(0, rect.y + (text.ySize * 1.5)/2., C2DO, C2DO);
	}
}

void MsgPopup::printf(uint secs, bool error, const char *format, ...)
{
	va_list args;
	va_start( args, format );
	vsnprintf(str, sizeof(str), format, args);
	va_end( args );
	//logMsg("%s", str);
	post(str, secs, error);
}
