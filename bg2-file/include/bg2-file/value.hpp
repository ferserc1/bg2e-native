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

#ifndef _bg2model_json_value_hpp_
#define _bg2model_json_value_hpp_

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <iostream>

#include <bg2-file/types.hpp>

namespace bg2file {
namespace json {

class Value {
public:
	enum Type {
		kNull = 0,
		kNumber,
		kString,
		kBool,
		kArray,
		kObject
	};
	
	typedef std::vector<Value*> ValueArray;
	typedef std::map<std::string, Value*> ValueMap;
	
	Value() :_type(kNull), _boolVal(false), _numberVal(0.0) {}
	Value(Type t) :_type(t), _boolVal(false), _numberVal(0.0) {}
	Value(bool v) :_type(kBool), _boolVal(v), _numberVal(0.0) {}
	Value(const char * v) :_type(kString), _boolVal(false), _stringVal(v), _numberVal(0.0) {}
	Value(char v) :_type(kString), _boolVal(false), _numberVal(0.0) { _stringVal = v; }
	Value(const std::string & v) :_type(kString), _boolVal(false), _stringVal(v), _numberVal(0.0) {}
	Value(short v) :_type(kNumber), _boolVal(false), _numberVal(static_cast<double>(v)) {}
	Value(int v) :_type(kNumber), _boolVal(false), _numberVal(static_cast<double>(v)) {}
	Value(long v) :_type(kNumber), _boolVal(false), _numberVal(static_cast<double>(v)) {}
	Value(long long v) :_type(kNumber), _boolVal(false), _numberVal(static_cast<double>(v)) {}
	Value(unsigned short v) :_type(kNumber), _boolVal(false), _numberVal(static_cast<double>(v)) {}
	Value(unsigned int v) :_type(kNumber), _boolVal(false), _numberVal(static_cast<double>(v)) {}
	Value(unsigned long v) :_type(kNumber), _boolVal(false), _numberVal(static_cast<double>(v)) {}
	Value(unsigned long long v) :_type(kNumber), _boolVal(false), _numberVal(static_cast<double>(v)) {}
	Value(float v) :_type(kNumber), _boolVal(false), _numberVal(static_cast<double>(v)) {}
	Value(double v) :_type(kNumber), _boolVal(false), _numberVal(v) {}
	Value(const ValueArray & arr) :_type(kArray), _boolVal(false), _numberVal(0.0), _array(arr) {}
	Value(const ValueMap & map) :_type(kObject), _boolVal(false), _numberVal(0.0), _map(map) {}

	virtual ~Value();
	
	inline Type type() const { return _type; }
	
	inline bool boolValue() const { return _boolVal; }
	inline const std::string & stringValue() const { return _stringVal; }
	
	inline short shortValue() const { return static_cast<short>(_numberVal); }
	inline int intValue() const { return static_cast<int>(_numberVal); }
	inline long longValue() const { return static_cast<long>(_numberVal); }
	inline long long llongValue() const { return static_cast<long long>(_numberVal); }
	inline unsigned short ushortValue() const { return static_cast<unsigned short>(_numberVal); }
	inline unsigned int uintValue() const { return static_cast<unsigned int>(_numberVal); }
	inline unsigned long ulongValue() const { return static_cast<unsigned long>(_numberVal); }
	inline unsigned long long ullongValue() const { return static_cast<unsigned long long>(_numberVal); }
	inline float floatValue() const { return static_cast<float>(_numberVal); }
	inline double numberValue() const { return _numberVal; }
	
	inline const ValueArray & array() const { return _array; }
	inline ValueArray & array() { return _array; }
	inline const ValueMap & map() const { return _map; }
	inline ValueMap & map() { return _map; }
	
	
	static bool Bool(Value * val, bool def = false);
	static std::string String(Value * val, const std::string & def = "");
	
	static short Short(Value * val, short def = 0);
	static int Int(Value * val, int def = 0);
	static long Long(Value * val, long def = 0);
	static long long LLong(Value * val, long long def = 0);
	static unsigned short UShort(Value * val, unsigned short def = 0);
	static unsigned int UInt(Value * val, unsigned int def = 0);
	static unsigned long ULong(Value * val, unsigned long def = 0);
	static unsigned long long ULLong(Value * val, unsigned long long def = 0);
	static float Float(Value * val, float def = 0.0f);
	static double Number(Value * val, double def = 0.0);
	
    static Vec2 Vector2(Value * val, const Vec2 & def = Vec2{ { 0.f, 0.f } });
    static Vec3 Vector3(Value * val, const Vec3 & def = Vec3{ { 0.f, 0.f, 0.f } });
    static Vec4 Vector4(Value * val, const Vec4 & def = Vec4{ { 0.f, 0.f, 0.f, 0.f } });
	
	inline void setValue(bool b) { _type = kBool; _boolVal = b; }
	inline void setValue(const char * v) { _type = kString; _stringVal = v; }
	inline void setValue(char v) { _type = kString; _stringVal = v; }
	inline void setValue(const std::string & v) { _type = kString; _stringVal = v; }
	inline void setValue(short v) { _type = kNumber; _numberVal = static_cast<double>(v); }
	inline void setValue(int v) { _type = kNumber; _numberVal = static_cast<double>(v); }
	inline void setValue(long v) { _type = kNumber; _numberVal = static_cast<double>(v); }
	inline void setValue(long long v) { _type = kNumber; _numberVal = static_cast<double>(v); }
	inline void setValue(unsigned short v) { _type = kNumber; _numberVal = static_cast<double>(v); }
	inline void setValue(unsigned int v) { _type = kNumber; _numberVal = static_cast<double>(v); }
	inline void setValue(unsigned long v) { _type = kNumber; _numberVal = static_cast<double>(v); }
	inline void setValue(unsigned long long v) { _type = kNumber; _numberVal = static_cast<double>(v); }
	inline void setValue(float v) { _type = kNumber; _numberVal = static_cast<double>(v); }
	inline void setValue(double v) { _type = kNumber; _numberVal = v; }
	inline void setValue(const ValueArray & v) { _type = kArray; _array = v; }
	inline void setValue(const ValueMap & v) { _type = kObject; _map = v; }
	
