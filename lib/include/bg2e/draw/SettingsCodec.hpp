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
