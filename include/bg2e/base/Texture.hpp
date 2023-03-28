#ifndef bg2e_base_texture_hpp
#define bg2e_base_texture_hpp

#include <bg2e/export.hpp>
#include <bg2e/types.hpp>

#include <bg2e/base/Image.hpp>

#include <glm/vec4.hpp>

#include <string>
#include <memory>

namespace bg2e {
namespace base {

enum class TextureDataType
{
    None = 0,
    Image,
    ImageData,
    Cubemap,
    CubemapData,
    Procedural,
    RenderTarget
};

enum class TextureWrap
{
    Repeat = 0,
    Clap = 1,
    MirroredRepeat = 2
};

enum class TextureFilter
{
    NearestMipmapNearest = 0,
    LinearMipmapNearest,
    NearestMipmapLInear,
    LinearMipmapLinear,
    Nearest,
    Linear
};

enum class TextureTarget
{
    Texture2D = 0,
    CubeMap
};

enum class TextureRenderTargetAttachment
{
    ColorAttachment0 = 0,
    ColorAttachment1 = 1,
    ColorAttachment2 = 2,
    ColorAttachment3 = 3,
    ColorAttachment4 = 4,
    ColorAttachment5 = 5,
    ColorAttachment6 = 6,
    ColorAttachment7 = 7,
    ColorAttachment8 = 8,
    ColorAttachment9 = 9,
    ColorAttachment10 = 10,
    ColorAttachment11 = 11,
    ColorAttachment12 = 12,
    ColorAttachment13 = 13,
    ColorAttachment14 = 14,
    ColorAttachment15 = 15,
    DepthAttachment = 100,
    StencilAttachment = 200
};

enum class TextureComponentFormat
{
    UnsignedByte = 0,
    Float32
};

enum class TextureChannel {
    R = 1,
    G = 2,
    B = 3,
    A = 4
};

class ProceduralTextureFunction {
public:
    ProceduralTextureFunction() {}
    
    virtual std::shared_ptr<Image> getImage() = 0;
};


class BG2E_EXPORT Texture
{
public:
    Texture();
    
    std::shared_ptr<Texture> clone();
    Texture& operator=(const Texture&);
    
    bool dirty() const
    {
        return _dirty;
    }
    
    void setUpdated(bool u)
    {
        _dirty = !u;
    }
    
    inline void setDataType(TextureDataType t)
    {
        _dataType = t;
        if (!_size.isPowerOfTwo())
        {
            setWrapModeXY(TextureWrap::Clap);
        }
        setMagFilter(TextureFilter::Linear);
        setMinFilter(TextureFilter::Linear);
        _dirty = true;
    }
    
    inline TextureDataType dataType() const
    {
        return _dataType;
    }

    inline TextureWrap wrapModeX() const
    {
        return _wrapModeX;
    }
    
    inline void setWrapModeX(TextureWrap m)
    {
        _wrapModeX = m;
        _dirty = true;
    }
    
    inline void setWrapModeY(TextureWrap m)
    {
        _wrapModeY = m;
        _dirty = true;
    }
        
    inline TextureWrap wrapModeY() const
    {
        return _wrapModeY;
    }
    
    inline void setWrapModeXY(TextureWrap m)
    {
        _wrapModeX = m;
        _wrapModeY = m;
        _dirty = true;
    }
    
    inline void setMagFilter(TextureFilter f)
    {
        _magFilter = f;
        _dirty = true;
    }
    
    inline TextureFilter magFilter() const
    {
        return _magFilter;
    }
    
    inline void setMinFilter(TextureFilter f)
    {
        _minFilter = f;
        _dirty = true;
    }
    
    inline TextureFilter minFilter() const
    {
        return _minFilter;
    }
    
    inline void setTarget(TextureTarget t)
    {
        _target = t;
        _dirty = true;
    }
    
    inline TextureTarget target() const
    {
        return _target;
    }
    
    inline void setSize(const Size& s)
    {
        _size = s;
        _dirty = true;
    }
    
    inline void setSize(Size&& s)
    {
        _size = s;
        _dirty = true;
    }
    
    inline const Size& size() const
    {
        return _size;
    }
    
    inline void setFileName(const std::string& fileName)
    {
        _fileName = fileName;
        _dirty = true;
    }
    
    inline const std::string& fileName() const
    {
        return _fileName;
    }
    
    inline void setChannels(uint32_t c)
    {
        _channels = c;
        _dirty = true;
    }
    
    inline uint32_t channels() const
    {
        return _channels;
    }
    
    inline void setProceduralTextureFunction(std::shared_ptr<ProceduralTextureFunction> fn)
    {
        _proceduralTextureFunction = fn;
        _dirty = true;
    }
    
    inline void setRenderTargetAttachment(TextureRenderTargetAttachment att)
    {
        _renderTargetAttachment = att;
        _dirty = true;
    }
    
    inline TextureRenderTargetAttachment renderTargetAttachment() const
    {
        return _renderTargetAttachment;
    }
    
    inline void setComponentFormat(TextureComponentFormat fmt)
    {
        _componentFormat = fmt;
        _dirty = true;
    }
    
    inline TextureComponentFormat componentFormat() const
    {
        return _componentFormat;
    }
    
    
    inline std::shared_ptr<Image> image() const
    {
        return _image;
    }
    
    inline bool mipmapRequired() const
    {
        return  _minFilter == TextureFilter::NearestMipmapNearest ||
                _minFilter == TextureFilter::LinearMipmapNearest ||
                _minFilter == TextureFilter::LinearMipmapLinear ||
                _minFilter == TextureFilter::NearestMipmapLInear ||
                _magFilter == TextureFilter::NearestMipmapNearest ||
                _magFilter == TextureFilter::LinearMipmapNearest ||
                _magFilter == TextureFilter::LinearMipmapLinear ||
                _magFilter == TextureFilter::NearestMipmapLInear;
    }
    
    void loadImageData(bool refresh = false);
    
protected:
    bool _dirty = true;
    
    TextureDataType _dataType = TextureDataType::None;
    TextureWrap _wrapModeX = TextureWrap::Repeat;
    TextureWrap _wrapModeY = TextureWrap::Repeat;
    TextureFilter _magFilter = TextureFilter::Linear;
    TextureFilter _minFilter = TextureFilter::Linear;
    TextureTarget _target = TextureTarget::Texture2D;
    Size _size;
    uint32_t _channels;
    std::string _fileName;
    std::shared_ptr<ProceduralTextureFunction> _proceduralTextureFunction;
    std::shared_ptr<Image> _image;
    TextureRenderTargetAttachment _renderTargetAttachment = TextureRenderTargetAttachment::ColorAttachment0;
    TextureComponentFormat _componentFormat = TextureComponentFormat::UnsignedByte;
};


}
}
#endif
