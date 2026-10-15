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

#include <bg2e/draw/RenderSettingsPreferences.hpp>
#include <bg2e/draw/RendererDeferred.hpp>
#include <bg2e/reflection/Builder.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <algorithm>
#include <mutex>
#include <stdexcept>

namespace bg2e::draw {
namespace {
using reflection::TypeInfoBuilder;
using reflection::TypeRegistry;
using Final = pipeline::FinalLayer;
using Shadows = pipeline::SoftShadowPass;
using Generation = environment::GenerationSettings;
using Environment = scene::EnvironmentComponent;
using Skybox = environment::SkyboxRenderer;
constexpr auto RendererIdentity = "bg2e.draw.deferred.v1";

void registerTypes()
{
    static std::once_flag once;
    std::call_once(once, []
    {
        using Lighting = pipeline::DeferredLayer;
        TypeInfoBuilder<Lighting> lighting("bg2e.draw.lighting.v1");
        lighting.property("debugView", &Lighting::debugView, &Lighting::setDebugView).category("Diagnostics")
            .enumValue("Final lighting", 0).enumValue("Unlit albedo", 1).enumValue("Direct light only", 2)
            .enumValue("IBL only", 3).enumValue("Emission only", 4).enumValue("Nonfinite values (magenta)", 5)
            .enumValue("IBL irradiance sample", 6).enumValue("IBL prefiltered sample", 7)
            .enumValue("IBL BRDF LUT sample", 8).enumValue("Ambient occlusion", 9).enumValue("Baked multiplier", 10);
        TypeRegistry::get().registerType(lighting.build());
        TypeInfoBuilder<Final> final("bg2e.draw.final.v1");
        final.property("exposure", &Final::exposure, &Final::setExposure).category("Color").range(-32, 32);
        final.property("contrast", &Final::contrast, &Final::setContrast).category("Color").range(0, 10);
        final.property("brightness", &Final::brightness, &Final::setBrightness).category("Color").range(-1, 1);
        final.property("gamma", &Final::gamma, &Final::setGamma).category("Color").range(0.1, 8);
        auto debug = final.property("debugView", &Final::debugView, &Final::setDebugView).category("Debug");
        const char* labels[] = {"Final", "Albedo", "Normal and emission", "Material", "Fresnel flags",
            "Sheen and refraction", "Baked multiplier", "Depth", "Raw HDR (no color processing)"};
        for (int64_t i = 0; i < 9; ++i)
        {
            debug.enumValue(labels[i], i);
        }
        TypeRegistry::get().registerType(final.build());

        TypeInfoBuilder<Shadows> shadows("bg2e.draw.shadows.v1");
        shadows.accessor<bool>("enabled", [](const Shadows& o)
        {
            return o.settings().enabled;
        },
            [](Shadows& o, bool v)
        {
            o.settings().enabled = v;
        }).category("Shadows");
        shadows.accessor<float>("bias", [](const Shadows& o)
        {
            return o.settings().bias;
        },
            [](Shadows& o, float v)
        {
            o.settings().bias = v;
        }).category("Shadows").range(0.000001f, 1000);
        shadows.accessor<float>("directionalDistance", [](const Shadows& o)
        {
            return o.settings().directionalDistance;
        },
            [](Shadows& o, float v)
        {
            o.settings().directionalDistance = v;
        }).category("Shadows").range(0.001, 1e9);
        shadows.accessor<uint32_t>("maxSamples", [](const Shadows& o)
        {
            return o.settings().maxSamples;
        },
            [](Shadows& o, uint32_t v)
        {
            o.settings().maxSamples = v;
        }).category("Shadows").range(1, pipeline::MaxShadowSamples);
        shadows.property("available", &Shadows::available).category("Capabilities");
        shadows.property("status", &Shadows::status).category("Capabilities");
        shadows.accessor<uint32_t>("alphaTextureLimit", [](const Shadows&)
        {
            return pipeline::MaxShadowAlphaTextures;
        })
            .category("Capabilities");
        TypeRegistry::get().registerType(shadows.build());

        TypeInfoBuilder<Skybox> skybox("bg2e.draw.skybox.v1");
        skybox.property("exposure", &Skybox::exposure, &Skybox::setExposure).category("Sky").range(-32, 32);
        TypeRegistry::get().registerType(skybox.build());

        TypeInfoBuilder<Environment> env("bg2e.draw.environment.v1");
        env.accessor<std::string>("source", [](const Environment& o)
        {
            return o.environmentImage();
        },
            [](Environment& o, const std::string& v)
        {
            o.setEnvironmentImage(v);
        })
            .category("Environment").resource("image", {".hdr"});
        env.property("skyboxBlur", &Environment::skyboxBlur, &Environment::setSkyboxBlur).category("Sky").range(0, 1);
        env.property("skyboxVisible", &Environment::skyboxVisible, &Environment::setSkyboxVisible).category("Sky");
        TypeRegistry::get().registerType(env.build());

        TypeInfoBuilder<base::Light> light("bg2e.draw.light.v1");
        light.property("color", &base::Light::color, &base::Light::setColor).category("Light").range(0, 1);
        light.property("intensity", &base::Light::intensity, &base::Light::setIntensity).category("Light").min(0);
        light.property("type", &base::Light::type, &base::Light::setType).category("Light")
            .enumValue("Omni", base::Light::TypeOmni).enumValue("Spot", base::Light::TypeSpot)
            .enumValue("Directional", base::Light::TypeDirectional).enumValue("Disabled", base::Light::TypeDisabled);
        light.property("spotAngle", &base::Light::spotAngle, &base::Light::setSpotAngle).category("Spot").range(0, 90);
        light.property("spotCutoff", &base::Light::spotCutoff, &base::Light::setSpotCutoff).category("Spot").range(0, 90);
        light.property("castShadows", &base::Light::castShadows, &base::Light::setCastShadows).category("Shadows");
        light.property("sourceSize", &base::Light::sourceSize, &base::Light::setSourceSize).category("Shadows").min(0);
        light.property("shadowSamples", &base::Light::shadowSamples, &base::Light::setShadowSamples)
            .category("Shadows").range(0, pipeline::MaxShadowSamples);
        TypeRegistry::get().registerType(light.build());

        TypeInfoBuilder<Generation> generation("bg2e.draw.environment.generation.v1");
        generation.accessor<uint32_t>("size", [](const Generation& o)
        {
            return o.size;
        },
            [](Generation& o, uint32_t v)
        {
            o.size = v;
        }).category("Generation").min(1);
        generation.accessor<uint32_t>("mipLevels", [](const Generation& o)
        {
            return o.mipLevels;
        },
            [](Generation& o, uint32_t v)
        {
            o.mipLevels = v;
        }).category("Generation").range(0, 32);
        generation.accessor<uint32_t>("samples", [](const Generation& o)
        {
            return o.samples;
        },
            [](Generation& o, uint32_t v)
        {
            o.samples = v;
        }).category("Generation").range(1, 4096);
        TypeRegistry::get().registerType(generation.build());

        TypeInfoBuilder<RendererDeferred> renderer("bg2e.draw.renderer.deferred.v1");
        renderer.property("inspectOpaqueLayer", &RendererDeferred::inspectOpaqueLayer, &RendererDeferred::setInspectOpaqueLayer).category("Diagnostics");
        renderer.property("queryGraphSelected", &RendererDeferred::usesQueryShadows).category("Capabilities");
        renderer.property("environmentError", &RendererDeferred::environmentError).category("Environment");
        TypeRegistry::get().registerType(renderer.build());
    });
}
SettingsTarget target(std::string id, const std::string& type, void* instance, std::shared_ptr<void> owner = {})
{
    SettingsTarget result;
    result.id = std::move(id);
    result.type = *TypeRegistry::get().type(type); // Instance copy; never edit the registry.
    result.instance = instance;
    result.owner = std::move(owner);
    return result;
}
bool queryAvailable(RendererDeferred& renderer)
{
    const auto pass = renderer.softShadows();
    return renderer.usesQueryShadows() && pass && pass->available();
}
void bindScene(SettingsCatalog& catalog, RendererDeferred& renderer, const std::shared_ptr<scene::Node>& node,
    bool& environmentBound)
{
    if (!node || !node->enabled())
    {
        return;
    }
    if (auto component = node->environment(); component && !environmentBound)
    {
        environmentBound = true;
        auto entry = target("environment/" + node->identifier(), "bg2e.draw.environment.v1", component,
            component->shared_from_this());
        entry.available = [&renderer](const std::string&)
        {
            return renderer.environmentGenerator() != nullptr;
        };
        catalog.add(std::move(entry));
    }
    if (auto component = node->light())
    {
        auto* light = &component->light();
        auto entry = target("light/" + node->identifier(), "bg2e.draw.light.v1", light, component->shared_from_this());
        entry.available = [&renderer, light](const std::string& property)
        {
            if (property == "spotAngle" || property == "spotCutoff")
            {
                return light->type() == base::Light::TypeSpot;
            }
            if (property == "castShadows" || property == "sourceSize" || property == "shadowSamples")
            {
                return queryAvailable(renderer);
            }
            return true;
        };
        catalog.add(std::move(entry));
    }
    for (const auto& child : node->children())
    {
        bindScene(catalog, renderer, child, environmentBound);
    }
}
void bindGeneration(SettingsCatalog& catalog, environment::EnvironmentGenerator& generator)
{
    const auto& config = generator.configuration();
    const std::pair<const char*, std::shared_ptr<environment::CubemapRenderer>> tools[] = {
        {"sphere", config.sphere}, {"defaultSky", config.defaultSky}, {"irradiance", config.irradiance},
        {"specular", config.specular}, {"brdf", config.brdf}};
    for (const auto& [role, tool] : tools)
    {
        if (!tool)
        {
            continue;
        }
        auto entry = target(std::string("environment.generator.") + role,
            "bg2e.draw.environment.generation.v1", &tool->settings(), tool);
        const bool singleMip = std::string(role) == "irradiance" || std::string(role) == "brdf";
        // Use the selected output's actual device limit, not a backend-wide guess.
        if (generator.resources() && generator.resources()->sky)
        {
            for (auto& property : entry.type.properties)
            {
                if (property.name == "size")
                {
                    property.metadata.max = generator.resources()->sky->device()->renderingCapabilities().maxImageDimension2D;
                }
                if (singleMip && property.name == "mipLevels")
                {
                    property.metadata.min = 1;
                    property.metadata.max = 1;
                }
            }
        }
        entry.validate = [singleMip](const std::map<std::string, std::any>& values)
        {
            const auto size = std::any_cast<uint32_t>(values.at("size"));
            const auto mips = std::any_cast<uint32_t>(values.at("mipLevels"));
            uint32_t fullChain = 0;
            for (auto n = size; n; n >>= 1)
            {
                ++fullChain;
            }
            if ((singleMip && mips != 1) || mips > fullChain)
            {
                throw std::invalid_argument("Mip count is incompatible with generation size/kind");
            }
        };
        entry.changed = [&generator]
        {
            generator.requestRebuild();
        };
        catalog.add(std::move(entry));
    }
}
SettingsResult failure(const char* code, const std::string& message)
{
    SettingsResult result;
    result.issues.push_back({code, "", message});
    return result;
}
std::string hexEncode(const std::string& input)
{
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(input.size() * 2);
    for (unsigned char c : input)
    {
        result += digits[c >> 4];
        result += digits[c & 15];
    }
    return result;
}
std::string hexDecode(const std::string& input)
{
    if (input.size() % 2)
    {
        throw std::invalid_argument("Malformed persisted settings encoding");
    }
    auto digit = [](char c) -> unsigned
    {
        const std::string digits = "0123456789abcdef";
        const auto pos = digits.find(c);
        if (pos == std::string::npos)
        {
            throw std::invalid_argument("Malformed persisted settings encoding");
        }
        return unsigned(pos);
    };
    std::string result;
    for (size_t i = 0; i < input.size(); i += 2)
    {
        result += char((digit(input[i]) << 4) | digit(input[i + 1]));
    }
    return result;
}
}
RenderSettingsPreferences::RenderSettingsPreferences(app::Preferences* persistence, std::string key)
    : _persistence(persistence), _key(std::move(key))
{
    if (_key.empty())
    {
        throw std::invalid_argument("Settings persistence requires a key");
    }
}
SettingsCatalog& RenderSettingsPreferences::catalog()
{
    return _catalog;
}
const SettingsCatalog& RenderSettingsPreferences::catalog() const
{
    return _catalog;
}
void RenderSettingsPreferences::clear()
{
    _catalog.reset();
    _rendererGeneration = 0;
}
void RenderSettingsPreferences::bind(Renderer& renderer, std::shared_ptr<scene::Node> root, ExtendCatalog extensions)
{
    clear();
    registerTypes();
    try
    {
        if (auto* deferred = dynamic_cast<RendererDeferred*>(&renderer))
        {
            _catalog.reset(RendererIdentity);
            _catalog.add(target("renderer", "bg2e.draw.renderer.deferred.v1", deferred));
            if (auto resolution = deferred->resolution())
            {
                _catalog.add({"resolution", resolution->settingsType(), resolution.get(), resolution, {}, {}, {}});
            }
            if (auto upscaler = deferred->upscaler())
            {
                _catalog.add({"upscaler/" + upscaler->identifier(), upscaler->settingsType(), dynamic_cast<void*>(upscaler.get()), upscaler, {}, {}, {}});
            }
            for (const auto& selection : renderer.configuration().layers)
            {
                if (auto lighting = std::dynamic_pointer_cast<pipeline::DeferredLayer>(selection.layer))
                {
                    _catalog.add(target("lighting", "bg2e.draw.lighting.v1", lighting.get(), lighting));
                }
                if (auto final = std::dynamic_pointer_cast<Final>(selection.layer))
                {
                    _catalog.add(target("final", "bg2e.draw.final.v1", final.get(), final));
                }
                if (auto skybox = std::dynamic_pointer_cast<Skybox>(selection.layer))
                {
                    _catalog.add(target("skybox", "bg2e.draw.skybox.v1", skybox.get(), skybox));
                }
            }
            if (auto shadows = deferred->softShadows())
            {
                auto entry = target("shadows", "bg2e.draw.shadows.v1", shadows.get(), shadows);
                entry.available = [deferred](const std::string& property)
                {
                    return property == "available" || property == "status" || property == "alphaTextureLimit" || queryAvailable(*deferred);
                };
                entry.validate = [](const std::map<std::string, std::any>& values)
                {
                    if (std::any_cast<float>(values.at("directionalDistance")) <= 2 * std::any_cast<float>(values.at("bias")))
                    {
                        throw std::invalid_argument("Directional shadow distance must exceed twice the bias");
                    }
                };
                _catalog.add(std::move(entry));
            }
            if (auto generator = deferred->environmentGenerator())
            {
                bindGeneration(_catalog, *generator);
            }
            bool environmentBound = false;
            bindScene(_catalog, *deferred, root, environmentBound);
        }
        if (extensions)
        {
            extensions(renderer, _catalog);
        }
        if (_catalog.rendererId().empty())
        {
            throw std::invalid_argument("Custom renderers require a catalog extension with a stable renderer identity");
        }
    }
    catch (...)
    {
        clear();
        throw;
    }
}
void RenderSettingsPreferences::connect(const std::shared_ptr<DefaultRenderLoopDelegate>& delegate, ExtendCatalog extensions)
{
    if (!delegate || !_delegate.expired())
    {
        throw std::logic_error("Settings facade requires one delegate connection");
    }
    const std::weak_ptr<RenderSettingsPreferences> self = shared_from_this();
    const std::weak_ptr<DefaultRenderLoopDelegate> weakDelegate = delegate;
    delegate->setRendererChangeCallbacks([self]
    {
        if (auto owner = self.lock())
        {
            owner->clear();
        }
    }, [self, weakDelegate, extensions](Renderer& renderer, uint64_t generation)
    {
        if (auto owner = self.lock())
        {
            if (auto loop = weakDelegate.lock())
            {
                owner->bind(renderer, loop->scene(), extensions);
                owner->_rendererGeneration = generation;
            }
        }
    });
    _delegate = delegate;
}
void RenderSettingsPreferences::queueSet(SettingHandle handle, std::any value, Completion completion)
{
    enqueue(SettingsCodec::prepare(_catalog, {{std::move(handle), std::move(value)}}), std::move(completion));
}
void RenderSettingsPreferences::queueImport(const json::JsonNode& document, UnknownSettingPolicy policy, Completion completion)
{
    enqueue(SettingsCodec::validate(document, _catalog, policy), std::move(completion));
}
void RenderSettingsPreferences::enqueue(SettingsImport plan, Completion completion)
{
    if (!plan.result().valid())
    {
        if (completion)
        {
            completion(plan.result());
        }
        return;
    }
    auto delegate = _delegate.lock();
    if (!delegate)
    {
        if (completion)
        {
            completion(failure("no_delegate", "Queued settings require a connected live delegate"));
        }
        return;
    }
    const std::weak_ptr<RenderSettingsPreferences> self = shared_from_this();
    delegate->enqueueRendererUpdate(_rendererGeneration, [self, plan, completion](Renderer&)
    {
        auto owner = self.lock();
        auto result = owner ? plan.apply(owner->_catalog) : failure("target_expired", "Settings facade no longer exists");
        if (completion)
        {
            completion(std::move(result));
        }
    }, [completion]
    {
        if (completion)
        {
            completion(failure("stale_renderer", "Renderer was replaced or rebound before the settings update"));
        }
    });
}
bool RenderSettingsPreferences::hasPersistence() const
{
    return _persistence != nullptr;
}
void RenderSettingsPreferences::queueRestore(UnknownSettingPolicy policy, Completion completion)
{
    if (!_persistence)
    {
        if (completion)
        {
            completion(failure("no_preferences", "No preferences service supplied"));
        }
        return;
    }
    try
    {
        const auto stored = _persistence->get(_key, std::string{});
        if (stored.empty())
        {
            if (completion)
            {
                completion({});
            }
            return;
        }
        queueImport(*SettingsCodec::parse(hexDecode(stored)), policy, completion);
    }
    catch (const std::exception& error)
    {
        if (completion)
        {
            completion(failure("invalid_document", error.what()));
        }
    }
}
void RenderSettingsPreferences::persist() const
{
    if (!_persistence)
    {
        throw std::logic_error("No preferences service supplied");
    }
    // Preferences uses the legacy JsonNode writer; ASCII hex avoids its unescaped
    // string limitation without losing paths, Unicode or exact numeric strings.
    _persistence->set(_key, hexEncode(SettingsCodec::stringify(*SettingsCodec::serialize(_catalog))));
    _persistence->save();
}
SettingsResult RenderSettingsPreferences::restore(const json::JsonNode* explicitDocument, UnknownSettingPolicy policy)
{
    try
    {
        if (explicitDocument)
        {
            return SettingsCodec::validate(*explicitDocument, _catalog, policy).apply(_catalog);
        }
        if (!_persistence)
        {
            return failure("no_preferences", "No preferences service supplied");
        }
        const auto stored = _persistence->get(_key, std::string{});
        if (stored.empty())
        {
            return {};
        }
        return SettingsCodec::validate(*SettingsCodec::parse(hexDecode(stored)), _catalog, policy).apply(_catalog);
    }
    catch (const std::exception& error)
    {
        return failure("invalid_document", error.what());
    }
}
}
