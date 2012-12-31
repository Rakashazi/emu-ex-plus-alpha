#pragma once

#include <gfx/GeomQuadMesh.hh>

namespace Gfx
{

CallResult GeomQuadMesh::init(const VertexPos *x, uint xVals, const VertexPos *y, uint yVals, VertexColor color)
{
	if(xVals < 2 || yVals < 2)
		return INVALID_PARAMETER;
	verts = xVals * yVals;
	int quads = (xVals - 1) * (yVals - 1);
	idxs = quads*6;
	//logMsg("mesh with %d verts, %d idxs, %d quads", verts, idxs, quads);
	uchar *mem = (uchar*)mem_alloc((sizeof(ColVertex) * verts) + (sizeof(VertexIndex) * idxs));
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
	mem_free(v.arr);
}

void GeomQuadMesh::draw()
{
	v.arr->draw(v.arr, i, Gfx::TRIANGLE, idxs);
}

void GeomQuadMesh::setColorRGB(GColor r, GColor g, GColor b)
{
	iterateTimes(verts, i)
	{
		v[i].color = VertexColorPixelFormat.build(r, g, b, VertexColorPixelFormat.a(v[i].color));
	}
}

void GeomQuadMesh::setColorTranslucent(GColor a)
{
	iterateTimes(verts, i)
	{
		v[i].color = VertexColorPixelFormat.build(VertexColorPixelFormat.r(v[i].color), VertexColorPixelFormat.g(v[i].color), VertexColorPixelFormat.b(v[i].color), a);
	}
}

void GeomQuadMesh::setColorRGBV(GColor r, GColor g, GColor b, uint i)
{
	v[i].color = VertexColorPixelFormat.build(r, g, b, VertexColorPixelFormat.a(v[i].color));
}

void GeomQuadMesh::setColorTranslucentV(GColor a, uint i)
{
	// swap for tri strip
	v[i].color = VertexColorPixelFormat.build(VertexColorPixelFormat.r(v[i].color), VertexColorPixelFormat.g(v[i].color), VertexColorPixelFormat.b(v[i].color), a);
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
