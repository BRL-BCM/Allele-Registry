#include <iostream>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <mutex>
#include <thread>
#include <vector>

int main() {
	boost::interprocess::message_queue * mq = NULL;
	try {
		mq = new boost::interprocess::message_queue
		(
			boost::interprocess::open_only,
			"ca_id_queue"
		);
	} catch (boost::interprocess::interprocess_exception &ex) {
		std::cerr << ex.what() << std::endl;
	}
	std::mutex * m = new std::mutex();
	std::vector<std::thread> worker;
	for(int i=0;i<32;i++) worker.push_back(std::thread([m,mq](int j){
		char * dat = new char[sizeof(uint64_t)];
		while(true) {
			//usleep(rand()%100000);
			unsigned int priority;
			boost::interprocess::message_queue::size_type recvd_size;
			bool rcv_ret = mq->try_receive(&dat[0], sizeof(uint64_t), recvd_size, priority);
			uint64_t ret = *(reinterpret_cast<uint64_t*>(&dat[0]));
			uint64_t ret_parity = ret/10;
			int sum = 0;
			while(ret_parity > 0) {
				sum += (ret_parity%10);
				ret_parity /= 10;
			}
			int parity = sum%10;
			m->lock();
			std::cout << j << '\t' << ret << '\t' << parity << std::endl;
			m->unlock();
		}
	},i));
	for(int i=0;i<worker.size();i++){
		worker[i].join();
	}
	return 0;
}
