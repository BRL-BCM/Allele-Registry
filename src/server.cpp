#include <iostream>
#include <string>
#include <chrono>
#include <boost/lexical_cast.hpp>

#include <yaml-cpp/yaml.h>
#include "commonTools/json.hpp"
#include "commonTools/assert.hpp"


namespace {

	void convert(YAML::Node const & src, json::Value & trg)
	{
		if (src.IsMap()) {
			for ( auto it = src.begin();  it != src.end();  ++it ) {
				convert( it->second, trg[it->first.as<std::string>()] );
			}
		} else if (src.IsSequence()) {
			for ( unsigned i = 0;  i < src.size();  ++i ) {
				json::Value newVal;
				convert( src[i], newVal );
				trg.push_back(newVal);
			}
		} else if (src.IsScalar()) {
//			std::string const s = src.Scalar();
//			unsigned i = 0;
//			while ( i < s.size() && s[i] >= '0' && s[i] <= '9' ) ++i;
//			if ( i > 0 && i == s.size()) {
//				trg = src.as<json::Integer>();
//			} else {
//				trg = src;
//			}
			trg = src.Scalar();
		} else if (src.IsNull()) {
			trg = trg.null;
		}
	}

	json::Value readConfigurationFile(std::string const & path)
	{
		YAML::Node config;
		try {
			config = YAML::LoadFile(path);
		} catch (std::exception const & e) {
			throw std::runtime_error("Cannot read configuration file " + path + ": " + e.what());
		}
		json::Value data;
		convert(config, data);
		return data;
	}

	void extractField( json::Value & node, std::string & target, std::vector<std::string> path )
	{
		ASSERT(! path.empty());
		std::vector<json::Value*> nodes;
		nodes.push_back(&node);
		std::string fullPath = "";
		for (std::string const & s: path) {
			if (! fullPath.empty()) fullPath.append(".");
			if ( nodes.back()->type() != json::ValueType::object ) {
				throw std::runtime_error("The element at the location \"" + fullPath + "\" is not an object");
			}
			fullPath.append(s);
			if ( nodes.back()->asObject().count(s) == 0 ) {
				throw std::runtime_error("The location \"" + fullPath + "\" does not exist");
			}
			nodes.push_back( &(nodes.back()->asObject()[s]) );
		}
		if ( nodes.back()->type() != json::ValueType::string ) {
			throw std::runtime_error("The element at the location \"" + fullPath + "\" is not a string");
		}
		target = nodes.back()->asString();
		nodes.pop_back();
		while (! nodes.empty()) {
			nodes.back()->asObject().erase(path.back());
			if (! nodes.back()->asObject().empty()) break;
			nodes.pop_back();
			path.pop_back();
		}
	}

	void extractField( json::Value & node, unsigned & target, std::vector<std::string> path )
	{
		std::string s;
		extractField( node, s, path );
		unsigned i = 0;
		while ( i < s.size() && s[i] >= '0' && s[i] <= '9' ) ++i;
		if ( i > 0 && i == s.size()) {
			target = std::stoi(s);
		} else {
			std::string fullPath = path.front();
			for ( unsigned i = 1; i < path.size(); ++i ) {
				fullPath.append(".");
				fullPath.append(path[i]);
			}
			throw std::runtime_error("The element at the location \"" + fullPath + "\" is not an unsigned integer");
		}
	}

	void extractField( json::Value & node, std::vector<std::string> & target, std::vector<std::string> path )
	{
		ASSERT(! path.empty());
		std::vector<json::Value*> nodes;
		nodes.push_back(&node);
		std::string fullPath = "";
		for (std::string const & s: path) {
			if (! fullPath.empty()) fullPath.append(".");
			if ( nodes.back()->type() != json::ValueType::object ) {
				throw std::runtime_error("The element at the location \"" + fullPath + "\" is not an object");
			}
			fullPath.append(s);
			if ( nodes.back()->asObject().count(s) == 0 ) {
				throw std::runtime_error("The location \"" + fullPath + "\" does not exist");
			}
			nodes.push_back( &(nodes.back()->asObject()[s]) );
		}
		target.clear();
		if ( nodes.back()->type() != json::ValueType::null ) {
			if ( nodes.back()->type() != json::ValueType::array ) {
				throw std::runtime_error("The element at the location \"" + fullPath + "\" is not an array");
			}
			json::Array & arr = nodes.back()->asArray();
			for ( auto & n: arr ) target.push_back(n.asString());
		}
		nodes.pop_back();
		while (! nodes.empty()) {
			nodes.back()->asObject().erase(path.back());
			if (! nodes.back()->asObject().empty()) break;
			nodes.pop_back();
			path.pop_back();
		}
	}
}

