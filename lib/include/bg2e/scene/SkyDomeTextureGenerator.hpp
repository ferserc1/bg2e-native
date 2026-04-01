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

#include <bg2e/base/Texture.hpp>
#include <bg2e/base/Color.hpp>

namespace bg2e {
namespace scene {

class BG2E_API SkyDomeTextureGenerator : public base::ProceduralTextureGenerator {
public:
    SkyDomeTextureGenerator();
    SkyDomeTextureGenerator(uint32_t w, uint32_t h, uint32_t bpp);
    
    // Generate the buffer based on _width, _height and _bpp parameters.
    uint8_t* generate() override;
    
    // Return a texture image identifier, used to compare it with the TextureCache mechanism
    // and prevent duplication identical textures.
    std::string imageIdentifier() override;
    
    base::Color::Type colorType() override { return base::Color::TypeLinear; }
    
    base::Color ceilColor { 0.2f, 0.2f, 0.55f, 1.0f };
    base::Color ceilHorizonColor { 0.34f, 0.34f, 0.95f, 1.0f };
    base::Color horizonColor { 0.01f, 0.01f, 0.01f, 1.0f };
    base::Color groundColor { 0.0f, 0.0f, 0.0f, 1.0f };
};

}
}
