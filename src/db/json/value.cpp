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


#include <bg2e/db/json/value.hpp>
#include <bg2e/db/json/grammar.hpp>
#include <bg2e/db/json/convert.hpp>
#include <bg2e/db/json/escaper.hpp>

#include <cassert>
#include <stack>
#include <sstream>
#include <list>
#include <iomanip>
#include <fstream>
#include <iostream>

namespace bg2e {
namespace db {
namespace json {
		
	bool isHexDigit(char digit);
	bool isWhiteSpace(char whiteSpace);
	void readString(std::istream &input, std::string &result, bool strict);
	void readObject(std::istream &input, Value * result, bool strict);
	void readArray(std::istream &input, Value * result, bool strict);
	void readNumber(std::istream &input, Value * result, bool strict);
	void readToNonWhiteSpace(std::istream &input, char &currentCharacter);


	bool isHexDigit(char digit) {
		return (digit >= Numbers::DIGITS[0] && digit <= Numbers::DIGITS[9]) || (digit >= Numbers::DIGITS[10] && digit <= Numbers::DIGITS[15]) ||
			(digit >= Numbers::DIGITS[16] && digit <= Numbers::DIGITS[21]);
	}

	bool isWhiteSpace(char whiteSpace) {
		return whiteSpace == Whitespace::SPACE ||
			whiteSpace == Whitespace::HORIZONTAL_TAB ||
			whiteSpace == Whitespace::NEW_LINE ||
			whiteSpace == Whitespace::CARRIAGE_RETURN;
	}

	void readString(std::istream &input, std::string &result, bool strict) {
		bool noErrors = true, noUnicodeError = true;
		char currentCharacter, tmpCharacter;
		std::stringstream constructing;
		std::string tmpStr(4, ' ');
		std::stringstream tmpSs;
		int32_t tmpInt;
		String32 tmpStr32;
		unsigned int tmpCounter;

		// As long as there aren't any errors and that we haven't reached the
		// end of the input stream.
		while (noErrors && !input.eof()) {
			input.get(currentCharacter);

			if (input.good()) {
				if (currentCharacter & 0x80) { // 0x80 --> 10000000
												// The character is part of an utf8 character.
					constructing << currentCharacter;

				}
				else if (currentCharacter == Strings::Json::Escape::BEGIN_ESCAPE) {
					if (!input.eof()) {
						input.get(tmpCharacter);

						switch (tmpCharacter) {
						case Strings::Json::Escape::QUOTATION_MARK:
							constructing << Strings::Std::QUOTATION_MARK;
							break;

						case Strings::Json::Escape::REVERSE_SOLIDUS:
							constructing << Strings::Std::REVERSE_SOLIDUS;
							break;

						case Strings::Json::Escape::SOLIDUS:
							constructing << Strings::Std::SOLIDUS;
							break;

						case Strings::Json::Escape::BACKSPACE:
							constructing << Strings::Std::BACKSPACE;
							break;

						case Strings::Json::Escape::FORM_FEED:
							constructing << Strings::Std::FORM_FEED;
							break;

						case Strings::Json::Escape::LINE_FEED:
							constructing << Strings::Std::LINE_FEED;
							break;

						case Strings::Json::Escape::CARRIAGE_RETURN:
							constructing << Strings::Std::CARRIAGE_RETURN;
							break;

						case Strings::Json::Escape::TAB:
							constructing << Strings::Std::TAB;
							break;

						case Strings::Json::Escape::BEGIN_UNICODE:
							// TODO: Check for utf16 surrogate pairs.
							tmpCounter = 0;
							tmpStr.clear();
							tmpStr = "    ";
							noUnicodeError = true;

							while (tmpCounter < 4 && !input.eof()) {
								input.get(tmpCharacter);

								if (isHexDigit(tmpCharacter)) {
									tmpStr[tmpCounter] = tmpCharacter;

								}
								else {
									noUnicodeError = false;
									std::stringstream msg;
									msg << "Invalid \\u character, skipping it.";
									if (strict) {
										throw std::runtime_error("Json warning found in strict mode: " + msg.str());
									}
									else {
										std::cerr << msg.str() << std::endl;
									}
								}

								++tmpCounter;
							}

							if (noUnicodeError) {
								tmpSs.clear();
								tmpSs.str("");
								tmpSs << std::hex << tmpStr;
								tmpSs >> tmpInt;
								tmpStr32.clear();
								tmpStr32.push_back(tmpInt);
								tmpStr = Convert::encodeToUTF8(tmpStr32);
								constructing << tmpStr;
							}

							break;

						default:
							break;
						}
					}

				}
				else if (currentCharacter == '"') {
					result = constructing.str();
					noErrors = false;

				}
				else {
					constructing << currentCharacter;
				}
			}
		}
	}

