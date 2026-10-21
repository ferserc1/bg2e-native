/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <bg2e/base/Image.hpp>
#include <bg2e/base/Color.hpp>

#include <memory>
#include <cstdint>
#include <filesystem>
#include <string>
#include <functional>

namespace bg2e {
namespace base {

class ProceduralTextureGenerator {
  public:
    ProceduralTextureGenerator() : _width{0}, _height{0}, _bpp{0}
    {
    }
    ProceduralTextureGenerator(uint32_t w, uint32_t h, uint32_t bpp) : _width{w}, _height{h}, _bpp{bpp}
    {
    }
    virtual ~ProceduralTextureGenerator() = default;

    // Generate the buffer based on _width, _height and _bpp parameters.
    virtual uint8_t* generate() = 0;

    // Return a texture image identifier, used to compare it with the TextureCache mechanism
    // and prevent duplication identical textures.
    virtual std::string imageIdentifier() = 0;

    virtual base::Color::Type colorType()
    {
        return base::Color::TypeLinear;
    }

    inline void setDimensions(uint32_t w, uint32_t h, uint32_t bpp)
    {
        if (_width != w || _height != h || _bpp != bpp)
        {
            _width = w;
            _height = h;
            _bpp = bpp;
            markModified();
        }
    }

    inline void setWidth(uint32_t w)
    {
        setDimensions(w, _height, _bpp);
    }
    inline void setHeight(uint32_t h)
    {
        setDimensions(_width, h, _bpp);
    }
    inline void setBytesPerPixel(uint32_t bpp)
    {
        setDimensions(_width, _height, bpp);
    }

    inline uint32_t width() const
    {
        return _width;
    }
    inline uint32_t height() const
    {
        return _height;
    }
    inline uint32_t bytesPerPixel() const
    {
        return _bpp;
    }

    uint64_t revision() const
    {
        return _revision;
    }
    // Custom generators call this when their pixel-producing state changes.
    void markModified()
    {
        ++_revision;
    }

  protected:
    uint32_t _width;
    uint32_t _height;
    uint32_t _bpp;

  private:
    uint64_t _revision = 1;
};

class Texture {
  public:
    enum TextureType
    {
        TypeFilesystem = 1,
        TypeProcedural = 2
    };

    enum Filter
    {
        FilterLinear = 0,
        FilterNearest = 1
    };

    enum AddressMode
    {
        AddressModeRepeat = 0,
        AddressModeMirroredRepeat = 1,
        AddressModeClampToEdge = 2,
        AddressModeClampToBorder = 3
    };

    Texture()
    {
    }

    Texture(const std::filesystem::path& path)
        : _imageFilePath{path.string()}, _type{TypeFilesystem}
    {
    }

    Texture(const std::string& path)
        : _imageFilePath{path}, _type{TypeFilesystem}
    {
    }

    Texture(const std::filesystem::path& filePath, const std::string& fileName)
        : _type{TypeFilesystem}
    {
        auto fullPath = filePath;
        fullPath.append(fileName);
        assign(_imageFilePath, fullPath.string());
    }

    Texture(ProceduralTextureGenerator* gen)
        : _type{TypeProcedural}
    {
        assign(_proceduralGenerator, std::shared_ptr<ProceduralTextureGenerator>(gen));
    }

    Texture(std::shared_ptr<ProceduralTextureGenerator> gen)
        : _type{TypeProcedural}
    {
        assign(_proceduralGenerator, gen);
    }

    Texture(const Texture&) = default;
    Texture& operator=(const Texture& source)
    {
        if (this != &source)
        {
            assign(_imageFilePath, source._imageFilePath);
            assign(_type, source._type);
            assign(_useMipmaps, source._useMipmaps);
            assign(_magFilter, source._magFilter);
            assign(_minFilter, source._minFilter);
            assign(_mipLodBias, source._mipLodBias);
            assign(_minLod, source._minLod);
            assign(_maxLod, source._maxLod);
            assign(_addressModeU, source._addressModeU);
            assign(_addressModeV, source._addressModeV);
            assign(_addressModeW, source._addressModeW);
            assign(_colorType, source._colorType);
            assign(_proceduralGenerator, source._proceduralGenerator);
        }
        return *this;
    }