#include "httpp/HttpServer.hpp"
#include "httpp/http/Utils.hpp"
#include "httpp/utils/Exception.hpp"

using HTTPP::HttpServer;
using HTTPP::HTTP::Request;
using HTTPP::HTTP::Connection;
using HTTPP::HTTP::HttpCode;

#include "dispatcher.hpp"

std::shared_ptr<Dispatcher> dispatcher;


void handler(Connection* connection, Request&& request)
{
	try {
		auto headers = request.getSortedHeaders();
		auto const& content_length = headers["Content-Length"];

		unsigned long const maxSize = 4ul*1024*1024*1024 - 1; // 4 gigabytes - 1 byte
		unsigned long size = 0;
		if ( ! content_length.empty() ) {
			if (content_length.size() > 10) {
				size = maxSize+1;
			} else {
				size = boost::lexical_cast<unsigned long>(content_length);
			}
		}

		connection->response().addHeader("X-CAR-Version", alleleRegistryVersion);

		if (size == 0) {
			// no body
			HttpCode httpStatus;
			std::string contentType = "";
			std::function<std::string()> callbackNextBodyChunk;
			std::string redirected = "";
			dispatcher->processRequest( request.fullUri, request.method, request.uri, request.query_params, nullptr, httpStatus, contentType, callbackNextBodyChunk, redirected );
			if (redirected == "") {
				if (contentType != "") connection->response().addHeader("Content-Type", contentType);
				connection->response().setCode(httpStatus).setBody( std::move(callbackNextBodyChunk) );
			} else {
				connection->response().setCode(httpStatus).addHeader("Location", redirected);
			}
			HTTPP::HTTP::setShouldConnectionBeClosed(request, connection->response());
			connection->sendResponse(); // connection pointer may become invalid
		} else if (size <= maxSize) {
			// standard body
			std::shared_ptr<std::vector<char>> ptrBody(new std::vector<char>());
			ptrBody->reserve(size);

			auto callbackReadBodyChunk = [=]( Request const & request, Connection * connection
					, boost::system::error_code const & ec, char const * buffer, size_t n)->void
					{
						if (ec == boost::asio::error::eof) {
							HttpCode httpStatus;
							std::string contentType = "";
							std::function<std::string()> callbackNextBodyChunk;
							std::string redirected = "";
							dispatcher->processRequest( request.fullUri, request.method, request.uri, request.query_params, ptrBody, httpStatus, contentType, callbackNextBodyChunk, redirected );
							if (redirected == "") {
								if (contentType != "") connection->response().addHeader("Content-Type", contentType);
								connection->response().setCode(httpStatus).setBody( std::move(callbackNextBodyChunk) );
							} else {
								connection->response().setCode(httpStatus).addHeader("Location", redirected);
							}
							HTTPP::HTTP::setShouldConnectionBeClosed(request, connection->response());
							connection->sendResponse(); // connection pointer may become invalid
						} else if (ec) {
							throw HTTPP::UTILS::convert_boost_ec_to_std_ec(ec);
						} else {
							ptrBody->insert( ptrBody->end(), buffer, buffer+n );
						}
					};

			connection->readBody( size, std::bind( callbackReadBodyChunk, request, connection
												 , std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
												 ) );
		} else {
			// body too long
			std::string const body = "{ \"errorType\": \"RequestTooLarge\"\n, \"description\": \"The body is too long. It must be smaller than 4 gigabytes.\"\n}";
			connection->response().setCode(HttpCode::BadRequest).setBody( body );
			HTTPP::HTTP::setShouldConnectionBeClosed(request, connection->response());
			connection->sendResponse(); // connection pointer may become invalid
		}
	} catch (std::exception const & e) {
		std::cerr << "EXCEPTION when processing request: " << e.what() << std::endl;
	}
}

