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

#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>

namespace bg2e {
namespace db {

struct Bg2Mesh
{
    std::shared_ptr<bg2e::geo::MeshPNUUT> mesh;
    std::vector<bg2e::base::MaterialAttributes> materials;
};

extern BG2E_API Bg2Mesh * loadMeshBg2(const std::filesystem::path& filePath);
extern BG2E_API Bg2Mesh * loadMeshBg2(const std::filesystem::path& filePath, const std::string& fileName);

}
}
