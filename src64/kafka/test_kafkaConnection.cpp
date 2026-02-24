#include <iostream>
#include "kafkaConnection.hpp"

int main() {
	std::cout << "test kafka connection" << std::endl;
	kafkaProducer * producer = new kafkaProducer();
	producer->init("user", "pwd", "cert", "test_topic", "localhost", 9092);
	{
		std::string payload_str("hello world");
		size_t payload_len = payload_str.length();
		char * payload = new char[payload_len];
		strcpy(payload, payload_str.c_str());
		std::string key_str("key");
		const char * key = key_str.c_str();
		size_t key_len = key_str.length();
		producer->asyncWrite(payload, payload_len, key, key_len);
	}
	{
		std::string payload_str("hello world 2");
		size_t payload_len = payload_str.length();
		char * payload = new char[payload_len];
		strcpy(payload, payload_str.c_str());
		std::string key_str("key 2");
		const char * key = key_str.c_str();
		size_t key_len = key_str.length();
		producer->asyncWrite(payload, payload_len, key, key_len);
	}
	//kafkaConsumer * consumer = new kafkaConsumer();
	//consumer->init("test-group", "localhost", 9092);
	//consumer->asyncRead("test_topic");
	return 0;
}
