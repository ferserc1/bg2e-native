
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/base/Image.hpp>

#include <filesystem>

namespace bg2e {
namespace db {

extern BG2E_API bg2e::base::Image * loadImage(const std::filesystem::path& filePath);

}
}
