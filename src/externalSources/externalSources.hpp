#ifndef EXTERNALSOURCES_EXTERNALSOURCES_HPP_
#define EXTERNALSOURCES_EXTERNALSOURCES_HPP_

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include "../mysql/mysqlConnection.hpp"
#include "../commonTools/DocumentSettings.hpp"

namespace externalSources {

	// must be called once when the server starts (before other threads are started)
	void init(MySqlConnectionParameters const & db);

	// returns true if the given user is authorized to modify the given source
	// throws exception if source does not exist
	bool authorization( std::string const & user, std::string const & sourceName );

	// =============================== LINKS

	// queries links for given identifiers (CA/PA)
	// output documents are organized in the same order as ids parameter, if no results then an empty string is returned instead of JSON document
	void queryLinksByIds
			( std::vector<std::string> & output		// output(overwritten): vector of output JSON documents
			, jsonBuilder::DocumentSettings	const &	// document settings (set of fields to show)
			, bool prettyJson						// pretty JSON or compressed JSON
			, std::vector<uint32_t> const & ids		// set of variants identifiers to fetch
			);

	// fetch identifiers (CA/PA) with links to given external source
	// this function set ids parameter
	// throws exception if source does not exist or last parameter is incorrect
	// the last parameter must be empty (=all links) or match parameters format in the external source
	void queryLinksBySource
			( std::vector<uint32_t> & ids			// output(overwritten): variants identifiers for documents returned in output parameter
			, std::string const & sourceName		// name of external source
			, std::vector<std::string> const &      // parameters of links to return, size of the vector must be the same as number of parameters in the source or empty
			);

	// register new links for given identifiers (CA/PA)
	// throws exception if source does not exist or last parameter is incorrect
	// the last parameter match parameters format in the external source
	void registerLinks
			( std::vector<uint32_t> const & ids		// vector of identifiers of variants to modify
			, std::string const & sourceName		// name of external source name
			, std::vector<std::vector<std::string>> const & // parameters of links to register, size of the vector must be the same as ids vector
			);

	// delete all links for given identifiers (CA/PA) and belonging to given external source
	// throws exception if source does not exist or last parameter is incorrect
	// the last parameter must be empty (=all links) or match parameters format in the external source
	void deleteLinks
			( std::vector<uint32_t> const & ids		// vector of identifiers of variants to modify
			, std::string const & sourceName		// name of external source, whose links will be deleted
			, std::vector<std::vector<std::string>> const & // parameters of links to delete, size of the vector must be the same as ids vector or empty
			);

	// delete all links (to any external source) for given identifiers (CA/PA)
	void deleteLinks
			( std::vector<uint32_t> const & ids		// vector of identifiers of variants to modify
			);

	// delete all links from given external source
	// throws exception if source does not exist
	void deleteLinks
			( std::string const & sourceName		// name of external source, whose links will be deleted
			);

	// ============================= SOURCES

	// queries sources
	// sets output to an array of source document or single source document if sourceName is empty
	// throws exception if sourceName is set and does not exists
	void querySources
			( std::string & output
			, std::string const sourceName = ""
			);

	// creates new source
	// sets output to single source document
	// throws exception if source exists or any parameter is incorrect
	void createSource
			( std::string & output
			, std::string const & srcName
			, std::string const & url
			, std::string const & paramTypes
			, std::string const & guiSrcName
			, std::string const & guiLabel
			, std::string const & guiUrl
			);

	// modifies existing source
	// sets output to single source document
	// throws exception if source does not exist or any parameter is incorrect
	void modifySource
			( std::string & output
			, std::string const & srcName
			, unsigned    const   fieldToUpdate  // 1 - url, 2 - guiSrcName, 3 - guiLabel, 4 - guiUrl
			, std::string const & newValue
			);

	// deletes source and all links
	// sets output to {}
	// throws exception if source does not exists
	void deleteSource
			( std::string & output
			, std::string const & srcName
			);

	// assigns user to source (do nothing if user is already assigned)
	// sets output to single source document
	// throws exception if source does not exists
	void assignUserToSource
			( std::string & output
			, std::string const & sourceName
			, std::string const & userName
			);

	// removes user from source (do nothing if user is not assigned)
	// sets output to single source document
	// throws exception if source does not exists
	void removeUserFromSource
			( std::string & output
			, std::string const & sourceName
			, std::string const & userName
			);

}



#endif /* EXTERNALSOURCES_EXTERNALSOURCES_HPP_ */
