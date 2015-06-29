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

#include <imagine/gfx/Gfx.hh>
#include <imagine/util/edge.h>
#include "private.hh"

namespace Gfx
{

const uint ColVertex::colorOffset = offsetof(ColVertex, color);

const uint TexVertex::textureOffset = offsetof(TexVertex, u);

const uint ColTexVertex::colorOffset = offsetof(ColTexVertex, color);
const uint ColTexVertex::textureOffset = offsetof(ColTexVertex, u);

static_assertIsPod(Vertex);
static_assertIsPod(ColVertex);
static_assertIsPod(TexVertex);
static_assertIsPod(ColTexVertex);

static const GLenum GL_VERT_ARRAY_TYPE = GL_FLOAT;
static const GLenum GL_TEX_ARRAY_TYPE = GL_FLOAT;

#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
template<class Vtx>
static void setupVertexArrayPointers(const Vtx *v, int numV)
{
	if(useVBOFuncs && v != 0) // turn off VBO when rendering from memory
	{
		//logMsg("un-binding VBO");
		glcBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	if(Vtx::hasTexture)
	{
		glcEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glcTexCoordPointer(2, GL_TEX_ARRAY_TYPE, sizeof(Vtx), (char*)v + Vtx::textureOffset);
		//logMsg("drawing u,v %f,%f", (float)TextureCoordinate(*((TextureCoordinatePOD*)texOffset)),
		//		(float)TextureCoordinate(*((TextureCoordinatePOD*)(texOffset+4))));
	}
	else
		glcDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if(Vtx::hasColor)
	{
		glcEnableClientState(GL_COLOR_ARRAY);
		glcColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vtx), (char*)v + Vtx::colorOffset);
		glState.colorState[0] = -1; //invalidate glColor state cache
	}
	else
		glcDisableClientState(GL_COLOR_ARRAY);

	glcVertexPointer(numV, GL_VERT_ARRAY_TYPE, sizeof(Vtx), (char*)v + Vtx::posOffset);
}
#endif

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
static uint currentVtxArrayPointerID = 0;
template<class Vtx>
static void setupShaderVertexArrayPointers(const Vtx *v, int numV)
{
	if(currentVtxArrayPointerID != Vtx::ID)
	{
		//logMsg("setting vertex array pointers for type: %d", Vtx::ID);
		currentVtxArrayPointerID = Vtx::ID;
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
		glcVertexAttribPointer(VATTR_TEX_UV, 2, GL_TEX_ARRAY_TYPE, GL_FALSE, sizeof(Vtx), (char*)v + Vtx::textureOffset);
		//glUniform1i(textureUniform, 0);
		//logMsg("drawing u,v %f,%f", (float)TextureCoordinate(*((TextureCoordinatePOD*)texOffset)),
		//		(float)TextureCoordinate(*((TextureCoordinatePOD*)(texOffset+4))));
	}

	if(Vtx::hasColor)
	{
		glcVertexAttribPointer(VATTR_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vtx), (char*)v + Vtx::colorOffset);
	}

	glcVertexAttribPointer(VATTR_POS, numV, GL_VERT_ARRAY_TYPE, GL_FALSE, sizeof(Vtx), (char*)v + Vtx::posOffset);
}
#endif

template<class Vtx>
void VertexInfo::bindAttribs(const Vtx *v)
{
	if(useVBOFuncs)
		v = nullptr;
	int numV = 2; // number of position elements
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
		Gfx::setupVertexArrayPointers(v, numV);
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!useFixedFunctionPipeline)
		Gfx::setupShaderVertexArrayPointers(v, numV);
	#endif
}

void bindTempVertexBuffer()
{
	if(useVBOFuncs)
	{
		glcBindBuffer(GL_ARRAY_BUFFER, getVBO());
	}
}

void vertexBufferData(const void *v, uint size)
{
	if(useVBOFuncs)
	{
		glBufferData(GL_ARRAY_BUFFER, size, v, GL_STREAM_DRAW);
	}
}

