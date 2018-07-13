#include "externalSources.hpp"
#include <iostream>

int main(int argc, char ** argv)
{

	MySqlConnectionParameters connParams;
	connParams.hostOrSocket = "/usr/local/brl/local/var/mysql.sock";
	connParams.dbName = "externalSources";
	connParams.user = "externalSources";
	connParams.password = "externalSources";

	externalSources::init(connParams);

	std::vector<std::string> output;
	jsonBuilder::DocumentSettings docConf; docConf.layout = jsonBuilder::DocumentSettings::standard;

	std::vector<uint32_t> ids;
	for (unsigned i = 0; i < 100; ++i) ids.push_back(i);
	std::vector<std::vector<std::string>> params(ids.size(),{"test2"});

	externalSources::registerLinks
		( ids		// vector of identifiers of variants to modify
		, "dwa"		// name of external source name
		, params	// parameters of links to register, size of the vector must be the same as ids vector
		);



	ids.resize(4);
	ids[0] = 11;
	ids[1] = 22;
	ids[2] = 33;
	ids[3] = 22;
	externalSources::queryLinksByIds
		( output		// output(overwritten): vector of output JSON documents
		, docConf	    // document settings (set of fields to show)
		, true          // pretty JSON or compressed JSON
		, ids		// set of variants identifiers to fetch
		);


	externalSources::queryLinksBySource
		( ids			// output(overwritten): variants identifiers for documents returned in output parameter
		, "dwa"		// name of external source
		, params[0]
		);

	for (auto & s: output) std::cout << s << "\n===================================\n";

	return 0;
}