	void readObject(std::istream &input, Value * result, bool strict) {
		bool noErrors = true;
		char currentCharacter;
		std::string tmpString;

		while (noErrors && !input.eof()) {
			input.get(currentCharacter);

			if (input.good()) {
				if (currentCharacter == Structural::BEGIN_END_STRING) {
					// We read the object's member's name.
					readString(input, tmpString, strict);
					currentCharacter = input.peek();
					// We read white spaces until the next non white space.
					readToNonWhiteSpace(input, currentCharacter);

					if (!input.eof()) {

						// We make sure it's the right character.
						if (currentCharacter == Structural::NAME_SEPARATOR) {
							// We read until the value starts.
							readToNonWhiteSpace(input, currentCharacter);

							if (!input.eof()) {
								// We put the character back and we load the value
								// from the stream.
								input.putback(currentCharacter);
								Value * objValue = new Value();
								objValue->loadFromStream(input, strict);
								result->setValue(tmpString, objValue);

								while (!input.eof() && currentCharacter != Structural::VALUE_SEPARATOR &&
									currentCharacter != Structural::END_OBJECT) {
									input.get(currentCharacter);
								}

								if (currentCharacter == Structural::END_OBJECT) {
									// We are done reading the object.
									noErrors = false;
								}
							}
						}
					}

				}
				else if (currentCharacter == Structural::END_OBJECT) {
					noErrors = false;

				}
				else if (!isWhiteSpace(currentCharacter)) {
					std::stringstream msg;
					msg << "Expected '\"', got '" << currentCharacter << "', ignoring it.";
					if (strict) {
						throw std::runtime_error("Json warning found in strict mode: " + msg.str());
					}
					else {
						std::cerr << msg.str() << std::endl;
					}
					
				}
			}
		}
	}

	void readArray(std::istream &input, Value * result, bool strict) {
		bool notDone = true;
		char currentChar;

		while (notDone && !input.eof()) {
			input.get(currentChar);

			if (input.good()) {
				if (currentChar == Structural::END_ARRAY) {
					notDone = false;

				}
				else if (!isWhiteSpace(currentChar)) {
					input.putback(currentChar);
					Value * newVal = new Value(Value::kArray);
					newVal->loadFromStream(input, strict);
					result->push(newVal);

					while (!input.eof() && currentChar != ',' &&
						currentChar != Structural::END_ARRAY) {
						input.get(currentChar);
					}

					if (currentChar == Structural::END_ARRAY) {
						notDone = false;
					}
				}
			}
		}
	}

	void readNumber(std::istream &input, Value * result, bool strict) {
		bool notDone = true, inFraction = false, inExponent = false;
		char currentCharacter;
		std::stringstream constructing;

		if (!input.eof() && input.peek() == Numbers::DIGITS[0]) {
			// We make sure there isn't more than one zero.
			input.get(currentCharacter);

			if (input.peek() == '0') {
				notDone = false;

			}
			else {
				input.putback(currentCharacter);
			}
		}

		while (notDone && !input.eof()) {
			input.get(currentCharacter);

			if (currentCharacter == '-') {
				if (constructing.str().empty()) {
					constructing << currentCharacter;

				}
				else {
					std::stringstream msg;
					msg << "Expected a digit, '.', 'e' or 'E', got '" << currentCharacter << "' instead, ignoring it.";
					if (strict) {
						throw std::runtime_error("Json warning found in strict mode: " + msg.str());
					}
					else {
						std::cout << msg.str() << std::endl;
					}
				}

			}
			else if (currentCharacter >= '0' && currentCharacter <= '9') {
				constructing << currentCharacter;

			}
			else if (currentCharacter == '.') {
				if (!inFraction && !inExponent) {
					inFraction = true;
					constructing << currentCharacter;
				}

			}
			else if (currentCharacter == 'e' || currentCharacter == 'E') {
				if (!inExponent) {
					inExponent = true;
					constructing << currentCharacter;

					if (!input.eof() && (input.peek() == '-' || input.peek() == '+')) {
						input.get(currentCharacter);
						constructing << currentCharacter;
					}
				}

			}
			else {
				input.putback(currentCharacter);
				notDone = false;
			}
		}

		if (inFraction || inExponent) {
			double doubleResult;
			constructing >> doubleResult;
			result->setValue(doubleResult);
		}
		else {
			int intResult;
			constructing >> intResult;
			result->setValue(intResult);
		}
	}

