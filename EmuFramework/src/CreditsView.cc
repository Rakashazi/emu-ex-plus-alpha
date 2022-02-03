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
#include <imagine/util/variant.hh>

namespace EmuEx
{

CreditsView::CreditsView(ViewAttachParams attach, IG::utf16String str):
	View{attach},
	text{std::move(str), &defaultFace()},
	animate
	{
		[this](IG::FrameParams params)
		{
			window().setNeedsDraw(true);
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
	using namespace IG::Gfx;
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

bool CreditsView::inputEvent(const Input::Event &e)
{
	if(visit(overloaded
		{
			[&](const Input::MotionEvent &e) { return viewRect().overlaps(e.pos()) && e.released(); },
			[&](const Input::KeyEvent &e) { return e.pushed(Input::DefaultKey::CANCEL); }
		}, e.asVariant()))
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

std::u16string_view CreditsView::name() const
{
	return appViewTitle();
}

}
