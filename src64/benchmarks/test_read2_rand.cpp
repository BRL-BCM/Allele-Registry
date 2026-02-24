#include "db_api.hpp"
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include "../commonTools/Stopwatch.hpp"




int main(int argc, char ** argv)
{
	try {

		unsigned const chunkSize = 1024*1024;
		unsigned recordsCount = 0;

		std::vector<uint32_t> keys(1000*1000*1000);
		for (unsigned i = 0; i < keys.size(); ++i) keys[i] = i;
		std::random_shuffle( keys.begin(), keys.end() );

		Stopwatch timer;
		{
			DB db;
			while (recordsCount < 1000*1000*1000) {
				std::vector<Record> values(chunkSize);
				for (auto & kv: values) {
					kv.key = keys[++recordsCount];
					kv.data1 = static_cast<uint64_t>(kv.key) * 1234567890 + 1234567890;
					kv.data2 = ~(kv.data1);
				}
				db.getAndValidate(values);
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
