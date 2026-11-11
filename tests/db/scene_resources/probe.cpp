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

#include <bg2e/db/SceneResourceManager.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/geo/cube.hpp>
#include <bg2e/base/Texture.hpp>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
namespace fs = std::filesystem;
using Manager = bg2e::db::SceneResourceManager;

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

template <typename F>
void mustThrow(F function)
{
    bool threw = false;
    try
    {
        function();
    }
    catch (const std::exception&)
    {
        threw = true;
    }
    require(threw, "Expected operation to reject the input");
}

void write(const fs::path& path, const std::string& content)
{
    std::ofstream output(path, std::ios::binary);
    output.exceptions(std::ios::failbit | std::ios::badbit);
    output << content;
}

void run(const fs::path& base)
{
    const auto scene = base / "scene";
    fs::create_directories(scene / "models");
    fs::create_directory(scene / "fonts");
    fs::create_directory(scene / "nested");
    const auto image = scene / "models/shared.png";
    write(image, "image");
    write(base / "external.hdr", "external");
    write(scene / "fonts/test.ttf", "font");
    write(scene / "fonts/LICENSE.txt", "license");
    write(scene / "orphan.txt", "unused");
    write(scene / "nested/orphan.bin", "unused");

    bg2e::db::Bg2Mesh mesh;
    mesh.mesh.reset(bg2e::geo::createCube(1, 1, 1));
    mesh.materials.resize(mesh.mesh->submeshes.size());
    auto texture = std::make_shared<bg2e::base::Texture>();
    texture->setImageFilePath(image.string());
    for (auto& material : mesh.materials)
    {
        material.setAlbedoTexture(texture);
        material.setAoTexture(texture);
    }
    const auto model = scene / "models/model.bg2";
    bg2e::db::storeMeshBg2(model, &mesh);
    const auto paths = bg2e::db::meshTexturePaths(model);
    require(paths.size() == mesh.materials.size() * 2, "Repeated material slots were lost");
    require(bg2e::db::countMeshTextures(model) == paths.size(), "Loader count disagrees with paths");
    for (const auto& path : paths)
    {
        require(fs::equivalent(path, image), "Model texture resolved against the wrong directory");
    }
    const auto document = scene / "scene.json";
    write(document, R"({"fileType":"bg2e::scene","scene":[{"components":[
        {"type":"Drawable","path":"models/model.bg2"},
        {"type":"Environment","equirectangularTexture":"../external.hdr"},
        {"type":"TextFonts","fonts":[{"path":"fonts/test.ttf","notices":["fonts/LICENSE.txt"]}]},
        {"type":"Text","material":{"normalTexture":"models/shared.png"}}
    ],"children":[{"components":[{"type":"Drawable","name":"models/model"}]}]}]})");
    auto analysis = Manager::analyze(scene);
    require(analysis.warnings.empty(), "Valid dependency graph produced warnings");
    require(analysis.resources.size() == 6, "Resources were lost or not deduplicated");
    require(analysis.externalResources.size() == 1, "External resource was not classified");
    require(analysis.unusedFiles.size() == 2, "Unexpected cleanup candidates");
    require(Manager::analyze(document).resources == analysis.resources, "Direct JSON input failed");

    auto plan = Manager::createCleanupPlan(analysis);
    const auto time = fs::last_write_time(image);
    write(image, "IMAGE"); // Same size and timestamp: content fingerprint must detect it.
    fs::last_write_time(image, time);
    mustThrow([&]
    {
        Manager::executeCleanup(plan);
    });
    require(fs::exists(scene / "orphan.txt"), "Stale plan deleted a file");
    mustThrow([&]
    {
        Manager::createCleanupPlan(analysis);
    });
    analysis = Manager::analyze(scene);
    auto result = Manager::executeCleanup(Manager::createCleanupPlan(analysis));
    require(result.errors.empty() && result.deleted.size() == 2, "Cleanup did not delete exactly the candidates");
    require(fs::exists(image) && fs::exists(model) && fs::exists(document) &&
        fs::exists(scene / "fonts/LICENSE.txt") && fs::exists(base / "external.hdr"),
        "Cleanup removed a dependency");

    write(scene / "unknown-resource.bin", "unknown");
    write(document, R"({"scene":[{"components":[{"type":"CustomComponent","path":"unknown-resource.bin"}]}]})");
    analysis = Manager::analyze(scene);
    require(!analysis.warnings.empty(), "Unknown component was silently accepted");
    mustThrow([&]
    {
        Manager::createCleanupPlan(analysis);
    });
    result = Manager::executeCleanup(Manager::createCleanupPlan(analysis, true));
    require(result.errors.empty() && !fs::exists(scene / "unknown-resource.bin"),
        "Explicit warning override did not allow cleanup");
    require(fs::exists(base / "external.hdr"), "External file was removed");

    write(scene / "invalid.bg2", "bad");
    write(document, R"({"scene":[{"components":[{"type":"Drawable","path":"invalid.bg2"}]}]})");
    analysis = Manager::analyze(scene);
    require(!analysis.warnings.empty(), "Malformed model dependencies did not warn");
    require(bg2e::db::countMeshTextures(scene / "invalid.bg2") == 0, "Loader failure contract changed");
    mustThrow([&]
    {
        bg2e::db::meshTexturePaths(scene / "invalid.bg2");
    });

    write(scene / "orphan.txt", "unused");
    std::error_code linkError;
    fs::create_directory_symlink(base, scene / "outside-link", linkError);
    if (!linkError)
    {
        analysis = Manager::analyze(scene);
        require(analysis.symbolicLinks.size() == 1, "Directory symlink was traversed or omitted");
        result = Manager::executeCleanup(Manager::createCleanupPlan(analysis, true));
        require(result.errors.empty() && fs::is_symlink(scene / "outside-link") &&
            fs::exists(base / "external.hdr"), "Cleanup followed or removed a symbolic link");
    }
}
}

int main(int argc, char** argv)
{
    try
    {
        if (argc != 2 || !fs::is_directory(argv[1]))
        {
            throw std::runtime_error("Usage: scene_resources_probe EXISTING_DISPOSABLE_DIRECTORY");
        }
        const auto token = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto directory = fs::absolute(argv[1]) / ("scene-resources-" + std::to_string(token));
        require(fs::create_directory(directory), "Probe directory already exists");
        std::cout << "Fixtures: " << directory << '\n';
        run(directory);
        std::cout << "Scene resource checks passed (fixtures retained).\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
