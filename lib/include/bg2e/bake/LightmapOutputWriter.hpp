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

#include <bg2e/bake/LightmapSettings.hpp>
#include <bg2e/bake/LightmapPixels.hpp>
#include <bg2e/db/image.hpp>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace bg2e::draw { class Engine; }
namespace bg2e::scene { class Node; class Component; }
namespace bg2e::bake {
// A resolver must account for ALL external inputs of an unrecognized component.
// Enumerate files, not directory placeholders. nullopt rejects the job; an
// explicit empty list declares no external resources.
// It is a read-only CPU callback, called with stopped scene/resource producers.
using BakeDependencyResolver = std::function<std::optional<std::vector<std::filesystem::path>>(
    const scene::Component&)>;

// One complete generation staged privately. commit() publishes lightmaps.json
// atomically LAST; readers resolve all paths through that manifest. Previous
// generations remain intact, including after overwrite, cancel or staging error.
class BG2E_API LightmapOutputWriter final {
  public:
    struct Target {
        std::string identity;
        std::filesystem::path inputModelPath;
        std::shared_ptr<scene::Node> node;
    };
    struct OutputPaths {
        std::string identity;
        std::filesystem::path imagePath, metadataPath;
        std::optional<std::filesystem::path> modelPath;
    };

    LightmapOutputWriter(draw::Engine& engine, std::filesystem::path outputDirectory,
        LightmapSettings settings, db::ImageFormat byteFormat = db::ImageFormat::PNG);
    ~LightmapOutputWriter();
    LightmapOutputWriter(const LightmapOutputWriter&) = delete;
    LightmapOutputWriter& operator=(const LightmapOutputWriter&) = delete;

    void setOverwriteExisting(bool overwrite);
    std::vector<OutputPaths> preflight(const std::vector<Target>& targets, bool writeModelCopies,
        const std::vector<std::filesystem::path>& protectedInputs = {}, BakeDependencyResolver resolver = {});
    // Stage only; no target is visible through the committed manifest yet.
    void write(const LightmapPixels& pixels, const std::string& identity);
    void validateSources() const;
    void commit(); // Requires every preflighted target staged once; one-shot.

    const std::filesystem::path& manifestPath() const;
    const std::vector<OutputPaths>& outputs() const;

  private:
    struct State;
    std::unique_ptr<State> _state;
};
}
