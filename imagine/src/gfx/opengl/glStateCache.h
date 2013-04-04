#pragma once

#include <config/env.hh>
#include <util/cLang.h>
#include "utils.h"

#ifdef CONFIG_BASE_ANDROID
	#include <base/android/public.hh>
#else
	static const bool glPointerStateHack = 0;
#endif

static const bool glEnableStateHack = 0;

static GLenum textureTargetToGet(GLenum target)
{
	switch(target)
	{
		#if !defined CONFIG_BASE_PS3
		case GL_TEXTURE_2D: return GL_TEXTURE_BINDING_2D;
		#endif
		#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
		case GL_TEXTURE_EXTERNAL_OES: return GL_TEXTURE_BINDING_EXTERNAL_OES;
		#endif
		default: bug_branch("%d", target); return 0;
	}
}

class GLStateCache
{
public:
	constexpr GLStateCache() { }
	
#ifdef NDEBUG
	static constexpr bool verifyState = 0;
#else
	bool verifyState = 0;
#endif

	GLenum matrixModeState = GL_MODELVIEW;
	void matrixMode(GLenum mode)
	{
		if(mode != matrixModeState)
		{
			glMatrixMode(mode);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glMatrixMode", err); });
			matrixModeState = mode;
		}
	}

	struct GLBindTextureState
	{
		constexpr GLBindTextureState() { }
		GLuint GL_TEXTURE_2D_state = 0;
		#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
		GLuint GL_TEXTURE_EXTERNAL_OES_state = 0;
		#endif
	} bindTextureState;

	GLuint *getBindTextureState(GLenum target)
	{
		#define GLTARGET_CASE(target) case target: return &bindTextureState.target ## _state
		switch(target)
		{
			GLTARGET_CASE(GL_TEXTURE_2D);
			#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
			GLTARGET_CASE(GL_TEXTURE_EXTERNAL_OES);
			#endif
		default: bug_branch("%d", target); return nullptr;
		}
		#undef GLTARGET_CASE
	}

	void bindTexture(GLenum target, GLuint texture)
	{
		GLuint *state = getBindTextureState(target);
		if(texture != *state)
		{
			//logMsg("binding texture %d to target %d", texture, target);
			glBindTexture(target, texture);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glBindTexture", err); });
			*state = texture;
		}

		/*if(target == GL_TEXTURE_EXTERNAL_OES)
		{
			logMsg("bound texture %d TEXTURE_EXTERNAL_OES", texture);
		}*/

		if(verifyState)
		{
			GLint realTexture = 0;
			handleGLErrors();
			glGetIntegerv(textureTargetToGet(target), &realTexture);
			if(!handleGLErrors([](GLenum, const char *err) { /*logWarn("%s in glGetIntegerv while verifying state", err);*/ }))
			{
				if(texture != (GLuint)realTexture)
				{
					bug_exit("out of sync, expected %u but got %u, target %d", texture, realTexture, target);
				}
			}
		}
	}

	void deleteTextures(GLsizei n, const GLuint *textures)
	{
		// From the OpenGL manual:
		// If a texture that is currently bound is deleted, the binding reverts to 0 (the default texture)
		iterateTimes(n, i)
		{
			//logMsg("deleting texture %u", textures[i]);
			if(textures[i] == bindTextureState.GL_TEXTURE_2D_state)
				bindTextureState.GL_TEXTURE_2D_state = 0;
			#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
			if(textures[i] == bindTextureState.GL_TEXTURE_EXTERNAL_OES_state)
			{
				//logMsg("is bound to TEXTURE_EXTERNAL_OES");
				bindTextureState.GL_TEXTURE_EXTERNAL_OES_state = 0;
			}
			#endif
		}

		glDeleteTextures(n, textures);
	}

	GLenum blendFuncSfactor = GL_ONE, blendFuncDfactor = GL_ZERO;
	void blendFunc(GLenum sfactor, GLenum dfactor)
	{
		if(!(sfactor == blendFuncSfactor && dfactor == blendFuncDfactor))
		{
			glBlendFunc(sfactor, dfactor);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glBlendFunc", err); });
			blendFuncSfactor = sfactor;
			blendFuncDfactor = dfactor;
		}
	}

