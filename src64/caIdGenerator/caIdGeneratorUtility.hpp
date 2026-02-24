#ifndef CA_ID_GENERATOR_UTILITY_HPP
#define CA_ID_GENERATOR_UTILITY_HPP

#include <iostream>
#include <random>
#include "bloom.h"
#include <boost/interprocess/ipc/message_queue.hpp>
#include <yaml-cpp/yaml.h>
#include "../commonTools/json.hpp"
#include "../commonTools/assert.hpp"
#include "yamlUtil.hpp"

class caIdGeneratorUtility {
public:
	void run(std::string bloom_path, std::string batch_info, unsigned batch_sleep, std::mutex * m);

};

#endif
