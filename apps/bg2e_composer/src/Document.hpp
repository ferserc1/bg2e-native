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

#include "command/Dispatcher.hpp"
#include <bg2e/scene/Node.hpp>
#include <stdexcept>
#include <cstdint>
#include <filesystem>
#include <memory>

namespace bg2e_composer {

// UI-facing state changes only after an IO operation commits successfully.
struct Document {
    std::shared_ptr<bg2e::scene::Node> root;
    std::filesystem::path sourcePath;
    std::filesystem::path savePath;
    std::shared_ptr<command::Dispatcher> commands;
    bool dirty = false;
    uint64_t generation = 0;

    void synchronizeDirty()
    {
        if (commands)
        {
            dirty = commands->history().dirty();
        }
    }

    // Transitional authored edits invalidate undo/redo until migrated. Persisted
    // navigation explicitly opts into Preserve and only advances its revision.
    void markModified(bg2e::app::CommandCompatibility compatibility = bg2e::app::CommandCompatibility::ResetHistory)
    {
        if (commands)
        {
            const auto result = commands->history().noteNonhistoricalChange(compatibility);
            synchronizeDirty();
            if (!result.accepted() && result.status != bg2e::app::CommandStatus::Faulted)
            {
                throw std::logic_error("Settle command interactions before legacy document edits");
            }
        }
        else
        {
            dirty = true;
        }
    }
};

}
