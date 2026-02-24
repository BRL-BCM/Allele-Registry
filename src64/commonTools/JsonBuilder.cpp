
#include <sstream>
#include <functional>
#include <algorithm>
#include <map>

#include "../debug.hpp"

#include "JsonBuilder.hpp"


namespace jsonBuilder {

	void JsonTreeBase::removeEmptyArraysAndObjects()
	{
		// --- remove empty arrays
		for ( auto it = arraysOfValues.begin();  it != arraysOfValues.end(); ) {
			if (it->second.empty()) {
				auto it2 = it;
				++it;
				arraysOfValues.erase(it2);
			} else {
				++it;
			}
		}
		// --- process arrays of objects (subnodes), empty subtrees are deleted and removed
		for ( auto & kv: arraysOfObjects ) {
			std::vector<JsonTreeBase*> & subnodes = kv.second;
			unsigned iVerifiedCount = 0;
			unsigned iFirstDeleted = subnodes.size();
			while (iVerifiedCount < iFirstDeleted) {
				subnodes[iVerifiedCount]->removeEmptyArraysAndObjects();
				if (subnodes[iVerifiedCount]->isEmpty()) {
					delete subnodes[iVerifiedCount];
					subnodes[iVerifiedCount] = nullptr;
					std::swap( subnodes[iVerifiedCount], subnodes[--iFirstDeleted] );
				} else {
					++iVerifiedCount;
				}
			}
			subnodes.resize(iVerifiedCount);
		}
		// --- remove empty arrays
		for ( auto it = arraysOfObjects.begin();  it != arraysOfObjects.end(); ) {
			if (it->second.empty()) {
				auto it2 = it;
				++it;
				arraysOfObjects.erase(it2);
			} else {
				++it;
			}
		}
	}


	struct PrintField
	{
		PathBase const fields;
		enum class FieldContentType { value, arrayOfValues, arrayOfObjects, subdocument };
		FieldContentType const fieldType;
		union {
			std::string value;
			std::vector<std::string> arrayOfValues;
			std::vector<JsonTreeBase*> arrayOfObjects;
		};
		PrintField(PathBase const & f, std::string const & s, bool subdocument = false)
		: fields(f), fieldType( subdocument ? FieldContentType::subdocument : FieldContentType::value)
		{
			new (&(value))std::string(s);
		}
		PrintField(PathBase const & f, std::vector<std::string> const & s) : fields(f), fieldType(FieldContentType::arrayOfValues)
		{
			new (&(arrayOfValues))std::vector<std::string>(s);
		}
		PrintField(PathBase const & f, std::vector<JsonTreeBase*> const & s) : fields(f), fieldType(FieldContentType::arrayOfObjects)
		{
			new (&(arrayOfObjects))std::vector<JsonTreeBase*>(s);
		}
		PrintField(PrintField const & p) : fields(p.fields), fieldType(p.fieldType)
		{
			switch (fieldType) {
			case FieldContentType::subdocument   :
			case FieldContentType::value         : new (&(value))std::string(p.value); break;
			case FieldContentType::arrayOfValues : new (&(arrayOfValues))std::vector<std::string>(p.arrayOfValues); break;
			case FieldContentType::arrayOfObjects: new (&(arrayOfObjects))std::vector<JsonTreeBase*>(p.arrayOfObjects); break;
			}
		}
		~PrintField()
		{
			switch (fieldType) {
			case FieldContentType::subdocument   :
			case FieldContentType::value         : value.~basic_string<char>()         ; break;
			case FieldContentType::arrayOfValues : arrayOfValues.~vector<std::string>(); break;
			case FieldContentType::arrayOfObjects: arrayOfObjects.~vector<JsonTreeBase*>() ; break;
			}
		}
	};


	template<bool tPretty> inline std::string formatEol();
	template<> inline std::string formatEol<false>() { return ""; }
	template<> inline std::string formatEol<true >() { return "\n"; }

	template<bool tPretty> inline std::string formatIndent(unsigned const indent);
	template<> inline std::string formatIndent<false>(unsigned const indent) { return ""; }
	template<> inline std::string formatIndent<true >(unsigned const indent) { return std::string(2*indent,' '); }

	template<bool tPretty> inline std::string formatFieldValueSeparator();
	template<> inline std::string formatFieldValueSeparator<false>() { return ":"; }
	template<> inline std::string formatFieldValueSeparator<true >() { return ": "; }

