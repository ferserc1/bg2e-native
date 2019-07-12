
#ifndef _bg2_file_hpp_
#define _bg2_file_hpp_

#include <bg2-file/bg2_reader.hpp>
#include <bg2-file/bg2_writter.hpp>
#include <bg2-file/binary_parser.hpp>
#include <bg2-file/convert.hpp>
#include <bg2-file/escaper.hpp>
#include <bg2-file/grammar.hpp>
#include <bg2-file/parser.hpp>
#include <bg2-file/types.hpp>
#include <bg2-file/value.hpp>

namespace bg2file {

	extern uint32_t versionMajor();
	extern uint32_t versionMinor();
	extern uint32_t versionRev();
	extern std::string versionString();

}

#endif
