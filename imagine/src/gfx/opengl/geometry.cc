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
#include <imagine/gfx/Vertex.hh>
#include <imagine/util/edge.h>
#include "private.hh"

namespace Gfx
{

const uint32_t ColVertex::colorOffset = offsetof(ColVertex, color);

const uint32_t TexVertex::textureOffset = offsetof(TexVertex, u);

const uint32_t ColTexVertex::colorOffset = offsetof(ColTexVertex, color);
const uint32_t ColTexVertex::textureOffset = offsetof(ColTexVertex, u);

static_assertIsStandardLayout(Vertex);
static_assertIsStandardLayout(ColVertex);
static_assertIsStandardLayout(TexVertex);
static_assertIsStandardLayout(ColTexVertex);

static const GLenum GL_VERT_ARRAY_TYPE = GL_FLOAT;
static const GLenum GL_TEX_ARRAY_TYPE = GL_FLOAT;

#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
template<class Vtx>
static void setupVertexArrayPointers(RendererCommands &cmds, const Vtx *v, int numV)
{
	if(cmds.renderer().support.hasVBOFuncs && v != 0) // turn off VBO when rendering from memory
	{
		//logMsg("un-binding VBO");
		cmds.bindGLArrayBuffer(0);
	}

	cmds.glcEnableClientState(GL_VERTEX_ARRAY);

	if(Vtx::hasTexture)
	{
		cmds.glcEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_TEX_ARRAY_TYPE, sizeof(Vtx), (char*)v + Vtx::textureOffset);
		//logMsg("drawing u,v %f,%f", (float)TextureCoordinate(*((TextureCoordinatePOD*)texOffset)),
		//		(float)TextureCoordinate(*((TextureCoordinatePOD*)(texOffset+4))));
	}
	else
		cmds.glcDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if(Vtx::hasColor)
	{
		cmds.glcEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vtx), (char*)v + Vtx::colorOffset);
		cmds.glState.colorState[0] = -1; //invalidate glColor state cache
	}
	else
		cmds.glcDisableClientState(GL_COLOR_ARRAY);

	glVertexPointer(numV, GL_VERT_ARRAY_TYPE, sizeof(Vtx), (char*)v + Vtx::posOffset);
}
#endif

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
template<class Vtx>
static void setupShaderVertexArrayPointers(RendererCommands &cmds, const Vtx *v, int numV)
{
	if(cmds.currentVtxArrayPointerID != Vtx::ID)
	{
		//logMsg("setting vertex array pointers for type: %d", Vtx::ID);
		cmds.currentVtxArrayPointerID = Vtx::ID;
		if(Vtx::hasTexture)
			glEnableVertexAttribArray(VATTR_TEX_UV);
		else
			glDisableVertexAttribArray(VATTR_TEX_UV);
		if(Vtx::hasColor)
			glEnableVertexAttribArray(VATTR_COLOR);
		else
			glDisableVertexAttribArray(VATTR_COLOR);
	}

	if(Vtx::hasTexture)
	{
		glVertexAttribPointer(VATTR_TEX_UV, 2, GL_TEX_ARRAY_TYPE, GL_FALSE, sizeof(Vtx), (char*)v + Vtx::textureOffset);
		//glUniform1i(textureUniform, 0);
		//logMsg("drawing u,v %f,%f", (float)TextureCoordinate(*((TextureCoordinatePOD*)texOffset)),
		//		(float)TextureCoordinate(*((TextureCoordinatePOD*)(texOffset+4))));
	}

	if(Vtx::hasColor)
	{
		glVertexAttribPointer(VATTR_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vtx), (char*)v + Vtx::colorOffset);
	}

	glVertexAttribPointer(VATTR_POS, numV, GL_VERT_ARRAY_TYPE, GL_FALSE, sizeof(Vtx), (char*)v + Vtx::posOffset);
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
		Gfx::setupVertexArrayPointers(cmds, v, numV);
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!cmds.renderer().support.useFixedFunctionPipeline)
		Gfx::setupShaderVertexArrayPointers(cmds, v, numV);
	#endif
}

