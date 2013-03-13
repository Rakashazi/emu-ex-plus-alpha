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
#include <util/Vector3d.hh>
#include <util/Vector4d.hh>

template<typename T>
class Matrix4x4
{
public:

	union
	{
		T v[16] {0};
		struct
		{
			T  _11, _12, _13, _14;
			T  _21, _22, _23, _24;
			T  _31, _32, _33, _34;
			T  _41, _42, _43, _44;
		};
	};

	constexpr Matrix4x4() { }

	void ident()
	{
		_11 = 1;
		_12 = 0;
		_13 = 0;
		_14 = 0;

		_21 = 0;
		_22 = 1;
		_23 = 0;
		_24 = 0;

		_31 = 0;
		_32 = 0;
		_33 = 1;
		_34 = 0;

		_41 = 0;
		_42 = 0;
		_43 = 0;
		_44 = 1;
	}
	
	void orthographicProjection(T xmax, T xmin, T ymax, T ymin, T zmax, T zmin)
	{
		_11 = (T)2 / (xmax - xmin);
		_12 = 0;
		_13 = 0;
		_14 = -((xmax + xmin) / (xmax - xmin));

		_21 = 0;
		_22 = (T)2 / (ymax - ymin);
		_23 = 0;
		_24 = -((ymax + ymin) / (ymax - ymin));

		_31 = 0;
		_32 = 0;
		_33 = (T)2 / (zmax - zmin);
		_34 = -((zmax + zmin) / (zmax - zmin));

		_41 = 0;
		_42 = 0;
		_43 = 0;
		_44 = 1;
	}
	
	void orthogonalLH(T width, T height, T zmax, T zmin)
	{
		_11 = (T)2 / width;
		_12 = 0;
		_13 = 0;
		_14 = 0;

		_21 = 0;
		_22 = (T)2 / height;
		_23 = 0;
		_24 = 0;
	
		_31 = 0;
		_32 = 0;
		_33 = (T)1 / (zmax - zmin);
		_34 = 0;

		_41 = 0;
		_42 = 0;
		_43 = zmin / (zmin - zmax);
		_44 = 1;
	}
	
	void mult(const Vector4d<T> &in, Vector4d<T> &out) const
	{
		iterateTimes(4, i)
		{
			out.v[i] =
				in.v[0] * v[0*4+i] +
				in.v[1] * v[1*4+i] +
				in.v[2] * v[2*4+i] +
				in.v[3] * v[3*4+i];
		}
	}

	static void mult(Matrix4x4* result, const Matrix4x4* mat1, const Matrix4x4* mat2)
	{
		result->_11 = (mat1->_11 * mat2->_11) + (mat1->_12 * mat2->_21) + (mat1->_13 * mat2->_31) + (mat1->_14 * mat2->_41);
		result->_12 = (mat1->_11 * mat2->_12) + (mat1->_12 * mat2->_22) + (mat1->_13 * mat2->_32) + (mat1->_14 * mat2->_42);
		result->_13 = (mat1->_11 * mat2->_13) + (mat1->_12 * mat2->_23) + (mat1->_13 * mat2->_33) + (mat1->_14 * mat2->_43);
		result->_14 = (mat1->_11 * mat2->_14) + (mat1->_12 * mat2->_24) + (mat1->_13 * mat2->_34) + (mat1->_14 * mat2->_44);

		result->_21 = (mat1->_21 * mat2->_11) + (mat1->_22 * mat2->_21) + (mat1->_23 * mat2->_31) + (mat1->_24 * mat2->_41);
		result->_22 = (mat1->_21 * mat2->_12) + (mat1->_22 * mat2->_22) + (mat1->_23 * mat2->_32) + (mat1->_24 * mat2->_42);
		result->_23 = (mat1->_21 * mat2->_13) + (mat1->_22 * mat2->_23) + (mat1->_23 * mat2->_33) + (mat1->_24 * mat2->_43);
		result->_24 = (mat1->_21 * mat2->_14) + (mat1->_22 * mat2->_24) + (mat1->_23 * mat2->_34) + (mat1->_24 * mat2->_44);

		result->_31 = (mat1->_31 * mat2->_11) + (mat1->_32 * mat2->_21) + (mat1->_33 * mat2->_31) + (mat1->_34 * mat2->_41);
		result->_32 = (mat1->_31 * mat2->_12) + (mat1->_32 * mat2->_22) + (mat1->_33 * mat2->_32) + (mat1->_34 * mat2->_42);
		result->_33 = (mat1->_31 * mat2->_13) + (mat1->_32 * mat2->_23) + (mat1->_33 * mat2->_33) + (mat1->_34 * mat2->_43);
		result->_34 = (mat1->_31 * mat2->_14) + (mat1->_32 * mat2->_24) + (mat1->_33 * mat2->_34) + (mat1->_34 * mat2->_44);

		result->_41 = (mat1->_41 * mat2->_11) + (mat1->_42 * mat2->_21) + (mat1->_43 * mat2->_31) + (mat1->_44 * mat2->_41);
		result->_42 = (mat1->_41 * mat2->_12) + (mat1->_42 * mat2->_22) + (mat1->_43 * mat2->_32) + (mat1->_44 * mat2->_42);
		result->_43 = (mat1->_41 * mat2->_13) + (mat1->_42 * mat2->_23) + (mat1->_43 * mat2->_33) + (mat1->_44 * mat2->_43);
		result->_44 = (mat1->_41 * mat2->_14) + (mat1->_42 * mat2->_24) + (mat1->_43 * mat2->_34) + (mat1->_44 * mat2->_44);
	}
	
