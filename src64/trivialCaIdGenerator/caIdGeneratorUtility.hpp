#ifndef CA_ID_GENERATOR_UTILITY_HPP
#define CA_ID_GENERATOR_UTILITY_HPP

#include <iostream>
#include <random>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <yaml-cpp/yaml.h>
#include "../commonTools/json.hpp"
#include "../commonTools/assert.hpp"
#include "../allelesDatabase/IndexIdentifierCa.hpp"
#include "yamlUtil.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <mutex>
#include <fstream>
#include <csignal>
#include <chrono>

class caIdGeneratorUtility {
public:
	void run(std::string directory, std::string id_path, uint64_t * _id, std::mutex * m, unsigned logDelay);

};

#endif
