#include "sharedMemoryToDiskUtility.hpp"

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime() {
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
	// for more information about date/time format
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
	return buf;
}

void sharedMemoryToDiskUtility::run(std::string base_path, unsigned batch_sleep, unsigned error_sleep, unsigned message_size, unsigned flush_timeout) {
	try {
		mb = new MessageBuffer(base_path);
		mq = new message_queue
		(
			open_only,
			"kafka_message_queue"
		);
	}
	catch(std::exception &ex)
	{
		std::cerr << ex.what() << std::endl;
		exit(1);
	}
	std::cout << "sharedMemoryToDiskDaemon started successfully - listening..." << std::endl;
	sharedDataEntry me_total;
	std::clock_t start = std::clock();
	while(true) {
		try {
			m.lock();
			{
				usleep(100000);
				std::stringstream iss;
				std::string serialized_string2;
				serialized_string2.resize(MAX_SIZE);
				unsigned int priority;
				message_queue::size_type recvd_size;
				bool rcv_ret = mq->try_receive(&serialized_string2[0], MAX_SIZE, recvd_size, priority);
				if(rcv_ret == true) {
					sharedDataEntry me;
					iss << serialized_string2;
					boost::archive::text_iarchive ia(iss);
					ia >> me;
					me_total.add(me);
				}
				double duration = 1000000.0 * ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
				if(10*me_total.size() > message_size || duration > flush_timeout) {
					start = std::clock();
					std::cout << currentDateTime() << std::endl;
					if(me_total.size() > 0) {
						std::cout << "flush " << me_total.size() << " ids" << std::endl;
						std::stringstream oss;
						bool init = true;
						oss << "[";
						while(true) {
							auto const & d = me_total.front();
							if(init==false) oss << ",";
							oss << "\"" << ((d.getType())?"CA":"PA") << d.getId() << "\"";
							init=false;
							me_total.pop_front();
							if(oss.str().size() > message_size) break;
							if(me_total.empty()) break;
						}
						oss << "]";
						std::string serialized_string(oss.str());
						if(mb->try_send(serialized_string.data(), serialized_string.size(), 0)) {
							// good job!
						}
					}
				}
			}
			m.unlock();
		}
		catch(std::exception const &ex)
		{
			m.unlock();
			std::cerr << "Shared memory consumer exception: " << ex.what() << std::endl;
		}
	}
}

