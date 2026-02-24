#include "IndexIdentifierCa32.hpp"
#include "IndexIdentifierCa.hpp"
#include "TableProtein32.hpp"
#include "TableProtein64.hpp"
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
	std::signal(SIGINT, sighandler);

	if(argc != 3) {
		std::cout << "try ./convertProteinDataNodeDeployment <in-directory> <out-directory>" << std::endl;
		exit(1);
	}

	unsigned skip = 0;
	std::string inDir = argv[1];
	std::string outDir = argv[2];

	std::atomic<uint64_t> nextFreeCaId32(1);
	std::atomic<uint64_t> nextFreeCaId64(1);
	TasksManager taskManager1(32);
	TasksManager taskManager2(32);
	TasksManager taskManager3(32);
	TasksManager taskManager4(32);
	TableProtein32 * tab32 = new TableProtein32( inDir, parse( inDir+"/"+"protein.yaml"), &taskManager1, &taskManager2, 128*5, nextFreeCaId32);
	TableProtein   * tab   = new TableProtein  (outDir, parse(outDir+"/"+"protein.yaml"), &taskManager3, &taskManager4, 128*5, nextFreeCaId64);

	uint64_t skipG = 0;
	uint64_t incG = 16L*1024*1024;
	uint64_t maxG = std::numeric_limits<uint64_t>::max();
	auto visitor = [&tab,&skipG,&incG,&maxG](std::vector<RecordProteinVariant32*> & varRecords32, bool lastCall)
	{
		std::cout << "P progress:" << skipG << "\t" << maxG << std::endl;
		std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr>>> changesInIndexes;
		std::vector<RecordProteinVariant*> varRecords;
		for(auto rec32 : varRecords32) {
			std::vector<BinaryAminoAcidSequenceModification> definition;
			for(auto modification32 : rec32->definition.raw()) {
				BinaryAminoAcidSequenceModification modification64;
				modification64.position = modification32.position;
				modification64.lengthBefore = modification32.lengthBefore;
				modification64.lengthChangeOrSeqLength = modification32.lengthChangeOrSeqLength;
				modification64.sequence = modification32.sequence;
				modification64.category = modification32.category;
				definition.push_back(modification64);
			}
			RecordProteinVariant* rec64 = new RecordProteinVariant(BinaryProteinVariantDefinition(rec32->definition.proteinAccessionId(), definition));
			rec64->identifiers = BinaryIdentifiers( identifierType::PA
						, rec32->identifiers.rawShort()
						, rec32->identifiers.rawHgvs() );
			rec64->identifiers.lastId() = rec32->identifiers.lastId();
			rec64->revision = rec32->revision;
			varRecords.push_back(rec64);
		}
		tab->fetchAndAdd(varRecords, changesInIndexes);
		for(auto rec64 : varRecords) {
			delete rec64;
		}
		varRecords.clear();
	};
	tab32->query(visitor, skip, skipG, maxG, incG);
	return 0;
}





















