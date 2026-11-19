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

#include <bg2e/reflection/Registration.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/app/FileDialog.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/app/MessageBox.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <stdexcept>
#include <utility>

using namespace bg2e;

namespace {

reflection::TypeRegistration<scene::DrawableComponent> drawableReflection(
    [](reflection::TypeInfoBuilder<scene::DrawableComponent>& type)
    {
        type.displayName("Drawable");
        type.action("Swap Drawable", [](scene::DrawableComponent* owner)
        {
            try
            {
                if (!owner || !owner->isSceneGeometry() || !owner->requiresBg2Export() || !owner->drawable() || !owner->drawable()->engine() || !app::MainLoop::current())
                {
                    throw std::invalid_argument("Drawable replacement requires a realized owner and an active scene update loop");
                }
                const auto path = app::FileDialog::getOpenFilePath({{"BG2 model", "bg2,vwglb"}});
                if (path.empty())
                {
                    return;
                }
                const std::weak_ptr<scene::DrawableComponent> weak = std::static_pointer_cast<scene::DrawableComponent>(owner->shared_from_this());
                app::MainLoop::current()->safeUpdateScene([weak, path]
                {
                    try
                    {
                        const auto target = weak.lock();
                        if (!target || !target->drawable() || !target->drawable()->engine())
                        {
                            throw std::invalid_argument("The drawable owner is no longer available");
                        }
                        auto resource = db::loadDrawableBg2(path, *target->drawable()->engine());
                        const auto assigned = target->setDrawable(std::move(resource));
                        if (!assigned.ok())
                        {
                            app::MessageBox::showError("Drawable replacement rejected", assigned.message);
                        }
                    }
                    catch (const std::exception& error)
                    {
                        app::MessageBox::showError("Drawable replacement failed", error.what());
                    }
                    catch (...)
                    {
                        app::MessageBox::showError("Drawable replacement failed", "Previous owner state retained.");
                    }
                });
            }
            catch (const std::exception& error)
            {
                app::MessageBox::showError("Drawable replacement failed", error.what());
            }
            catch (...)
            {
                app::MessageBox::showError("Drawable replacement failed", "Previous owner state retained.");
            }
        });
    });

}
