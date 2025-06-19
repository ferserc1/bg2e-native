//
//  BRDFIntegrationMapTextureGenerator.hpp
#pragma once

#include <bg2e/base/Texture.hpp>
#include <bg2e/base/Color.hpp>


namespace bg2e {
namespace render {


class BG2E_API BRDFIntegrationMapTextureGenerator : public base::ProceduralTextureGenerator {
public:
    BRDFIntegrationMapTextureGenerator() = default;
    
    uint8_t* generate() override;
    
    std::string imageIdentifier() override;
    
    base::Color::Type colorType() override { return base::Color::TypeLinear; }
};

}
}
