
#ifndef _bg2math_matrix_hpp_
#define _bg2math_matrix_hpp_

#include <bg2math/utils.hpp>
#include <bg2math/vector.hpp>

#include <string>

#ifdef _MSC_VER
#ifndef isnan
#define std::isnan(x) _std::isnan(x)
#endif
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif
#endif

namespace bg2math {

	template <class T>
	class mat3x3 {
	public:

		static mat3x3<T> Identity() {
			return mat3x3(
				static_cast<T>(1), static_cast<T>(0), static_cast<T>(0),
				static_cast<T>(0), static_cast<T>(1), static_cast<T>(0),
				static_cast<T>(0), static_cast<T>(0), static_cast<T>(1));
		}

		mat3x3() :_m{
			static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
			static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
			static_cast<T>(0), static_cast<T>(0), static_cast<T>(0)
		} {}
		mat3x3(T v) :_m{
			v, v, v, v, v, v, v, v, v
		} {}
		mat3x3(T m00, T m01, T m02, T m03, T m04, T m05, T m06, T m07, T m08)
			:_m{ m00, m01, m02, m03, m04, m05, m06, m07, m08 }
		{}
		mat3x3(const vec3<T>& row0, const vec3<T>& row1, const vec3<T>& row2)
			:_m{ row0.x(), row0.y(), row0.z(),
				 row1.x(), row1.y(), row1.z(),
				 row2.x(), row2.y(), row2.z()
		} {}
		mat3x3(const mat3x3<T>& other) :_m{
			other._m[0], other._m[1], other._m[2],
			other._m[3], other._m[4], other._m[5],
			other._m[6], other._m[7], other._m[8]
		} {}

		inline bool isIdentity() const {
			return	_m[0] == static_cast<T>(1) && _m[1] == static_cast<T>(0) && _m[2] == static_cast<T>(0) &&
					_m[3] == static_cast<T>(0) && _m[4] == static_cast<T>(1) && _m[5] == static_cast<T>(0) &&
					_m[6] == static_cast<T>(0) && _m[7] == static_cast<T>(0) && _m[8] == static_cast<T>(1);
		}

		inline bool isZero() const {
			return	_m[0] == static_cast<T>(0) && _m[1] == static_cast<T>(0) && _m[2] == static_cast<T>(0) &&
					_m[3] == static_cast<T>(0) && _m[4] == static_cast<T>(0) && _m[5] == static_cast<T>(0) &&
					_m[6] == static_cast<T>(0) && _m[7] == static_cast<T>(0) && _m[8] == static_cast<T>(0);
		}

		inline bool isNaN() const {
			return	std::isnan(_m[0]) || std::isnan(_m[1]) || std::isnan(_m[2]) ||
					std::isnan(_m[3]) || std::isnan(_m[4]) || std::isnan(_m[5]) ||
					std::isnan(_m[6]) || std::isnan(_m[7]) || std::isnan(_m[8]);
		}

		inline T* operator&() { return _m; }
		inline T* raw() { return _m; }
		inline const T* raw() const { return _m; }
		inline T& operator[](int i) { return _m[i]; }
		inline T operator[](int i) const { return _m[i]; }
		inline void operator=(const mat3x3<T>& other) {
			_m[0] = other[0]; _m[1] = other[1]; _m[2] = other[2];
			_m[3] = other[3]; _m[4] = other[4]; _m[5] = other[5];
			_m[6] = other[6]; _m[7] = other[7]; _m[8] = other[8];
		}
		inline bool operator==(const mat3x3<T>& other) {
			return	_m[0] == other[0]; _m[1] == other[1]; _m[2] == other[2];
					_m[3] == other[3]; _m[4] == other[4]; _m[5] == other[5];
					_m[6] == other[6]; _m[7] == other[7]; _m[8] == other[8];
		}

		inline T& element(int i, int j) { return _m[i * 3 + j]; }
		inline T element(int i, int j) const { return _m[i * 3 + j]; }

		inline vec3<T> row(int i) const { return vec3<T>(_m[i * 3], _m[i * 3 + 1], _m[i * 3 + 2]); }
		inline void setRow(int i, const vec3<T>& value) { _m[i * 3] = value.x(); _m[i * 3 + 1] = value.y(); _m[i * 3 + 2] = value.z(); }
		inline void set(T v) {
			_m[0] = v; _m[1] = v; _m[2] = v;
			_m[3] = v; _m[4] = v; _m[5] = v;
			_m[6] = v; _m[7] = v; _m[8] = v;
		}