	template<bool tPretty>
	std::string JsonTreeBase::toString(std::array<std::string,256> const & textLabels, unsigned indentSize) const
	{
		std::map<PathBase,PrintField*> allFields;
		for (auto kv: subdocuments   ) allFields[kv.first] = new PrintField(kv.first, kv.second, true);
		for (auto kv: values         ) allFields[kv.first] = new PrintField(kv.first, kv.second);
		for (auto kv: arraysOfValues ) allFields[kv.first] = new PrintField(kv.first, kv.second);
		for (auto kv: arraysOfObjects) allFields[kv.first] = new PrintField(kv.first, kv.second);

		std::vector<PrintField const *> printInstructions;
		printInstructions.reserve(values.size() + arraysOfValues.size() + arraysOfObjects.size());
		for (auto kv: allFields) printInstructions.push_back(kv.second);

		std::ostringstream str;
		PathBase lastPath;
		for (PrintField const * p: printInstructions) {
			// find number of last common labels with the previous entry
			unsigned commonLabels = 0;
			for ( ; commonLabels < std::min(lastPath.length(),p->fields.length()); ++commonLabels) {
				if (lastPath[commonLabels] != p->fields[commonLabels]) break;
			}
			for (unsigned i = lastPath.length(); i > commonLabels+1; --i) {
				str << formatEol<tPretty>() << formatIndent<tPretty>(indentSize+i-1) << "}";
			}
			if (lastPath.length() == 0) {
				str << formatIndent<tPretty>(indentSize) << "{"; // the first one
			} else {
				str << ",";
			}
			for (unsigned i = commonLabels; i+1 < p->fields.length(); ++i) {
				str << formatEol<tPretty>() << formatIndent<tPretty>(indentSize+i+1) << jsonString(textLabels[p->fields[i]]);
				str << formatFieldValueSeparator<tPretty>() << "{";
			}
			str << formatEol<tPretty>() << formatIndent<tPretty>(indentSize+p->fields.length()) << jsonString(textLabels[p->fields.last()]) << formatFieldValueSeparator<tPretty>();
			switch (p->fieldType) {
			case PrintField::FieldContentType::subdocument:
				if (tPretty) {
					std::string::size_type bol = 0;
					while (true) {
						std::string::size_type eol = p->value.find('\n', bol);
						if (eol == std::string::npos) {
							str << p->value.substr(bol);
							break;
						}
						str << p->value.substr(bol,++eol-bol) << formatIndent<tPretty>(indentSize+p->fields.length());
						bol = eol;
					}
				} else {
					str << p->value;
				}
				break;
			case PrintField::FieldContentType::value:
				str << p->value;
				break;
			case PrintField::FieldContentType::arrayOfValues:
				str << "[";
				for (unsigned i = 0; i+1 < p->arrayOfValues.size(); ++i) {
					str << formatEol<tPretty>() << formatIndent<tPretty>(indentSize+p->fields.length()+1) << p->arrayOfValues[i] << ",";
				}
				if ( ! p->arrayOfValues.empty() ) {
					str << formatEol<tPretty>() << formatIndent<tPretty>(indentSize+p->fields.length()+1) << p->arrayOfValues.back();
				}
				str << formatEol<tPretty>() << formatIndent<tPretty>(indentSize+p->fields.length()) << "]";
				break;
			case PrintField::FieldContentType::arrayOfObjects:
				str << "[";
				for (unsigned i = 0; i+1 < p->arrayOfObjects.size(); ++i) {
					str << formatEol<tPretty>() << p->arrayOfObjects[i]->toString<tPretty>(textLabels,indentSize+p->fields.length()+1) << ",";
				}
				if ( ! p->arrayOfObjects.empty() ) {
					str << formatEol<tPretty>() << p->arrayOfObjects.back()->toString<tPretty>(textLabels,indentSize+p->fields.length()+1);
				}
				str << formatEol<tPretty>() << formatIndent<tPretty>(indentSize+p->fields.length()) << "]";
				break;
			}
			lastPath = p->fields;
		}
		for (unsigned i = lastPath.length(); i > 0; --i) {
			str << formatEol<tPretty>() << formatIndent<tPretty>(indentSize+i-1) << "}";
		}

		// free memory
		for (auto kv: allFields) delete kv.second;

		return str.str();
	}


	// Instantiations
	template std::string JsonTreeBase::toString<false>(std::array<std::string,256> const &, unsigned indentSize) const;
	template std::string JsonTreeBase::toString<true >(std::array<std::string,256> const &, unsigned indentSize) const;
}
