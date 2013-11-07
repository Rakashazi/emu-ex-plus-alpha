#pragma once

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

#include <engine-globals.h>
#include <util/bits.h>
#include <util/rectangle2.h>
#include <util/time/sys.hh>
#include <gfx/defs.hh>
#include <gfx/Projector.hh>
#include <util/pixel.h>
#include <base/Base.hh>

namespace Gfx
{

// init & control
CallResult init() ATTRS(cold);
void setViewport(Base::Window &win);
void setProjector(Base::Window &win);

// commit/sync
void renderFrame(Base::Window &win, FrameTimeBase frameTime);
void waitVideoSync();
void updateFrameTime();
extern uint frameTime, frameTimeRel;

enum { TRIANGLE = 1, TRIANGLE_STRIP, QUAD, };

extern bool preferBGRA, preferBGR;

// render states

enum { BLEND_MODE_OFF = 0, BLEND_MODE_ALPHA, BLEND_MODE_INTENSITY };
void setBlendMode(uint mode);

enum { IMG_MODE_MODULATE = 0, IMG_MODE_BLEND, IMG_MODE_REPLACE, IMG_MODE_ADD };
void setImgMode(uint mode);

enum { BLEND_EQ_ADD, BLEND_EQ_SUB, BLEND_EQ_RSUB };
void setBlendEquation(uint mode);

void setActiveTexture(GfxTextureHandle tex, uint type = 0);

void setImgBlendColor(GColor r, GColor g, GColor b, GColor a);

void setDither(uint on);
uint dither();

void setZTest(bool on);

enum { BOTH_FACES, FRONT_FACES, BACK_FACES };
void setVisibleGeomFace(uint sides);

void setClipRect(bool on);
void setClipRectBounds(const Base::Window &win, int x, int y, int w, int h);
static void setClipRectBounds(const Base::Window &win, IG::Rect2<int> r)
{
	setClipRectBounds(win, r.x, r.y, r.xSize(), r.ySize());
}

void setZBlend(bool on);
void setZBlendColor(GColor r, GColor g, GColor b);

void clear();
void setClearColor(GColor r, GColor g, GColor b, GColor a = 1.);

void setColor(GColor r, GColor g, GColor b, GColor a = 1.);

static void setColor(GColor i) { setColor(i, i, i, 1.); }

enum GfxColorEnum { COLOR_WHITE, COLOR_BLACK };
static void setColor(GfxColorEnum colConst)
{
	switch(colConst)
	{
		bcase COLOR_WHITE: setColor(1., 1., 1.);
		bcase COLOR_BLACK: setColor(0., 0., 0.);
	}
}

static const PixelFormatDesc &ColorFormat = PixelFormatRGBA8888;
uint color();

// state shortcuts
static void shadeMod()
{
	setActiveTexture(0);
	setImgMode(IMG_MODE_MODULATE);
	setBlendMode(BLEND_MODE_OFF);
}

static void shadeModAlpha()
{
	setActiveTexture(0);
	setImgMode(IMG_MODE_MODULATE);
	setBlendMode(BLEND_MODE_ALPHA);
}

// transforms

enum TransformTargetEnum { TARGET_WORLD, TARGET_TEXTURE };
void setTransformTarget(TransformTargetEnum target);

void applyTranslate(TransformCoordinate x, TransformCoordinate y, TransformCoordinate z);
static void applyTranslate(TransformCoordinate x, TransformCoordinate y) { applyTranslate(x, y, Gfx::proj.focal); }
void loadTranslate(TransformCoordinate x, TransformCoordinate y, TransformCoordinate z);
static void loadTranslate(TransformCoordinate x, TransformCoordinate y) { loadTranslate(x, y, Gfx::proj.focal); }
void applyScale(TransformCoordinate sx, TransformCoordinate sy, TransformCoordinate sz);
static void applyScale(TransformCoordinate sx, TransformCoordinate sy) { applyScale(sx, sy, 1.); }
static void applyScale(TransformCoordinate s) { applyScale(s, s, 1.); }
static void applyScale(GfxPoint p) { applyScale(p.x, p.y, 1.); }
static void applyYScale2D(TransformCoordinate sy, TransformCoordinate aR) { applyScale(sy * aR, sy, 1.); }
void applyPitchRotate(Angle t);
void applyRollRotate(Angle t);
void applyYawRotate(Angle t);
static void resetTransforms()
{
	loadTranslate(0., 0., proj.focal);
}
void loadIdentTransform();

static void loadTransforms(const IG::Rect2<int> &r, _2DOrigin o)
{
	loadTranslate(gXPos(r, o), gYPos(r, o));
	applyScale(gXSize(r), gYSize(r));
}

/*static GC adjustZScales(GC pos, GC origPosZ, GC newPosZ)
{
	return pos * (newPosZ / origPosZ);
}

static GC adjustZScalesInv(GC pos, GC origPosZ, GC newPosZ)
{
	return pos / (newPosZ / origPosZ);
}*/

struct GfxViewState
{
	Coordinate width, height, aspectRatio;
	uint pixelWidth, pixelHeight;
};

// callbacks

void onDraw(Base::Window &win, FrameTimeBase frameTime);
void onViewChange(Base::Window &win, GfxViewState *oldState);

}
