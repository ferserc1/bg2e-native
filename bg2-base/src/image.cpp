
#include <bg2base/image.hpp>
#include <exception>
#include <stdexcept>

namespace bg2base {
    
    image::image()
    {
        
    }
    
    image::image(const uint8_t * buffer, const bg2math::uint2 & size, uint8_t bytesPerPixel, ImageFormat format)
    {
        setData(buffer, size, bytesPerPixel, format);
    }
    
    image::image(const image & clone)
    {
        setData(clone.data(), clone.size(), clone.bytesPerPixel(), clone.format());
        setFilePath(clone.filePath());
    }
    
    image::~image() {
        destroy();
    }
    
    void image::setData(const uint8_t * data, const bg2math::uint2 & size, uint8_t bytesPerPixel, ImageFormat format) {
        if (data != nullptr &&
            bg2math::isvalid(size) && !bg2math::iszero(size) &&
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
    
    void image::destroy() {
        if (_data != nullptr) {
            delete [] _data;
        }
        _size = { 0, 0 };
        _bytesPerPixel = 0;
        _format = kFormatNone;
    }
}
