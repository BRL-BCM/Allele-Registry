#ifndef EXTERNALSOURCES_DOCBUILDER_HPP_
#define EXTERNALSOURCES_DOCBUILDER_HPP_

#include "core.hpp"
#include <vector>
#include <string>
#include <map>
#include "../commonTools/DocumentSettings.hpp"
#include "../commonTools/JsonBuilder.hpp"


namespace externalSources {


	void prepareDocumentStructure
		( jsonBuilder::DocumentStructure<label> & docSchema
		, jsonBuilder::DocumentSettings const & docConf
		, Dictionary<label> const & dict
		, std::map<unsigned,Source> const & sources
		);

	// throws exception if pattern is incorrect
	void validatePattern(std::string const & pattern, std::string const & paramTypes);


	class DocBuilder
	{
	private:
		struct Pim;
		Pim * pim;
	public:
		DocBuilder(std::map<unsigned,Source> const &, jsonBuilder::DocumentStructure<label> const & docSchema, Dictionary<label> const & dict, bool prettyJson);
		virtual ~DocBuilder();
		std::string createJson(unsigned extId, std::vector<std::pair<unsigned,std::vector<uint8_t>>> const & doc) const; // vector<srcId->params>
	};

}


#endif /* EXTERNALSOURCES_DOCBUILDER_HPP_ */
