
#ifndef _bg2e_math_vector_hpp_
#define _bg2e_math_vector_hpp_

#include <cmath>
#include <string>
#include <vector>
#include <algorithm>

namespace bg2e {
    namespace math {

    template <class T>
    class vec2 {
    public:
        vec2<T>() {}
        vec2<T>(T x, T y) :_v{ x, y } {}
        vec2<T>(const vec2<T> & other) :_v{ other[0], other[1] } {}
        vec2<T>(T * v) :_v{ v[0], v[1] } {}
        vec2<T>(const std::vector<T> & v) :_v{ v[0], v[1] } {}

        inline T * operator&() { return _v; }
        inline T & operator[](int i) { return _v[i]; }
        inline T operator[](int i) const { return _v[i]; }
        inline void operator=(const vec2<T> & other) { _v[0] = other[0]; _v[1] = other[1]; }
        inline bool operator==(const vec2<T> & other) { return _v[0] == other[0]; _v[1] == other[1]; }
        
        inline T length() const { return sqrt(_v[0]*_v[0] + _v[1]*_v[1]); }
        
        inline vec2<T> & normalize() {
            T l = length();
            _v[0] = _v[0] / l;
            _v[1] = _v[1] / l;
            return *this;
        }
        
        inline std::string toString() const {
            return "[" + std::to_string(_v[0]) + "," + std::to_string(_v[1]) + "]";
        }
        
        inline T & x() { return _v[0]; }
        inline T x() const { return _v[0]; }
        inline T & y() { return _v[1]; }
        inline T y() const { return _v[1]; }

		inline T& width() { return _v[0]; }
		inline const T& width() const { return _v[0]; }
		inline T& height() { return _v[1]; }
		inline const T& height() const { return _v[1]; }
        
        inline bool belongsToRect(T left, T top, T width, T height) {
            return _v[0] >= left && _v[0] <= left + width && _v[1] >= top && _v[1] <= top + height;
        }
        
    protected:
        T _v[2] = { static_cast<T>(0), static_cast<T>(0) };
    };
    
    template <class T> inline T distance(const vec2<T> & v1, const vec2<T> & v2) {
        return vec2<T>(v1[0] - v2[0], v1[1] - v2[1]).length();
    }
    template <class T> inline T length(const vec2<T> & v) { return v.length(); }
    template <class T> inline vec2<T> normalize(const vec2<T> & v) {
        vec2<T> result = v;
        result.normalize();
        return result;
    }
    template <class T> inline vec2<T> operator + (const vec2<T> & v1, const vec2<T> & v2) {
        return vec2<T>(v1[0] + v2[0], v1[1] + v2[1]);
    }
    template <class T> inline vec2<T> operator - (const vec2<T> & v1, const vec2<T> & v2) {
        return vec2<T>(v1[0] - v2[0], v1[1] - v2[1]);
    }
    template <class T> inline vec2<T> operator * (const vec2<T> & v1, const vec2<T> & v2) {
        return vec2<T>(v1[0] * v2[0], v1[1] * v2[1]);
    }
    template <class T> inline vec2<T> operator / (const vec2<T> & v1, const vec2<T> & v2) {
        return vec2<T>(v1[0] / v2[0], v1[1] / v2[1]);
    }
    template <class T> inline vec2<T> operator * (const vec2<T> & v, T s) {
        return vec2<T>(v[0] * s, v[1] * s);
    }
    template <class T> inline vec2<T> operator * (T s, const vec2<T> & v) {
        return vec2<T>(v[0] * s, v[1] * s);
    }
    template <class T> inline vec2<T> operator / (const vec2<T> & v, T s) {
        return vec2<T>(v[0] / s, v[1] / s);
    }
    template <class T> inline T dot(const vec2<T> & v1, const vec2<T> & v2) {
        return v1[0] * v2[0] + v1[1] * v2[1];
    }
    template <class T> inline bool isnan(const vec2<T> & v) {
        return std::isnan(static_cast<double>(v[0])) || std::isnan(static_cast<double>(v[1]));
    }
    template <class T> inline bool isinf(const vec2<T> & v) {
        return std::isinf(static_cast<double>(v[0])) || std::isinf(static_cast<double>(v[1]));
    }
    template <class T> inline bool iszero(const vec2<T> & v) {
        return v[0] == 0 && v[1] == 0;
    }
    template <class T> inline bool isvalid(const vec2<T> & v) {
        return !isnan(v) && !isinf(v);
    }
    
    
    typedef vec2<int8_t> byte2;
    typedef vec2<int16_t> short2;
    typedef vec2<int32_t> int2;
    typedef vec2<uint8_t> ubyte2;
    typedef vec2<uint16_t> ushort2;
    typedef vec2<uint32_t> uint2;
    typedef vec2<float> float2;
    typedef vec2<double> double2;
    
    
    
    
    
