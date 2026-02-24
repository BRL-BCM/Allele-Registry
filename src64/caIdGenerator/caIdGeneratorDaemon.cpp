#include "caIdGeneratorUtility.hpp"

struct Configuration {
	std::string logFile_path;
	std::string bloom_path;
	std::string batch_info;
	unsigned batch_sleep;
};
Configuration configuration;
std::mutex * m = new std::mutex();

void sighandler(int s)
{
	std::cout << "Caught signal " << s << ".\n"; // this is undefined behaviour
	std::cout << "acquiring lock on I/O" << std::endl;
	m->lock(); // make sure to acquire the lock, wait until bloom filter save is finished
	std::cout << "lock acquired, terminating ..." << std::endl;
	exit(0);
}

int main(int argc, char** argv) {
	std::cout << "Starting CA Id Generator Daemon ..." << std::endl;
	if(argc != 2) {
		std::cout << "try: ./caIdGeneratorDaemon ./caIdGeneratorDaemon.yaml" << std::endl;
		exit(1);
	}
	std::string confFile(argv[1]);
	std::cout << "Parsing config file: " << confFile << std::endl;
	try { 
		json::Value conf = readConfigurationFile(confFile);
		extractField(conf, configuration.logFile_path  , {"logFile"     , "path"         } );
		extractField(conf, configuration.bloom_path    , {"generator"   , "bloomPath"    } );
		extractField(conf, configuration.batch_info    , {"generator"   , "batchInfo"    } );
		extractField(conf, configuration.batch_sleep   , {"generator"   , "batchSleep"   } );
	} catch (std::exception const & e) {
		std::cerr << "EXCEPTION when reading configuration file " << confFile << ": " << e.what() << std::endl;
		exit(2);
	}
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
	caIdGeneratorUtility generator;
	generator.run(configuration.bloom_path, configuration.batch_info, configuration.batch_sleep, m);
	return 0;
}

