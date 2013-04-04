#pragma once

#include "defs.hh"
#include "viewport.hh"
#include <util/Matrix4x4.hh>
#include <util/rectangle2.h>

namespace Gfx
{
struct Projector
{
	Rect2<GC> rect;
	GC w = 0, h = 0,
		focal = 0,
		xToPixScale = 0, yToPixScale = 0, // screen -> projection space at focal z
		pixToXScale = 0, pixToYScale = 0, // projection -> screen space at focal z
		mmToXScale = 0, mmToYScale = 0,   // MM of screen -> projection space at focal z
#ifdef CONFIG_BASE_ANDROID
		smmToXScale = 0, smmToYScale = 0,
#endif
		aspectRatio = 0;
	Matrix4x4<GC> mat, matInv;

	constexpr Projector() { }

	GC wHalf() const
	{
		return rect.x2;
	}

	GC hHalf() const
	{
		return rect.y;
	}

	void updateMMSize()
	{
		mmToXScale = w/(GC)viewMMWidth();
		mmToYScale = h/(GC)viewMMHeight();
#ifdef CONFIG_BASE_ANDROID
		smmToXScale = w/(GC)viewSMMWidth();
		smmToYScale = h/(GC)viewSMMHeight();
#endif
		//logMsg("projector to mm %fx%f", (double)mmToXScale, (double)mmToYScale);
	}

	void setMatrix(Matrix4x4<GC> &mat, bool isSideways)
	{
		var_selfs(mat);
		mat.invert(matInv);

		GC x = 0,y = 0,z = .5;
		unProject(x,y,z);
		//logMsg("Lower-left projection point %f %f %f",x,y,z);
		GC x2 = viewPixelWidth(),y2 = viewPixelHeight(),z2 = .5;
		unProject(x2,y2,z2);
		//logMsg("Upper-right projection point %f %f %f",x2,y2,z2);
		w = x2 - x, h = y2 - y;
		/*if(isSideways)
			IG::swap(w, h);*/
		rect.x = -w/2.;
		rect.y = h/2.;
		rect.x2 = w/2.;
		rect.y2 = -h/2.;
		pixToXScale = w / (GC)viewPixelWidth();
		pixToYScale = h / (GC)viewPixelHeight();
		xToPixScale = (GC)viewPixelWidth() / w;
		yToPixScale = (GC)viewPixelHeight() / h;
		logMsg("projector size %fx%f, to pix %fx%f, to view %fx%f",
			(double)w, (double)h, (double)xToPixScale, (double)yToPixScale, (double)pixToXScale, (double)pixToYScale);
	}

	bool project(GC objx, GC objy, GC objz, GC &winx, GC &winy, GC &winz)
	{
		const Vector4d<GC> in = { { { objx, objy, objz, 1.0 } } };
		Vector4d<GC> out;

		mat.mult(in, out);
		if (out[3] == 0.0)
		{
			logErr("projection failed, output %f, %f, %f, %f", (double)out.x, (double)out.y, (double)out.z, (double)out.w);
			return 0;
		}
		//logMsg("vec x proj, %f, %f, %f, %f", out.x, out.y, out.z, out.w);
		out[0] /= out[3];
		out[1] /= out[3];
		out[2] /= out[3];

		out[0] = out[0] * 0.5 + 0.5;
		out[1] = out[1] * 0.5 + 0.5;
		out[2] = out[2] * 0.5 + 0.5;
		//logMsg("mapped to range 0-1, %f, %f, %f, %f", out.x, out.y, out.z, out.w);


		out[0] = out[0] * viewPixelWidth() + 0; // X viewport
		out[1] = out[1] * viewPixelHeight() + 0; // Y viewport
		//logMsg("mapped to viewport, %f, %f, %f, %f", out.x, out.y, out.z, out.w);

		winx=out[0];
		winy=out[1];
		winz=out[2];
		return 1;
	}

	bool project(GC &objx, GC &objy, GC &objz)
	{
		return project(objx, objy, objz, objx, objy, objz);
	}

	bool unProject(GC winx, GC winy, GC winz, GC &objx, GC &objy, GC &objz)
	{
		Vector4d<GC> in = { { { winx, winy, winz, 1.0 } } };
		Vector4d<GC> out;

		// Map x and y from window coordinates
		in[0] = (in[0] - 0) / viewPixelWidth(); // X viewport
		in[1] = (in[1] - 0) / viewPixelHeight(); // Y viewport

		// Map to range -1 to 1
		in[0] = in[0] * 2 - 1;
		in[1] = in[1] * 2 - 1;
		in[2] = in[2] * 2 - 1;

		matInv.mult(in, out);
		if (out[3] == 0.0) return 0;
		out[0] /= out[3];
		out[1] /= out[3];
		out[2] /= out[3];
		objx = out[0];
		objy = out[1];
		objz = out[2];
		return 1;
	}

