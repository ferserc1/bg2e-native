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

#include "TemporalSignatureCache.hpp"
#include <bg2e/base/CpuProfile.hpp>
#include <cstring>
#include <algorithm>
#include <utility>
#include <tuple>

namespace bg2e::draw {
namespace {
struct Writer {
    std::vector<uint8_t> bytes;

    template<class T> void append(const T& value)
    {
        const auto* first = reinterpret_cast<const uint8_t*>(&value);
        bytes.insert(bytes.end(), first, first + sizeof(value));
    }
    void pointer(const void* value)
    {
        append(reinterpret_cast<uintptr_t>(value));
    }
    std::vector<uint8_t> finish()
    {
        base::CpuProfileScope::count("temporal.signatureBytesRebuilt", bytes.size(), "bytes");
        return std::move(bytes);
    }
};

template<class A, class B, size_t... I> bool sameFields(const A& a, const B& b, std::index_sequence<I...>)
{
    return ((std::memcmp(&std::get<I>(a), &std::get<I>(b), sizeof(std::get<I>(a))) == 0) && ...);
}

template<class... T> bool sameFields(const std::tuple<T...>& a, const std::tuple<T...>& b)
{
    return sameFields(a, b, std::index_sequence_for<T...>{});
}

template<class T> bool sameComponent(const std::shared_ptr<const T>& a, const std::shared_ptr<const T>& b)
{
    return a == b || (a && b && a->bytes == b->bytes);
}

bool sameGeometry(const std::shared_ptr<const SignatureGeometry>& a, const std::shared_ptr<const SignatureGeometry>& b)
{
    return a == b || (a && b && a->prefix.bytes == b->prefix.bytes && a->inclusion.bytes == b->inclusion.bytes);
}

bool sameMaterial(const std::shared_ptr<const SignatureMaterial>& a, const std::shared_ptr<const SignatureMaterial>& b)
{
    return a == b || (a && b && a->identity.bytes == b->identity.bytes && a->values.bytes == b->values.bytes &&
        a->textures.bytes == b->textures.bytes);
}

void hit(const char* counter)
{
    base::CpuProfileScope::count("temporal.signatureComponentHits", 1);
    base::CpuProfileScope::count(counter, 1);
}
}

struct SignatureCamera : SignatureBytes {
    glm::mat4 projection{1};
    bool explicitCamera = false;
    const void* identity = nullptr;
    gpu::Size2D output, internal;
    IndirectLightingMode mode{};
    std::shared_ptr<Upscaler> upscaler;
    std::shared_ptr<scene::Component> cameraOwner; // Retain camera identity against pointer ABA.
    UpscalerRequirements requirements{};

    auto fields() const
    {
        return std::tie(projection, explicitCamera, identity, output.width, output.height, internal.width, internal.height,
            mode, requirements.opaqueDepth, requirements.cameraMotion, requirements.jitter, requirements.storageOutput);
    }
    bool matches(const SignatureCamera& other) const
    {
        return upscaler == other.upscaler && sameFields(fields(), other.fields());
    }
};

struct SignatureEnvironment : SignatureBytes {
    std::shared_ptr<const environment::EnvironmentResources> owner;
    std::array<std::shared_ptr<gpu::Image>,4> images;
    std::shared_ptr<gpu::Sampler> sampler;
    float blur = 0;
    bool visible = false;

    bool matches(const SignatureEnvironment& other) const
    {
        return owner == other.owner && images == other.images && sampler == other.sampler &&
            sameFields(std::tie(blur, visible), std::tie(other.blur, other.visible));
    }
};

struct SignatureNoiseState {
    std::shared_ptr<utils::BlueNoise> owner;
    std::shared_ptr<gpu::Image> image;
    std::shared_ptr<gpu::Sampler> sampler;
    glm::uvec4 metadata{0};
    bool ready = false;

    bool matches(const SignatureNoiseState& other) const
    {
        return owner == other.owner && ready == other.ready && image == other.image && sampler == other.sampler &&
            sameFields(std::tie(metadata), std::tie(other.metadata));
    }
};

struct SignatureSettings : SignatureBytes {
    pipeline::RTAOSettings ao;
    pipeline::RTGISettings gi;
    pipeline::RTReflectionSettings reflection;
    pipeline::SoftShadowSettings shadows;
    pipeline::TemporalSettings temporal;
    std::tuple<bool,uint32_t,float,float,float,float,float> denoise;
    std::array<SignatureNoiseState,3> noise;

