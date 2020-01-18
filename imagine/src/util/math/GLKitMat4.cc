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

#include <imagine/util/math/GLKitMat4.hh>
#include <imagine/util/math/GLKitVec3.hh>
#include <imagine/util/math/GLKitVec4.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/operators.hh>
#include <GLKit/GLKMathUtils.h>

GLKitMat4 GLKitMat4::translate(GLKitVec3 translation) const
{
	return GLKMatrix4TranslateWithVector3(m, translation.vec());
}

GLKitMat4 GLKitMat4::makeTranslate(GLKitVec3 translation)
{
	GLKitMat4 ident;
	return GLKMatrix4TranslateWithVector3(ident.m, translation.vec());
}

GLKitMat4 GLKitMat4::makePerspectiveFovRH(float fovy, float aspect, float znear, float zfar)
{
	return GLKMatrix4MakePerspective(fovy, aspect, znear, zfar);
}

GLKitMat4 GLKitMat4::scale(GLKitVec3 factors) const
{
	return GLKMatrix4ScaleWithVector3(m, factors.vec());
}

GLKitMat4 GLKitMat4::rotate(float angle, GLKitVec3 axis) const
{
	return GLKMatrix4RotateWithVector3(m, angle, axis.vec());
}

GLKitMat4 GLKitMat4::invert() const
{
	bool isInvertible;
	auto mat = GLKMatrix4Invert(m, &isInvertible);
	assert(isInvertible);
	return mat;
}

GLKitMat4 GLKitMat4::mult(GLKitMat4 mat) const
{
	return GLKMatrix4Multiply(m, mat.m);
}

GLKitVec4 GLKitMat4::mult(GLKitVec4 vec) const
{
	return GLKMatrix4MultiplyVector4(m, vec.vec());
}

GLKitVec3 GLKitMat4::project(IG::Rect2<int> viewport, GLKitVec3 obj) const
{
	GLKitMat4 ident;
	int viewportVec[4] {viewport.x, viewport.y, viewport.x2, viewport.y2};
	return GLKMathProject(obj.vec(), ident.m, m, viewportVec);
}

GLKitVec3 GLKitMat4::unproject(IG::Rect2<int> viewport, GLKitVec3 win, GLKitMat4 inverse) const
{
	GLKitMat4 ident;
	int viewportVec[4] {viewport.x, viewport.y, viewport.x2, viewport.y2};
	bool success;
	auto mat = GLKMathUnproject(win.vec(), ident.m, m, viewportVec, &success);
	assert(success);
	return mat;
}

bool GLKitMat4::operator ==(GLKitMat4 const &rhs) const
{
	return memcmp(&m, &rhs.m, sizeof(GLKMatrix4)) == 0;
}
