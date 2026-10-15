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

#include <bg2e/draw/SettingsCatalog.hpp>
#include <bg2e/json/JsonNode.hpp>
#include <filesystem>

namespace bg2e::draw {
enum class UnknownSettingPolicy
{
    Reject,
    SkipUnknownOrUnavailable
};
// Transient validated command payload, not a preferences value model.
class BG2E_API SettingsImport {
  public:
    const SettingsResult& result() const;
    SettingsResult apply(SettingsCatalog& catalog) const;
  private:
    friend class SettingsCodec;
    uint64_t _generation = 0;
    SettingsResult _result;
    std::vector<SettingChange> _changes;
};
class BG2E_API SettingsCodec {
  public:
    static std::shared_ptr<json::JsonNode> serialize(const SettingsCatalog& catalog);
    static SettingsImport prepare(const SettingsCatalog& catalog, std::vector<SettingChange> changes);
    static SettingsImport validate(const json::JsonNode& document, const SettingsCatalog& catalog,
        UnknownSettingPolicy policy = UnknownSettingPolicy::Reject);
    // Strict settings JSON grammar; numeric payloads are typed decimal strings
    // because JsonNode stores ordinary JSON numbers as float.
    static std::shared_ptr<json::JsonNode> parse(const std::string& text);
    static std::string stringify(const json::JsonNode& document);
    static std::shared_ptr<json::JsonNode> readFile(const std::filesystem::path& path);
    static void exportFile(const SettingsCatalog& catalog, const std::filesystem::path& path);
};
}
