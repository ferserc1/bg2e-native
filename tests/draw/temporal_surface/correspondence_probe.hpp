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
// Repository-internal fixture; build with the private correspondence implementation.
#include "../../../lib/src/bg2e/draw/TemporalCorrespondence.hpp"
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

namespace bg2e::draw::tests {
inline void runTemporalCorrespondenceProbe()
{
    auto require = [](bool value, const char* message)
    {
        if (!value)
        {
            throw std::runtime_error(message);
        }
    };
    auto instance = [](std::string node, uint32_t submesh, uint32_t object, glm::mat4 world = glm::mat4(1))
    {
        auto geometry = std::make_shared<SignatureGeometry>();
        geometry->nodeId = std::move(node);
        geometry->submesh = submesh;
        geometry->objectId = object;
        auto transform = std::make_shared<SignatureBytes>();
        transform->bytes.resize(sizeof(world));
        std::memcpy(transform->bytes.data(),&world,sizeof(world));
        return SignatureInstance{geometry,transform,std::make_shared<SignatureMaterial>()};
    };
    TemporalCorrespondence correspondence;
    TemporalSignatureState signature;
    signature.instances = {instance("wall",0,0),instance("foreground",0,1),instance("foreground",1,2)};
    const auto prepared = correspondence.prepare(signature,0,1);
    const auto wallToken = prepared->instances.at({"wall",0}).token;
    const auto foregroundToken = prepared->instances.at({"foreground",0}).token;
    require(wallToken && foregroundToken && wallToken != foregroundToken &&
        foregroundToken != prepared->instances.at({"foreground",1}).token,"Submeshes must have independent nonzero tokens");
    unsigned waits = 0;
    auto firstTicket = std::make_shared<gpu::detail::RecordingCompletion>();
    TemporalCameraRecord firstCamera;
    firstCamera.extent = {101,73};
    firstCamera.transforms.jitteredProjection[2][0] = 0.01f;
    auto first = correspondence.record(*prepared,0,1,firstTicket,firstCamera);
    correspondence.publish(first);
    auto submission = std::make_shared<gpu::detail::CompletionRecord>([]
    {
        return false;
    },[&]
    {
        ++waits;
    });
    firstTicket->submitted(submission); // Submitted, deliberately not complete.
    // Prepare a future update without recording it; it cannot become the predecessor.
    glm::mat4 futureWorld(1);
    futureWorld[3].x = 4;
    auto future = signature;
    future.instances[1] = instance("foreground",0,1,futureWorld);
    correspondence.prepare(future,1,1);
    signature.instances.insert(signature.instances.begin(),instance("inserted",0,0));
    signature.instances[1] = instance("wall",0,1);
    glm::mat4 moved(1);
    moved[3].x = 0.25f;
    signature.instances[2] = instance("foreground",0,2,moved);
    signature.instances[3] = instance("foreground",1,3);
    auto secondPrepared = correspondence.prepare(signature,1,1);
    auto secondTicket = std::make_shared<gpu::detail::RecordingCompletion>();
    auto secondCamera = firstCamera;
    secondCamera.transforms.view[3].x = -0.5f;
    secondCamera.transforms.jitteredProjection[2][0] = -0.01f;
    auto second = correspondence.record(*secondPrepared,1,1,secondTicket,secondCamera);
    require(second->previousCameraValid && second->previousCamera.transforms.view == firstCamera.transforms.view &&
        second->previousCamera.transforms.jitteredProjection == firstCamera.transforms.jitteredProjection &&
        second->camera.transforms.view == secondCamera.transforms.view && second->previousRecording == firstTicket,
        "Camera snapshots must follow the submitted scene, including both projections and completion owner");
    require(first->camera.transforms.view == firstCamera.transforms.view,
        "Later camera updates must not mutate retained recordings");
    auto gapPrepared = correspondence.prepare(signature,3,1);
    auto gap = correspondence.record(*gapPrepared,3,1,std::make_shared<gpu::detail::RecordingCompletion>(),secondCamera);
    require(!gap->previousCameraValid && !gap->instances.at({"wall",0}).previousValid,
        "Scene sequence gaps must reject previous surfaces");
    auto badCamera = secondCamera;
    badCamera.transforms.view[0][0] = std::numeric_limits<float>::quiet_NaN();
    auto invalidCamera = correspondence.record(*secondPrepared,1,1,
        std::make_shared<gpu::detail::RecordingCompletion>(),badCamera);
    require(!invalidCamera->previousCameraValid && !invalidCamera->instances.at({"wall",0}).previousValid,
        "Invalid cameras must reject locally");
    auto invalidSignature = signature;
    glm::mat4 singular(1);
    singular[0][0] = 0;
    invalidSignature.instances[2] = instance("foreground",0,2,singular);
    auto singularPrepared = correspondence.prepare(invalidSignature,1,1);
    auto singularFrame = correspondence.record(*singularPrepared,1,1,
        std::make_shared<gpu::detail::RecordingCompletion>(),secondCamera);
    require(singularFrame->previousCameraValid && !singularFrame->instances.at({"foreground",0}).previousValid &&
        singularFrame->instances.at({"wall",0}).previousValid,
        "Singular instance matrices must reject only that receiver");
    require(second->instances.at({"wall",0}).token == wallToken && second->instances.at({"wall",0}).correspondenceValid,
        "Insertion/extraction-ID shifts must preserve the static wall token and submitted predecessor");
    const auto& moving = second->instances.at({"foreground",0});
    require(moving.token == foregroundToken && moving.previousValid && moving.moved && !moving.correspondenceValid &&
        moving.previousWorld == glm::mat4(1),"Movement must preserve identity but reject locally using recorded previous world");
    require(second->instances.at({"foreground",1}).correspondenceValid && waits == 0 && !submission->completed(),
        "Independent static submesh must survive an incomplete predecessor without waiting");
    require(first->instances.at({"foreground",0}).world == glm::mat4(1),"Retained recorded snapshot must stay immutable");
    correspondence.publish(second);
    auto unsubmittedPrepared = correspondence.prepare(signature,2,1);
    auto unsubmitted = correspondence.record(*unsubmittedPrepared,2,1,
        std::make_shared<gpu::detail::RecordingCompletion>(),secondCamera);
    require(!unsubmitted->previousCameraValid && !unsubmitted->previousRecording,
        "A different unsubmitted recording must not supply previous camera state");
    secondTicket->abandon();
    auto thirdPrepared = correspondence.prepare(signature,2,1);
    auto third = correspondence.record(*thirdPrepared,2,1,std::make_shared<gpu::detail::RecordingCompletion>());
    require(!third->previousCameraValid && !third->instances.at({"wall",0}).previousValid,"Abandoned scene must never become a valid predecessor");
    correspondence.invalidate();
    auto invalidated = correspondence.record(*thirdPrepared,2,1,std::make_shared<gpu::detail::RecordingCompletion>());
    require(!invalidated->instances.at({"wall",0}).previousValid && invalidated->instances.at({"wall",0}).token == wallToken,
        "Reset must discard chronology without recycling tokens");
    auto changedMaterial = std::make_shared<SignatureMaterial>();
    changedMaterial->values.bytes.push_back(1);
    signature.instances[1].material = changedMaterial;
    const auto changed = correspondence.prepare(signature,3,2);
    require(changed->instances.at({"wall",0}).token != wallToken,"Effective material edits must replace the token");
    signature.instances.erase(signature.instances.begin()+1);
    correspondence.prepare(signature,4,2);
    signature.instances.push_back(instance("wall",0,4));
    const auto reinserted = correspondence.prepare(signature,5,3);
    require(reinserted->instances.at({"wall",0}).token != wallToken &&
        reinserted->instances.at({"wall",0}).token != changed->instances.at({"wall",0}).token,
        "Removal/reinsertion must not recycle historical tokens");
}

// CPU submission-ticket model only: slot rings do not assert native GPU overlap.
inline void runTemporalCorrespondenceChronologyProbe()
{
    const auto require = [](bool value, const char* message)
    {
        if (!value)
        {
            throw std::runtime_error(message);
        }
    };
    const auto makeInstance = [](const char* node, float x, bool revised = false)
    {
        auto geometry = std::make_shared<SignatureGeometry>();
        geometry->nodeId = node;
        geometry->tracing = revised; // Isolated captured geometry revision, no RT device needed.
        glm::mat4 world(1);
        world[3].x = x;
        auto transform = std::make_shared<SignatureBytes>();
        transform->bytes.resize(sizeof(world));
        std::memcpy(transform->bytes.data(),&world,sizeof(world));
        return SignatureInstance{geometry,transform,std::make_shared<SignatureMaterial>()};
    };
    for (const uint32_t slots : {2u,3u})
    {
        TemporalCorrespondence chronology;
        TemporalCameraRecord camera;
        camera.extent = {101,73};
        std::vector<std::shared_ptr<const TemporalCorrespondenceFrame>> ring(slots);
        std::shared_ptr<const TemporalCorrespondenceFrame> first, last;
        unsigned waits = 0;
        uint32_t originalToken = 0;
        for (uint64_t sequence = 0; sequence < 2*slots; ++sequence)
        {
            TemporalSignatureState signature;
            signature.instances = {makeInstance("wall",0),makeInstance("mover",float(sequence)*0.1f,sequence >= 3)};
            if (sequence >= 2)
            {
                signature.instances.push_back(makeInstance("new receiver",0.2f));
            }
            auto ghost = signature;
            ghost.instances[1] = makeInstance("mover",99,sequence >= 3);
            chronology.prepare(ghost,sequence+1,1); // Update-only work must not publish camera/model state.
            const auto prepared = chronology.prepare(signature,sequence,1);
            camera.transforms.view[3].x = -0.01f*float(sequence);
            camera.transforms.jitteredProjection[3].x = sequence%2 ? 0.01f : -0.01f;
            auto ticket = std::make_shared<gpu::detail::RecordingCompletion>();
            const auto recorded = chronology.record(*prepared,sequence,1,ticket,camera);
            if (sequence == 0)
            {
                first = recorded;
                originalToken = recorded->instances.at({"mover",0}).token;
                require(!recorded->previousCameraValid,"First scene must have no previous submitted camera");
            }
            else
            {
                require(recorded->previousCameraValid && recorded->previousRecording == last->recording &&
                    recorded->previousCamera.transforms.view == last->camera.transforms.view &&
                    recorded->instances.at({"wall",0}).previousValid,
                    "Two/three-slot reuse must follow the preceding submitted scene, not the prior slot occupant");
                const auto& mover = recorded->instances.at({"mover",0});
                if (sequence == 3)
                {
                    require(!mover.previousValid && mover.token != originalToken,
                        "Captured incompatible geometry must replace tokens and reject only that correspondence");
                }
                else
                {
                    require(mover.previousValid && mover.previousWorld == last->instances.at({"mover",0}).world &&
                        mover.previousWorld[3].x != 99,
                        "Moving matrices must come from submitted snapshots rather than later preparation");
                }
                if (sequence == 2)
                {
                    require(!recorded->instances.at({"new receiver",0}).previousValid,
                        "A newly introduced instance must not inherit an unrelated predecessor");
                }
            }
            chronology.publish(recorded);
            ticket->submitted(std::make_shared<gpu::detail::CompletionRecord>([]
            {
                return false;
            },[&]
            {
                ++waits;
            })); // All predecessors deliberately remain incomplete in this model.
            ring[sequence%slots] = recorded;
            last = recorded;
        }
        require(first->sequence == 0 && first->instances.at({"mover",0}).world == glm::mat4(1) && waits == 0,
            "Retained camera/model snapshots must survive slot reuse without waits");
        TemporalSignatureState signature;
        signature.instances = {makeInstance("wall",0),makeInstance("mover",1,true),makeInstance("new receiver",0.2f)};
        const auto sequence = last->sequence+1;
        auto prepared = chronology.prepare(signature,sequence,1);
        auto changedCamera = camera;
        changedCamera.extent = {103,75};
        require(!chronology.record(*prepared,sequence,1,std::make_shared<gpu::detail::RecordingCompletion>(),changedCamera)->previousCameraValid,
            "Changed receiver extent must reject camera chronology");
        changedCamera = camera;
        changedCamera.transforms.projection[0][0] = 1.1f;
        require(!chronology.record(*prepared,sequence,1,std::make_shared<gpu::detail::RecordingCompletion>(),changedCamera)->previousCameraValid,
            "Projection changes must reject camera chronology");
        auto abandonedTicket = std::make_shared<gpu::detail::RecordingCompletion>();
        chronology.publish(chronology.record(*prepared,sequence,1,abandonedTicket,camera));
        abandonedTicket->abandon();
        prepared = chronology.prepare(signature,sequence+1,1);
        require(!chronology.record(*prepared,sequence+1,1,std::make_shared<gpu::detail::RecordingCompletion>(),camera)->previousCameraValid,
            "Abandoned recordings cannot become predecessors even after slot reuse");
        chronology.publish(last);
        prepared = chronology.prepare(signature,sequence+2,1);
        require(!chronology.record(*prepared,sequence+2,1,std::make_shared<gpu::detail::RecordingCompletion>(),camera)->previousCameraValid,
            "Skipped rendered scene sequences must reject previous correspondence");
        chronology.invalidate();
        require(!chronology.record(*prepared,sequence+2,1,std::make_shared<gpu::detail::RecordingCompletion>(),camera)->previousCameraValid,
            "Explicit cuts must invalidate chronology without waiting");
    }
}
}
