#ifndef JSON_HPP_
#define JSON_HPP_

#include <string>
#include <vector>
#include <map>



// TODO - to commonTools
// ============================== general stuff - operators - operator < must be defined
#define RELATIONAL_OPERATORS(tType) \
inline bool operator> (tType const & a, tType const & b) { return   (b < a); } \
inline bool operator<=(tType const & a, tType const & b) { return ! (b < a); } \
inline bool operator>=(tType const & a, tType const & b) { return ! (a < b); } \
inline bool operator==(tType const & a, tType const & b) { return ! (a < b || b < a); } \
inline bool operator!=(tType const & a, tType const & b) { return   (a < b || b < a); }


namespace json {

	class Value;

	typedef bool                        Boolean;
	typedef long long int               Integer;
	typedef std::string                 String;
	typedef std::vector<Value>          Array;
	typedef std::map<std::string,Value> Object;

	enum ValueType {
		  null    = 0
		, boolean
		, integer
		, string
		, array
		, object
	};

	class Value {
	private:
		ValueType fType = ValueType::null;
		union {
			void * pointer;
			Integer integer;
			Boolean boolean;
		} fData;
		inline void freeMem();
		inline void copyFrom(Value const & v);
		inline void set(Boolean);
		inline void set(Integer);
		inline void set(char const *);
		inline void set(String const &);
		inline void set(Array  const &);
		inline void set(Object const &);
	public:
		static Value const null;
		static Value const array;
		static Value const object;
		Value() {}
		Value(Value const & v) { copyFrom(v); }
		Value(Value && v) : fType(v.fType), fData(v.fData) { v.fType = ValueType::null; }
		~Value() { freeMem(); }
		Value & operator=(Value const & v) { freeMem(); copyFrom(v); return *this; }
		Value & operator=(Value && v) { std::swap(fType,v.fType); std::swap(fData,v.fData); return *this; }
		Value(Boolean        v) { set(v); }
		Value(Integer        v) { set(v); }
		Value(char const *   v) { set(v); }
		Value(String const & v) { set(v); }
		Value(Array  const & v) { set(v); }
		Value(Object const & v) { set(v); }
		Value & operator=(Boolean        v) { freeMem(); set(v); return *this; }
		Value & operator=(Integer        v) { freeMem(); set(v); return *this; }
		Value & operator=(char const *   v) { freeMem(); set(v); return *this; }
		Value & operator=(String const & v) { freeMem(); set(v); return *this; }
		Value & operator=(Array  const & v) { freeMem(); set(v); return *this; }
		Value & operator=(Object const & v) { freeMem(); set(v); return *this; }
		Boolean & asBoolean() { if (fType != ValueType::boolean) std::logic_error("json::Value - not a boolean" ); return fData.boolean; }
		Integer & asInteger() { if (fType != ValueType::integer) std::logic_error("json::Value - not an integer"); return fData.integer; }
		String  & asString () { if (fType != ValueType::string ) std::logic_error("json::Value - not a string"  ); return *(reinterpret_cast<String*>(fData.pointer)); }
		Array   & asArray  () { if (fType != ValueType::array  ) std::logic_error("json::Value - not an array"  ); return *(reinterpret_cast<Array *>(fData.pointer)); }
		Object  & asObject () { if (fType != ValueType::object ) std::logic_error("json::Value - not an object" ); return *(reinterpret_cast<Object*>(fData.pointer)); }
		Boolean const & asBoolean() const { if (fType != ValueType::boolean) std::logic_error("json::Value - not a boolean" ); return fData.boolean; }
		Integer const & asInteger() const { if (fType != ValueType::integer) std::logic_error("json::Value - not an integer"); return fData.integer; }
		String  const & asString () const { if (fType != ValueType::string ) std::logic_error("json::Value - not a string"  ); return *(reinterpret_cast<String*>(fData.pointer)); }
		Array   const & asArray  () const { if (fType != ValueType::array  ) std::logic_error("json::Value - not an array"  ); return *(reinterpret_cast<Array *>(fData.pointer)); }
		Object  const & asObject () const { if (fType != ValueType::object ) std::logic_error("json::Value - not an object" ); return *(reinterpret_cast<Object*>(fData.pointer)); }
		ValueType type() const { return fType; }
		std::string toString() const;  // compact - no white-spaces
		std::string toStringPretty(unsigned indent = 0) const; // formatted, indent is added after every EOL, so first line is not affected, there is no EOL at the end
		// Array specific
		Value & operator[](unsigned i)
		{
			if (fType != ValueType::array) *this = array;
			Array * a = reinterpret_cast<Array*>(fData.pointer);
			if (a->size() <= i) a->resize(i+1);
			return (*a)[i];
		}
		Value & at(unsigned i)
		{
			if (fType != ValueType::array) std::runtime_error("json::Value - not an array");
			return reinterpret_cast<Array*>(fData.pointer)->at(i);
		}
		Value const & at(unsigned i) const
		{
			if (fType != ValueType::array) std::runtime_error("json::Value - not an array");
			return reinterpret_cast<Array*>(fData.pointer)->at(i);
		}
		void push_back(Value const & v)
		{
			if (fType != ValueType::array) *this = array;
			return reinterpret_cast<Array*>(fData.pointer)->push_back(v);
		}
		// Object specific
		Value & operator[](std::string const & i)
		{
			if (fType != ValueType::object) *this = object;
			Object * o = reinterpret_cast<Object*>(fData.pointer);
			return (*o)[i];
		}
		Value & at(std::string const & i)
		{
			if (fType != ValueType::object) std::runtime_error("json::Value - not an object");
			return reinterpret_cast<Object*>(fData.pointer)->at(i);
		}
		Value const & at(std::string const & i) const
		{
			if (fType != ValueType::object) std::runtime_error("json::Value - not an object");
			return reinterpret_cast<Object*>(fData.pointer)->at(i);
		}
		// operator less
		bool operator<(Value const & v) const
		{
			if (fType == v.fType) {
				switch (fType) {
					case ValueType::null   : return false;
					case ValueType::boolean: return (fData.boolean < v.fData.boolean);
					case ValueType::integer: return (fData.integer < v.fData.integer);
					case ValueType::string : return (*reinterpret_cast<String*>(fData.pointer) < *reinterpret_cast<String*>(v.fData.pointer));
					case ValueType::array  : return (*reinterpret_cast<Array *>(fData.pointer) < *reinterpret_cast<Array *>(v.fData.pointer));
					case ValueType::object : return (*reinterpret_cast<Object*>(fData.pointer) < *reinterpret_cast<Object*>(v.fData.pointer));
				}
			}
			return (fType < v.fType);
		}
	};
	RELATIONAL_OPERATORS(Value);


