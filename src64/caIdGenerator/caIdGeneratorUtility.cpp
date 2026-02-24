#include "caIdGeneratorUtility.hpp"

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

void caIdGeneratorUtility::run(std::string bloom_path, std::string batch_info, unsigned batch_sleep, std::mutex * m) {
	std::cout << "CA Id Generator Daemon Starting ..." << std::endl;
	unsigned MAX_SIZE = sizeof(uint64_t);
	unsigned step = 100;
	float bloomStep = 1.0/10.0;
	long num = 4294967296/(step);
	unsigned MAX_MSG = num*3;
	boost::interprocess::message_queue * mq = NULL;
	try {
		std::cout << "create mq" << std::endl;
		mq = new boost::interprocess::message_queue
		(
			boost::interprocess::open_or_create,
			"ca_id_queue",
			MAX_MSG,
			MAX_SIZE
		);
	}
	catch(boost::interprocess::interprocess_exception &ex)
	{
		std::cerr << ex.what() << std::endl;
	}
	std::cout << "mq ready ..." << std::endl;
	uint64_t count = 0;
	uint64_t * D = new uint64_t[1];
	std::mt19937 generator(time(0));
	unsigned _bloomId = 0;
	std::cout << "Parsing batch info:" << batch_info << std::endl;
	try {
		json::Value conf = readConfigurationFile(batch_info);
		extractField(conf, _bloomId, { "batch", "epoch" } );
	} catch (std::exception const & e) {
		//std::cerr << "error while reading " << batch_info << ": " << e.what() << std::endl;
	}
	float bloomId = _bloomId;
	while(true) {
		std::cout << currentDateTime() << " - size:" << mq->get_num_msg() << std::endl;
		if(mq->get_num_msg() < 0.4*MAX_MSG) {
			std::cout << "generating new messages" << std::endl;
			std::stringstream ss;
			ss << bloom_path << "/bloom-" << (int)bloomId << ".dat";
			HashBuffer buf(ss.str(), 536870912);
			std::vector<uint64_t> newIds;
			for(long i=0;i<4294967296;i+=step) {
				//if(i%100000000==0)std::cout << i << std::endl;
				bool cont = true;
				while(cont) {
					const uint32_t R = generator();
					const uint64_t Z = R+4294967296L*((int)bloomId+1L);
					int parity = 0;
					for(uint64_t k=Z; k > 0; k/=10) {
						int d = k%10;
						parity += d;
					}
					if(buf.get(R) == false) {
						buf.set(R);
						*D = Z*10L + (parity%10);
						newIds.push_back(*D);
						cont = false;
					}
				}
			}
			m->lock();
			buf.save();
			bloomId+=bloomStep;
			std::ofstream batchFile;
			batchFile.open(batch_info.c_str());
			batchFile << "batch:" << std::endl;
			batchFile << "  epoch: " << (int)bloomId << std::endl;
			batchFile.close();
			m->unlock();
			for(auto id : newIds) {
				try {
					*D = id;
					if(mq->try_send((char*)reinterpret_cast<char*>(&D[0]), sizeof(uint64_t), 0)) {
						// message sent
					}
					count++;
				} catch(boost::interprocess::interprocess_exception &ex) {
					std::cerr << "MESSAGE QUEUE ERROR:\t" << ex.what() << std::endl;
					continue;
				}
			}
		}
		sleep(batch_sleep);
	}
}

