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

#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/scene/Node.hpp>

#include <vector>
#include <memory>
#include <string>

namespace bg2e {
namespace scene {

class BG2E_API FindNodeByProperties : public NodeVisitor {
public:
    enum FilterMask : uint8_t {
        None         = 0,
        ByName       = 1 << 0,
        ByIdentifier = 1 << 1,
        ByEnabled    = 1 << 2,
        BySteady     = 1 << 3
    };

    FindNodeByProperties() = default;
    virtual ~FindNodeByProperties() = default;

    FindNodeByProperties& byName(const std::string& name);
    FindNodeByProperties& byIdentifier(const std::string& identifier);
    FindNodeByProperties& byEnabled(bool enabled);
    FindNodeByProperties& bySteady(bool steady);

    const std::vector<std::shared_ptr<Node>>& find(Node* node);

    void visit(Node * node) override;

    void clearResults();
    void clearFilters();
    void clear();

    const std::vector<std::shared_ptr<Node>>& results() const { return _results; }

protected:
    uint8_t _filterMask = None;

    std::string _filterName;
    std::string _filterIdentifier;
    bool _filterEnabled = true;
    bool _filterSteady = false;

    std::vector<std::shared_ptr<Node>> _results;
};

}
}