	GLenum blendEquationState = GL_FUNC_ADD;
	void blendEquation(GLenum mode)
	{
		if(mode != blendEquationState)
		{
			glBlendEquation(mode);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glBlendEquation", err); });
			blendEquationState = mode;
		}
	}

	struct GLStateCaps
	{
		constexpr GLStateCaps() { }
		bool GL_ALPHA_TEST_state = 0;
		bool GL_DEPTH_TEST_state = 0;
		bool GL_FOG_state = 0;
		bool GL_BLEND_state = 0;
		bool GL_SCISSOR_TEST_state = 0;
		bool GL_CULL_FACE_state = 0;
		bool GL_TEXTURE_2D_state = 0;
		#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
		bool GL_TEXTURE_EXTERNAL_OES_state = 0;
		#endif
		bool GL_DITHER_state = 1;
		#ifndef CONFIG_GFX_OPENGL_ES
		// extensions
		bool GL_MULTISAMPLE_ARB_state = 0;
		#endif
	} stateCap;

	bool *getCap(GLenum cap)
	{
		#define GLCAP_CASE(cap) case cap: return &stateCap.cap ## _state
		switch(cap)
		{
			GLCAP_CASE(GL_ALPHA_TEST);
			GLCAP_CASE(GL_DEPTH_TEST);
			GLCAP_CASE(GL_FOG);
			GLCAP_CASE(GL_BLEND);
			GLCAP_CASE(GL_SCISSOR_TEST);
			GLCAP_CASE(GL_CULL_FACE);
			GLCAP_CASE(GL_TEXTURE_2D);
			#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
			GLCAP_CASE(GL_TEXTURE_EXTERNAL_OES);
			#endif
			GLCAP_CASE(GL_DITHER);
			#ifndef CONFIG_GFX_OPENGL_ES
			GLCAP_CASE(GL_MULTISAMPLE_ARB);
			#endif
		default: return 0;
		}
		#undef GLCAP_CASE
	}

	void enable(GLenum cap)
	{
		bool *state = getCap(cap);
		if(unlikely(!state))
		{
			// unmanaged cap
			logMsg("glEnable unmanaged %d", (int)cap);
			glEnable(cap);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glEnable", err); });
			return;
		}

		if(!(*state))
		{
			// not enabled
			//logMsg("glEnable %d", (int)cap);
			glEnable(cap);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glEnable", err); });
			*state = 1;
		}

		#if !defined CONFIG_BASE_PS3
		if(verifyState)
		{
			handleGLErrors();
			auto enabled = glIsEnabled(cap);
			if(!handleGLErrors([](GLenum, const char *err) { /*logWarn("%s in glIsEnabled while verifying state", err);*/ }))
			{
				if(!enabled)
				{
						bug_exit("state %d out of sync", cap);
				}
			}
		}
		#endif
	}

	void disable(GLenum cap)
	{
		bool *state = getCap(cap);
		if(unlikely(!state))
		{
			// unmanaged cap
			logMsg("glDisable unmanaged %d", (int)cap);
			glDisable(cap);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glDisable", err); });
			return;
		}

		if((*state))
		{
			// is enabled
			//logMsg("glDisable %d", (int)cap);
			glDisable(cap);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glDisable", err); });
			*state = 0;
		}

		#if !defined CONFIG_BASE_PS3
		if(verifyState)
		{
			handleGLErrors();
			auto enabled = glIsEnabled(cap);
			if(!handleGLErrors([](GLenum, const char *err) { /*logWarn("%s in glIsEnabled while verifying state", err);*/ }))
			{
				if(enabled)
				{
						bug_exit("state %d out of sync", cap);
				}
			}
		}
		#endif
	}

	GLboolean isEnabled(GLenum cap)
	{
		bool *state = getCap(cap);
		if(unlikely(!state))
		{
			// unmanaged cap
			logMsg("glIsEnabled unmanaged %d", (int)cap);
			#if !defined(CONFIG_BASE_PS3)
				return glIsEnabled(cap);
			#else
				bug_exit("glIsEnabled unsupported");
			#endif
		}

		return *state;
	}

	struct GLClientStateCaps
	{
		constexpr GLClientStateCaps() { }
		bool GL_TEXTURE_COORD_ARRAY_state = 0;
		bool GL_COLOR_ARRAY_state = 0;
	} clientStateCap;

