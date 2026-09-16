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

#include <bg2e/common.hpp>

#include <cstddef>
#include <string>

namespace bg2e {
namespace ui {

// Wrapper for ImGui drag & drop. Payloads are identified by a string type
// and carry a raw memory block of payloadSize bytes copied on set/accept.
class BG2E_API DragDrop {
public:
    // Starts a drag source on the last submitted item. Returns true while a
    // drag is in progress; in that case endSource() must be called after
    // drawing the drag preview (see setPreviewText()). The payload data is
    // copied at this point.
    static bool beginSource(const std::string & payloadType,
                            const void * payloadData, size_t payloadSize,
                            bool disabled = false);
    static void endSource();

    // Starts a drop target on the last submitted item. Returns true (and
    // copies the payload into payloadData) on the frame the drop happens.
    // The payload is only accepted if its size matches payloadSize.
    static bool acceptPayload(const std::string & payloadType,
                              void * payloadData, size_t payloadSize);

    // Draws a preview text inside an active drag source (between
    // beginSource() and endSource()).
    static void setPreviewText(const std::string & text);
};

}
}
