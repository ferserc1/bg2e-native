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
#include <bg2e/db/image.hpp>
#include <bg2e/math/base.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/scene/DrawableComponent.hpp>

#include <stdexcept>
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <random>
#include <unordered_map>
#include <unordered_set>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace bg2e::db {

namespace gltf {

    static std::vector<uint8_t> decodeBase64(const std::string& encoded)
    {
        auto digit = [](char c) -> int {
            if (c >= 'A' && c <= 'Z') return c - 'A';
            if (c >= 'a' && c <= 'z') return c - 'a' + 26;
            if (c >= '0' && c <= '9') return c - '0' + 52;
            if (c == '+') return 62;
            if (c == '/') return 63;
            return -1;
        };

        if (encoded.empty() || encoded.size() % 4 != 0)
        {
            throw std::runtime_error("Invalid base64 image data");
        }

        std::vector<uint8_t> bytes;
        bytes.reserve(encoded.size() / 4 * 3);
        for (size_t i = 0; i < encoded.size(); i += 4)
        {
            const int a = digit(encoded[i]);
            const int b = digit(encoded[i + 1]);
            const bool last = i + 4 == encoded.size();
            const bool pad2 = encoded[i + 2] == '=';
            const bool pad3 = encoded[i + 3] == '=';
            const int c = pad2 ? 0 : digit(encoded[i + 2]);
            const int d = pad3 ? 0 : digit(encoded[i + 3]);
            if (a < 0 || b < 0 || c < 0 || d < 0 || (pad2 && !pad3) ||
                ((pad2 || pad3) && !last))
            {
                throw std::runtime_error("Invalid base64 image data");
            }
            bytes.push_back(static_cast<uint8_t>((a << 2) | (b >> 4)));
            if (!pad2) bytes.push_back(static_cast<uint8_t>((b << 4) | (c >> 2)));
            if (!pad3) bytes.push_back(static_cast<uint8_t>((c << 6) | d));
        }
        return bytes;
    }

    class TemporaryImages {
    public:
        TemporaryImages(const std::filesystem::path& gltfPath, const cgltf_data* data)
            : _gltfPath(gltfPath), _data(data) {}

        ~TemporaryImages()
        {
            if (!_committed && !_directory.empty())
            {
                std::error_code error;
                std::filesystem::remove_all(_directory, error);
            }
        }

        std::filesystem::path resolve(const cgltf_image* image)
        {
            if (!image || image < _data->images || image >= _data->images + _data->images_count)
            {
                throw std::runtime_error("Invalid glTF image reference");
            }
            const size_t index = static_cast<size_t>(image - _data->images);
            if (auto found = _paths.find(index); found != _paths.end()) return found->second;

            try
            {
                auto encoded = readEncoded(image);
                uint32_t width = 0;
                uint32_t height = 0;
                auto pixels = db::decodeImageRGBA8(encoded.data(), encoded.size(), width, height);
                ensureDirectory();
                const auto path = _directory /
                    (_directory.filename().string() + "_image_" + std::to_string(index) + ".png");
                db::saveImage(path, pixels.data(), width, height, 4);
                return _paths.emplace(index, path).first->second;
            }
            catch (const std::exception& error)
            {
                throw std::runtime_error("glTF image " + std::to_string(index) + " in '" +
                                         _gltfPath.string() + "': " + error.what());
            }
        }

        void commit() { _committed = true; }

    private:
        std::vector<uint8_t> readEncoded(const cgltf_image* image) const
        {
            if (image->buffer_view)
            {
                const auto* bytes = cgltf_buffer_view_data(image->buffer_view);
                const size_t size = image->buffer_view->size;
                if (!bytes || size == 0) throw std::runtime_error("Empty image buffer view");
                return { bytes, bytes + size };
            }
            if (!image->uri) throw std::runtime_error("Image has no URI or buffer view");

            const std::string uri(image->uri);
            if (uri.rfind("data:", 0) == 0)
            {
                const auto comma = uri.find(',');
                if (comma == std::string::npos || comma < 7 ||
                    uri.substr(comma - 7, 7) != ";base64")
                {
                    throw std::runtime_error("Unsupported image data URI");
                }
                return decodeBase64(uri.substr(comma + 1));
            }

            if (uri.find("://") != std::string::npos)
                throw std::runtime_error("Remote image URI is unsupported");

            std::string decodedUri = uri;
            decodedUri.resize(cgltf_decode_uri(decodedUri.data()));
            const auto path = _gltfPath.parent_path() / decodedUri;
            std::ifstream input(path, std::ios::binary);
            if (!input) throw std::runtime_error("Could not open image '" + path.string() + "'");
            return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
        }