	void multWith(const Matrix4x4* mat)
	{
		Matrix4x4 resultTemp = *this;
		mult(this, &resultTemp, mat);
	}
	
	static Matrix4x4 mult(const Matrix4x4 &m1, const Matrix4x4 &m2)
	{
		Matrix4x4 result;
		mult(&result, &m1, &m2);
		return result;
	}

	void perspectiveFovSetZeros()
	{
		_12 = 0;
		_13 = 0;
		_14 = 0;

		_21 = 0;
		_23 = 0;
		_24 = 0;

		_31 = 0;
		_32 = 0;
	
		_41 = 0;
		_42 = 0;
		_44 = 0;
	}
	
	void perspectiveFovSetHW(T fovy, T aspect)
	{
		// TODO: fix when not using float value
		T h = IG::perspectiveFovViewSpaceHeight(fovy);
		_11 = IG::perspectiveFovViewSpaceWidth(h, aspect);
		_22 = h;
	}
	
	void perspectiveFovLH(T fovy, T aspect, T znear, T zfar)
	{
		perspectiveFovSetZeros();
		perspectiveFovSetHW(fovy, aspect);
	
		_33 = zfar / (zfar - znear);
		_34 = 1;

		_43 = (-znear * zfar) / (zfar - znear);
	}
	
	void perspectiveFovRH(T fovy, T aspect, T znear, T zfar)
	{
		perspectiveFovSetZeros();
		perspectiveFovSetHW(fovy, aspect);

		_33 = zfar / (znear - zfar);
		_34 = -1;

		_43 = (znear * zfar) / (znear - zfar);
	}
	
	void perspectiveFrustum(T left, T right, T bottom, T top, T near, T far)
	{
		perspectiveFovSetZeros();

		_11 = (2 * near)/(right - left);
		_22 = (2 * near)/(top - bottom);

		_31 = (right + left)/(right - left);
		_32 = (top + bottom)/(top - bottom);
		_33 = -((far + near)/(far - near));
		_34 = -1;

		_43 = -((2*far*near)/(far - near));
	}

	void perspectiveFrustumWithView(T x, T y, T near, T far, T focal)
	{
		float yMax = y/2. * near / focal;
		float yMin = -yMax;
		float xMax = x/2. * near / focal;
		float xMin = -xMax;
		perspectiveFrustum(xMin, xMax, yMin, yMax, near, far);
	}

	void lookAt(Vector3d<T> *eye, Vector3d<T> *up, Vector3d<T> *zaxis)
	{
		Vector3d<T> xaxis;
		Vector3d<T>::crossProduct(&xaxis, up, zaxis);
		xaxis->normalize();

		Vector3d<T> yaxis;
		Vector3d<T>::crossProduct(&yaxis, zaxis, &xaxis);

		_11 = xaxis.x;
		_12 = yaxis.x;
		_13 = zaxis->x;
		_14 = 0;

		_21 = xaxis.y;
		_22 = yaxis.y;
		_23 = zaxis->y;
		_24 = 0;

		_31 = xaxis.z;
		_32 = yaxis.z;
		_33 = zaxis->z;
		_34 = 0;

		_41 = - xaxis.dotProduct(eye);
		_42 = - yaxis.dotProduct(eye);
		_43 = - zaxis->dotProduct(eye);
		_44 = 1;
	}
	
	void lookAtLH(Vector3d<T> *eye, Vector3d<T> *at, Vector3d<T> *up )
	{
		Vector3d<T> zaxis;
		Vector3d<T>::subtract(&zaxis, at, eye);
		zaxis.normalize();
	
		lookAt(eye, up, &zaxis);
	}
	
	void lookAtRH(Vector3d<T> *eye, Vector3d<T> *at, Vector3d<T> *up )
	{
		Vector3d<T> zaxis;
		Vector3d<T>::subtract(&zaxis, eye, at);
		zaxis.normalize();
	
		lookAt(eye, up, &zaxis);
	}
	
	void zRotationLH(T radians)
	{
		ident();
		_11 = IG::cos(radians);
		_12 = -IG::sin(radians);
		_21 = IG::sin(radians);
		_22 = IG::cos(radians);
	}

	void zRotationRH(T radians)
	{
		ident();
		_11 = IG::cos(radians);
		_12 = IG::sin(radians);
		_21 = -IG::sin(radians);
		_22 = IG::cos(radians);
	}