template<class Vtx>
static void setPos(std::array<Vtx, 4> &v, GC x, GC y, GC x2, GC y2, GC x3, GC y3, GC x4, GC y4)
{
	v[0].x = x; v[0].y = y; //BL
	v[1].x = x2; v[1].y = y2; //TL
	v[2].x = x4; v[2].y = y4; //BR
	v[3].x = x3; v[3].y = y3; //TR
}

template<class Vtx>
static void setPos(std::array<Vtx, 4> &v, GC x, GC y, GC x2, GC y2)
{
	setPos(v, x, y,  x, y2,  x2, y2,  x2, y);
}

template<class Vtx>
static void setColor(std::array<Vtx, 4> &v, VertexColor col, uint32_t edges)
{
	if(edges & EDGE_BL) v[0].color = col;
	if(edges & EDGE_TL) v[1].color = col;
	if(edges & EDGE_TR) v[3].color = col;
	if(edges & EDGE_BR) v[2].color = col;
}

template<class Vtx>
static void setColor(std::array<Vtx, 4> &v, ColorComp r, ColorComp g, ColorComp b, ColorComp a, uint32_t edges)
{
	if(edges & EDGE_BL) v[0].color = VertexColorPixelFormat.build((uint32_t)r, (uint32_t)g, (uint32_t)b, (uint32_t)a);
	if(edges & EDGE_TL) v[1].color = VertexColorPixelFormat.build((uint32_t)r, (uint32_t)g, (uint32_t)b, (uint32_t)a);
	if(edges & EDGE_TR) v[3].color = VertexColorPixelFormat.build((uint32_t)r, (uint32_t)g, (uint32_t)b, (uint32_t)a);
	if(edges & EDGE_BR) v[2].color = VertexColorPixelFormat.build((uint32_t)r, (uint32_t)g, (uint32_t)b, (uint32_t)a);
}

template<class Vtx>
static void setColorRGB(std::array<Vtx, 4> &v, ColorComp r, ColorComp g, ColorComp b, uint32_t edges)
{
	if(edges & EDGE_BL) setColor(v, r, g, b, VertexColorPixelFormat.a(v[0].color), EDGE_BL);
	if(edges & EDGE_TL) setColor(v, r, g, b, VertexColorPixelFormat.a(v[1].color), EDGE_TL);
	if(edges & EDGE_TR) setColor(v, r, g, b, VertexColorPixelFormat.a(v[3].color), EDGE_TR);
	if(edges & EDGE_BR) setColor(v, r, g, b, VertexColorPixelFormat.a(v[2].color), EDGE_BR);
}

template<class Vtx>
static void setColorAlpha(std::array<Vtx, 4> &v, ColorComp a, uint32_t edges)
{
	if(edges & EDGE_BL) setColor(v, VertexColorPixelFormat.r(v[0].color), VertexColorPixelFormat.g(v[0].color), VertexColorPixelFormat.b(v[0].color), a, EDGE_BL);
	if(edges & EDGE_TL) setColor(v, VertexColorPixelFormat.r(v[1].color), VertexColorPixelFormat.g(v[1].color), VertexColorPixelFormat.b(v[1].color), a, EDGE_TL);
	if(edges & EDGE_TR) setColor(v, VertexColorPixelFormat.r(v[3].color), VertexColorPixelFormat.g(v[3].color), VertexColorPixelFormat.b(v[3].color), a, EDGE_TR);
	if(edges & EDGE_BR) setColor(v, VertexColorPixelFormat.r(v[2].color), VertexColorPixelFormat.g(v[2].color), VertexColorPixelFormat.b(v[2].color), a, EDGE_BR);
}

template void VertexInfo::bindAttribs<Vertex>(RendererCommands &cmds, const Vertex *v);
template void VertexInfo::bindAttribs<ColVertex>(RendererCommands &cmds, const ColVertex *v);
template void VertexInfo::bindAttribs<TexVertex>(RendererCommands &cmds, const TexVertex *v);
template void VertexInfo::bindAttribs<ColTexVertex>(RendererCommands &cmds, const ColTexVertex *v);

}

#include "drawable/sprite.hh"
#include "drawable/quad.hh"
