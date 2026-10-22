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

#include "TemporalCorrespondence.hpp"
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace bg2e::draw {
namespace {
bool sameGeometry(const SignatureGeometry& a, const SignatureGeometry& b)
{
    // Extraction objectId/order is deliberately not persistent surface identity.
    return a.mesh == b.mesh && a.blas == b.blas && a.tracing == b.tracing;
}

bool sameMaterial(const SignatureMaterial& a, const SignatureMaterial& b)
{
    return a.identity.bytes == b.identity.bytes && a.values.bytes == b.values.bytes &&
        a.textures.bytes == b.textures.bytes;
}
}

std::shared_ptr<const TemporalCorrespondenceFrame> TemporalCorrespondence::prepare(
    const TemporalSignatureState& signature, uint64_t sequence, uint64_t generation)
{
    auto next = std::make_shared<TemporalCorrespondenceFrame>();
    next->sequence = sequence;
    next->generation = generation;
    std::map<TemporalInstanceKey,size_t> multiplicity;
    for (const auto& instance : signature.instances)
    {
        ++multiplicity[{instance.geometry->nodeId,instance.geometry->submesh}];
    }
    decltype(_identities) identities;
    auto nextToken = _nextToken;
    for (const auto& instance : signature.instances)
    {
        const TemporalInstanceKey key{instance.geometry->nodeId,instance.geometry->submesh};
        if (key.first.empty() || multiplicity.at(key) != 1)
        {
            // Signature preparation already diagnoses these when correspondence
            // is required. Otherwise preserve Phase 0 admission, without inventing
            // a token or selecting an arbitrary instance from an ambiguous bucket.
            next->identitiesValid = false;
            continue;
        }
        if (!instance.transform || instance.transform->bytes.size() != sizeof(glm::mat4))
        {
            throw std::logic_error("Temporal correspondence requires a captured world matrix");
        }
        const auto found = _identities.find(key);
        TemporalInstanceRecord record;
        record.geometry = instance.geometry;
        record.material = instance.material;
        std::memcpy(&record.world,instance.transform->bytes.data(),sizeof(record.world));
        record.previousWorld = record.world;
        record.newInstance = found == _identities.end();
        record.geometryChanged = !record.newInstance && !sameGeometry(*record.geometry,*found->second.geometry);
        record.materialChanged = !record.newInstance && !sameMaterial(*record.material,*found->second.material);
        if (record.newInstance || record.geometryChanged || record.materialChanged)
        {
            if (nextToken > std::numeric_limits<uint32_t>::max())
            {
                throw std::overflow_error("Temporal surface tokens exhausted; drain and recreate the renderer lifecycle");
            }
            record.token = uint32_t(nextToken++);
        }
        else
        {
            record.token = found->second.token;
        }
        identities.emplace(key,Identity{record.geometry,record.material,record.token});
        next->instances.emplace(key,std::move(record));
    }
    // Failed preparation publishes neither partial identity maps nor allocations.
    // Removed/ambiguous keys lose their identity, but their tokens are never reused.
    _identities.swap(identities);
    _nextToken = nextToken;
    return next;
}

std::shared_ptr<const TemporalCorrespondenceFrame> TemporalCorrespondence::record(
    const TemporalCorrespondenceFrame& prepared, uint64_t sequence, uint64_t generation,
    std::shared_ptr<const gpu::detail::RecordingCompletion> recording) const
{
    if (prepared.sequence != sequence || !recording)
    {
        throw std::logic_error("Temporal correspondence requires its prepared scene sequence and active recording");
    }
    auto next = std::make_shared<TemporalCorrespondenceFrame>(prepared);
    next->generation = generation;
    next->recording = std::move(recording);
    const bool consecutive = prepared.generation == generation && _recorded &&
        _recorded->generation == generation && _recorded->recording && _recorded->recording->wasSubmitted() &&
        _recorded->sequence < sequence && sequence-_recorded->sequence == 1;
    for (auto& [key, instance] : next->instances)
    {
        // Never use the slot's last occupant or the latest prepared update as
        // previous scene state. Unknown/discarded chronology rejects locally.
        instance.moved = instance.previousValid = instance.correspondenceValid = false;
        instance.previousWorld = instance.world;
        if (!consecutive)
        {
            continue;
        }
        const auto found = _recorded->instances.find(key);
        if (found == _recorded->instances.end())
        {
            continue;
        }
        const auto& previous = found->second;
        instance.geometryChanged |= !sameGeometry(*instance.geometry,*previous.geometry);
        instance.materialChanged |= !sameMaterial(*instance.material,*previous.material);
        instance.previousValid = instance.token == previous.token &&
            !instance.geometryChanged && !instance.materialChanged;
        if (instance.previousValid)
        {
            instance.previousWorld = previous.world;
            instance.moved = std::memcmp(&instance.world,&previous.world,sizeof(instance.world)) != 0;
            // Phase 1 accepts only static receivers. Phase 2 may reuse the valid
            // previous matrix for moving receivers without changing their token.
            instance.correspondenceValid = !instance.moved;
        }
    }
    return next;
}

void TemporalCorrespondence::publish(std::shared_ptr<const TemporalCorrespondenceFrame> recorded)
{
    _recorded = std::move(recorded);
}

void TemporalCorrespondence::invalidate()
{
    // Global policy resets invalidate chronology, not persistent token allocation.
    _recorded.reset();
}
}