		inline vec3<T> eulerAngles() const {
			T rx = std::atan2(element(1,2), element(2,2));
			T c2 = sqrt(element(0,0) * element(0,0) + element(0,1) * element(0,1));
			T ry = std::atan2(-element(0,2), c2);
			T s1 = std::sin(rx);
			T c1 = std::cos(rx);
			T rz = std::atan2(s1 * element(2,0) - c1 * element(1,0), c1 * element(1,1) - s1 * element(2,1));
			return vec3<T>(rx, ry, rz);
		}

		inline std::string toString() {
			return	"[ [ " + std::to_string(_m[0]) + ", " + std::to_string(_m[1]) + ", " + std::to_string(_m[2]) + " ]\n" +
					"  [ " + std::to_string(_m[3]) + ", " + std::to_string(_m[4]) + ", " + std::to_string(_m[5]) + " ]\n" +
					"  [ " + std::to_string(_m[6]) + ", " + std::to_string(_m[7]) + ", " + std::to_string(_m[8]) + " ] ]";
		}

		inline mat3x3<T>& transpose() {
			vec3<T> r0(_m[0], _m[3], _m[6]);
			vec3<T> r1(_m[1], _m[4], _m[7]);
			vec3<T> r2(_m[2], _m[5], _m[8]);

			setRow(0, r0);
			setRow(1, r1);
			setRow(2, r2);

			return *this;
		}

	protected:
		T _m[9];
	};

	typedef mat3x3<float> float3x3;
	typedef mat3x3<double> double3x3;

	template <class T>
	inline mat3x3<T> operator *(const mat3x3<T>& a, const mat3x3<T> b) {
		mat3x3<T> res;
		const T* rm = a.raw();
		const T* lm = b.raw();

		res[0] = lm[0] * rm[0] + lm[1] * rm[1] + lm[2] * rm[2];
		res[1] = lm[0] * rm[1] + lm[1] * rm[4] + lm[2] * rm[7];
		res[2] = lm[0] * rm[2] + lm[1] * rm[5] + lm[2] * rm[8];

		res[3] = lm[3] * rm[0] + lm[4] * rm[3] + lm[5] * rm[6];
		res[4] = lm[3] * rm[1] + lm[4] * rm[4] + lm[5] * rm[7];
		res[5] = lm[3] * rm[2] + lm[4] * rm[5] + lm[5] * rm[8];

		res[6] = lm[6] * rm[0] + lm[7] * rm[3] + lm[8] * rm[6];
		res[7] = lm[6] * rm[1] + lm[7] * rm[4] + lm[8] * rm[7];
		res[8] = lm[6] * rm[2] + lm[7] * rm[5] + lm[8] * rm[8];

		return res;
	}


	template <class T>
	class mat4x4 {
	public:
		static mat4x4<T> Perspective(T fovy, T aspect, T nearPlane, T farPlane) {
			T fovy2 = std::tan(fovy * static_cast<T>(3.14159265359f) /	static_cast<T>(360)) * nearPlane;
			T fovy2aspect = fovy2 * aspect;
		
			return mat4x4<T>::Frustum(-fovy2aspect,fovy2aspect,-fovy2,fovy2,nearPlane,farPlane);
		}
	
		static mat4x4<T> Frustum(T left, T right, T bottom, T top, T nearPlane, T farPlane) {
			mat4x4 res;
			double A = static_cast<double>(right-left);
			double B = static_cast<double>(top-bottom);
			double C = static_cast<double>(farPlane-nearPlane);
		
			res.setRow(0, vec4<T>(static_cast<T>(nearPlane*2.0/A), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0)));
			res.setRow(1, vec4<T>(static_cast<T>(0), static_cast<T>(nearPlane*2.0/B), static_cast<T>(0), static_cast<T>(0)));
			res.setRow(2, vec4<T>((right+left)/static_cast<T>(A), (top+bottom)/static_cast<T>(B), static_cast<T>((-farPlane-nearPlane)/C), static_cast<T>(-1)));
			res.setRow(3, vec4<T>(static_cast<T>(0), static_cast<T>(0),	static_cast<T>((-farPlane*nearPlane*2.0)/C), static_cast<T>(0)));
		
