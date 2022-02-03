#pragma once

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

#include <imagine/gfx/GfxText.hh>
#include <imagine/gui/View.hh>
#include <imagine/util/Interpolator.hh>

namespace EmuEx
{

using namespace IG;

class CreditsView : public View
{
public:
	CreditsView(ViewAttachParams attach, IG::utf16String str);
	~CreditsView();
	void prepareDraw() final;
	void draw(Gfx::RendererCommands &cmds) final;
	void place() final;
	bool inputEvent(const Input::Event &) final;
	std::u16string_view name() const final;

private:
	Gfx::Text text{};
	IG::InterpolatorValue<float, IG::FrameTime, IG::InterpolatorType::LINEAR> fade{};
	IG::OnFrameDelegate animate{};
};

}