    auto fields() const;
    bool matches(const SignatureSettings& other) const;
};

auto SignatureSettings::fields() const
{
    const auto& s = *this;
    return std::tie(s.ao.enabled,
        s.ao.quality,
        s.ao.sampleCount,
        s.ao.bounceCount,
        s.ao.radius,
        s.ao.bias,
        s.ao.falloff,
        s.ao.bounceAttenuation,
        s.ao.useBlueNoise,
        s.gi.enabled,
        s.gi.quality,
        s.gi.sampleCount,
        s.gi.bounceCount,
        s.gi.bias,
        s.gi.maxDistance,
        s.gi.shadowSamples,
        s.gi.useShadows,
        s.gi.useBlueNoise,
        s.reflection.enabled,
        s.reflection.quality,
        s.reflection.sampleCount,
        s.reflection.bias,
        s.reflection.maxDistance,
        s.reflection.maxRoughness,
        s.reflection.roughnessSpread,
        s.reflection.shadowSamples,
        s.reflection.useShadows,
        s.reflection.useBlueNoise,
        s.shadows.enabled,
        s.shadows.bias,
        s.shadows.directionalDistance,
        s.shadows.maxSamples,
        s.temporal.enabled,
        s.temporal.maxFrames,
        s.temporal.freezeAtLimit,
        s.temporal.absolutePositionThreshold,
        s.temporal.relativePositionThreshold,
        s.temporal.normalThreshold);
}

bool SignatureSettings::matches(const SignatureSettings& other) const
{
    return sameFields(fields(), other.fields()) && sameFields(denoise, other.denoise) &&
        noise[0].matches(other.noise[0]) && noise[1].matches(other.noise[1]) && noise[2].matches(other.noise[2]);
}

namespace {
std::shared_ptr<const SignatureCamera> camera(const TemporalSignatureContext& context,
    const std::shared_ptr<const SignatureCamera>& previous)
{
    SignatureCamera snapshot;
    auto* key = &snapshot;
    key->projection = context.projection;
    key->explicitCamera = context.explicitCamera;
    key->identity = context.cameraIdentity;
    key->output = context.configuration.outputExtent;
    key->internal = context.configuration.internalExtent;
    key->mode = context.configuration.indirectLighting;
    key->upscaler = context.configuration.upscaler;
    key->cameraOwner = context.cameraOwner;
    key->requirements = key->upscaler->requirements();
    if (previous && key->matches(*previous))
    {
        hit("temporal.signatureCameraHits");
        return previous;
    }
    Writer writer;
    writer.append(key->projection);
    writer.append(key->explicitCamera);
    writer.pointer(key->identity);
    writer.append(key->output.width);
    writer.append(key->output.height);
    writer.append(key->internal.width);
    writer.append(key->internal.height);
    writer.append(key->mode);
    writer.pointer(key->upscaler.get());
    writer.append(key->requirements.opaqueDepth);
    writer.append(key->requirements.cameraMotion);
    writer.append(key->requirements.jitter);
    writer.append(key->requirements.storageOutput);
    key->bytes = writer.finish();
    return std::make_shared<SignatureCamera>(std::move(snapshot));
}

std::shared_ptr<const SignatureEnvironment> environmentState(const TemporalSignatureContext& context,
    const std::shared_ptr<const SignatureEnvironment>& previous)
{
    SignatureEnvironment snapshot;
    auto* key = &snapshot;
    key->owner = context.environment;
    if (key->owner)
    {
        key->images = {key->owner->sky, key->owner->irradiance, key->owner->prefilteredSpecular, key->owner->brdf};
        key->sampler = key->owner->sampler;
        key->blur = key->owner->skyboxBlur;
        key->visible = key->owner->skyboxVisible;
    }
    if (previous && key->matches(*previous))
    {
        hit("temporal.signatureEnvironmentHits");
        return previous;
    }
    Writer writer;
    writer.pointer(key->owner.get());
    if (key->owner)
    {
        for (const auto& image : key->images)
        {
            writer.pointer(image.get());
        }
        writer.pointer(key->sampler.get());
        writer.append(key->blur);
        writer.append(key->visible);
    }
    key->bytes = writer.finish();
    return std::make_shared<SignatureEnvironment>(std::move(snapshot));
}

std::shared_ptr<const SignatureSettings> settings(const TemporalSignatureContext& context,
    const std::shared_ptr<const SignatureSettings>& previous)
{
    SignatureSettings snapshot;
    auto* key = &snapshot;
    const auto& configuration = context.configuration;
    key->ao = configuration.ambientOcclusion->settings();
    key->gi = configuration.globalIllumination->settings();
    key->reflection = configuration.reflections->settings();
    key->shadows = configuration.softShadows->settings();
    key->temporal = configuration.temporal->settings();
    // Virtual effective getters and mutable settings remain observable without
    // trusting setter-only revision counters or comparing struct padding.
    key->denoise = {configuration.denoise->enabled(), configuration.denoise->kernelRadius(),
        configuration.denoise->depthThreshold(), configuration.denoise->normalThreshold(), configuration.denoise->depthSigma(),
        configuration.denoise->normalSigma(), configuration.denoise->spatialSigma()};
    const std::array<std::shared_ptr<utils::BlueNoise>,3> sources{configuration.ambientOcclusion->noise(),
        configuration.globalIllumination->noise(), configuration.reflections->noise()};
    for (size_t i = 0; i < sources.size(); ++i)
    {
        auto& noise = key->noise[i];
        noise.owner = sources[i];
        noise.ready = noise.owner && noise.owner->isReady();
        if (noise.ready)
        {
            noise.image = noise.owner->image();
            noise.sampler = noise.owner->sampler();
            noise.metadata = noise.owner->shaderMetadata();
        }
    }
    if (previous && key->matches(*previous))
    {
        hit("temporal.signatureSettingsHits");
        return previous;
    }
    Writer writer;
    const auto noise = [&](const SignatureNoiseState& value)
    {
        writer.pointer(value.owner.get());
        if (value.ready)
        {
            writer.pointer(value.image.get());
            writer.pointer(value.sampler.get());
            writer.append(value.metadata);
        }
    };
    writer.append(key->ao.enabled);
    writer.append(key->ao.quality);
    writer.append(key->ao.sampleCount);
    writer.append(key->ao.bounceCount);
    writer.append(key->ao.radius);
    writer.append(key->ao.bias);
    writer.append(key->ao.falloff);
    writer.append(key->ao.bounceAttenuation);
    writer.append(key->ao.useBlueNoise);
    noise(key->noise[0]);
    writer.append(key->gi.enabled);
    writer.append(key->gi.quality);
    writer.append(key->gi.sampleCount);
    writer.append(key->gi.bounceCount);
    writer.append(key->gi.bias);
    writer.append(key->gi.maxDistance);
    writer.append(key->gi.shadowSamples);
    writer.append(key->gi.useShadows);
    writer.append(key->gi.useBlueNoise);
    noise(key->noise[1]);
    writer.append(key->reflection.enabled);
    writer.append(key->reflection.quality);
    writer.append(key->reflection.sampleCount);
    writer.append(key->reflection.bias);
    writer.append(key->reflection.maxDistance);
    writer.append(key->reflection.maxRoughness);
    writer.append(key->reflection.roughnessSpread);
    writer.append(key->reflection.shadowSamples);
    writer.append(key->reflection.useShadows);
    writer.append(key->reflection.useBlueNoise);
    noise(key->noise[2]);
    writer.append(key->shadows.enabled);
    writer.append(key->shadows.bias);
    writer.append(key->shadows.directionalDistance);
    writer.append(key->shadows.maxSamples);
    writer.append(key->temporal.enabled);
    writer.append(key->temporal.maxFrames);
    writer.append(key->temporal.freezeAtLimit);
    writer.append(key->temporal.absolutePositionThreshold);
    writer.append(key->temporal.relativePositionThreshold);
    writer.append(key->temporal.normalThreshold);
    std::apply([&](const auto&... value)
    {
        (writer.append(value), ...);
    }, key->denoise);
    key->bytes = writer.finish();
    return std::make_shared<SignatureSettings>(std::move(snapshot));
}

std::shared_ptr<const SignatureGeometry> geometry(const RenderQueue::Item& item,
    const std::shared_ptr<const SignatureGeometry>& previous)
{
    if (previous && previous->nodeId == item.nodeId && previous->submesh == item.submeshIndex &&
        previous->objectId == item.objectId && previous->mesh == item.drawMesh && previous->blas == item.tracingMesh &&
        previous->tracing == item.rayTracingEnabled)
    {
        hit("temporal.signatureGeometryHits");
        return previous;
    }
    auto next = std::make_shared<SignatureGeometry>();
    next->nodeId = item.nodeId;
    next->objectId = item.objectId;
    next->submesh = item.submeshIndex;
    next->mesh = item.drawMesh;
    next->blas = item.tracingMesh;
    next->tracing = item.rayTracingEnabled;
    Writer prefix;
    prefix.append(item.objectId);
    prefix.append(item.submeshIndex);
    prefix.append(item.nodeId.size());
    prefix.bytes.insert(prefix.bytes.end(), item.nodeId.begin(), item.nodeId.end());
    prefix.pointer(item.drawMesh.get());
    prefix.pointer(item.tracingMesh.get());
    next->prefix.bytes = prefix.finish();
    Writer inclusion;
    inclusion.append(item.rayTracingEnabled);
    next->inclusion.bytes = inclusion.finish();
    return next;
}

std::shared_ptr<const SignatureBytes> transform(const RenderQueue::Item& item,
    const std::shared_ptr<const SignatureBytes>& previous)
{
    if (previous && previous->bytes.size() == sizeof(item.worldMatrix) &&
        std::memcmp(previous->bytes.data(), &item.worldMatrix, sizeof(item.worldMatrix)) == 0)
    {
        hit("temporal.signatureTransformHits");
        return previous;
    }
    auto next = std::make_shared<SignatureBytes>();
    Writer writer;
    writer.append(item.worldMatrix);
    next->bytes = writer.finish();
    return next;
}

std::shared_ptr<const SignatureMaterial> material(const RenderQueue::Item& item)
{
    auto next = std::make_shared<SignatureMaterial>();
    next->source = item.drawMaterial;
    Writer identity, values, textures;
    identity.pointer(item.drawMaterial.get());
    values.append(item.materialData);
    for (size_t i = 0; i < item.textures.size(); ++i)
    {
        textures.pointer(item.textureImages[i]);
        textures.pointer(item.textureSamplers[i]);
        // Capture native owners separately from mutable source wrappers.
        if (item.textures[i])
        {
            auto image = item.textures[i]->imageResource();
            auto sampler = item.textures[i]->samplerResource();
            if (image.get() == item.textureImages[i])
            {
                next->images[i] = std::move(image);
            }
            if (sampler.get() == item.textureSamplers[i])
            {
                next->samplers[i] = std::move(sampler);
            }
        }
    }
    next->identity.bytes = identity.finish();
    next->values.bytes = values.finish();
    next->textures.bytes = textures.finish();
    base::CpuProfileScope::count("temporal.signatureMaterialRebuilds", 1);
    return next;
}
}

size_t SignatureMaterialHash::operator()(const SignatureMaterialKey& key) const
{
    size_t result = std::hash<const MaterialBase*>{}(key.material);
    const auto add = [&](size_t value)
    {
        result ^= value + size_t(0x9e3779b9u) + (result << 6) + (result >> 2);
    };
    add(std::hash<uint64_t>{}(key.values));
    add(std::hash<uint64_t>{}(key.bindings));
    add(std::hash<uint64_t>{}(key.classification));
    for (size_t i = 0; i < key.images.size(); ++i)
    {
        add(std::hash<gpu::Image*>{}(key.images[i]));
        add(std::hash<gpu::Sampler*>{}(key.samplers[i]));
        add(std::hash<uint64_t>{}(key.textureRevisions[i]));
        add(std::hash<uint64_t>{}(key.imageGenerations[i]));
    }
    return result;
}

std::shared_ptr<const TemporalSignatureState> TemporalSignatureCache::prepare(const RenderQueue& queue,
    const TemporalSignatureContext& context)
{
    base::CpuProfileScope profile("renderer.prepareTemporalSignatureComponents");
    TemporalSignatureState state;
    auto* next = &state;
    next->camera = camera(context, _latest ? _latest->camera : nullptr);
    next->environment = environmentState(context, _latest ? _latest->environment : nullptr);
    next->settings = settings(context, _latest ? _latest->settings : nullptr);
    if (_latest && _latest->lights->bytes.size() == sizeof(queue.lights()) &&
        std::memcmp(_latest->lights->bytes.data(), &queue.lights(), sizeof(queue.lights())) == 0)
    {
        next->lights = _latest->lights;
        hit("temporal.signatureLightHits");
    }
    else
    {
        auto lights = std::make_shared<SignatureBytes>();
        Writer writer;
        writer.append(queue.lights());
        lights->bytes = writer.finish();
        next->lights = lights;
    }
    bool stableInstances = _latest && queue.items().size() == _latest->instances.size() &&
        _materialKeys.size() == queue.items().size();
    if (context.correspondenceNeeded && _latest && !_correspondenceValid && stableInstances)
    {
        // The slow path still decides if membership changed to a valid collection.
        stableInstances = false;
    }
    for (size_t i = 0; stableInstances && i < queue.items().size(); ++i)
    {
        const auto& item = queue.items()[i];
        const auto& old = _latest->instances[i];
        const auto& geometry = *old.geometry;
        const auto& key = _materialKeys[i];
        const auto& revision = item.materialRevisions;
        stableInstances = geometry.nodeId == item.nodeId && geometry.objectId == item.objectId &&
            geometry.submesh == item.submeshIndex && geometry.mesh == item.drawMesh && geometry.blas == item.tracingMesh &&
            geometry.tracing == item.rayTracingEnabled && key.material == item.drawMaterial.get() &&
            key.values == revision.values && key.bindings == revision.bindings && key.classification == revision.classification &&
            old.transform->bytes.size() == sizeof(item.worldMatrix) &&
            std::memcmp(old.transform->bytes.data(), &item.worldMatrix, sizeof(item.worldMatrix)) == 0;
    }
    if (stableInstances)
    {
        const auto count = queue.items().size();
        base::CpuProfileScope::count("temporal.signatureComponentHits", count * 3);
        base::CpuProfileScope::count("temporal.signatureMaterialHits", count);
        base::CpuProfileScope::count("temporal.signatureGeometryHits", count);
        base::CpuProfileScope::count("temporal.signatureTransformHits", count);
        if (sameComponent(next->camera, _latest->camera) && sameComponent(next->environment, _latest->environment) &&
            sameComponent(next->settings, _latest->settings) && sameComponent(next->lights, _latest->lights))
        {
            base::CpuProfileScope::count("temporal.signatureStateHits", 1);
            return _latest;
        }
        // Settings/light-only changes share every instance component and keep the
        // existing correspondence/material maps; no per-object cache churn.
        next->instances = _latest->instances;
        _latest = std::make_shared<TemporalSignatureState>(std::move(state));
        return _latest;
    }
    std::vector<SignatureMaterialKey> materialKeys;
    materialKeys.reserve(queue.items().size());
    bool correspondenceValid = true;
    decltype(_instances) instances;
    decltype(_materials) materials;
    next->instances.reserve(queue.items().size());
    for (const auto& item : queue.items())
    {
        const Correspondence key{item.nodeId, item.submeshIndex};
        auto& bucket = instances[key];
        correspondenceValid &= !item.nodeId.empty() && bucket.empty();
        if (context.correspondenceNeeded && (item.nodeId.empty() || !bucket.empty()))
        {
            throw std::invalid_argument("Temporal correspondence requires unique nonempty node/submesh identities");
        }
        const auto found = _instances.find(key);
        const auto* old = found != _instances.end() && bucket.size() < found->second.size() ? &found->second[bucket.size()] : nullptr;
        SignatureInstance instance;
        instance.geometry = geometry(item, old ? old->geometry : nullptr);
        instance.transform = transform(item, old ? old->transform : nullptr);
        const auto& r = item.materialRevisions;
        const SignatureMaterialKey materialKey{item.drawMaterial.get(), r.values, r.bindings, r.classification,
            item.textureImages, item.textureSamplers, item.textureRevisions, item.imageGenerations};
        materialKeys.push_back(materialKey);
        const auto current = materials.find(materialKey);
        const auto previous = _materials.find(materialKey);
        if (current != materials.end())
        {
            instance.material = current->second;
            hit("temporal.signatureMaterialHits");
        }
        else if (previous != _materials.end())
        {
            instance.material = previous->second;
            materials.emplace(materialKey, instance.material);
            hit("temporal.signatureMaterialHits");
        }
        else
        {
            instance.material = material(item);
            // Dirty revisions with identical effective serialized values retain
            // the original policy; they do not invent an additional reset.
            if (old && sameMaterial(instance.material, old->material))
            {
                instance.material = old->material;
            }
            materials.emplace(materialKey, instance.material);
        }
        bucket.push_back(instance);
        next->instances.push_back(std::move(instance));
    }
    const bool sameState = _latest && next->equals(*_latest);
    auto published = sameState ? _latest : std::make_shared<const TemporalSignatureState>(std::move(state));
    // Allocate the final state before publishing any lookup/key collection.
    // Failed preparation leaves all previous caches mutually consistent.
    _instances.swap(instances);
    _materials.swap(materials);
    _materialKeys.swap(materialKeys);
    _correspondenceValid = correspondenceValid;
    _latest = std::move(published);
    if (sameState)
    {
        base::CpuProfileScope::count("temporal.signatureStateHits", 1);
    }
    return _latest;
}

bool TemporalSignatureState::equals(const TemporalSignatureState& other) const
{
    if (this == &other)
    {
        return true;
    }
    if (!sameComponent(camera, other.camera) || !sameComponent(lights, other.lights) ||
        !sameComponent(environment, other.environment) || !sameComponent(settings, other.settings) || instances.size() != other.instances.size())
    {
        return false;
    }
    for (size_t i = 0; i < instances.size(); ++i)
    {
        const auto& a = instances[i];
        const auto& b = other.instances[i];
        if (!sameGeometry(a.geometry, b.geometry) || !sameComponent(a.transform, b.transform) || !sameMaterial(a.material, b.material))
        {
            return false;
        }
    }
    return true;
}

void TemporalSignatureState::countResetCauses(const TemporalSignatureState* previous) const
{
    if (!previous)
    {
        base::CpuProfileScope::count("temporal.reset.initialSignature", 1);
        return;
    }
    if (!sameComponent(camera, previous->camera))
    {
        base::CpuProfileScope::count("temporal.reset.frameConfiguration", 1);
    }
    if (!sameComponent(lights, previous->lights))
    {
        base::CpuProfileScope::count("temporal.reset.lights", 1);
    }
    if (!sameComponent(environment, previous->environment))
    {
        base::CpuProfileScope::count("temporal.reset.environment", 1);
    }
    if (!sameComponent(settings, previous->settings))
    {
        base::CpuProfileScope::count("temporal.reset.settings", 1);
    }
    bool membership = instances.size() != previous->instances.size(), geometryChanged = false, motion = false, materialChanged = false;
    for (size_t i = 0; i < std::min(instances.size(), previous->instances.size()); ++i)
    {
        const auto& a = instances[i];
        const auto& b = previous->instances[i];
        membership |= a.geometry->nodeId != b.geometry->nodeId || a.geometry->submesh != b.geometry->submesh ||
            a.geometry->objectId != b.geometry->objectId;
        geometryChanged |= !sameGeometry(a.geometry, b.geometry);
        motion |= !sameComponent(a.transform, b.transform);
        materialChanged |= !sameMaterial(a.material, b.material);
    }
    if (membership)
    {
        base::CpuProfileScope::count("temporal.reset.membershipOrOrder", 1);
    }
    if (geometryChanged)
    {
        base::CpuProfileScope::count("temporal.reset.geometryOrInclusion", 1);
    }
    if (motion)
    {
        base::CpuProfileScope::count("temporal.reset.objectMotion", 1);
    }
    if (materialChanged)
    {
        base::CpuProfileScope::count("temporal.reset.materialOrTextures", 1);
    }
}

const std::vector<uint8_t>& TemporalSignatureState::flattened() const
{
    if (_flattened)
    {
        return _bytes;
    }
    std::vector<uint8_t> bytes;
    const auto append = [&](const SignatureBytes& component)
    {
        bytes.insert(bytes.end(), component.bytes.begin(), component.bytes.end());
    };
    append(*camera);
    for (const auto& item : instances)
    {
        append(item.geometry->prefix);
        append(item.material->identity);
        append(*item.transform);
        append(item.material->values);
        append(item.geometry->inclusion);
        append(item.material->textures);
    }
    append(*lights);
    append(*environment);
    append(*settings);
    base::CpuProfileScope::count("temporal.signatureFlattenedBytes", bytes.size(), "bytes");
    _bytes = std::move(bytes);
    _flattened = true;
    return _bytes;
}
}
