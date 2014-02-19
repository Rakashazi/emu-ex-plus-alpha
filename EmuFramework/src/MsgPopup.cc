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

#define LOGTAG "MsgPopup"
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
		unpostTimer.deinit();
	}
	text.str = 0;
}

void MsgPopup::place()
{
	text.maxLineSize = View::projP.w;
	if(text.str)
		text.compile();
}

void MsgPopup::unpost()
{
	logMsg("unposting");
	text.str = 0;
	Base::mainWindow().postDraw();
}

void MsgPopup::post(const char *msg, int secs, bool error)
{
	logMsg("%s", msg);
	text.setString(msg);
	text.compile();
	this->error = error;
	unpostTimer.callbackAfterSec([this](){unpost();}, secs);
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
		noTexProgram.use(View::projP.makeTranslate());
		setBlendMode(BLEND_MODE_ALPHA);
		if(error)
			setColor(1., 0, 0, .7);
		else
			setColor(0, 0, 1., .7);
		Gfx::GCRect rect(-View::projP.wHalf(), -View::projP.hHalf(),
				View::projP.wHalf(), -View::projP.hHalf() + (text.ySize * 1.5));
		#if CONFIG_ENV_WEBOS_OS >= 3
		if(Input::softInputIsActive())
		{
			// Show messages on top on WebOS 3.x since there's no way to know how large the on-screen keyboard is
			rect.y = View::projP.hHalf - (text.ySize * 1.5);
			rect.y2 = View::projP.hHalf;
		}
		#endif
		GeomRect::draw(rect);
		setColor(1., 1., 1., 1.);
		texAlphaProgram.use();
		text.draw(0, View::projP.alignYToPixel(rect.y + (text.ySize * 1.5)/2.), C2DO);
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
