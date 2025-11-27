//
//  ProceduralSkyDomeGenerator.hpp

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
