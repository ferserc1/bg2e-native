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

#include <iostream>
#include <sstream>
#include <memory>
#include <cstdint>

#include <bg2e/json/JsonNode.hpp>
#include <bg2e/json/JsonToken.hpp>

namespace bg2e {
namespace json {

class BG2E_API JsonParser {
    std::istream * stream;
    std::shared_ptr<JsonNode> root;
    std::unique_ptr<JsonNode> current;
    JsonTokenizer tokenizer;

public:
    JsonParser(std::istream * stream) :tokenizer(stream) {}
    JsonParser(const std::string& buffer) :tokenizer(&_sstream), _sstream(buffer) {}
    JsonParser(const char* buffer) :tokenizer(&_sstream), _sstream(buffer) {}

    std::shared_ptr<JsonNode> & parse();

    std::shared_ptr<JsonNode> parseObject();
    std::shared_ptr<JsonNode> parseString();
    std::shared_ptr<JsonNode> parseNumber();
    std::shared_ptr<JsonNode> parseList();
    std::shared_ptr<JsonNode> parseBoolean();
    std::shared_ptr<JsonNode> parseNull();
    
    std::stringstream _sstream;
};

}
}
