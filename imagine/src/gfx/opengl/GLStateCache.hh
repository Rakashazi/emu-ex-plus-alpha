#pragma once

#include <imagine/config/env.hh>
#include <imagine/util/algorithm.h>
#include "utils.h"

#if !defined CONFIG_GFX_OPENGL_ES || CONFIG_GFX_OPENGL_ES_MAJOR_VERSION > 1
#define HAS_UNPACK_ROW_LENGTH
#endif

static GLenum textureTargetToGet(GLenum target)
{
	switch(target)
	{
		case GL_TEXTURE_2D: return GL_TEXTURE_BINDING_2D;
		#ifdef CONFIG_GFX_OPENGL_MULTIPLE_TEXTURE_TARGETS
		case GL_TEXTURE_EXTERNAL_OES: return GL_TEXTURE_BINDING_EXTERNAL_OES;
		#endif
		default: bug_branch("%d", target); return 0;
	}
}

class GLStateCache
{
public:
	constexpr GLStateCache() {}
	
	struct GLPointerVal
	{
		GLint size = 4;
		GLenum type = GL_FLOAT;
		GLsizei stride = 0;
		const GLvoid *pointer = nullptr;

		constexpr GLPointerVal() {}
	};

	struct GLAttribPointerVal
	{
		GLint size = 4;
		GLenum type = GL_FLOAT;
		GLsizei stride = 0;
		GLboolean normalized = GL_FALSE;
		const GLvoid *pointer = nullptr;

		constexpr GLAttribPointerVal() {}
	};

	struct GLBindTextureState
	{
		GLuint GL_TEXTURE_2D_state = 0;
		#ifdef CONFIG_GFX_OPENGL_MULTIPLE_TEXTURE_TARGETS
		GLuint GL_TEXTURE_EXTERNAL_OES_state = 0;
		#endif

		constexpr GLBindTextureState() {}
	};

	struct GLStateCaps
	{
		#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
		bool GL_ALPHA_TEST_state = 0;
		bool GL_FOG_state = 0;
		bool GL_TEXTURE_2D_state = 0;
			#ifdef CONFIG_GFX_OPENGL_MULTIPLE_TEXTURE_TARGETS
			bool GL_TEXTURE_EXTERNAL_OES_state = 0;
			#endif
		#endif
		bool GL_DEPTH_TEST_state = 0;
		bool GL_BLEND_state = 0;
		bool GL_SCISSOR_TEST_state = 0;
		bool GL_CULL_FACE_state = 0;
		bool GL_DITHER_state = 1;
		#ifndef CONFIG_GFX_OPENGL_ES
		// extensions
		bool GL_MULTISAMPLE_state = 0;
		#endif

		constexpr GLStateCaps() {}
	};

	struct GLBindBufferVal
	{
		GLenum target;
		GLuint buffer;
	};

	struct GLClientStateCaps
	{
		bool GL_TEXTURE_COORD_ARRAY_state = 0;
		bool GL_COLOR_ARRAY_state = 0;

		constexpr GLClientStateCaps() {}
	};

	struct GLPixelStoreParams
	{
		GLint GL_UNPACK_ALIGNMENT_state = 4;
		#ifdef HAS_UNPACK_ROW_LENGTH
		GLint GL_UNPACK_ROW_LENGTH_state = 0;
		#endif

		constexpr GLPixelStoreParams() {}
	};

	static bool verifyState;

	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	GLenum matrixModeState = GL_MODELVIEW;
	void matrixMode(GLenum mode);
	#endif

	GLBindTextureState bindTextureState;
	GLuint *getBindTextureState(GLenum target);
	void bindTexture(GLenum target, GLuint texture);
	void deleteTextures(GLsizei n, const GLuint *textures);

	GLenum blendFuncSfactor = GL_ONE, blendFuncDfactor = GL_ZERO;
	void blendFunc(GLenum sfactor, GLenum dfactor);

	GLenum blendEquationState = GL_FUNC_ADD;
	void blendEquation(GLenum mode);

	GLStateCaps stateCap;
	bool *getCap(GLenum cap);
	void enable(GLenum cap);
	void disable(GLenum cap);
	GLboolean isEnabled(GLenum cap);

	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	GLClientStateCaps clientStateCap;
	bool *getClientCap(GLenum cap);
	void enableClientState(GLenum cap);
	void disableClientState(GLenum cap);
	GLint GL_TEXTURE_ENV_GL_TEXTURE_ENV_MODE_state = GL_MODULATE;
	void texEnvi(GLenum target, GLenum pname, GLint param);
	GLfloat GL_TEXTURE_ENV_GL_TEXTURE_ENV_COLOR_state[4] = { 0, 0, 0, 0 };
	void texEnvfv(GLenum target, GLenum pname, const GLfloat *params);
	GLfloat colorState[4] = { 1, 1, 1, 1 };
	void color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	GLPointerVal texCoordPointerState;
	void texCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	GLPointerVal colorPointerState;
	void colorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	GLPointerVal vertexPointerState;
	void vertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	GLAttribPointerVal vertexAttribPointerState[3];
	void vertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
	#endif

	GLBindBufferVal glBindBufferVal[4]
	{
		{ GL_ARRAY_BUFFER, 0 }, // needs to be first array element for glState_vertexPointer, etc
		{ GL_ELEMENT_ARRAY_BUFFER, 0 },
		#if !defined CONFIG_GFX_OPENGL_ES || CONFIG_GFX_OPENGL_ES_MAJOR_VERSION > 1
		{ GL_PIXEL_PACK_BUFFER, 0 },
		{ GL_PIXEL_UNPACK_BUFFER, 0 }
		#endif
	};

	bool vboIsBound();
	void bindBuffer(GLenum target, GLuint buffer);
	void deleteBuffers(GLsizei n, const GLuint *buffers);

	GLPixelStoreParams pixelStoreParam;
	GLint *getPixelStoreParam(GLenum pname);
	void pixelStorei(GLenum pname, GLint param);

private:
	static uint applyGenericPointerState(GLPointerVal &state, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
};
