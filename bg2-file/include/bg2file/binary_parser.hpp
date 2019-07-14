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

#ifndef _bg2model_binary_parser_hpp_
#define _bg2model_binary_parser_hpp_

#include <string>
#include <vector>
#include <fstream>

namespace bg2file {

class BinaryParser {
public:
	enum OpenMode {
		kModeClosed,
		kModeRead,
		kModeWrite
	};
	enum BlockType {
		kHeader = 'hedr',			// cabecera, seguido del número de polyLists que hay en el fichero y a continuación los materiales (bloque 'mtrl')
		kPolyList = 'plst',			// polyList
		kVertexArray = 'varr',		// Vector de vértices
		kNormalArray = 'narr',		// Vector de normales
		kTexCoord0Array = 't0ar',	// Vector de coordenadas de textura
		kTexCoord1Array = 't1ar',	// Vector de coordenadas de textura
		kTexCoord2Array = 't2ar',	// Vector de coordenadas de textura
		kTexCoord3Array = 't3ar',	// Vector de coordenadas de textura
		kTexCoord4Array = 't4ar',	// Vector de coordenadas de textura
		kIndexArray = 'indx',		// Vector de índices
		kMaterials = 'mtrl',		// Materiales
		kPlistName = 'pnam',		// Nombre del polyList
		kMatName = 'mnam',			// Nombre del material para un plist
		kShadowProjector = 'proj',	// Sombra proyectada
		kJoint = 'join',			// Junta física
		kEnd = 'endf'				// Final del fichero
	};
	BinaryParser();
	virtual ~BinaryParser();

	bool open(const std::string & path, OpenMode mode);

	void close();

	bool writeByte(unsigned char byte);
	bool writeBlock(BlockType b);
	bool writeInteger(int value);
	bool writeFloat(float value);
	bool writeString(const std::string & str);
	bool writeArray(const std::vector<float> array);
	bool writeArray(const std::vector<int> array);
	bool writeArray(const std::vector<unsigned int> array);

	bool readByte(unsigned char & byte);
	bool readBlock(BlockType & b);
	bool readInteger(int & value);
	bool readFloat(float & value);
	bool readString(std::string & str);
	bool readArray(std::vector<float> & array);
	bool readArray(std::vector<int> & array);
	bool readArray(std::vector<unsigned int> & array);

	void setBigEndian();
	void setLittleEndian();

	void seekForward(int bytes);

	std::fstream & stream() { return _stream; }
	
	bool isBigEndian();
	bool isLittleEndian();

protected:
	std::fstream _stream;
	OpenMode _mode;
	bool _swapBytes;
};


}

#endif
