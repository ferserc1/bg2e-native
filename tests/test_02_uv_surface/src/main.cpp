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

#include <bg2e/render/Engine.hpp>
#include <bg2e/render/UvMapPreviewRenderer.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/scene/Mesh.hpp>
#include <bg2e/geo/UvAtlasValidator.hpp>
#include <bg2e/render/vulkan/Image.hpp>

#include "UvSurfacePass.hpp"

#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::shared_ptr<bg2e::scene::Mesh> makeTwoSubmeshFixture()
{
    using namespace bg2e;
    auto mesh = std::make_shared<scene::Mesh>();

    const auto vertex = [](glm::vec3 position, glm::vec2 uv1, glm::vec2 uv2) {
        geo::VertexPNUUT result{};
        result.position = position;
        result.normal = glm::vec3(0.0f, 0.0f, 1.0f);
        result.texCoord0 = uv1;
        result.texCoord1 = uv2;
        result.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        return result;
    };

    mesh->vertices = {
        vertex({-0.8f, -0.8f, 0.0f}, {0.0f, 0.0f}, {0.1f, 0.1f}),
        vertex({-0.2f, -0.8f, 0.0f}, {1.0f, 0.0f}, {0.4f, 0.1f}),
        vertex({-0.8f, -0.2f, 0.0f}, {0.0f, 1.0f}, {0.1f, 0.4f}),
        vertex({ 0.2f,  0.2f, 0.0f}, {0.0f, 0.0f}, {0.6f, 0.6f}),
        vertex({ 0.8f,  0.2f, 0.0f}, {1.0f, 0.0f}, {0.9f, 0.6f}),
        vertex({ 0.8f,  0.8f, 0.0f}, {1.0f, 1.0f}, {0.9f, 0.9f})
    };
    mesh->indices = { 0, 1, 2, 3, 4, 5 };
    mesh->submeshes = { { 0, 3 }, { 3, 3 } };
    return mesh;
}

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

}

