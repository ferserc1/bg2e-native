/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
#include <functional>

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
    const std::filesystem::path& filePath,
    bg2e::render::Engine * engine,
    std::function<void()> onTextureLoaded
);
extern BG2E_API std::shared_ptr<bg2e::scene::Drawable> loadDrawableBg2(
    const std::filesystem::path& basePath,
    const std::string& fileName,
    bg2e::render::Engine * engine
);

extern BG2E_API uint32_t countMeshTextures(const std::filesystem::path& filePath);

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
