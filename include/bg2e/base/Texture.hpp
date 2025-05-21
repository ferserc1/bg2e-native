#pragma once

#include <bg2e/base/Image.hpp>

#include <memory>
#include <filesystem>
#include <string>

namespace bg2e {
namespace base {

class Texture {
public:
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

    Texture(const std::filesystem::path& path) :_imageFilePath(path.string()) {}
    Texture(const std::string& path) :_imageFilePath(path) {}
    Texture(const std::filesystem::path& filePath, const std::string & fileName) {
        auto fullPath = filePath;
        fullPath.append(fileName);
        _imageFilePath = fullPath.string();
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
    
    inline void setImageFilePath(const std::string& filePath) { _imageFilePath = filePath; }
    inline const std::string& imageFilePath() const { return _imageFilePath; }

protected:
    std::string _imageFilePath;

    bool _useMipmaps = true;
    Filter _magFilter = FilterLinear;
    Filter _minFilter = FilterNearest;
    float _mipLodBias = 0.0f;
    float _minLod = 0.0f;
    float _maxLod = -1.0f; // -1=maximum number of image mipmaps
    AddressMode _addressModeU = AddressModeRepeat;
    AddressMode _addressModeV = AddressModeRepeat;
    AddressMode _addressModeW = AddressModeRepeat;
};

}
}
