#include "allelesDatabase.hpp"
#include "canonicalization.hpp"
#include <yaml-cpp/yaml.h>
#include <boost/test/minimal.hpp>
#include <memory>

int test_main(int argc, char ** argv)
{
	if (argc != 2) {
		std::cout << "Parameter: path to configuration file\n" << std::endl;
		std::cout << "Exit!" << std::endl;
		return 1;
	}

	std::string server_interface;
	std::string server_port;
	Configuration configuration;
	try {
		YAML::Node config = YAML::LoadFile(argv[1]);
		std::cout << "Parse configuration file..." << std::endl;
		std::cout << "  -> server.interface..." << std::endl;
		server_interface                      = config["server"           ]["interface"].as<std::string>();
		std::cout << "  -> server.port..." << std::endl;
		server_port                           = config["server"           ]["port"    ].as<std::string>();
		std::cout << "  -> referenceDatabase.path..." << std::endl;
		configuration.referenceDatabase_path  = config["referenceDatabase"]["path"    ].as<std::string>();
		if (! config["alleleDatabase"]["socket"].IsDefined()) {
			std::cout << "  -> alleleDatabase.host..." << std::endl;
			configuration.alleleDatabase_hostOrSocket = config["alleleDatabase"   ]["host"    ].as<std::string>();
			std::cout << "  -> alleleDatabase.port..." << std::endl;
			configuration.alleleDatabase_port     = config["alleleDatabase"   ]["port"    ].as<unsigned   >(); // must be larger than 0 and <= 65535
		} else {
			std::cout << "  -> alleleDatabase.socket..." << std::endl;
			configuration.alleleDatabase_hostOrSocket = config["alleleDatabase"]["socket"].as<std::string>();
		}
		std::cout << "  -> alleleDatabase.user..." << std::endl;
		configuration.alleleDatabase_user     = config["alleleDatabase"   ]["user"    ].as<std::string>();
		std::cout << "  -> alleleDatabase.password..." << std::endl;
		configuration.alleleDatabase_password = config["alleleDatabase"   ]["password"].as<std::string>();
		std::cout << "  -> alleleDatabase.dbName..." << std::endl;
		configuration.alleleDatabase_dbName   = config["alleleDatabase"   ]["dbName"  ].as<std::string>();
		std::cout << "  -> logFile.path..." << std::endl;
		configuration.logFile_path            = config["logFile"          ]["path"    ].as<std::string>();
		std::cout << "OK" << std::endl;
	} catch (std::exception const & e) {
		std::cerr << "EXCEPTION when reading configuration file: " << e.what() << std::endl;
		return 2;
	}

	std::unique_ptr<ReferencesDatabase> refDb(new ReferencesDatabase(configuration.referenceDatabase_path));

	for (unsigned i = 0; i < 1000; ++i) {
		std::cout << "====================== Iteration " << i << std::endl;
		std::cout << "Create AllelesDatabase object ... " << std::flush;
		AllelesDatabase * db = new AllelesDatabase(refDb.get(),configuration);
		std::cout << "OK" << std::endl;
		std::cout << "Delete AllelesDatabase object ... " << std::flush;
		delete db;
		std::cout << "OK" << std::endl;
	}


//	std::unique_ptr<ReferencesDatabase> refDb(new ReferencesDatabase("./referencesDatabase"));
//	std::unique_ptr<AllelesDatabase> db(new AllelesDatabase(refDb.get()));
//
//	std::vector<SimpleVariant> defs;
//	SimpleVariant sv;
//	// CA000123
//	sv.refId = refDb->getReferenceId("NC_000017.11");
//	sv.modifications = { SimpleSequenceModification(7676039,7676040,"C","T") };
//	defs.push_back(sv);
//	// CA001234
//	sv.refId = refDb->getReferenceId("NC_000017.11");
//	sv.modifications = { SimpleSequenceModification(7673586,7673587,"G","A") };
//	defs.push_back(sv);
//	// CA123456
//	sv.refId = refDb->getReferenceId("NC_000002.12");
//	sv.modifications = { SimpleSequenceModification(113235495,113235496,"A","G") };
//	defs.push_back(sv);
//	// CA028311
//	sv.refId = refDb->getReferenceId("NC_000009.12");
//	sv.modifications = { SimpleSequenceModification(132906881,132906881,"","CGCTCTCCAGAGAAACTGAGTAGTCAAACACACTGAGAGCCGTTTTCAA") };
//	defs.push_back(sv);
//
//	std::vector<CanonicalVariant> norm_defs;
//	std::vector<unsigned> carCaIds;
//
//	for (SimpleVariant const & sv: defs) norm_defs.push_back( canonicalize(refDb.get(),sv) );
//
//	carCaIds = db->fetchAlleles_definition_arCaId(norm_defs);
//
//	std::cout << carCaIds.at(3) << std::endl;
//
//	BOOST_CHECK( carCaIds.size() == norm_defs.size() );
//	BOOST_CHECK( carCaIds.at(0) ==    123 );
//	BOOST_CHECK( carCaIds.at(1) ==   1234 );
//	BOOST_CHECK( carCaIds.at(2) == 123456 );
//	BOOST_CHECK( carCaIds.at(3) ==  28311 );

	return 0;
}
