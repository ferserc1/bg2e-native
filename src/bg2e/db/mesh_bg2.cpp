//
//  mesh_bg2.cpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 30/6/25.
//

#include <bg2e/db/mesh_bg2.hpp>

#include <bg2e/geo/sphere.hpp>

namespace bg2e::db {

Bg2Mesh * loadMeshBg2(const std::filesystem::path& filePath)
{
    auto result = new Bg2Mesh();
    //result->mesh = std::make_shared<geo::MeshPNUUT>();
    result->mesh = std::shared_ptr<bg2e::geo::Mesh>(geo::createSphere(1.3, 40, 40));
    
    base::MaterialAttributes mat;
    mat.setAlbedo(base::Color::Red());
    mat.setMetalness(0.92);
    mat.setRoughness(0.12);
    result->materials.push_back(mat);
    
    return result;
}

Bg2Mesh * loadMeshBg2(const std::filesystem::path& filePath, const std::string& fileName)
{
    auto fullPath = filePath;
    fullPath.append(fileName);
    return loadMeshBg2(fullPath);
}

}
