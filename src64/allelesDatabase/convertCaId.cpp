#include "IndexIdentifierCa32.hpp"
#include "IndexIdentifierCa.hpp"
#include "../apiDb/TasksManager.hpp"
#include "../flatDb/Convert.hpp"
#include <iostream>
#include <csignal>
#include "../commonTools/parseHeader.h"

void sighandler(int s)
{
	std::cout << "Caught signal " << s << ".\n"; // this is undefined behaviour
	std::cout << "try 'touch stop'" << std::endl;
}

int main(int argc, char ** argv)
{
	if(argc != 3) {
		std::cout << "try ./convertCaId <in-dir> <out-dir>" << std::endl;
		exit(1);
	}

	std::string inDir = argv[1];
	std::string outDir = argv[2];

	std::signal(SIGINT, sighandler);

	TasksManager taskManager(32);
	TasksManager taskManager2(32);
	IndexIdentifierCa32 * caTab32 = new IndexIdentifierCa32( inDir, parse(inDir +"/"+"caId.yaml"), &taskManager, &taskManager2, 128*5);
	IndexIdentifierCa   * caTab   = new IndexIdentifierCa  (outDir, parse(outDir+"/"+"caId.yaml"), &taskManager, &taskManager2, 128*5);
	uint32_t largestId = caTab->getMaxIdentifier();
	std::cout << "largestId:" << largestId << std::endl;
	if(largestId > 10) largestId -= 10;
	
	std::vector<uint32_t> caIds;
	for(uint32_t i=largestId;i<largestId+10000000;i++) {
		caIds.push_back(i);
	}
	std::vector<RecordGenomicVariant*> ret = caTab32->fetchDefinitions(caIds);
	std::vector<RecordGenomicVariant*> ret2;
	for(uint32_t i=0;i<ret.size();i++) {
		//std::cout << i << "\t" << ret[i] << std::endl;
		if(ret[i] != NULL) {
			ret2.push_back(ret[i]);
		}
	}
	std::vector<RecordGenomicVariant const *> ind(ret2.begin(), ret2.end());
	caTab->addIdentifiers(ind);

	return 0;
}
