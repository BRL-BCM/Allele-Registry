#include "db_api.hpp"
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include "../Stopwatch.hpp"

int main(int argc, char ** argv)
{
	try {

		unsigned const chunkSize = 16*1024;
		unsigned recordsCount = 0;

		std::vector<uint32_t> keys(1000*1000*1000 + chunkSize);
		for (unsigned i = 0; i < keys.size(); ++i) keys[i] = i;

		Stopwatch timer;
		{
			DB db;
			while (recordsCount < 1000*1000*1000) {
				std::vector<uint32_t> value(3);
				for (unsigned i = 0; i < chunkSize; ++i) {
					value[0] = recordsCount % (1234567890 + i);
					value[1] = recordsCount % (234567890 + i);
					value[2] = recordsCount % (34567890 + i);
					db.set(keys[++recordsCount], value);
				}
				unsigned const currentTime = timer.save_and_restart_ms();
				std::cout << recordsCount << "\t" << currentTime << "\t" << timer.get_summary_time_sec() << std::endl;
			}
		}
		unsigned const currentTime = timer.save_and_restart_ms();
		std::cout << "ClosingDb\t" << currentTime << "\t" << timer.get_summary_time_sec() << std::endl;

	} catch (std::exception const & e) {
		std::cerr << "EXCEPTION: " << e.what() << std::endl;
		return -1;
	}

	return 0;
}