	bool *getClientCap(GLenum cap)
	{
		#define GLCAP_CASE(cap) case cap: return &clientStateCap.cap ## _state
		switch(cap)
		{
			GLCAP_CASE(GL_TEXTURE_COORD_ARRAY);
			GLCAP_CASE(GL_COLOR_ARRAY);
		default: return 0;
		}
		#undef GLCAP_CASE
	}

	void enableClientState(GLenum cap)
	{
		bool *state = getClientCap(cap);
		if(unlikely(!state))
		{
			// unmanaged cap
			logMsg("glEnableClientState unmanaged %d", (int)cap);
			glEnableClientState(cap);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glEnableClientState", err); });
			return;
		}

		if(!(*state)) // not enabled
		{
			//logMsg("glEnableClientState %d", (int)cap);
			glEnableClientState(cap);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glEnableClientState", err); });
			*state = 1;
		}

		#if !defined CONFIG_BASE_PS3
		if(verifyState)
		{
			handleGLErrors();
			auto enabled = glIsEnabled(cap);
			if(!handleGLErrors([](GLenum, const char *err) { logWarn("%s in glIsEnabled while verifying state", err); }))
			{
				if(!enabled)
				{
						bug_exit("state %d out of sync", cap);
				}
			}
		}
		#endif
	}

	void disableClientState(GLenum cap)
	{
		bool *state = getClientCap(cap);
		if(unlikely(!state))
		{
			// unmanaged cap
			logMsg("glDisableClientState unmanaged %d", (int)cap);
			glDisableClientState(cap);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glDisableClientState", err); });
			return;
		}

		if((*state)) // is enabled
		{
			//logMsg("glDisableClientState %d", (int)cap);
			glDisableClientState(cap);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glDisableClientState", err); });
			*state = 0;
		}

		#if !defined CONFIG_BASE_PS3
		if(verifyState)
		{
			handleGLErrors();
			auto enabled = glIsEnabled(cap);
			if(!handleGLErrors([](GLenum, const char *err) { logWarn("%s in glIsEnabled while verifying state", err); }))
			{
				if(enabled)
				{
						bug_exit("state %d out of sync", cap);
				}
			}
		}
		#endif
	}

	GLint GL_TEXTURE_ENV_GL_TEXTURE_ENV_MODE_state = GL_MODULATE;
	void texEnvi(GLenum target, GLenum pname, GLint param)
	{
		if(Config::envIsPS3)
			return; // TODO: use shaders to support modes besides GL_MODULATE

		if(target == GL_TEXTURE_ENV && pname == GL_TEXTURE_ENV_MODE)
		{
			if(param != GL_TEXTURE_ENV_GL_TEXTURE_ENV_MODE_state)
			{
				GL_TEXTURE_ENV_GL_TEXTURE_ENV_MODE_state = param;
				glTexEnvi(target, pname, param);
				handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glTexEnvi", err); });
			}
		}
		else // cases we don't handle
		{
			glTexEnvi(target, pname, param);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glTexEnvi", err); });
		}
	}

	GLfloat GL_TEXTURE_ENV_GL_TEXTURE_ENV_COLOR_state[4] = { 0, 0, 0, 0 };
	void texEnvfv(GLenum target, GLenum pname, const GLfloat *params)
	{
		if(target == GL_TEXTURE_ENV && pname == GL_TEXTURE_ENV_COLOR)
		{
			if(memcmp(params, GL_TEXTURE_ENV_GL_TEXTURE_ENV_COLOR_state, sizeof(GLfloat)*4))
			{
				memcpy(GL_TEXTURE_ENV_GL_TEXTURE_ENV_COLOR_state, params, sizeof(GLfloat)*4);
				glTexEnvfv(target, pname, params);
				handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glTexEnvfv", err); });
			}
		}
		else // cases we don't handle
		{
			glTexEnvfv(target, pname, params);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glTexEnvfv", err); });
		}
	}

	GLfloat colorState[4] = { 1, 1, 1, 1 };
	void color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
	{
		if(red != colorState[0] || green != colorState[1] || blue != colorState[2] || alpha != colorState[3])
		{
			glColor4f(red, green, blue, alpha);
			colorState[0] = red; colorState[1] = green; colorState[2] = blue; colorState[3] = alpha;
		}
		else
		{
			//logMsg("glColor state cache hit");
		}
	}

