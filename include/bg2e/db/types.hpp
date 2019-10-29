/*
 *	bg2 engine license
 *	Copyright (c) 2016 Fernando Serrano <ferserc1@gmail.com>
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a copy
 *	of this software and associated documentation files (the "Software"), to deal
 *	in the Software without restriction, including without limitation the rights
 *	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *	of the Software, and to permit persons to whom the Software is furnished to do
 *	so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be included in all
 *	copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 *	INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 *	PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _bg2e_db_types_hpp_
#define _bg2e_db_types_hpp_

namespace bg2e {
namespace db {

template <class T>
union Vec2Generic {
	struct comp {
		T x;
		T y;
	} comp;
	T v[2];
};

template <class T>
union Vec3Generic {
	struct comp {
		T x;
		T y;
		T z;
	} comp;
	T v[3];
};

template <class T>
union Vec4Generic {
	struct comp {
		T x;
		T y;
		T z;
		T w;
	} comp;
	T v[4];
};

template <class T>
union Matrix3Generic {
	struct comp {
		T m00; T m01; T m02;
		T m10; T m11; T m12;
		T m20; T m21; T m22;
	} comp;
	float v[9];
};

template <class T>
union Matrix4Generic {
	struct comp {
		T m00; T m01; T m02; T m03;
		T m10; T m11; T m12; T m13;
		T m20; T m21; T m22; T m23;
		T m30; T m31; T m32; T m33;
	} comp;
	float v[16];
};

typedef Vec2Generic<float> Vec2;
typedef Vec3Generic<float> Vec3;
typedef Vec4Generic<float> Vec4;

typedef Vec2Generic<int> Vec2i;
typedef Vec3Generic<int> Vec3i;
typedef Vec4Generic<int> Vec4i;

typedef Matrix3Generic<float> Matrix3;
typedef Matrix4Generic<float> Matrix4;

}
}

#endif
