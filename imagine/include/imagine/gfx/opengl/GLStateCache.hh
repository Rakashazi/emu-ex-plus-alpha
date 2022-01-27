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

#include <imagine/config/env.hh>
#include <imagine/gfx/defs.hh>

class GLStateCache
{
public:
	constexpr GLStateCache() = default;

	struct GLBindTextureState
	{
		GLuint GL_TEXTURE_2D_state = 0;
	};

	struct GLStateCaps
	{
		#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
		int8_t GL_ALPHA_TEST_state = -1;
		int8_t GL_FOG_state = -1;
		int8_t GL_TEXTURE_2D_state = -1;
		#endif
		int8_t GL_DEPTH_TEST_state = -1;
		int8_t GL_BLEND_state = -1;
		int8_t GL_SCISSOR_TEST_state = -1;
		int8_t GL_CULL_FACE_state = -1;
		int8_t GL_DITHER_state = -1;
		#ifndef CONFIG_GFX_OPENGL_ES
		// extensions
		int8_t GL_MULTISAMPLE_state = -1;
		#endif
	};

	struct GLClientStateCaps
	{
		int8_t GL_TEXTURE_COORD_ARRAY_state = -1;
		int8_t GL_COLOR_ARRAY_state = -1;
	};

	static bool verifyState;

	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	GLenum matrixModeState = -1;
	void matrixMode(GLenum mode);
	#endif

	GLBindTextureState bindTextureState;
	GLuint *getBindTextureState(GLenum target);
	void bindTexture(GLenum target, GLuint texture);

	GLenum blendFuncSfactor = -1, blendFuncDfactor = -1;
	void blendFunc(GLenum sfactor, GLenum dfactor);

	GLenum blendEquationState = -1;
	void blendEquation(GLenum mode);

	GLStateCaps stateCap;
	int8_t *getCap(GLenum cap);
	void enable(GLenum cap);
	void disable(GLenum cap);
	GLboolean isEnabled(GLenum cap);

	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	GLClientStateCaps clientStateCap;
	int8_t *getClientCap(GLenum cap);
	void enableClientState(GLenum cap);
	void disableClientState(GLenum cap);
	GLint GL_TEXTURE_ENV_GL_TEXTURE_ENV_MODE_state = GL_MODULATE;
	void texEnvi(GLenum target, GLenum pname, GLint param);
	GLfloat GL_TEXTURE_ENV_GL_TEXTURE_ENV_COLOR_state[4] = { 0, 0, 0, 0 };
	void texEnvfv(GLenum target, GLenum pname, const GLfloat *params);
	std::array<GLfloat, 4> colorState{1, 1, 1, 1};
	void color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	#endif
};
