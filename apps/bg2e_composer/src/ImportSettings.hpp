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

#include <bg2e/app/Preferences.hpp>

#include <cstdint>
#include <string>

// Typed wrapper over bg2e::app::Preferences("import") persisting the HTTP
// scene import service settings (TCP port and enable flag) in
// preferences_import.json.
class ImportSettings {
public:
    static constexpr uint32_t DefaultPort = 8643;
    static constexpr uint32_t MinPort = 1024;    // below: privileged
    static constexpr uint32_t MaxPort = 49151;   // above: ephemeral/dynamic range

    ImportSettings();   // constructs Preferences("import"), does NOT load

    void load();
    void save();

    uint32_t port() const;
    void setPort(uint32_t port);

    bool serviceEnabled() const;        // default: true
    void setServiceEnabled(bool enabled);

    // Numeric string typed in the UI -> validated port.
    // Returns false when empty, non-numeric, or outside [MinPort, MaxPort].
    static bool parsePort(const std::string& text, uint32_t& outPort);

private:
    bg2e::app::Preferences _prefs { "import" };
};
