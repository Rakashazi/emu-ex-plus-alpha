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

#include "GLStateCache.hh"
#include "utils.h"

bool GLStateCache::verifyState = false;

#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
void GLStateCache::matrixMode(GLenum mode)
{
	handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s before glMatrixMode", err); });
	if(mode != matrixModeState)
	{
		glMatrixMode(mode);
		handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glMatrixMode", err); });
		matrixModeState = mode;
	}
}
#endif

GLuint *GLStateCache::getBindTextureState(GLenum target)
{
	#define GLTARGET_CASE(target) case target: return &bindTextureState.target ## _state
	switch(target)
	{
		GLTARGET_CASE(GL_TEXTURE_2D);
		#ifdef CONFIG_GFX_OPENGL_MULTIPLE_TEXTURE_TARGETS
		GLTARGET_CASE(GL_TEXTURE_EXTERNAL_OES);
		#endif
	default: bug_branch("%d", target); return nullptr;
	}
	#undef GLTARGET_CASE
}

void GLStateCache::bindTexture(GLenum target, GLuint texture)
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

void GLStateCache::deleteTextures(GLsizei n, const GLuint *textures)
{
	// From the OpenGL manual:
	// If a texture that is currently bound is deleted, the binding reverts to 0 (the default texture)
	iterateTimes(n, i)
	{
		//logMsg("deleting texture %u", textures[i]);
		if(textures[i] == bindTextureState.GL_TEXTURE_2D_state)
			bindTextureState.GL_TEXTURE_2D_state = 0;
		#ifdef CONFIG_GFX_OPENGL_MULTIPLE_TEXTURE_TARGETS
		if(textures[i] == bindTextureState.GL_TEXTURE_EXTERNAL_OES_state)
		{
			//logMsg("is bound to TEXTURE_EXTERNAL_OES");
			bindTextureState.GL_TEXTURE_EXTERNAL_OES_state = 0;
		}
		#endif
	}

	glDeleteTextures(n, textures);
}

void GLStateCache::blendFunc(GLenum sfactor, GLenum dfactor)
{
	if(!(sfactor == blendFuncSfactor && dfactor == blendFuncDfactor))
	{
		glBlendFunc(sfactor, dfactor);
		handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glBlendFunc", err); });
		blendFuncSfactor = sfactor;
		blendFuncDfactor = dfactor;
	}
}

void GLStateCache::blendEquation(GLenum mode)
{
	if(mode != blendEquationState)
	{
		glBlendEquation(mode);
		handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glBlendEquation", err); });
		blendEquationState = mode;
	}
}

bool *GLStateCache::getCap(GLenum cap)
{
	#define GLCAP_CASE(cap) case cap: return &stateCap.cap ## _state
	switch(cap)
	{
		#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
		GLCAP_CASE(GL_ALPHA_TEST);
		GLCAP_CASE(GL_FOG);
		GLCAP_CASE(GL_TEXTURE_2D);
			#ifdef CONFIG_GFX_OPENGL_MULTIPLE_TEXTURE_TARGETS
			GLCAP_CASE(GL_TEXTURE_EXTERNAL_OES);
			#endif
		#endif
		GLCAP_CASE(GL_DEPTH_TEST);
		GLCAP_CASE(GL_BLEND);
		GLCAP_CASE(GL_SCISSOR_TEST);
		GLCAP_CASE(GL_CULL_FACE);
		GLCAP_CASE(GL_DITHER);
		#ifndef CONFIG_GFX_OPENGL_ES
		GLCAP_CASE(GL_MULTISAMPLE);
		#endif
	default: return 0;
	}
	#undef GLCAP_CASE
}

void GLStateCache::enable(GLenum cap)
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
}

void GLStateCache::disable(GLenum cap)
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
}

GLboolean GLStateCache::isEnabled(GLenum cap)
{
	bool *state = getCap(cap);
	if(unlikely(!state))
	{
		// unmanaged cap
		logMsg("glIsEnabled unmanaged %d", (int)cap);
		return glIsEnabled(cap);
	}

	return *state;
}

uint GLStateCache::applyGenericPointerState(GLPointerVal &state, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	if(size != state.size
		|| pointer != state.pointer
		|| type != state.type
		|| stride != state.stride)
	{
		state.size = size;
		state.type = type;
		state.stride = stride;
		state.pointer = pointer;
		return 1;
	}
	return 0;
}

