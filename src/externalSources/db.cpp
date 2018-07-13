#include "db.hpp"
#include "../commonTools/assert.hpp"
#include <algorithm>
#include <string>
#include "../core/exceptions.hpp"


namespace externalSources
{

	template<typename tElement>
	void buildSortTransformation(std::vector<tElement> const & data, std::vector<unsigned> & indices)
	{
		indices.resize(data.size());
		for (unsigned i = 0; i < indices.size(); ++i) indices[i] = i;
		std::sort( indices.begin(), indices.end(), [&data](unsigned const i1, unsigned const i2)->bool{ return (data[i1] < data[i2]); } );
	}

	void buildReverseTransformation(std::vector<unsigned> const & indices, std::vector<unsigned> & indicesReverse)
	{
		indicesReverse.resize(indices.size());
		for (unsigned i = 0; i < indices.size(); ++i) indicesReverse[indices[i]] = i;
	}

	template<typename tElement>
	void applyTransformation(std::vector<tElement> const & data, std::vector<unsigned> const & indices, std::vector<tElement> & dataTransformed)
	{
		dataTransformed.resize(data.size());
		for (unsigned i = 0; i < indices.size(); ++i) dataTransformed[i] = data[indices[i]];
	}

	inline std::string toHex(std::vector<uint8_t> const & v)
	{
		std::string s(v.size()*2,' ');
		for (unsigned i = 0; i < v.size(); ++i) {
			unsigned x = v[i] / 16;
			s[2*i] = (x > 9) ? ('A' + (x-10u)) : ('0' + x);
			x = v[i] % 16;
			s[2*i+1] = (x > 9) ? ('A' + (x-10u)) : ('0' + x);
		}
		return s;
	}


	void DB::fetchSources( std::map<unsigned,Source> & sources )
	{
		sources.clear();
		std::string const sqlCmd = "SELECT srcId,srcName,url,paramTypes,guiSrcName,guiLabel,guiUrl FROM sources ORDER BY srcId";
		fConn->execute(sqlCmd);
		while (fConn->fetchRow()) {
			unsigned srcId;
			fConn->parseField(0, srcId);
			std::string paramTypes;
			sources[srcId].srcId = srcId;
			fConn->parseField(1, sources[srcId].srcName);
			fConn->parseField(2, sources[srcId].url);
			fConn->parseField(3, paramTypes);
			fConn->parseField(4, sources[srcId].guiSrcName);
			fConn->parseField(5, sources[srcId].guiLabel);
			fConn->parseField(6, sources[srcId].guiUrl);
			ASSERT(paramTypes.size() < 4);
			strncpy(sources[srcId].paramTypes, paramTypes.data(), paramTypes.size());
			sources[srcId].paramTypes[paramTypes.size()] = '\0';
		}
	}


	void DB::fetchSource(std::string const & srcName, Source & src)
	{
		std::string sqlCmd = "SELECT srcId,srcName,url,paramTypes,guiSrcName,guiLabel,guiUrl FROM sources";
		sqlCmd.append(" WHERE srcName='");
		sqlCmd.append(srcName);
		sqlCmd.append("'");
		fConn->execute(sqlCmd);
		if (! fConn->fetchRow()) {
			throw ExceptionExternalSourceDoesNotExists();
		}
		std::string paramTypes;
		fConn->parseField(0, src.srcId);
		fConn->parseField(1, src.srcName);
		fConn->parseField(2, src.url);
		fConn->parseField(3, paramTypes);
		fConn->parseField(4, src.guiSrcName);
		fConn->parseField(5, src.guiLabel);
		fConn->parseField(6, src.guiUrl);
		ASSERT(paramTypes.size() < 4);
		strncpy(src.paramTypes, paramTypes.data(), paramTypes.size());
		src.paramTypes[paramTypes.size()] = '\0';
	}


	void DB::fetchSourcesUsers(std::map<unsigned,std::vector<std::string>> & users)
	{
		users.clear();
		std::string const sqlCmd = "SELECT srcId,userName FROM users ORDER BY srcId,userName";
		fConn->execute(sqlCmd);
		while (fConn->fetchRow()) {
			unsigned srcId;
			fConn->parseField(0, srcId);
			std::string user;
			fConn->parseField(1, user);
			users[srcId].push_back(user);
		}
	}


