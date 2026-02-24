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
#include "tools.hpp"
#include "../requests/hgvs.hpp"
#include <fstream>
#include <set>

void sighandler(int s)
{
	std::cout << "Caught signal " << s << ".\n"; // this is undefined behaviour
	std::cout << "try 'touch stop'" << std::endl;
}

std::vector<HashBuffer*> buf;

uint64_t total_count = 0;
std::mutex m;
ReferencesDatabase * refDb = nullptr;
TableGenomic * tabGenomic = nullptr;
unsigned skip = 0;
std::vector<uint64_t> genomicReferencesToKeyOffsets;
inline void genomicKey2Coordinates(uint64_t key, ReferenceId & refId, uint64_t & position)
{
	auto it = std::upper_bound( genomicReferencesToKeyOffsets.begin(), genomicReferencesToKeyOffsets.end(), key );
	ASSERT( it != genomicReferencesToKeyOffsets.begin() );
	--it;
	position = key - *it;
	refId.value = it - genomicReferencesToKeyOffsets.begin();
}


int main(int argc, char ** argv)
{
	std::signal(SIGINT, sighandler);

	if(argc != 5) {
		std::cout << "try ./getDuplicateCaIdRecords <start> <increment> <dbAlleles-directory> <bloom-path>" << std::endl;
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
	tabGenomic = new TableGenomic(directory, parse(directory+"/"+"genomic.yaml"), &taskManager, &taskManager2, 32, nextCaId);
	uint64_t incG = 64L*1024*1024;
	uint64_t __incG = 1L*1024*1024;
	uint64_t skipG = start;
	uint64_t maxG = skipG+increment;
	std::vector<std::thread> workers;
	int N_THREADS = 32;
	std::vector<std::vector<std::pair<uint64_t,uint64_t> > > params(N_THREADS);
	for(int i=0;;i++) {
		uint64_t _skipG = skipG+i*incG;
		uint64_t _maxG = _skipG+incG;
		if(_maxG > 4294967295) break;
		params[i%N_THREADS].push_back(std::make_pair(_skipG,_maxG));
		if(_skipG > maxG) break;
	}
	char ch;
	//std::cin >> ch;
	for(int t=0;t<N_THREADS;t++) {
		workers.push_back(std::thread([t,params,__incG]()
		{
			for(int j=0;j<params[t].size();j++) {
				uint64_t __skipG = params[t][j].first;
				uint64_t __maxG = params[t][j].second;
				auto visitor = [&__skipG,&__incG,&__maxG](std::vector<RecordGenomicVariant*> & varRecords, bool lastCall)
				{
					for(auto r:varRecords) {
						uint64_t buf_id = -1;
						uint64_t val_id = -1;
						buf_id = r->identifiers.lastId()/4294967296L;
						val_id = r->identifiers.lastId()%4294967296L;
						//if(r->identifiers.lastId() < 4294967296L) {
						//	buf_id = 0;
						//	val_id = r->identifiers.lastId()%4294967296L;
						//} else {
						//	buf_id = (r->identifiers.lastId()/10)/4294967296L;
						//	val_id = (r->identifiers.lastId()/10)%4294967296L;
						//}
						m.lock();
						if(buf[buf_id]->get(val_id) == false) {
							buf[buf_id]->set(val_id);
							total_count++;
						} else {
							std::cout << "CA" << r->identifiers.lastId() << std::endl;
						}
						m.unlock();
					}
					m.lock();
					std::cout << total_count << std::endl;
					m.unlock();
				};
				tabGenomic->query(visitor, skip, __skipG, __maxG, __incG);
			}
		}
		));
	}
	for(int t=0;t<N_THREADS;t++) {
		workers[t].join();
	}
	std::cout << "Total count: " << total_count << std::endl;
	for(int i=0;i<buf.size();i++) {
		std::cout << "start - save bloom filter" << std::endl;
		buf[i]->save();
		std::cout << "done - save bloom filter" << std::endl;
	}
	return 0;
}
