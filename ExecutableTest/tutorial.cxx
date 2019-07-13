#include <iostream>
#include <bg2-file/bg2-file.hpp>
#include <bg2-base/path.hpp>

int main(int argc, char ** argv) {

	bg2base::path path = "data";

	std::cout << "bg2-file library test version " << bg2file::versionString() << std::endl;

	bg2file::Bg2Reader reader;

	reader.version([](uint8_t major, uint8_t minor, uint8_t rev) {
		std::cout << "bg2 file version "
			<< static_cast<int>(major) << "."
			<< static_cast<int>(minor) << "."
			<< static_cast<int>(rev) << std::endl;
	});

	reader.materials([](const std::string& e) {
		std::cout << "Material data loaded" << std::endl;
	});

	reader.error([](const std::exception& e) {
		std::cerr << e.what() << std::endl;
	});

	reader.metadata([](const bg2file::Bg2Reader::FileMetadata& md) {
		std::cout << "Reading " << md.numberOfPolyList << " polyList" << std::endl;
	});

	reader.inJoint([](float offset[3], float pitch, float roll, float yaw) {
		std::cout << "Input joint: offset=[" << offset[0] << "," << offset[1] << "," << offset[2] << "], "
			<< "pitch=" << pitch << ", roll=" << roll << ", yaw=" << yaw << std::endl;
	});

	reader.outJoint([](float offset[3], float pitch, float roll, float yaw) {
		std::cout << "Output joint: offset=[" << offset[0] << "," << offset[1] << "," << offset[2] << "], "
			<< "pitch=" << pitch << ", roll=" << roll << ", yaw=" << yaw << std::endl;
	});

	reader.plistName([](const std::string& name) {
		std::cout << "Poly list=" << name << std::endl;
	});

	reader.materialName([](const std::string& name) {
		std::cout << "Material name=" << name << std::endl;
	});

	reader.vertex([](const std::vector<float>& v) {
		std::cout << "Vertex data: " << v.size() / 3 << " vertices" << std::endl;
	});

	reader.normal([](const std::vector<float>& n) {
		std::cout << "Normal data: " << n.size() / 3 << " normals" << std::endl;
	});

	reader.uv0([](const std::vector<float>& uv) {
		std::cout << "UV0 data: " << uv.size() / 2 << " elements" << std::endl;
	});

	reader.uv1([](const std::vector<float>& uv) {
		std::cout << "UV1 data: " << uv.size() / 2 << " elements" << std::endl;
	});

	reader.uv2([](const std::vector<float>& uv) {
		std::cout << "UV2 data: " << uv.size() / 2 << " elements" << std::endl;
	});

	reader.index([](const std::vector<unsigned int>& index) {
		std::cout << "Index data: " << index.size() << " elements, "
			<< "Number of triangles: " << index.size() / 3 << std::endl;
	});

	reader.materialOverrides([](const std::string& matOverrides) {
		std::cout << "Material overrides found" << std::endl;
	});


	if (reader.load(path.pathAddingComponent("test.bg2"))) {
		std::cout << "Read ok" << std::endl;
	}

    return 0;
}