#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
bool *GLStateCache::getClientCap(GLenum cap)
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

void GLStateCache::enableClientState(GLenum cap)
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
}

void GLStateCache::disableClientState(GLenum cap)
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
}

void GLStateCache::texEnvi(GLenum target, GLenum pname, GLint param)
{
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

void GLStateCache::texEnvfv(GLenum target, GLenum pname, const GLfloat *params)
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

void GLStateCache::color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
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

void GLStateCache::texCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	/*		,bufferBindState = { 4, GL_FLOAT, 0, 0 };
	GLPointerVal *state = glBindBufferVal[0].buffer ? &bufferBindState : &normalState;*/
	if(vboIsBound())
	{
		glTexCoordPointer(size, type, stride, pointer);
		return;
	}
	if(applyGenericPointerState(texCoordPointerState, size, type, stride, pointer))
	{
		glTexCoordPointer(size, type, stride, pointer);
		handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glTexCoordPointer", err); });
	}
	else
	{
		//logMsg("cached");
	}
}

void GLStateCache::colorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	//static GLPointerVal normalState = { 4, GL_FLOAT, 0, 0 };
			/*		,bufferBindState = { 4, GL_FLOAT, 0, 0 };
			GLPointerVal *state = glBindBufferVal[0].buffer ? &bufferBindState : &normalState;*/
	if(vboIsBound())
	{
		glColorPointer(size, type, stride, pointer);
		return;
	}
	if(applyGenericPointerState(colorPointerState, size, type, stride, pointer))
	{
		glColorPointer(size, type, stride, pointer);
		handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glColorPointer", err); });
	}
	else
	{
		//logMsg("cached");
	}
}

void GLStateCache::vertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	//static GLPointerVal normalState = { 4, GL_FLOAT, 0, 0 };
			/*		,bufferBindState = { 4, GL_FLOAT, 0, 0 };
			GLPointerVal *state = glBindBufferVal[0].buffer ? &bufferBindState : &normalState;*/
	if(vboIsBound())
	{
		glVertexPointer(size, type, stride, pointer);
		return;
	}
	if(applyGenericPointerState(vertexPointerState, size, type, stride, pointer))
	{
		glVertexPointer(size, type, stride, pointer);
		handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glVertexPointer", err); });
	}
	else
	{
		//logMsg("cached");
	}
}
#endif

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
void GLStateCache::vertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer)
{
	if(index >= sizeofArray(vertexAttribPointerState) || vboIsBound())
	{
		// uncached
		glVertexAttribPointer(index, size, type, normalized, stride, pointer);
		handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glVertexAttribPointer", err); });
		return;
	}
	auto &state = vertexAttribPointerState[index];
	if(pointer != state.pointer
		|| size != state.size
		|| type != state.type
		|| normalized != state.normalized
		|| stride != state.stride)
	{
		glVertexAttribPointer(index, size, type, normalized, stride, pointer);
		handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glVertexAttribPointer", err); });
		state.size = size;
		state.type = type;
		state.stride = stride;
		state.normalized = normalized;
		state.pointer = pointer;
	}
	else
	{
		//logMsg("cached");
	}
}
#endif

bool GLStateCache::vboIsBound()
{
	return glBindBufferVal[0].buffer;
}

void GLStateCache::bindBuffer(GLenum target, GLuint buffer)
{
	forEachInArray(glBindBufferVal, e)
	{
		if(e->target == target)
		{
			if(e->buffer != buffer)
			{
				//logMsg("binding buffer %u to target %u", buffer, target);
				glBindBuffer(target, buffer);
				e->buffer = buffer;
			}
			return;
		}
	}
}

GLint *GLStateCache::getPixelStoreParam(GLenum pname)
{
	#define GLPARAM_CASE(pname) case pname: return &pixelStoreParam.pname ## _state
	switch(pname)
	{
		GLPARAM_CASE(GL_UNPACK_ALIGNMENT);
		#ifdef HAS_UNPACK_ROW_LENGTH
		GLPARAM_CASE(GL_UNPACK_ROW_LENGTH);
		#endif
	default: return 0;
	}
	#undef GLPARAM_CASE
}

void GLStateCache::pixelStorei(GLenum pname, GLint param)
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
	}
}
