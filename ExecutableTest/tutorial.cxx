#include <iostream>
#include <bg2-file/bg2-file.hpp>

int main(int argc, char ** argv) {

	std::cout << "bg2-file library test version " << bg2file::versionString() << std::endl;

	bg2file::Bg2Reader reader;

	if (reader.load("data/test.bg2")) {
		std::cout << "Read ok" << std::endl;
	}

    return 0;
}