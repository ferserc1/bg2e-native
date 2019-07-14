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


#ifndef _bg2model_bg2_reader_hpp_
#define _bg2model_bg2_reader_hpp_

#include <bg2file/binary_parser.hpp>
#include <bg2base/path.hpp>

#include <functional>

namespace bg2file {
	
class Bg2Reader {
public:
	struct FileMetadata {
		int numberOfPolyList;
	};
	
	typedef std::function<void(uint8_t,uint8_t,uint8_t)> VersionClosure;
	typedef std::function<void(const std::exception & e)> ExceptionClosure;
	typedef std::function<void(const FileMetadata & m)> MetadataClosure;
	typedef std::function<void(float offset[3], float pitch, float roll, float yaw)> JointClosure;
	typedef std::function<void(const std::string &)> StringClosure;
	typedef std::function<void(const std::vector<float> &)> VectorClosure;
	typedef std::function<void(const std::vector<unsigned int> &)> IndexClosure;
	
	Bg2Reader() {}

	bool load(const std::string & path);
	inline bool load(const bg2base::path& path) { return load(path.toString()); }
	inline bool load(const char * path) { return load(std::string(path)); }
	bool loadHeader(const std::string & path);
	inline bool loadHeader(const bg2base::path& path) { return loadHeader(path.toString()); }
	inline bool loadHeader(const char* path) { return load(std::string(path)); }


	inline Bg2Reader & version(VersionClosure c) { _versionClosure = c; return *this; }
	inline Bg2Reader & materials(StringClosure c) { _materialClosure = c; return *this; }
	inline Bg2Reader & error(ExceptionClosure c) { _exceptionClosure = c; return *this; }
	inline Bg2Reader & metadata(MetadataClosure c) { _metadataClosure = c; return *this; }
	inline Bg2Reader & inJoint(JointClosure c) { _inJointClosure = c; return *this; }
	inline Bg2Reader & outJoint(JointClosure c) { _outJointClosure = c; return *this; }
	inline Bg2Reader & plistName(StringClosure c) { _plistNameClosure = c; return *this; }
	inline Bg2Reader & materialName(StringClosure c) { _materialNameClosure = c; return *this; }
	inline Bg2Reader & vertex(VectorClosure c) { _vertexClosure = c; return *this; }
	inline Bg2Reader & normal(VectorClosure c) { _normalClosure = c; return *this; }
	inline Bg2Reader & uv0(VectorClosure c) { _uv0Closure = c; return *this; }
	inline Bg2Reader & uv1(VectorClosure c) { _uv1Closure = c; return *this; }
	inline Bg2Reader & uv2(VectorClosure c) { _uv2Closure = c; return *this; }
	inline Bg2Reader & index(IndexClosure c) { _indexClosure = c; return *this; }
	inline Bg2Reader & materialOverrides(StringClosure c) { _materialOverrideClosure = c; return *this; }
	
protected:
	void readHeader(std::string & materialsString, unsigned int & numberOfPlist);
	void readPolyList(int numberOfPlist);
	void readSinglePolyList();
	void parseMaterialOverride(const std::string &);

	BinaryParser _binaryParser;
	
	VersionClosure _versionClosure;
	StringClosure _materialClosure;
	ExceptionClosure _exceptionClosure;
	MetadataClosure _metadataClosure;
	JointClosure _inJointClosure;
	JointClosure _outJointClosure;
	StringClosure _plistNameClosure;
	StringClosure _materialNameClosure;
	VectorClosure _vertexClosure;
	VectorClosure _normalClosure;
	VectorClosure _uv0Closure;
	VectorClosure _uv1Closure;
	VectorClosure _uv2Closure;
	IndexClosure _indexClosure;
	StringClosure _materialOverrideClosure;
	
};
	
}


#endif
