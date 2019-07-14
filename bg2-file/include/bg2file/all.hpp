
#ifndef _bg2_file_all_hpp_
#define _bg2_file_all_hpp_

#include <bg2file/bg2_reader.hpp>
#include <bg2file/bg2_writter.hpp>
#include <bg2file/binary_parser.hpp>
#include <bg2file/convert.hpp>
#include <bg2file/escaper.hpp>
#include <bg2file/grammar.hpp>
#include <bg2file/parser.hpp>
#include <bg2file/types.hpp>
#include <bg2file/value.hpp>

namespace bg2file {

	extern uint32_t versionMajor();
	extern uint32_t versionMinor();
	extern uint32_t versionRev();
	extern std::string versionString();

}

#endif
