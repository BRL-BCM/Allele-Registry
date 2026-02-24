#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <boost/interprocess/ipc/message_queue.hpp>

boost::interprocess::message_queue * mq = NULL;
std::mutex * m = new std::mutex();
uint64_t running_id = 0;

uint64_t newCaId() {
	if(mq == NULL) {
		return -1;
	}
	uint64_t ret;
	char dat[8];
	unsigned int priority;
	boost::interprocess::message_queue::size_type recvd_size;
	try {
		bool rcf_ret = mq->try_receive(&dat[0], 8, recvd_size, priority);
		if(rcf_ret) {
			ret = *(reinterpret_cast<uint64_t*>(&dat[0]));
			return ret;
		} else {
			return -1;
		}
	} catch(boost::interprocess::interprocess_exception &ex) {
		std::cerr << "MESSAGE QUEUE ERROR:\t" << ex.what() << std::endl;
		return -1;
	}
}
int N_THREADS = 1;
int SLEEP_US = 1000;
int PRINTOUTS = 1;

int main(int argc,char ** argv) {
	if(argc != 4) {
		std::cout << "try ./test <n_threads> <sleep_us> <print_outs 0/1>" << std::endl;
		std::cout << "ex: ./test 8 1000000 1; run on 8 threads, wait 1000000 us = 1 sec between id requests, 1/0=printouts on/off" << std::endl;
		exit(1);
	}
	N_THREADS = atoi(argv[1]);
	SLEEP_US = atoi(argv[2]);
	PRINTOUTS = atoi(argv[3]);
	try {
		mq = new boost::interprocess::message_queue
		(
			boost::interprocess::open_only,
			"ca_id_queue"
		);
	} catch (boost::interprocess::interprocess_exception &ex) {
		std::cerr << "MESSAGE QUEUE ERROR:\t" << ex.what() << std::endl;
	}

	std::vector<std::thread> workers;
	for(int t=0;t<N_THREADS;t++) {
		workers.push_back(std::thread([t]()
		{
			while(true) {
				usleep(SLEEP_US);
				m->lock();
				uint64_t id = newCaId();
				if(PRINTOUTS) {
					std::cout << "thread:" << t << '\t' << "size:" << mq->get_num_msg() << '\t' << "id:" << id << std::endl;
				} else if(id%1000==0) {
					std::cout << "thread:" << t << '\t' << "size:" << mq->get_num_msg() << '\t' << "id:" << id << std::endl;
				}
				if(id == uint64_t(-1)) {
					std::cout << "EMPTY QUEUE! continuing ..." << std::endl;
					m->unlock();
					continue;
				}
				if(id <= running_id) {
					std::cout << "DUPLICATES!" << std::endl;
					exit(1);
				}
				running_id = id;
				m->unlock();
			}
		}));
	}
	for(int t=0;t<N_THREADS;t++) {
		workers[t].join();
	}

	return 0;
}
