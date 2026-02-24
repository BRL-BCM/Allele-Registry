#ifndef DISK_TO_KAFKA_UTILITY_HPP
#define DISK_TO_KAFKA_UTILITY_HPP

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

class diskToKafkaUtility {
	MessageBuffer * mb;
	message_queue * mq;
	boost::mutex m;
	kafkaProducer * producer = nullptr;
	unsigned messageSize;
public:
	void run(std::string base_path, std::string username, std::string password, std::string certificate, std::string topic, std::string host, unsigned port, unsigned batch_sleep, unsigned error_sleep);

};

#endif
