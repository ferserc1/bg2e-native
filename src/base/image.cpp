
#include <bg2e/base/image.hpp>
#include <exception>
#include <stdexcept>

namespace bg2e {
namespace base {
    
    Image::Image()
    {
        
    }
    
    Image::Image(const uint8_t * buffer, const math::uint2 & size, uint8_t bytesPerPixel, ImageFormat format)
    {
        setData(buffer, size, bytesPerPixel, format);
    }
    
    Image::Image(const Image & clone)
    {
        setData(clone.data(), clone.size(), clone.bytesPerPixel(), clone.format());
        setFilePath(clone.filePath());
    }
    
    Image::~Image() {
        destroy();
    }
    
    void Image::setData(const uint8_t * data, const math::uint2 & size, uint8_t bytesPerPixel, ImageFormat format) {
        if (data != nullptr &&
            math::isvalid(size) && !math::iszero(size) &&
            bytesPerPixel > 0 && bytesPerPixel == static_cast<int>(format))
        {
            destroy();
            size_t dataSize = size.x() * size.y() * bytesPerPixel;
            _data = new uint8_t[dataSize];
            std::memcpy(_data, data, dataSize);
            _size = size;
            _bytesPerPixel = bytesPerPixel;
            _format = format;
        }
        else {
            throw std::invalid_argument("Image::setData(): invalid argument.");
        }
    }
    
    void Image::destroy() {
        if (_data != nullptr) {
            delete [] _data;
        }
        _size = { 0, 0 };
        _bytesPerPixel = 0;
        _format = kFormatNone;
    }

}
}
