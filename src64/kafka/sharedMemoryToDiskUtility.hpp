#ifndef SHARED_MEMORY_TO_DISK_DAEMON_HPP
#define SHARED_MEMORY_TO_DISK_DAEMON_HPP

#include <string>
#include <iostream>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <thread>
#include <boost/thread/mutex.hpp>

#include "kafkaConnection.hpp"

#include "messageBuffer.hpp"

#include "info.hpp"

using namespace boost::interprocess;

class sharedMemoryToDiskUtility {
	MessageBuffer * mb;
	message_queue * mq;
	boost::mutex m;
	kafkaProducer * producer = nullptr;
public:
	void run(std::string base_path, unsigned batch_sleep, unsigned error_sleep, unsigned message_size, unsigned flush_timeout);

};

#endif
