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
#include <stdexcept>

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
    auto first = correspondence.record(*prepared,0,1,firstTicket);
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
    auto second = correspondence.record(*secondPrepared,1,1,secondTicket);
    require(second->instances.at({"wall",0}).token == wallToken && second->instances.at({"wall",0}).correspondenceValid,
        "Insertion/extraction-ID shifts must preserve the static wall token and submitted predecessor");
    const auto& moving = second->instances.at({"foreground",0});
    require(moving.token == foregroundToken && moving.previousValid && moving.moved && !moving.correspondenceValid &&
        moving.previousWorld == glm::mat4(1),"Movement must preserve identity but reject locally using recorded previous world");
    require(second->instances.at({"foreground",1}).correspondenceValid && waits == 0 && !submission->completed(),
        "Independent static submesh must survive an incomplete predecessor without waiting");
    require(first->instances.at({"foreground",0}).world == glm::mat4(1),"Retained recorded snapshot must stay immutable");
    correspondence.publish(second);
    secondTicket->abandon();
    auto thirdPrepared = correspondence.prepare(signature,2,1);
    auto third = correspondence.record(*thirdPrepared,2,1,std::make_shared<gpu::detail::RecordingCompletion>());
    require(!third->instances.at({"wall",0}).previousValid,"Abandoned scene must never become a valid predecessor");
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
}
