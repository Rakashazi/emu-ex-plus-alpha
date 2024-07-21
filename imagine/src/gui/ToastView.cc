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
#include <imagine/gui/ViewManager.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/BasicEffect.hh>
#include <imagine/gfx/Mat4.hh>
#include <imagine/input/Event.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/math.hh>
#include <string>

namespace IG
{

ToastView::ToastView(ViewAttachParams attach): View{attach},
	text{attach.rendererTask, &attach.viewManager.defaultFace},
	unpostTimer
	{
		{.debugLabel = "ToastView::unpostTimer"}, [this]{ unpost(); }
	},
	msgFrameQuads{attach.rendererTask, {.size = 1}} {}

void ToastView::setFace(Gfx::GlyphTextureSet &face)
{
	text.setFace(&face);
}

void ToastView::clear()
{
	if(text.stringSize())
	{
		unpostTimer.cancel();
		text.resetString();
	}
}

void ToastView::place()
{
	text.compile({.maxLineSize = int(viewRect().xSize() * 0.95f), .maxLines = 6, .alignment = Gfx::TextAlignment::center});
	int labelYSize = IG::makeEvenRoundedUp(text.fullHeight());
	//logMsg("label y size:%d", labelYSize);
	msgFrame.setPosRel(viewRect().pos(CB2DO),
		{viewRect().xSize(), labelYSize}, CB2DO);
	msgFrameQuads.write(0, {.bounds = msgFrame.as<int16_t>()});
}

void ToastView::unpost()
{
	logMsg("unposting");
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
	text.makeGlyphs();
}

void ToastView::draw(Gfx::RendererCommands &__restrict__ cmds, ViewDrawParams) const
{
	using namespace IG::Gfx;
	if(!text.isVisible())
		return;
	auto &basicEffect = cmds.basicEffect();
	basicEffect.disableTexture(cmds);
	basicEffect.setModelView(cmds, Gfx::Mat4::ident());
	cmds.set(BlendMode::ALPHA);
	if(error)
		cmds.setColor({1., 0, 0, .7});
	else
		cmds.setColor({0, 0, 1., .7});
	cmds.drawQuad(msgFrameQuads, 0);
	basicEffect.enableAlphaTexture(cmds);
	text.draw(cmds, {msgFrame.xCenter(), msgFrame.pos(C2DO).y}, C2DO, ColorName::WHITE);
}

}
