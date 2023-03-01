
#ifndef bg2e_tools_jsonparser_hpp
#define bg2e_tools_jsonparser_hpp

#include <iostream>
#include <memory>

#include <bg2e/export.hpp>
#include <bg2e/tools/Json.hpp>
#include <bg2e/tools/JsonToken.hpp>

namespace bg2e {
namespace tools {

class BG2E_EXPORT JsonParser {
    std::istream * stream;
    std::shared_ptr<JsonNode> root;
    std::unique_ptr<JsonNode> current;
    JsonTokenizer tokenizer;

public:
    JsonParser(std::istream * stream) :tokenizer(stream) {}

    std::shared_ptr<JsonNode> & parse();

    std::shared_ptr<JsonNode> parseObject();
    std::shared_ptr<JsonNode> parseString();
    std::shared_ptr<JsonNode> parseNumber();
    std::shared_ptr<JsonNode> parseList();
    std::shared_ptr<JsonNode> parseBoolean();
    std::shared_ptr<JsonNode> parseNull();
};

}
}

#endif
