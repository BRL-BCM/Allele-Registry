#include "IndexIdentifierCa32.hpp"
#include "IndexIdentifierCa.hpp"
#include "TableGenomic32.hpp"
#include "TableGenomic64.hpp"
#include "../apiDb/TasksManager.hpp"
#include "../flatDb/Convert.hpp"
#include <iostream>
#include <csignal>

void sighandler(int s)
{
	std::cout << "Caught signal " << s << ".\n"; // this is undefined behaviour
	std::cout << "try 'touch stop'" << std::endl;
}

int main(int argc, char ** argv)
{
	std::signal(SIGINT, sighandler);

	if(argc != 3) {
		std::cout << "try ./convertGenomicDataNode <start> <increment>" << std::endl;
		exit(1);
	}

	unsigned skip = 0;
	uint64_t start = atoll(argv[1]);
	uint64_t increment = atoll(argv[2]);

	std::atomic<uint64_t> nextFreeCaId32(1);
	std::atomic<uint64_t> nextFreeCaId64(1);
	TasksManager taskManager1(32);
	TasksManager taskManager2(32);
	TasksManager taskManager3(32);
	TasksManager taskManager4(32);
	TableGenomic32 * tab32 = new TableGenomic32("/data/WORKSPACE/antonk/data/dbAlleles-convert-genomic-dataNode-64bit-source", &taskManager1, &taskManager2, 128*5, nextFreeCaId32);
	TableGenomic   * tab   = new TableGenomic  ("/data/WORKSPACE/antonk/data/dbAlleles-convert-genomic-dataNode-64bit-target", &taskManager3, &taskManager4, 128*5, nextFreeCaId64);

	uint64_t skipG = start;
	uint64_t incG = 16L*1024*1024;
	uint64_t maxG = skipG+increment;
	auto visitor = [&tab,&skipG,&incG,&maxG](std::vector<RecordGenomicVariant32*> & varRecords32, bool lastCall)
	{
		std::cout << "G progress:" << skipG << "\t" << maxG << std::endl;
		std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr>>> changesInIndexes;
		std::vector<RecordGenomicVariant*> varRecords;
		for(auto rec32 : varRecords32) {
			std::vector<BinaryNucleotideSequenceModification> definition;
			for(auto modification32 : rec32->definition.raw()) {
				BinaryNucleotideSequenceModification modification64;
				modification64.position = modification32.position;
				modification64.lengthBefore = modification32.lengthBefore;
				modification64.lengthChangeOrSeqLength = modification32.lengthChangeOrSeqLength;
				modification64.sequence = modification32.sequence;
				modification64.category = modification32.category;
				definition.push_back(modification64);
			}
			RecordGenomicVariant* rec64 = new RecordGenomicVariant(BinaryGenomicVariantDefinition(definition));
			rec64->identifiers = BinaryIdentifiers( identifierType::CA
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





















