
#pragma once

#include <bg2e/common.hpp>

#include <bg2e/geo/Mesh.hpp>

#include <filesystem>
#include <iostream>
#include <fstream>

namespace bg2e {
namespace db {

template <class MeshT>
extern BG2E_API MeshT* loadMeshObj(std::istream& inputStream);

template <>
BG2E_API bg2e::geo::MeshP* loadMeshObj<bg2e::geo::MeshP>(std::istream& inputStream);

template <>
BG2E_API bg2e::geo::MeshPN* loadMeshObj<bg2e::geo::MeshPN>(std::istream& inputStream);

template <>
BG2E_API bg2e::geo::MeshPC* loadMeshObj<bg2e::geo::MeshPC>(std::istream& inputStream);

template <>
BG2E_API bg2e::geo::MeshPU* loadMeshObj<bg2e::geo::MeshPU>(std::istream& inputStream);

template <>
BG2E_API bg2e::geo::MeshPNU* loadMeshObj<bg2e::geo::MeshPNU>(std::istream& inputStream);

template <>
BG2E_API bg2e::geo::MeshPNC* loadMeshObj<bg2e::geo::MeshPNC>(std::istream& inputStream);

template <>
BG2E_API bg2e::geo::MeshPNUC* loadMeshObj<bg2e::geo::MeshPNUC>(std::istream& inputStream);

template <>
BG2E_API bg2e::geo::MeshPNUT* loadMeshObj<bg2e::geo::MeshPNUT>(std::istream& inputStream);

template <>
BG2E_API bg2e::geo::MeshPNUUT* loadMeshObj<bg2e::geo::MeshPNUUT>(std::istream& inputStream);

template <class MeshT>
MeshT* loadMeshObj(const std::filesystem::path& filePath)
{
	std::ifstream stream(filePath);
	if (!stream.is_open()) {
		throw std::runtime_error("Error loading OBJ mesh file: " + filePath.string());
	}

	return loadMeshObj<MeshT>(stream);
}

}
}