	void readToNonWhiteSpace(std::istream &input, char &currentCharacter) {
		do {
			input.get(currentCharacter);
		} while (!input.eof() && isWhiteSpace(currentCharacter));
	}

	Value::~Value() {
		switch (_type) {
		case kNull:
	#ifdef _DEBUG
			std::cout << "Destroy json::Value(null)" << std::endl;
	#endif
			break;
		case kNumber:
	#ifdef _DEBUG
			std::cout << "Destroy json::Value(number)" << std::endl;
	#endif
			break;
		case kString:
	#ifdef _DEBUG
			std::cout << "Destroy json::Value(string)" << std::endl;
	#endif
			break;
		case kBool:
	#ifdef _DEBUG
			std::cout << "Destroy json::Value(bool)" << std::endl;
	#endif
			break;
		case kArray:
			for (auto v : _array) {
				delete v;
			}
			_array.clear();
	#ifdef _DEBUG
			std::cout << "Destroy json::Value(array)" << std::endl;
	#endif
			break;
		case kObject:
			for (auto v : _map) {
				delete v.second;
			}
			_map.clear();
	#ifdef _DEBUG
			std::cout << "Destroy json::Value(object)" << std::endl;
	#endif
			break;
		}
	}

	void Value::loadFromStream(std::istream & input, bool strict) {
		char currentCharacter;

		// We check that the stream is in UTF-8.
		char encoding[2];
		input.get(encoding[0]);
		input.get(encoding[1]);

		if (encoding[0] != '\0' && encoding[1] != '\0') {
			// We put the characters back.
			input.putback(encoding[1]);
			input.putback(encoding[0]);

			// Boolean value used to stop reading characters after the value
			// is done loading.
			bool noErrors = true;

			while (noErrors && input.good()) {
				input.get(currentCharacter);

				if (input.good()) {
					if (currentCharacter == Structural::BEGIN_END_STRING) {
						// The value to be parsed is a string.
						setValue("");
						readString(input, _stringVal, strict);
						noErrors = false;

					}
					else if (currentCharacter == Structural::BEGIN_OBJECT) {
						// The value to be parsed is an object.
						readObject(input, this, strict);
						noErrors = false;

					}
					else if (currentCharacter == Structural::BEGIN_ARRAY) {
						// The value to be parsed is an array.
						readArray(input, this, strict);
						noErrors = false;

					}
					else if (currentCharacter == Literals::NULL_STRING[0]) {
						// We try to read the literal 'null'.
						if (!input.eof()) {
							input.get(currentCharacter);

							if (currentCharacter == Literals::NULL_STRING[1]) {
								if (!input.eof()) {
									input.get(currentCharacter);

									if (currentCharacter == Literals::NULL_STRING[2]) {
										if (!input.eof()) {
											input.get(currentCharacter);

											if (currentCharacter == Literals::NULL_STRING[3]) {
												_type = kNull;
												noErrors = false;
											}
											else {
												throw std::runtime_error("invalid characters found");
											}
										}
										else {
											throw std::runtime_error("json input ends incorrectly");
										}
									}
									else {
										throw std::runtime_error("invalid characters found");
									}
								}
								else {
									throw std::runtime_error("json input ends incorrectly");
								}
							}
							else {
								throw std::runtime_error("invalid characters found");
							}
						}
						else {
							throw std::runtime_error("json input ends incorrectly");
						}
					}
					else if (currentCharacter == Numbers::MINUS ||
						(currentCharacter >= Numbers::DIGITS[0] && currentCharacter <= Numbers::DIGITS[9])) {
						// Numbers can't start with zeroes.
						input.putback(currentCharacter);
						readNumber(input, this, strict);
						noErrors = false;
					}
					else if (currentCharacter == Literals::TRUE_STRING[0]) {
						// We try to read the boolean literal 'true'.
						if (!input.eof()) {
							input.get(currentCharacter);

							if (currentCharacter == Literals::TRUE_STRING[1]) {
								if (!input.eof()) {
									input.get(currentCharacter);

									if (currentCharacter == Literals::TRUE_STRING[2]) {
										if (!input.eof()) {
											input.get(currentCharacter);

											if (currentCharacter == Literals::TRUE_STRING[3]) {
												setValue(true);
												noErrors = false;
											}
										}
									}
								}
							}
						}

					}
					else if (currentCharacter == Literals::FALSE_STRING[0]) {
						// We try to read the boolean literal 'false'.
						if (!input.eof()) {
							input.get(currentCharacter);

							if (currentCharacter == Literals::FALSE_STRING[1]) {
								if (!input.eof()) {
									input.get(currentCharacter);

									if (currentCharacter == Literals::FALSE_STRING[2]) {
										if (!input.eof()) {
											input.get(currentCharacter);

											if (currentCharacter == Literals::FALSE_STRING[3]) {
												if (!input.eof()) {
													input.get(currentCharacter);

													if (currentCharacter == Literals::FALSE_STRING[4]) {
														setValue(false);
														noErrors = false;
													}
												}
											}
										}
									}
								}
							}
						}

					}
					else if (!isWhiteSpace(currentCharacter)) {
						std::stringstream msg;
						msg << "Invalid character found: '" << currentCharacter << "'";
						throw std::runtime_error(msg.str());
					}
				}
			}

		}
		else {
			throw std::runtime_error("File is not in UTF-8, not parsing.");
		}
	}

