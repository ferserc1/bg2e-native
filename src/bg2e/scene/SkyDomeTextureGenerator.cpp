//
//  SkyDomeTextureGenerator.cpp
#include <bg2e/scene/SkyDomeTextureGenerator.hpp>
#include <bg2e/base/Color.hpp>

#include <cmath>

namespace bg2e::scene {

SkyDomeTextureGenerator::SkyDomeTextureGenerator()
{

}

SkyDomeTextureGenerator::SkyDomeTextureGenerator(uint32_t w, uint32_t h, uint32_t bpp) :ProceduralTextureGenerator(w, h, bpp)
{

}

uint8_t* SkyDomeTextureGenerator::generate()
{
    auto size = _width * _height * _bpp;
    uint8_t * buffer = new uint8_t[size];

    auto halfHeight = _height / 2;
    for (auto line = 0; line < _height; ++line)
    {
        auto lineOffset = line * _width * _bpp;
        for (auto column = 0; column < _width; ++column)
        {
            auto colOffset = column * _bpp;
            if (line < halfHeight)
            {
                auto t = static_cast<float>(line) / static_cast<float>(halfHeight);
                auto r = static_cast<uint8_t>(std::lerp(ceilColor.r, ceilHorizonColor.r, t) * 255.0f);
                auto g = static_cast<uint8_t>(std::lerp(ceilColor.g, ceilHorizonColor.g, t) * 255.0f);
                auto b = static_cast<uint8_t>(std::lerp(ceilColor.b, ceilHorizonColor.b, t) * 255.0f);
                buffer[lineOffset + colOffset] = r;
                buffer[lineOffset + colOffset + 1] = g;
                buffer[lineOffset + colOffset + 2] = b;
                buffer[lineOffset + colOffset + 3] = 255;
            }
            else{
                auto t = static_cast<float>(line - halfHeight) / static_cast<float>(halfHeight);
                auto r = static_cast<uint8_t>(std::lerp(horizonColor.r, groundColor.r, t) * 255.0f);
                auto g = static_cast<uint8_t>(std::lerp(horizonColor.g, groundColor.g, t) * 255.0f);
                auto b = static_cast<uint8_t>(std::lerp(horizonColor.b, groundColor.b, t) * 255.0f);
                buffer[lineOffset + colOffset] = r;
                buffer[lineOffset + colOffset + 1] = g;
                buffer[lineOffset + colOffset + 2] = b;
                buffer[lineOffset + colOffset + 3] = 255;
            }
        }
    }
    return buffer;
}

std::string SkyDomeTextureGenerator::imageIdentifier()
{
    return std::string("sky_dome_") +
        std::to_string(_width) + "x" + std::to_string(_height) + "@" + std::to_string(_bpp) +
        ceilColor.toString() + "-" + ceilHorizonColor.toString() + "-" + horizonColor.toString() + "-" + groundColor.toString();
}

}