	// Specific array functions
	inline void push(bool v) { _type = kArray; _array.push_back(new Value(v)); }
	inline void push(const char * v) { _type = kArray; _array.push_back(new Value(v)); }
	inline void push(char v) { _type = kArray; _array.push_back(new Value(v)); }
	inline void push(const std::string & v) { _type = kArray; _array.push_back(new Value(v)); }
	inline void push(short v) { _type = kArray; _array.push_back(new Value(v)); }
	inline void push(int v) { _type = kArray; _array.push_back(new Value(v)); }
	inline void push(long v) { _type = kArray; _array.push_back(new Value(v)); }
	inline void push(long long v) { _type = kArray; _array.push_back(new Value(v)); }
	inline void push(unsigned short v) { _type = kArray; _array.push_back(new Value(v)); }
	inline void push(unsigned int v) { _type = kArray; _array.push_back(new Value(v)); }
	inline void push(unsigned long v) { _type = kArray; _array.push_back(new Value(v)); }
	inline void push(unsigned long long v) { _type = kArray; _array.push_back(new Value(v)); }
	inline void push(float v) { _type = kArray; _array.push_back(new Value(v)); }
	inline void push(double v) { _type = kArray; _array.push_back(new Value(v)); }
	inline void push(const ValueArray & v) { _type = kArray; _array.push_back(new Value(v)); }
	inline void push(const ValueMap & v) { _type = kArray; _array.push_back(new Value(v)); }
	inline void push(Value * val) { _type = kArray; _array.push_back(val ? val: new Value(kNull)); }
	
	// Specific object functions
	inline void setValue(const std::string & key, bool v) { _type = kObject; _map[key] = new Value(v); }
	inline void setValue(const std::string & key, const char * v) { _type = kObject; _map[key] = new Value(v); }
	inline void setValue(const std::string & key, char v) { _type = kObject; _map[key] = new Value(v); }
	inline void setValue(const std::string & key, const std::string & v) { _type = kObject; _map[key] = new Value(v); }
	inline void setValue(const std::string & key, short v) { _type = kObject; _map[key] = new Value(v); }
	inline void setValue(const std::string & key, int v) { _type = kObject; _map[key] = new Value(v); }
	inline void setValue(const std::string & key, long v) { _type = kObject; _map[key] = new Value(v); }
	inline void setValue(const std::string & key, long long v) { _type = kObject; _map[key] = new Value(v); }
	inline void setValue(const std::string & key, unsigned short v) { _type = kObject; _map[key] = new Value(v); }
	inline void setValue(const std::string & key, unsigned int v) { _type = kObject; _map[key] = new Value(v); }
	inline void setValue(const std::string & key, unsigned long v) { _type = kObject; _map[key] = new Value(v); }
	inline void setValue(const std::string & key, unsigned long long v) { _type = kObject; _map[key] = new Value(v); }
	inline void setValue(const std::string & key, float v) { _type = kObject; _map[key] = new Value(v); }
	inline void setValue(const std::string & key, double v) { _type = kObject; _map[key] = new Value(v); }
	inline void setValue(const std::string & key, const ValueArray & v) { _type = kObject; _map[key] = new Value(v); }
	inline void setValue(const std::string & key, const ValueMap & v) { _type = kObject; _map[key] = new Value(v); }
	inline void setValue(const std::string & key, Value * val) { _type = kObject; _map[key] = val ? val : new Value(kNull); }
	
	inline Value * operator[](size_t i) { return _array.size()>i ? _array[i]:nullptr; }
	
	inline Value * operator[](const std::string & key) { return _map[key]; }
	
	// Iterate array items
	inline void eachItem(std::function<void (Value *, size_t, const ValueArray &)> lambda) {
		size_t i = 0;
		for (auto item : _array) {
			lambda(item, i++, _array);
		}
	}
	
	inline void eachItem(std::function<void (Value *, size_t)> lambda) {
		size_t i = 0;
		for (auto item : _array) {
			lambda(item, i++);
		}
	}
	
	inline void eachItem(std::function<void (Value *)> lambda) {
		for (auto item : _array) {
			lambda(item);
		}
	}
	
	// Iterate object items
	inline void eachItem(std::function<void (const std::string &, Value *, size_t, const ValueMap &)> lambda) {
		size_t i = 0;
		for (auto item : _map) {
			lambda(item.first, item.second, i++, _map);
		}
	}
	
	inline void eachItem(std::function<void (const std::string &, Value *, size_t)> lambda) {
		size_t i = 0;
		for (auto item : _map) {
			lambda(item.first, item.second, i++);
		}
	}
	
	inline void eachItem(std::function<void (const std::string &, Value *)> lambda) {
		for (auto item : _map) {
			lambda(item.first, item.second);
		}
	}
	
	void loadFromStream(std::istream & input, bool strict);
	
	void writeToStream(std::ostream & stream);

protected:
	
	Type _type;
	bool _boolVal;
	std::string _stringVal;
	double _numberVal;
	ValueArray _array;
	ValueMap _map;
};

}
}

#endif