        void ensureDirectory()
        {
            if (!_directory.empty()) return;
            const auto root = std::filesystem::temp_directory_path();
            std::random_device random;
            for (int attempt = 0; attempt < 32; ++attempt)
            {
                auto candidate = root / ("bg2e-gltf-" + std::to_string(random()) + "-" +
                                         std::to_string(random()));
                if (std::filesystem::create_directory(candidate))
                {
                    _directory = std::filesystem::absolute(candidate);
                    return;
                }
            }
            throw std::runtime_error("Could not create temporary glTF image directory");
        }

        std::filesystem::path _gltfPath;
        const cgltf_data* _data;
        std::filesystem::path _directory;
        std::unordered_map<size_t, std::filesystem::path> _paths;
        bool _committed = false;
    };

    static cgltf_accessor* findAccessor(
        const cgltf_primitive* primitive, cgltf_attribute_type type, int index);

    static uint32_t textureUVSet(const cgltf_texture_view& view, const cgltf_primitive* primitive)
    {
        const int uvSet = view.has_transform && view.transform.has_texcoord
            ? view.transform.texcoord : view.texcoord;
        if (uvSet < 0 || uvSet > 1 ||
            !findAccessor(primitive, cgltf_attribute_type_texcoord, uvSet))
        {
            throw std::runtime_error("glTF texture refers to a missing or unsupported UV set");
        }
        return static_cast<uint32_t>(uvSet);
    }

    static glm::vec2 textureUVScale(const cgltf_texture_view& view)
    {
        if (!view.has_transform) return { 1.0f, 1.0f };
        if (view.transform.offset[0] != 0.0f || view.transform.offset[1] != 0.0f ||
            view.transform.rotation != 0.0f)
        {
            std::clog << "WARNING: glTF texture offset and rotation are unsupported; "
                         "only UV scale is imported" << std::endl;
        }
        return { view.transform.scale[0], view.transform.scale[1] };
    }

    static base::Texture::AddressMode addressMode(cgltf_wrap_mode mode)
    {
        if (mode == cgltf_wrap_mode_clamp_to_edge) return base::Texture::AddressModeClampToEdge;
        if (mode == cgltf_wrap_mode_mirrored_repeat) return base::Texture::AddressModeMirroredRepeat;
        return base::Texture::AddressModeRepeat;
    }

    static std::shared_ptr<base::Texture> textureFromView(
        const cgltf_texture_view& view, TemporaryImages& images, base::Color::Type colorType)
    {
        if (!view.texture || !view.texture->image) return nullptr;
        auto texture = std::make_shared<base::Texture>(images.resolve(view.texture->image));
        texture->setColorType(colorType);
        texture->setUseMipmaps(true);
        texture->setMagFilter(base::Texture::FilterLinear);
        texture->setMinFilter(base::Texture::FilterLinear);

        if (const auto* sampler = view.texture->sampler)
        {
            if (sampler->mag_filter == cgltf_filter_type_nearest)
                texture->setMagFilter(base::Texture::FilterNearest);
            if (sampler->min_filter == cgltf_filter_type_nearest ||
                sampler->min_filter == cgltf_filter_type_nearest_mipmap_nearest ||
                sampler->min_filter == cgltf_filter_type_nearest_mipmap_linear)
                texture->setMinFilter(base::Texture::FilterNearest);
            if (sampler->min_filter == cgltf_filter_type_nearest ||
                sampler->min_filter == cgltf_filter_type_linear)
                texture->setUseMipmaps(false);
            texture->setAddressMode(addressMode(sampler->wrap_s), addressMode(sampler->wrap_t));
        }
        return texture;
    }

