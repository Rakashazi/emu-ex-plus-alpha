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
class ProjectionPlane;
}

namespace IG
{

class ViewManager
{
public:
	static constexpr bool needsBackControlDefault = ViewDefs::needsBackControlDefault;
	static constexpr bool needsBackControlIsMutable = ViewDefs::needsBackControlIsMutable;

	constexpr ViewManager() = default;
	ViewManager(Gfx::Renderer &);
	void setDefaultFace(Gfx::GlyphTextureSet);
	void setDefaultBoldFace(Gfx::GlyphTextureSet);
	Gfx::GlyphTextureSet &defaultFace();
	Gfx::GlyphTextureSet &defaultBoldFace();
	constexpr bool needsBackControl() const { return needsBackControl_; }
	void setNeedsBackControl(std::optional<bool>);
	std::optional<bool> needsBackControlOption() const;
	float tableXIndent() const;
	void setTableXIndentMM(float indentMM, Gfx::ProjectionPlane);
	float defaultTableXIndentMM(const Window &);
	void setTableXIndentToDefault(const Window &, Gfx::ProjectionPlane);

protected:
	Gfx::GlyphTextureSet defaultFace_{};
	Gfx::GlyphTextureSet defaultBoldFace_{};
	float tableXIndent_{};
	// True if the platform needs an on-screen/pointer-based control to move to a previous view
	IG_UseMemberIfOrConstant(needsBackControlIsMutable,
		bool, needsBackControlDefault, needsBackControl_){needsBackControlDefault};
};

}