void drawPrimitives(Primitive mode, uint start, uint count)
{
	glDrawArrays((GLenum)mode, start, count);
	handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glDrawArrays", err); });
}

void drawPrimitiveElements(Primitive mode, const VertexIndex *idx, uint count)
{
	glDrawElements((GLenum)mode, count, GL_UNSIGNED_SHORT, idx);
	handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glDrawElements", err); });
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
static void mapImg(std::array<Vtx, 4> &v, GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV)
{
	v[0].u = leftTexU; v[0].v = bottomTexV; //BL
	v[1].u = leftTexU; v[1].v = topTexV; //TL
	v[2].u = rightTexU; v[2].v = bottomTexV; //BR
	v[3].u = rightTexU; v[3].v = topTexV; //TR
}

template<class Vtx>
static void setColor(std::array<Vtx, 4> &v, VertexColor col, uint edges)
{
	if(edges & EDGE_BL) v[0].color = col;
	if(edges & EDGE_TL) v[1].color = col;
	if(edges & EDGE_TR) v[3].color = col;
	if(edges & EDGE_BR) v[2].color = col;
}

template<class Vtx>
static void setColor(std::array<Vtx, 4> &v, ColorComp r, ColorComp g, ColorComp b, ColorComp a, uint edges)
{
	if(edges & EDGE_BL) v[0].color = VertexColorPixelFormat.build((uint)r, (uint)g, (uint)b, (uint)a);
	if(edges & EDGE_TL) v[1].color = VertexColorPixelFormat.build((uint)r, (uint)g, (uint)b, (uint)a);
	if(edges & EDGE_TR) v[3].color = VertexColorPixelFormat.build((uint)r, (uint)g, (uint)b, (uint)a);
	if(edges & EDGE_BR) v[2].color = VertexColorPixelFormat.build((uint)r, (uint)g, (uint)b, (uint)a);
}

template<class Vtx>
static void setColorRGB(std::array<Vtx, 4> &v, ColorComp r, ColorComp g, ColorComp b, uint edges)
{
	if(edges & EDGE_BL) setColor(v, r, g, b, VertexColorPixelFormat.a(v[0].color), EDGE_BL);
	if(edges & EDGE_TL) setColor(v, r, g, b, VertexColorPixelFormat.a(v[1].color), EDGE_TL);
	if(edges & EDGE_TR) setColor(v, r, g, b, VertexColorPixelFormat.a(v[3].color), EDGE_TR);
	if(edges & EDGE_BR) setColor(v, r, g, b, VertexColorPixelFormat.a(v[2].color), EDGE_BR);
}

template<class Vtx>
static void setColorAlpha(std::array<Vtx, 4> &v, ColorComp a, uint edges)
{
	if(edges & EDGE_BL) setColor(v, VertexColorPixelFormat.r(v[0].color), VertexColorPixelFormat.g(v[0].color), VertexColorPixelFormat.b(v[0].color), a, EDGE_BL);
	if(edges & EDGE_TL) setColor(v, VertexColorPixelFormat.r(v[1].color), VertexColorPixelFormat.g(v[1].color), VertexColorPixelFormat.b(v[1].color), a, EDGE_TL);
	if(edges & EDGE_TR) setColor(v, VertexColorPixelFormat.r(v[3].color), VertexColorPixelFormat.g(v[3].color), VertexColorPixelFormat.b(v[3].color), a, EDGE_TR);
	if(edges & EDGE_BR) setColor(v, VertexColorPixelFormat.r(v[2].color), VertexColorPixelFormat.g(v[2].color), VertexColorPixelFormat.b(v[2].color), a, EDGE_BR);
}

template void VertexInfo::bindAttribs<Vertex>(const Vertex *v);
template void VertexInfo::bindAttribs<ColVertex>(const ColVertex *v);
template void VertexInfo::bindAttribs<TexVertex>(const TexVertex *v);
template void VertexInfo::bindAttribs<ColTexVertex>(const ColTexVertex *v);

}

#include "drawable/sprite.hh"
#include "drawable/quad.hh"