    uint64_t revision() const
    {
        const auto generatorRevision = _proceduralGenerator ? _proceduralGenerator->revision() : 0;
        if (_generatorRevision != generatorRevision)
        {
            _generatorRevision = generatorRevision;
            ++_revision;
        }
        return _revision;
    }

    inline TextureType type() const
    {
        return _type;
    }

    inline void setProceduralGenerator(ProceduralTextureGenerator* gen)
    {
        setProceduralGenerator(std::shared_ptr<ProceduralTextureGenerator>(gen));
    }
    inline void setProceduralGenerator(std::shared_ptr<ProceduralTextureGenerator> gen)
    {
        assign(_type, TypeProcedural);
        assign(_proceduralGenerator, gen);
    }
    ProceduralTextureGenerator* proceduralTextureGenerator()
    {
        return _proceduralGenerator.get();
    }

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
        assign(_colorType, _proceduralGenerator->colorType());
        return result;
    }

    inline void setUseMipmaps(bool mipmaps)
    {
        assign(_useMipmaps, mipmaps);
    }
    inline bool useMipmaps() const
    {
        return _useMipmaps;
    }
    inline void setMagFilter(Filter filter)
    {
        assign(_magFilter, filter);
    }
    inline Filter magFilter() const
    {
        return _magFilter;
    }
    inline void setMinFilter(Filter filter)
    {
        assign(_minFilter, filter);
    }
    inline Filter minFilter() const
    {
        return _minFilter;
    }
    inline void setMipLodBias(float b)
    {
        assign(_mipLodBias, b);
    }
    inline float mipLodBias() const
    {
        return _mipLodBias;
    }
    inline void setMinLod(float l)
    {
        assign(_minLod, l);
    }
    inline float minLod() const
    {
        return _minLod;
    }
    inline void setMaxLod(float l)
    {
        assign(_maxLod, l);
    }
    inline float maxLod() const
    {
        return _maxLod;
    }
    inline void setAddressMode(AddressMode uvw)
    {
        assign(_addressModeU, uvw);
        assign(_addressModeV, uvw);
    }
    inline void setAddressMode(AddressMode u, AddressMode v)
    {
        assign(_addressModeU, u);
        assign(_addressModeV, v);
    }
    inline void setAddressMode(AddressMode u, AddressMode v, AddressMode w)
    {
        assign(_addressModeU, u);
        assign(_addressModeV, v);
        assign(_addressModeW, w);
    }
    inline void setAddressModeU(AddressMode u)
    {
        assign(_addressModeU, u);
    }
    inline void setAddressModeV(AddressMode v)
    {
        assign(_addressModeV, v);
    }
    inline void setAddressModeW(AddressMode w)
    {
        assign(_addressModeW, w);
    }
    inline AddressMode addressModeU() const
    {
        return _addressModeU;
    }
    inline AddressMode addressModeV() const
    {
        return _addressModeV;
    }
    inline AddressMode addressModeW() const
    {
        return _addressModeW;
    }
    inline void setColorType(base::Color::Type t)
    {
        assign(_colorType, t);
    }
    inline base::Color::Type colorType() const
    {
        return _colorType;
    }

    inline void setImageFilePath(const std::string& filePath)
    {
        assign(_type, TypeFilesystem);
        assign(_imageFilePath, filePath);
    }
    inline const std::string& imageFilePath() const
    {
        return _imageFilePath;
    }

  private:
    template<class T>
    void assign(T& target, const T& value)
    {
        if (!(target == value))
        {
            target = value;
            ++_revision;
        }
    }

    mutable uint64_t _revision = 1;
    mutable uint64_t _generatorRevision = 0;

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