    template <class T>
    class vec3 {
    public:
        vec3<T>() {}
        vec3<T>(T x, T y, T z) :_v{ x, y, z } {}
        vec3<T>(const vec3<T> & other) :_v{ other[0], other[1], other[2] } {}
        vec3<T>(T * v) :_v{ v[0], v[1], v[2] } {}
        vec3<T>(const std::vector<T> & v) :_v{ v[0], v[1], v[2] } {}
        
        inline T * operator&() { return _v; }
        inline T & operator[](int i) { return _v[i]; }
        inline T operator[](int i) const { return _v[i]; }
        inline void operator=(const vec3<T> & other) { _v[0] = other[0]; _v[1] = other[1]; _v[2] = other[2]; }
        inline bool operator==(const vec3<T> & other) { return _v[0] == other[0]; _v[1] == other[1]; _v[2] == other[2]; }
        
        inline T length() const { return sqrt(_v[0]*_v[0] + _v[1]*_v[1] + _v[2]*_v[2]); }
        
        inline vec3<T> & normalize() {
            T l = length();
            _v[0] = _v[0] / l;
            _v[1] = _v[1] / l;
            _v[2] = _v[2] / l;
            return *this;
        }
        
        inline std::string toString() const {
            return "[" + std::to_string(_v[0]) + "," + std::to_string(_v[1]) + "," + std::to_string(_v[2]) + "]";
        }
        
        inline T & x() { return _v[0]; }
        inline T x() const { return _v[0]; }
        inline T & y() { return _v[1]; }
        inline T y() const { return _v[1]; }
        inline T & z() { return _v[2]; }
        inline T z() const { return _v[2]; }
        
        inline vec2<T> xy() const { return vec2<T>(_v[0], _v[1]); }
        inline vec2<T> yz() const { return vec2<T>(_v[1], _v[2]); }
        inline vec2<T> xz() const { return vec2<T>(_v[0], _v[2]); }
        
        inline T & r() { return _v[0]; }
        inline T r() const { return _v[0]; }
        inline T & g() { return _v[1]; }
        inline T g() const { return _v[1]; }
        inline T & b() { return _v[2]; }
        inline T b() const { return _v[2]; }

		inline T& width() { return _v[0]; }
		inline const T& width() const { return _v[0]; }
		inline T& height() { return _v[1]; }
		inline const T& height() const { return _v[1]; }
		inline T& depth() { return _v[2]; }
		inline const T& depth() const { return _v[2]; }
        
    protected:
        T _v[3] = { static_cast<T>(0), static_cast<T>(0), static_cast<T>(0) };
    };
    
