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

#include "AppDelegate.hpp"
#include "command/scene/TransformInteraction.hpp"

#include <bg2e/scene/PolarTransformController.hpp>
#include <bg2e/scene/FixedScaleTransformController.hpp>
#include <utility>
#include <typeinfo>
#include <bg2e/manipulation/SelectableComponent.hpp>
#include <stdexcept>

namespace bg2e_composer {

void AppDelegate::initializeTransformInteractions()
{
    const auto weak = weak_from_this();
    _selection->setTransformEditObserver([weak](bg2e::manipulation::TransformEditPhase phase,
        const std::shared_ptr<bg2e::scene::Node>& node, const glm::mat4&, const glm::mat4&)
    {
        using namespace bg2e::app;
        using bg2e::manipulation::TransformEditPhase;
        auto owner = weak.lock();
        if (!owner || !owner->document() || !owner->document()->commands)
        {
            return false;
        }
        auto commands = owner->document()->commands;
        auto& history = commands->history();
        auto* previousRenderer = owner->_commandRenderer;
        owner->_commandRenderer = owner->renderer();
        struct RestoreRenderer {
            bg2e::draw::Renderer*& slot;
            bg2e::draw::Renderer* previous;
            ~RestoreRenderer()
            {
                slot = previous;
            }
        } scope{owner->_commandRenderer, previousRenderer};
        if (phase == TransformEditPhase::Begin)
        {
            if (!node || owner->_pendingIo || owner->_imports.busy() || owner->sceneLoading() ||
                commands->pending() || history.transactionActive() || history.faulted())
            {
                return false;
            }
            // This writer has no pause/base-state adapter yet. Reject rather
            // than race an evaluated scale against authored transform edits.
            if (node->getComponent<bg2e::scene::FixedScaleTransformControllerComponent>())
            {
                owner->_status = "Fixed-scale controller does not support transform editing";
                return false;
            }
            // Unknown/custom update writers require their own authored-state
            // adapter. Exact types prevent an overriding subclass slipping in.
            for (const auto& entry : node->components())
            {
                using namespace bg2e::scene;
                const auto* component = entry.second.get();
                const auto& type = typeid(*component);
                if (type != typeid(TransformComponent) && type != typeid(DrawableComponent) &&
                    type != typeid(CameraComponent) && type != typeid(LightComponent) &&
                    type != typeid(EnvironmentComponent) && type != typeid(OrbitCameraComponent) &&
                    type != typeid(PolarTransformControllerComponent) &&
                    type != typeid(bg2e::manipulation::GizmoComponent) &&
                    type != typeid(bg2e::manipulation::SelectableComponent))
                {
                    owner->_status = "A component requires a dedicated transform editing adapter";
                    return false;
                }
            }
            const auto generation = owner->document()->generation;
            const auto target = std::weak_ptr<bg2e::scene::Node>(node);
            const auto stage = std::weak_ptr<StageScene>(owner->_stage);
            auto transaction = std::make_unique<command::scene::TransformInteraction>(target,
                [stage, target, generation]
                {
                    auto current = stage.lock();
                    auto node = target.lock();
                    return current && node && current->document().generation == generation &&
                        (current->contains(node.get()) || node == current->fillLightNode());
                }, [weak]
                {
                    auto owner = weak.lock();
                    if (!owner || !owner->_commandRenderer)
                    {
                        throw std::logic_error("Transform effects require a safe boundary");
                    }
                    command::Effects effects;
                    effects.lightmapSources = true;
                    effects.renderHistories = true;
                    owner->applyCommandEffects(*owner->_commandRenderer, effects);
                });
            // Stop prepared bake publication before opening an authored preview.
            owner->stopLightmapBake(true);
            // Allocate resume callbacks before pausing any writer or beginning.
            std::vector<std::function<void()>> resume;
            auto navigation = owner->_stage->orbitCamera();
            if (navigation && navigation->enabled())
            {
                resume.push_back([navigation = std::static_pointer_cast<bg2e::scene::OrbitCameraComponent>(navigation->shared_from_this())]
                {
                    navigation->setEnabled(true);
                });
            }
            if (auto orbit = node->getComponent<bg2e::scene::OrbitCameraComponent>();
                orbit && orbit != navigation && orbit->enabled())
            {
                resume.push_back([orbit = std::static_pointer_cast<bg2e::scene::OrbitCameraComponent>(orbit->shared_from_this())]
                {
                    orbit->setEnabled(true);
                });
            }
            if (auto polar = node->getComponent<bg2e::scene::PolarTransformControllerComponent>(); polar && polar->enabled())
            {
                resume.push_back([polar = std::static_pointer_cast<bg2e::scene::PolarTransformControllerComponent>(polar->shared_from_this())]
                {
                    polar->setEnabled(true);
                });
            }
            const auto result = commands->beginCpuInteraction(std::move(transaction));
            if (!result.accepted())
            {
                owner->_status = std::string(result.message);
                return false;
            }
            owner->_resumeTransformWriters = std::move(resume);
            if (navigation)
            {
                navigation->setEnabled(false);
            }
            if (auto orbit = node->getComponent<bg2e::scene::OrbitCameraComponent>())
            {
                orbit->setEnabled(false);
            }
            if (auto polar = node->getComponent<bg2e::scene::PolarTransformControllerComponent>())
            {
                polar->setEnabled(false);
            }
            owner->_transformTransaction = history.transactionId();
            owner->_status.clear();
            return true;
        }
        if (!owner->_transformTransaction || history.transactionId() != owner->_transformTransaction)
        {
            return false;
        }
        CommandResult result;
        if (phase == TransformEditPhase::Update)
        {
            result = history.updateTransaction({});
        }
        else
        {
            result = phase == TransformEditPhase::Commit ? history.commitTransaction() : history.cancelTransaction();
            if (result.accepted())
            {
                owner->_transformTransaction = 0;
                for (const auto& resume : owner->_resumeTransformWriters)
                {
                    resume();
                }
                owner->_resumeTransformWriters.clear();
            }
        }
        if (!result.accepted())
        {
            owner->_status = std::string(result.message);
            if (phase == TransformEditPhase::Cancel)
            {
                // The SDK has already restored its initial matrix. A failed
                // effects/history restoration cannot remain a committable preview.
                history.reportExternalFault();
            }
        }
        owner->_stage->synchronizeDirty();
        owner->sampleNavigation(false);
        owner->requestSceneFrame();
        return result.accepted();
    });
}


void AppDelegate::focusLost()
{
    queueInput([this]
    {
        if (_settleUiInteractions)
        {
            _settleUiInteractions(false);
        }
        finishTransformInteraction(false);
    });
}


void AppDelegate::finishTransformInteraction(bool commit)
{
    if (_selection)
    {
        if (commit)
        {
            _selection->commitTransform();
        }
        else
        {
            _selection->cancelTransform();
        }
    }
}

}
