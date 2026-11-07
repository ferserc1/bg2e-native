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

    // A legacy writer may already have changed the model before notification.
    // Keep its dirty state until the history reaches a safe boundary.
    bg2e::app::CommandCompatibility pendingCompatibility = bg2e::app::CommandCompatibility::Unspecified;

    void synchronizeDirty()
    {
        using namespace bg2e::app;
        if (commands)
        {
            auto& history = commands->history();
            if (pendingCompatibility != CommandCompatibility::Unspecified)
            {
                const auto result = history.noteNonhistoricalChange(pendingCompatibility);
                if (result.accepted())
                {
                    pendingCompatibility = CommandCompatibility::Unspecified;
                }
            }
            dirty = history.dirty() || pendingCompatibility != CommandCompatibility::Unspecified;
        }
    }

    // Persisted nonhistorical changes require an explicit compatibility policy.
    // Rejection is recoverable; never turn a busy history into an exception.
    bg2e::app::CommandResult markModified(bg2e::app::CommandCompatibility compatibility)
    {
        using namespace bg2e::app;
        if (compatibility == CommandCompatibility::Unspecified)
        {
            return {CommandStatus::Rejected, "Legacy edits require an explicit compatibility policy"};
        }
        if (!commands)
        {
            dirty = true;
            return {CommandStatus::Applied, {}};
        }
        // Coalesce pending notifications using the strongest invalidation.
        if (compatibility > pendingCompatibility)
        {
            pendingCompatibility = compatibility;
        }
        const auto result = commands->history().noteNonhistoricalChange(pendingCompatibility);
        if (result.accepted())
        {
            pendingCompatibility = CommandCompatibility::Unspecified;
        }
        dirty = commands->history().dirty() || pendingCompatibility != CommandCompatibility::Unspecified;
        return result;
    }
};

}
