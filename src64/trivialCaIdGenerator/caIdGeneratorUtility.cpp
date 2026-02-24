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

void caIdGeneratorUtility::run(std::string directory, std::string id_path, uint64_t * _id, std::mutex * m, unsigned logDelay) {
	std::cout << "CA Id Generator Daemon Starting ..." << std::endl;
	TasksManager taskManager(32);
	TasksManager taskManager2(32);
	json::Value json = readConfigurationFile(directory+"/idCa.yaml");
	Header header;
	extractField(json, header.keySize, {"keySize"} );
	extractField(json, header.pageSize, {"pageSize"} );
	extractField(json, header.indexNodePages, {"indexNodePages"} );
	extractField(json, header.revision, {"revision"} );
	std::cout << "conf:" << '\t' << "keySize:" << header.keySize << std::endl;
	std::cout << "conf:" << '\t' << "pageSize:" << header.pageSize << std::endl;
	std::cout << "conf:" << '\t' << "indexNodePages:" << header.indexNodePages << std::endl;
	std::cout << "conf:" << '\t' << "revision:" << header.revision << std::endl;
	uint64_t nextCaId = 0;
	try {
		IndexIdentifierCa * indexIdentifierCa = 
			new IndexIdentifierCa(
				directory, 
				header, 
				&taskManager, 
				&taskManager2, 32);
		nextCaId = indexIdentifierCa->getMaxIdentifier();
		delete indexIdentifierCa;
	} catch (...) {}
	uint64_t dat = -1;
	m->lock();
	std::ifstream myfile;
	myfile.open (id_path);
	myfile >> dat;
	myfile.close();
	m->unlock();
	if(dat < uint64_t(-1)) {
		if(dat > nextCaId) {
			nextCaId = dat;
		}
	}
	//uint64_t nextCaId = 0;
	unsigned MAX_SIZE = sizeof(uint64_t);
	unsigned MAX_MSG = 100000000;
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
	uint64_t count = nextCaId;
	std::cout << "initial id:\t" << count << std::endl;
	uint64_t * D = new uint64_t[1];
	auto curr_num_msg = mq->get_num_msg();
	auto prev_num_msg = curr_num_msg;
	auto now = std::chrono::system_clock::now();
	while(true) {
		curr_num_msg = mq->get_num_msg();
		//if(curr_num_msg != prev_num_msg) 
		auto duration = now.time_since_epoch();
		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
		auto now1 = std::chrono::system_clock::now();
		auto duration1 = now1.time_since_epoch();
		auto milliseconds1 = std::chrono::duration_cast<std::chrono::milliseconds>(duration1).count();
		if(milliseconds1-milliseconds > logDelay)
		{
			now = now1;
			std::cout << currentDateTime() << " - queue size: " << curr_num_msg << std::endl;
		}
		prev_num_msg = curr_num_msg;
		if(mq->get_num_msg() < 0.5*MAX_MSG) {
			std::cout << currentDateTime() << " - generating new messages - start" << std::endl;
			m->lock();
			std::ofstream myfile;
			myfile.open (id_path);
			myfile << uint64_t(count+0.5*MAX_MSG);
			myfile.close();
			m->unlock();
			for(long i=0;i<0.5*MAX_MSG;i++) {
				m->lock();
				count++;
				*_id = count;
				*D = count;
				try {
					if(mq->timed_send((char*)reinterpret_cast<char*>(&D[0]), sizeof(uint64_t), 0, boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time()) + boost::posix_time::milliseconds(1))) {
						// message sent
						//if(i%1000000 == 0) std::cout << i << '\t' << count << std::endl;
					}
				} catch(boost::interprocess::interprocess_exception &ex) {
					std::cerr << "MESSAGE QUEUE ERROR:\t" << ex.what() << std::endl;
					m->unlock();
					continue;
				}
				m->unlock();
			}
			std::cout << currentDateTime() << " - generating new messages - done" << std::endl;
		}
		sleep(1);
	}
}