	void Value::writeToStream(std::ostream & stream) {
		int tabs = 0;
		std::function<void(const std::string &, json::Value *, size_t, const json::Value::ValueMap &)> printObject;
		std::function<void(json::Value *, size_t, const json::Value::ValueArray &)> printArray = [&](json::Value * val, size_t index, const json::Value::ValueArray & array) {
			for (auto i = 0; i<tabs; ++i) { stream << "\t"; }
			switch (val->type()) {
			case json::Value::kNull:
				stream << "null";
				break;
			case json::Value::kBool:
				stream << (val->boolValue() ? "true" : "false");
				break;
			case json::Value::kString:
				stream << "\"" << val->stringValue() << "\"";
				break;
			case json::Value::kNumber:
				stream << val->numberValue();
				break;
			case json::Value::kArray:
				stream << "[" << std::endl;
				++tabs;
				val->eachItem(printArray);
				--tabs;
				for (auto i = 0; i<tabs; ++i) { stream << "\t"; }
				stream << "]";
				break;
			case json::Value::kObject:
				stream << "{" << std::endl;
				++tabs;
				val->eachItem(printObject);
				--tabs;
				for (auto i = 0; i<tabs; ++i) { stream << "\t"; }
				stream << "}";
				break;
			}
			if (index + 1<array.size()) {
				stream << ",";
			}
			stream << std::endl;
		};

		printObject = [&](const std::string & key, json::Value * val, size_t index, const json::Value::ValueMap & map) {
			for (auto i = 0; i<tabs; ++i) {
				stream << "\t";
			}
			stream << '\"' << key << "\": ";
			switch (val->type()) {
			case json::Value::kNull:
				stream << "null";
				break;
			case json::Value::kBool:
				stream << (val->boolValue() ? "true" : "false");
				break;
			case json::Value::kString:
				stream << "\"" << val->stringValue() << "\"";
				break;
			case json::Value::kNumber:
				stream << val->numberValue();
				break;
			case json::Value::kArray:
				stream << "[" << std::endl;
				++tabs;
				val->eachItem(printArray);
				--tabs;
				for (auto i = 0; i<tabs; ++i) { stream << "\t"; }
				stream << "]";
				break;
			case json::Value::kObject:
				stream << "{" << std::endl;
				++tabs;
				val->eachItem(printObject);
				--tabs;
				for (auto i = 0; i<tabs; ++i) { stream << "\t"; }
				stream << "}";
				break;
			}
			if (index + 1<map.size()) {
				stream << ",";
			}
			stream << std::endl;
		};

		switch (type()) {
		case kNull:
			stream << "null";
			break;
		case kBool:
			stream << (boolValue() ? "true" : "false");
			break;
		case kString:
			stream << stringValue();
			break;
		case kNumber:
			stream << numberValue();
			break;
		case kArray:
			stream << "[" << std::endl; ++tabs;
			eachItem(printArray);
			--tabs;
			stream << "]" << std::endl;
			break;
		case kObject:
			stream << "{" << std::endl; ++tabs;
			eachItem(printObject);
			--tabs;
			stream << "}" << std::endl;
			break;
		}
	}