	bool unProject(GC &objx, GC &objy, GC &objz)
	{
		return unProject(objx, objy, objz, objx, objy, objz);
	}

	Rect2<GC> relRect(GC newX, GC newY, GC xSize, GC ySize, _2DOrigin posOrigin, _2DOrigin screenOrigin)
	{
		// adjust to the requested origin on the screen
		newX = C2DO.adjustX(newX, w, screenOrigin);
		newY = C2DO.adjustY(newY, h, screenOrigin);
		Rect2<GC> rect;
		rect.setPosRel(newX, newY, xSize, ySize, posOrigin);
		return rect;
	}
};

extern Projector proj;

// convert screen/input-space to projection-space
static GC iXSize(int x)
{
	GC r = (GC)x * proj.pixToXScale;
	//logMsg("project x %d, to %f", x, r);
	return r;
}

static GC iYSize(int y)
{
	GC r = (GC)y * proj.pixToYScale;
	//logMsg("project y %d, to %f", y, r);
	return r;
}

static GC iXPos(int x)
{
	return iXSize(x) - proj.wHalf();
	//return x - GC(viewPixelWidth())/GC(2);
}

static GC iYPos(int y)
{
	return -iYSize(y) + proj.hHalf();
	//return -y + GC(viewPixelHeight())/GC(2);
}

// convert projection-space to screen/input-space
static int toIXSize(GC x)
{
	int r = int(roundf(x * proj.xToPixScale));
	//if(r) logMsg("unproject x %f, to %d", x, r);
	return r;
}

static int toIYSize(GC y)
{
	int r = int(roundf(y * proj.yToPixScale));
	//if(r) logMsg("unproject y %f, to %d", y, r);
	return r;
}

static int toIXPos(GC x)
{
	//logMsg("unproject x %f", x);
	return toIXSize(x + proj.wHalf());
	//return x + GC(viewPixelWidth())/GC(2);
}

static int toIYPos(GC y)
{
	//logMsg("unproject y %f", y);
	return toIYSize(-(y - proj.hHalf()));
	//return -(y - GC(viewPixelHeight())/GC(2));
}

// TODO: remove
static Rect2<GC> unProjectRect(int x, int y, int x2, int y2)
{
	Rect2<GC> r;
	r.x = Gfx::iXPos(x);
	r.y = Gfx::iYPos(y2); // flip y-axis
	r.x2 = Gfx::iXPos(x2);
	r.y2 = Gfx::iYPos(y);
	return r;
}

// TODO: remove
static Rect2<GC> unProjectRect(const Rect2<int> &src)
{
	return unProjectRect(src.x, src.y, src.x2, src.y2);
}

static Rect2<int> projectRect2(const Rect2<GC> &src)
{
	Rect2<int> r;
	r.x = Gfx::toIXPos(src.x);
	r.y = Gfx::toIYPos(src.y);
	r.x2 = Gfx::toIXPos(src.x2);
	r.y2 = Gfx::toIYPos(src.y2);
	return r;
}

static Rect2<GC> unProjectRect2(const Rect2<int> &src)
{
	Rect2<GC> r;
	r.x = Gfx::iXPos(src.x);
	r.y = Gfx::iYPos(src.y);
	r.x2 = Gfx::iXPos(src.x2);
	r.y2 = Gfx::iYPos(src.y2);
	return r;
}

static GC alignXToPixel(GC x)
{
	return iXPos(toIXPos(x));
}

static GC alignYToPixel(GC y)
{
	return iYPos(toIYPos(y));
}

static GC xMMSize(GC mm) { return mm * proj.mmToXScale; }
static GC yMMSize(GC mm) { return mm * proj.mmToYScale; }

#ifdef CONFIG_BASE_ANDROID
static GC xSMMSize(GC mm) { return mm * proj.smmToXScale; }
static GC ySMMSize(GC mm) { return mm * proj.smmToYScale; }
#else
static GC xSMMSize(GC mm) { return xMMSize(mm); }
static GC ySMMSize(GC mm) { return yMMSize(mm); }
#endif

static Coordinate gXSize(const Rect2<int> &r) { return iXSize(r.xSize()); }
static Coordinate gYSize(const Rect2<int> &r) { return iYSize(r.ySize()); }
static Coordinate gXPos(const Rect2<int> &r, _2DOrigin o)
{
	int pos = r.xPos(o);
	if(o.xScaler() == 1)
		pos++;
	return iXPos(pos);
}
static Coordinate gYPos(const Rect2<int> &r, _2DOrigin o)
{
	int pos = r.yPos(o);
	if(o.invertYIfCartesian().yScaler() == 1)
		pos++;
	return iYPos(pos);
}
static Coordinate gXPos(const Rect2<int> &r, Coordinate scale, _2DOrigin o) { return gXPos(r, o) + (gXSize(r) * (scale)); }
static Coordinate gYPos(const Rect2<int> &r, Coordinate scale, _2DOrigin o) { return gYPos(r, o) + (gYSize(r) * (scale)); }

}