	void translate(T x, T y, T z)
	{
		ident();
		_41 = x;
		_42 = y;
		_43 = z;
	}

	bool invert(Matrix4x4<T> &invOut)
	{
		Matrix4x4<T> inv;

		inv.v[0] =   v[5]*v[10]*v[15] - v[5]*v[11]*v[14] - v[9]*v[6]*v[15]
				 + v[9]*v[7]*v[14] + v[13]*v[6]*v[11] - v[13]*v[7]*v[10];
		inv.v[4] =  -v[4]*v[10]*v[15] + v[4]*v[11]*v[14] + v[8]*v[6]*v[15]
				 - v[8]*v[7]*v[14] - v[12]*v[6]*v[11] + v[12]*v[7]*v[10];
		inv.v[8] =   v[4]*v[9]*v[15] - v[4]*v[11]*v[13] - v[8]*v[5]*v[15]
				 + v[8]*v[7]*v[13] + v[12]*v[5]*v[11] - v[12]*v[7]*v[9];
		inv.v[12] = -v[4]*v[9]*v[14] + v[4]*v[10]*v[13] + v[8]*v[5]*v[14]
				 - v[8]*v[6]*v[13] - v[12]*v[5]*v[10] + v[12]*v[6]*v[9];
		inv.v[1] =  -v[1]*v[10]*v[15] + v[1]*v[11]*v[14] + v[9]*v[2]*v[15]
				 - v[9]*v[3]*v[14] - v[13]*v[2]*v[11] + v[13]*v[3]*v[10];
		inv.v[5] =   v[0]*v[10]*v[15] - v[0]*v[11]*v[14] - v[8]*v[2]*v[15]
				 + v[8]*v[3]*v[14] + v[12]*v[2]*v[11] - v[12]*v[3]*v[10];
		inv.v[9] =  -v[0]*v[9]*v[15] + v[0]*v[11]*v[13] + v[8]*v[1]*v[15]
				 - v[8]*v[3]*v[13] - v[12]*v[1]*v[11] + v[12]*v[3]*v[9];
		inv.v[13] =  v[0]*v[9]*v[14] - v[0]*v[10]*v[13] - v[8]*v[1]*v[14]
				 + v[8]*v[2]*v[13] + v[12]*v[1]*v[10] - v[12]*v[2]*v[9];
		inv.v[2] =   v[1]*v[6]*v[15] - v[1]*v[7]*v[14] - v[5]*v[2]*v[15]
				 + v[5]*v[3]*v[14] + v[13]*v[2]*v[7] - v[13]*v[3]*v[6];
		inv.v[6] =  -v[0]*v[6]*v[15] + v[0]*v[7]*v[14] + v[4]*v[2]*v[15]
				 - v[4]*v[3]*v[14] - v[12]*v[2]*v[7] + v[12]*v[3]*v[6];
		inv.v[10] =  v[0]*v[5]*v[15] - v[0]*v[7]*v[13] - v[4]*v[1]*v[15]
				 + v[4]*v[3]*v[13] + v[12]*v[1]*v[7] - v[12]*v[3]*v[5];
		inv.v[14] = -v[0]*v[5]*v[14] + v[0]*v[6]*v[13] + v[4]*v[1]*v[14]
				 - v[4]*v[2]*v[13] - v[12]*v[1]*v[6] + v[12]*v[2]*v[5];
		inv.v[3] =  -v[1]*v[6]*v[11] + v[1]*v[7]*v[10] + v[5]*v[2]*v[11]
				 - v[5]*v[3]*v[10] - v[9]*v[2]*v[7] + v[9]*v[3]*v[6];
		inv.v[7] =   v[0]*v[6]*v[11] - v[0]*v[7]*v[10] - v[4]*v[2]*v[11]
				 + v[4]*v[3]*v[10] + v[8]*v[2]*v[7] - v[8]*v[3]*v[6];
		inv.v[11] = -v[0]*v[5]*v[11] + v[0]*v[7]*v[9] + v[4]*v[1]*v[11]
				 - v[4]*v[3]*v[9] - v[8]*v[1]*v[7] + v[8]*v[3]*v[5];
		inv.v[15] =  v[0]*v[5]*v[10] - v[0]*v[6]*v[9] - v[4]*v[1]*v[10]
				 + v[4]*v[2]*v[9] + v[8]*v[1]*v[6] - v[8]*v[2]*v[5];

		T det = v[0]*inv.v[0] + v[1]*inv.v[4] + v[2]*inv.v[8] + v[3]*inv.v[12];
		if (det == 0)
			return 0;

		det = 1.0 / det;

		iterateTimes(16, i)
			invOut.v[i] = inv.v[i] * det;

		return 1;
	}

	void print()
	{
		// TODO: only works for float types
		iterateTimes(4, i)
		{
			logMsg("[ %4.2f, %4.2f, %4.2f, %4.2f ]", v[0*4+i], v[1*4+i], v[2*4+i], v[3*4+i]);
		}
	}
};
