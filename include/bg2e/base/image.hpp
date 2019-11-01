
#ifndef _bg2e_base_image_hpp_
#define _bg2e_base_image_hpp_

#include <bg2e/base/path.hpp>
#include <bg2e/math/vector.hpp>
#include <bg2e/base/referenced_pointer.hpp>

#include <bgfx/bgfx.h>

#include <cstring>

namespace bg2e {
namespace base {
    
    class Image : public bg2e::base::ReferencedPointer {
    public:
        enum ImageFormat {
            kFormatNone = 0,
            kFormatGrayScale = 1,
            kFormatRGB = 3,
            kFormatRGBA = 4
        };
        
        Image();
        Image(const Image & clone);
        Image(const uint8_t * buffer, const math::uint2 & size, uint8_t bytesPerPixel, ImageFormat format);
        
        inline const path & filePath() const { return _filePath; }
        inline const uint8_t * data() const { return _data; }
        inline const math::uint2 & size() const { return _size; }
        inline uint8_t bytesPerPixel() const { return _bytesPerPixel; }
        inline ImageFormat format() const { return _format; }
		inline uint32_t dataSize() const { return _size.width() * _size.height() * _bytesPerPixel; }
        
        inline void setFilePath(const path & p) { _filePath = p; }
        
        void setData(const uint8_t * data, const math::uint2 & size, uint8_t bytesPerPixel, ImageFormat format);
        void destroy();
        
        inline bool valid() const {
            return _data != nullptr &&
                math::isvalid(_size) && !math::iszero(_size) &&
                _bytesPerPixel>0 && _bytesPerPixel == static_cast<int>(_format);
        }

		const bgfx::Memory* memoryRef() const;
        
    protected:
		virtual ~Image();

        path _filePath;
        uint8_t * _data = nullptr;
        math::uint2 _size;
        uint8_t _bytesPerPixel = 0;
        ImageFormat _format = kFormatNone;
    };

}
}

#endif
