#pragma once

#include <bg2e/base/Image.hpp>
#include <bg2e/base/Color.hpp>

#include <memory>
#include <filesystem>
#include <string>
#include <functional>

namespace bg2e {
namespace base {

class ProceduralTextureGenerator {
public:
    
    ProceduralTextureGenerator() :_width{0}, _height{0}, _bpp{0} {}
    ProceduralTextureGenerator(uint32_t w, uint32_t h, uint32_t bpp) :_width{w}, _height{h}, _bpp{bpp} {}
    virtual ~ProceduralTextureGenerator() = default;
    
    // Generate the buffer based on _width, _height and _bpp parameters.
    virtual uint8_t* generate() = 0;
    
    // Return a texture image identifier, used to compare it with the TextureCache mechanism
    // and prevent duplication identical textures.
    virtual std::string imageIdentifier() = 0;
    
    virtual base::Color::Type colorType() { return base::Color::TypeLinear; }
    
    inline void setDimensions(uint32_t w, uint32_t h, uint32_t bpp)
    {
        _width = w;
        _height = h;
        _bpp = bpp;
    }

    inline void setWidth(uint32_t w) { _width = w; }
    inline void setHeight(uint32_t h) { _height = h; }
    inline void setBytesPerPixel(uint32_t bpp) { _bpp = bpp; }
    
    inline uint32_t width() const { return _width; }
    inline uint32_t height() const { return _height; }
    inline uint32_t bytesPerPixel() const { return _bpp; }

protected:
    uint32_t _width;
    uint32_t _height;
    uint32_t _bpp;
};

class Texture {
public:
    
    enum TextureType
    {
        TypeFilesystem = 1,
        TypeProcedural = 2
    };
    
    enum Filter {
        FilterLinear = 0,
        FilterNearest = 1
    };
    
    enum AddressMode {
        AddressModeRepeat = 0,
        AddressModeMirroredRepeat = 1,
        AddressModeClampToEdge = 2,
        AddressModeClampToBorder = 3
    };
    
    Texture() {}

    Texture(const std::filesystem::path& path)
        :_imageFilePath{ path.string() }
        ,_type{ TypeFilesystem }
    {}
    
    Texture(const std::string& path)
        :_imageFilePath{ path }
        ,_type{ TypeFilesystem }
    {}
    
    Texture(const std::filesystem::path& filePath, const std::string & fileName)
        :_type{ TypeFilesystem }
    {
        auto fullPath = filePath;
        fullPath.append(fileName);
        _imageFilePath = fullPath.string();
    }
    
    Texture(ProceduralTextureGenerator * gen)
        :_type{ TypeProcedural }
    { _proceduralGenerator = std::shared_ptr<ProceduralTextureGenerator>(gen); }
    
    Texture(std::shared_ptr<ProceduralTextureGenerator> gen)
        :_type{ TypeProcedural }
    { _proceduralGenerator = gen; }
    

    inline TextureType type() const { return _type; }
    
    inline void setProceduralGenerator(ProceduralTextureGenerator * gen) { setProceduralGenerator(std::shared_ptr<ProceduralTextureGenerator>(gen)); }
    inline void setProceduralGenerator(std::shared_ptr<ProceduralTextureGenerator> gen) { _type = TypeProcedural; _proceduralGenerator = gen; }
    ProceduralTextureGenerator * proceduralTextureGenerator() { return _proceduralGenerator.get(); }
    
    base::Image* generateImage()
    {
        if (!_proceduralGenerator.get())
        {
            throw std::runtime_error("base::Texture::generateImage(): Invalid procedural texture generator");
        }
        auto result = new Image();
        auto width = _proceduralGenerator->width();
        auto height = _proceduralGenerator->height();
        auto channels = _proceduralGenerator->bytesPerPixel();
        auto data = _proceduralGenerator->generate();
        auto identifier = std::string("procedural-texture:") + _proceduralGenerator->imageIdentifier();
        result->setData(data, width, height, channels);
        result->setPath(identifier);
        result->setColorType(_proceduralGenerator->colorType());
        _colorType = _proceduralGenerator->colorType();
        return result;
    }
    
    inline void setUseMipmaps(bool mipmaps) { _useMipmaps = mipmaps; }
    inline bool useMipmaps() const { return _useMipmaps; }
    inline void setMagFilter(Filter filter) { _magFilter = filter; }
    inline Filter magFilter() const { return _magFilter; }
    inline void setMinFilter(Filter filter) { _minFilter = filter; }
    inline Filter minFilter() const { return _minFilter; }
    inline void setMipLodBias(float b) { _mipLodBias = b; }
    inline float mipLodBias() const { return _mipLodBias; }
    inline void setMinLod(float l) { _minLod = l; }
    inline float minLod() const { return _minLod; }
    inline void setMaxLod(float l) { _maxLod = l; }
    inline float maxLod() const { return _maxLod; }
    inline void setAddressMode(AddressMode uvw) { _addressModeU = uvw; _addressModeV = uvw; }
    inline void setAddressMode(AddressMode u, AddressMode v) { _addressModeU = u; _addressModeV = v; }
    inline void setAddressMode(AddressMode u, AddressMode v, AddressMode w) { _addressModeU = u; _addressModeV = v; _addressModeW = w; }
    inline void setAddressModeU(AddressMode u) { _addressModeU = u; }
    inline void setAddressModeV(AddressMode v) { _addressModeV = v; }
    inline void setAddressModeW(AddressMode w) { _addressModeW = w; }
    inline AddressMode addressModeU() const { return _addressModeU; }
    inline AddressMode addressModeV() const { return _addressModeV; }
    inline AddressMode addressModeW() const { return _addressModeW; }
    inline void setColorType(base::Color::Type t) { _colorType = t; }
    inline base::Color::Type colorType() const { return _colorType; }
    
    inline void setImageFilePath(const std::string& filePath) { _type = TypeFilesystem; _imageFilePath = filePath; }
    inline const std::string& imageFilePath() const { return _imageFilePath; }

protected:
    std::string _imageFilePath;

    TextureType _type = TypeFilesystem;
    bool _useMipmaps = true;
    Filter _magFilter = FilterLinear;
    Filter _minFilter = FilterNearest;
    float _mipLodBias = 0.0f;
    float _minLod = 0.0f;
    float _maxLod = -1.0f; // -1=maximum number of image mipmaps
    AddressMode _addressModeU = AddressModeRepeat;
    AddressMode _addressModeV = AddressModeRepeat;
    AddressMode _addressModeW = AddressModeRepeat;
    base::Color::Type _colorType = base::Color::TypeSRGB;
    
    std::shared_ptr<ProceduralTextureGenerator> _proceduralGenerator;
};

}
}
