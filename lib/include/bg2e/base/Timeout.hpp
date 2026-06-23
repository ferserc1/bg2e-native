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
#include <functional>
#include <vector>
#include <cstdint>
#include <chrono>

namespace bg2e {
namespace base {

class BG2E_API Timeout {
public:
    // Adds a timer. callback fires after delayMs. Returns timer ID.
    // If executeOnExit is true, callback fires once more at main loop exit
    // regardless of elapsed time.
    uint32_t add(std::function<bool()> callback, uint32_t delayMs, bool executeOnExit = false);

    void cancel(uint32_t id);

    // Call once per frame. Fires due timers.
    void executeTimers();

    // Call once at main loop exit. Fires all executeOnExit timers.
    void executeExitTimers();

private:
    struct TimerEntry {
        uint32_t id;
        std::function<bool()> callback;
        uint32_t delayMs;
        bool executeOnExit;
        std::chrono::steady_clock::time_point lastFireTime;
    };

    std::vector<TimerEntry> _timers;
    uint32_t _nextId = 1;
};

}
}
