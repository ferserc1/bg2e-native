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

#include <bg2e/db/image.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bg2e {
namespace render {
struct LightmapPixels;
}
namespace scene {
class Node;
}
namespace db {

class BG2E_API LightmapOutputWriter final {
public:
    struct Target {
        std::string identity;
        std::filesystem::path inputModelPath;
        std::shared_ptr<scene::Node> node;
    };

    struct OutputPaths {
        std::string identity;
        std::filesystem::path imagePath;
        std::optional<std::filesystem::path> modelPath;
    };

    LightmapOutputWriter(std::filesystem::path outputDirectory, ImageFormat format);
    ~LightmapOutputWriter();

    // When enabled, preflight() accepts output paths that already exist and
    // write() atomically replaces them. Disabled by default.
    void setOverwriteExisting(bool overwrite);

    LightmapOutputWriter(const LightmapOutputWriter&) = delete;
    LightmapOutputWriter& operator=(const LightmapOutputWriter&) = delete;
    LightmapOutputWriter(LightmapOutputWriter&&) noexcept;
    LightmapOutputWriter& operator=(LightmapOutputWriter&&) noexcept;

    // Call once before baking. This validates the complete batch's outputs,
    // target-scene textures, caller-supplied source assets, existing paths,
    // and any .bg2 texture sidecars. Include context/prefab files and non-target
    // .bg2 assets in protectedInputPaths.
    [[nodiscard]] std::vector<OutputPaths> preflight(
        const std::vector<Target>& targets,
        bool writeModelCopies,
        const std::vector<std::filesystem::path>& protectedInputPaths = {});

    // Writes the preflighted target's image, and its .bg2 copy when requested.
    // The identity must match one target supplied to preflight().
    void write(const render::LightmapPixels& pixels, std::string_view targetIdentity);

    [[nodiscard]] const std::filesystem::path& outputDirectory() const;

private:
    struct State;
    std::unique_ptr<State> _state;
};

}
}
