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

#include <util/number.h>

template<typename T>
class Vector3d
{
public:
	T x, y, z;

	T const squareMagnitude()
	{
		return (x * x) + (y * y) + (z * z);
	}

	T const magnitude()
	{
		return std::sqrt(squareMagnitude());
	}

	void normalize()
	{
		T magnitude = magnitude();
		x = x / magnitude;
		y = y / magnitude;
		z = z / magnitude;
	}

	static void crossProduct(Vector3d* vOut, const Vector3d* v1, const Vector3d* v2)
	{
		vOut->x = (v1->y * v2->z) - (v1->z * v2->y);
		vOut->y = (v1->z * v2->x) - (v1->x * v2->z);
		vOut->z = (v1->x * v2->y) - (v1->y * v2->x);
	}

	static void subtract(Vector3d *out, const Vector3d *v1, const Vector3d *v2)
	{
		out->x = v1->x - v2->x;
		out->y = v1->y - v2->y;
		out->z = v1->z - v2->z;
	}

	void add(Vector3d *out, const Vector3d *v1, const Vector3d *v2)
	{
		out->x = v1->x + v2->x;
		out->y = v1->y + v2->y;
		out->z = v1->z + v2->z;
	}

	void subtractFrom(const Vector3d *v2)
	{
		x = x - v2->x;
		y = y - v2->y;
		z = z - v2->z;
	}

	void addTo(const Vector3d *v2)
	{
		x = x + v2->x;
		y = y + v2->y;
		z = z + v2->z;
	}

	T const dotProduct(const Vector3d *v2)
	{
		return (x * v2->x) + (y * v2->y) + (z * v2->z);
	}

	static void computeFaceNormal(const Vector3d *p1, const Vector3d *p2, const Vector3d *p3, Vector3d *out)
	{
		// Uses p2 as a new origin for p1,p3
		Vector3d a;
		subtract(&a, p3, p2);
		Vector3d b;
		subtract(&b, p1, p2);
		// Compute the cross product a X b to get the face normal
		crossProduct(out, &a, &b);
		// normalized vector
		out->normalize();
	}
};
