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

#include <bg2e/db/scene_gltf.hpp>
#include <bg2e/geo/Mesh.hpp>
#include <bg2e/geo/modifiers.hpp>
#include <bg2e/math/base.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/scene/DrawableComponent.hpp>

#include <stdexcept>
#include <vector>
#include <memory>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace bg2e::db {

namespace gltf {

    static cgltf_data* loadGltfFile(const std::filesystem::path& filePath)
    {
        cgltf_options options {};
        cgltf_data* data = nullptr;

        cgltf_result result = cgltf_parse_file(&options, filePath.string().c_str(), &data);
        if (result != cgltf_result_success)
        {
            throw std::runtime_error("Failed to parse GLTF file " + filePath.string());
        }

        result = cgltf_load_buffers(&options, data, filePath.string().c_str());
        if (result != cgltf_result_success)
        {
            cgltf_free(data);
            throw std::runtime_error("Failed to load buffers for " + filePath.string());
        }

        return data;
    }

    static cgltf_accessor* findAccessor(
        const cgltf_primitive* primitive,
        cgltf_attribute_type type,
        int index = 0  // Used for the second UV set
    ) {
        for (cgltf_size i = 0; i < primitive->attributes_count; ++i)
        {
            const cgltf_attribute& attr = primitive->attributes[i];
            if (attr.type == type && attr.index == index)
            {
                return attr.data;
            }
        }
        return nullptr;
    }

    static void appendPrimitive(
        const cgltf_data* data,
        const cgltf_primitive* primitive,
        std::shared_ptr<bg2e::geo::Mesh> mesh
    )
    {
        using namespace bg2e::geo;

        // Index
        if (!primitive->indices)
        {
            throw std::runtime_error("Primitive without indices is not supported");
        }

        auto posAccessor = findAccessor(primitive, cgltf_attribute_type_position);
        if (!posAccessor)
        {
            throw std::runtime_error("Invalid mesh: Primitive is missing POSITION");
        }

        auto normAccessor = findAccessor(primitive, cgltf_attribute_type_normal);
        if (!normAccessor)
        {
            throw std::runtime_error("Invalid mesh: Primitive is missing NORMAL");
        }

        auto uv0Accessor = findAccessor(primitive, cgltf_attribute_type_texcoord, 0);
        if (!uv0Accessor)
        {
            throw std::runtime_error("Invalid mesh: Primitive is missing UV0");
        }
        auto uv1Accessor = findAccessor(primitive, cgltf_attribute_type_texcoord, 1);
        if (!uv1Accessor)
        {
            uv1Accessor = uv0Accessor;
        }

        // Tangents are optionals because it can be generated procedurally, but its better to import tangents
        auto tangentAccessor = findAccessor(primitive, cgltf_attribute_type_tangent);
        if (!tangentAccessor)
        {
            std::cout << "WARNING: Primitive is missing TANGENTS. The tangents will be generated procedurally, but it is better to import them from a file" << std::endl;
        }

        if (primitive->type != cgltf_primitive_type_triangles)
        {
            std::cout << "WARNING: Unsupported primitive type: " << primitive->type << std::endl;
        }

        const size_t vertexCount = posAccessor->count;
        std::vector<Vertex> localVertices;
        // Vertex data
        for (size_t i = 0; i < vertexCount; ++i)
        {
            Vertex v;
            // POSITION
            float pos[3];
            cgltf_accessor_read_float(posAccessor, i, pos, 3);
            v.position = glm::vec3(pos[0], pos[1], pos[2]);

            // NORMAL
            float n[3];
            cgltf_accessor_read_float(normAccessor, i, n, 3);
            v.normal = glm::vec3(n[0], n[1], n[2]);

            // UV0
            float uv[2];
            cgltf_accessor_read_float(uv0Accessor, i, uv, 2);
            v.texCoord0 = glm::vec2(uv[0], uv[1]);

            // UV1
            cgltf_accessor_read_float(uv1Accessor, i, uv, 2);
            v.texCoord1 = glm::vec2(uv[0], uv[1]);

            // TANGENT
            if (tangentAccessor)
            {
                float t[4];
                cgltf_accessor_read_float(tangentAccessor, i, t, 4);
                v.tangent = glm::vec3(t[0], t[1], t[2]);
            }
            else
            {
                // This is used to tell the load function that we want to generate the tangents procedurally.
                v.tangent = glm::vec3(0, 0, 0);
            }
            localVertices.push_back(v);
        }

        const size_t indexCount = primitive->indices->count;
        size_t baseIndex = mesh->indices.size();
        for (size_t i = 0; i < indexCount; ++i)
        {
            uint32_t idx = static_cast<uint32_t>(cgltf_accessor_read_index(primitive->indices, i));
            auto & v = localVertices[idx];
            mesh->vertices.push_back(v);
            mesh->indices.push_back(static_cast<uint32_t>(baseIndex + i));
        }

        mesh->submeshes.push_back(Submesh{
            static_cast<uint32_t>(baseIndex),
            static_cast<uint32_t>(indexCount)
        });
    }

