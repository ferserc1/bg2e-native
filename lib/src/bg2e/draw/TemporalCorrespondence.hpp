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

#include "TemporalSignatureCache.hpp"
#include <bg2e/gpu/detail/SubmissionState.hpp>
#include <map>

namespace bg2e::draw {
using TemporalInstanceKey = std::pair<std::string,uint32_t>;

struct TemporalInstanceRecord {
    std::shared_ptr<const SignatureGeometry> geometry;
    std::shared_ptr<const SignatureMaterial> material;
    glm::mat4 world{1}, previousWorld{1};
    uint32_t token = 0;
    // Preparation records identity changes even when a global reset removes
    // the recorded predecessor. Recording additionally checks captured resources.
    bool newInstance = true, geometryChanged = false, materialChanged = false;
    bool moved = false, previousValid = false, correspondenceValid = false;
};

// Renderer-private immutable values. Preparation never advances scene chronology.
// The recorded version retains the exact command recording that owns this scene.
struct TemporalCorrespondenceFrame {
    std::map<TemporalInstanceKey,TemporalInstanceRecord> instances;
    uint64_t sequence = 0, generation = 0;
    bool identitiesValid = true;
    std::shared_ptr<const gpu::detail::RecordingCompletion> recording;
};

class TemporalCorrespondence {
  public:
    std::shared_ptr<const TemporalCorrespondenceFrame> prepare(const TemporalSignatureState& signature,
        uint64_t sequence, uint64_t generation);

    std::shared_ptr<const TemporalCorrespondenceFrame> record(const TemporalCorrespondenceFrame& prepared,
        uint64_t sequence, uint64_t generation,
        std::shared_ptr<const gpu::detail::RecordingCompletion> recording) const;
    void publish(std::shared_ptr<const TemporalCorrespondenceFrame> recorded);
    void invalidate();

  private:
    struct Identity {
        std::shared_ptr<const SignatureGeometry> geometry;
        std::shared_ptr<const SignatureMaterial> material;
        uint32_t token = 0;
    };
    std::map<TemporalInstanceKey,Identity> _identities;
    std::shared_ptr<const TemporalCorrespondenceFrame> _recorded;
    // UINT32_MAX+1 is an exhausted sentinel, never a usable/recycled token.
    // Reset only by destroying this state during drained renderer cleanup.
    uint64_t _nextToken = 1;
};
}