	void DB::insertSource( Source & src )
	{
		std::string sqlCmd = "SELECT srcId,srcName FROM sources ORDER BY srcId";
		fConn->execute(sqlCmd);
		unsigned freeId = 1;
		while (fConn->fetchRow()) {
			unsigned srcId;
			fConn->parseField(0, srcId);
			std::string name;
			fConn->parseField(1, name);
			if (srcId == freeId) ++freeId;
			if (name == src.srcName) {
				throw std::runtime_error("External source with the name '" + name + "' already exists!");
			}
		}
		if (freeId >= 250) {
			throw std::runtime_error("Maximum number of external source was reached!");
		}
		src.srcId = freeId;
		// insert
		sqlCmd = "INSERT INTO sources(srcId,srcName,url,paramTypes,guiSrcName,guiLabel,guiUrl) VALUES(";
		sqlCmd.append(std::to_string(src.srcId));
		sqlCmd.append(",'");
		sqlCmd.append(src.srcName);
		sqlCmd.append("','");
		sqlCmd.append(fConn->escapeString(src.url));
		sqlCmd.append("','");
		sqlCmd.append(src.paramTypes);
		sqlCmd.append("','");
		sqlCmd.append(fConn->escapeString(src.guiSrcName));
		sqlCmd.append("','");
		sqlCmd.append(fConn->escapeString(src.guiLabel));
		sqlCmd.append("','");
		sqlCmd.append(fConn->escapeString(src.guiUrl));
		sqlCmd.append("')");
		fConn->execute(sqlCmd);
	}


	void DB::updateSource( Source & src )
	{
		std::string sqlCmd = "UPDATE sources SET url='";
		sqlCmd.append(fConn->escapeString(src.url));
		sqlCmd.append("',guiSrcName='");
		sqlCmd.append(fConn->escapeString(src.guiSrcName));
		sqlCmd.append("',guiLabel='");
		sqlCmd.append(fConn->escapeString(src.guiLabel));
		sqlCmd.append("',guiUrl='");
		sqlCmd.append(fConn->escapeString(src.guiUrl));
		sqlCmd.append("' WHERE srcId=");
		sqlCmd.append(std::to_string(src.srcId));
		fConn->execute(sqlCmd);
	}


	void DB::deleteSource( unsigned srcId )
	{
		std::string const sqlCmd = "DELETE FROM sources WHERE srcId=" + std::to_string(srcId);
		fConn->execute(sqlCmd);
	}


	void DB::assignUserToSource( unsigned srcId, std::string const & user )
	{
		std::string sqlCmd = "INSERT IGNORE INTO users(srcId,userName) VALUES(" + std::to_string(srcId);
		sqlCmd.append(",'");
		sqlCmd.append(fConn->escapeString(user));
		sqlCmd.append("')");
		fConn->execute(sqlCmd);
	}


	void DB::removeUsersFromSource( unsigned srcId, std::vector<std::string> const users )
	{
		std::string sqlCmd = "DELETE FROM users WHERE srcId=" + std::to_string(srcId);
		if (! users.empty()) {
			sqlCmd.append(" AND userName IN (");
			for (std::string const & user: users) {
				sqlCmd.append( "'" + fConn->escapeString(user) + "',");
			}
			sqlCmd.back() = ')';
		}
		fConn->execute(sqlCmd);
	}


	bool DB::isAssignedToSource( std::string const & user, unsigned srcId )
	{
		std::string const sqlCmd = "SELECT 1 FROM users WHERE srcId=" + std::to_string(srcId) + " AND userName='" + user +"'";
		fConn->execute(sqlCmd);
		return (fConn->fetchRow());
	}


