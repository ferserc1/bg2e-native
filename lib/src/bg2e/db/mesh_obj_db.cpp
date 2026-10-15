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

#include <bg2e/db/mesh_obj.hpp>

#include <bg2e/base/Log.hpp>

#include <glm/glm.hpp>

#include <bg2e/geo/modifiers.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <fstream>
#include <functional>
#include <memory>
#include <cstdint>
#include <stdexcept>

namespace bg2e::db {

template <class MeshT>
MeshT* loadMeshObj(std::istream& inputStream)
{
    throw std::runtime_error("Unsupported mesh type");
}

void readObjMesh(
    std::istream& inputStream,
    std::function<void(const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv)> vertexCallback,
    std::function<void(uint32_t index)> indexCallback,
    std::function<void(uint32_t offset, uint32_t count)> submeshCallback)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    const bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &inputStream);
    if (!warn.empty())
    {
        bg2e_log_warning << "Warning loading OBJ file: " << warn << bg2e_log_end;
    }

    if (!loaded || !err.empty())
    {
        throw std::runtime_error(std::string("Error loading OBJ file: ") + err);
    }

    auto positionAt = [&](int index)
    {
        if (index < 0 || size_t(index) >= attrib.vertices.size() / 3)
        {
            throw std::runtime_error("OBJ position index is out of range");
        }
        const auto offset = size_t(index) * 3;
        return glm::vec3{attrib.vertices[offset], attrib.vertices[offset + 1], attrib.vertices[offset + 2]};
    };
    uint32_t currentIndex = 0;
    for (const auto& shape : shapes)
    {
        if (shape.mesh.indices.size() % 3 != 0 || shape.mesh.indices.size() > UINT32_MAX - currentIndex)
        {
            throw std::runtime_error("OBJ requires triangulated 32-bit indexed geometry");
        }
        const auto submeshOffset = currentIndex;
        for (size_t i = 0; i < shape.mesh.indices.size(); ++i)
        {
            const auto& index = shape.mesh.indices[i];
            const auto position = positionAt(index.vertex_index);
            glm::vec3 normal;
            if (index.normal_index < 0)
            {
                const auto first = i - i % 3;
                const auto p0 = positionAt(shape.mesh.indices[first].vertex_index);
                const auto p1 = positionAt(shape.mesh.indices[first + 1].vertex_index);
                const auto p2 = positionAt(shape.mesh.indices[first + 2].vertex_index);
                const auto cross = glm::cross(p1 - p0, p2 - p0);
                normal = glm::length(cross) > 1e-8f ? glm::normalize(cross) : glm::vec3{0, 0, 1};
            }
            else
            {
                if (size_t(index.normal_index) >= attrib.normals.size() / 3)
                {
                    throw std::runtime_error("OBJ normal index is out of range");
                }
                const auto offset = size_t(index.normal_index) * 3;
                normal = {attrib.normals[offset], attrib.normals[offset + 1], attrib.normals[offset + 2]};
            }
            glm::vec2 uv{0};
            if (index.texcoord_index >= 0)
            {
                if (size_t(index.texcoord_index) >= attrib.texcoords.size() / 2)
                {
                    throw std::runtime_error("OBJ UV index is out of range");
                }
                const auto offset = size_t(index.texcoord_index) * 2;
                uv = {attrib.texcoords[offset], 1.0f - attrib.texcoords[offset + 1]};
            }
            vertexCallback(position, normal, uv);
            indexCallback(currentIndex++);
        }
        if (currentIndex > submeshOffset)
        {
            submeshCallback(submeshOffset, currentIndex - submeshOffset);
        }
    }
}

template <>
bg2e::geo::MeshP* loadMeshObj<bg2e::geo::MeshP>(std::istream& /* inputStream */)
{
    return nullptr;
}

template <>
bg2e::geo::MeshPN* loadMeshObj<bg2e::geo::MeshPN>(std::istream& inputStream)
{
    auto mesh = new bg2e::geo::MeshPN();

    readObjMesh(
        inputStream,
        [&](const glm::vec3& position, const glm::vec3& normal, const glm::vec2& /* uv */)
        {
            mesh->vertices.push_back(bg2e::geo::VertexPN{position, normal});
        },
        [&](uint32_t index)
        {
            mesh->indices.push_back(index);
        },
        [&](uint32_t offset, uint32_t count)
        {
            mesh->submeshes.push_back({offset, count});
        });
    return mesh;
}

template <>
bg2e::geo::MeshPC* loadMeshObj<bg2e::geo::MeshPC>(std::istream& inputStream)
{
    auto mesh = new bg2e::geo::MeshPC();

    readObjMesh(
        inputStream,
        [&](const glm::vec3& position, const glm::vec3& /* normal */, const glm::vec2& /* uv */)
        {
            mesh->vertices.push_back(bg2e::geo::VertexPC{position, {1.0f, 1.0f, 1.0f, 1.0f}});
        },
        [&](uint32_t index)
        {
            mesh->indices.push_back(index);
        },
        [&](uint32_t offset, uint32_t count)
        {
            mesh->submeshes.push_back({offset, count});
        });
    return mesh;
}

template <>
bg2e::geo::MeshPU* loadMeshObj<bg2e::geo::MeshPU>(std::istream& inputStream)
{
    auto mesh = new bg2e::geo::MeshPU();

    readObjMesh(
        inputStream,
        [&](const glm::vec3& position, const glm::vec3& /* normal */, const glm::vec2& uv)
        {
            mesh->vertices.push_back(bg2e::geo::VertexPU{position, uv});
        },
        [&](uint32_t index)
        {
            mesh->indices.push_back(index);
        },
        [&](uint32_t offset, uint32_t count)
        {
            mesh->submeshes.push_back({offset, count});
        });
    return mesh;
}

