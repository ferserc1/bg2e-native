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

#include <bg2e/app/Uv2SafeReload.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/bake/Uv2Reload.hpp>
#include <bg2e/draw/DefaultRenderLoopDelegate.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <stdexcept>

namespace bg2e::app {
std::shared_ptr<SafeUpdateToken> Uv2SafeReload::regenerateDraw(
    std::weak_ptr<draw::DefaultRenderLoopDelegate> weakDelegate,
    std::weak_ptr<scene::Node> weakNode, const geo::Uv2AtlasOptions& options,
    CompletionCallback onComplete, std::function<void()> beforeChange)
{
    auto token = std::make_shared<SafeUpdateToken>();
    auto delegate = weakDelegate.lock();
    auto node = weakNode.lock();
    auto report = [onComplete](const Uv2RegenerationResult& result)
    {
        if (onComplete)
        {
            onComplete(result);
        }
    };
    auto* component = node ? node->drawable() : nullptr;
    auto drawable = component ? component->drawDrawable() : nullptr;
    if (!delegate || !node || !delegate->scene() || node->sceneRoot() != delegate->scene().get() ||
        !drawable || !drawable->isLoaded())
    {
        report({false,"UV2 draw reload requires a live delegate and loaded target in its scene"});
        return token;
    }
    const std::weak_ptr<SafeUpdateToken> cancel = token;
    const std::weak_ptr<scene::Node> expectedRoot = delegate->scene();
    const std::weak_ptr<draw::Drawable> expectedDrawable = drawable;
    const std::weak_ptr<draw::Drawable::DrawMesh> expectedMesh = drawable->drawMesh();
    delegate->enqueueRendererUpdate(delegate->rendererGeneration(),
        [weakDelegate,weakNode,expectedRoot,expectedDrawable,expectedMesh,cancel,options,report,beforeChange](draw::Renderer& renderer)
        {
            if (cancel.expired())
            {
                return;
            }
            auto delegate = weakDelegate.lock();
            auto target = weakNode.lock();
            auto root = expectedRoot.lock();
            auto original = expectedDrawable.lock();
            auto mesh = expectedMesh.lock();
            auto* component = target ? target->drawable() : nullptr;
            if (!delegate || !target || !root || delegate->scene() != root || target->sceneRoot() != root.get() ||
                !component || !original || !mesh || component->drawDrawable() != original || original->drawMesh() != mesh)
            {
                report({false,"UV2 draw target/scene/geometry changed before the queued edit"});
                return;
            }
            Uv2RegenerationResult result;
            try
            {
                renderer.invalidateHistories();
                if (beforeChange)
                {
                    beforeChange();
                }
                if (cancel.expired())
                {
                    return;
                }
                component = target->drawable();
                if (!component || component->drawDrawable() != original ||
                    target->sceneRoot() != root.get() || delegate->scene() != root || original->drawMesh() != mesh)
                {
                    throw std::runtime_error("UV2 invalidation callback changed the target; request again");
                }
                auto generated = bake::generateUv2AndReload(original,options);
                if (cancel.expired())
                {
                    return;
                }
                component->setDrawable(std::move(generated.drawable));
                result = {true,"",generated.atlas.width,generated.atlas.height,
                    generated.atlas.chartCount,generated.atlas.utilization};
            }
            catch (const std::exception& error)
            {
                result.message = error.what();
            }
            // Report outside the operation catch: a throwing callback is not invoked twice.
            if (!cancel.expired())
            {
                report(result);
            }
        }, [cancel,report]
        {
            if (!cancel.expired())
            {
                report({false,"Renderer changed before the queued UV2 draw reload"});
            }
        });
    return token;
}
}
