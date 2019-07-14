
#include <bg2file/all.hpp>

#include "bg2e-config.hpp"

#include <iostream>
#include <string>

namespace bg2file {

	uint32_t versionMajor() {
		return bg2e_MAJOR_VERSION;
	}

	uint32_t versionMinor() {
		return bg2e_MINOR_VERSION;
	}

	uint32_t versionRev() {
		return bg2e_REV_VERSION;
	}

	std::string versionString() {
		return std::to_string(bg2e_MAJOR_VERSION) + "." + std::to_string(bg2e_MINOR_VERSION) + "." + std::to_string(bg2e_REV_VERSION);
	}

}
