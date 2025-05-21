
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/base/Image.hpp>
#include <bg2e/base/Texture.hpp>

#include <filesystem>

namespace bg2e {
namespace db {

extern BG2E_API bg2e::base::Image * loadImage(const std::filesystem::path& filePath);

extern BG2E_API bg2e::base::Image * loadImage(const std::filesystem::path& basePath, const std::string& fileName);

}
}
