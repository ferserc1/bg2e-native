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
#include <any>
#include <cstdint>
#include <functional>
#include <string>

namespace bg2e::ui {

enum class EditPhase { Begin, Update, Commit, Cancel };

struct EditInteractionState {
    bool active = false;
    bool cancel = false;
};

// Groups compound controls, including their popup, into one interaction.
// Invoke around transient-value widgets only; no live mutation occurs here.
class BG2E_API EditInteraction {
  public:
    static EditInteractionState draw(const std::function<void()>& controls);
};

// One optional session per editor/target. The host brackets every visible frame
// with beginFrame/endFrame, and calls cancel before rebinding or destruction.
// A missing control cancels its preview. Callbacks own all live mutations and
// return false on rejected admission/update; a rejected Begin receives no Cancel.
// Handlers must not throw; a rejected Begin must leave the model unchanged.
// Pending/failed cancellation is retried on subsequent frames; updates and
// commit cannot overtake it. Payloads own CPU values, never borrowed pointers.
class BG2E_API EditSession {
  public:
    using Handler = std::function<bool(EditPhase, uint64_t, const std::string&, const std::any&)>;

    explicit EditSession(Handler handler);
    EditSession(const EditSession&) = delete;
    EditSession& operator=(const EditSession&) = delete;

    void beginFrame();
    void endFrame();
    void cancel();
    bool commit();
    bool active() const;
    std::any value(const std::string& key, std::any current) const;
    bool submit(const std::string& key, std::any proposed, bool changed, EditInteractionState state);

  private:
    Handler _handler;
    std::string _key;
    std::any _value;
    uint64_t _interaction = 0;
    bool _active = false;
    bool _cancelling = false;
    bool _seen = false;
};

}