	struct GLPointerVal
	{
		constexpr GLPointerVal() { }
		GLint size = 4;
		GLenum type = GL_FLOAT;
		GLsizei stride = 0;
		const GLvoid *pointer = nullptr;
	};

	static uint applyGenericPointerState(GLPointerVal *state, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
	{
		if(size != state->size
			|| type != state->type
			|| stride != state->stride
			|| pointer != state->pointer)
		{
			state->size = size;
			state->type = type;
			state->stride = stride;
			state->pointer = pointer;
			return 1;
		}
		return 0;
	}

	GLPointerVal texCoordPointerState;
	void texCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
	{
		/*		,bufferBindState = { 4, GL_FLOAT, 0, 0 };
		GLPointerVal *state = glBindBufferVal[0].buffer ? &bufferBindState : &normalState;*/

		if(applyGenericPointerState(&texCoordPointerState, size, type, stride, pointer))
		{
			glTexCoordPointer(size, type, stride, pointer);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glTexCoordPointer", err); });
		}
		else
		{
			//logMsg("cached");
		}
	}

	GLPointerVal colorPointerState;
	void colorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
	{
		//static GLPointerVal normalState = { 4, GL_FLOAT, 0, 0 };
				/*		,bufferBindState = { 4, GL_FLOAT, 0, 0 };
				GLPointerVal *state = glBindBufferVal[0].buffer ? &bufferBindState : &normalState;*/

		if(applyGenericPointerState(&colorPointerState, size, type, stride, pointer))
		{
			glColorPointer(size, type, stride, pointer);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glColorPointer", err); });
		}
		else
		{
			//logMsg("cached");
		}
	}

	GLPointerVal vertexPointerState;
	void vertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
	{
		//static GLPointerVal normalState = { 4, GL_FLOAT, 0, 0 };
				/*		,bufferBindState = { 4, GL_FLOAT, 0, 0 };
				GLPointerVal *state = glBindBufferVal[0].buffer ? &bufferBindState : &normalState;*/

		if(applyGenericPointerState(&vertexPointerState, size, type, stride, pointer))
		{
			glVertexPointer(size, type, stride, pointer);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glVertexPointer", err); });
		}
		else
		{
			//logMsg("cached");
		}
	}

	struct GLPixelStoreParams
	{
		constexpr GLPixelStoreParams() { }
		GLint GL_UNPACK_ALIGNMENT_state = 4;
		#ifndef CONFIG_GFX_OPENGL_ES
		GLint GL_UNPACK_ROW_LENGTH_state = 0;
		#endif
	} pixelStoreParam;

	GLint *getPixelStoreParam(GLenum pname)
	{
		#define GLPARAM_CASE(pname) case pname: return &pixelStoreParam.pname ## _state
		switch(pname)
		{
			GLPARAM_CASE(GL_UNPACK_ALIGNMENT);
			#ifndef CONFIG_GFX_OPENGL_ES
			GLPARAM_CASE(GL_UNPACK_ROW_LENGTH);
			#endif
		default: return 0;
		}
		#undef GLPARAM_CASE
	}

	void pixelStorei(GLenum pname, GLint param)
	{
		GLint *state = getPixelStoreParam(pname);
		if(unlikely(!state))
		{
			// unmanaged param
			logMsg("glPixelStorei unmanaged %d", (int)pname);
			glPixelStorei(pname, param);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glPixelStorei", err); });
			return;
		}

		if(*state != param)
		{
			//logMsg("glPixelStorei %d set to %d", (int)pname, param);
			glPixelStorei(pname, param);
			handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glPixelStorei", err); });
			*state = param;
			#ifndef CONFIG_GFX_OPENGL_ES
			if(pname == GL_UNPACK_ROW_LENGTH && param != 0)
				logMsg("using GL_UNPACK_ROW_LENGTH %d", (int)param);
			#endif
		}
	}
};

extern GLStateCache glState;
static const bool useGLCache = 1;

static void glcMatrixMode(GLenum mode)
{ if(useGLCache) glState.matrixMode(mode); else glMatrixMode(mode); }
static void glcBindTexture(GLenum target, GLuint texture)
{ if(useGLCache) glState.bindTexture(target, texture); else glBindTexture(target, texture); }
static void glcDeleteTextures(GLsizei n, const GLuint *textures)
{ if(useGLCache) glState.deleteTextures(n, textures); else glDeleteTextures(n, textures); }
static void glcBlendFunc(GLenum sfactor, GLenum dfactor)
{ if(useGLCache) glState.blendFunc(sfactor, dfactor); else glBlendFunc(sfactor, dfactor); }
static void glcBlendEquation(GLenum mode)
{ if(useGLCache) glState.blendEquation(mode); else glBlendEquation(mode); }
static void glcEnable(GLenum cap)
{ if(useGLCache && likely(!glEnableStateHack)) glState.enable(cap); else glEnable(cap); }
static void glcDisable(GLenum cap)
{ if(useGLCache && likely(!glEnableStateHack)) glState.disable(cap); else glDisable(cap); }
static GLboolean glcIsEnabled(GLenum cap)
{
	if(useGLCache && likely(!glEnableStateHack))
		return glState.isEnabled(cap);
	else
		#if !defined(CONFIG_BASE_PS3)
			return glIsEnabled(cap);
		#else
			bug_exit("glIsEnabled unsupported");
			return 0;
		#endif
}
static void glcEnableClientState(GLenum cap)
{ if(useGLCache) glState.enableClientState(cap); else glEnableClientState(cap); }
static void glcDisableClientState(GLenum cap)
{ if(useGLCache) glState.disableClientState(cap); else glDisableClientState(cap); }
static void glcTexEnvi(GLenum target, GLenum pname, GLint param)
{ if(useGLCache) glState.texEnvi(target, pname, param); else glTexEnvi(target, pname, param); }
static void glcTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{ if(useGLCache) glState.texEnvfv(target, pname, params); else glTexEnvfv(target, pname, params); }
static void glcColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	if(useGLCache)
		glState.color4f(red, green, blue, alpha);
	else
	{
		glColor4f(red, green, blue, alpha);
		glState.colorState[0] = red; glState.colorState[1] = green; glState.colorState[2] = blue; glState.colorState[3] = alpha; // for color()
	}
}
static void glcTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{ if(useGLCache && likely(!glPointerStateHack)) glState.texCoordPointer(size, type, stride, pointer); else glTexCoordPointer(size, type, stride, pointer); }
static void glcColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{ if(useGLCache && likely(!glPointerStateHack)) glState.colorPointer(size, type, stride, pointer); else glColorPointer(size, type, stride, pointer); }
static void glcVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{ if(useGLCache && likely(!glPointerStateHack)) glState.vertexPointer(size, type, stride, pointer); else glVertexPointer(size, type, stride, pointer); }
static void glcPixelStorei(GLenum pname, GLint param)
{ if(useGLCache) glState.pixelStorei(pname, param); else glPixelStorei(pname, param); }

