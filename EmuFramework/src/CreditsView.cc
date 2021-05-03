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

#define LOGTAG "CreditsView"
#include <emuframework/CreditsView.hh>
#include "private.hh"
#include <imagine/base/Window.hh>
#include <imagine/input/Input.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/util/math/int.hh>

CreditsView::CreditsView(ViewAttachParams attach, const char *str):
	View{appViewTitle(), attach},
	text{str, &defaultFace()},
	animate
	{
		[this](IG::FrameParams params)
		{
			postDraw();
			return fade.update(params.timestamp());
		}
	}
{
	fade = {0., 1., {}, IG::steadyClockTimestamp(), IG::Milliseconds{320}};
	window().addOnFrame(animate);
	place();
}

void CreditsView::prepareDraw()
{
	text.makeGlyphs(renderer());
}

void CreditsView::draw(Gfx::RendererCommands &cmds)
{
	using namespace Gfx;
	cmds.setColor(1., 1., 1., fade);
	cmds.setCommonProgram(CommonProgram::TEX_ALPHA, projP.makeTranslate());
	auto textRect = viewRect();
	if(IG::isOdd(textRect.ySize()))
		textRect.y2--;
	text.draw(cmds, projP.unProjectRect(textRect).pos(C2DO), C2DO, projP);
}

void CreditsView::place()
{
	text.compile(renderer(), projP);
}

bool CreditsView::inputEvent(Input::Event e)
{
	if((e.isPointer() && viewRect().overlaps(e.pos()) && e.released())
			|| (!e.isPointer() && !e.isSystemFunction() && e.pushed()))
	{
		dismiss();
		return true;
	}
	return false;
}

CreditsView::~CreditsView()
{
	window().removeOnFrame(animate);
}
