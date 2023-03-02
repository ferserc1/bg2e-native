
#include <bg2e/tools/JsonToken.hpp>

namespace bg2e {
namespace tools {

std::string JsonToken::toString() {
    switch (type) {
    case JsonTokenType::CurlyOpen:
        return "Curly open";
    case JsonTokenType::CurlyClose:
        return "Curly close";
    case JsonTokenType::Colon:
        return "COLON";
    case JsonTokenType::Number:
        return "Number: " + value;
    case JsonTokenType::String:
        return "String: " + value;
    case JsonTokenType::ListOpen:
        return "Array open";
    case JsonTokenType::ListClose:
        return "Array close";
    case JsonTokenType::Comma:
        return "Comma";
    case JsonTokenType::Boolean:
        return "Boolean: " + value;;
    case JsonTokenType::NullType:
        return "Null";
    default:
        return "";
    }
}

JsonTokenizer::JsonTokenizer(std::istream * s)
    :stream(s)
{

}

char JsonTokenizer::getWithoutWhiteSpace() {
    char c = ' ';
    while (c == ' ' || c == '\t' || c == '\n') {
        stream->get(c);

        if ((c == ' ' || c == '\n' || c == '\t') && !stream->good()) {
            throw std::logic_error("Ran out of tokens");
        }
        else if (!stream->good()) {
            return c;
        }
    }
    return c;
}

JsonToken JsonTokenizer::getToken() {
    char c;
    if (stream->eof()) {
        throw std::logic_error("Exhaused tokens");
    }
    prevPos = stream->tellg();
    c = getWithoutWhiteSpace();

    struct JsonToken token;
    if (c == '"') {
        token.type = JsonTokenType::String;
        token.value = "";
        stream->get(c);
        while (c != '"') {
            token.value += c;
            stream->get(c);
        }
    }
    else if (c == '{') {
        token.type = JsonTokenType::CurlyOpen;
    }
    else if (c == '}') {
        token.type = JsonTokenType::CurlyClose;
    }
    else if (c == '-' || (c>='0' && c <='9')) {
        token.type = JsonTokenType::Number;
        token.value = "";
        token.value += c;
        std::streampos prevCharPos = stream->tellg();
        while ((c=='-') || (c>='0' && c<='9') || c == '.') {
            prevCharPos = stream->tellg();
            stream->get(c);

            if (stream->eof()) {
                break;
            }
            else {
                if (c=='-' || (c>='0' && c<='9') || c=='.') {
                    token.value += c;
                }
                else {
                    stream->seekg(prevCharPos);
                }
            }
        }
    }
    else if (c == 'f') {
        token.type = JsonTokenType::Boolean;
        token.value = "false";
        stream->seekg(4, std::ios_base::cur);
    }
    else if (c == 't') {
        token.type = JsonTokenType::Boolean;
        token.value = "true";
        stream->seekg(3, std::ios_base::cur);
    }
    else if (c == 'n') {
        token.type = JsonTokenType::NullType;
        stream->seekg(3, std::ios_base::cur);
    }
    else if (c == '[') {
        token.type = JsonTokenType::ListOpen;
    }
    else if (c == ']') {
        token.type = JsonTokenType::ListClose;
    }
    else if (c == ':') {
        token.type = JsonTokenType::Colon;
    }
    else if (c == ',') {
        token.type = JsonTokenType::Comma;
    }
    return token;
}

bool JsonTokenizer::hasMoreTokens() {
    size_t prevPos = stream->tellg();
    bool result = true;
    try {
        getToken();
    }
    catch(std::exception &) {
        result = false;
    }
    stream->seekg(prevPos);
    return result;
}

void JsonTokenizer::rollBackToken() {
    if (stream->eof()) {
        stream->clear();
    }
    stream->seekg(prevPos);
}

}
}
