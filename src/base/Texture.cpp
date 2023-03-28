
#include <bg2e/base/Texture.hpp>

#include <bg2e/base/Image.hpp>

#include <stdexcept>

namespace bg2e {
namespace base {

Texture::Texture()
{
    
}

void Texture::loadImageData(bool refresh)
{
    if (_fileName != "")
    {
        _image = Image::LoadFromFile(_fileName);
        
    }
    else if (_dataType == TextureDataType::RenderTarget)
    {
        // TODO: implement this
    }
    else if (_proceduralTextureFunction.get() != nullptr)
    {
        _image = _proceduralTextureFunction->getImage();
        _size = _image->size();
        _channels = _image->channels();
    }
    else
    {
        throw std::runtime_error("Error");
    }
}

}
}
