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

#include <string>
#include <iostream>

namespace bg2e {
namespace json {

enum class JsonTokenType {
    CurlyOpen,
    CurlyClose,
    Colon,
    String,
    Number,
    ListOpen,
    ListClose,
    Comma,
    Boolean,
    NullType
};

struct JsonToken {
    std::string value;
    JsonTokenType type;
    std::string toString();
};


class BG2E_API JsonTokenizer {
public:
    JsonTokenizer(std::istream * s);

    char getWithoutWhiteSpace();
    JsonToken getToken();
    bool hasMoreTokens();
    void rollBackToken();

protected:
    std::istream * stream;
    std::fpos<std::mbstate_t> prevPos;
};

}
}
