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

#include <imagine/gfx/GeomQuadMesh.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/util/math/space.hh>
#include "private.hh"

namespace Gfx
{

GeomQuadMesh::GeomQuadMesh(const VertexPos *x, uint32_t xVals, const VertexPos *y, uint32_t yVals, VertexColor color)
{
	if(xVals < 2 || yVals < 2)
		return;
	verts = xVals * yVals;
	int quads = (xVals - 1) * (yVals - 1);
	idxs = quads*6;
	//logMsg("mesh with %d verts, %d idxs, %d quads", verts, idxs, quads);
	vMem = std::make_unique<char[]>((sizeof(ColVertex) * verts) + (sizeof(VertexIndex) * idxs));
	this->xVals = xVals;
	i = (VertexIndex*)(vMem.get() + (sizeof(ColVertex) * verts));

	/*ColVertex *currV = v;
	iterateTimes(yVals, yIdx)
		iterateTimes(xVals, xIdx)
		{
			*currV = ColVertex(x[xIdx], y[yIdx], color);
			logMsg("vert %f,%f", currV->x, currV->y);
			currV++;
		}*/

	VertexIndex *currI = i;
	quads = 0;
	auto vArr = v();
	iterateTimes(yVals-1, yIdx)
		iterateTimes(xVals-1, xIdx)
		{
			// Triangle 1, LB LT RT
			currI[0] = vArr.flatOffset(yIdx, xIdx);
			currI[1] = vArr.flatOffset(yIdx, xIdx+1);
			currI[2] = vArr.flatOffset(yIdx+1, xIdx);

			// Triangle 1, LB RT RB
			currI[3] = vArr.flatOffset(yIdx, xIdx+1);
			currI[4] = vArr.flatOffset(yIdx+1, xIdx+1);
			currI[5] = vArr.flatOffset(yIdx+1, xIdx);

			//logMsg("quad %d %d,%d,%d %d,%d,%d", quads, currI[0], currI[1], currI[2], currI[3], currI[4], currI[5]);

			currI += 6;
			quads++;
		}
}

void GeomQuadMesh::draw(RendererCommands &cmds)
{
	cmds.bindTempVertexBuffer();
	cmds.vertexBufferData(v().arr, sizeof(ColVertex) * verts);
	ColVertex::bindAttribs(cmds, v().arr);
	cmds.drawPrimitiveElements(Primitive::TRIANGLE, i, idxs);
}

void GeomQuadMesh::setColorRGB(ColorComp r, ColorComp g, ColorComp b)
{
	auto vPtr = v().data();
	iterateTimes(verts, i)
	{
		vPtr[i].color = VertexColorPixelFormat.build((uint32_t)r, (uint32_t)g, (uint32_t)b, VertexColorPixelFormat.a(vPtr[i].color));
	}
}

void GeomQuadMesh::setColorTranslucent(ColorComp a)
{
	auto vPtr = v().data();
	iterateTimes(verts, i)
	{
		vPtr[i].color = VertexColorPixelFormat.build(VertexColorPixelFormat.r(vPtr[i].color), VertexColorPixelFormat.g(vPtr[i].color), VertexColorPixelFormat.b(vPtr[i].color), (uint32_t)a);
	}
}

void GeomQuadMesh::setColorRGBV(ColorComp r, ColorComp g, ColorComp b, uint32_t i)
{
	auto vPtr = v().data();
	vPtr[i].color = VertexColorPixelFormat.build((uint32_t)r, (uint32_t)g, (uint32_t)b, VertexColorPixelFormat.a(vPtr[i].color));
}

void GeomQuadMesh::setColorTranslucentV(ColorComp a, uint32_t i)
{
	// swap for tri strip
	auto vPtr = v().data();
	vPtr[i].color = VertexColorPixelFormat.build(VertexColorPixelFormat.r(vPtr[i].color), VertexColorPixelFormat.g(vPtr[i].color), VertexColorPixelFormat.b(vPtr[i].color), (uint32_t)a);
}

void GeomQuadMesh::setPos(GC x, GC y, GC x2, GC y2)
{
	uint32_t yVals = verts/xVals;
	auto vPtr = v().data();
	iterateTimes(yVals, yIdx)
		iterateTimes(xVals, xIdx)
		{
			vPtr->x = yIdx == 0 ? IG::scalePointRange((GC)xIdx, (GC)0, GC(xVals-1), x, x2)
					: (vPtr-xVals)->x;
			vPtr->y = xIdx == 0 ? IG::scalePointRange((GC)yIdx, (GC)0, GC(yVals-1), y, y2)
					: (vPtr-xIdx)->y;
			vPtr++;
		}
}

ArrayView2<ColVertex> GeomQuadMesh::v() const
{
	return {(ColVertex*)vMem.get(), xVals};
}

}
