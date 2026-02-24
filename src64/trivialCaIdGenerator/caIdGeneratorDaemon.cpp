#include "caIdGeneratorUtility.hpp"

struct ConfigurationU {
	std::string directory;
	std::string logFile_path;
	std::string latestCaId_path;
	unsigned logDelay;
};
ConfigurationU configuration;
std::mutex * m = new std::mutex();
uint64_t * latest_caid = new uint64_t[1];

void sighandler(int s)
{
	std::cout << "Caught signal " << s << ".\n"; // this is undefined behaviour
	std::cout << "acquiring lock on I/O" << std::endl;
	m->lock();
	if(*latest_caid < uint64_t(-1)) {
		std::cout << "Latest ca id: " << *latest_caid << std::endl;
		std::cout << "Saving to disk: " << configuration.latestCaId_path << std::endl;
		std::ofstream myfile;
		myfile.open (configuration.latestCaId_path);
		myfile << *latest_caid;
		myfile.close();
	} else {
		std::cout << "no new ids were pushed to queue" << std::endl;
	}
	std::cout << "terminating ..." << std::endl;
	exit(0);
}

int main(int argc, char** argv) {
	*latest_caid = -1;
	std::cout << "Starting CA Id Generator Daemon ..." << std::endl;
	if(argc != 2) {
		std::cout << "try: ./caIdGeneratorDaemon ./caIdGeneratorDaemon.yaml" << std::endl;
		exit(1);
	}
	std::signal(SIGINT, sighandler);
	std::signal(SIGTERM, sighandler);
	std::signal(SIGKILL, sighandler);
	std::string confFile(argv[1]);
	std::cout << "Parsing config file: " << confFile << std::endl;
	try { 
		json::Value conf = readConfigurationFile(confFile);
		extractField(conf, configuration.directory       , {"generator"   , "dbAlleles"   } );
		extractField(conf, configuration.logFile_path    , {"logFile"     , "path"        } );
		extractField(conf, configuration.logDelay        , {"logFile"     , "delay"       } );
		extractField(conf, configuration.latestCaId_path , {"latestCaId"  , "path"        } );
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
	generator.run(configuration.directory, configuration.latestCaId_path, latest_caid, m, configuration.logDelay);
	return 0;
}

