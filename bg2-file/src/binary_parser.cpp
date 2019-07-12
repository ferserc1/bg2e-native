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


#include <bg2-file/binary_parser.hpp>

#include <iostream>

namespace bg2file {

void writeIntegerValue(std::fstream & file, int value, bool swapBytes) {
	if (!swapBytes) file.write(reinterpret_cast<const char *>(&value), sizeof(int));
	else {
		const char * c_value = reinterpret_cast<const char*>(&value);
		file.write(&c_value[3], 1);
		file.write(&c_value[2], 1);
		file.write(&c_value[1], 1);
		file.write(&c_value[0], 1);
	}
}

int readIntegerValue(std::fstream & file, int & value, bool swapBytes) {
	int readed = 0;
	if (!swapBytes) readed = static_cast<int>(file.read(reinterpret_cast<char *>(&value), sizeof(int)).gcount());
	else {
		union {
			int integer;
			char byte[4];
		} r_value;
		file.read(&r_value.byte[3], 1);
		file.read(&r_value.byte[2], 1);
		file.read(&r_value.byte[1], 1);
		readed = static_cast<int>(file.read(&r_value.byte[0], 1).gcount());
		value = r_value.integer;
	}
	return readed;
}

void writeFloatValue(std::fstream & file, float value, bool swapBytes) {
	if (!swapBytes) file.write(reinterpret_cast<const char*>(&value), sizeof(float));
	else {
		const char * c_value = reinterpret_cast<const char*>(&value);
		file.write(&c_value[3], 1);
		file.write(&c_value[2], 1);
		file.write(&c_value[1], 1);
		file.write(&c_value[0], 1);
	}
}

int readFloatValue(std::fstream & file, float & value, bool swapBytes) {
	int readed = 0;
	if (!swapBytes) readed = static_cast<int>(file.read(reinterpret_cast<char*>(&value), sizeof(float)).gcount());
	else {
		union {
			float floating;
			char byte[4];
		} r_value;
		file.read(&r_value.byte[3], 1);
		file.read(&r_value.byte[2], 1);
		file.read(&r_value.byte[1], 1);
		readed = static_cast<int>(file.read(&r_value.byte[0], 1).gcount());
		value = r_value.floating;
	}
	return readed;
}

void writeStringValue(std::fstream & file, const std::string & str, bool swapBytes) {
	writeIntegerValue(file, (int)str.length(), swapBytes);
	const char * buffer = str.c_str();
	file.write(buffer, str.length());
}

void writeFloatArray(std::fstream & file, const std::vector<float> fArray, bool swapBytes) {
	writeIntegerValue(file, static_cast<int>(fArray.size()),swapBytes);
	for (int i=0;i<static_cast<int>(fArray.size());++i) {
		writeFloatValue(file, fArray[i],swapBytes);
	}
}

void writeIntegerArray(std::fstream & file, const std::vector<int> iArray, bool swapBytes) {
	writeIntegerValue(file, static_cast<int>(iArray.size()),swapBytes);
	for (int i=0;i<static_cast<int>(iArray.size());++i) {
		writeIntegerValue(file, iArray[i],swapBytes);
	}
}

void writeIntegerArray(std::fstream & file, const std::vector<unsigned int> iArray, bool swapBytes) {
	writeIntegerValue(file, static_cast<int>(iArray.size()),swapBytes);
	for (int i=0;i<static_cast<int>(iArray.size());++i) {
		writeIntegerValue(file, iArray[i],swapBytes);
	}
}

int readFloatArray(std::fstream & file, std::vector<float> & fArray, bool swapBytes) {
	int size = 0;
	int readed = 0;
	readed = readIntegerValue(file, size, swapBytes);
	if (readed == 0) return 0;
	fArray.reserve(size);
	float value;

	for (int i=0;i<size;++i) {
		readed += readFloatValue(file, value, swapBytes);
		fArray.push_back(value);
	}

	return readed;
}

int readIntegerArray(std::fstream & file, std::vector<int> & iArray, bool swapBytes) {
	int size = 0;
	int readed = 0;
	readed = readIntegerValue(file, size, swapBytes);
	if (readed == 0) return 0;
	iArray.reserve(size);
	int value;

	for (int i=0;i<size;++i) {
		readed += readIntegerValue(file, value,swapBytes);
		iArray.push_back(value);
	}
	return readed;
}

int readIntegerArray(std::fstream & file, std::vector<unsigned int> & iArray, bool swapBytes) {
	int size = 0;
	int readed = 0;
	readed = readIntegerValue(file, size, swapBytes);
	if (readed == 0) return 0;
	iArray.reserve(size);
	int value;

	for (int i=0;i<size;++i) {
		readed += readIntegerValue(file, value, swapBytes);
		iArray.push_back(value);
	}

	return readed;
}

int readStringValue(std::fstream & file, std::string & str, bool swapBytes) {
	int stringSize;
	int readed = 0;
	char * buffer;
	readed = readIntegerValue(file, stringSize,swapBytes);

	if (readed == 0) return 0;

	buffer = new char[stringSize + 1];
	readed += static_cast<int>(file.read(buffer, stringSize).gcount());
	buffer[stringSize] = '\0';
	str = buffer;
	delete[] buffer;

	return readed;
}


BinaryParser::BinaryParser() :_mode(BinaryParser::kModeClosed) {
}

BinaryParser::~BinaryParser() {
	close();
}

bool BinaryParser::open(const std::string &path, OpenMode mode) {
	close();
	_stream.open(path,((mode==OpenMode::kModeRead) ? std::ios::in:std::ios::out) | std::ios::binary );
	if (_stream.is_open()) _mode = mode;
	return _mode!=kModeClosed;
}

void BinaryParser::close() {
	if (_stream.is_open()) {
		_stream.close();
	}
	_mode = kModeClosed;
}

bool BinaryParser::writeByte(unsigned char byte) {
	if (_mode!=kModeWrite) return false;
	_stream.write(reinterpret_cast<char*>(&byte), 1);
	return true;
}

bool BinaryParser::writeBlock(BinaryParser::BlockType b) {
	if (_mode!=kModeWrite) return false;
	writeInteger(static_cast<int>(b));
	return true;
}

bool BinaryParser::writeInteger(int value) {
	if (_mode!=kModeWrite) return false;
	writeIntegerValue(_stream, value, _swapBytes);
	return true;
}

bool BinaryParser::writeFloat(float value) {
	if (_mode!=kModeWrite) return false;
	writeFloatValue(_stream,value, _swapBytes);
	return true;
}

bool BinaryParser::writeString(const std::string & str) {
	if (_mode!=kModeWrite) return false;
	writeStringValue(_stream,str, _swapBytes);
	return true;
}

bool BinaryParser::writeArray(const std::vector<float> array) {
	if (_mode!=kModeWrite) return false;
	writeFloatArray(_stream,array, _swapBytes);
	return true;
}

bool BinaryParser::writeArray(const std::vector<int> array) {
	if (_mode!=kModeWrite) return false;
	writeIntegerArray(_stream,array, _swapBytes);
	return true;
}

bool BinaryParser::writeArray(const std::vector<unsigned int> array) {
	if (_mode!=kModeWrite) return false;
	writeIntegerArray(_stream,array, _swapBytes);
	return true;
}

bool BinaryParser::readByte(unsigned char & byte) {
	if (_mode!=kModeRead) return false;
	return _stream.read(reinterpret_cast<char*>(&byte), 1).gcount()==1;
}

bool BinaryParser::readBlock(BinaryParser::BlockType & b) {
	if (_mode!=kModeRead) return false;
	return readInteger(reinterpret_cast<int&>(b))!=0;
}

bool BinaryParser::readInteger(int & value) {
	if (_mode!=kModeRead) return false;
	return readIntegerValue(_stream, value, _swapBytes)!=0;
}

bool BinaryParser::readFloat(float & value) {
	if (_mode!=kModeRead) return false;
	return readFloatValue(_stream, value, _swapBytes)!=0;
}

bool BinaryParser::readString(std::string & str) {
	if (_mode!=kModeRead) return false;
	return readStringValue(_stream, str, _swapBytes)!=0;
}

bool BinaryParser::readArray(std::vector<float> & array) {
	if (_mode!=kModeRead) return false;
	return readFloatArray(_stream,array, _swapBytes)!=0;
}

bool BinaryParser::readArray(std::vector<int> & array) {
	if (_mode!=kModeRead) return false;
	return readIntegerArray(_stream, array, _swapBytes)!=0;
}

bool BinaryParser::readArray(std::vector<unsigned int> & array) {
	if (_mode!=kModeRead) return false;
	return readIntegerArray(_stream, array, _swapBytes)!=0;
}

bool BinaryParser::isBigEndian() {
    unsigned int v = 0x01020304;
	return reinterpret_cast<unsigned char*>(&v)[0] == 1;;
}

bool BinaryParser::isLittleEndian() {
	unsigned int v = 0x01020304;
	return reinterpret_cast<unsigned char*>(&v)[0] == 4;
}

void BinaryParser::setBigEndian() {
	if (isBigEndian()) {
		_swapBytes = false;
	}
	else {
		_swapBytes = true;
	}
}

void BinaryParser::setLittleEndian() {
	if (isLittleEndian()) {
		_swapBytes = false;
	}
	else {
		_swapBytes = true;
	}
}

void BinaryParser::seekForward(int bytes) {
	_stream.seekg(bytes, std::ios::cur);
}

}
