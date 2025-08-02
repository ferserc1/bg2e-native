//
//  mesh_bg2.hpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 30/6/25.
//

#pragma once

#include <bg2e/common.hpp>

#include <bg2e/base/MaterialAttributes.hpp>
#include <bg2e/geo/Mesh.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/render/Engine.hpp>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

namespace bg2e {
namespace db {

struct Bg2Mesh
{
    std::shared_ptr<bg2e::geo::MeshPNUUT> mesh;
    std::vector<bg2e::base::MaterialAttributes> materials;
};

extern BG2E_API Bg2Mesh * loadMeshBg2(
    const std::filesystem::path& filePath
);
extern BG2E_API Bg2Mesh * loadMeshBg2(
    const std::filesystem::path& filePath,
    const std::string& fileName
);

extern BG2E_API void storeMeshBg2(
    const std::filesystem::path& filePath,
    Bg2Mesh * mesh
);
extern BG2E_API void storeMeshBg2(
    const std::filesystem::path& filePath,
    const std::string& fileName,
    Bg2Mesh * mesh
);

extern BG2E_API std::shared_ptr<bg2e::scene::Drawable> loadDrawableBg2(
    const std::filesystem::path& filePath,
    bg2e::render::Engine * engine
);
extern BG2E_API std::shared_ptr<bg2e::scene::Drawable> loadDrawableBg2(
    const std::filesystem::path& basePath,
    const std::string& fileName,
    bg2e::render::Engine * engine
);

extern BG2E_API void storeDrawableBg2(
    const std::filesystem::path& filePath,
    bg2e::scene::Drawable* drawable
);
extern BG2E_API void storeDrawableBg2(
    const std::filesystem::path& basePath,
    const std::string& fileName,
    bg2e::scene::Drawable* drawable
);
extern BG2E_API void storeDrawableBg2(
    const std::filesystem::path& filePath,
    std::shared_ptr<bg2e::scene::Drawable> drawable
);
extern BG2E_API void storeDrawableBg2(
    const std::filesystem::path& basePath,
    const std::string& fileName,
    std::shared_ptr<bg2e::scene::Drawable> drawable
);

}
}
