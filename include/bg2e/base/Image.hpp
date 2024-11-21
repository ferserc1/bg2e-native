
#pragma once

#include <bg2e/common.hpp>

namespace bg2e {
namespace base {

class BG2E_API Image {
public:
    Image();
    Image(unsigned char* data, uint32_t width, uint32_t height, uint32_t channels);
    ~Image();
    
    void setData(unsigned char* data, uint32_t width, uint32_t height, uint32_t channels);
    void cleanup();
    
    inline unsigned char* data() { return _data; }
    inline const unsigned char* data() const { return _data; }
    inline uint32_t width() const { return _width; }
    inline uint32_t height() const { return _height; }
    inline uint32_t channels() const { return _channels; }
    
    // TODO: Support for HDR images in floating point format
    
    inline bool isValid() const { return _data != nullptr && _width > 0 && _height > 0 && _channels > 0; }

protected:
    unsigned char* _data = nullptr;
    uint32_t _width = 0;
    uint32_t _height = 0;
    uint32_t _channels = 0;
};

}
}
