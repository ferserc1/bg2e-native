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
#include "ImportSettings.hpp"

#include <algorithm>
#include <cctype>

ImportSettings::ImportSettings() = default;

void ImportSettings::load()
{
    _prefs.load();
}

void ImportSettings::save()
{
    _prefs.save();
}

uint32_t ImportSettings::port() const
{
    return _prefs.get<uint32_t>("port", DefaultPort);
}

void ImportSettings::setPort(uint32_t port)
{
    _prefs.set<uint32_t>("port", port);
}

bool ImportSettings::serviceEnabled() const
{
    return _prefs.get<bool>("serviceEnabled", true);
}

void ImportSettings::setServiceEnabled(bool enabled)
{
    _prefs.set<bool>("serviceEnabled", enabled);
}

bool ImportSettings::parsePort(const std::string& text, uint32_t& outPort)
{
    if (text.empty() ||
        !std::all_of(text.begin(), text.end(),
                     [](unsigned char c){ return std::isdigit(c); }))
    {
        return false;
    }
    try
    {
        unsigned long v = std::stoul(text);
        if (v < MinPort || v > MaxPort)
        {
            return false;
        }
        outPort = static_cast<uint32_t>(v);
        return true;
    }
    catch (...)
    {
        return false;   // out_of_range on absurdly long input
    }
}
