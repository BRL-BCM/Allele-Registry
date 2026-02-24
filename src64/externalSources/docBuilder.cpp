#include "docBuilder.hpp"
#include "../commonTools/JsonBuilder.hpp"
#include "../core/exceptions.hpp"

namespace externalSources
{

	void prepareDocumentStructure
		( jsonBuilder::DocumentStructure<label> & docSchema
		, jsonBuilder::DocumentSettings const & docConf
		, Dictionary<label> const & dict
		, std::map<unsigned,Source> const & sources
		)
	{
		// create full structure
		for (auto const & kv: sources) {
			uint8_t srcId = kv.first;
			docSchema[srcId][label::_at_context].addNode();
			docSchema[srcId][label::_at_id].addNode();
			docSchema[srcId][label::guiLabel].addNode();
			docSchema[srcId][label::guiUrl].addNode();
		}
		// apply document settings
		if (docConf.layout != jsonBuilder::DocumentSettings::full) {
			docSchema.hideAllNodes();
		}
		for (auto const & mw: docConf.modificators) {
			bool const active = (mw.first == jsonBuilder::DocumentSettings::add);
			std::vector<std::string> const & path = mw.second;
			jsonBuilder::NodeOfDocumentStructure<label> n = docSchema.root();
			for ( auto const & p: path ) {
				label id;
				if (! dict.tryTextToLabel(p,id)) {
					throw ExceptionIncorrectRequest("Unknown field name: '" + p + "'");
				}
				n = n[id];
			}
			n.setState(active);
		}
	}

	// parse param ID (1-9) and set min number length if included in param definition
	// return 0 if parameter is set to id
	inline unsigned parseParam(std::string const & def, unsigned & outMinNumberLength)
	{
		if ( def.size() < 2 || ((def[0] != 'p' || def[1] < '1' || def[1] > '9') && (def[0] != 'i' || def[1] != 'd')) ) {
			throw std::logic_error("Incorrect parameter definition: " + def);
		}
		if (def.size() > 2) {
			if (def[2] != '_' || def.size() == 3) throw std::logic_error("Incorrect parameter definition: " + def);
			outMinNumberLength = 0;
			for (unsigned i = 3; i < def.size(); ++i) {
				if (def[i] < '0' || def[i] > '9') throw std::logic_error("Incorrect parameter definition: " + def);
				outMinNumberLength *= 10;
				outMinNumberLength += (def[3] - '0');
			}
		}
		if (def[0] == 'i') return 0;
		return (def[1] - '0');
	}

	// parse param value from binaries
	inline unsigned long long parseValueInteger(std::vector<uint8_t> const & data, unsigned paramId)
	{
		auto it = data.begin();
		for (  ; paramId;  --paramId ) {
			if (it >= data.end()) throw std::logic_error("Incorrect binary representation of parameters values (1)");
			it += (*it) + 1;
		}
		if (it >= data.end()) throw std::logic_error("Incorrect binary representation of parameters values (2)");
		auto itEnd = it + 1 + (*it);
		if (itEnd > data.end()) throw std::logic_error("Incorrect binary representation of parameters values (3)");
		unsigned long long r = 0;
		for ( ++it;  it != itEnd;  ++it ) {
			r <<= 8;
			r += *it;
		}
		return r;
	}

	// parse param value from binaries
	inline std::string parseValueString(std::vector<uint8_t> const & data, unsigned paramId)
	{
		auto it = data.begin();
		for (  ; paramId;  --paramId ) {
			if (it >= data.end()) throw std::logic_error("Incorrect binary representation of parameters values (4)");
			it += (*it) + 1;
		}
		if (it >= data.end()) throw std::logic_error("Incorrect binary representation of parameters values (5)");
		auto itEnd = it + 1 + (*it);
		if (itEnd > data.end()) throw std::logic_error("Incorrect binary representation of parameters values (6)");
		std::string r = "";
		for ( ++it;  it != itEnd;  ++it ) {
			r.push_back( static_cast<char>(*it) );
		}
		return r;
	}

