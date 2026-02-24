#ifndef SHARED_MEMORY_CONSUMER_HPP
#define SHARED_MEMORY_CONSUMER_HPP

#include <string>
#include <iostream>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/thread/mutex.hpp>

#include "info.hpp"
#include "messageBuffer.hpp"
#include <list>

using namespace boost::interprocess;

class sharedMemoryConsumer {
	message_queue * mq;
	boost::mutex m;
	std::list<std::pair<bool, uint64_t> > ids;
public:
	sharedMemoryConsumer() {
		//message_queue::remove("mq");
		try {
			mq = new message_queue
			(
				open_or_create,
				"kafka_message_queue",
				MAX_MSG,
				MAX_SIZE
			);
		}
		catch(interprocess_exception &ex)
		{
			std::cerr << ex.what() << std::endl;
		}
		std::thread * worker = new std::thread(&sharedMemoryConsumer::workerThread, this);
		worker->detach();
	}
	void workerThread() {
		int iter = 0;
		while(true) {
			iter++;
			m.lock();
			int size = ids.size();
			m.unlock();
			if (size < 1) {
				usleep(100000);
			} else {
				int size = 1;
				while(size > 0) {
					try {
						m.lock();
						size = ids.size();
						if(size > 0) {
							sharedDataEntry me;
							while(ids.empty() == false && me.size() < 1000) {
								auto x = ids.front();
								me.add(x.first, x.second);
								ids.pop_front();
							}
							std::stringstream oss;
							boost::archive::text_oarchive oa(oss);
							oa << me;
							std::string serialized_string(oss.str());
							if(mq->try_send(serialized_string.data(), serialized_string.size(), 0)) {
								// good job
								me.clear();
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
		}
	}
	void send(std::vector<std::pair<bool, uint64_t> > const & _ids) {
		m.lock();
		for(auto x : _ids) {
			ids.push_back(x);
		}
		m.unlock();
	}
};

#endif