	void DB::fetchLinksByIds
		( std::vector<std::vector<std::pair<unsigned,std::vector<uint8_t>>>> & linkParams  // output, element=vectorOf(source,params)
		, std::vector<unsigned> const & ids     // cannot be empty, identifiers of records to fetch
		, std::vector<unsigned> const & srcIds  // list of sources; if empty, records for all sources are fetched
		)
	{
		ASSERT(! ids.empty());
		linkParams.resize(ids.size());

		// ====== sort by id
		std::vector<unsigned> sortTransform;
		std::vector<unsigned> idsSorted;
		buildSortTransformation(ids, sortTransform);
		applyTransformation(ids, sortTransform, idsSorted);
		std::vector<std::vector<std::pair<unsigned,std::vector<uint8_t>>>> linkParamsSorted(linkParams.size());

		// ====== fetch data
		unsigned currentIdsIndex = 0;
		std::string const sqlSuffix = " ORDER BY extId,srcId,params";
		for ( auto iId = idsSorted.begin();  iId != idsSorted.end(); ) {
			// --- build first part of the SQL cmd
			std::string sqlCmd = "SELECT extId,srcId,params FROM links WHERE";
			if (! srcIds.empty()) {
				sqlCmd.append(" srcId IN (");
				for (unsigned si: srcIds) {
					sqlCmd.append( std::to_string(si) + "," );
				}
				sqlCmd.back() = ')';
				sqlCmd.append(" AND");
			}
			// --- build second part of the SQL cmd until maxLength is reached
			sqlCmd.append(" extId IN (");
			for ( ;  iId != idsSorted.end() && sqlCmd.size() + sqlSuffix.size() < 4*1024*1024-1024;  ++iId ) { // 4MB - max query size
				sqlCmd.append( std::to_string(*iId) + "," );
			}
			sqlCmd.back() = ')';
			sqlCmd.append(sqlSuffix);
			// --- run SQL cmd and save results
			fConn->execute(sqlCmd);
			while (fConn->fetchRow()) {
				unsigned extId;
				std::pair<unsigned,std::vector<uint8_t>> p;
				fConn->parseField(0, extId);
				fConn->parseField(1, p.first);
				fConn->parseField(2, p.second);
				while ( idsSorted[currentIdsIndex] != extId ) {
					++currentIdsIndex;
					ASSERT(currentIdsIndex < idsSorted.size());
				}
				linkParamsSorted[currentIdsIndex].push_back(p);
			}
		}

		// ====== correction for duplicated records
		for ( unsigned i = 1;  i < linkParamsSorted.size();  ++i ) {
			if (idsSorted[i] == idsSorted[i-1]) {
				linkParamsSorted[i] = linkParamsSorted[i-1];
			}
		}

		// ====== restore the original order
		std::vector<unsigned> unsortTransform;
		buildReverseTransformation(sortTransform, unsortTransform);
		applyTransformation(linkParamsSorted, unsortTransform, linkParams);
	}


	void DB::fetchLinksIdsBySourceAndParams
		( std::vector<unsigned> & ids           // output, list of fetched records' ids, there are no repetitive elements
		, unsigned const srcId                  // srcId
		, std::vector<uint8_t> const & param	// values of parameters, if empty all links from the source are matched
		)
	{
		ids.clear();

		// --- build SQL cmd
		std::string sqlCmd = "SELECT DISTINCT(extId) FROM links WHERE srcId=" + std::to_string(srcId);
		if (! param.empty()) {
			sqlCmd.append(" AND params=x'" + toHex(param) + "'");
		}
		sqlCmd.append(" ORDER BY extId");

		// --- run SQL cmd and save results
		fConn->execute(sqlCmd);
		while (fConn->fetchRow()) {
			unsigned extId;
			fConn->parseField(0, extId);
			ids.push_back(extId);
		}
	}


