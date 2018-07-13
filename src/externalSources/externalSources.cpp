#include <cstdint>
#include <vector>
#include <string>
#include "db.hpp"
#include "docBuilder.hpp"
#include "core.hpp"
#include "../core/exceptions.hpp"
#include "../commonTools/sharedMutex.hpp"

#include <iostream>

namespace externalSources {

	void convertParamsToBinaryForm(Source const & src, std::vector<std::string> const & paramsText, std::vector<uint8_t> & binary)
	{
		binary.clear();
		char const * paramType = src.paramTypes;

		for (std::string const & text: paramsText) {
			if (*paramType == '\0') {
				return;
			}
			auto i1 = text.begin();
			if (*paramType == 'I') {
				unsigned long long v = 0;
				if (i1 == text.end()) {
					throw ExceptionIncorrectLinksParameters("An empty string was found, but number was expected");
				}
				for ( ;  i1 < text.end();  ++i1 ) {
					if (*i1 < '0' || *i1 > '9') {
						throw ExceptionIncorrectLinksParameters("Incorrect value '" + text + "'. Number was expected");
					}
					v *= 10;
					v += (*i1 - '0');
				}
				unsigned length = 0;
				for ( unsigned long long x = v;  x;  x /= 255 ) ++length;
				binary.push_back(length);
				for ( unsigned i = 0;  i < length;  ++i ) binary.push_back(v >> 8*(length-1-i));
			} else { // string
				if (text.size() > 127) {
					throw ExceptionIncorrectLinksParameters("String value cannot be longer than 127 bytes");
				}
				binary.push_back(text.size());
				for ( ;  i1 < text.end();  ++i1 ) {
					binary.push_back(static_cast<uint8_t>(*i1));
				}
			}
			++paramType;
		}

		if (*paramType != '\0') {
			throw ExceptionIncorrectLinksParameters("Number of parameters does not match");
		}
	}


	MySqlConnectionParameters dbConnectionParams;
	SharedMutex globalMutex;

	void init(MySqlConnectionParameters const & dbParams)
	{
		dbConnectionParams = dbParams;
	}


	bool authorization( std::string const & user, std::string const & sourceName )
	{
		if (user == "admin") return true;

		SharedLockGuard guard(globalMutex);
		std::unique_ptr<DB> db( new DB(MySqlConnection::connect(dbConnectionParams)) );

		Source src;
		db->fetchSource(sourceName, src);

		return (db->isAssignedToSource(user,src.srcId));
	}


	void queryLinksByIds
			( std::vector<std::string> & output		// output(overwritten): vector of output JSON documents
			, jsonBuilder::DocumentSettings	const &	docConf // document settings (set of fields to show)
			, bool prettyJson						// pretty JSON or compressed JSON
			, std::vector<uint32_t> const & ids		// set of variants identifiers to fetch
			)
	{
		output.clear();
		if (ids.empty()) return;

		SharedLockGuard guard(globalMutex);
		std::unique_ptr<DB> db( new DB(MySqlConnection::connect(dbConnectionParams)) );

		// fetch list of sources
		std::map<unsigned,Source> sources;
		db->fetchSources(sources);

		// prepare dictionary
		Dictionary<label> dictionary;
		for (auto const & kv: sources) {
			dictionary.addLabel(kv.first, kv.second.srcName);
		}

		// prepare document schema
		jsonBuilder::DocumentStructure<label> docSchema;
		prepareDocumentStructure(docSchema, docConf, dictionary, sources);

		// prepare list of external sources to fetch
		std::vector<unsigned> sourcesIdsToFetch;
		std::vector<label> nodes = docSchema.root().descendants();
		for (label l: nodes) sourcesIdsToFetch.push_back(l);

		// fetch links
		std::vector<std::vector<std::pair<unsigned,std::vector<uint8_t>>>> links;
		db->fetchLinksByIds(links, ids, sourcesIdsToFetch);

		// build documents
		DocBuilder docBuilder(sources,docSchema,dictionary,prettyJson);
		output.clear();
		output.reserve(links.size());
		for ( unsigned i = 0;  i < links.size();  ++i ) {
			auto const & data = links[i];
			output.push_back( docBuilder.createJson(ids[i],data) );
		}
	}


	void queryLinksBySource
			( std::vector<uint32_t> & ids			// output(overwritten): variants identifiers for matched links
			, std::string const & sourceName		// name of external source
			, std::vector<std::string> const & param // parameters of links to return, size of the vector must be the same as number of parameters in the source or empty
			)
	{
		SharedLockGuard guard(globalMutex);
		std::unique_ptr<DB> db( new DB(MySqlConnection::connect(dbConnectionParams)) );

		Source source;
		db->fetchSource(sourceName,source);

		// convert parameters
		std::vector<uint8_t> binParam;
		if ( ! param.empty()) {
			if (param.size() != std::strlen(source.paramTypes)) {
				throw ExceptionIncorrectLinksParameters("Number of parameters does not match");
			}
			convertParamsToBinaryForm(source, param, binParam);
		}

		// fetch links
		db->fetchLinksIdsBySourceAndParams(ids, source.srcId, binParam);
	}


