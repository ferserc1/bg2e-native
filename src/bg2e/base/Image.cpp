
#include <bg2e/base/Image.hpp>

namespace bg2e::base {

Image::Image()
{

}

Image::Image(unsigned char* data, uint32_t width, uint32_t height, uint32_t channels)
    :_data(data)
    ,_width(width)
    ,_height(height)
    ,_channels(channels)
{

}

Image::~Image()
{
    cleanup();
}
    
void Image::setData(unsigned char* data, uint32_t width, uint32_t height, uint32_t channels)
{
    cleanup();
    
    _data = data;
    _width = width;
    _height = height;
    _channels = channels;
}

void Image::cleanup()
{
    if (_data)
    {
        delete [] _data;
        _data = nullptr;
        _width = 0;
        _height = 0;
        _channels = 0;
    }
}

}