template <>
bg2e::geo::MeshPNU* loadMeshObj<bg2e::geo::MeshPNU>(std::istream& inputStream)
{
    auto mesh = new bg2e::geo::MeshPNU();

    readObjMesh(
        inputStream,
        [&](const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv)
        {
            mesh->vertices.push_back(bg2e::geo::VertexPNU{position, normal, uv});
        },
        [&](uint32_t index)
        {
            mesh->indices.push_back(index);
        },
        [&](uint32_t offset, uint32_t count)
        {
            mesh->submeshes.push_back({offset, count});
        });
    return mesh;
}

template <>
bg2e::geo::MeshPNC* loadMeshObj<bg2e::geo::MeshPNC>(std::istream& inputStream)
{
    auto mesh = new bg2e::geo::MeshPNC();

    readObjMesh(
        inputStream,
        [&](const glm::vec3& position, const glm::vec3& normal, const glm::vec2& /* uv */)
        {
            mesh->vertices.push_back(bg2e::geo::VertexPNC{position, normal, {1.0f, 1.0f, 1.0f, 1.0f}});
        },
        [&](uint32_t index)
        {
            mesh->indices.push_back(index);
        },
        [&](uint32_t offset, uint32_t count)
        {
            mesh->submeshes.push_back({offset, count});
        });
    return mesh;
}

template <>
bg2e::geo::MeshPNUC* loadMeshObj<bg2e::geo::MeshPNUC>(std::istream& inputStream)
{
    auto mesh = new bg2e::geo::MeshPNUC();

    readObjMesh(
        inputStream,
        [&](const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv)
        {
            mesh->vertices.push_back(bg2e::geo::VertexPNUC{position, normal, uv, {1.0f, 1.0f, 1.0f, 1.0f}});
        },
        [&](uint32_t index)
        {
            mesh->indices.push_back(index);
        },
        [&](uint32_t offset, uint32_t count)
        {
            mesh->submeshes.push_back({offset, count});
        });
    return mesh;
}

template <>
bg2e::geo::MeshPNUT* loadMeshObj<bg2e::geo::MeshPNUT>(std::istream& inputStream)
{
    auto mesh = new bg2e::geo::MeshPNUT();

    readObjMesh(
        inputStream,
        [&](const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv)
        {
            mesh->vertices.push_back(bg2e::geo::VertexPNUT{position, normal, uv, {0.0f, 0.0f, 0.0f}});
        },
        [&](uint32_t index)
        {
            mesh->indices.push_back(index);
        },
        [&](uint32_t offset, uint32_t count)
        {
            mesh->submeshes.push_back({offset, count});
        });

    geo::GenTangentsModifier<geo::MeshPNUT> tangentGenerator(mesh);
    tangentGenerator.apply();

    return mesh;
}

template <>
bg2e::geo::MeshPNUUT* loadMeshObj<bg2e::geo::MeshPNUUT>(std::istream& inputStream)
{
    auto mesh = std::make_unique<bg2e::geo::MeshPNUUT>();

    readObjMesh(
        inputStream,
        [&](const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv)
        {
            mesh->vertices.push_back(bg2e::geo::VertexPNUUT{position, normal, uv, uv, {0.0f, 0.0f, 0.0f}});
        },
        [&](uint32_t index)
        {
            mesh->indices.push_back(index);
        },
        [&](uint32_t offset, uint32_t count)
        {
            mesh->submeshes.push_back({offset, count});
        });

    geo::GenTangentsModifier<geo::MeshPNUUT> tangentGenerator(mesh.get());
    tangentGenerator.apply();

    return mesh.release();
}

std::shared_ptr<draw::Drawable> loadDrawableObj(const std::filesystem::path& filePath, draw::Engine& engine)
{
    auto result = std::make_shared<draw::Drawable>();
    result->setName(filePath.stem().string());
    result->setMesh(std::shared_ptr<geo::MeshPNUUT>(loadMeshObj<geo::MeshPNUUT>(filePath)));
    for (uint32_t i = 0; i < result->submeshesCount(); ++i)
    {
        result->setSubmeshName("submesh_" + std::to_string(i), i);
    }
    result->load(&engine);
    return result;
}

std::shared_ptr<draw::Drawable> loadDrawableObj(const std::filesystem::path& basePath,
    const std::string& fileName, draw::Engine& engine)
{
    return loadDrawableObj(basePath / fileName, engine);
}

std::shared_ptr<bg2e::scene::Drawable> loadDrawableObj(
    const std::filesystem::path& filePath,
    bg2e::render::Engine* engine)
{
    auto mesh = loadMeshObj<bg2e::geo::Mesh>(filePath);
    auto drawable = std::make_shared<bg2e::scene::Drawable>();
    drawable->setName(filePath.stem().string());
    drawable->setMesh(mesh);
    drawable->load(engine);
    for (uint32_t i = 0; i < mesh->submeshes.size(); i++)
    {
        drawable->setSubmeshName("submesh_" + std::to_string(i));
    }
    return drawable;
}

std::shared_ptr<bg2e::scene::Drawable> loadDrawableObj(
    const std::filesystem::path& basePath,
    const std::string& fileName,
    bg2e::render::Engine* engine)
{
    auto fullPath = basePath / fileName;
    return loadDrawableObj(fullPath, engine);
}

}
