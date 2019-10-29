/*
 *	bg2 engine license
 *	Copyright (c) 2016 Fernando Serrano <ferserc1@gmail.com>
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a copy
 *	of this software and associated documentation files (the "Software"), to deal
 *	in the Software without restriction, including without limitation the rights
 *	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *	of the Software, and to permit persons to whom the Software is furnished to do
 *	so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be included in all
 *	copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 *	INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 *	PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#include <bg2e/db/bg2_reader.hpp>
#include <bg2e/db/json/parser.hpp>

namespace bg2e {
namespace db {

	void readVector2(json::Value * val, float * result) {
		if (val && (*val)[0] && (*val)[1]) {
			result[0] = (*val)[0]->floatValue();
			result[1] = (*val)[1]->floatValue();
		}
	}

	void readVector3(json::Value * val, float * result) {
		if (val && (*val)[0] && (*val)[1] && (*val)[2]) {
			result[0] = (*val)[0]->floatValue();
			result[1] = (*val)[1]->floatValue();
			result[2] = (*val)[2]->floatValue();
		}
	}

	void readVector4(json::Value * val, float * result) {
		if (val && (*val)[0] && (*val)[1] && (*val)[2] && (*val)[3]) {
			result[0] = (*val)[0]->floatValue();
			result[1] = (*val)[1]->floatValue();
			result[2] = (*val)[2]->floatValue();
			result[3] = (*val)[3]->floatValue();
		}
	}

	bool Bg2Reader::load(const std::string &path) {
		
		if (_binaryParser.open(path, BinaryParser::kModeRead)) {
			try {
				std::string materialsString;
				unsigned int numberOfPlist = 0;
				readHeader(materialsString,numberOfPlist);

				readPolyList(numberOfPlist);
				
				_binaryParser.close();
				
				parseMaterialOverride(path);

				return true;
			}
			catch (std::exception & e) {
				if (_exceptionClosure) _exceptionClosure(e);
			}
		}
		return false;
	}

	bool Bg2Reader::loadHeader(const std::string &path) {
		return true;
	}
		
	void Bg2Reader::readHeader(std::string & materialsString, unsigned int & numberOfPlist) {
		//// File header
		// File endian 0=big endian, 1=little endian
		unsigned char endian;
		_binaryParser.readByte(endian);
		if (endian == 0) _binaryParser.setBigEndian();
		else _binaryParser.setLittleEndian();

		// Version (major, minor, rev)
		uint8_t versionMajor;
		uint8_t versionMinor;
		uint8_t versionRevision;
		_binaryParser.readByte(versionMajor);
		_binaryParser.readByte(versionMinor);
		_binaryParser.readByte(versionRevision);

		if (_vertexClosure) _versionClosure(versionMajor, versionMinor, versionRevision);

		// Header type
		BinaryParser::BlockType btype;
		_binaryParser.readBlock(btype);
		if (btype != BinaryParser::kHeader) {
			throw std::runtime_error("Bg2 file format exception. Expecting begin of file header.");
		}

		// Number of poly list
		FileMetadata metadata;
		_binaryParser.readInteger(metadata.numberOfPolyList);
		numberOfPlist = metadata.numberOfPolyList;
		if (_metadataClosure) _metadataClosure(metadata);

		// Materials
		BinaryParser::BlockType block;
		_binaryParser.readBlock(block);
		if (block != BinaryParser::kMaterials) {
			throw std::runtime_error("Bg2 file format exception. Expecting material list.");
		}

		_binaryParser.readString(materialsString);
		if (_materialClosure) _materialClosure(materialsString);

		// Shadow projectors are deprecated, this block only skips the projector section
		_binaryParser.readBlock(block);
		if (block == BinaryParser::kShadowProjector) {
			std::string fileName;
			float proj[16];
			float trans[16];
			float attenuation;
			_binaryParser.readString(fileName);
			_binaryParser.readFloat(attenuation);
			for (int i = 0; i<16; ++i) {
				_binaryParser.readFloat(proj[i]);
			}
			for (int i = 0; i<16; ++i) {
				_binaryParser.readFloat(trans[i]);
			}
		}
		else {
			_binaryParser.seekForward(-4);
		}

		// Joint list
		_binaryParser.readBlock(block);
		if (block == BinaryParser::kJoint) {
			using namespace json;
			std::string jointString;
			float offset[3];
			float pitch, roll, yaw;
			_binaryParser.readString(jointString);

			Value * jointData = Parser::ParseString(jointString);
			Value * input = (*jointData)["input"];
			if (input) {
				readVector3((*input)["offset"], offset);
				pitch = (*input)["pitch"] ? (*input)["pitch"]->floatValue():0.0f;
				yaw = (*input)["yaw"] ? (*input)["yaw"]->floatValue():0.0f;
				roll = (*input)["roll"] ? (*input)["roll"]->floatValue():0.0f;
				
				if (_inJointClosure) _inJointClosure(offset, pitch, roll, yaw);
			}


			Value * output = (*jointData)["output"];
			if (output && (*output)[0]) {
				readVector3((*(*output)[0])["offset"], offset);
				pitch = (*(*output)[0])["pitch"] ? (*(*output)[0])["pitch"]->floatValue():0.0f;
				yaw = (*(*output)[0])["yaw"] ? (*(*output)[0])["yaw"]->floatValue():0.0f;
				roll = (*(*output)[0])["roll"] ? (*(*output)[0])["roll"]->floatValue():0.0f;
				
				if (_outJointClosure) _outJointClosure(offset, pitch, roll, yaw);
			}
		}
		else {
			_binaryParser.seekForward(-4);
		}
	}

	void Bg2Reader::readPolyList(int numberOfPlist) {
		BinaryParser::BlockType block;

		_binaryParser.readBlock(block);
		if (block != BinaryParser::kPolyList) throw std::runtime_error("Bg2: File format exception. Expecting poly list");
		for (int i = 0; i < numberOfPlist; ++i) {
			readSinglePolyList();
		}
	}

	void Bg2Reader::readSinglePolyList() {
		BinaryParser::BlockType block;
		bool done = false;
		bool vertexFound = false;
		bool normalFound = false;
		bool tex0Found = false;
		bool tex1Found = false;
		bool tex2Found = false;
		bool indexFound = false;
		std::vector<float> vector;
		std::vector<unsigned int> ivector;
		std::string name;

		while (!done) {
			vector.clear();
			_binaryParser.readBlock(block);
			switch (block) {
			case BinaryParser::kPlistName:
				_binaryParser.readString(name);
				if (_plistNameClosure) _plistNameClosure(name);
				break;
			case BinaryParser::kMatName:
				_binaryParser.readString(name);
				if (_materialNameClosure) _materialNameClosure(name);
				break;
			case BinaryParser::kVertexArray:
				if (vertexFound) throw std::runtime_error("Bg2: File format exception. duplicate vertex array");
				vertexFound = true;
				_binaryParser.readArray(vector);
				if (_vertexClosure) _vertexClosure(vector);
				break;
			case BinaryParser::kNormalArray:
				if (normalFound) throw std::runtime_error("Bg2: File format exception. duplicate normal array");
				normalFound = true;
				_binaryParser.readArray(vector);
				if (_normalClosure) _normalClosure(vector);
				break;
			case BinaryParser::kTexCoord0Array:
				if (tex0Found) throw std::runtime_error("Bg2: File format exception. duplicate texcoord0 array");
				tex0Found = true;
				_binaryParser.readArray(vector);
				if (_uv0Closure) _uv0Closure(vector);
				break;
			case BinaryParser::kTexCoord1Array:
				if (tex1Found) throw std::runtime_error("Bg2: File format exception. duplicate texcoord1 array");
				tex1Found = true;
				_binaryParser.readArray(vector);
				if (_uv1Closure) _uv1Closure(vector);
				break;
			case BinaryParser::kTexCoord2Array:
				if (tex2Found) throw std::runtime_error("Bg2: File format exception. duplicate texcoord2 array");
				tex2Found = true;
				_binaryParser.readArray(vector);
				if (_uv2Closure) _uv2Closure(vector);
				break;
			case BinaryParser::kIndexArray:
				if (indexFound) throw std::runtime_error("Bg2: File format exception. duplicate index array");
				indexFound = true;
				_binaryParser.readArray(ivector);
				if (_indexClosure) _indexClosure(ivector);
				break;
			case BinaryParser::kPolyList:
			case BinaryParser::kEnd:
				done = true;
				break;
			default:
				throw std::runtime_error("Bg2: File format exception. Unexpected poly list member found");
			}
		}
	}

	void Bg2Reader::parseMaterialOverride(const std::string & path) {
		if (_materialOverrideClosure) {
			size_t index = path.find_last_of(".");
			std::string bg2matPath = path.substr(0, index) + ".bg2mat";
			std::cout << bg2matPath << std::endl;
			std::ifstream file;
			file.open(bg2matPath);
			if (file.good()) {
				std::string result((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
				_materialOverrideClosure(result);
				file.close();
			}
		}
	}

}
}
