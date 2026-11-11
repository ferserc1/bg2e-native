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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/app/Command.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace bg2e::scene {

class Node;
class Scene;
class Component;

enum class ComponentIssueSeverity
{
    Warning,
    Error
};

struct ComponentIssueFix {
    std::string label;
    std::string description;
    // Construction is read-only. Capture targets weakly; execution validates them.
    // Applications own safe scheduling, resource refresh and optional history.
    std::function<std::unique_ptr<app::Command>()> createCommand;
};

struct ComponentIssue {
    std::string code;
    ComponentIssueSeverity severity = ComponentIssueSeverity::Warning;
    std::string message;
    std::string help;
    // Alternatives, not steps: each command is one complete reversible solution.
    std::vector<ComponentIssueFix> fixes;
};

struct ComponentValidationContext {
    const Node& node;
    const Scene* scene = nullptr;
    // Optional document subtree; external dependencies and root fixes stay here.
    const Node* root = nullptr;
};

struct ComponentIssueEntry {
    std::weak_ptr<Component> component;
    std::weak_ptr<Component> relatedComponent;
    ComponentIssue issue;
};

// Read-only diagnostics, attributed to the component declaring each rule.
// The caller can traverse a scene and concatenate these node-local results.
BG2E_API std::vector<ComponentIssueEntry> validateNodeComponents(const Node& node, const Node* root = nullptr);

}
