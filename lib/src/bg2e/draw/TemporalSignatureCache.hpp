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

#include <bg2e/draw/RendererDeferred.hpp>
#include <map>
#include <unordered_map>

namespace bg2e::draw {
struct TemporalSignatureContext {
    const RendererConfiguration& configuration;
    const glm::mat4& projection;
    bool explicitCamera;
    const void* cameraIdentity;
    std::shared_ptr<scene::Component> cameraOwner;
    std::shared_ptr<const environment::EnvironmentResources> environment;
    bool correspondenceNeeded;
};

struct SignatureBytes {
    std::vector<uint8_t> bytes;
};
struct SignatureMaterialKey {
    const MaterialBase* material = nullptr;
    uint64_t values = 0, bindings = 0, classification = 0;
    std::array<gpu::Image*,6> images{};
    std::array<gpu::Sampler*,6> samplers{};
    std::array<uint64_t,6> textureRevisions{}, imageGenerations{};
    bool operator==(const SignatureMaterialKey&) const = default;
};
struct SignatureMaterialHash {
    size_t operator()(const SignatureMaterialKey& key) const;
};
struct SignatureMaterial {
    std::shared_ptr<MaterialBase> source;
    std::array<std::shared_ptr<gpu::Image>,6> images;
    std::array<std::shared_ptr<gpu::Sampler>,6> samplers;
    SignatureBytes identity, values, textures;
};
struct SignatureGeometry {
    std::string nodeId;
    uint32_t objectId = 0, submesh = 0;
    std::shared_ptr<Drawable::DrawMesh> mesh;
    std::shared_ptr<gpu::RayTracingMesh> blas;
    bool tracing = false;
    SignatureBytes prefix, inclusion;
};
struct SignatureInstance {
    std::shared_ptr<const SignatureGeometry> geometry;
    std::shared_ptr<const SignatureBytes> transform;
    std::shared_ptr<const SignatureMaterial> material;
};
struct SignatureCamera;
struct SignatureEnvironment;
struct SignatureSettings;

// Immutable component graph. Flattening is lazy for explicit fingerprints and
// diagnostic comparison only; normal invalidation compares retained components.
class TemporalSignatureState {
  public:
    std::shared_ptr<const SignatureCamera> camera;
    std::shared_ptr<const SignatureBytes> lights;
    std::shared_ptr<const SignatureEnvironment> environment;
    std::shared_ptr<const SignatureSettings> settings;
    std::vector<SignatureInstance> instances;

    bool equals(const TemporalSignatureState& other) const;
    void countResetCauses(const TemporalSignatureState* previous) const;
    const std::vector<uint8_t>& flattened() const;

  private:
    mutable bool _flattened = false;
    mutable std::vector<uint8_t> _bytes;
};

class TemporalSignatureCache {
  public:
    std::shared_ptr<const TemporalSignatureState> prepare(const RenderQueue& queue,
        const TemporalSignatureContext& context);

  private:
    // Stable correspondence is node/submesh. Buckets preserve the legacy policy
    // when correspondence is not required and repeated identities are admitted.
    using Correspondence = std::pair<std::string,uint32_t>;
    std::map<Correspondence,std::vector<SignatureInstance>> _instances;
    std::unordered_map<SignatureMaterialKey,std::shared_ptr<const SignatureMaterial>,SignatureMaterialHash> _materials;
    std::shared_ptr<const TemporalSignatureState> _latest;
    std::vector<SignatureMaterialKey> _materialKeys;
    bool _correspondenceValid = true;
};
}