			return res;
		}
	
		static mat4x4<T> Ortho(T left, T right, T bottom, T top, T nearPlane, T farPlane) {
			mat4x4<T> p;
			T m = right-left;
			T l = top-bottom;
			T k=farPlane-nearPlane;;
		
			p[0] = static_cast<T>(2)/m; p[1] = static_cast<T>(0); p[2] = static_cast<T>(0); p[3] = static_cast<T>(0);
			p[4] = static_cast<T>(0); p[5] = static_cast<T>(2)/l; p[6] = static_cast<T>(0); p._m[7] = static_cast<T>(0);
			p[8] = static_cast<T>(0); p[9] = static_cast<T>(0); p[10] = static_cast<T>(-2)/k; p[11]=static_cast<T>(0);
			p[12]=-(left+right)/m; p[13] = -(top+bottom)/l; p[14] = -(farPlane+nearPlane)/k; p._m[15]=static_cast<T>(1);
		
			return p;
		}
	
		static mat4x4<T> LookAt(const vec3<T> & p_eye, const vec3<T> & p_center, const vec3<T> & p_up) {
			vec3<T>  f = normalize(p_center - p_eye);
			vec3<T>  u = normalize(p_up);
			vec3<T>  s = normalize(cross(f, u));
			u = cross(s, f);

			mat4x4<T> result;
			result.element(0,0) = s.x();
			result.element(1,0) = s.y();
			result.element(2,0) = s.z();
			result.element(0,1) = u.x();
			result.element(1,1) = u.y();
			result.element(2,1) = u.z();
			result.element(0,2) = -f.x();
			result.element(1,2) = -f.y();
			result.element(2,2) = -f.z();
			result.element(3,0) = -dot(s, p_eye);
			result.element(3,1) = -dot(u, p_eye);
			result.element(3,2) = dot(f, p_eye);
			result.element(3,3) = static_cast<T>(1);
			return result;
		}
	
		static mat4x4<T> Translation(T x, T y, T z) {
			mat4x4<T> t;
			t[ 0] = static_cast<T>(1); t[ 1] = static_cast<T>(0); t[ 2] = static_cast<T>(0); t[ 3] = static_cast<T>(0);
			t[ 4] = static_cast<T>(0); t[ 5] = static_cast<T>(1); t[ 6] = static_cast<T>(0); t[ 7] = static_cast<T>(0);
			t[ 8] = static_cast<T>(0); t[ 9] = static_cast<T>(0); t[10] = static_cast<T>(1); t[11] = static_cast<T>(0);
			t[12] =    x; t[13] =    y; t[14] =    z; t[15] = static_cast<T>(1);
			return t;
		}
	
	
		static mat4x4<T> Translation(const vec3<T> & t) {
			return Translation(t.x(), t.y(), t.z());
		}
	
		static mat4x4<T> Rotation(T alpha, T x, T y, T z) {
			vec3<T> axis(x,y,z);
			axis = normalize(axis);
			mat4x4<T> rot;
		
			rot = mat4x4<T>::Identity();
		
			T cosAlpha = std::cos(alpha);
			T acosAlpha = static_cast<T>(1) - cosAlpha;
			T sinAlpha = std::sin(alpha);
		
			rot[0] = axis.x() * axis.x() * acosAlpha + cosAlpha;
			rot[1] = axis.x() * axis.y() * acosAlpha + axis.z() * sinAlpha;
			rot[2] = axis.x() * axis.z() * acosAlpha - axis.y() * sinAlpha;
		
			rot[4] = axis.y() * axis.x() * acosAlpha - axis.z() * sinAlpha;
			rot[5] = axis.y() * axis.y() * acosAlpha + cosAlpha;
			rot[6] = axis.y() * axis.z() * acosAlpha + axis.x() * sinAlpha;
		
			rot[8] = axis.z() * axis.x() * acosAlpha + axis.y() * sinAlpha;
			rot[9] = axis.z() * axis.y() * acosAlpha - axis.x() * sinAlpha;
			rot[10] = axis.z() * axis.z() * acosAlpha + cosAlpha;
		
			return rot;
		}
	
