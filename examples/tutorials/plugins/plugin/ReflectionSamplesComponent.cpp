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

#include "ReflectionSamplesComponent.hpp"
#include "PluginComponents.hpp"

#include <bg2e/reflection/Builder.hpp>

namespace tutorial {

void registerSamples(bg2e::plugin::ComponentRegistrar& registrar)
{
    using namespace bg2e;
    reflection::TypeInfoBuilder<SampleValues> values("tutorial.SampleValues");
    values.displayName("Sample values");
    values.accessor<bool>("active", [](const SampleValues& object)
    {
        return object.active;
    }, [](SampleValues& object, const bool& value)
    {
        object.active = value;
    }).category("Values").checkbox();
    values.accessor<int32_t>("count", [](const SampleValues& object)
    {
        return object.count;
    }, [](SampleValues& object, const int32_t& value)
    {
        object.count = value;
    }).category("Values").input().range(-10, 10);
    values.accessor<uint32_t>("mask", [](const SampleValues& object)
    {
        return object.mask;
    }, [](SampleValues& object, const uint32_t& value)
    {
        object.mask = value;
    }).category("Values").drag().step(1);
    values.accessor<float>("weight", [](const SampleValues& object)
    {
        return object.weight;
    }, [](SampleValues& object, const float& value)
    {
        object.weight = value;
    }).category("Values").slider().range(0, 1);
    values.accessor<double>("precision", [](const SampleValues& object)
    {
        return object.precision;
    }, [](SampleValues& object, const double& value)
    {
        object.precision = value;
    }).category("Values").input();
    values.accessor<std::string>("title", [](const SampleValues& object)
    {
        return object.title;
    }, [](SampleValues& object, const std::string& value)
    {
        object.title = value;
    }).category("Values").input();
    values.accessor<glm::vec2>("uv", [](const SampleValues& object)
    {
        return object.uv;
    }, [](SampleValues& object, const glm::vec2& value)
    {
        object.uv = value;
    }).category("Values").drag().step(0.01);
    values.accessor<glm::vec3>("offset", [](const SampleValues& object)
    {
        return object.offset;
    }, [](SampleValues& object, const glm::vec3& value)
    {
        object.offset = value;
    }).category("Values").drag();
    values.accessor<glm::vec4>("vector", [](const SampleValues& object)
    {
        return object.vector;
    }, [](SampleValues& object, const glm::vec4& value)
    {
        object.vector = value;
    }).category("Values").input();
    values.accessor<glm::mat4>("matrix", [](const SampleValues& object)
    {
        return object.matrix;
    }, [](SampleValues& object, const glm::mat4& value)
    {
        object.matrix = value;
    }).category("Values");
    values.accessor<base::Color>("tint", [](const SampleValues& object)
    {
        return object.tint;
    }, [](SampleValues& object, const base::Color& value)
    {
        object.tint = value;
    }).category("Values").colorEditor();
    values.accessor<std::filesystem::path>("path", [](const SampleValues& object)
    {
        return object.path;
    }, [](SampleValues& object, const std::filesystem::path& value)
    {
        object.path = value;
    }).category("Values");
    values.accessor<std::filesystem::path>("image", [](const SampleValues& object)
    {
        return object.image;
    }, [](SampleValues& object, const std::filesystem::path& value)
    {
        object.image = value;
    }).category("Values").resource("Images", {"png", "jpg"}).clearableResource().resourceEmptySelection();
    values.accessor<std::string>("imageName", [](const SampleValues& object)
    {
        return object.imageName;
    }, [](SampleValues& object, const std::string& value)
    {
        object.imageName = value;
    }).category("Values").resource("Images", {"png", "jpg"}, true).clearableResource();
    registrar.registerReflection(values.build());

    reflection::TypeInfoBuilder<SampleMode> base("tutorial.SampleMode");
    base.accessor<bool>("active", [](const SampleMode& mode)
    {
        return mode.active;
    }, [](SampleMode& mode, bool value)
    {
        mode.active = value;
    }).checkbox();
    registrar.registerReflection(base.build());
    reflection::TypeInfoBuilder<SmoothMode> smooth("tutorial.SmoothMode");
    smooth.accessor<float>("smoothing", [](const SmoothMode& mode)
    {
        return mode.smoothing;
    }, [](SmoothMode& mode, float value)
    {
        mode.smoothing = value;
    }).slider().range(0, 1);
    registrar.registerReflection(smooth.build());
    reflection::TypeInfoBuilder<StepMode> step("tutorial.StepMode");
    step.accessor<uint32_t>("steps", [](const StepMode& mode)
    {
        return mode.steps;
    }, [](StepMode& mode, uint32_t value)
    {
        mode.steps = value;
    }).drag().step(1);
    registrar.registerReflection(step.build());
    registrar.registerSubtype<SampleMode, SmoothMode>(
        "tutorial.SampleMode", "smooth", "Smooth", "tutorial.SmoothMode");
    registrar.registerSubtype<SampleMode, StepMode>(
        "tutorial.SampleMode", "step", "Step", "tutorial.StepMode");

    using Samples = ReflectionSamplesComponent;
    reflection::TypeInfoBuilder<Samples> component(Samples::staticTypeName());
    component.displayName("Reflection samples (tutorial)");
    component.property("description", &Samples::description).category("About");
    component.object("values", "tutorial.SampleValues", &Samples::values, &Samples::editableValues)
        .displayName("Editable values").category("Objects");
    component.object("preview", "tutorial.SampleValues", &Samples::values)
        .displayName("Read-only view of the same values").category("Objects");
    component.polymorphicObject<SampleMode>("mode", "tutorial.SampleMode",
        &Samples::mode, &Samples::editableMode, &Samples::setMode)
        .displayName("Mode").category("Objects").subtype("smooth").subtype("step");
    component.polymorphicObject<SampleMode>("modePreview", "tutorial.SampleMode", &Samples::mode)
        .displayName("Read-only mode").category("Objects");
    component.action("reset", [](Samples* target)
    {
        target->reset();
    }).displayName("Reset sample values").category("Samples");
    registrar.registerComponent<Samples>(component.build());
    if (!reflection::TypeRegistry::get().validateObjectDepth())
    {
        throw std::logic_error("Reflection object nesting exceeds the supported depth");
    }
}

}
