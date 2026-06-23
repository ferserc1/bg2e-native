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

#include <bg2e/base/Timeout.hpp>

#include <algorithm>

namespace bg2e {
namespace base {

uint32_t Timeout::add(std::function<bool()> callback, uint32_t delayMs, bool executeOnExit)
{
    uint32_t id = _nextId++;
    _timers.push_back({
        id,
        std::move(callback),
        delayMs,
        executeOnExit,
        std::chrono::steady_clock::now()
    });
    return id;
}

void Timeout::cancel(uint32_t id)
{
    _timers.erase(
        std::remove_if(_timers.begin(), _timers.end(),
            [id](const TimerEntry& e) { return e.id == id; }),
        _timers.end()
    );
}

void Timeout::executeTimers()
{
    auto now = std::chrono::steady_clock::now();
    for (size_t i = 0; i < _timers.size(); )
    {
        auto& entry = _timers[i];
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - entry.lastFireTime).count();

        if (static_cast<uint32_t>(elapsed) >= entry.delayMs)
        {
            bool keepAlive = entry.callback();
            if (keepAlive)
            {
                entry.lastFireTime = now;
                ++i;
            }
            else
            {
                _timers.erase(_timers.begin() + i);
            }
        }
        else
        {
            ++i;
        }
    }
}

void Timeout::executeExitTimers()
{
    for (size_t i = 0; i < _timers.size(); )
    {
        if (_timers[i].executeOnExit)
        {
            _timers[i].callback();
            _timers.erase(_timers.begin() + i);
        }
        else
        {
            ++i;
        }
    }
}

}
}
