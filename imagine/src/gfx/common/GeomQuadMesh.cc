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
#include <imagine/util/math/math.hh>
#include <imagine/util/ranges.hh>

namespace IG::Gfx
{

GeomQuadMesh::GeomQuadMesh(std::span<const int> x, std::span<const int> y, PackedColor color)
{
	if(x.size() < 2 || y.size() < 2)
		return;
	verts = x.size() * y.size();
	int quads = (x.size() - 1) * (y.size() - 1);
	idxs = quads*6;
	//logMsg("mesh with %d verts, %d idxs, %d quads", verts, idxs, quads);
	vMem = std::make_unique<char[]>((sizeof(Vertex) * verts) + (sizeof(VertexIndex) * idxs));
	xVals = x.size();
	i = (VertexIndex*)(vMem.get() + (sizeof(Vertex) * verts));

	/*Vertex *currV = v;
	iterateTimes(y.size(), yIdx)
		iterateTimes(x.size(), xIdx)
		{
			*currV = Vertex(x[xIdx], y[yIdx], color);
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
	cmds.vertexBufferData(v().arr, sizeof(Vertex) * verts);
	cmds.setVertexAttribs(v().arr);
	cmds.drawPrimitiveElements(Primitive::TRIANGLE, {i, (size_t)idxs});
}

void GeomQuadMesh::setColor(PackedColor c)
{
	auto vPtr = v().data();
	for(auto i : iotaCount(verts))
	{
		vPtr[i].color = c;
	}
}

void GeomQuadMesh::setColorV(PackedColor c, size_t i)
{
	auto vPtr = v().data();
	vPtr[i].color = c;
}

void GeomQuadMesh::setPos(int x, int y, int x2, int y2)
{
	auto yVals = verts/xVals;
	auto vPtr = v().data();
	for(auto yIdx : iotaCount(yVals))
		for(auto xIdx : iotaCount(xVals))
		{
			vPtr->pos.x = yIdx == 0 ? IG::remap(xIdx, 0, xVals-1, x, x2)
					: (vPtr-xVals)->pos.x;
			vPtr->pos.y = xIdx == 0 ? IG::remap(yIdx, 0, yVals-1, y, y2)
					: (vPtr-xIdx)->pos.y;
			vPtr++;
		}
}

auto GeomQuadMesh::v() const -> ArrayView2<Vertex>
{
	return {(Vertex*)vMem.get(), (size_t)xVals};
}

}
