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

#include <emuframework/CreditsView.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/base/Window.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/util/math.hh>
#include <imagine/util/variant.hh>

namespace EmuEx
{

CreditsView::CreditsView(ViewAttachParams attach, UTF16String str):
	View{attach},
	text{attach.rendererTask, std::move(str), &defaultFace()},
	animate
	{
		[this](IG::FrameParams params)
		{
			window().setNeedsDraw(true);
			return fade.update(params.timestamp);
		}
	}
{
	fade = {0., 1., {}, SteadyClock::now(), Milliseconds{320}};
	window().addOnFrame(animate);
	place();
}

void CreditsView::prepareDraw()
{
	text.makeGlyphs();
}

void CreditsView::draw(Gfx::RendererCommands&__restrict__ cmds, ViewDrawParams) const
{
	using namespace IG::Gfx;
	cmds.basicEffect().enableAlphaTexture(cmds);
	text.draw(cmds, viewRect().pos(C2DO), C2DO, Color{1., 1., 1., fade});
}

void CreditsView::place()
{
	text.compile({.alignment = Gfx::TextAlignment::center});
}

bool CreditsView::inputEvent(const Input::Event& e, ViewInputEventParams)
{
	if(e.visit(overloaded
		{
			[&](const Input::MotionEvent &e) { return viewRect().overlaps(e.pos()) && e.released(); },
			[&](const Input::KeyEvent &e) { return e.pushed(Input::DefaultKey::CANCEL); }
		}))
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
	return EmuApp::mainViewName();
}

}
