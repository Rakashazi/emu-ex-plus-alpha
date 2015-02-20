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

static int convGeomTypeToOGL(uint type)
{
	int glType = type == TRIANGLE ? GL_TRIANGLES :
			type == TRIANGLE_STRIP ? GL_TRIANGLE_STRIP :
			#ifndef CONFIG_GFX_OPENGL_ES
			type == QUAD ? GL_QUADS :
			#else
			type == QUAD ? GL_TRIANGLE_FAN : // Warning: this will only work if drawing one quad
			#endif
			GL_INVALID_ENUM;
	assert(glType != GL_INVALID_ENUM);
	return glType;
}

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
void VertexInfo::draw(const Vtx *v, uint type, uint count)
{
	int numV = 2; // number of position elements
	int glType = Gfx::convGeomTypeToOGL(type);

	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
		Gfx::setupVertexArrayPointers(v, numV);
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!useFixedFunctionPipeline)
		Gfx::setupShaderVertexArrayPointers(v, numV);
	#endif
	glDrawArrays(glType, 0, count);
	handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glDrawArrays", err); });
}

template<class Vtx>
void VertexInfo::draw(const Vtx *v, const VertexIndex *idx, uint type, uint count)
{
	//logMsg("drawing with idx");
	int numV = 2; // number of position elements
	int glType = Gfx::convGeomTypeToOGL(type);

	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
		Gfx::setupVertexArrayPointers(v, numV);
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!useFixedFunctionPipeline)
		Gfx::setupShaderVertexArrayPointers(v, numV);
	#endif
	glDrawElements(glType, count, GL_UNSIGNED_SHORT, idx);
	handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glDrawElements", err); });
}

template<class Vtx>
static void mapImg(Vtx v[4], GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV)
{
	v[0].u = leftTexU; v[0].v = bottomTexV; //BL
	v[1].u = leftTexU; v[1].v = topTexV; //TL
	v[2].u = rightTexU; v[2].v = bottomTexV; //BR
	v[3].u = rightTexU; v[3].v = topTexV; //TR
}

template<class Vtx>
static void setColor(Vtx v[4], ColorComp r, ColorComp g, ColorComp b, ColorComp a, uint edges)
{
	if(edges & EDGE_BL) v[0].color = VertexColorPixelFormat.build((uint)r, (uint)g, (uint)b, (uint)a);
	if(edges & EDGE_TL) v[1].color = VertexColorPixelFormat.build((uint)r, (uint)g, (uint)b, (uint)a);
	if(edges & EDGE_TR) v[3].color = VertexColorPixelFormat.build((uint)r, (uint)g, (uint)b, (uint)a);
	if(edges & EDGE_BR) v[2].color = VertexColorPixelFormat.build((uint)r, (uint)g, (uint)b, (uint)a);
}

template<class Vtx>
static void setColorRGB(Vtx v[4], ColorComp r, ColorComp g, ColorComp b, uint edges)
{
	if(edges & EDGE_BL) setColor(v, r, g, b, VertexColorPixelFormat.a(v[0].color), EDGE_BL);
	if(edges & EDGE_TL) setColor(v, r, g, b, VertexColorPixelFormat.a(v[1].color), EDGE_TL);
	if(edges & EDGE_TR) setColor(v, r, g, b, VertexColorPixelFormat.a(v[3].color), EDGE_TR);
	if(edges & EDGE_BR) setColor(v, r, g, b, VertexColorPixelFormat.a(v[2].color), EDGE_BR);
}

template<class Vtx>
static void setColorAlpha(Vtx v[4], ColorComp a, uint edges)
{
	if(edges & EDGE_BL) setColor(v, VertexColorPixelFormat.r(v[0].color), VertexColorPixelFormat.g(v[0].color), VertexColorPixelFormat.b(v[0].color), a, EDGE_BL);
	if(edges & EDGE_TL) setColor(v, VertexColorPixelFormat.r(v[1].color), VertexColorPixelFormat.g(v[1].color), VertexColorPixelFormat.b(v[1].color), a, EDGE_TL);
	if(edges & EDGE_TR) setColor(v, VertexColorPixelFormat.r(v[3].color), VertexColorPixelFormat.g(v[3].color), VertexColorPixelFormat.b(v[3].color), a, EDGE_TR);
	if(edges & EDGE_BR) setColor(v, VertexColorPixelFormat.r(v[2].color), VertexColorPixelFormat.g(v[2].color), VertexColorPixelFormat.b(v[2].color), a, EDGE_BR);
}

template void VertexInfo::draw<Vertex>(const Vertex *v, uint type, uint count);
template void VertexInfo::draw<Vertex>(const Vertex *v, const VertexIndex *idx, uint type, uint count);
template void VertexInfo::draw<ColVertex>(const ColVertex *v, uint type, uint count);
template void VertexInfo::draw<ColVertex>(const ColVertex *v, const VertexIndex *idx, uint type, uint count);
template void VertexInfo::draw<TexVertex>(const TexVertex *v, uint type, uint count);
template void VertexInfo::draw<TexVertex>(const TexVertex *v, const VertexIndex *idx, uint type, uint count);
template void VertexInfo::draw<ColTexVertex>(const ColTexVertex *v, uint type, uint count);
template void VertexInfo::draw<ColTexVertex>(const ColTexVertex *v, const VertexIndex *idx, uint type, uint count);

}

#include "drawable/sprite.hh"
#include "drawable/quad.hh"
