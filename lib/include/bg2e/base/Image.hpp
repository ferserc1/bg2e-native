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

#include <bg2e/common.hpp>
#include <bg2e/base/Color.hpp>

namespace bg2e {
namespace base {

class BG2E_API Image {
public:
    Image();
    Image(unsigned char* data, uint32_t width, uint32_t height, uint32_t channels);
    Image(float* data, uint32_t width, uint32_t height, uint32_t channels);
    ~Image();
    
    // SDR images
    void setData(unsigned char* data, uint32_t width, uint32_t height, uint32_t channels);
    
    // HDR images
    void setData(float* data, uint32_t width, uint32_t height, uint32_t channels);
    
    void cleanup();
    
    inline unsigned char* data() { return _data; }
    inline const unsigned char* data() const { return _data; }
    inline float* dataf() { return _dataf; }
    inline const float* dataf() const { return _dataf; }
    inline uint32_t width() const { return _width; }
    inline uint32_t height() const { return _height; }
    inline uint32_t channels() const { return _channels; }
    
    // TODO: Support for HDR images in floating point format
    
    inline bool isValid() const { return _data != nullptr && _width > 0 && _height > 0 && _channels > 0; }
    
    inline void setPath(const std::string& path) { _path = path; }
    inline const std::string& path() const { return _path; }

    inline void setColorType(Color::Type t) { _colorType = t; }
    inline Color::Type colorType() const { return _colorType; }
    
protected:
    unsigned char* _data = nullptr;
    float * _dataf = nullptr;
    uint32_t _width = 0;
    uint32_t _height = 0;
    uint32_t _channels = 0;
    
    
    std::string _path;
    
    Color::Type _colorType = Color::TypeSRGB;
};

}
}
