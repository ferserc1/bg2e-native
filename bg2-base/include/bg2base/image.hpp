
#ifndef _bg2_base_image_hpp_
#define _bg2_base_image_hpp_

#include <bg2base/path.hpp>
#include <bg2math/vector.hpp>

#include <cstring>

namespace bg2base {
    
    class image {
    public:
        enum ImageFormat {
            kFormatNone = 0,
            kFormatGrayScale = 1,
            kFormatRGB = 3,
            kFormatRGBA = 4
        };
        
        image();
        image(const image & clone);
        image(const uint8_t * buffer, const bg2math::uint2 & size, uint8_t bytesPerPixel, ImageFormat format);
        virtual ~image();
        
        inline const path & filePath() const { return _filePath; }
        inline const uint8_t * data() const { return _data; }
        inline const bg2math::uint2 & size() const { return _size; }
        inline uint8_t bytesPerPixel() const { return _bytesPerPixel; }
        inline ImageFormat format() const { return _format; }
        
        inline void setFilePath(const path & p) { _filePath = p; }
        
        void setData(const uint8_t * data, const bg2math::uint2 & size, uint8_t bytesPerPixel, ImageFormat format);
        void destroy();
        
        inline bool valid() {
            return _data != nullptr &&
                bg2math::isvalid(_size) && !bg2math::iszero(_size) &&
                _bytesPerPixel>0 && _bytesPerPixel == static_cast<int>(_format);
        }
        
    protected:
        path _filePath;
        uint8_t * _data = nullptr;
        bg2math::uint2 _size;
        uint8_t _bytesPerPixel = 0;
        ImageFormat _format = kFormatNone;
    };
    
}

#endif
