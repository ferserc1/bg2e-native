
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/base/Image.hpp>
#include <bg2e/base/Texture.hpp>

#include <filesystem>

namespace bg2e {
namespace db {

extern BG2E_API bg2e::base::Image * loadImage(const std::filesystem::path& filePath);

extern BG2E_API bg2e::base::Image * loadImage(const std::filesystem::path& basePath, const std::string& fileName);

extern BG2E_API void saveImage(
    const std::filesystem::path& filePath,
    const uint8_t* data,
    uint32_t width,
    uint32_t height,
    uint32_t bpp
);

extern BG2E_API void saveImage(
    const std::filesystem::path& basePath,
    const std::string& fileName,
    const uint8_t* data,
    uint32_t width,
    uint32_t height,
    uint32_t bpp
);

}
}
