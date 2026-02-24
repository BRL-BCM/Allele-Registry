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

void sighandler(int s)
{
	std::cout << "Caught signal " << s << ".\n"; // this is undefined behaviour
	std::cout << "try 'touch stop'" << std::endl;
}

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

	std::string directory = argv[3];
	TasksManager taskManager(32);
	TasksManager taskManager2(32);
	std::cout << "pass 1" << std::endl;
	std::atomic<uint64_t> nextCaId((uint64_t)0);
	std::map<identifierType, IndexIdentifierUInt64*> index;
	IndexIdentifierPa * indexIdentifierPa = new IndexIdentifierPa(directory, parse(directory+"/"+"idPa.yaml"), &taskManager, &taskManager2, 32);
	TableGenomic * tabGenomic = new TableGenomic(directory, parse(directory+"/"+"genomic.yaml"), &taskManager, &taskManager2, 32, nextCaId);
	TableProtein * tabProtein = new TableProtein(directory, parse(directory+"/"+"protein.yaml"), &taskManager, &taskManager2, 32, nextCaId);
	std::cout << "pass 2" << std::endl;
	index[identifierType::dbSNP] = new IndexIdentifierUInt64(directory, parse(directory+"/"+"idDbSnp.yaml"), &taskManager, &taskManager2, "DbSnp", 32);
	index[identifierType::ClinVarAllele] = new IndexIdentifierUInt64(directory, parse(directory+"/"+"idClinVarAllele.yaml"), &taskManager, &taskManager2, "ClinVarAllele", 32);
	index[identifierType::ClinVarVariant] = new IndexIdentifierUInt64(directory, parse(directory+"/"+"idClinVarVariant.yaml"), &taskManager, &taskManager2, "ClinVarVariant", 32);
	index[identifierType::ClinVarRCV] = new IndexIdentifierUInt64(directory, parse(directory+"/"+"idClinVarRCV.yaml"), &taskManager, &taskManager2, "ClinVarRCV", 32);
	std::cout << "pass 3" << std::endl;
	std::cout << "pass 4" << std::endl;
	uint64_t skipG = start;
	uint64_t incG = 16L*1024*1024;
	uint64_t maxG = skipG+increment;
	uint64_t skipP = 0;
	uint64_t incP = 16L*1024*1024;
	uint64_t maxP = std::numeric_limits<uint64_t>::max();
	auto visitor = [&index,&skipG,&incG,&maxG](std::vector<RecordGenomicVariant*> & varRecords, bool lastCall)
	{
		std::cout << "G progress:" << skipG << "\t" << maxG << std::endl;
		std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr>>> ids;
		for(auto r:varRecords) {
			r->identifiers.saveShortIdsToContainer(ids, RecordVariantPtr(r));
		}
		for(auto const & kv:ids) {
			if(index.count(kv.first)) {
				index[kv.first]->addIdentifiers(kv.second);
			}
		}
		for(auto & kv:ids) {
			kv.second.clear();
		}
		ids.clear();
		skipG+=incG;
	};
	tabGenomic->query(visitor, skip, skipG, maxG, incG);
	auto visitor2 = [&index,&skipP,&incP,&maxP,&indexIdentifierPa](std::vector<RecordProteinVariant*> & varRecords, bool lastCall)
	{
		std::cout << "P progress:" << skipP << "\t" << maxP << std::endl;
		std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr>>> ids;
		for(auto r:varRecords) {
			r->identifiers.saveShortIdsToContainer(ids, RecordVariantPtr(r));
		}
		for(auto const & kv:ids) {
			if(index.count(kv.first)) {
				index[kv.first]->addIdentifiers(kv.second);
			}
		}
		for(auto & kv:ids) {
			kv.second.clear();
		}
		ids.clear();
		std::vector<RecordProteinVariant const *> varRecords2(varRecords.begin(), varRecords.end());
		indexIdentifierPa->addIdentifiers(varRecords2);
		skipP+=incP;
	};
	tabProtein->query(visitor2, skip, skipP, maxP, incP);
	return 0;
}