	void registerLinks
			( std::vector<uint32_t> const & ids		// vector of identifiers of variants to modify
			, std::string const & sourceName		// name of external source name
			, std::vector<std::vector<std::string>> const & params // parameters of links to register, size of the vector must be the same as ids vector
			)
	{
		ASSERT(ids.size() == params.size());
		if (ids.empty()) return;

		SharedLockGuard guard(globalMutex);
		std::unique_ptr<DB> db( new DB(MySqlConnection::connect(dbConnectionParams)) );

		Source source;
		db->fetchSource(sourceName, source);

		// convert params to binary form
		std::vector<std::vector<std::pair<unsigned,std::vector<uint8_t>>>> links(params.size(),std::vector<std::pair<unsigned,std::vector<uint8_t>>>(1));
		for ( unsigned i = 0;  i < params.size();  ++i ) {
			links[i].front().first = source.srcId;
			convertParamsToBinaryForm(source, params[i], links[i].front().second);
		}

		db->insertLinks(links,ids);
	}


	void deleteLinks
			( std::vector<uint32_t> const & ids		// vector of identifiers of variants to modify
			, std::string const & sourceName		// name of external source, whose links will be deleted
			, std::vector<std::vector<std::string>> const & params // parameters of links to delete, size of the vector must be the same as ids vector or empty
			)
	{
		ASSERT(ids.size() == params.size() || params.empty());
		if (ids.empty()) return;

		SharedLockGuard guard(globalMutex);
		std::unique_ptr<DB> db( new DB(MySqlConnection::connect(dbConnectionParams)) );

		Source source;
		db->fetchSource(sourceName, source);

		if (params.empty()) {
			db->deleteLinksByIds(ids, source.srcId);
		} else {
			// convert params to binary form
			std::vector<std::vector<uint8_t>> links(params.size());
			for ( unsigned i = 0;  i < params.size();  ++i ) {
				convertParamsToBinaryForm(source, params[i], links[i]);
			}
			// delete links
			db->deleteLinksByIds(ids, source.srcId, links);
		}
	}


	void deleteLinks
			( std::vector<uint32_t> const & ids		// vector of identifiers of variants to modify
			)
	{
		if (ids.empty()) return;

		SharedLockGuard guard(globalMutex);
		std::unique_ptr<DB> db( new DB(MySqlConnection::connect(dbConnectionParams)) );

		db->deleteLinksByIds(ids);
	}


	void deleteLinks
			( std::string const & sourceName		// name of external source, whose links will be deleted
			)
	{
		SharedLockGuard guard(globalMutex);
		std::unique_ptr<DB> db( new DB(MySqlConnection::connect(dbConnectionParams)) );

		Source source;
		db->fetchSource(sourceName, source);

		db->deleteLinksBySource( std::vector<unsigned>(1,source.srcId) );
	}


	// ============================= SOURCES

	inline std::string createJsonWithSource(Source const & src, std::vector<std::string> const & users)
	{
		std::string const indent = "    ";
		std::string out = "";
		out.append("{\n");
		out.append(indent + "\"name\": "        + jsonBuilder::jsonString(src.srcName   ) + ",\n");
		out.append(indent + "\"url\": "         + jsonBuilder::jsonString(src.url       ) + ",\n");
		out.append(indent + "\"guiName\": "     + jsonBuilder::jsonString(src.guiSrcName) + ",\n");
		out.append(indent + "\"guiLabel\": "    + jsonBuilder::jsonString(src.guiLabel  ) + ",\n");
		out.append(indent + "\"guiUrl\": "      + jsonBuilder::jsonString(src.guiUrl    ) + ",\n");
		out.append(indent + "\"paramTypes\": "  + jsonBuilder::jsonString(src.paramTypes) + ",\n");
		out.append(indent + "\"users\": [");
		for (std::string const & user: users) out.append( jsonBuilder::jsonString(user) +"," );
		if (users.empty()) out.append("]"); else out.back() = ']';
		out.append("\n}");
		return out;
	}

	// query sources
	void querySources
			( std::string & output
			, std::string const sourceName
			)
	{
		SharedLockGuard guard(globalMutex);
		std::unique_ptr<DB> db( new DB(MySqlConnection::connect(dbConnectionParams)) );

		// fetch list of sources users
		std::map<unsigned,std::vector<std::string>> users;
		db->fetchSourcesUsers(users);

		// create output
		if (sourceName.empty()) {
			// fetch list of sources
			std::map<unsigned,Source> sources;
			db->fetchSources(sources);
			// create output
			output = "[";
			for (auto const & kv: sources) {
				output.append("\n");
				output.append( createJsonWithSource(kv.second,users[kv.first]) );
				output.append(",");
			}
			if (! sources.empty()) {
				output.back() = '\n';
			}
			output.append("]");
		} else {
			// fetch source
			Source source;
			db->fetchSource(sourceName, source);
			// create output
			output = createJsonWithSource(source,users[source.srcId]);
		}
	}


