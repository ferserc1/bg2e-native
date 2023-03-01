#ifndef bg2e_tools_jsontoken_hpp
#define bg2e_tools_jsontoken_hpp

#include <bg2e/export.hpp>

#include <string>
#include <iostream>

namespace bg2e {
namespace tools {

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


class BG2E_EXPORT JsonTokenizer {
    std::istream * stream;
    size_t prevPos;

public:
    JsonTokenizer(std::istream * s);

    char getWithoutWhiteSpace();
    JsonToken getToken();
    bool hasMoreTokens();
    void rollBackToken();
};

}
}


#endif
