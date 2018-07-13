#include "json.hpp"
#include <sstream>

namespace json {

	Value const Value::null;
	Value const Value::array  = std::vector<Value>(0);
	Value const Value::object = std::map<std::string,Value>();

	static inline std::string jsonString(std::string const & v)
	{
		std::string s = "";
		s.reserve(v.size()*3/2);
		s.push_back('"');
		for (char c: v) {
			switch (c) {
				// case '/' : THIS IS NOT CLEAR AT json.org
				case '"' :
				case '\\': s.push_back('\\'); s.push_back(c); break;
				case '\b': s.push_back('\b'); break;
				case '\f': s.push_back('\f'); break;
				case '\n': s.push_back('\n'); break;
				case '\r': s.push_back('\r'); break;
				case '\t': s.push_back('\t'); break;
				default  : s.push_back(c); break;
			}
		}
		s.push_back('"');
		return s;
	}

	// compact - no white-spaces
	std::string Value::toString() const
	{
		std::ostringstream str;
		switch (fType) {
			case ValueType::boolean: if (fData.boolean) str << "true"; else str << "false"; break;
			case ValueType::integer: str << fData.integer; break;
			case ValueType::null   : str << "null"; break;
			case ValueType::string : str << jsonString(*reinterpret_cast<std::string*>(fData.pointer)); break;
			case ValueType::object :
			{
				Object const & a = *reinterpret_cast<Object*>(fData.pointer);
				if (a.empty()) {
					str << "{}";
				} else {
					str << "{";
					auto i = a.begin();
					while ( true ) {
						auto next_i = i;
						if (++next_i == a.end()) break;
						str << jsonString(i->first) << ":" << i->second.toString() << ",";
						i = next_i;
					}
					str << jsonString(i->first) << ":" << i->second.toString() << "}";
				}
				break;
			}
			case ValueType::array  :
			{
				Array const & a = *reinterpret_cast<Array*>(fData.pointer);
				if (a.empty()) {
					str << "[]";
				} else {
					str << "[";
					for ( unsigned i = 0;  i+1 < a.size();  ++i ) str << a[i].toString() << ",";
					str << a.back().toString() << "]";
				}
				break;
			}
		}
		return str.str();
	}


	// formatted, indent is added after every EOL, so first line is not affected, there is no EOL at the end
	std::string Value::toStringPretty(unsigned indent) const
	{
		unsigned const indentStepSize = 2;
		std::string const indentStep(indentStepSize, ' ');
		std::ostringstream str;
		std::string const eol = "\n" + std::string(indent,' ');
		switch (fType) {
			case ValueType::boolean:
			case ValueType::integer:
			case ValueType::null   :
			case ValueType::string :
				str << toString();
				break;
			case ValueType::object :
			{
				Object const & a = *reinterpret_cast<Object*>(fData.pointer);
				if (a.empty()) {
					str << "{}";
				} else {
					str << "{" << eol;
					auto i = a.begin();
					while ( true ) {
						auto next_i = i;
						if (++next_i == a.end()) break;
						str << indentStep << jsonString(i->first) << ": " << i->second.toStringPretty(indent+indentStepSize) << "," << eol;
						i = next_i;
					}
					str << indentStep << jsonString(i->first) << ": " << i->second.toStringPretty(indent+indentStepSize) << eol << "}";
				}
				break;
			}
			case ValueType::array  :
			{
				Array const & a = *reinterpret_cast<Array*>(fData.pointer);
				if (a.empty()) {
					str << "[]";
				} else {
					str << "[" << eol;
					for ( unsigned i = 0;  i+1 < a.size();  ++i ) str << indentStep << a[i].toStringPretty(indent+indentStepSize) << "," << eol;
					str << indentStep << a.back().toStringPretty(indent+indentStepSize) << eol << "]";
				}
				break;
			}
		}
		return str.str();
	}

}
