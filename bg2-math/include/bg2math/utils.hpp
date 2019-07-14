#ifndef _bg2_math_utils_hpp_
#define _bg2_math_utils_hpp_

#ifdef _WIN32
    #ifdef max
        #undef max
    #endif
    #ifdef min
        #undef min
    #endif
#endif

#include <bg2math/vector.hpp>
#include <bg2base/export.hpp>

namespace bg2math {

    class EXPORT kf {
    public:
        static float pi;
        static float piOver2;
        static float piOver4;
        static float piOver180;
    };

    class EXPORT kd {
    public:
        static double pi;
        static double piOver2;
        static double piOver4;
        static double piOver180;
    };
    
    
    template <class T> inline T clamp(T v, T lo, T hi) {
        return v<lo ? lo : (v>hi ? hi : v);
    }
    template <class T> inline vec2<T> clamp(const vec2<T> & v, T lo, T hi) {
        return vec2<T> {
            clamp(v[0], lo, hi),
            clamp(v[1], lo, hi)
        };
    }
    template <class T> inline vec3<T> clamp(const vec3<T> & v, T lo, T hi) {
        return vec3<T> {
            clamp(v[0], lo, hi),
            clamp(v[1], lo, hi),
            clamp(v[2], lo, hi)
        };
    }
    template <class T> inline vec4<T> clamp(const vec4<T> & v, T lo, T hi) {
        return vec4<T> {
            clamp(v[0], lo, hi),
            clamp(v[1], lo, hi),
            clamp(v[2], lo, hi),
            clamp(v[3], lo, hi)
        };
    }
    template <class T> inline T min(T a, T b) { return a<b ? a : b; }
    template <class T> inline T max(T a, T b) { return a>b ? a : b; }
    template <class T> inline vec2<T> min(const vec2<T> & v1, const vec2<T> & v2) {
        return vec2<T> {
            min(v1[0],v2[0]),
            min(v1[1],v2[1])
        };
    }
    template <class T> inline vec2<T> max(const vec2<T> & v1, const vec2<T> & v2) {
        return vec2<T> {
            max(v1[0],v2[0]),
            max(v1[1],v2[1])
        };
    }
    template <class T> inline vec3<T> min(const vec3<T> & v1, const vec3<T> & v2) {
        return vec3<T> {
            min(v1[0],v2[0]),
            min(v1[1],v2[1]),
            min(v1[2],v2[2])
        };
    }
    template <class T> inline vec3<T> max(const vec3<T> & v1, const vec3<T> & v2) {
        return vec3<T> {
            max(v1[0],v2[0]),
            max(v1[1],v2[1]),
            max(v1[2],v2[2])
        };
    }
    template <class T> inline vec4<T> min(const vec4<T> & v1, const vec4<T> & v2) {
        return vec4<T> {
            min(v1[0],v2[0]),
            min(v1[1],v2[1]),
            min(v1[2],v2[2]),
            min(v1[3],v2[3])
        };
    }
    template <class T> inline vec4<T> max(const vec4<T> & v1, const vec4<T> & v2) {
        return vec4<T> {
            max(v1[0],v2[0]),
            max(v1[1],v2[1]),
            max(v1[2],v2[2]),
            max(v1[3],v2[3])
        };
    }
}

#endif
