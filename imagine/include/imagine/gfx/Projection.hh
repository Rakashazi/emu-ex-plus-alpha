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

#include <imagine/config/defs.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/gfx/ProjectionPlane.hh>
#include <imagine/gfx/Mat4.hh>

namespace Gfx
{

class Projection
{
public:
	constexpr Projection() {}

	Projection(Viewport viewport, Mat4 matrix)
	{
		mat = matrix;
		plane_ = ProjectionPlane::makeWithMatrix(viewport, matrix);
	}

	constexpr Mat4 matrix() const { return mat; };
	constexpr ProjectionPlane plane() const { return plane_; };

protected:
	Mat4 mat;
	ProjectionPlane plane_;
};

}