	void DB::insertLinks
		( std::vector<std::vector<std::pair<unsigned,std::vector<uint8_t>>>> const & linkParams  // input, element=vectorOf(source,params)
		, std::vector<unsigned> const & ids     // input, corresponding identifiers of records to modify, the size must be = to linkParams
		)
	{
		ASSERT(ids.size() == linkParams.size());

		// ====== sort by id
		std::vector<unsigned> sortTransform;
		std::vector<unsigned> idsSorted;
		std::vector<std::vector<std::pair<unsigned,std::vector<uint8_t>>>> linkParamsSorted;
		buildSortTransformation(ids, sortTransform);
		applyTransformation(ids, sortTransform, idsSorted);
		applyTransformation(linkParams, sortTransform, linkParamsSorted);

		// ====== insert data
		for ( unsigned iRow = 0;  iRow < idsSorted.size();  ) {
			// --- build first part of the SQL cmd
			std::string const sqlCmd1 = "INSERT IGNORE INTO links(extId,srcId,params) VALUES ";
			// --- build second part of the SQL cmd until maxLength is reached
			std::string sqlCmd2 = "";
			while ( sqlCmd1.size() + sqlCmd2.size() < 4*1024*1024-1024 ) { // 4MB - max query size
				while ( iRow < idsSorted.size() && linkParamsSorted[iRow].empty() ) ++iRow;
				if ( iRow == idsSorted.size() ) break;
				sqlCmd2.append( ",(" + std::to_string(idsSorted[iRow]) );
				sqlCmd2.append( "," + std::to_string(linkParamsSorted[iRow].back().first) );
				sqlCmd2.append( ",x'" + toHex(linkParamsSorted[iRow].back().second) + "')" );
				linkParamsSorted[iRow].pop_back();
			}
			if (sqlCmd2.empty()) break;
			sqlCmd2.front() = ' ';
			// --- run SQL cmd and save results
			fConn->execute( sqlCmd1 + sqlCmd2 );
		}
	}


	void DB::deleteLinksByIds
		( std::vector<unsigned> const & ids     // input, cannot be empty, corresponding identifiers of records to modify, the size must be = to linkParams
		, unsigned srcId                        // source id
		, std::vector<std::vector<uint8_t>> const & linkParams
		)
	{
		ASSERT(! ids.empty());
		ASSERT(ids.size() == linkParams.size());
		// ---- build SQL query
		std::string sqlCmd = "DELETE FROM links WHERE srcId=" + std::to_string(srcId) + " AND (";
		for (unsigned i = 0; i < ids.size(); ++i) {
			if (i != 0) sqlCmd.append(" OR ");
			sqlCmd.append("(extId=" + std::to_string(ids[i]) + " AND params=x'" + toHex(linkParams[i]) + "')");
		}
		sqlCmd.append(")");
		// --- run SQL cmd and save results
		fConn->execute( sqlCmd );
	}


	void DB::deleteLinksByIds
		( std::vector<unsigned> const & ids     // input: cannot be empty, corresponding identifiers of records to modify
		, unsigned srcId                        // source id
		)
	{
		ASSERT(! ids.empty());
		// ---- build SQL query
		std::string sqlCmd = "DELETE FROM links WHERE srcId=" + std::to_string(srcId) + " AND extId IN (";
		for (unsigned i = 0; i < ids.size(); ++i) {
			if (i != 0) sqlCmd.append(",");
			sqlCmd.append(std::to_string(ids[i]));
		}
		sqlCmd.append(")");
		// --- run SQL cmd and save results
		fConn->execute( sqlCmd );
	}


	void DB::deleteLinksByIds
		( std::vector<unsigned> const & ids     // input: cannot be empty, corresponding identifiers of records to modify
		)
	{
		ASSERT(! ids.empty());
		// ---- build SQL query
		std::string sqlCmd = "DELETE FROM links WHERE extId IN (";
		for (unsigned i = 0; i < ids.size(); ++i) {
			if (i != 0) sqlCmd.append(",");
			sqlCmd.append(std::to_string(ids[i]));
		}
		sqlCmd.append(")");
		// --- run SQL cmd and save results
		fConn->execute( sqlCmd );
	}


	void DB::deleteLinksBySource
		( std::vector<unsigned> const & srcIds  // input: list of source, whose links will be removed; if empty all links are removed
		)
	{
		// ---- build SQL query
		std::string sqlCmd = "DELETE FROM links";
		if (! srcIds.empty()) {
			sqlCmd.append(" WHERE srcId IN (");
			for (unsigned i: srcIds) {
				sqlCmd.append(std::to_string(i) + ",");
			}
			sqlCmd.back() = ')';
		}
		// --- run SQL cmd and save results
		fConn->execute( sqlCmd );
	}

}