int main()
{
    bg2e::render::Engine engine;
    bool engineInitialized = false;
    try
    {
        engine.init();
        engineInitialized = true;

        {
            bg2e::render::GBufferManager cameraProfile(&engine);
            cameraProfile.build({ 4, 4 });
            require(cameraProfile.imageCount() == 6, "default camera G-buffer did not add the baked RGB attachment");
            require(cameraProfile.depthImage() != nullptr, "default camera G-buffer lost its depth image");
            require(cameraProfile.depthFormat() == VK_FORMAT_D32_SFLOAT,
                "default camera G-buffer changed its D32 depth format");
        }

        {
            bg2e::scene::Drawable drawable;
            drawable.setMesh(makeTwoSubmeshFixture());
            const auto uvCheck = bg2e::geo::UvAtlasValidator::validate(*drawable.mesh(), 1);
            require(uvCheck.valid, uvCheck.message);
            drawable.load(&engine);

            constexpr uint32_t resolution = 64;
            bg2e::render::UvSurfacePass surfacePass(&engine, { resolution, resolution });
            const auto& gbuffer = surfacePass.manager(0);
            require(gbuffer.imageCount() == 4, "UV surface pass did not create four color attachments");
            require(!gbuffer.depthImage(), "UV surface G-buffer unexpectedly has a depth image");
            require(gbuffer.depthFormat() == VK_FORMAT_UNDEFINED,
                "UV surface G-buffer did not report VK_FORMAT_UNDEFINED depth");

            engine.command().immediateSubmit([&](VkCommandBuffer cmd) {
                surfacePass.record(cmd, drawable, glm::mat4(1.0f), 0);
            });
            const auto diagnostic = surfacePass.readDiagnostic(0);

            std::array<uint32_t, 2> coverageBySubmesh{};
            uint32_t emptyTexels = 0;
            for (size_t pixel = 0; pixel < diagnostic.validTexels.size(); ++pixel)
            {
                const uint32_t valid = diagnostic.validTexels[pixel];
                const uint32_t submesh = diagnostic.submeshIndices[pixel];
                require(valid <= 1, "valid-texel mask contains a value other than zero or one");
                if (valid == 0)
                {
                    ++emptyTexels;
                    require(submesh == 0, "uncovered texel did not retain the zero-cleared submesh ID");
                    continue;
                }
                require(submesh < coverageBySubmesh.size(), "covered texel has an invalid submesh ID");
                ++coverageBySubmesh[submesh];
            }

            require(coverageBySubmesh[0] > 0, "first UV2 submesh produced no covered texels");
            require(coverageBySubmesh[1] > 0, "second UV2 submesh produced no covered texels");
            require(emptyTexels > 0, "fixture unexpectedly covered every atlas texel");

            const auto texelAtUv = [resolution](float u, float v) {
                const uint32_t x = static_cast<uint32_t>(u * resolution);
                const uint32_t y = static_cast<uint32_t>((1.0f - v) * resolution);
                return static_cast<size_t>(y) * resolution + x;
            };
            const size_t firstRegion = texelAtUv(0.2f, 0.2f);
            const size_t secondRegion = texelAtUv(0.75f, 0.7f);
            const size_t emptyRegion = texelAtUv(0.5f, 0.5f);
            require(diagnostic.validTexels[firstRegion] == 1 && diagnostic.submeshIndices[firstRegion] == 0,
                "known first-submesh UV sample has incorrect coverage or identity");
            require(diagnostic.validTexels[secondRegion] == 1 && diagnostic.submeshIndices[secondRegion] == 1,
                "known second-submesh UV sample has incorrect coverage or identity");
            require(diagnostic.validTexels[emptyRegion] == 0,
                "uncovered atlas sample was marked valid; bake consumers could leak light there");

            std::cout << "UV surface diagnostic passed: submesh 0=" << coverageBySubmesh[0]
                      << " texels, submesh 1=" << coverageBySubmesh[1]
                      << " texels, empty=" << emptyTexels << std::endl;
        }

        {
            const auto mesh = makeTwoSubmeshFixture();
            constexpr uint32_t previewResolution = 64;
            bg2e::render::UvMapPreviewRenderer preview(&engine, previewResolution);
            require(preview.texture() != nullptr && preview.texture()->image() != nullptr,
                "UV map preview did not expose a renderable texture");

            const auto texelAtUv = [previewResolution](float u, float v) {
                const uint32_t x = static_cast<uint32_t>(u * previewResolution);
                const uint32_t y = static_cast<uint32_t>((1.0f - v) * previewResolution);
                return (static_cast<size_t>(y) * previewResolution + x) * 4;
            };
            const auto readPreview = [&]() {
                std::vector<uint8_t> pixels;
                bg2e::render::vulkan::Image::readPixelsRGBA8(&engine, preview.texture()->image(),
                    0, 0, previewResolution, previewResolution, pixels,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                require(pixels.size() == static_cast<size_t>(previewResolution) * previewResolution * 4,
                    "UV map preview readback returned an unexpected size");
                return pixels;
            };
            const auto isBlack = [](const std::vector<uint8_t>& pixels, size_t offset) {
                return pixels[offset] == 0 && pixels[offset + 1] == 0 && pixels[offset + 2] == 0;
            };

            // UV1: the first triangle covers half of the [0,1] square
            preview.render(*mesh, 0);
            auto uv1Pixels = readPreview();
            require(!isBlack(uv1Pixels, texelAtUv(0.2f, 0.2f)),
                "UV1 preview did not fill the covered triangle region");

            // UV2: two small triangles; (0.2, 0.2) is covered and (0.5, 0.5) is empty
            preview.render(*mesh, 1);
            auto uv2Pixels = readPreview();
            require(!isBlack(uv2Pixels, texelAtUv(0.2f, 0.2f)),
                "UV2 preview did not fill the first submesh region");
            require(isBlack(uv2Pixels, texelAtUv(0.5f, 0.5f)),
                "UV2 preview filled an uncovered texel; coverage preview is unreliable");

            bool boundaryFound = false;
            for (uint32_t i = 0; i < previewResolution && !boundaryFound; ++i)
            {
                for (uint32_t edge : { 0u, previewResolution - 1u })
                {
                    for (auto offset : { (static_cast<size_t>(edge) * previewResolution + i) * 4,
                                         (static_cast<size_t>(i) * previewResolution + edge) * 4 })
                    {
                        if (uv2Pixels[offset] > 200 && uv2Pixels[offset + 1] > 200 && uv2Pixels[offset + 2] > 200)
                        {
                            boundaryFound = true;
                            break;
                        }
                    }
                }
            }
            require(boundaryFound, "UV2 preview did not draw the [0,1] atlas boundary");

            // Resolution changes recreate the target without losing the texture handle
            preview.setResolution(32);
            preview.render(*mesh, 1);
            require(preview.texture() != nullptr && preview.texture()->image() != nullptr,
                "UV map preview lost its texture after a resolution change");
            require(preview.texture()->image()->extent2D().width == 32,
                "UV map preview target was not recreated at the new resolution");

            std::cout << "UV map preview smoke test passed" << std::endl;
        }
        engine.cleanup();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "UV surface diagnostic failed: " << error.what() << std::endl;
        if (engineInitialized)
        {
            engine.cleanup();
        }
        return 1;
    }
}
