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

#include <bg2e/scene/Node.hpp>
#include <cstdint>
#include <filesystem>
#include <memory>

namespace model_edit {

// UI-facing state changes only after an IO operation commits successfully.
struct Document {
    std::shared_ptr<bg2e::scene::Node> root;
    std::filesystem::path sourcePath;
    std::filesystem::path savePath;
    bool dirty = false;
    uint64_t generation = 0;

    void markModified()
    {
        dirty = true;
    }
};

}
