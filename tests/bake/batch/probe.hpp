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

#include <bg2e/bake/StandaloneBakeBatch.hpp>
#include <bg2e/db/file.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/db/texture_archive.hpp>
#include <bg2e/scene/Node.hpp>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace bg2e::bake::tests {
inline void batchRequire(bool value, const char* message)
{
    if (!value)
    {
        throw std::runtime_error(message);
    }
}

template <typename F> void batchReject(F operation)
{
    bool rejected = false;
    try
    {
        operation();
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    batchRequire(rejected, "Invalid batch operation was accepted");
}

inline std::string batchFileBytes(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    batchRequire(bool(input), "Cannot read probe artifact");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

// CPU IO probe. directory must be a fresh, caller-owned scratch directory.
inline void runBakePublicationProbe(const std::filesystem::path& directory)
{
    std::filesystem::create_directories(directory);
    const auto pending = directory / "pending.txt";
    const auto destination = directory / "published.txt";
    {
        std::ofstream(pending) << "first";
    }
    db::publishFile(pending, destination);
    batchRequire(batchFileBytes(destination) == "first" && !std::filesystem::exists(pending),
        "No-replace publication failed");
    {
        std::ofstream(pending) << "second";
    }
    batchReject([&]
    {
        db::publishFile(pending, destination);
    });
    batchRequire(batchFileBytes(destination) == "first" && batchFileBytes(pending) == "second",
        "No-replace publication damaged existing bytes");
    db::publishFile(pending, destination, true);
    batchRequire(batchFileBytes(destination) == "second", "Atomic replacement failed");
    db::TextureArchive archive;
    archive.description.size = {1, 1};
    archive.description.format = gpu::PixelFormat::R32G32B32A32_SFLOAT;
    const float values[]{0.0f, 100000.0f, 2.5f, 1.0f};
    archive.subresources.emplace_back(sizeof(values));
    std::memcpy(archive.subresources[0].data(), values, sizeof(values));
    const auto texture = directory / "hdr.bg2tex";
    db::writeTextureArchive(texture, archive);
    batchRequire(db::readTextureArchive(texture).subresources == archive.subresources,
        "CPU archive did not preserve exact HDR bits");
    archive.subresources[0].pop_back();
    batchReject([&]
    {
        db::writeTextureArchive(directory / "invalid.bg2tex", archive);
    });
    batchRequire(!std::filesystem::exists(directory / "invalid.bg2tex"),
        "Malformed archive created a file");
}

// Stopped offscreen Engine, loaded plain UV2 target and existing source model.
// directory is fresh scratch OUTSIDE all source assets. No bake shader is used.
inline void runLightmapOutputProbe(draw::Engine& engine, std::shared_ptr<scene::Node> target,
    const std::filesystem::path& inputModel, const std::filesystem::path& directory)
{
    LightmapSettings settings;
    settings.setResolution(2);
    settings.setDilationPixels(0);
    settings.setCpuFormat(LightmapPixelFormat::RGB32F);
    const std::vector<LightmapOutputWriter::Target> targets{{"mesh/0", inputModel, target}};
    LightmapPixels pixels{2, 2, LightmapPixelFormat::RGB32F,
        std::vector<float>{0, 100000, 2.5f, 1, 2, 3, 4, 5, 6, 7, 8, 9}};
    const auto sourceBytes = batchFileBytes(inputModel);
    const auto originalMode = target->drawable()->drawDrawable()->material().indirectTextureMode();
    const auto originalTexture = target->drawable()->drawDrawable()->material().aoTexture();
    std::vector<LightmapOutputWriter::OutputPaths> outputs;
    {
        LightmapOutputWriter writer(engine, directory, settings);
        outputs = writer.preflight(targets, true);
        batchReject([&]
        {
            writer.commit();
        });
        writer.write(pixels, "mesh/0");
        batchRequire(!std::filesystem::exists(writer.manifestPath()) &&
            !std::filesystem::exists(outputs[0].imagePath), "Staging published an incomplete batch");
        batchReject([&]
        {
            writer.write(pixels, "mesh/0");
        });
        writer.commit();
        batchRequire(std::filesystem::exists(writer.manifestPath()), "Missing complete manifest");
        batchReject([&]
        {
            writer.commit();
        });
    }
    batchRequire(batchFileBytes(inputModel) == sourceBytes &&
        target->drawable()->drawDrawable()->material().indirectTextureMode() == originalMode &&
        target->drawable()->drawDrawable()->material().aoTexture() == originalTexture,
        "Export mutated a source asset/material");
    const auto image = db::readTextureArchive(outputs[0].imagePath);
    float hdr;
    std::memcpy(&hdr, image.subresources[0].data() + sizeof(float), sizeof(float));
    batchRequire(hdr == 100000.0f, "Writer clamped HDR");
    auto model = db::loadDrawableBg2(*outputs[0].modelPath, engine);
    const auto& m = model->material();
    batchRequire(m.aoUVSet() == 1 && m.aoTexture()->colorType() == base::Color::TypeLinear &&
        m.indirectTextureMode() == base::MaterialAttributes::IndirectTextureMode::BakedGlobalIllumination,
        "Exported model lost UV2/linear/baked GI semantics");
    const auto nativeMap = db::readTextureArchive(m.aoTexture()->imageFilePath());
    batchRequire(nativeMap.subresources == image.subresources, "Model did not embed actual HDR pixels");
    const auto oldManifest = batchFileBytes(directory / "lightmaps.json");
    batchReject([&]
    {
        LightmapOutputWriter writer(engine, directory, settings);
        writer.preflight(targets, false);
    });
    batchReject([&]
    {
        LightmapOutputWriter writer(engine, directory, settings);
        writer.setOverwriteExisting(true);
        writer.preflight(targets, false, {directory / "lightmaps.json"});
    });
    {
        LightmapOutputWriter writer(engine, directory, settings);
        writer.setOverwriteExisting(true);
        writer.preflight(targets, false);
        writer.write(pixels, "mesh/0");
        // Abandon a fully staged replacement: the old manifest must survive.
    }
    batchRequire(batchFileBytes(directory / "lightmaps.json") == oldManifest &&
        std::filesystem::exists(outputs[0].imagePath), "Abandoned replacement damaged prior output");
    batchReject([&]
    {
        LightmapOutputWriter writer(engine, directory / "duplicates", settings);
        writer.preflight({targets[0], targets[0]}, false);
    });
    const auto name = target->name();
    {
        LightmapOutputWriter writer(engine, directory / "stale", settings);
        writer.preflight(targets, false);
        target->setName(name + " changed");
        batchReject([&]
        {
            writer.write(pixels, "mesh/0");
        });
        target->setName(name);
    }
    settings.setCpuFormat(LightmapPixelFormat::RGB8);
    LightmapOutputWriter bytes(engine, directory / "bytes", settings);
    const auto bytePaths = bytes.preflight(targets, false);
    bytes.write({2, 2, LightmapPixelFormat::RGB8,
        std::vector<uint8_t>{0, 255, 128, 1, 2, 3, 4, 5, 6, 7, 8, 9}}, "mesh/0");
    bytes.commit(); // Writer itself decodes/verifies every lossless RGB byte.
    batchRequire(bytePaths[0].imagePath.extension() == ".png", "Unexpected byte encoding");
}
}
