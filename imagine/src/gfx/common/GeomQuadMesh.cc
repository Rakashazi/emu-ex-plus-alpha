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
#include <imagine/util/ranges.hh>

namespace IG::Gfx
{

GeomQuadMesh::GeomQuadMesh(std::span<const VertexPos> x, std::span<const VertexPos> y, VertexColor color)
{
	if(x.size() < 2 || y.size() < 2)
		return;
	verts = x.size() * y.size();
	int quads = (x.size() - 1) * (y.size() - 1);
	idxs = quads*6;
	//logMsg("mesh with %d verts, %d idxs, %d quads", verts, idxs, quads);
	vMem = std::make_unique<char[]>((sizeof(ColVertex) * verts) + (sizeof(VertexIndex) * idxs));
	xVals = x.size();
	i = (VertexIndex*)(vMem.get() + (sizeof(ColVertex) * verts));

	/*ColVertex *currV = v;
	iterateTimes(y.size(), yIdx)
		iterateTimes(x.size(), xIdx)
		{
			*currV = ColVertex(x[xIdx], y[yIdx], color);
			logMsg("vert %f,%f", currV->x, currV->y);
			currV++;
		}*/

	VertexIndex *currI = i;
	quads = 0;
	auto vArr = v();
	for(auto yIdx : iotaCount(y.size()-1))
		for(auto xIdx : iotaCount(x.size()-1))
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

void GeomQuadMesh::draw(RendererCommands &cmds) const
{
	cmds.bindTempVertexBuffer();
	cmds.vertexBufferData(v().arr, sizeof(ColVertex) * verts);
	ColVertex::bindAttribs(cmds, v().arr);
	cmds.drawPrimitiveElements(Primitive::TRIANGLE, i, idxs);
}

void GeomQuadMesh::setColorRGB(ColorComp r, ColorComp g, ColorComp b)
{
	auto vPtr = v().data();
	for(auto i : iotaCount(verts))
	{
		vPtr[i].color = VertexColorPixelFormat.build((uint32_t)r, (uint32_t)g, (uint32_t)b, VertexColorPixelFormat.a(vPtr[i].color));
	}
}

void GeomQuadMesh::setColorTranslucent(ColorComp a)
{
	auto vPtr = v().data();
	for(auto i : iotaCount(verts))
	{
		vPtr[i].color = VertexColorPixelFormat.build(VertexColorPixelFormat.r(vPtr[i].color), VertexColorPixelFormat.g(vPtr[i].color), VertexColorPixelFormat.b(vPtr[i].color), (uint32_t)a);
	}
}

void GeomQuadMesh::setColorRGBV(ColorComp r, ColorComp g, ColorComp b, size_t i)
{
	auto vPtr = v().data();
	vPtr[i].color = VertexColorPixelFormat.build((uint32_t)r, (uint32_t)g, (uint32_t)b, VertexColorPixelFormat.a(vPtr[i].color));
}

void GeomQuadMesh::setColorTranslucentV(ColorComp a, size_t i)
{
	// swap for tri strip
	auto vPtr = v().data();
	vPtr[i].color = VertexColorPixelFormat.build(VertexColorPixelFormat.r(vPtr[i].color), VertexColorPixelFormat.g(vPtr[i].color), VertexColorPixelFormat.b(vPtr[i].color), (uint32_t)a);
}

void GeomQuadMesh::setPos(float x, float y, float x2, float y2)
{
	auto yVals = verts/xVals;
	auto vPtr = v().data();
	for(auto yIdx : iotaCount(yVals))
		for(auto xIdx : iotaCount(xVals))
		{
			vPtr->x = yIdx == 0 ? IG::remap((float)xIdx, 0.f, float(xVals-1), x, x2)
					: (vPtr-xVals)->x;
			vPtr->y = xIdx == 0 ? IG::remap((float)yIdx, 0.f, float(yVals-1), y, y2)
					: (vPtr-xIdx)->y;
			vPtr++;
		}
}

ArrayView2<ColVertex> GeomQuadMesh::v() const
{
	return {(ColVertex*)vMem.get(), (size_t)xVals};
}

}