    template <class T> inline T distance(const vec3<T> & v1, const vec3<T> & v2) {
        return vec3<T>(v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2]).length();
    }
    template <class T> inline T length(const vec3<T> & v) { return v.length(); }
    template <class T> inline vec3<T> normalize(const vec3<T> & v) {
        vec3<T> result = v;
        result.normalize();
        return result;
    }
    template <class T> inline vec3<T> operator + (const vec3<T> & v1, const vec3<T> & v2) {
        return vec3<T>(v1[0] + v2[0], v1[1] + v2[1], v1[2] + v2[2]);
    }
    template <class T> inline vec3<T> operator - (const vec3<T> & v1, const vec3<T> & v2) {
        return vec3<T>(v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2]);
    }
    template <class T> inline vec3<T> operator * (const vec3<T> & v1, const vec3<T> & v2) {
        return vec3<T>(v1[0] * v2[0], v1[1] * v2[1], v1[2] * v2[2]);
    }
    template <class T> inline vec3<T> operator / (const vec3<T> & v1, const vec3<T> & v2) {
        return vec3<T>(v1[0] / v2[0], v1[1] / v2[1], v1[2] / v2[2]);
    }
    template <class T> inline vec3<T> operator * (const vec3<T> & v, T s) {
        return vec3<T>(v[0] * s, v[1] * s, v[2] * s);
    }
    template <class T> inline vec3<T> operator * (T s, const vec3<T> & v) {
        return vec3<T>(v[0] * s, v[1] * s, v[2] * s);
    }
    template <class T> inline vec3<T> operator / (const vec3<T> & v, T s) {
        return vec3<T>(v[0] / s, v[1] / s, v[2] / s);
    }
    template <class T> inline T dot(const vec3<T> & v1, const vec3<T> & v2) {
        return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
    }
    template <class T> inline bool isnan(const vec3<T> & v) {
        return std::isnan(static_cast<double>(v[0])) || std::isnan(static_cast<double>(v[1])) || std::isnan(static_cast<double>(v[2]));
    }
    template <class T> inline bool isinf(const vec3<T> & v) {
        return std::isinf(static_cast<double>(v[0])) || std::isinf(static_cast<double>(v[1])) || std::isinf(static_cast<double>(v[2]));
    }
    template <class T> inline bool iszero(const vec3<T> & v) {
        return v[0] == 0 && v[1] == 0 && v[2] == 0;
    }
    template <class T> inline bool isvalid(const vec3<T> & v) {
        return !isnan(v) && !isinf(v);
    }
    template <class T> inline vec3<T> cross(const vec3<T> & v1, const vec3<T> & v2) {
        return vec3<T> {
            v1[1] * v2[2] - v1[2] * v2[1],
            v1[2] * v2[0] - v1[0] * v2[2],
            v1[0] * v2[1] - v1[1] * v2[0]
        };
    }
    
    
    typedef vec3<int8_t> byte3;
    typedef vec3<int16_t> short3;
    typedef vec3<int32_t> int3;
    typedef vec3<uint8_t> ubyte3;
    typedef vec3<uint16_t> ushort3;
    typedef vec3<uint32_t> uint3;
    typedef vec3<float> float3;
    typedef vec3<double> double3;
    
    
    
    template <class T>
    class vec4 {
    public:
        vec4<T>() {}
        vec4<T>(T x, T y, T z, T w) :_v{ x, y, z, w } {}
        vec4<T>(const vec4<T> & other) :_v{ other[0], other[1], other[2], other[3] } {}
        vec4<T>(T * v) :_v{ v[0], v[1], v[2], v[3] } {}
        vec4<T>(const std::vector<T> & v) :_v{ v[0], v[1], v[2], v[3] } {}
        
        inline T * operator&() { return _v; }
        inline T & operator[](int i) { return _v[i]; }
        inline T operator[](int i) const { return _v[i]; }
        inline void operator=(const vec4<T> & other) { _v[0] = other[0]; _v[1] = other[1]; _v[2] = other[2]; _v[3] = other[3]; }
        inline bool operator==(const vec4<T> & other) { return _v[0] == other[0]; _v[1] == other[1]; _v[2] == other[2]; _v[3] == other[3]; }
        
        inline T length() const { return sqrt(_v[0]*_v[0] + _v[1]*_v[1] + _v[2]*_v[2] + _v[3]*_v[3]); }
        
        inline vec4<T> & normalize() {
            T l = length();
            _v[0] = _v[0] / l;
            _v[1] = _v[1] / l;
            _v[2] = _v[2] / l;
            _v[3] = _v[3] / l;
            return *this;
        }
        
        inline std::string toString() const {
            return "[" + std::to_string(_v[0]) + "," + std::to_string(_v[1]) + "," + std::to_string(_v[2]) + "," + std::to_string(_v[3]) + "]";
        }
        
        inline T & x() { return _v[0]; }
        inline T x() const { return _v[0]; }
        inline T & y() { return _v[1]; }
        inline T y() const { return _v[1]; }
        inline T & z() { return _v[2]; }
        inline T z() const { return _v[2]; }
        inline T & w() { return _v[3]; }
        inline T w() const { return _v[3]; }
        
        inline vec3<T> xyz() const { return vec3<T>(_v[0], _v[1], _v[2]); }
        
        inline T & r() { return _v[0]; }
        inline T r() const { return _v[0]; }
        inline T & g() { return _v[1]; }
        inline T g() const { return _v[1]; }
        inline T & b() { return _v[2]; }
        inline T b() const { return _v[2]; }
        inline T & a() { return _v[3]; }
        inline T a() const { return _v[3]; }
        
    protected:
        T _v[4] = { static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0) };
    };
    
    template <class T> inline T distance(const vec4<T> & v1, const vec4<T> & v2) {
        return vec4<T>(v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2], v1[3] - v2[3]).length();
    }
    template <class T> inline T length(const vec4<T> & v) { return v.length(); }
    template <class T> inline vec4<T> normalize(const vec4<T> & v) {
        vec4<T> result = v;
        result.normalize();
        return result;
    }
    template <class T> inline vec4<T> operator + (const vec4<T> & v1, const vec4<T> & v2) {
        return vec4<T>(v1[0] + v2[0], v1[1] + v2[1], v1[2] + v2[2], v1[3] + v2[3]);
    }
    template <class T> inline vec4<T> operator - (const vec4<T> & v1, const vec4<T> & v2) {
        return vec4<T>(v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2], v1[3] - v2[3]);
    }
    template <class T> inline vec4<T> operator * (const vec4<T> & v1, const vec4<T> & v2) {
        return vec4<T>(v1[0] * v2[0], v1[1] * v2[1], v1[2] * v2[2], v1[3] * v2[3]);
    }
    template <class T> inline vec4<T> operator / (const vec4<T> & v1, const vec4<T> & v2) {
        return vec4<T>(v1[0] / v2[0], v1[1] / v2[1], v1[2] / v2[2], v1[3] / v2[3]);
    }
    template <class T> inline vec4<T> operator * (const vec4<T> & v, T s) {
        return vec4<T>(v[0] * s, v[1] * s, v[2] * s, v[3] * s);
    }
    template <class T> inline vec4<T> operator * (T s, const vec4<T> & v) {
        return vec4<T>(v[0] * s, v[1] * s, v[2] * s, v[3] * s);
    }
    template <class T> inline vec4<T> operator / (const vec4<T> & v, T s) {
        return vec4<T>(v[0] / s, v[1] / s, v[2] / s, v[3] / s);
    }
    template <class T> inline T dot(const vec4<T> & v1, const vec4<T> & v2) {
        return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2] + v1[3] * v2[3];
    }
    template <class T> inline bool isnan(const vec4<T> & v) {
        return std::isnan(static_cast<double>(v[0])) || std::isnan(static_cast<double>(v[1])) || std::isnan(static_cast<double>(v[2])) || std::isnan(static_cast<double>(v[3]));
    }
    template <class T> inline bool isinf(const vec4<T> & v) {
        return std::isinf(static_cast<double>(v[0])) || std::isinf(static_cast<double>(v[1])) || std::isinf(static_cast<double>(v[2])) || std::isinf(static_cast<double>(v[3]));
    }
    template <class T> inline bool iszero(const vec4<T> & v) {
        return v[0] == 0 && v[1] == 0 && v[2] == 0 && v[3] == 0;
    }
    template <class T> inline bool isvalid(const vec4<T> & v) {
        return !isnan(v) && !isinf(v);
    }
    
    typedef vec4<int8_t> byte4;
    typedef vec4<int16_t> short4;
    typedef vec4<int32_t> int4;
    typedef vec4<uint8_t> ubyte4;
    typedef vec4<uint16_t> ushort4;
    typedef vec4<uint32_t> uint4;
    typedef vec4<float> float4;
    typedef vec4<double> double4;
	typedef float4 color;
    
    }
}
#endif