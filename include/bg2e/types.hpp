#ifndef bg2e_base_types_hpp
#define bg2e_base_types_hpp

#include <cstdint>

namespace bg2e {

template <typename T>
bool isPOT(T n)
{
    return n != static_cast<T>(0) ?
        (n & (n - 1)) == 0 :
        false;
}

template <typename T>
struct SizeBase
{
    T width = static_cast<T>(0);
    T height = static_cast<T>(0);
    
    inline void set(uint16_t width, uint16_t height)
    {
        width = static_cast<T>(width);
        height = static_cast<T>(height);
    }
    
    inline void set(int16_t width, int16_t height)
    {
        width = static_cast<T>(width);
        height = static_cast<T>(height);
    }
    
    inline void set(uint32_t width, uint32_t height)
    {
        width = static_cast<T>(width);
        height = static_cast<T>(height);
    }
    
    inline void set(int32_t width, int32_t height)
    {
        width = static_cast<T>(width);
        height = static_cast<T>(height);
    }
    
    inline void set(float width, float height)
    {
        width = static_cast<T>(width);
        height = static_cast<T>(height);
    }
    
    inline void set(double width, double height)
    {
        width = static_cast<T>(width);
        height = static_cast<T>(height);
    }
    
    inline SizeBase<T> operator=(const SizeBase<uint16_t>& other)
    {
        width = static_cast<T>(other.width);
        height = static_cast<T>(other.height);
        return *this;
    }
    
    inline SizeBase<T> operator=(const SizeBase<int16_t>& other)
    {
        width = static_cast<T>(other.width);
        height = static_cast<T>(other.height);
        return *this;
    }
    
    inline SizeBase operator=(const SizeBase<uint32_t>& other)
    {
        width = static_cast<T>(other.width);
        height = static_cast<T>(other.height);
        return *this;
    }
    
    inline SizeBase operator=(const SizeBase<int32_t>& other)
    {
        width = static_cast<T>(other.width);
        height = static_cast<T>(other.height);
        return *this;
    }
    
    inline SizeBase operator=(const SizeBase<float>& other)
    {
        width = static_cast<T>(other.width);
        height = static_cast<T>(other.height);
        return *this;
    }
    
    inline SizeBase operator=(const SizeBase<double>& other)
    {
        width = static_cast<T>(other.width);
        height = static_cast<T>(other.height);
        return *this;
    }
    
    inline bool isPowerOfTwo() const
    {
        return isPOT<T>(width) && isPOT<T>(height);
    }
};

typedef SizeBase<uint32_t> Size;
typedef SizeBase<float> Sizef;
typedef SizeBase<double> Sized;

typedef uint8_t Byte;

struct Viewport
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    
    inline float aspectRatio() const { return height != 0 ? static_cast<float>(width) / static_cast<float>(height) : 0.0f; }
};

}

#endif

