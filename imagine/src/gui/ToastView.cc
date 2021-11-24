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

#define LOGTAG "ToastView"
#include <imagine/gui/ToastView.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/GeomRect.hh>
#include <imagine/input/Input.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/math/int.hh>
#include <string>

ToastView::ToastView() {}

ToastView::ToastView(ViewAttachParams attach): View{attach},
	unpostTimer
	{
		"ToastView::unpostTimer",
		[this]()
		{
			unpost();
		}
	}
{
	text.setMaxLines(6);
}

void ToastView::setFace(Gfx::GlyphTextureSet &face)
{
	waitForDrawFinished();
	text.setFace(&face);
}

void ToastView::clear()
{
	if(text.stringSize())
	{
		unpostTimer.cancel();
		waitForDrawFinished();
		text.setString({});
	}
}

void ToastView::place()
{
	text.setMaxLineSize(projP.width());
	text.compile(renderer(), projP);

	int labelYSize = IG::makeEvenRoundedUp(projP.projectYSize(text.fullHeight()));
	IG::WindowRect viewFrame;
	//logMsg("label y size:%d", labelYSize);
	viewFrame.setPosRel(viewRect().pos(CB2DO),
		{viewRect().xSize(), labelYSize}, CB2DO);
	msgFrame = projP.unProjectRect(viewFrame);
}

void ToastView::post(IG::utf16String msg, int secs, bool error)
{
	text.setString(std::move(msg));
	place();
	this->error = error;
	postContent(secs);
}

void ToastView::postError(IG::utf16String msg, int secs)
{
	post(std::move(msg), secs, true);
}

void ToastView::unpost()
{
	logMsg("unposting");
	waitForDrawFinished();
	text.setString({});
	postDraw();
}

void ToastView::postContent(int secs)
{
	postDraw();
	unpostTimer.runIn(IG::Seconds{secs});
}

void ToastView::prepareDraw()
{
	text.makeGlyphs(renderer());
}

void ToastView::draw(Gfx::RendererCommands &cmds)
{
	using namespace Gfx;
	if(!text.isVisible())
		return;
	cmds.setCommonProgram(CommonProgram::NO_TEX, projP.makeTranslate());
	cmds.setBlendMode(BLEND_MODE_ALPHA);
	if(error)
		cmds.setColor(1., 0, 0, .7);
	else
		cmds.setColor(0, 0, 1., .7);
	GeomRect::draw(cmds, msgFrame);
	cmds.setColor(1., 1., 1., 1.);
	cmds.setCommonProgram(CommonProgram::TEX_ALPHA);
	text.draw(cmds, 0, projP.alignYToPixel(msgFrame.pos(C2DO).y), C2DO, projP);
}

bool ToastView::inputEvent(Input::Event event)
{
	return false;
}
