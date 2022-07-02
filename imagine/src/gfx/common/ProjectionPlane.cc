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

#define LOGTAG "GfxProjectionPlane"
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/ProjectionPlane.hh>
#include <imagine/base/Viewport.hh>
#include <imagine/logger/logger.h>

namespace IG::Gfx
{

float ProjectionPlane::width() const
{
	return w;
}

float ProjectionPlane::height() const
{
	return h;
}

FP ProjectionPlane::size() const
{
	return {w, h};
}

float ProjectionPlane::focalZ() const
{
	return focal;
}

ProjectionPlane ProjectionPlane::makeWithMatrix(Viewport viewport, Mat4 mat)
{
	ProjectionPlane p;
	auto matInv = mat.invert();
	p.winBounds = viewport.bounds();
	auto lowerLeft = mat.unproject(asYUpRelRect(viewport), {(float)viewport.bounds().x, (float)viewport.bounds().y, .5}, matInv);
	//logMsg("Lower-left projection point %d,%d -> %f %f %f", viewport.bounds().x, viewport.bounds().y, (double)lowerLeft.v.x, (double)lowerLeft.v.y, (double)lowerLeft.v.z);
	auto upperRight = mat.unproject(asYUpRelRect(viewport), {(float)viewport.bounds().x2, (float)viewport.bounds().y2, .5}, matInv);
	//logMsg("Upper-right projection point %d,%d -> %f %f %f", viewport.bounds().x2, viewport.bounds().y2, (double)upperRight.v.x, (double)upperRight.v.y, (double)upperRight.v.z);
	p.w = upperRight.x() - lowerLeft.x(), p.h = upperRight.y() - lowerLeft.y();
	p.focal = upperRight.z();
	p.rect.x = -p.w/2.f;
	p.rect.y = -p.h/2.f;
	p.rect.x2 = p.w/2.f;
	p.rect.y2 = p.h/2.f;
	p.pixToXScale = p.w / (float)viewport.width();
	p.pixToYScale = p.h / (float)viewport.height();
	p.xToPixScale = (float)viewport.width() / p.w;
	p.yToPixScale = (float)viewport.height() / p.h;
	logMsg("made with size %fx%f, to pix %fx%f, to view %fx%f",
		(double)p.w, (double)p.h, (double)p.xToPixScale, (double)p.yToPixScale, (double)p.pixToXScale, (double)p.pixToYScale);
	return p;
}

Mat4 ProjectionPlane::makeTranslate(IG::Point2D<float> p) const
{
	return Mat4::makeTranslate({p.x, p.y, focal});
}

Mat4 ProjectionPlane::makeTranslate() const
{
	return Mat4::makeTranslate({0.f, 0.f, focal});
}

void ProjectionPlane::loadTranslate(Gfx::RendererCommands &cmds, float x, float y) const
{
	cmds.loadTranslate(x, y, focal);
}

void ProjectionPlane::loadTranslate(Gfx::RendererCommands &cmds, FP p) const
{
	loadTranslate(cmds, p.x, p.y);
}

void ProjectionPlane::resetTransforms(Gfx::RendererCommands &cmds) const
{
	loadTranslate(cmds, 0., 0.);
}

float ProjectionPlane::unprojectXSize(float x) const
{
	float s = x * pixToXScale;
	//logMsg("project x %f, to %f", x, r);
	return s;
}

float ProjectionPlane::unprojectYSize(float y) const
{
	float s = y * pixToYScale;
	//logMsg("project y %f, to %f", y, r);
	return s;
}

float ProjectionPlane::unprojectX(float x) const
{
	return unprojectXSize(x - windowBounds().x) - wHalf();
}

float ProjectionPlane::unprojectY(float y) const
{
	return -unprojectYSize(y - windowBounds().y) + hHalf();
}

float ProjectionPlane::projectXSize(float x) const
{
	float s = x * xToPixScale;
	//if(s) logMsg("unproject x %f, to %f", x, s);
	return s;
}

float ProjectionPlane::projectYSize(float y) const
{
	float s = y * yToPixScale;
	//if(s) logMsg("unproject y %f, to %f", y, s);
	return s;
}

float ProjectionPlane::projectX(float x) const
{
	//logMsg("unproject x %f", x);
	return projectXSize(x + wHalf()) + windowBounds().x;
}

float ProjectionPlane::projectY(float y) const
{
	//logMsg("unproject y %f", y);
	return projectYSize(-(y - hHalf())) + windowBounds().y;
}

GCRect ProjectionPlane::unProjectRect(int x, int y, int x2, int y2) const
{
	GCRect objRect;
	objRect.x = unprojectX(x);
	objRect.y = unprojectY(y2); // flip y-axis
	objRect.x2 = unprojectX(x2);
	objRect.y2 = unprojectY(y);
	return objRect;
}

GCRect ProjectionPlane::unProjectRect(IG::WindowRect r) const
{
	return unProjectRect(r.x, r.y, r.x2, r.y2);
}

IG::WindowRect ProjectionPlane::projectRect(GCRect r) const
{
	IG::WindowRect winRect;
	winRect.x = projectX(r.x);
	winRect.y = projectY(r.y2);
	winRect.x2 = projectX(r.x2);
	winRect.y2 = projectY(r.y);
	return winRect;
}

float ProjectionPlane::alignXToPixel(float x) const
{
	return unprojectX(std::floor(projectX(x)));
}

float ProjectionPlane::alignYToPixel(float y) const
{
	return unprojectY(std::floor(projectY(y)));
}

IG::Point2D<float> ProjectionPlane::alignToPixel(IG::Point2D<float> p) const
{
	return {alignXToPixel(p.x), alignYToPixel(p.y)};
}

}