		static mat4x4<T> Scale(T x, T y, T z) {
			mat4x4<T> s;
		
			s[ 0] = x;                  s[ 1] = static_cast<T>(0);  s[ 2] = static_cast<T>(0);  s[ 3] = static_cast<T>(0);
			s[ 4] = static_cast<T>(0);  s[ 5] = y;                  s[ 6] = static_cast<T>(0);  s[ 7] = static_cast<T>(0);
			s[ 8] = static_cast<T>(0);  s[ 9] = static_cast<T>(0);  s[10] = z;                  s[11] = static_cast<T>(0);
			s[12] = static_cast<T>(0);  s[13] = static_cast<T>(0);  s[14] = static_cast<T>(0);  s[15] = static_cast<T>(1);
		
			return s;
		}
	
		static mat4x4<T> Scale(const vec3<T> & s) {
			return Scale(s.x(), s.y(), s.z());
		}

		static mat4x4<T> Identity() {
			return mat4x4(
				static_cast<T>(1), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
				static_cast<T>(0), static_cast<T>(1), static_cast<T>(0), static_cast<T>(0),
				static_cast<T>(0), static_cast<T>(0), static_cast<T>(1), static_cast<T>(0),
				static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1));
		}

		mat4x4() :_m{
			static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
			static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
			static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
			static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0)
		} {}
		mat4x4(T v) :_m{
			v, v, v, v,
			v, v, v, v,
			v, v, v, v,
			v, v, v, v
		} {}
		mat4x4(
			T m00, T m01, T m02, T m03,
			T m10, T m11, T m12, T m13,
			T m20, T m21, T m22, T m23,
			T m30, T m31, T m32, T m33)
		:_m{
			m00, m01, m02, m03,
			m10, m11, m12, m13,
			m20, m21, m22, m23,
			m30, m31, m32, m33
			}
		{}
		mat4x4(const vec4<T>& row0, const vec4<T>& row1, const vec4<T>& row2, const vec4<T>& row3)
			:_m{ row0.x(), row0.y(), row0.z(), row0.w(),
				 row1.x(), row1.y(), row1.z(), row1.w(),
				 row2.x(), row2.y(), row2.z(), row2.w(),
				 row3.x(), row3.y(), row3.z(), row3.w()
		} {}
		mat4x4(const mat4x4<T>& other) :_m{
			other._m[ 0], other._m[ 1], other._m[ 2], other._m[ 3],
			other._m[ 4], other._m[ 5], other._m[ 6], other._m[ 7],
			other._m[ 8], other._m[ 9], other._m[10], other._m[11],
			other._m[12], other._m[13], other._m[14], other._m[15]
		} {}
		mat4x4(const mat3x3<T>& other) :_m{
			other[0], other[1], other[2], static_cast<T>(0),
			other[3], other[4], other[5], static_cast<T>(0),
			other[6], other[7], other[8], static_cast<T>(0),
			static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1)
		} {}

		inline mat4x4<T>& perspective(float fovy, float aspect, float nearPlane, float farPlane) {
			*this = mat4x4<T>::Perspective(fovy, aspect, nearPlane, farPlane);
			return *this;
		}

		inline mat4x4<T>& frustum(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
			*this = mat4x4<T>::Frustum(left, right, bottom, top, nearPlane, farPlane);
			return *this;
		}

		inline mat4x4<T>& ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
			*this = mat4x4<T>::Ortho(left, right, bottom, top, nearPlane, farPlane);
			return *this;
		}

		inline mat4x4<T>& lookAt(const vec3<T>& origin, const vec3<T>& target, const vec3<T>& up) {
			*this = mat4x4<T>::LookAt(origin, target, up);
			return *this;
		}

		inline mat4x4<T>& translate(float x, float y, float z) {
			*this = *this * mat4x4<T>::Translation(x, y, z);
			return *this;
		}

		inline mat4x4<T>& translate(const vec3<T>& t) {
			*this = *this * mat4x4<T>::Translation(t);
			return *this;
		}

		inline mat4x4<T>& rotate(float alpha, float x, float y, float z) {
			*this = *this * mat4x4<T>::Rotation(alpha, x, y, z);
			return *this;
		}

		inline mat4x4<T>& scale(float x, float y, float z) {
			*this = *this * mat4x4<T>::Scale(x, y, z);
			return *this;
		}

		inline mat4x4<T>& scale(const vec3<T>& s) {
			*this = *this * mat4x4<T>::Scale(s);
			return *this;
		}

		inline bool isIdentity() const {
			return	_m[ 0] == static_cast<T>(1) && _m[ 1] == static_cast<T>(0) && _m[ 2] == static_cast<T>(0) && _m[ 3] == static_cast<T>(0) &&
					_m[ 4] == static_cast<T>(0) && _m[ 5] == static_cast<T>(0) && _m[ 6] == static_cast<T>(0) && _m[ 7] == static_cast<T>(0) &&
					_m[ 8] == static_cast<T>(0) && _m[ 9] == static_cast<T>(0) && _m[10] == static_cast<T>(1) && _m[11] == static_cast<T>(0) &&
					_m[12] == static_cast<T>(0) && _m[13] == static_cast<T>(0) && _m[14] == static_cast<T>(0) && _m[15] == static_cast<T>(1);
		}

		inline bool isZero() const {
			return	_m[ 0] == static_cast<T>(0) && _m[ 1] == static_cast<T>(0) && _m[ 2] == static_cast<T>(0) && _m[ 3] == static_cast<T>(0) &&
					_m[ 4] == static_cast<T>(0) && _m[ 5] == static_cast<T>(0) && _m[ 6] == static_cast<T>(0) && _m[ 7] == static_cast<T>(0) &&
					_m[ 8] == static_cast<T>(0) && _m[ 9] == static_cast<T>(0) && _m[10] == static_cast<T>(0) && _m[11] == static_cast<T>(0) &&
					_m[12] == static_cast<T>(0) && _m[13] == static_cast<T>(0) && _m[14] == static_cast<T>(0) && _m[15] == static_cast<T>(0);
		}

		inline bool isNaN() const {
			return	std::isnan(_m[ 0]) || std::isnan(_m[ 1]) || std::isnan(_m[ 2]) || std::isnan(_m[ 3]) ||
					std::isnan(_m[ 4]) || std::isnan(_m[ 5]) || std::isnan(_m[ 6]) || std::isnan(_m[ 7]) ||
					std::isnan(_m[ 8]) || std::isnan(_m[ 9]) || std::isnan(_m[10]) || std::isnan(_m[11]) ||
					std::isnan(_m[12]) || std::isnan(_m[13]) || std::isnan(_m[14]) || std::isnan(_m[15]);
		}

		inline T* operator&() { return _m; }
		inline T* raw() { return _m; }
		inline const T* raw() const { return _m; }
		inline T& operator[](int i) { return _m[i]; }
		inline T operator[](int i) const { return _m[i]; }
		inline void operator=(const mat4x4<T>& other) {
			_m[ 0] = other[ 0]; _m[ 1] = other[ 1]; _m[ 2] = other[ 2]; _m[ 3] = other[ 3];
			_m[ 4] = other[ 4]; _m[ 5] = other[ 5]; _m[ 6] = other[ 6]; _m[ 7] = other[ 7];
			_m[ 8] = other[ 8]; _m[ 9] = other[ 9]; _m[10] = other[10]; _m[11] = other[11];
			_m[12] = other[12]; _m[13] = other[13]; _m[14] = other[14]; _m[15] = other[15];
		}
		inline bool operator==(const mat4x4<T>& other) {
			return	_m[ 0] == other[ 0] && _m[ 1] == other[ 1] && _m[ 2] == other[ 2] && _m[ 3] == other[ 3] &&
					_m[ 4] == other[ 4] && _m[ 5] == other[ 5] && _m[ 6] == other[ 6] && _m[ 7] == other[ 7] &&
					_m[ 8] == other[ 8] && _m[ 9] == other[ 9] && _m[10] == other[10] && _m[11] == other[11] &&
					_m[12] == other[12] && _m[13] == other[13] && _m[14] == other[14] && _m[15] == other[15];
		}

		inline T& element(int i, int j) { return _m[i * 4 + j]; }
		inline T element(int i, int j) const { return _m[i * 4 + j]; }
		inline void set(T v) {
			_m[ 0] = v; _m[ 1] = v; _m[ 2] = v; _m[ 3] = v;
			_m[ 4] = v; _m[ 5] = v; _m[ 6] = v; _m[ 7] = v;
			_m[ 8] = v; _m[ 9] = v; _m[10] = v; _m[11] = v;
			_m[12] = v; _m[13] = v; _m[14] = v; _m[15] = v;
		}

		inline vec4<T> row(int i) const { return vec4<T>(_m[i * 4], _m[i * 4 + 1], _m[i * 4 + 2], _m[i * 4 + 3]); }
		inline void setRow(int i, const vec4<T>& value) { _m[i * 4] = value.x(); _m[i * 4 + 1] = value.y(); _m[i * 4 + 2] = value.z(); _m[i * 4 + 3] = value.w();}
		inline mat3x3<T> rotation() const {
			return mat3x3<T>(
				_m[0], _m[1], _m[2],
				_m[4], _m[5], _m[6],
				_m[8], _m[9], _m[10]
			);
		}
		inline vec3<T> position() const {
			return vec3<T>(_m[12], _m[13], _m[14]);
		}
		inline void setPosition(const vec3<T>& pos) {
			_m[12] = pos.x(); _m[13] = pos.y(); _m[14] = pos.z();
		}
		inline void setPosition(const vec4<T>& pos) {
			_m[12] = pos.x(); _m[13] = pos.y(); _m[14] = pos.z();
		}

		inline vec3<T> eulerAngles() const {
			T rx = std::atan2(element(1, 2), element(2, 2));
			T c2 = sqrt(element(0, 0) * element(0, 0) + element(0, 1) * element(0, 1));
			T ry = std::atan2(-element(0, 2), c2);
			T s1 = std::sin(rx);
			T c1 = std::cos(rx);
			T rz = std::atan2(s1 * element(2, 0) - c1 * element(1, 0), c1 * element(1, 1) - s1 * element(2, 1));
			return vec3<T>(rx, ry, rz);
		}

		inline std::string toString() {
			return	"[ [ " + std::to_string(_m[ 0]) + ", " + std::to_string(_m[ 1]) + ", " + std::to_string(_m[ 2]) + ", " + std::to_string(_m[ 3]) + " ]\n" +
					"  [ " + std::to_string(_m[ 4]) + ", " + std::to_string(_m[ 5]) + ", " + std::to_string(_m[ 6]) + ", " + std::to_string(_m[ 7]) + " ]\n" +
					"  [ " + std::to_string(_m[ 8]) + ", " + std::to_string(_m[ 9]) + ", " + std::to_string(_m[10]) + ", " + std::to_string(_m[11]) + " ]\n" +
					"  [ " + std::to_string(_m[12]) + ", " + std::to_string(_m[13]) + ", " + std::to_string(_m[14]) + ", " + std::to_string(_m[15]) + " ] ]";
		}

		inline mat4x4<T> & invert() {
			T inv[16];
		
			inv[0] = _m[5] * _m[10] * _m[15] - _m[5] * _m[11] * _m[14] - _m[9] * _m[6] * _m[15] + _m[9] * _m[7] * _m[14] + _m[13] * _m[6] * _m[11] - _m[13] * _m[7] * _m[10];
			inv[4] = -_m[4] * _m[10] * _m[15] + _m[4] * _m[11] * _m[14] + _m[8] * _m[6] * _m[15] - _m[8] * _m[7] * _m[14] - _m[12] * _m[6] * _m[11] + _m[12] * _m[7] * _m[10];
			inv[8] = _m[4] * _m[9] * _m[15] - _m[4] * _m[11] * _m[13] - _m[8] * _m[5] * _m[15] + _m[8] * _m[7] * _m[13] + _m[12] * _m[5] * _m[11] - _m[12] * _m[7] * _m[9];
			inv[12] = -_m[4] * _m[9] * _m[14] + _m[4] * _m[10] * _m[13] + _m[8] * _m[5] * _m[14] - _m[8] * _m[6] * _m[13] - _m[12] * _m[5] * _m[10] + _m[12] * _m[6] * _m[9];
			inv[1] = -_m[1] * _m[10] * _m[15] + _m[1] * _m[11] * _m[14] + _m[9] * _m[2] * _m[15] - _m[9] * _m[3] * _m[14] - _m[13] * _m[2] * _m[11] + _m[13] * _m[3] * _m[10];
			inv[5] = _m[0] * _m[10] * _m[15] - _m[0] * _m[11] * _m[14] - _m[8] * _m[2] * _m[15] + _m[8] * _m[3] * _m[14] + _m[12] * _m[2] * _m[11] - _m[12] * _m[3] * _m[10];
			inv[9] = -_m[0] * _m[9] * _m[15] + _m[0] * _m[11] * _m[13] + _m[8] * _m[1] * _m[15] - _m[8] * _m[3] * _m[13] - _m[12] * _m[1] * _m[11] + _m[12] * _m[3] * _m[9];
			inv[13] = _m[0] * _m[9] * _m[14] - _m[0] * _m[10] * _m[13] - _m[8] * _m[1] * _m[14] + _m[8] * _m[2] * _m[13] + _m[12] * _m[1] * _m[10] - _m[12] * _m[2] * _m[9];
			inv[2] = _m[1] * _m[6] * _m[15] - _m[1] * _m[7] * _m[14] - _m[5] * _m[2] * _m[15] + _m[5] * _m[3] * _m[14] + _m[13] * _m[2] * _m[7] - _m[13] * _m[3] * _m[6];
			inv[6] = -_m[0] * _m[6] * _m[15] + _m[0] * _m[7] * _m[14] + _m[4] * _m[2] * _m[15] - _m[4] * _m[3] * _m[14] - _m[12] * _m[2] * _m[7] + _m[12] * _m[3] * _m[6];
			inv[10] = _m[0] * _m[5] * _m[15] - _m[0] * _m[7] * _m[13] - _m[4] * _m[1] * _m[15] + _m[4] * _m[3] * _m[13] + _m[12] * _m[1] * _m[7] - _m[12] * _m[3] * _m[5];
			inv[14] = -_m[0] * _m[5] * _m[14] + _m[0] * _m[6] * _m[13] + _m[4] * _m[1] * _m[14] - _m[4] * _m[2] * _m[13] - _m[12] * _m[1] * _m[6] + _m[12] * _m[2] * _m[5];
			inv[3] = -_m[1] * _m[6] * _m[11] + _m[1] * _m[7] * _m[10] + _m[5] * _m[2] * _m[11] - _m[5] * _m[3] * _m[10] - _m[9] * _m[2] * _m[7] + _m[9] * _m[3] * _m[6];
			inv[7] = _m[0] * _m[6] * _m[11] - _m[0] * _m[7] * _m[10] - _m[4] * _m[2] * _m[11] + _m[4] * _m[3] * _m[10] + _m[8] * _m[2] * _m[7] - _m[8] * _m[3] * _m[6];
			inv[11] = -_m[0] * _m[5] * _m[11] + _m[0] * _m[7] * _m[9] + _m[4] * _m[1] * _m[11] - _m[4] * _m[3] * _m[9] - _m[8] * _m[1] * _m[7] + _m[8] * _m[3] * _m[5];
			inv[15] = _m[0] * _m[5] * _m[10] - _m[0] * _m[6] * _m[9] - _m[4] * _m[1] * _m[10] + _m[4] * _m[2] * _m[9] + _m[8] * _m[1] * _m[6] - _m[8] * _m[2] * _m[5];
		
			T det = _m[0] * inv[0] + _m[1] * inv[4] + _m[2] * inv[8] + _m[3] * inv[12];
		
			if (det==static_cast<T>(0)) {
				set(static_cast<T>(0));
			}
			else {
				det = static_cast<T>(1) / det;
		
				_m[0]  = inv[0]  * det;
				_m[1]  = inv[1]  * det;
				_m[2]  = inv[2]  * det;
				_m[3]  = inv[3]  * det;
				_m[4]  = inv[4]  * det;
				_m[5]  = inv[5]  * det;
				_m[6]  = inv[6]  * det;
				_m[7]  = inv[7]  * det;
				_m[8]  = inv[8]  * det;
				_m[9]  = inv[9]  * det;
				_m[10] = inv[10] * det;
				_m[11] = inv[11] * det;
				_m[12] = inv[12] * det;
				_m[13] = inv[13] * det;
				_m[14] = inv[14] * det;
				_m[15] = inv[15] * det;
			}
		
			return *this;
		}

		inline mat3x3<T>& transpose() {
			vec4<T> r0(_m[0], _m[4], _m[8], _m[12]);
			vec4<T> r1(_m[1], _m[5], _m[9], _m[13]);
			vec4<T> r2(_m[2], _m[6], _m[10], _m[14]);
			vec4<T> r3(_m[3], _m[7], _m[11], _m[15]);

			setRow(0, r0);
			setRow(1, r1);
			setRow(2, r2);
			setRow(3, r3);
			return *this;
		}

	protected:
		T _m[16];
	};

	typedef mat4x4<float> float4x4;
	typedef mat4x4<double> double4x4;

	template <class T>
	inline mat4x4<T> operator *(const mat4x4<T>& a, const mat4x4<T> b) {
		mat4x4<T> res;
		const T* rm = a.raw();
		const T* lm = b.raw();

		res[ 0] = lm[ 0] * rm[ 0] + lm[ 1] * rm[ 4] + lm[ 2] * rm[ 8] + lm[ 3] * rm[12];
		res[ 1] = lm[ 0] * rm[ 1] + lm[ 1] * rm[ 5] + lm[ 2] * rm[ 9] + lm[ 3] * rm[13];
		res[ 2] = lm[ 0] * rm[ 2] + lm[ 1] * rm[ 6] + lm[ 2] * rm[10] + lm[ 3] * rm[14];
		res[ 3] = lm[ 0] * rm[ 3] + lm[ 1] * rm[ 7] + lm[ 2] * rm[11] + lm[ 3] * rm[15];
		
		res[ 4] = lm[ 4] * rm[ 0] + lm[ 5] * rm[ 4] + lm[ 6] * rm[ 8] + lm[ 7] * rm[12];
		res[ 5] = lm[ 4] * rm[ 1] + lm[ 5] * rm[ 5] + lm[ 6] * rm[ 9] + lm[ 7] * rm[13];
		res[ 6] = lm[ 4] * rm[ 2] + lm[ 5] * rm[ 6] + lm[ 6] * rm[10] + lm[ 7] * rm[14];
		res[ 7] = lm[ 4] * rm[ 3] + lm[ 5] * rm[ 7] + lm[ 6] * rm[11] + lm[ 7] * rm[15];
		
		res[ 8] = lm[ 8] * rm[ 0] + lm[ 9] * rm[ 4] + lm[10] * rm[ 8] + lm[11] * rm[12];
		res[ 9] = lm[ 8] * rm[ 1] + lm[ 9] * rm[ 5] + lm[10] * rm[ 9] + lm[11] * rm[13];
		res[10] = lm[ 8] * rm[ 2] + lm[ 9] * rm[ 6] + lm[10] * rm[10] + lm[11] * rm[14];
		res[11] = lm[ 8] * rm[ 3] + lm[ 9] * rm[ 7] + lm[10] * rm[11] + lm[11] * rm[15];
		
		res[12] = lm[12] * rm[ 0] + lm[13] * rm[ 4] + lm[14] * rm[ 8] + lm[15] * rm[12];
		res[13] = lm[12] * rm[ 1] + lm[13] * rm[ 5] + lm[14] * rm[ 9] + lm[15] * rm[13];
		res[14] = lm[12] * rm[ 2] + lm[13] * rm[ 6] + lm[14] * rm[10] + lm[15] * rm[14];
		res[15] = lm[12] * rm[ 3] + lm[13] * rm[ 7] + lm[14] * rm[11] + lm[15] * rm[15];

		return res;
	}

	template <class T>
	inline mat4x4<T> operator *(const mat4x4<T>& m, const vec3<T>& v) {
		mat4x4<T> res;
		T x = v[0]; T y = v[1]; T z = v[2]; T w = static_cast<T>(1);

		res[0] = m[0] * x + m[4] * y + m[8] * z +  m[12] * w;
		res[1] = m[1] * x + m[5] * y + m[9] * z +  m[13] * w;
		res[2] = m[2] * x + m[6] * y + m[10] * z + m[14] * w;
		res[3] = m[3] * x + m[7] * y + m[11] * z + m[15] * w;

		return res;
	}

	template <class T>
	inline mat4x4<T> operator *(const mat4x4<T>& m, const vec4<T>& v) {
		mat4x4<T> res;
		T x = v[0]; T y = v[1]; T z = v[2]; T w = v[3];

		res[0] = m[0] * x + m[4] * y + m[8] * z +  m[12] * w;
		res[1] = m[1] * x + m[5] * y + m[9] * z +  m[13] * w;
		res[2] = m[2] * x + m[6] * y + m[10] * z + m[14] * w;
		res[3] = m[3] * x + m[7] * y + m[11] * z + m[15] * w;

		return res;
	}


}

#endif