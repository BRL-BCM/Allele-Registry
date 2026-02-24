#include "diskToKafkaUtility.hpp"
#include <yaml-cpp/yaml.h>
#include "../commonTools/json.hpp"
#include "../commonTools/assert.hpp"

struct Configuration {
	std::string base_path;
	std::string logFile_path;
	std::string username;
	std::string password;
	std::string certificate;
	std::string kafka_topic;
	std::string kafka_host;
	unsigned kafka_port;
	unsigned batch_sleep;
	unsigned error_sleep;
};

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

int main(int argc, char** argv) {
	std::cout << "Starting Kafka Connection Daemon ..." << std::endl;
	if(argc != 2) {
		std::cout << "try: ./kafkaConnectionDaemon ./kafkaConnectionDaemon.yaml" << std::endl;
		exit(1);
	}
	Configuration configuration;
	std::string confFile(argv[1]);
	std::cout << "Parsing config file: " << confFile << std::endl;
	try { 
		json::Value conf = readConfigurationFile(confFile);
		extractField(conf, configuration.base_path     , {"basePath"    , "path"        } );
		extractField(conf, configuration.logFile_path  , {"logFile"     , "path"        } );
		extractField(conf, configuration.kafka_topic   , {"kafka"       , "topic"       } );
		extractField(conf, configuration.kafka_host    , {"kafka"       , "host"        } );
		extractField(conf, configuration.kafka_port    , {"kafka"       , "port"        } );
		extractField(conf, configuration.username      , {"kafka"       , "username"    } );
		extractField(conf, configuration.password      , {"kafka"       , "password"    } );
		extractField(conf, configuration.certificate   , {"kafka"       , "certificate" } );
		extractField(conf, configuration.batch_sleep   , {"performance" , "batchSleep"  } );
		extractField(conf, configuration.error_sleep   , {"performance" , "errorSleep"  } );
	} catch (std::exception const & e) {
		std::cerr << "EXCEPTION when reading configuration file " << confFile << ": " << e.what() << std::endl;
		exit(2);
	}
	std::cout << "Subscribing to Kafka Broker: " << configuration.kafka_host << ":" << configuration.kafka_port << "\tTopic: " << configuration.kafka_topic << std::endl;
	std::cout << "Log output: " << configuration.logFile_path << std::endl;
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
	diskToKafkaUtility q;
	q.run( configuration.base_path
	     , configuration.username
	     , configuration.password
	     , configuration.certificate
	     , configuration.kafka_topic
	     , configuration.kafka_host
	     , configuration.kafka_port
	     , configuration.batch_sleep
	     , configuration.error_sleep
	     );
	return 0;
}
