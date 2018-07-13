#ifndef EXTERNALSOURCES_DB_HPP_
#define EXTERNALSOURCES_DB_HPP_

#include "../mysql/mysqlConnection.hpp"
#include "core.hpp"
#include <vector>
#include <map>



namespace externalSources
{

	class DB
	{
	private:
		MySqlConnection::SP fConn;
	public:
		DB(MySqlConnection::SP conn) : fConn(conn) {}

		void fetchSources
			( std::map<unsigned,Source> & sources	// output, it is overwritten
			);

		void fetchSource(std::string const & srcName, Source & src);

		void insertSource( Source & src ); // input, srcID is set in this method

		void updateSource( Source & src );

		void deleteSource( unsigned srcId );

		void fetchSourcesUsers
			( std::map<unsigned,std::vector<std::string>> & users // output, it is overwritten, srcId->users
			);

		void assignUserToSource( unsigned srcId, std::string const & user );

		void removeUsersFromSource( unsigned srcId, std::vector<std::string> const users = std::vector<std::string>() ); // if users is empty then all users from the source are removed

		bool isAssignedToSource( std::string const & user, unsigned srcId );

		void fetchLinksByIds
			( std::vector<std::vector<std::pair<unsigned,std::vector<uint8_t>>>> & linkParams  // output, element=vectorOf(source,params)
			, std::vector<unsigned> const & ids     // cannot be empty, identifiers of records to fetch
			, std::vector<unsigned> const & srcIds  // list of sources; if empty, records for all sources are fetched
			);

		void fetchLinksIdsBySourceAndParams
			( std::vector<unsigned> & ids           // output, list of fetched records' ids, there are no repetitive elements
			, unsigned const srcId                  // srcId
			, std::vector<uint8_t> const & param	// values of parameters, if empty all links from the source are matched
			);

		void insertLinks
			( std::vector<std::vector<std::pair<unsigned,std::vector<uint8_t>>>> const & linkParams  // input, element=vectorOf(source,params)
			, std::vector<unsigned> const & ids     // input, corresponding identifiers of records to modify, the size must be = to linkParams
			);

		void deleteLinksByIds
			( std::vector<unsigned> const & ids     // input, cannot be empty, corresponding identifiers of records to modify, the size must be = to linkParams
			, unsigned srcId                        // source id
			, std::vector<std::vector<uint8_t>> const & linkParams  // input, list of params to remove, each element (paramsValues) correspond to single id from vector ids
			);

		void deleteLinksByIds
			( std::vector<unsigned> const & ids     // input: cannot be empty, corresponding identifiers of records to modify
			, unsigned srcId                        // source id
			);

		void deleteLinksByIds
			( std::vector<unsigned> const & ids     // input: cannot be empty, corresponding identifiers of records to modify
			);

		void deleteLinksBySource
			( std::vector<unsigned> const & srcIds  // input: list of source, whose links will be removed; if empty all links are removed
			);

	};

}


#endif /* EXTERNALSOURCES_DB_HPP_ */
