/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 */

#include <bg2e/reflection/Registration.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/app/MessageBox.hpp>

using namespace bg2e;

namespace {

reflection::TypeRegistration<scene::DrawableComponent> _drawableReflection(
    [](reflection::TypeInfoBuilder<scene::DrawableComponent>& t) {
        t.displayName("Drawable");

        t.action("Swap Drawable", [](scene::DrawableComponent* comp)
        {
            app::MessageBox::showInfo("Swap drawable", "Method not implemented");
        });
    });

}
