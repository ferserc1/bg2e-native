//
//  SkyDomeTextureGenerator.cpp
#include <bg2e/scene/SkyDomeTextureGenerator.hpp>

namespace bg2e::scene {

SkyDomeTextureGenerator::SkyDomeTextureGenerator()
{

}

SkyDomeTextureGenerator::SkyDomeTextureGenerator(uint32_t w, uint32_t h, uint32_t bpp) :ProceduralTextureGenerator(w, h, bpp)
{

}

uint8_t* SkyDomeTextureGenerator::generate()
{
    throw std::runtime_error("SkyDomeTextureGenerator::generate(): Not implemented");
}

std::string SkyDomeTextureGenerator::imageIdentifier()
{
    return std::string("sky_dome_") + std::to_string(_width) + "x" + std::to_string(_height) + "@" + std::to_string(_bpp);
}

}
