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
#include <bg2e/base/Joint.hpp>

using namespace bg2e;

namespace {

reflection::TypeRegistration<base::LinkJoint> _linkJointReflection(
    "bg2e::base::LinkJoint",
    [](reflection::TypeInfoBuilder<base::LinkJoint>& t) {
        t.displayName("Link Joint");

        t.property("offset", &base::LinkJoint::offset, &base::LinkJoint::setOffset)
            .displayName("Offset")
            .category("Transform");

        t.property("eulerRotation", &base::LinkJoint::eulerRotation,
                                    &base::LinkJoint::setEulerRotation)
            .displayName("Euler Rotation")
            .category("Transform")
            .tooltip("Euler rotation in radians");

        t.property("transformOrder", &base::LinkJoint::transformOrder,
                                    &base::LinkJoint::setTransformOrder)
            .displayName("Transform Order")
            .category("Transform")
            .combo()
            .enumValue("Rotate, then translate", base::LinkTransformOrder::RotateTranslate)
            .enumValue("Translate, then rotate", base::LinkTransformOrder::TranslateRotate);
    });

}
