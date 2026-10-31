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

#include <bg2e/ui/EditInteraction.hpp>

#include <imgui.h>
#include <atomic>
#include <stdexcept>
#include <utility>

namespace bg2e::ui {
namespace {
std::atomic<uint64_t> nextInteraction{1};
}

EditInteractionState EditInteraction::draw(const std::function<void()>& controls)
{
    ImGui::BeginGroup();
    struct GroupScope {
        bool ended = false;
        ~GroupScope()
        {
            if (!ended)
            {
                ImGui::EndGroup();
            }
        }
    } group;
    controls();
    ImGui::EndGroup();
    group.ended = true;
    // Keep popup editing open across releases and switches between internal
    // controls. Public ImGui popup queries are level-wide, so a sibling popup
    // may conservatively delay commit until it closes (no time-based debounce).
    const bool popup = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);
    return {ImGui::IsItemActive() || popup,
        ImGui::IsKeyPressed(ImGuiKey_Escape, false) ||
        ImGui::GetIO().AppFocusLost};
}


EditSession::EditSession(Handler handler) : _handler(std::move(handler))
{
    if (!_handler)
    {
        throw std::invalid_argument("Edit session requires an intent handler");
    }
}


void EditSession::beginFrame()
{
    _seen = false;
    if (_cancelling)
    {
        cancel();
    }
}


void EditSession::endFrame()
{
    if (_active && !_seen)
    {
        cancel();
    }
}


void EditSession::cancel()
{
    if (_active)
    {
        _cancelling = true;
        // Keep ownership until the receiver accepts restoration. The host must
        // retain this session and retry settlement if cancellation fails.
        if (!_handler(EditPhase::Cancel, _interaction, _key, _value))
        {
            return;
        }
        _active = false;
        _cancelling = false;
        _key.clear();
        _value.reset();
    }
}


bool EditSession::commit()
{
    if (_cancelling)
    {
        cancel();
        return !_active;
    }
    if (!_active)
    {
        return true;
    }
    if (!_handler(EditPhase::Commit, _interaction, _key, _value))
    {
        return false;
    }
    _active = false;
    _key.clear();
    _value.reset();
    return true;
}


bool EditSession::active() const
{
    return _active;
}


std::any EditSession::value(const std::string& key, std::any current) const
{
    return _active && _key == key ? _value : std::move(current);
}


bool EditSession::submit(const std::string& key, std::any proposed, bool changed, EditInteractionState state)
{
    if (_cancelling)
    {
        cancel();
        return false;
    }
    if (_active && _key == key)
    {
        _seen = true;
        if (state.cancel)
        {
            cancel();
            return false;
        }
    }
    if (changed && !state.cancel)
    {
        if (_active && _key != key)
        {
            // A different property is a distinct gesture. Commit the previous
            // one before admission; never interleave two live mutation owners.
            if (!_handler(EditPhase::Commit, _interaction, _key, _value))
            {
                return false;
            }
            _active = false;
        }
        if (!_active)
        {
            // Allocate/copy before asking the receiver to open a transaction.
            _key = key;
            _value = proposed;
            _interaction = nextInteraction.fetch_add(1);
            if (!_handler(EditPhase::Begin, _interaction, _key, _value))
            {
                _key.clear();
                _value.reset();
                return false;
            }
            _active = true;
        }
        _seen = true;
        if (!_handler(EditPhase::Update, _interaction, _key, proposed))
        {
            cancel();
            return false;
        }
        _value = std::move(proposed);
    }
    if (_active && _key == key && !state.active)
    {
        if (_handler(EditPhase::Commit, _interaction, _key, _value))
        {
            _active = false;
            _key.clear();
            _value.reset();
        }
    }
    return changed && !state.cancel;
}

}
