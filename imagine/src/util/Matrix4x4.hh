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
			T  v11, v12, v13, v14;
			T  v21, v22, v23, v24;
			T  v31, v32, v33, v34;
			T  v41, v42, v43, v44;
		};
	};

	constexpr Matrix4x4() { }

	void ident()
	{
		v11 = 1;
		v12 = 0;
		v13 = 0;
		v14 = 0;

		v21 = 0;
		v22 = 1;
		v23 = 0;
		v24 = 0;

		v31 = 0;
		v32 = 0;
		v33 = 1;
		v34 = 0;

		v41 = 0;
		v42 = 0;
		v43 = 0;
		v44 = 1;
	}
	
	void orthographicProjection(T xmax, T xmin, T ymax, T ymin, T zmax, T zmin)
	{
		v11 = (T)2 / (xmax - xmin);
		v12 = 0;
		v13 = 0;
		v14 = -((xmax + xmin) / (xmax - xmin));

		v21 = 0;
		v22 = (T)2 / (ymax - ymin);
		v23 = 0;
		v24 = -((ymax + ymin) / (ymax - ymin));

		v31 = 0;
		v32 = 0;
		v33 = (T)2 / (zmax - zmin);
		v34 = -((zmax + zmin) / (zmax - zmin));

		v41 = 0;
		v42 = 0;
		v43 = 0;
		v44 = 1;
	}
	
	void orthogonalLH(T width, T height, T zmax, T zmin)
	{
		v11 = (T)2 / width;
		v12 = 0;
		v13 = 0;
		v14 = 0;

		v21 = 0;
		v22 = (T)2 / height;
		v23 = 0;
		v24 = 0;
	
		v31 = 0;
		v32 = 0;
		v33 = (T)1 / (zmax - zmin);
		v34 = 0;

		v41 = 0;
		v42 = 0;
		v43 = zmin / (zmin - zmax);
		v44 = 1;
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
		result->v11 = (mat1->v11 * mat2->v11) + (mat1->v12 * mat2->v21) + (mat1->v13 * mat2->v31) + (mat1->v14 * mat2->v41);
		result->v12 = (mat1->v11 * mat2->v12) + (mat1->v12 * mat2->v22) + (mat1->v13 * mat2->v32) + (mat1->v14 * mat2->v42);
		result->v13 = (mat1->v11 * mat2->v13) + (mat1->v12 * mat2->v23) + (mat1->v13 * mat2->v33) + (mat1->v14 * mat2->v43);
		result->v14 = (mat1->v11 * mat2->v14) + (mat1->v12 * mat2->v24) + (mat1->v13 * mat2->v34) + (mat1->v14 * mat2->v44);

		result->v21 = (mat1->v21 * mat2->v11) + (mat1->v22 * mat2->v21) + (mat1->v23 * mat2->v31) + (mat1->v24 * mat2->v41);
		result->v22 = (mat1->v21 * mat2->v12) + (mat1->v22 * mat2->v22) + (mat1->v23 * mat2->v32) + (mat1->v24 * mat2->v42);
		result->v23 = (mat1->v21 * mat2->v13) + (mat1->v22 * mat2->v23) + (mat1->v23 * mat2->v33) + (mat1->v24 * mat2->v43);
		result->v24 = (mat1->v21 * mat2->v14) + (mat1->v22 * mat2->v24) + (mat1->v23 * mat2->v34) + (mat1->v24 * mat2->v44);

		result->v31 = (mat1->v31 * mat2->v11) + (mat1->v32 * mat2->v21) + (mat1->v33 * mat2->v31) + (mat1->v34 * mat2->v41);
		result->v32 = (mat1->v31 * mat2->v12) + (mat1->v32 * mat2->v22) + (mat1->v33 * mat2->v32) + (mat1->v34 * mat2->v42);
		result->v33 = (mat1->v31 * mat2->v13) + (mat1->v32 * mat2->v23) + (mat1->v33 * mat2->v33) + (mat1->v34 * mat2->v43);
		result->v34 = (mat1->v31 * mat2->v14) + (mat1->v32 * mat2->v24) + (mat1->v33 * mat2->v34) + (mat1->v34 * mat2->v44);

		result->v41 = (mat1->v41 * mat2->v11) + (mat1->v42 * mat2->v21) + (mat1->v43 * mat2->v31) + (mat1->v44 * mat2->v41);
		result->v42 = (mat1->v41 * mat2->v12) + (mat1->v42 * mat2->v22) + (mat1->v43 * mat2->v32) + (mat1->v44 * mat2->v42);
		result->v43 = (mat1->v41 * mat2->v13) + (mat1->v42 * mat2->v23) + (mat1->v43 * mat2->v33) + (mat1->v44 * mat2->v43);
		result->v44 = (mat1->v41 * mat2->v14) + (mat1->v42 * mat2->v24) + (mat1->v43 * mat2->v34) + (mat1->v44 * mat2->v44);
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
		v12 = 0;
		v13 = 0;
		v14 = 0;

		v21 = 0;
		v23 = 0;
		v24 = 0;

		v31 = 0;
		v32 = 0;
	
		v41 = 0;
		v42 = 0;
		v44 = 0;
	}
	
	void perspectiveFovSetHW(T fovy, T aspect)
	{
		// TODO: fix when not using float value
		T h = IG::perspectiveFovViewSpaceHeight(fovy);
		v11 = IG::perspectiveFovViewSpaceWidth(h, aspect);
		v22 = h;
	}
	
	void perspectiveFovLH(T fovy, T aspect, T znear, T zfar)
	{
		perspectiveFovSetZeros();
		perspectiveFovSetHW(fovy, aspect);
	
		v33 = zfar / (zfar - znear);
		v34 = 1;

		v43 = (-znear * zfar) / (zfar - znear);
	}
	
	void perspectiveFovRH(T fovy, T aspect, T znear, T zfar)
	{
		perspectiveFovSetZeros();
		perspectiveFovSetHW(fovy, aspect);

		v33 = zfar / (znear - zfar);
		v34 = -1;

		v43 = (znear * zfar) / (znear - zfar);
	}
	
	void perspectiveFrustum(T left, T right, T bottom, T top, T near, T far)
	{
		perspectiveFovSetZeros();

		v11 = (2 * near)/(right - left);
		v22 = (2 * near)/(top - bottom);

		v31 = (right + left)/(right - left);
		v32 = (top + bottom)/(top - bottom);
		v33 = -((far + near)/(far - near));
		v34 = -1;

		v43 = -((2*far*near)/(far - near));
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

		v11 = xaxis.x;
		v12 = yaxis.x;
		v13 = zaxis->x;
		v14 = 0;

		v21 = xaxis.y;
		v22 = yaxis.y;
		v23 = zaxis->y;
		v24 = 0;

		v31 = xaxis.z;
		v32 = yaxis.z;
		v33 = zaxis->z;
		v34 = 0;

		v41 = - xaxis.dotProduct(eye);
		v42 = - yaxis.dotProduct(eye);
		v43 = - zaxis->dotProduct(eye);
		v44 = 1;
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
		v11 = IG::cos(radians);
		v12 = -IG::sin(radians);
		v21 = IG::sin(radians);
		v22 = IG::cos(radians);
	}

	void zRotationRH(T radians)
	{
		ident();
		v11 = IG::cos(radians);
		v12 = IG::sin(radians);
		v21 = -IG::sin(radians);
		v22 = IG::cos(radians);
	}

	void translate(T x, T y, T z)
	{
		ident();
		v41 = x;
		v42 = y;
		v43 = z;
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
