#pragma once
#include <gfx/GfxBufferImage.hh>
#include <util/edge.h>

namespace Gfx
{

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

template<class Vtx>
static void setupVertexArrayPointers(const Vtx *v, int numV)
{
	if(useVBOFuncs && v != 0) // turn off VBO when rendering from memory
	{
		//logMsg("un-binding VBO");
		glState_bindBuffer(GL_ARRAY_BUFFER, 0);
	}

	if(Vtx::hasTexture)
	{
		glcEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glcTexCoordPointer(2, GL_TEX_ARRAY_TYPE, sizeof(typename Vtx::POD), (uchar*)v + Vtx::textureOffset);
		//logMsg("drawing u,v %f,%f", (float)TextureCoordinate(*((TextureCoordinatePOD*)texOffset)),
		//		(float)TextureCoordinate(*((TextureCoordinatePOD*)(texOffset+4))));
	}
	else
		glcDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if(Vtx::hasColor)
	{
		glcEnableClientState(GL_COLOR_ARRAY);
		glcColorPointer(4, GL_UNSIGNED_BYTE, sizeof(typename Vtx::POD), (uchar*)v + Vtx::colorOffset);
		glState.colorState[0] = -1; //invalidate glColor state cache
	}
	else
		glcDisableClientState(GL_COLOR_ARRAY);

	glcVertexPointer(numV, GL_VERT_ARRAY_TYPE, sizeof(typename Vtx::POD), (uchar*)v + Vtx::posOffset);
}

}

template<class Vtx>
void VertexInfo::draw(const Vtx *v, uint type, uint count)
{
	int numV = 2; // number of position elements
	int glType = Gfx::convGeomTypeToOGL(type);

	Gfx::setupVertexArrayPointers(v, numV);
	glDrawArrays(glType, 0, count);
	handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glDrawArrays", err); });
}

template<class Vtx>
void VertexInfo::draw(const Vtx *v, const VertexIndex *idx, uint type, uint count)
{
	//logMsg("drawing with idx");
	int numV = 2; // number of position elements
	int glType = Gfx::convGeomTypeToOGL(type);

	Gfx::setupVertexArrayPointers(v, numV);
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
	//vArr.write(v, sizeof(v));
}

template<class Vtx>
static void mapImg(Vtx v[4], const Gfx::TextureDesc &img)
{
	TextureCoordinate leftTexU = img.xStart;
	TextureCoordinate topTexV = img.yStart;
	TextureCoordinate rightTexU = img.xEnd;
	TextureCoordinate bottomTexV = img.yEnd;
	mapImg(v, leftTexU, topTexV, rightTexU, bottomTexV);
}

template<class Vtx>
static void setColor(Vtx v[4], GColor r, GColor g, GColor b, GColor a, uint edges)
{
	if(edges & EDGE_BL) v[0].color = VertexColorPixelFormat.build((uint)r, (uint)g, (uint)b, (uint)a);
	if(edges & EDGE_TL) v[1].color = VertexColorPixelFormat.build((uint)r, (uint)g, (uint)b, (uint)a);
	if(edges & EDGE_TR) v[3].color = VertexColorPixelFormat.build((uint)r, (uint)g, (uint)b, (uint)a);
	if(edges & EDGE_BR) v[2].color = VertexColorPixelFormat.build((uint)r, (uint)g, (uint)b, (uint)a);
}

template<class Vtx>
static void setColorRGB(Vtx v[4], GColor r, GColor g, GColor b, uint edges)
{
	if(edges & EDGE_BL) setColor(v, r, g, b, VertexColorPixelFormat.a(v[0].color), EDGE_BL);
	if(edges & EDGE_TL) setColor(v, r, g, b, VertexColorPixelFormat.a(v[1].color), EDGE_TL);
	if(edges & EDGE_TR) setColor(v, r, g, b, VertexColorPixelFormat.a(v[3].color), EDGE_TR);
	if(edges & EDGE_BR) setColor(v, r, g, b, VertexColorPixelFormat.a(v[2].color), EDGE_BR);
}

template<class Vtx>
static void setColorAlpha(Vtx v[4], GColor a, uint edges)
{
	if(edges & EDGE_BL) setColor(v, VertexColorPixelFormat.r(v[0].color), VertexColorPixelFormat.g(v[0].color), VertexColorPixelFormat.b(v[0].color), a, EDGE_BL);
	if(edges & EDGE_TL) setColor(v, VertexColorPixelFormat.r(v[1].color), VertexColorPixelFormat.g(v[1].color), VertexColorPixelFormat.b(v[1].color), a, EDGE_TL);
	if(edges & EDGE_TR) setColor(v, VertexColorPixelFormat.r(v[3].color), VertexColorPixelFormat.g(v[3].color), VertexColorPixelFormat.b(v[3].color), a, EDGE_TR);
	if(edges & EDGE_BR) setColor(v, VertexColorPixelFormat.r(v[2].color), VertexColorPixelFormat.g(v[2].color), VertexColorPixelFormat.b(v[2].color), a, EDGE_BR);
}

#include "drawable/sprite.hh"
#include "drawable/quad.hh"

#if defined(CONFIG_RESOURCE_FACE)
	#include <gfx/common/GfxText.hh>
#endif

#include <gfx/common/GeomQuadMesh.hh>
