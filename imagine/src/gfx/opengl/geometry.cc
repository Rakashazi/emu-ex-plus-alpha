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

#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/Vertex.hh>
#include <imagine/util/edge.h>
#include <imagine/util/typeTraits.hh>
#include <imagine/logger/logger.h>
#include "internalDefs.hh"

namespace IG::Gfx
{

const uint32_t ColVertex::colorOffset = offsetof(ColVertex, color);

const uint32_t TexVertex::textureOffset = offsetof(TexVertex, u);

const uint32_t ColTexVertex::colorOffset = offsetof(ColTexVertex, color);
const uint32_t ColTexVertex::textureOffset = offsetof(ColTexVertex, u);

static_assertIsStandardLayout(Vertex2D);
static_assertIsStandardLayout(ColVertex);
static_assertIsStandardLayout(TexVertex);
static_assertIsStandardLayout(ColTexVertex);

static const GLenum GL_VERT_ARRAY_TYPE = GL_FLOAT;
static const GLenum GL_TEX_ARRAY_TYPE = GL_FLOAT;

#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
template<class Vtx>
static void setupVertexArrayPointers(RendererCommands &cmds, const Vtx *v, int numV)
{
	cmds.setupVertexArrayPointers((const char*)v, numV, sizeof(Vtx), Vtx::textureOffset, Vtx::colorOffset, Vtx::posOffset,
		Vtx::hasTexture, Vtx::hasColor);
}

void GLRendererCommands::setupVertexArrayPointers(const char *v, int numV, int stride,
	int textureOffset, int colorOffset, int posOffset,
	bool hasTexture, bool hasColor)
{
	if(r->support.hasVBOFuncs && v) // turn off VBO when rendering from memory
	{
		//logMsg("un-binding VBO");
		bindGLArrayBuffer(0);
	}

	glcEnableClientState(GL_VERTEX_ARRAY);

	if(hasTexture)
	{
		glcEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_TEX_ARRAY_TYPE, stride, v + textureOffset);
		//logMsg("drawing u,v %f,%f", (float)TextureCoordinate(*((TextureCoordinatePOD*)texOffset)),
		//		(float)TextureCoordinate(*((TextureCoordinatePOD*)(texOffset+4))));
	}
	else
		glcDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if(hasColor)
	{
		glcEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_UNSIGNED_BYTE, stride, v + colorOffset);
		glState.colorState[0] = -1; //invalidate glColor state cache
	}
	else
		glcDisableClientState(GL_COLOR_ARRAY);

	glVertexPointer(numV, GL_VERT_ARRAY_TYPE, stride, v + posOffset);
}
#endif

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
template<class Vtx>
static void setupShaderVertexArrayPointers(RendererCommands &cmds, const Vtx *v, int numV)
{
	cmds.setupShaderVertexArrayPointers((const char*)v, numV, sizeof(Vtx), Vtx::ID,
		Vtx::textureOffset, Vtx::colorOffset, Vtx::posOffset,
		Vtx::hasTexture, Vtx::hasColor);
}

void GLRendererCommands::setupShaderVertexArrayPointers(const char *v, int numV, int stride, int id,
	int textureOffset, int colorOffset, int posOffset, bool hasTexture, bool hasColor)
{
	if(currentVtxArrayPointerID != id)
	{
		//logMsg("setting vertex array pointers for type: %d", id);
		currentVtxArrayPointerID = id;
		if(hasTexture)
			glEnableVertexAttribArray(VATTR_TEX_UV);
		else
			glDisableVertexAttribArray(VATTR_TEX_UV);
		if(hasColor)
			glEnableVertexAttribArray(VATTR_COLOR);
		else
			glDisableVertexAttribArray(VATTR_COLOR);
	}

	if(hasTexture)
	{
		glVertexAttribPointer(VATTR_TEX_UV, 2, GL_TEX_ARRAY_TYPE, GL_FALSE, stride, v + textureOffset);
		//glUniform1i(textureUniform, 0);
		//logMsg("drawing u,v %f,%f", (float)TextureCoordinate(*((TextureCoordinatePOD*)texOffset)),
		//		(float)TextureCoordinate(*((TextureCoordinatePOD*)(texOffset+4))));
	}

	if(hasColor)
	{
		glVertexAttribPointer(VATTR_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, v + colorOffset);
	}

	glVertexAttribPointer(VATTR_POS, numV, GL_VERT_ARRAY_TYPE, GL_FALSE, stride, v + posOffset);
}

#endif

template<class Vtx>
void VertexInfo::bindAttribs(RendererCommands &cmds, const Vtx *v)
{
	if(cmds.renderer().support.hasVBOFuncs)
		v = nullptr;
	int numV = 2; // number of position elements
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(cmds.renderer().support.useFixedFunctionPipeline)
		setupVertexArrayPointers(cmds, v, numV);
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!cmds.renderer().support.useFixedFunctionPipeline)
		setupShaderVertexArrayPointers(cmds, v, numV);
	#endif
}

template void VertexInfo::bindAttribs<Vertex2D>(RendererCommands &cmds, const Vertex2D *v);
template void VertexInfo::bindAttribs<ColVertex>(RendererCommands &cmds, const ColVertex *v);
template void VertexInfo::bindAttribs<TexVertex>(RendererCommands &cmds, const TexVertex *v);
template void VertexInfo::bindAttribs<ColTexVertex>(RendererCommands &cmds, const ColTexVertex *v);

}
