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

#include <bg2e/app/Command.hpp>
#include <any>

namespace bg2e::app {

// One command representation for preview, adoption, undo and redo. The caller
// begins/updates/commits/cancels through CommandHistory at its safe boundary.
class BG2E_API CommandTransaction : public Command {
  public:
    // Capture before state once; successful begin returns NoOp, never mutates.
    virtual CommandResult begin() noexcept = 0;
    // Full upper bound for the resulting preview AND before/redo/recovery state.
    // Includes update payload ownership until the update completes.
    virtual CommandMemory memoryForUpdate(const std::any& value) const = 0;

    // Failed update restores the preceding preview, not the original baseline.
    virtual CommandResult update(const std::any& value) noexcept = 0;
    virtual bool changed() const noexcept = 0;
};

}