	// create new source
	void createSource
			( std::string & output
			, std::string const & srcName
			, std::string const & url
			, std::string const & paramTypes
			, std::string const & guiSrcName
			, std::string const & guiLabel
			, std::string const & guiUrl
			)
	{
		ExclusiveLockGuard guard(globalMutex);
		std::unique_ptr<DB> db( new DB(MySqlConnection::connect(dbConnectionParams)) );

		if (srcName.empty()) {
			throw ExceptionIncorrectRequest("External source name cannot be empty");
		}

		Source src;
		src.srcName = srcName;
		src.url = url;
		src.guiSrcName = guiSrcName;
		src.guiLabel = guiLabel;
		src.guiUrl = guiUrl;

		if (paramTypes.size() > 3) {
			throw ExceptionIncorrectRequest("External source cannot have more than 3 parameters");
		}
		for (unsigned i = 0; i < paramTypes.size(); ++i) {
			if (paramTypes[i] != 'I' && paramTypes[i] != 'S') {
				throw ExceptionIncorrectRequest("External source can have only two types of parameters: S (string) and I (unsigned integer)");
			}
			src.paramTypes[i] = paramTypes[i];
		}
		src.paramTypes[paramTypes.length()] = '\0';

		std::string paramName = "url";
		try {
			validatePattern(src.url, src.paramTypes);
			paramName = "guiLabel";
			validatePattern(src.guiLabel, src.paramTypes);
			paramName = "guiUrl";
			validatePattern(src.guiUrl, src.paramTypes);
		} catch (std::exception const & e) {
			throw ExceptionIncorrectRequest("The pattern given in parameter " + paramName + " is incorrect. Details: " + e.what());
		}

		db->insertSource(src);

		output = createJsonWithSource(src,std::vector<std::string>());
	}


	// modify source
	void modifySource
			( std::string & output
			, std::string const & srcName
			, unsigned    const   fieldToUpdate  // 1 - url, 2 - guiSrcName, 3 - guiLabel, 4 - guiUrl
			, std::string const & newValue
			)
	{
		SharedLockGuard guard(globalMutex);
		std::unique_ptr<DB> db( new DB(MySqlConnection::connect(dbConnectionParams)) );

		Source source;
		db->fetchSource(srcName,source);

		std::string paramName = "";
		switch (fieldToUpdate) {
			case 1: source.url        = newValue; paramName = "url"     ; break;
			case 2: source.guiSrcName = newValue;                         break;
			case 3: source.guiLabel   = newValue; paramName = "guiLabel"; break;
			case 4: source.guiUrl     = newValue; paramName = "guiUrl"  ; break;
			default: ASSERT(false);
		}

		if (paramName != "") {
			try {
				validatePattern(newValue, source.paramTypes);
			} catch (std::exception const & e) {
				throw ExceptionIncorrectRequest("The pattern given in parameter " + paramName + " is incorrect. Details: " + e.what());
			}
		}

		db->updateSource(source);

		std::map<unsigned,std::vector<std::string>> users;
		db->fetchSourcesUsers(users);

		output = createJsonWithSource(source,users[source.srcId]);
	}


	// delete source and all links
	void deleteSource
			( std::string & output
			, std::string const & srcName
			)
	{
		ExclusiveLockGuard guard(globalMutex);
		std::unique_ptr<DB> db( new DB(MySqlConnection::connect(dbConnectionParams)) );

		Source source;
		db->fetchSource(srcName,source);

		db->deleteLinksBySource(std::vector<unsigned>(1,source.srcId));
		db->removeUsersFromSource(source.srcId);
		db->deleteSource(source.srcId);

		output = "{}";
	}


	void assignUserToSource
			( std::string & output
			, std::string const & sourceName
			, std::string const & userName
			)
	{
		SharedLockGuard guard(globalMutex);
		std::unique_ptr<DB> db( new DB(MySqlConnection::connect(dbConnectionParams)) );

		Source source;
		db->fetchSource(sourceName,source);

		db->assignUserToSource(source.srcId, userName);

		std::map<unsigned,std::vector<std::string>> users;
		db->fetchSourcesUsers(users);

		output = createJsonWithSource(source,users[source.srcId]);
	}


	void removeUserFromSource
			( std::string & output
			, std::string const & sourceName
			, std::string const & userName
			)
	{
		SharedLockGuard guard(globalMutex);
		std::unique_ptr<DB> db( new DB(MySqlConnection::connect(dbConnectionParams)) );

		Source source;
		db->fetchSource(sourceName,source);

		db->removeUsersFromSource(source.srcId, std::vector<std::string>(1,userName) );

		std::map<unsigned,std::vector<std::string>> users;
		db->fetchSourcesUsers(users);

		output = createJsonWithSource(source,users[source.srcId]);
	}

}
