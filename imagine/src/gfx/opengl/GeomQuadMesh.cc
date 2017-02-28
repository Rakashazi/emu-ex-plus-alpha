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
#include <imagine/mem/mem.h>
#include <imagine/util/math/space.hh>
#include "private.hh"

namespace Gfx
{

CallResult GeomQuadMesh::init(const VertexPos *x, uint xVals, const VertexPos *y, uint yVals, VertexColor color)
{
	deinit();
	if(xVals < 2 || yVals < 2)
		return INVALID_PARAMETER;
	verts = xVals * yVals;
	int quads = (xVals - 1) * (yVals - 1);
	idxs = quads*6;
	//logMsg("mesh with %d verts, %d idxs, %d quads", verts, idxs, quads);
	auto mem = (char*)mem_alloc((sizeof(ColVertex) * verts) + (sizeof(VertexIndex) * idxs));
	v = {(ColVertex*)mem, xVals};
	i = (VertexIndex*)(mem + (sizeof(ColVertex) * verts));

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
	iterateTimes(yVals-1, yIdx)
		iterateTimes(xVals-1, xIdx)
		{
			// Triangle 1, LB LT RT
			currI[0] = v.idxOf(yIdx, xIdx);
			currI[1] = v.idxOf(yIdx, xIdx+1);
			currI[2] = v.idxOf(yIdx+1, xIdx);

			// Triangle 1, LB RT RB
			currI[3] = v.idxOf(yIdx, xIdx+1);
			currI[4] = v.idxOf(yIdx+1, xIdx+1);
			currI[5] = v.idxOf(yIdx+1, xIdx);

			//logMsg("quad %d %d,%d,%d %d,%d,%d", quads, currI[0], currI[1], currI[2], currI[3], currI[4], currI[5]);

			currI += 6;
			quads++;
		}
	return OK;
}

void GeomQuadMesh::deinit()
{
	//logMsg("GeomQuadMesh::deinit()");
	if(v.arr)
	{
		mem_free(v.arr);
		v.arr = nullptr;
	}
}

void GeomQuadMesh::draw(Renderer &r)
{
	r.bindTempVertexBuffer();
	r.vertexBufferData(v.arr, sizeof(ColVertex) * verts);
	ColVertex::bindAttribs(r, v.arr);
	r.drawPrimitiveElements(Primitive::TRIANGLE, i, idxs);
}

void GeomQuadMesh::setColorRGB(ColorComp r, ColorComp g, ColorComp b)
{
	iterateTimes(verts, i)
	{
		v[i].color = VertexColorPixelFormat.build((uint)r, (uint)g, (uint)b, VertexColorPixelFormat.a(v[i].color));
	}
}

void GeomQuadMesh::setColorTranslucent(ColorComp a)
{
	iterateTimes(verts, i)
	{
		v[i].color = VertexColorPixelFormat.build(VertexColorPixelFormat.r(v[i].color), VertexColorPixelFormat.g(v[i].color), VertexColorPixelFormat.b(v[i].color), (uint)a);
	}
}

void GeomQuadMesh::setColorRGBV(ColorComp r, ColorComp g, ColorComp b, uint i)
{
	v[i].color = VertexColorPixelFormat.build((uint)r, (uint)g, (uint)b, VertexColorPixelFormat.a(v[i].color));
}

void GeomQuadMesh::setColorTranslucentV(ColorComp a, uint i)
{
	// swap for tri strip
	v[i].color = VertexColorPixelFormat.build(VertexColorPixelFormat.r(v[i].color), VertexColorPixelFormat.g(v[i].color), VertexColorPixelFormat.b(v[i].color), (uint)a);
}

void GeomQuadMesh::setPos(GC x, GC y, GC x2, GC y2)
{
	uint xVals = v.columns, yVals = verts/xVals;
	ColVertex *currV = v;
	iterateTimes(yVals, yIdx)
		iterateTimes(xVals, xIdx)
		{
			currV->x = yIdx == 0 ? IG::scalePointRange((GC)xIdx, (GC)0, GC(xVals-1), x, x2)
					: (currV-v.columns)->x;
			currV->y = xIdx == 0 ? IG::scalePointRange((GC)yIdx, (GC)0, GC(yVals-1), y, y2)
					: (currV-xIdx)->y;
			currV++;
		}
}

}
