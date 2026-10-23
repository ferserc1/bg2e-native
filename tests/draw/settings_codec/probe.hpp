/*
 *    business grade graphic engine (bg2e engine)
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

#include <bg2e/draw/SettingsCodec.hpp>
#include <bg2e/draw/pipeline/TemporalAccumulator.hpp>
#include <bg2e/reflection/Builder.hpp>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace bg2e::draw::tests {
namespace settings_probe {
enum class Mode : int64_t
{
    First = 10, Second = 9007199254740993LL
};
struct Values {
    float exposure = 1;
    double precise = 1.0000000000000002;
    uint32_t count = std::numeric_limits<uint32_t>::max();
    int32_t signedCount = -123;
    bool enabled = true;
    Mode mode = Mode::Second;
    std::filesystem::path path = std::filesystem::path("folder/quoted\"\\name\n\t\xc3\xb1.hdr");
    glm::vec2 v2{1, 2};
    glm::vec3 v3{3, 4, 5};
    glm::vec4 v4{6, 7, 8, 9};
    glm::mat4 matrix{1};
    base::Color color{0.25f, 0.5f, 0.75f, 1};
    bool optionalAvailable = true;
    bool failSetter = false;
    int writes = 0;
};
inline void require(bool condition)
{
    if (!condition)
    {
        throw std::runtime_error("Settings codec probe failed");
    }
}
template <typename V>
void property(reflection::TypeInfoBuilder<Values>& builder, const char* name, V Values::* member)
{
    builder.accessor<V>(name, [member](const Values& object)
    {
        return object.*member;
    }, [member](Values& object, V value)
    {
        object.*member = std::move(value);
        ++object.writes;
    });
}
inline reflection::TypeInfo metadata()
{
    reflection::TypeInfoBuilder<Values> builder("test.draw.settings.values.v1");
    builder.accessor<float>("exposure", [](const Values& object)
    {
        return object.exposure;
    }, [](Values& object, float value)
    {
        object.exposure = value; // Exercise rollback even after a partially mutating setter.
        ++object.writes;
        if (object.failSetter && value == 7)
        {
            throw std::invalid_argument("Intentional probe setter failure");
        }
    }).range(-10, 10).category("Color");
    property(builder, "precise", &Values::precise);
    property(builder, "count", &Values::count);
    property(builder, "signedCount", &Values::signedCount);
    property(builder, "enabled", &Values::enabled);
    builder.accessor<Mode>("mode", [](const Values& object)
    {
        return object.mode;
    }, [](Values& object, Mode value)
    {
        object.mode = value;
        ++object.writes;
    }).enumValue("First", Mode::First).enumValue("Second", Mode::Second);
    property(builder, "path", &Values::path);
    property(builder, "v2", &Values::v2);
    property(builder, "v3", &Values::v3);
    property(builder, "v4", &Values::v4);
    property(builder, "matrix", &Values::matrix);
    property(builder, "color", &Values::color);
    return builder.build();
}
inline void bind(SettingsCatalog& catalog, Values& values)
{
    catalog.reset("test.draw.renderer.v1");
    SettingsTarget target;
    target.id = "values";
    target.type = metadata();
    target.instance = &values;
    target.available = [&values](const std::string& name)
    {
        return name != "count" || values.optionalAvailable;
    };
    target.validate = [](const std::map<std::string, std::any>& proposed)
    {
        if (std::any_cast<float>(proposed.at("exposure")) > std::any_cast<double>(proposed.at("precise")))
        {
            throw std::invalid_argument("Exposure must not exceed precise budget");
        }
    };
    catalog.add(std::move(target));
}
inline std::shared_ptr<json::JsonNode> single(const std::string& key, const std::string& type,
    std::shared_ptr<json::JsonNode> value)
{
    using json::JSON;
    using json::JsonObject;
    return JSON(JsonObject{{"schema", JSON("bg2e.draw.settings")}, {"version", JSON("1")},
        {"renderer", JSON("test.draw.renderer.v1")},
        {"components", JSON(json::JsonList{JSON(JsonObject{{"id", JSON("values")},
            {"type", JSON("test.draw.settings.values.v1")},
            {"properties", JSON(JsonObject{{key, JSON(JsonObject{{"type", JSON(type)}, {"value", value}})}})}})})}});
}
}
// No GPU initialization: validates real temporal setters/reflection/codec contracts.
inline void runTemporalAdaptiveSettingsCodecProbe()
{
    using settings_probe::require;
    pipeline::TemporalAccumulator temporal;
    require(!temporal.settings().adaptiveEnabled && temporal.settings().minCurrentWeight == 0.03125f &&
        temporal.settings().reactiveCurrentWeight == 0.25f && temporal.settings().historyClipSigma == 3);
    const auto initial = temporal.generation();
    temporal.setAdaptiveSettings(false,0.03125f,0.25f,3);
    require(temporal.generation() == initial);
    temporal.setAdaptiveSettings(true,0.1f,0.2f,4);
    require(temporal.generation() == initial+1);
    SettingsCatalog catalog;
    catalog.reset("test.temporal");
    SettingsTarget target;
    target.id = "temporal";
    target.type = temporal.settingsType();
    target.instance = temporal.settingsInstance();
    target.validate = [](const std::map<std::string,std::any>& values)
    {
        pipeline::TemporalAccumulator::validateAdaptiveSettings(std::any_cast<bool>(values.at("adaptiveEnabled")),
            std::any_cast<float>(values.at("minCurrentWeight")),std::any_cast<float>(values.at("reactiveCurrentWeight")),
            std::any_cast<float>(values.at("historyClipSigma")));
    };
    catalog.add(std::move(target));
    const auto minimum = catalog.handle("temporal","minCurrentWeight");
    const auto reactive = catalog.handle("temporal","reactiveCurrentWeight");
    const auto sigma = catalog.handle("temporal","historyClipSigma");
    // Both property orders must support the entire valid range, including rollback staging.
    require(SettingsCodec::prepare(catalog,{{minimum,0.8f},{reactive,0.9f}}).apply(catalog).applied);
    require(temporal.settings().minCurrentWeight == 0.8f && temporal.settings().reactiveCurrentWeight == 0.9f);
    require(SettingsCodec::prepare(catalog,{{reactive,0.2f},{minimum,0.1f}}).apply(catalog).applied);
    const auto saved = SettingsCodec::stringify(*SettingsCodec::serialize(catalog));
    temporal.setAdaptiveSettings(false,0,1,0);
    require(SettingsCodec::validate(*SettingsCodec::parse(saved),catalog).apply(catalog).applied);
    require(temporal.settings().adaptiveEnabled && temporal.settings().minCurrentWeight == 0.1f &&
        temporal.settings().reactiveCurrentWeight == 0.2f && temporal.settings().historyClipSigma == 4);
    const auto before = temporal.generation();
    for (const auto& changes : std::vector<std::vector<SettingChange>>{
        {{minimum,0.8f},{reactive,0.2f}}, {{minimum,-0.1f}}, {{reactive,1.1f}}, {{sigma,10.1f}},
        {{sigma,std::numeric_limits<float>::infinity()}}, {{minimum,std::numeric_limits<float>::quiet_NaN()}}})
    {
        const auto plan = SettingsCodec::prepare(catalog,changes);
        require(!plan.result().valid() && !plan.apply(catalog).applied && temporal.generation() == before);
    }
    bool rejected = false;
    try
    {
        temporal.setAdaptiveSettings(true,0.8f,0.2f,3);
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }
    require(rejected && temporal.generation() == before);
    // Legacy payload omits all new fields. A fresh target retains their defaults.
    pipeline::TemporalAccumulator legacy;
    SettingsCatalog legacyCatalog;
    legacyCatalog.reset("test.temporal");
    SettingsTarget legacyTarget;
    legacyTarget.id = "temporal";
    legacyTarget.type = legacy.settingsType();
    legacyTarget.instance = &legacy;
    legacyCatalog.add(std::move(legacyTarget));
    const auto oldPayload = SettingsCodec::parse(R"({"schema":"bg2e.draw.settings","version":"1","renderer":"test.temporal","components":[{"id":"temporal","type":"bg2e.draw.temporal.v1","properties":{"enabled":{"type":"bool","value":true}}}]})");
    require(SettingsCodec::validate(*oldPayload,legacyCatalog).apply(legacyCatalog).applied);
    require(!legacy.settings().adaptiveEnabled && legacy.settings().minCurrentWeight == 0.03125f &&
        legacy.settings().reactiveCurrentWeight == 0.25f && legacy.settings().historyClipSigma == 3);
}

// CPU-only manual fixture. Does not initialize Engine, window, UI, preferences or GPU.
inline void runSettingsCodecProbe()
{
    runTemporalAdaptiveSettingsCodecProbe();
    using namespace settings_probe;
    // Type-only validation must accept false/empty values and reject other
    // stored types (including an empty any) without implicit conversions.
    reflection::PropertyInfo typed;
    typed.name = "typeOnly";
    const auto expectType = [&](reflection::PropertyType type, const std::any& valid, const std::any& invalid)
    {
        typed.type = type;
        SettingsCatalog::validateValue(typed, valid);
        for (const auto& value : {invalid, std::any{}})
        {
            bool rejected = false;
            try
            {
                SettingsCatalog::validateValue(typed, value);
            }
            catch (const std::bad_any_cast&)
            {
                rejected = true;
            }
            require(rejected);
        }
    };
    expectType(reflection::PropertyType::Bool, false, int32_t(0));
    expectType(reflection::PropertyType::String, std::string{}, std::filesystem::path{});
    expectType(reflection::PropertyType::Path, std::filesystem::path{}, std::string{});
    typed.metadata.resourceValueType = reflection::PropertyType::String;
    expectType(reflection::PropertyType::Resource, std::string{}, std::filesystem::path{});
    typed.metadata.resourceValueType = reflection::PropertyType::Path;
    expectType(reflection::PropertyType::Resource, std::filesystem::path{}, std::string{});

    Values values;
    SettingsCatalog catalog;
    bind(catalog, values);
    const auto exposure = catalog.handle("values", "exposure");
    values.exposure = 0.5f;
    values.matrix[2][1] = 12.5f;
    require(std::any_cast<float>(catalog.read(exposure)) == 0.5f);
    const auto savedPath = values.path;
    const auto document = SettingsCodec::serialize(catalog);
    const auto wire = SettingsCodec::stringify(*document);
    require(wire.find("9007199254740993") != std::string::npos);
    require(wire.find("4294967295") != std::string::npos);
    values.precise = 2;
    values.count = 1;
    values.mode = Mode::First;
    values.path.clear();
    values.v2 = glm::vec2(0);
    values.v3 = glm::vec3(0);
    values.v4 = glm::vec4(0);
    values.matrix = glm::mat4(0);
    values.color = base::Color(0, 0, 0, 0);
    require(SettingsCodec::validate(*SettingsCodec::parse(wire), catalog).apply(catalog).applied);
    require(values.precise == 1.0000000000000002 && values.count == std::numeric_limits<uint32_t>::max());
    require(values.mode == Mode::Second && values.path == savedPath && values.matrix[2][1] == 12.5f);
    require(values.v2.y == 2 && values.v3.z == 5 && values.v4.w == 9 && values.color.b == 0.75f);

    auto checkRejected = [&](const std::string& name, const std::string& type, const char* value)
    {
        const auto writes = values.writes;
        const auto plan = SettingsCodec::validate(*single(name, type, json::JSON(value)), catalog);
        require(!plan.result().valid() && !plan.apply(catalog).applied && writes == values.writes);
    };
    checkRejected("exposure", "double", "0.5");
    checkRejected("exposure", "float", "100");
    checkRejected("exposure", "float", "nan");
    checkRejected("mode", "enum64", "1"); // Index is not enum identity.
    checkRejected("count", "uint32", "4294967296");
    checkRejected("missing", "float", "1");
    require(!SettingsCodec::prepare(catalog, {{exposure, double(0.5)}}).result().valid());
    require(!SettingsCodec::prepare(catalog, {{exposure, std::numeric_limits<float>::infinity()}}).result().valid());
    const auto before = values.writes;
    const auto invalidBatch = SettingsCodec::prepare(catalog,
        {{exposure, 5.0f}, {catalog.handle("values", "precise"), 2.0}});
    require(!invalidBatch.result().valid() && !invalidBatch.apply(catalog).applied && before == values.writes);
    const auto validBatch = SettingsCodec::prepare(catalog,
        {{exposure, 5.0f}, {catalog.handle("values", "precise"), 10.0}});
    require(validBatch.apply(catalog).applied && values.exposure == 5 && values.precise == 10);
    values.failSetter = true;
    const auto failed = SettingsCodec::prepare(catalog,
        {{catalog.handle("values", "precise"), 20.0}, {exposure, 7.0f}}).apply(catalog);
    require(!failed.valid() && !failed.applied && values.exposure == 5 && values.precise == 10);
    values.failSetter = false;

    values.optionalAvailable = false;
    require(!SettingsCodec::validate(*document, catalog).result().valid());
    const auto skipped = SettingsCodec::validate(*document, catalog, UnknownSettingPolicy::SkipUnknownOrUnavailable);
    require(skipped.result().valid() && !skipped.result().issues.empty() && skipped.apply(catalog).applied);
    const auto unknown = single("future", "bool", json::JSON(true));
    require(SettingsCodec::validate(*unknown, catalog, UnknownSettingPolicy::SkipUnknownOrUnavailable).result().valid());
    const auto malformedUnknown = single("future", "float", json::JSON("oops"));
    require(!SettingsCodec::validate(*malformedUnknown, catalog, UnknownSettingPolicy::SkipUnknownOrUnavailable).result().valid());
    for (const auto* bad : {"{", "{} trailing", "{\"a\":true,\"a\":false}", "[1]", "\"\\uD800\"", "[true,]"})
    {
        bool rejected = false;
        try
        {
            SettingsCodec::parse(bad);
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        require(rejected);
    }
    require(!SettingsCodec::validate(*SettingsCodec::parse("{}"), catalog).result().valid());
    const auto pending = SettingsCodec::prepare(catalog, {{exposure, 0.25f}});
    bind(catalog, values);
    require(!pending.apply(catalog).valid());
    bool stale = false;
    try
    {
        catalog.read(exposure);
    }
    catch (const std::exception&)
    {
        stale = true;
    }
    require(stale);
    SettingsCatalog other;
    bind(other, values);
    require(!SettingsCodec::prepare(other, {{catalog.handle("values", "exposure"), 0.5f}}).result().valid());

    reflection::TypeRegistry::get().registerType(metadata());
    const auto* shared = reflection::TypeRegistry::get().type("test.draw.settings.values.v1");
    SettingsTarget local;
    local.id = "limited";
    local.type = *shared;
    local.type.properties.front().metadata.max = 0.5;
    local.instance = &values;
    other.add(std::move(local));
    require(*shared->property("exposure")->metadata.max == 10);
}
}