int main(int argc, char** argv)
{
	std::cout << "Allele Registry, version " << alleleRegistryVersion << std::endl;

	Configuration configuration;
	std::string confFile = "";

	bool paramError = false;
	for (unsigned i = 1; i < argc; ++i) {
		std::string param = argv[i];
		if (param.substr(0,1) == "-") {
			if (param == "--no_auth") configuration.noAuthentication = true;
			else if (param == "--read_only") configuration.readOnly = true;
			else {
				paramError = true;
				break;
			}
		} else if (confFile != "") {
			paramError = true;
			break;
		} else {
			confFile = param;
		}
	}
	if (confFile == "") paramError = true;

	if (paramError) {
		std::cout << "Incorrect command line parameters. Required parameters are: path_to_configuration_file [--no_auth] [--read_only]\n" << std::endl;
		std::cout << "Exit!" << std::endl;
		return 1;
	}

	std::string server_interface;
	std::string server_port;
	unsigned server_threads;

	try {
		json::Value conf = readConfigurationFile(confFile);

		extractField(conf, configuration.alleleRegistryFQDN, {"alleleRegistryFQDN"} );
		extractField(conf, server_interface                , {"server","interface"} );
		extractField(conf, server_port                     , {"server","port"     } );
		extractField(conf, server_threads                  , {"server","threads"  } );
		extractField(conf, configuration.referencesDatabase_path               , {"referencesDatabase", "path"} );
		extractField(conf, configuration.allelesDatabase_path                  , {"allelesDatabase", "path"   } );
		extractField(conf, configuration.allelesDatabase_threads               , {"allelesDatabase", "threads"} );
		extractField(conf, configuration.allelesDatabase_ioTasks               , {"allelesDatabase", "ioTasks"} );
		extractField(conf, configuration.allelesDatabase_cache_genomic         , {"allelesDatabase", "cache", "genomic"} );
		extractField(conf, configuration.allelesDatabase_cache_protein         , {"allelesDatabase", "cache", "protein"} );
		extractField(conf, configuration.allelesDatabase_cache_sequence        , {"allelesDatabase", "cache", "sequence"} );
		extractField(conf, configuration.allelesDatabase_cache_idCa            , {"allelesDatabase", "cache", "idCa"} );
		extractField(conf, configuration.allelesDatabase_cache_idClinVarAllele , {"allelesDatabase", "cache", "idClinVarAllele"} );
		extractField(conf, configuration.allelesDatabase_cache_idClinVarRCV    , {"allelesDatabase", "cache", "idClinVarRCV"} );
		extractField(conf, configuration.allelesDatabase_cache_idClinVarVariant, {"allelesDatabase", "cache", "idClinVarVariant"} );
		extractField(conf, configuration.allelesDatabase_cache_idDbSnp         , {"allelesDatabase", "cache", "idDbSnp"} );
		extractField(conf, configuration.allelesDatabase_cache_idPa            , {"allelesDatabase", "cache", "idPa"} );

		extractField(conf, configuration.logFile_path              , {"logFile", "path"} );

		extractField(conf, configuration.genboree_db.hostOrSocket  , {"genboree", "mysql", "hostOrSocket"} );
		extractField(conf, configuration.genboree_db.port          , {"genboree", "mysql", "port"        } );
		extractField(conf, configuration.genboree_db.user          , {"genboree", "mysql", "user"        } );
		extractField(conf, configuration.genboree_db.password      , {"genboree", "mysql", "password"    } );
		extractField(conf, configuration.genboree_db.dbName        , {"genboree", "mysql", "dbName"      } );

		extractField(conf, configuration.externalSources_db.hostOrSocket, {"externalSources", "mysql", "hostOrSocket"} );
		extractField(conf, configuration.externalSources_db.port        , {"externalSources", "mysql", "port"        } );
		extractField(conf, configuration.externalSources_db.user        , {"externalSources", "mysql", "user"        } );
		extractField(conf, configuration.externalSources_db.password    , {"externalSources", "mysql", "password"    } );
		extractField(conf, configuration.externalSources_db.dbName      , {"externalSources", "mysql", "dbName"      } );

		std::vector<std::string> superusers;
		extractField(conf, superusers, {"superusers"} );
		for (auto const & s: superusers) configuration.superusers.insert(s);

		std::string path;
		extractField(conf, path, {"genboree", "settings", "path"} );

		if (! conf.asObject().empty()) {
			throw std::runtime_error("Unknown fields: " + conf.toString());
		}

		if (! configuration.noAuthentication) {
			YAML::Node config2 = YAML::LoadFile(path);
			std::cout << "Parse genboree configuration file..." << std::endl;
			configuration.genboree_allowedHostnames = config2["allowedHostnames"].as<std::vector<std::string>>();
			std::cout << "OK" << std::endl;
		}
	} catch (std::exception const & e) {
		std::cerr << "EXCEPTION when reading configuration file " << confFile << ": " << e.what() << std::endl;
		return 2;
	}
//std::cout << "host/socket: " << configuration.genboree_db.hostOrSocket << std::endl;
//std::cout << "port:        " << configuration.genboree_db.port << std::endl;
//std::cout << "user:        " << configuration.genboree_db.user << std::endl;
//std::cout << "password:    " << configuration.genboree_db.password << std::endl;
//std::cout << "db name:     " << configuration.genboree_db.dbName << std::endl;
	// ============ start daemon

	boost::asio::io_service  io_service;

    // Register signal handlers so that the daemon may be shut down. You may
    // also want to register for other signals, such as SIGHUP to trigger a
    // re-read of a configuration file.
    boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
    signals.async_wait(boost::bind(&boost::asio::io_service::stop, &io_service));

    // Inform the io_service that we are about to become a daemon. The
    // io_service cleans up any internal resources, such as threads, that may
    // interfere with forking.
    io_service.notify_fork(boost::asio::io_service::fork_prepare);

    // Fork the process and have the parent exit. If the process was started
    // from a shell, this returns control to the user. Forking a new process is
    // also a prerequisite for the subsequent call to setsid().
    if (pid_t pid = fork()) {
    	if (pid > 0) {
    		// We're in the parent process and need to exit.
    		// When the exit() function is used, the program terminates without
    		// invoking local variables' destructors. Only global variables are
    		// destroyed. As the io_service object is a local variable, this means
    		// we do not have to call:
            //   io_service.notify_fork(boost::asio::io_service::fork_parent);
    		// However, this line should be added before each call to exit() if
    		// using a global io_service object.
    		exit(0);
    	} else {
    		std::cerr << "First fork failed. Exit!" << std::endl;
    		return -1;
    	}
    }

    // Make the process a new session leader. This detaches it from the terminal.
    setsid();

    // A second fork ensures the process cannot acquire a controlling terminal.
    if (pid_t pid = fork()) {
    	if (pid > 0) {
    		exit(0);
    	} else {
    		std::cerr << "Second fork failed. Exit!" << std::endl;
    		return -2;
    	}
    }

	// ============ close standard streams and redirect all output to the file

    // Close the standard streams and open corresponding files.
    // open()/dup() always chooses the lowest available fd.

    close(0);
    if ( open("/dev/null", O_RDONLY) < 0 ) {
    	std::cerr << "Cannot open /dev/null. Exit!" << std::endl;
    	return -3;
    }
    close(1);
    if ( open(configuration.logFile_path.c_str(), O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) < 0 ) {
    	std::cerr << "Cannot open log file: " << configuration.logFile_path << ". Exit!" << std::endl;
    	return -4;
    }
    close(2);
    if ( dup(1) < 0 ) {
    	std::cout << "Cannot redirect STDERR to descriptor 1. Exit!" << std::endl;
    	return -5;
    }

    // ============ finalize the daemon and start HTTP server

    try {
    	// create Dispatcher
    	std::cout << "Load references database... " << std::flush;
    	dispatcher.reset( new Dispatcher(configuration) );
		std::cout << "OK" << std::endl;

		// Inform the io_service that we have finished becoming a daemon. The
		// io_service uses this opportunity to create any internal file descriptors
		// that need to be private to the new process.
		io_service.notify_fork(boost::asio::io_service::fork_child);

		HTTPP::UTILS::ThreadPool threadPool(server_threads, "allele_registry_worker");
		HTTPP::HttpServer server(threadPool);
		std::cout << "Start HTTP server... " << std::flush;
		server.start();
		server.setSink(&handler);
		server.bind(server_interface, server_port);
		std::cout << "OK" << std::endl;
		io_service.run();
		std::cout << "Stop HTTP server... " << std::flush;
		server.stop();
		std::cout << "OK" << std::endl;
    } catch (std::exception const & e) {
    	std::cerr << "EXCEPTION: " << e.what() << std::endl;
    	io_service.stop();
    	return 2;
    }

    return 0;
}