typedef struct
{
	GLenum target;
	GLuint buffer;
} GLBindBufferVal;

static GLBindBufferVal glBindBufferVal[] =
{
	{ GL_ARRAY_BUFFER, 0 }, // needs to be first array element for glState_vertexPointer, etc
	{ GL_ELEMENT_ARRAY_BUFFER, 0 },
#ifndef CONFIG_GFX_OPENGL_ES
	{ GL_PIXEL_PACK_BUFFER_ARB, 0 },
	{ GL_PIXEL_UNPACK_BUFFER_ARB, 0 }
#endif
};

static void glState_bindBuffer(GLenum target, GLuint buffer)
{
	forEachInArray(glBindBufferVal, e)
	{
		if(e->target == target)
		{
			if(e->buffer != buffer)
			{
				if(target == GL_ARRAY_BUFFER
					//&& ((e->buffer == 0 && buffer != 0) || (e->buffer != 0 && buffer == 0))
				)
				{
					//reset pointer states when changing VBO state
					bug_exit("TODO");
					/*vertexPointerState.size = 0;
					colorPointerState.size = 0;
					texCoordPointerState.size = 0;*/
					//logMsg("reset pointer states");
				}

				glBindBuffer(target, buffer);
				e->buffer = buffer;
			}
			return;
		}
	}
}
