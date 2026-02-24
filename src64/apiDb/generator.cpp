#include <fstream>
#include <iostream>
#include <cstdlib>
#include <random>
#include <cstdint>
#include <limits>

std::default_random_engine generator;
std::uniform_int_distribution<uint32_t> distribution(0, std::numeric_limits<uint32_t>::max());

int main(int argc, char ** argv)
{

	if (argc != 4) {
		std::cerr << "Parameters: first_key  key_increase  number_of_records" << std::endl;
		return 1;
	}

	unsigned firstKey = atoi(argv[1]);
	unsigned keyStep  = atoi(argv[2]);
	unsigned numberOfRecords = atoi(argv[3]);

	for (unsigned key = firstKey; numberOfRecords > 0; --numberOfRecords) {
		uint32_t x = distribution(generator);
		uint32_t y = distribution(generator);
		std::cout << key << "\t" << x << "\t" << y << "\n";
		key += keyStep;
	}

	return 0;
}
