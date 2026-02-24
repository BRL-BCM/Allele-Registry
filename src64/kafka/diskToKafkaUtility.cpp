#include "diskToKafkaUtility.hpp"

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

void diskToKafkaUtility::run(std::string base_path, std::string username, std::string password, std::string certificate, std::string topic, std::string host, unsigned port, unsigned batch_sleep, unsigned error_sleep) {
	try {
		mb = new MessageBuffer(base_path);
	}
	catch(std::exception &ex)
	{
		std::cerr << ex.what() << std::endl;
		exit(1);
	}
	try {
		producer = new kafkaProducer();
		producer->init(username.c_str(), password.c_str(), certificate.c_str(), topic.c_str(), host.c_str(), port);
	} catch (std::exception & e) {
		std::cout << "failed to connect to kafka broker:" << topic << "\t" << host << ":" << port << std::endl;
		std::cout << e.what() << std::endl;
		exit(1);
	}
	try
	{
		MessageBuffer * mb = new MessageBuffer(base_path);
		message_queue::size_type recvd_size;
		unsigned int priority;
		unsigned int key = 0;
		while(true) {
			usleep(1000*batch_sleep);
			key++;
			std::stringstream payload_ss;
			std::stringstream id_ss;
			id_ss << key;
			bool init = true;
			unsigned rcv_count = 0;
			std::string fullfile;
			try {
				sharedDataEntry me;
				std::stringstream iss;
				std::string serialized_string;
				serialized_string.resize(MAX_SIZE);
				bool rcv_ret = mb->try_receive(&serialized_string[0], MAX_SIZE, recvd_size, priority, fullfile);
				if(rcv_ret == true) {
					rcv_count++;
					payload_ss << serialized_string.substr(0, recvd_size);
					//std::cout << payload_ss.str() << std::endl;
					std::cout << "payload size:" << payload_ss.str().size() << std::endl;
					init=false;
				} else {
					usleep(1000*error_sleep);
				}
			}
			catch(interprocess_exception &ex)
			{
				std::cerr << "interprocess exception: " << ex.what() << std::endl;
				usleep(1000*error_sleep);
			}
			catch(std::exception &ex) {
				std::cerr << "exception: " << ex.what() << std::endl;
				usleep(1000*error_sleep);
			}
			if(rcv_count>0) {
				bool success = false;
				while(true) {
					std::cout << currentDateTime() << " - " << fullfile << std::endl;
					std::cout << "send to kafka" << std::endl;
					try {
						std::string payload_str = payload_ss.str();
						size_t payload_len = payload_str.length();
						char payload_ptr[payload_len] = { 0 };
						strcpy(payload_ptr, payload_str.c_str());
						std::string key_str = id_ss.str();
						const char * key = key_str.c_str();
						size_t key_len = key_str.length();
						success = producer->asyncWrite(payload_ptr, payload_len, key, key_len);
					} catch (std::exception & e) {
						std::cerr << e.what() << std::endl;
					}
					if(success == true) {
						break;
					} else {
						std::cout << "going to sleep for " << error_sleep << "ms" << std::endl;
						usleep(1000*error_sleep);
					}
				}
			}
		}
	}
	catch(std::exception &ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}


