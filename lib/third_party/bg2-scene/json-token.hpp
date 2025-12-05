#ifndef _BG2_SCENE_JSON_TOKEN_HPP_
#define _BG2_SCENE_JSON_TOKEN_HPP_

#include <string>
#include <iostream>

namespace bg2scene {
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


        class JsonTokenizer {
            std::istream * stream;
            std::fpos<std::mbstate_t> prevPos;

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
