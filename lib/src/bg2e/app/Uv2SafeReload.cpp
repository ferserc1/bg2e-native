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
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/DrawableComponent.hpp>

#include <stdexcept>

namespace bg2e {
namespace app {

std::shared_ptr<SafeUpdateToken> Uv2SafeReload::regenerate(
    std::weak_ptr<scene::Node> targetNode,
    const geo::Uv2AtlasOptions & options,
    CompletionCallback onComplete)
{
    auto token = std::make_shared<SafeUpdateToken>();

    auto node = targetNode.lock();
    if (!node)
    {
        if (onComplete)
        {
            onComplete({ false, "Uv2SafeReload: the target node no longer exists" });
        }
        return token;
    }
    scene::Node * expectedRoot = node->sceneRoot();

    MainLoop::current()->safeUpdateScene(
        [weakNode = targetNode, expectedRoot, options, onComplete]()
        {
            auto report = [onComplete](const Uv2RegenerationResult & r)
            {
                if (onComplete)
                {
                    onComplete(r);
                }
            };

            auto target = weakNode.lock();
            if (!target)
            {
                report({ false, "Uv2SafeReload: the target node no longer exists" });
                return;
            }

            if (target->sceneRoot() != expectedRoot)
            {
                report({ false, "Uv2SafeReload: the target node belongs to a different scene root" });
                return;
            }

            auto * component = target->drawable();
            auto drawable = component != nullptr ? component->drawable() : nullptr;
            if (!drawable)
            {
                report({ false, "Uv2SafeReload: the target node has no standard Drawable component" });
                return;
            }

            auto mesh = drawable->mesh();
            if (!mesh || !drawable->isLoaded())
            {
                report({ false, "Uv2SafeReload: the target Drawable has no loaded mesh" });
                return;
            }

            const auto submeshCount = drawable->submeshesCount();

            try
            {
                geo::GenerateUv2AtlasModifier modifier(mesh.get(), options);
                modifier.apply();

                drawable->reload();

                if (drawable->submeshesCount() != submeshCount)
                {
                    report({ false, "Uv2SafeReload: submesh count changed during regeneration" });
                    return;
                }

                const auto & atlas = modifier.result();
                report({
                    true,
                    "",
                    atlas.width,
                    atlas.height,
                    atlas.chartCount,
                    atlas.utilization
                });
            }
            catch (const std::exception & e)
            {
                report({ false, std::string("Uv2SafeReload: ") + e.what() });
            }
        },
        token);

    return token;
}

}
}
