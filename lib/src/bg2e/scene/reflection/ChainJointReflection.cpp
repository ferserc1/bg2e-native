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
#include <bg2e/scene/ChainJoint.hpp>

using namespace bg2e;

namespace {

template<typename ComponentT>
void registerJointProperty(reflection::TypeInfoBuilder<ComponentT>& t)
{
    t.object(
        "joint",
        "bg2e::base::LinkJoint",
        static_cast<const base::LinkJoint&(ComponentT::*)() const>(&ComponentT::joint),
        static_cast<base::LinkJoint&(ComponentT::*)()>(&ComponentT::joint)
    )
        .displayName("Joint")
        .category("Chain")
        .tooltip("Transform applied at this chain connection");
}

reflection::TypeRegistration<scene::InputChainJointComponent> _inputJointReflection(
    [](reflection::TypeInfoBuilder<scene::InputChainJointComponent>& t) {
        t.displayName("Input Chain Joint");
        registerJointProperty(t);
    });

reflection::TypeRegistration<scene::OutputChainJointComponent> _outputJointReflection(
    [](reflection::TypeInfoBuilder<scene::OutputChainJointComponent>& t) {
        t.displayName("Output Chain Joint");
        registerJointProperty(t);
    });

}
