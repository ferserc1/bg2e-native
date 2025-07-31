//
//  scene.hpp

#pragma once

#include <bg2e/scene/Node.hpp>

#include <memory>
#include <string>
#include <filesystem>

namespace bg2e {
namespace db {

extern BG2E_API bg2e::scene::Node * loadScene(
    const std::filesystem::path& filePath
);

extern BG2E_API bg2e::scene::Node * loadScene(
    const std::filesystem::path& basePath,
    const std::string& fileName
);

extern BG2E_API void saveScene(
    bg2e::scene::Node * sceneRoot,
    const std::filesystem::path& filePath
);

extern BG2E_API void saveScene(
    bg2e::scene::Node * sceneRoot,
    const std::filesystem::path& basePath,
    const std::string& fileName
);

}
}
