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

void MsgPopup::place(const Gfx::ProjectionPlane &projP)
{
	var_selfs(projP)
	text.maxLineSize = projP.w;
	if(text.str)
		text.compile(projP);
}

void MsgPopup::unpost()
{
	logMsg("unposting");
	text.str = 0;
	mainWin.win.postDraw();
}

void MsgPopup::post(const char *msg, int secs, bool error)
{
	mainWin.win.postDraw();
	logMsg("%s", msg);
	text.setString(msg);
	text.compile(projP);
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
