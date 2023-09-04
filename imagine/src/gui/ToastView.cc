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
#include <imagine/gfx/GeomQuad.hh>
#include <imagine/gfx/BasicEffect.hh>
#include <imagine/input/Event.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/math/int.hh>
#include <string>

namespace IG
{

ToastView::ToastView(ViewAttachParams attach): View{attach},
	unpostTimer
	{
		"ToastView::unpostTimer",
		[this]()
		{
			unpost();
		}
	} {}

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
		text.resetString();
	}
}

void ToastView::place()
{
	text.compile(renderer(), {.maxLineSize = int(viewRect().xSize() * 0.95f), .maxLines = 6});
	int labelYSize = IG::makeEvenRoundedUp(text.fullHeight());
	//logMsg("label y size:%d", labelYSize);
	msgFrame.setPosRel(viewRect().pos(CB2DO),
		{viewRect().xSize(), labelYSize}, CB2DO);
}

void ToastView::unpost()
{
	logMsg("unposting");
	waitForDrawFinished();
	text.resetString();
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

void ToastView::draw(Gfx::RendererCommands &__restrict__ cmds)
{
	using namespace IG::Gfx;
	if(!text.isVisible())
		return;
	auto &basicEffect = cmds.basicEffect();
	basicEffect.disableTexture(cmds);
	cmds.set(BlendMode::ALPHA);
	if(error)
		cmds.setColor({1., 0, 0, .7});
	else
		cmds.setColor({0, 0, 1., .7});
	cmds.drawRect(msgFrame);
	basicEffect.enableAlphaTexture(cmds);
	text.draw(cmds, {msgFrame.xCenter(), msgFrame.pos(C2DO).y}, C2DO, ColorName::WHITE);
}

bool ToastView::inputEvent(const Input::Event &event)
{
	return false;
}

}
