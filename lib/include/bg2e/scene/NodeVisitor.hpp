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

namespace bg2e {
namespace scene {

class Node;

class NodeVisitor {
public:
    virtual ~NodeVisitor() = default;
    
    virtual void visit(Node *) {}
    virtual void didVisit(Node *) {}
    
    inline void setIgnoreDisabled(bool i) { _ignoreDisabled = i; }
    inline bool ignoreDisabled() const { return _ignoreDisabled; }
    
    virtual bool cancel() { return false; }
    
protected:
    bool _ignoreDisabled = false;
};

}
}