	inline void Value::freeMem()
	{
		switch (fType) {
			case ValueType::null   :
			case ValueType::boolean:
			case ValueType::integer: break;
			case ValueType::string : delete (reinterpret_cast<std::string*>(fData.pointer)); break;
			case ValueType::array  : delete (reinterpret_cast<Array      *>(fData.pointer)); break;
			case ValueType::object : delete (reinterpret_cast<Object     *>(fData.pointer)); break;
		}
	}

	inline void Value::copyFrom(Value const & v)
	{
		switch (v.fType) {
			case ValueType::null   : fType = ValueType::null; break;
			case ValueType::boolean: set(v.fData.boolean); break;
			case ValueType::integer: set(v.fData.integer); break;
			case ValueType::string : set(*reinterpret_cast<std::string*>(v.fData.pointer)); break;
			case ValueType::array  : set(*reinterpret_cast<Array      *>(v.fData.pointer)); break;
			case ValueType::object : set(*reinterpret_cast<Object     *>(v.fData.pointer)); break;
		}
	}

	inline void Value::set(Boolean        v) { fType = ValueType::boolean; fData.boolean = v; }
	inline void Value::set(Integer        v) { fType = ValueType::integer; fData.integer = v; }
	inline void Value::set(char   const * v) { if (v == nullptr) fType = ValueType::null; else set(std::string(v)); }
	inline void Value::set(String const & v) { fType = ValueType::string ; fData.pointer = new std::string(v); }
	inline void Value::set(Array  const & v) { fType = ValueType::array  ; fData.pointer = new Array      (v); }
	inline void Value::set(Object const & v) { fType = ValueType::object ; fData.pointer = new Object     (v); }
}


#endif /* JSON_HPP_ */
