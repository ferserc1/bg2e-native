//
// Created by fernando on 3/12/25.
//

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/render/Engine.hpp>
#include <filesystem>
#include <string>

namespace bg2e {
namespace db {

extern BG2E_API bg2e::scene::Node * loadGltf(
    const std::filesystem::path& filePath,
    render::Engine* engine
);

extern BG2E_API bg2e::scene::Node * loadGltf(
    const std::filesystem::path& basePath,
    const std::string& fileName,
    render::Engine* engine
);

}
}