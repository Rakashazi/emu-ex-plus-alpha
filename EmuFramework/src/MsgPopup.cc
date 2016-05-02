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
#include <emuframework/MsgPopup.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/gui/View.hh>

using namespace Base;

void MsgPopup::init()
{
	//logMsg("init MsgPopup");
	text = {nullptr, View::defaultFace};
	text.setString(str.data());
	text.maxLines = 6;
}

void MsgPopup::clear()
{
	if(strlen(str.data()))
	{
		unpostTimer.deinit();
		str = {};
	}
}

void MsgPopup::place(const Gfx::ProjectionPlane &projP)
{
	this->projP = projP;
	text.maxLineSize = projP.w;
	if(strlen(str.data()))
		text.compile(projP);
}

void MsgPopup::unpost()
{
	logMsg("unposting");
	str = {};
	mainWin.win.postDraw();
}

void MsgPopup::postContent(int secs, bool error)
{
	assert(strlen(str.data()));
	mainWin.win.postDraw();
	logMsg("%s", str.data());
	text.compile(projP);
	this->error = error;
	unpostTimer.callbackAfterSec([this](){unpost();}, secs);
}

void MsgPopup::post(const char *msg, int secs, bool error)
{
	string_copy(str, msg);
	postContent(secs, error);
}

void MsgPopup::postError(const char *msg, int secs)
{
	post(msg, secs, 1);
}

void MsgPopup::printf(uint secs, bool error, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	auto result = vsnprintf(str.data(), str.size(), format, args);
	va_end(args);
	postContent(secs, error);
}

void MsgPopup::draw()
{
	using namespace Gfx;
	if(strlen(str.data()))
	{
		noTexProgram.use(projP.makeTranslate());
		setBlendMode(BLEND_MODE_ALPHA);
		if(error)
			setColor(1., 0, 0, .7);
		else
			setColor(0, 0, 1., .7);
		Gfx::GCRect rect(-projP.wHalf(), -projP.hHalf(),
				projP.wHalf(), -projP.hHalf() + (text.ySize * 1.5));
		#if CONFIG_ENV_WEBOS_OS >= 3
		if(Input::softInputIsActive())
		{
			// Show messages on top on WebOS 3.x since there's no way to know how large the on-screen keyboard is
			rect.y = projP.hHalf - (text.ySize * 1.5);
			rect.y2 = projP.hHalf;
		}
		#endif
		GeomRect::draw(rect);
		setColor(1., 1., 1., 1.);
		texAlphaProgram.use();
		text.draw(0, projP.alignYToPixel(rect.y + (text.ySize * 1.5)/2.), C2DO, projP);
	}
}
