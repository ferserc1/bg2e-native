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
#include <bg2e/scene/Scene.hpp>
#include <bg2e/draw/RendererDeferred.hpp>
#include <bg2e/reflection/Builder.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <algorithm>
#include <charconv>
#include <typeinfo>
#include <mutex>
#include <stdexcept>
#include <iostream>

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

reflection::TypeInfo finalSettingsType()
{
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
    return final.build();
}

void registerTypes()
{
    static std::once_flag once;
    std::call_once(once, []
    {
        using Lighting = pipeline::DeferredLayer;
        TypeInfoBuilder<Lighting> lighting("bg2e.draw.lighting.v1");
        lighting.property("debugView", &Lighting::debugView, &Lighting::setDebugView).category("Diagnostics")
            .enumValue("Final lighting", 0).enumValue("Unlit albedo", 1).enumValue("Direct light only", 2)
            .enumValue("Indirect lighting", 3).enumValue("Emission only", 4).enumValue("Nonfinite values (magenta)", 5)
            .enumValue("IBL irradiance sample", 6).enumValue("IBL prefiltered sample", 7)
            .enumValue("IBL BRDF LUT sample", 8).enumValue("Ambient occlusion", 9).enumValue("Baked multiplier", 10)
            .enumValue("Ray visibility",11).enumValue("Diffuse GI signal",12)
            .enumValue("Reflected radiance signal",13).enumValue("Reflection confidence",14);
        TypeRegistry::get().registerType(lighting.build());
        TypeRegistry::get().registerType(finalSettingsType());

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
        shadows.property("sceneSupported", &Shadows::sceneSupported).category("Capabilities");
        shadows.property("status", &Shadows::status).category("Capabilities");
        shadows.accessor<uint32_t>("alphaTextureLimit", [](const Shadows& o)
        {
            return o.alphaTextureCapacity();
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
             .category("Environment").resource("image", {".hdr", ".jpg", ".jpeg", ".png", ".bmp", ".tga"});
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
        light.property("affectsReflections", &base::Light::affectsReflections, &base::Light::setAffectsReflections).category("Light");
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
        renderer.property("temporalSurfaceEnabled", &RendererDeferred::temporalSurfaceEnabled, &RendererDeferred::setTemporalSurfaceEnabled).category("Diagnostics");
        renderer.property("temporalDiagnosticView", &RendererDeferred::temporalDiagnosticView, &RendererDeferred::setTemporalDiagnosticView)
            .category("Diagnostics").enumValue("Off",0).enumValue("Surface tokens",1).enumValue("Local history (red restart, green accepted)",2)
            .enumValue("Motion (32 pixels per half-range)",3).enumValue("Expected previous position",4).enumValue("Expected previous normal",5);
        renderer.property("temporalDiagnosticSignal", &RendererDeferred::temporalDiagnosticSignal, &RendererDeferred::setTemporalDiagnosticSignal)
            .category("Diagnostics").enumValue("Visibility",0).enumValue("Diffuse radiance",1).enumValue("Reflected radiance",2);
        // Reflection has no uint64 scalar; decimal text preserves the complete layer key.
        renderer.accessor<std::string>("temporalDiagnosticLayer", [](const RendererDeferred& o)
        {
            return std::to_string(o.temporalDiagnosticLayer());
        }, [](RendererDeferred& o, const std::string& value)
        {
            uint64_t layer = 0;
            const auto result = std::from_chars(value.data(),value.data()+value.size(),layer);
            if (value.empty() || result.ec != std::errc{} || result.ptr != value.data()+value.size())
            {
                throw std::invalid_argument("Temporal diagnostic layer must be an unsigned 64-bit decimal integer");
            }
            o.setTemporalDiagnosticLayer(layer);
        }).category("Diagnostics");
        renderer.property("temporalDiagnosticsAvailable", &RendererDeferred::temporalDiagnosticsAvailable).category("Capabilities");
        renderer.property("temporalSurfaceDiagnosticsAvailable", &RendererDeferred::temporalSurfaceDiagnosticsAvailable).category("Capabilities");
        renderer.property("temporalSurfaceAvailable", &RendererDeferred::temporalSurfaceAvailable).category("Capabilities");
        renderer.property("temporalSurfaceStatus", &RendererDeferred::temporalSurfaceStatus).category("Capabilities");
        renderer.property("queryGraphSelected", &RendererDeferred::usesQueryShadows).category("Capabilities");
        renderer.property("indirectLighting", &RendererDeferred::indirectLightingMode, &RendererDeferred::setIndirectLightingMode)
            .category("Indirect lighting").enumValue("IBL",0).enumValue("AO",1).enumValue("GI",2);
        renderer.property("aoGraphSelected", &RendererDeferred::usesAmbientOcclusion).category("Capabilities");
        renderer.property("giGraphSelected", &RendererDeferred::usesGlobalIllumination).category("Capabilities");
        renderer.property("reflectionGraphSelected", &RendererDeferred::usesReflections).category("Capabilities");
        renderer.property("noiseStatus", &RendererDeferred::noiseStatus).category("Capabilities");
        renderer.property("environmentError", &RendererDeferred::environmentError).category("Environment");
        renderer.property("initializationWarnings", &RendererDeferred::initializationWarnings).category("Capabilities");
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
        entry.available = [light](const std::string& property)
        {
            if (property == "spotAngle" || property == "spotCutoff")
            {
                return light->type() == base::Light::TypeSpot;
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

void excludeSceneSettings(json::JsonNode& document)
{
    auto componentsNode = document.objectValue().at("components");
    if (!componentsNode)
    {
        throw std::invalid_argument("Missing settings components");
    }
    auto& components = componentsNode->listValue();
    std::erase_if(components, [](const auto& component)
    {
        if (!component)
        {
            throw std::invalid_argument("Missing settings component");
        }
        auto idNode = component->objectValue().at("id");
        if (!idNode)
        {
            throw std::invalid_argument("Missing settings component identity");
        }
        const auto& id = idNode->stringValue();
        return id.starts_with("environment/") || id.starts_with("light/");
    });
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
    _boundedFinal.reset();
    _boundedFinalCatalogGeneration = 0;
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
                    auto entry = target("final", "bg2e.draw.final.v1", final.get(), final);
                    const auto* finalPointer = final.get();
                    if (!extensions && typeid(renderer) == typeid(RendererDeferred) &&
                        typeid(*finalPointer) == typeid(Final))
                    {
                        entry.type = finalSettingsType();
                        _boundedFinal = final;
                    }
                    _catalog.add(std::move(entry));
                }
                if (auto skybox = std::dynamic_pointer_cast<Skybox>(selection.layer))
                {
                    _catalog.add(target("skybox", "bg2e.draw.skybox.v1", skybox.get(), skybox));
                }
            }
            if (auto shadows = deferred->softShadows())
            {
                auto entry = target("shadows", "bg2e.draw.shadows.v1", shadows.get(), shadows);
                entry.validate = [](const std::map<std::string, std::any>& values)
                {
                    if (std::any_cast<float>(values.at("directionalDistance")) <= 2 * std::any_cast<float>(values.at("bias")))
                    {
                        throw std::invalid_argument("Directional shadow distance must exceed twice the bias");
                    }
                };
                _catalog.add(std::move(entry));
            }
            // Writable requested values remain serializable on unsupported devices.
            // Each injected component supplies both its metadata and matching base address.
            auto addEffect = [&](const std::string& id, const auto& effect, const std::string& distance = {})
            {
                if (!effect)
                {
                    return;
                }
                SettingsTarget entry{id,effect->settingsType(),effect->settingsInstance(),effect,{},{},{}};
                entry.changed = [deferred]
                {
                    deferred->invalidateHistories();
                };
                if (!distance.empty())
                {
                    entry.validate = [distance](const std::map<std::string,std::any>& values)
                    {
                        // Replacement metadata can define a different public property set.
                        if (values.contains(distance) && values.contains("bias") &&
                            std::any_cast<float>(values.at(distance)) <= 2*std::any_cast<float>(values.at("bias")))
                        {
                            throw std::invalid_argument("Effect distance must exceed twice its ray bias");
                        }
                    };
                }
                _catalog.add(std::move(entry));
            };
            addEffect("ambientOcclusion",deferred->ambientOcclusion(),"radius");
            addEffect("globalIllumination",deferred->globalIllumination(),"maxDistance");
            addEffect("reflections",deferred->reflections(),"maxDistance");
            addEffect("temporal",deferred->temporal());
            addEffect("denoise",deferred->denoise());
            addEffect("finalProcessor",deferred->finalProcessor());
            if (auto generator = deferred->environmentGenerator())
            {
                bindGeneration(_catalog, *generator);
            }
            bool environmentBound = false;
            auto* ownerScene = root ? root->sceneRoot()->scene() : nullptr;
            if (ownerScene && ownerScene->hasExplicitMainEnvironment())
            {
                environmentBound = true; // Explicit nullptr must not bind an ignored component.
                if (auto* component = ownerScene->mainEnvironment())
                {
                    auto entry = target("environment/" + component->ownerNode()->identifier(),
                        "bg2e.draw.environment.v1", component, component->shared_from_this());
                    entry.available = [deferred](const std::string&)
                    {
                        return deferred->environmentGenerator() != nullptr;
                    };
                    _catalog.add(std::move(entry));
                }
            }
            bindScene(_catalog, *deferred, root, environmentBound);
        }
        if (extensions)
        {
            extensions(renderer, _catalog);
        }
        if (!_boundedFinal.expired())
        {
            _boundedFinalCatalogGeneration = _catalog.generation();
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
                if (owner->_automaticPersistence && !owner->_restored)
                {
                    owner->_restored = true;
                    const auto result = owner->restore(nullptr, UnknownSettingPolicy::SkipUnknownOrUnavailable);
                    for (const auto& issue : result.issues)
                    {
                        std::cerr << "WARN: Could not restore graphics preferences: " << issue.message << std::endl;
                    }
                }
            }
        }
    });
    _delegate = delegate;
}

void RenderSettingsPreferences::queueSet(SettingHandle handle, std::any value, Completion completion,
    SettingsUpdateSynchronization synchronization)
{
    auto plan = SettingsCodec::prepare(_catalog, {{handle, value}});
    auto delegate = _delegate.lock();
    auto target = _boundedFinal.lock();
    std::optional<FinalColorProperty> property;
    if (handle.property == "exposure")
    {
        property = FinalColorProperty::Exposure;
    }
    else if (handle.property == "contrast")
    {
        property = FinalColorProperty::Contrast;
    }
    else if (handle.property == "brightness")
    {
        property = FinalColorProperty::Brightness;
    }
    else if (handle.property == "gamma")
    {
        property = FinalColorProperty::Gamma;
    }
    const auto* number = std::any_cast<float>(&value);
    const auto* renderer = delegate ? delegate->renderer() : nullptr;
    if ((!completion || synchronization == SettingsUpdateSynchronization::ValueOnlyNotification) &&
        plan.result().valid() && delegate && target && property && number && handle.component == "final" &&
        _boundedFinalCatalogGeneration == _catalog.generation() && renderer &&
        renderer->configuration().layers.size() > 2 && renderer->configuration().layers[2].layer == target)
    {
        const std::weak_ptr<RenderSettingsPreferences> self = shared_from_this();
        const auto catalogGeneration = _catalog.generation();
        const bool queued = delegate->enqueueFinalColorUpdate(_rendererGeneration, *property, *number,
            [self, completion, catalogGeneration](ValueUpdateResult status)
        {
            auto owner = self.lock();
            SettingsResult result;
            if (!owner)
            {
                result = failure("target_expired", "Settings facade no longer exists");
            }
            else if (owner->_catalog.generation() != catalogGeneration)
            {
                result = failure("stale_catalog", "Settings targets changed after validation");
            }
            else if (status == ValueUpdateResult::Stale)
            {
                result = failure("stale_renderer", "Renderer was replaced or rebound before the settings update");
            }
            else
            {
                result.applied = true;
                if (owner->_automaticPersistence)
                {
                    try
                    {
                        owner->persist();
                    }
                    catch (const std::exception& error)
                    {
                        result.issues.push_back({"persistence_failed", "", error.what()});
                    }
                }
            }
            if (completion)
            {
                completion(std::move(result));
            }
        }, [self, catalogGeneration]
        {
            auto owner = self.lock();
            return owner && owner->_catalog.generation() == catalogGeneration;
        });
        if (queued)
        {
            return;
        }
    }
    enqueue(std::move(plan), std::move(completion));
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
    delegate->enqueueRendererUpdate(_rendererGeneration, [self, plan, completion](Renderer& renderer)
    {
        auto owner = self.lock();
        auto result = owner ? plan.apply(owner->_catalog) : failure("target_expired", "Settings facade no longer exists");
        renderer.invalidateHistories(); // Includes reported partial application failures.
        if (owner && owner->_automaticPersistence && result.valid())
        {
            try
            {
                owner->persist();
            }
            catch (const std::exception& error)
            {
                result.issues.push_back({"persistence_failed", "", error.what()});
            }
        }
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

void RenderSettingsPreferences::setAutomaticPersistence(bool enabled)
{
    if (enabled && !_persistence)
    {
        throw std::logic_error("Automatic settings persistence requires a preferences service");
    }
    _automaticPersistence = enabled;
}

void RenderSettingsPreferences::setExcludeSceneSettingsFromAutomaticPersistence(bool enabled)
{
    _excludeSceneSettingsFromPersistence = enabled;
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
        auto document = SettingsCodec::parse(hexDecode(stored));
        if (_excludeSceneSettingsFromPersistence)
        {
            excludeSceneSettings(*document);
        }
        queueImport(*document, policy, completion);
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
    auto document = SettingsCodec::serialize(_catalog);
    if (_excludeSceneSettingsFromPersistence)
    {
        excludeSceneSettings(*document);
    }
    _persistence->set(_key, hexEncode(SettingsCodec::stringify(*document)));
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
        auto document = SettingsCodec::parse(hexDecode(stored));
        if (_excludeSceneSettingsFromPersistence)
        {
            excludeSceneSettings(*document);
        }
        return SettingsCodec::validate(*document, _catalog, policy).apply(_catalog);
    }
    catch (const std::exception& error)
    {
        return failure("invalid_document", error.what());
    }
}
}
