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

#include <imagine/gfx/defs.hh>
#include <array>

class GLStateCache
{
public:
	constexpr GLStateCache() = default;

	struct GLStateCaps
	{
		int8_t GL_BLEND_state = -1;
		int8_t GL_SCISSOR_TEST_state = -1;
	};

	struct GLClientStateCaps
	{
		int8_t GL_VERTEX_ARRAY_state = -1;
		int8_t GL_TEXTURE_COORD_ARRAY_state = -1;
		int8_t GL_COLOR_ARRAY_state = -1;
	};

	static bool verifyState;

	GLenum blendFuncSfactor = -1, blendFuncDfactor = -1;
	void blendFunc(GLenum sfactor, GLenum dfactor);

	GLenum blendEquationState = -1;
	void blendEquation(GLenum mode);

	GLStateCaps stateCap;
	int8_t *getCap(GLenum cap);
	void enable(GLenum cap);
	void disable(GLenum cap);
	GLboolean isEnabled(GLenum cap);
};
