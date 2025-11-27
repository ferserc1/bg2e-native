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
