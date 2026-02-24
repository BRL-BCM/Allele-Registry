#include "TableGenomic64.hpp"
#include "TableProtein64.hpp"
#include "IndexIdentifierCa.hpp"
#include "IndexIdentifierPa.hpp"
#include "IndexIdentifierUInt32.hpp"
#include "IndexIdentifierUInt64.hpp"
#include "../apiDb/TasksManager.hpp"
#include "../flatDb/Convert.hpp"
#include <iostream>
#include <csignal>
#include "../commonTools/parseHeader.h"
#include "bloom.h"

void sighandler(int s)
{
	std::cout << "Caught signal " << s << ".\n"; // this is undefined behaviour
	std::cout << "try 'touch stop'" << std::endl;
}

std::vector<HashBuffer*> buf;

int main(int argc, char ** argv)
{
	unsigned skip = 0;
	std::signal(SIGINT, sighandler);

	if(argc != 4) {
		std::cout << "try ./convertIndexIdentifiers <start> <increment> <dbAlleles-directory>" << std::endl;
		exit(1);
	}

	uint64_t start = atoll(argv[1]);
	uint64_t increment = atoll(argv[2]);
	std::string bloomPath = argv[4];

	for(int i=0;i<10;i++) {
std::cout << "start - creating bloom filter" << std::endl;
		std::stringstream ss;
		ss << bloomPath << "bloom-" << i << ".dat";
		buf.push_back(new HashBuffer(ss.str(), 536870912));
std::cout << "done - creating bloom filter" << std::endl;
	}

	std::string directory = argv[3];
	TasksManager taskManager(32);
	TasksManager taskManager2(32);
	std::atomic<uint64_t> nextCaId((uint64_t)0);
	TableGenomic * tabGenomic = new TableGenomic(directory, parse(directory+"/"+"genomic.yaml"), &taskManager, &taskManager2, 32, nextCaId);
	uint64_t skipG = start;
	uint64_t incG = 16L*1024*1024;
	uint64_t maxG = skipG+increment;
	uint64_t skipP = 0;
	uint64_t incP = 16L*1024*1024;
	uint64_t maxP = std::numeric_limits<uint64_t>::max();
	auto visitor = [&skipG,&incG,&maxG](std::vector<RecordGenomicVariant*> & varRecords, bool lastCall)
	{
		std::cout << "G progress:" << skipG << "\t" << maxG << std::endl;
		for(auto r:varRecords) {
			uint64_t buf_id = -1;
			uint64_t val_id = -1;
			if(r->identifiers.lastId() < 4294967296L) {
				buf_id = 0;
				val_id = r->identifiers.lastId()%4294967296L;
			} else {
				buf_id = (r->identifiers.lastId()/10)/4294967296L;
				val_id = (r->identifiers.lastId()/10)%4294967296L;
			}
//std::cout << buf_id << '\t' << val_id << '\t' << r->identifiers.lastId() << std::endl;
			if(buf[buf_id]->get(val_id) == false) {
				buf[buf_id]->set(val_id);
			} else {
				std::cout << "CA" << r->identifiers.lastId() << std::endl;
			}
		}
		skipG+=incG;
	};
	tabGenomic->query(visitor, skip, skipG, maxG, incG);
	for(int i=0;i<buf.size();i++) {
		std::cout << "start - save bloom filter" << std::endl;
		buf[i]->save();
		std::cout << "done - save bloom filter" << std::endl;
	}
	return 0;
}
