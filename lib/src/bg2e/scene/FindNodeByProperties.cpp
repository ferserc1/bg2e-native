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

#include <bg2e/scene/FindNodeByProperties.hpp>

namespace bg2e::scene {

FindNodeByProperties& FindNodeByProperties::byName(const std::string& name)
{
    _filterName = name;
    _filterMask |= ByName;
    return *this;
}

FindNodeByProperties& FindNodeByProperties::byIdentifier(const std::string& identifier)
{
    _filterIdentifier = identifier;
    _filterMask |= ByIdentifier;
    return *this;
}

FindNodeByProperties& FindNodeByProperties::byEnabled(bool enabled)
{
    _filterEnabled = enabled;
    _filterMask |= ByEnabled;
    return *this;
}

FindNodeByProperties& FindNodeByProperties::bySteady(bool steady)
{
    _filterSteady = steady;
    _filterMask |= BySteady;
    return *this;
}

const std::vector<std::shared_ptr<Node>>& FindNodeByProperties::find(Node* node)
{
    node->accept(this);
    return _results;
}

void FindNodeByProperties::visit(Node * node)
{
    if (_filterMask == None)
    {
        return;
    }

    if ((_filterMask & ByName) && node->name() != _filterName)
    {
        return;
    }

    if ((_filterMask & ByIdentifier) && node->identifier() != _filterIdentifier)
    {
        return;
    }

    if ((_filterMask & ByEnabled) && node->enabled() != _filterEnabled)
    {
        return;
    }

    if ((_filterMask & BySteady) && node->steady() != _filterSteady)
    {
        return;
    }

    _results.push_back(node->shared_from_this());
}

void FindNodeByProperties::clearResults()
{
    _results.clear();
}

void FindNodeByProperties::clearFilters()
{
    _filterMask = None;
    _filterName.clear();
    _filterIdentifier.clear();
    _filterEnabled = true;
    _filterSteady = false;
}

void FindNodeByProperties::clear()
{
    clearResults();
    clearFilters();
}

}
