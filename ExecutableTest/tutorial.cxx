#include <iostream>
#include <memory>

#include <bg2file/all.hpp>
#include <bg2base/all.hpp>
#include <bg2base/path.hpp>
#include <bg2math/utils.hpp>
#include <bg2math/vector.hpp>
#include <bg2base/image.hpp>
#include <bg2db/image_load.hpp>
#include <bg2db/image_write.hpp>
#include <bg2math/matrix.hpp>

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

    std::cout << "PI is " << bg2math::kd::pi << std::endl;
    
    bg2math::float2 v;
    float * fp = &v;
    fp[0] = 0.2f;
    v[1] = 4.1f;
    
    std::cout << "V=" << v[0] << "," << v[1] << std::endl;
    
    bg2math::float2 a(1.0f,1.0f);
    bg2math::float2 b(2.0f,2.0f);
    std::cout << "Distance between A and B = " << bg2math::distance(a,b) << std::endl;
    

    std::cout << bg2math::clamp(1.3f, 0.2f, 2.0f) << std::endl;
    std::cout << bg2math::clamp(4.3f, 0.2f, 2.0f) << std::endl;
    std::cout << bg2math::clamp(-1.3f, 0.2f, 2.0f) << std::endl;
    
    bg2math::float2 cv = { 0.4f, 9.2f };
    std::cout << "cv=" << cv.toString() << std::endl;
    std::cout << "clamped cv=" << bg2math::clamp(cv, 1.4f, 3.4f).toString() << std::endl;
    
    std::unique_ptr<bg2base::image> image;
    std::unique_ptr<bg2base::image> copy;
    try {
        image = std::unique_ptr<bg2base::image>(bg2db::loadImage(path.pathAddingComponent("texture.jpg")));
        bg2db::writeImage(path.pathAddingComponent("texture_copy.jpg"), image.get());
        copy = std::unique_ptr<bg2base::image>(bg2db::loadImage(path.pathAddingComponent("texture_copy.jpg")));
    } catch (const std::exception & e) {
        std::cerr << e.what() << std::endl;
    }
    
	bg2math::float3x3 mat(
		bg2math::float3(0.0f, 1.0f, 2.0f),
		bg2math::float3(3.0f, 4.0f, 5.0f),
		bg2math::float3(6.0f, 7.0f, 8.0f)
	);

	std::cout << mat.toString() << std::endl;

	mat.transpose();

	std::cout << mat.toString() << std::endl;

	bg2math::float3x3 idMat = bg2math::float3x3::Identity();
	if (idMat.isIdentity()) {
		std::cout << "isIdentity ok" << std::endl;
	}
	bg2math::float3x3 zeroMat;
	if (zeroMat.isZero()) {
		std::cout << "isZero ok" << std::endl;
	}
	bg2math::float3x3 nanMat;
	auto other = 0.0f;
	nanMat[2] = 0.0f / other;
	if (nanMat.isNaN()) {
		std::cout << "NaN ok" << std::endl;
	}

	auto m1 = bg2math::float4x4::Identity();
	m1.rotate(bg2math::kf::piOver2, 0.0f, 1.0f, 0.0f)
		.translate(0.0f, 0.0f, 2.0f);

	std::cout << m1.toString() << std::endl;
	m1.invert();
	std::cout << m1.toString() << std::endl;

    
    return 0;
}
