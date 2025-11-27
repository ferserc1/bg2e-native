//
//  BRDFIntegrationMapTextureGenerator.hpp
#pragma once

#include <bg2e/render/GPUTextureGenerator.hpp>

namespace bg2e {
namespace render {


class BG2E_API BRDFIntegrationMapTextureGenerator : public GPUTextureGenerator {
public:
    BRDFIntegrationMapTextureGenerator(Engine * engine, uint32_t w, uint32_t h) :GPUTextureGenerator(engine, w, h) {}
    virtual ~BRDFIntegrationMapTextureGenerator() = default;
    
    Texture * generate() override;
};

}
}