    static base::MaterialAttributes materialAttributes(
        const cgltf_primitive* primitive, TemporaryImages& images)
    {
        base::MaterialAttributes result;
        // glTF defaults differ from MaterialAttributes defaults.
        result.setMetalness(1.0f);
        result.setRoughness(1.0f);

        const auto* material = primitive->material;
        if (material && material->has_pbr_metallic_roughness)
        {
            const auto& pbr = material->pbr_metallic_roughness;
            result.setAlbedo(base::Color{
                pbr.base_color_factor[0], pbr.base_color_factor[1],
                pbr.base_color_factor[2], pbr.base_color_factor[3]
            });
            result.setMetalness(pbr.metallic_factor);
            result.setRoughness(pbr.roughness_factor);

            if (auto texture = textureFromView(pbr.base_color_texture, images, base::Color::TypeSRGB))
            {
                result.setAlbedoTexture(texture);
                result.setAlbedoUVSet(textureUVSet(pbr.base_color_texture, primitive));
                result.setAlbedoScale(textureUVScale(pbr.base_color_texture));
            }
            if (auto texture = textureFromView(pbr.metallic_roughness_texture, images, base::Color::TypeLinear))
            {
                result.setMetalnessTexture(texture);
                result.setRoughnessTexture(texture);
                result.setMetalnessChannel(2);
                result.setRoughnessChannel(1);
                const auto uvSet = textureUVSet(pbr.metallic_roughness_texture, primitive);
                result.setMetalnessUVSet(uvSet);
                result.setRoughnessUVSet(uvSet);
                const auto scale = textureUVScale(pbr.metallic_roughness_texture);
                result.setMetalnessScale(scale);
                result.setRoughnessScale(scale);
            }
        }

        if (material)
        {
            result.setRefractionFactor(0.001f);

            // glTF defines transparency through alphaMode, not from the RGB
            // base color or its alpha factor alone. BLEND requires ordinary
            // alpha compositing; OPAQUE ignores alpha and MASK needs a cutoff
            // discard path that this importer does not yet provide.
            if (material->alpha_mode == cgltf_alpha_mode_blend)
            {
                result.setIsTransparent(true);
                result.setIsSolid(false);
            }

            if (auto texture = textureFromView(material->normal_texture, images, base::Color::TypeLinear))
            {
                result.setNormalTexture(texture);
                result.setNormalUVSet(textureUVSet(material->normal_texture, primitive));
                result.setNormalScale(textureUVScale(material->normal_texture));
                if (material->normal_texture.scale != 1.0f)
                {
                    std::clog << "WARNING: glTF normal texture strength is unsupported" << std::endl;
                }
            }
        }

        return result;
    }

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
    render::Engine* engine,
    scene::SceneProgressCallback onProgress
) {
    std::unique_ptr<cgltf_data, decltype(&cgltf_free)> data(gltf::loadGltfFile(filePath), &cgltf_free);
    gltf::TemporaryImages temporaryImages(filePath, data.get());

    std::unordered_set<const cgltf_image*> referencedImages;
    for (cgltf_size m = 0; m < data->meshes_count; ++m)
    {
        for (cgltf_size p = 0; p < data->meshes[m].primitives_count; ++p)
        {
            const auto* material = data->meshes[m].primitives[p].material;
            if (!material) continue;
            auto collect = [&referencedImages](const cgltf_texture_view& view) {
                if (view.texture && view.texture->image)
                    referencedImages.insert(view.texture->image);
            };
            if (material->has_pbr_metallic_roughness)
            {
                collect(material->pbr_metallic_roughness.base_color_texture);
                collect(material->pbr_metallic_roughness.metallic_roughness_texture);
            }
            collect(material->normal_texture);
        }
    }
    const int total = static_cast<int>(referencedImages.size() + data->meshes_count);
    int processed = 0;
    cgltf_size imageIndex = 0;
    for (const auto* image : referencedImages)
    {
        temporaryImages.resolve(image);
        if (onProgress) onProgress("image " + std::to_string(++imageIndex), ++processed, total);
    }


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
            gltf::appendPrimitive(data.get(), &primitive, mesh);
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
            drw->setName(std::string(filePath.stem().string()) + "_" + data->meshes[meshIdx].name);
        }
        else
        {
            drw->setName(std::string(filePath.stem().string()) + "_mesh_" + std::to_string(meshIdx));
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
        const auto& gltfMesh = data->meshes[meshIdx];
        for (uint32_t submeshIndex = 0; submeshIndex < drw->submeshesCount(); ++submeshIndex)
        {
            const auto& primitive = gltfMesh.primitives[submeshIndex];
            drw->setMaterial(gltf::materialAttributes(&primitive, temporaryImages), submeshIndex);
            drw->setSubmeshName(submeshNames[meshNameIndex], submeshIndex);
            ++meshNameIndex;
        }
        drw->load(engine);
        if (onProgress) onProgress(drw->name(), ++processed, total);
        drawables.push_back(drw);
        ++meshIdx;
    }

    // Load scene nodes
    auto result = new scene::Node();

    const cgltf_scene* scene = data->scene;
    for (cgltf_size i = 0; i < scene->nodes_count; ++i)
    {
        const cgltf_node* gltfRootNode = scene->nodes[i];
        result->addChild(gltf::createSceneTree(data.get(), gltfRootNode, drawables));
    }

    temporaryImages.commit();

    return result;
}

extern BG2E_API bg2e::scene::Node * loadGltf(
    const std::filesystem::path& basePath,
    const std::string& fileName,
    render::Engine* engine,
    scene::SceneProgressCallback onProgress
) {
    auto fullPath = basePath / fileName;
    return loadGltf(fullPath, engine, std::move(onProgress));
}

}
