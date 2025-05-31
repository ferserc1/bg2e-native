//
//  ProceduralSkyDomeGenerator.hpp

#pragma once

#include <bg2e/base/Texture.hpp>

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
};

}
}
