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
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#define thisModuleName "msgPopup"
#include <MsgPopup.hh>
#include <gui/View.hh>

void MsgPopup::init()
{
	//logMsg("init MsgPopup");
	text.init(View::defaultFace);
	text.maxLines = 4;
}

void MsgPopup::clear()
{
	if(text.str)
		Base::setTimerCallback(0, 0, 0);
	text.str = 0;
}

void MsgPopup::place()
{
	text.maxLineSize = Gfx::proj.w;
	if(text.str)
		text.compile();
}

void MsgPopup::unpost(void *ctx)
{
	MsgPopup *msg = (MsgPopup*)ctx;
	msg->text.str = 0;
	Base::displayNeedsUpdate();
}

void MsgPopup::post(const char *msg, int secs, bool error)
{
	logMsg("%s", msg);
	text.setString(msg);
	text.compile();
	this->error = error;
	Base::setTimerCallbackSec(unpost, this, secs);
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
		Rect2<GC> rect(-Gfx::proj.wHalf, -Gfx::proj.hHalf,
				Gfx::proj.wHalf, -Gfx::proj.hHalf + (text.ySize * 1.5));
		GeomRect::draw(rect);
		setColor(1., 1., 1., 1.);
		text.draw(0, -Gfx::proj.hHalf + (text.ySize * 1.5)/2., C2DO, C2DO);
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

#undef thisModuleName
