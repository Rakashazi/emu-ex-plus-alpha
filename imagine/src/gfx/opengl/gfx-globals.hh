#pragma once

#include <util/fixed.hh>
#include <util/normalFloat.hh>
#include "glIncludes.h"

/*#ifdef CONFIG_BASE_ANDROID
	typedef Fixed16S16 TransformCoordinate;
	typedef Fixed16S16POD TransformCoordinatePOD;
	typedef Fixed16S16 Angle;
	typedef Fixed16S16POD AnglePOD;
	typedef Fixed16S16 VertexPos;
	typedef Fixed16S16POD VertexPosPOD;
	#define GL_VERT_ARRAY_TYPE GL_FIXED
	typedef Fixed16S16 TextureCoordinate;
	typedef Fixed16S16POD TextureCoordinatePOD;
	#define GL_TEX_ARRAY_TYPE GL_FIXED
	#define GL_USE_OES_FIXED
#else*/
	typedef float TransformCoordinate;
	typedef float TransformCoordinatePOD;
	typedef float VertexPos;
	typedef float VertexPosPOD;
	typedef float Angle;
	typedef float AnglePOD;
	typedef float TextureCoordinate;
	typedef float TextureCoordinatePOD;
//#endif

#define angle_fromDegree(deg)  ((Angle)(deg))
#define angle_fromRadian(rad)  angle_fromDegree(toDegrees(rad))

static const uint gColor_steps = 255;
typedef NormalFloat<gColor_steps> GColor;
typedef NormalFloat<gColor_steps> GColorD;
//typedef NormalFixed<int32, 16, int64, gColor_steps> GColor;
//typedef NormalFixed<int32, 16, int64, gColor_steps> GColorD;

typedef uint GfxTextureHandle;

typedef ushort VertexIndex;
typedef uint VertexColor;
typedef uint VertexArrayRef;

#ifdef CONFIG_BASE_PS3
#define VertexColorPixelFormat PixelFormatRGBA8888
#else
#define VertexColorPixelFormat PixelFormatABGR8888
#endif

class VertexInfo
{
public:
	static const uint posOffset = 0;
	static const bool hasColor = 0;
	static const uint colorOffset = 0;
	static const bool hasTexture = 0;
	static const uint textureOffset = 0;
	template<class Vtx>
	static void draw(const Vtx *v, uint type, uint count);
	template<class Vtx>
	static void draw(const Vtx *v, const VertexIndex *idx, uint type, uint count);
};

struct VertexPOD
{
	VertexPosPOD x,y;
};

class Vertex : public VertexPOD, public VertexInfo
{
public:
	constexpr Vertex(): VertexPOD() { }
	Vertex(VertexPos x, VertexPos y)
	{
		this->x = x;
		this->y = y;
	}
	typedef VertexPOD POD;
};

struct ColVertexPOD
{
	VertexPosPOD x,y;
	uint color;
};

class ColVertex : public ColVertexPOD, public VertexInfo
{
public:
	constexpr ColVertex(): ColVertexPOD() { }
	ColVertex(VertexPos x, VertexPos y, uint color = 0)
	{
		this->x = x;
		this->y = y;
		this->color = color;
	}
	typedef ColVertexPOD POD;
	static const bool hasColor = 1;
	static const uint colorOffset = offsetof(ColVertexPOD, color);
};

struct TexVertexPOD
{
	VertexPosPOD x,y;
	TextureCoordinatePOD u,v;
};

class TexVertex : public TexVertexPOD, public VertexInfo
{
public:
	constexpr TexVertex(): TexVertexPOD() { }
	TexVertex(VertexPos x, VertexPos y, TextureCoordinate u = 0, TextureCoordinate v = 0)
	{
		this->x = x;
		this->y = y;
		this->u = u;
		this->v = v;
	}
	typedef TexVertexPOD POD;
	static const bool hasTexture = 1;
	static const uint textureOffset = offsetof(TexVertexPOD, u);
};

struct ColTexVertexPOD
{
	VertexPosPOD x,y;
	TextureCoordinatePOD u,v;
	uint color;
};

class ColTexVertex : public ColTexVertexPOD, public VertexInfo
{
public:
	constexpr ColTexVertex(): ColTexVertexPOD() { }
	ColTexVertex(VertexPos x, VertexPos y, uint color = 0, TextureCoordinate u = 0, TextureCoordinate v = 0)
	{
		this->x = x;
		this->y = y;
		this->color = color;
		this->u = u;
		this->v = v;
	}
	typedef ColTexVertexPOD POD;
	static const bool hasColor = 1;
	static const uint colorOffset = offsetof(ColTexVertexPOD, color);
	static const bool hasTexture = 1;
	static const uint textureOffset = offsetof(ColTexVertexPOD, u);
};