    static glm::mat4 nodeTransform(const cgltf_node* node)
    {
        if (node->has_matrix)
        {
            glm::mat4 m = glm::make_mat4(node->matrix);
            return m;
        }
        glm::vec3 T(0.0f);
        glm::quat R(1.0, 0.0, 0.0, 0.0);
        glm::vec3 S(1.0f);

        if (node->has_translation) {
            T = glm::vec3(node->translation[0], node->translation[1], node->translation[2]);
        }

        if (node->has_rotation) {
            R = glm::quat(
                node->rotation[3],
                node->rotation[0],
                node->rotation[1],
                node->rotation[2]
            );
        }

        if (node->has_scale) {
            S = glm::vec3(node->scale[0], node->scale[1], node->scale[2]);
        }

        return glm::translate(glm::mat4(1.0f), T)
             * glm::mat4_cast(R)
             * glm::scale(glm::mat4(1.0f), S);
    }

    scene::Node* createSceneTree(
        const cgltf_data* data,
        const cgltf_node* gltfNode,
        const std::vector<std::shared_ptr<scene::Drawable>>& drawables
    ) {
        auto node = new scene::Node();
        if (gltfNode->name)
        {
            node->setName(gltfNode->name);
        }

        auto localTransform = nodeTransform(gltfNode);
        node->addComponent(new scene::TransformComponent(localTransform));

        if (gltfNode->mesh)
        {
            size_t meshIndex = gltfNode->mesh - data->meshes;
            auto drawable = drawables[meshIndex];

            node->addComponent(new scene::DrawableComponent(drawable));
        }

        // Node children
        for (cgltf_size i = 0; i < gltfNode->children_count; ++i)
        {
            const cgltf_node* child = gltfNode->children[i];
            auto childNode = createSceneTree(data, child, drawables);
            node->addChild(childNode);
        }

        return node;
    }
}


extern BG2E_API bg2e::scene::Node * loadGltf(
    const std::filesystem::path& filePath,
    render::Engine* engine
) {
    auto data = gltf::loadGltfFile(filePath);

    std::vector<std::string> submeshNames;
    std::vector<std::shared_ptr<geo::Mesh>> meshes;

    for (cgltf_size m = 0; m < data->meshes_count; ++m)
    {
        const cgltf_mesh& gltfMesh = data->meshes[m];

        auto mesh = std::make_shared<geo::Mesh>();
        for (cgltf_size p = 0; p < gltfMesh.primitives_count; ++p)
        {
            const cgltf_primitive& primitive = gltfMesh.primitives[p];
            if (primitive.material && primitive.material->name)
            {
                submeshNames.push_back(primitive.material->name);
            }
            else
            {
                submeshNames.push_back((gltfMesh.name ? gltfMesh.name : "submesh_") + std::to_string(p));
            }
            gltf::appendPrimitive(data, &primitive, mesh);
        }
        meshes.push_back(mesh);
    }

    // Load drawables
    std::vector<std::shared_ptr<bg2e::scene::Drawable>> drawables;

    size_t meshNameIndex = 0;
    size_t meshIdx = 0;
    for (auto & mesh : meshes)
    {
        auto drw = std::make_shared<bg2e::scene::Drawable>();

        if (data->meshes[meshIdx].name)
        {
            drw->setName(std::string(filePath.stem()) + "_" + data->meshes[meshIdx].name);
        }
        else
        {
            drw->setName(std::string(filePath.stem()) + "_mesh_" + std::to_string(meshIdx));
        }

        // Check if the tangents must be generated procedurally
        if (mesh->vertices.size() > 0 &&
            mesh->vertices[0].tangent.x == 0.0f &&
            mesh->vertices[0].tangent.y == 0.0f &&
            mesh->vertices[0].tangent.z == 0.0f
        ) {
            geo::GenTangentsModifier<geo::Mesh> genTangents(mesh.get());
            genTangents.apply();
        }

        drw->setMesh(mesh);
        drw->load(engine);
        for (uint32_t submeshIndex = 0; submeshIndex < drw->submeshesCount(); ++submeshIndex)
        {
            drw->setSubmeshName(submeshNames[meshNameIndex], submeshIndex);
            ++meshNameIndex;
        }
        drawables.push_back(drw);
        ++meshIdx;
    }

    // Load scene nodes
    auto result = new scene::Node();

    const cgltf_scene* scene = data->scene;
    for (cgltf_size i = 0; i < scene->nodes_count; ++i)
    {
        const cgltf_node* gltfRootNode = scene->nodes[i];
        result->addChild(gltf::createSceneTree(data, gltfRootNode, drawables));
    }

    cgltf_free(data);

    return result;
}

extern BG2E_API bg2e::scene::Node * loadGltf(
    const std::filesystem::path& basePath,
    const std::string& fileName,
    render::Engine* engine
) {
    auto fullPath = basePath / fileName;
    return loadGltf(fullPath, engine);
}

}