	bool Value::Bool(Value * val, bool def) {
		if(val) {
			switch(val->type()) {
			case kBool:
				return val->boolValue();
				break;
			case kNumber:
				return val->intValue() != 0;
				break;
			default:
				return def;
			}
		}
		return def;
	}

	std::string Value::String(Value * val, const std::string & def) {
		if (val) {
			switch (val->type()) {
				case kBool:
					return val->boolValue() ? "true":"false";
				case kString:
					return val->stringValue();
				case kNumber:
					return std::to_string(val->numberValue());
				default:
					return def;
			}
		}
		return def;
	}

	short Value::Short(Value * val, short def) {
		if (val) {
			switch (val->type()) {
				case kBool:
					return 1;
				case kNumber:
					return val->shortValue();
				default:
					break;
			}
		}
		return def;
	}

	int Value::Int(Value * val, int def) {
		if(val) {
			switch(val->type()) {
			case kBool:
				return 1;
			case kNumber:
				return val->intValue();
			default:
				break;
			}
		}
		return def;
	}

	long Value::Long(Value * val, long def) {
		if(val) {
			switch(val->type()) {
			case kBool:
				return 1;
			case kNumber:
				return val->longValue();
			default:
				break;
			}
		}
		return def;
	}

	long long Value::LLong(Value * val, long long def) {
		if(val) {
			switch(val->type()) {
			case kBool:
				return 1;
			case kNumber:
				return val->llongValue();
			default:
				break;
			}
		}
		return def;
	}

	unsigned short Value::UShort(Value * val, unsigned short def) {
		if(val) {
			switch(val->type()) {
			case kBool:
				return 1;
			case kNumber:
				return val->ushortValue();
			default:
				break;
			}
		}
		return def;
	}

	unsigned int Value::UInt(Value * val, unsigned int def) {
		if(val) {
			switch(val->type()) {
			case kBool:
				return 1;
			case kNumber:
				return val->uintValue();
			default:
				break;
			}
		}
		return def;
	}

	unsigned long Value::ULong(Value * val, unsigned long def) {
		if(val) {
			switch(val->type()) {
			case kBool:
				return 1;
			case kNumber:
				return val->ulongValue();
			default:
				break;
			}
		}
		return def;
	}

	unsigned long long Value::ULLong(Value * val, unsigned long long def) {
		if(val) {
			switch(val->type()) {
			case kBool:
				return 1;
			case kNumber:
				return val->ullongValue();
			default:
				break;
			}
		}
		return def;
	}

	float Value::Float(Value * val, float def) {
		if(val) {
			switch(val->type()) {
			case kBool:
				return 1;
			case kNumber:
				return val->floatValue();
			default:
				break;
			}
		}
		return def;
	}

	double Value::Number(Value * val, double def) {
		if(val) {
			switch(val->type()) {
			case kBool:
				return 1;
			case kNumber:
				return val->numberValue();
			default:
				break;
			}
		}
		return def;
	}

	math::float2 Value::Float2(Value * val, const math::float2 & def) {
		math::float2 result;
		if(val && val->type() == kArray && val->array().size() >= 2) {
			result.x() = Float((*val)[0], def.x());
			result.y() = Float((*val)[1], def.y());
		}
		return result;
	}

