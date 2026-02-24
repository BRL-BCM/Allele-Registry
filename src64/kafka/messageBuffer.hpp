#ifndef MESSAGE_BUFFER_HPP
#define MESSAGE_BUFFER_HPP

#include <iostream>
#include <boost/filesystem.hpp>

struct MessageBuffer {
	std::size_t count;
	std::string base_path;
	std::string current_path;
	std::string ready_for_kafka_path;
	std::string consumed_by_kafka_path;
	MessageBuffer(std::string _base_path) {
		base_path = _base_path;
		current_path = base_path + "/current";
		boost::filesystem::create_directory(current_path);
		ready_for_kafka_path = base_path + "/ready";
		boost::filesystem::create_directory(ready_for_kafka_path);
		consumed_by_kafka_path = base_path + "/history";
		boost::filesystem::create_directory(consumed_by_kafka_path);
		count = 0;
	}
	const std::string currentDateTime() {
		time_t     now = time(0);
		struct tm  tstruct;
		char       buf[80];
		tstruct = *localtime(&now);
		strftime(buf, sizeof(buf), "%Y-%m-%d", &tstruct);
		return buf;
	}
	bool try_send(const char * dat, size_t size, int priority) {
		std::stringstream filess;
		pid_t pid = getpid();
		filess << "message-" << pid << "-" << count;
		std::string file = filess.str();
		std::stringstream filenamess;
		filenamess << current_path << "/" << file;
		std::string filename = filenamess.str();
		int flags = O_RDWR | O_CREAT;
		int f = open(filename.c_str(), flags, S_IRUSR | S_IWUSR);
		if(f < 0) {
			throw std::runtime_error("cannot open file: " + filename);
		}
		ssize_t const res = pwrite(f, dat, size, 0);
		if(res < 0) {
			close(f);
			throw std::runtime_error("cannot write file: " + filename);
		}
		close(f);
		boost::filesystem::rename(filename, ready_for_kafka_path + "/" + file);
		count++;
		return true;
	}
	bool try_receive(char * dat, size_t size, size_t & recvd_size, int priority, std::string & p_fullfile) {
		bool ret = false;
		std::string fullfile;
		std::string file;
		for(auto & p : boost::filesystem::directory_iterator(ready_for_kafka_path)) {
			ret = true;
			fullfile = p.path().string();
			p_fullfile = fullfile;
			file = p.path().filename().string();
			break;
		}
		if(ret) {
			int flags = O_RDWR;
			int f = open(fullfile.c_str(), flags, S_IRUSR | S_IWUSR);
			if(f < 0) {
				throw std::runtime_error("cannot open file: " + fullfile);
			}
			ssize_t const res = pread(f, dat, size, 0);
			if(res < 0) {
				close(f);
				throw std::runtime_error("cannot read file: " + fullfile);
			}
			recvd_size = res;
			std::cout << "size:" << size << "\t" << recvd_size << std::endl;
			dat[recvd_size] = '\0';
			close(f);
			std::string Date = currentDateTime();
			boost::filesystem::create_directory(consumed_by_kafka_path + "/" + Date);
			boost::filesystem::rename(ready_for_kafka_path + "/" + file, consumed_by_kafka_path + "/" + Date + "/" + file);
		}
		return ret;
	}
};

#endif