	// return URL filled with parameters
	inline std::string generateURL(std::string const & url, std::string paramTypes, std::vector<uint8_t> const & data, unsigned extId)
	{
		std::vector<bool> params(paramTypes.size()+1,true); // true - integer, false - string
		for (unsigned i = 0; i < paramTypes.size(); ++i) {
			params[i+1] = (paramTypes[i] == 'I');
		}

		std::string result = "";
		for ( std::string::size_type iBegin = 0;  iBegin < url.size();  ++iBegin ) {
			// copy static part of the url
			std::string::size_type iOpenBracket = url.find('{',iBegin);
			if (iOpenBracket == std::string::npos) {
				result.append(url.substr(iBegin));
				// end of url - exit
				break;
			} else {
				result.append(url.substr(iBegin,iOpenBracket-iBegin));
			}
			// extract parameter substitution from the url
			iBegin = url.find('}',iOpenBracket);
			if (iBegin == std::string::npos) throw std::logic_error("Incorrect pattern: " + url);
			std::string const paramSubst = url.substr(iOpenBracket+1,iBegin-iOpenBracket-1);
			// parse the parameter substitution
			unsigned minNumberLength = 0;
			unsigned paramId = parseParam(paramSubst, minNumberLength);
			if (paramId >= params.size()) {
				throw std::logic_error("Incorrect parameter(" + paramSubst + ") in pattern: " + url);
			}
			// append parameter's value to the url
			if (params[paramId]) {
				std::string const val = std::to_string( (paramId>0) ? (parseValueInteger(data, paramId-1)) : (extId) );
				if (val.size() < minNumberLength) {
					result.append( std::string(minNumberLength-val.size(),'0') );
				}
				result.append(val);
			} else {
				result.append( parseValueString(data, paramId-1) );
			}
		}

		return result;
	}


	void validatePattern(std::string const & pattern, std::string const & paramTypes)
	{
		std::vector<uint8_t> data(paramTypes.size(), static_cast<uint8_t>(0u));
		generateURL(pattern, paramTypes, data, 1234);
	}


	struct DocBuilder::Pim
	{
		std::map<unsigned,Source> sources;
		jsonBuilder::DocumentStructure<externalSources::label> docSchema;
		Dictionary<label> dictionary;
		bool compressedJson;
		Pim(  std::map<unsigned,Source> const & pSources
			, jsonBuilder::DocumentStructure<label> const & pDocSchema
			, Dictionary<label> const & pDictionary
			, bool prettyJson
			) : sources(pSources), docSchema(pDocSchema), dictionary(pDictionary), compressedJson(!prettyJson) {}
	};

	DocBuilder::DocBuilder
			( std::map<unsigned,Source> const & pSources
			, jsonBuilder::DocumentStructure<label> const & pDocSchema
			, Dictionary<label> const & pDictionary
			, bool prettyJson
			) : pim(new Pim(pSources, pDocSchema, pDictionary, prettyJson))
	{}

	DocBuilder::~DocBuilder()
	{
		delete pim;
	}

	std::string DocBuilder::createJson(unsigned extId, std::vector<std::pair<unsigned,std::vector<uint8_t>>> const & doc) const
	{
		jsonBuilder::JsonTree<label> jsonDoc(&(pim->docSchema));
		jsonBuilder::NodeOfJsonTree<label> out = jsonDoc.getContext();

		for (auto const & kv: doc) {
			uint8_t srcId = kv.first;
			auto const & data = kv.second;
			Source const & src = pim->sources[srcId];
			out[srcId].push_back( [&src,&data,&extId](jsonBuilder::NodeOfJsonTree<label> const & node)
									{
										node[label::_at_id  ] = generateURL(src.url     , src.paramTypes, data, extId);
										node[label::guiLabel] = generateURL(src.guiLabel, src.paramTypes, data, extId);
										node[label::guiUrl  ] = generateURL(src.guiUrl  , src.paramTypes, data, extId);
									} );
		}

		jsonDoc.removeEmptyArraysAndObjects();
		std::string r;
		if (pim->compressedJson) {
			r = jsonDoc.toString<false>(pim->dictionary.textLabelsById());
		} else {
			r = jsonDoc.toString<true>(pim->dictionary.textLabelsById());
		}
		return r;
	}

}