	math::float3 Value::Float3(Value * val, const math::float3 & def) {
		math::float3 result;
		if(val && val->type() == kArray && val->array().size() >= 3) {
			result.x() = Float((*val)[0], def.x());
			result.y() = Float((*val)[1], def.y());
			result.z() = Float((*val)[2], def.z());
		}
		return result;
	}

	math::float4 Value::Float4(Value * val, const math::float4 & def) {
		math::float4 result;
		if(val && val->type() == kArray && val->array().size() >= 4) {
			result.x() = Float((*val)[0], def.x());
			result.y() = Float((*val)[1], def.y());
			result.z() = Float((*val)[2], def.z());
			result.w() = Float((*val)[3], def.w());
		}
		return result;
	}

	math::color Value::Color(Value * val, const math::color & def) {
		math::color result = def;
		if (val && val->type() == kArray && val->array().size() >= 4) {
			result = math::color(Float((*val)[0], def.x()),
				Float((*val)[1], def.y()),
				Float((*val)[2], def.z()),
				Float((*val)[3], def.w()));
		}
		return result;
	}

	math::float3x3 Value::Float3x3(Value * val, const math::float3x3 & def) {
		math::float3x3 result = def;
		if (val && val->type() == kArray && val->array().size() >= 9) {
			result.setRow(0, math::float3(Float((*val)[0], def.element(0,0)), Float((*val)[1], def.element(0,1)), Float((*val)[2], def.element(0,2))));
			result.setRow(1, math::float3(Float((*val)[3], def.element(1,0)), Float((*val)[4], def.element(1,1)), Float((*val)[5], def.element(2,2))));
			result.setRow(2, math::float3(Float((*val)[6], def.element(2,0)), Float((*val)[7], def.element(2,1)), Float((*val)[8], def.element(2,2))));
		}
		return result;
	}

	math::float4x4 Value::Float4x4(Value * val, const math::float4x4 & def) {
		math::float4x4 result = def;
		if (val && val->type() == kArray && val->array().size() >= 16) {
			result.setRow(0, math::float4(Float((*val)[0], def.element(0,0)), Float((*val)[1], def.element(0,1)), Float((*val)[2], def.element(0,2)), Float((*val)[3], def.element(0,3))));
			result.setRow(1, math::float4(Float((*val)[4], def.element(1,0)), Float((*val)[5], def.element(1,1)), Float((*val)[6], def.element(1,2)), Float((*val)[7], def.element(1,3))));
			result.setRow(2, math::float4(Float((*val)[8], def.element(2,0)), Float((*val)[9], def.element(2,1)), Float((*val)[10], def.element(2,2)), Float((*val)[11], def.element(2,3))));
			result.setRow(3, math::float4(Float((*val)[12], def.element(3,0)), Float((*val)[13], def.element(3,1)), Float((*val)[14], def.element(3,2)), Float((*val)[15], def.element(3,3))));
		}
		return result;
	}

	void Value::FloatVector(Value * val, math::FloatVector & result) {
		if (val && val->type() == kArray) {
			result.clear();
			for (size_t i = 0; i < val->array().size(); ++i) {
				result.push_back(Float((*val)[i]));
			}
		}
	}

	math::int2 Value::Int2(Value * val, const math::int2 & def) {
		math::int2 result = def;
		if (val && val->type() == kArray && val->array().size() >= 2) {
			result = math::int2(Int((*val)[0], def.x()),
				Int((*val)[1], def.y()));
		}
		return result;
	}

	math::int3 Value::Int3(Value * val, const math::int3 & def) {
		math::int3 result = def;
		if (val && val->type() == kArray && val->array().size() >= 3) {
			result = math::int3(Int((*val)[0], def.x()),
				Int((*val)[1], def.y()),
				Int((*val)[2], def.z()));
		}
		return result;
	}

	math::int4 Value::Int4(Value * val, const math::int4 & def) {
		math::int4 result = def;
		if (val && val->type() == kArray && val->array().size() >= 4) {
			result = math::int4(Int((*val)[0], def.x()),
				Int((*val)[1], def.y()),
				Int((*val)[2], def.z()),
				Int((*val)[3], def.w()));
		}
		return result;
	}


}
}
}

