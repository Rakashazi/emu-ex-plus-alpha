#pragma once

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

#include <imagine/gui/viewDefs.hh>
#include <imagine/gfx/GlyphTextureSet.hh>
#include <imagine/util/used.hh>
#include <optional>

namespace IG
{
class Window;
}

namespace IG::Gfx
{
class Renderer;
}

namespace IG
{

class ViewManager
{
public:
	static constexpr bool needsBackControlDefault = ViewDefs::needsBackControlDefault;
	static constexpr bool needsBackControlIsMutable = ViewDefs::needsBackControlIsMutable;

	Gfx::GlyphTextureSet defaultFace;
	Gfx::GlyphTextureSet defaultBoldFace;
	int tableXIndentPx{};
	// True if the platform needs an on-screen/pointer-based control to move to a previous view
	ConditionalMemberOr<needsBackControlIsMutable, bool, needsBackControlDefault>
		needsBackControl{needsBackControlDefault};

	std::optional<bool> needsBackControlOption() const;
	void setTableXIndentMM(float indentMM, const Window &);
	float defaultTableXIndentMM(const Window &);
	void setTableXIndentToDefault(const Window &);
};

}
