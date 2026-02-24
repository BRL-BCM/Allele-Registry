#include "TableGenomic64.hpp"
#include "IndexIdentifierCa.hpp"
#include "../apiDb/TasksManager.hpp"
#include <iostream>
#include <time.h>
#include <chrono>
#include <fstream>
#include <csignal>
using namespace std::chrono;

uint16_t getLengthParam(unsigned val, variantCategory vc, unsigned regionLength)
{
	unsigned min = 0;
	unsigned max = 10000;
	switch (vc) {
		case variantCategory::nonShiftable      : break;
		case variantCategory::shiftableInsertion: min = 1; break;
		case variantCategory::duplication       : min = 1; max = regionLength; break;
		case variantCategory::shiftableDeletion : min = 1; max = regionLength-1; break;
	}
	return ((val-min) % (max-min+1) + min);
}



void sighandler(int s)
{
	std::cout << "Caught signal " << s << ".\n"; // this is undefined behaviour
	std::cout << "try 'touch stop'" << std::endl;
}


int main(int argc, char ** argv)
{
	std::signal(SIGINT, sighandler);

	auto clock0 = high_resolution_clock::now();
	srand(time(0));

	std::atomic<uint64_t> nextFreeCaId(1);
	TasksManager taskManager (64);
	TasksManager taskManager2(64);
	TasksManager taskManager3(64);
	TasksManager taskManager4(64);
	std::cout << "pass 1" << "\t";
	auto clock1 = high_resolution_clock::now();
	std::cout << std::chrono::duration<double>(duration_cast<microseconds>(clock1 - clock0)).count() << std::endl;
	IndexIdentifierCa * caTab = new IndexIdentifierCa("/data/WORKSPACE/antonk/data/dbAlleles-convert-genomic-dataNode-64bit-target", &taskManager, &taskManager2, 1280/2);
	nextFreeCaId = caTab->getMaxIdentifier() + 1;
	std::cout << nextFreeCaId << std::endl;
	std::cout << "pass 2" << "\t";
	auto clock2 = high_resolution_clock::now();
	std::cout << std::chrono::duration<double>(duration_cast<microseconds>(clock2 - clock1)).count() << std::endl;
	TableGenomic * tab = new TableGenomic("/data/WORKSPACE/antonk/data/dbAlleles-convert-genomic-dataNode-64bit-target", &taskManager3, &taskManager4, 1280/2, nextFreeCaId);
	std::cout << "pass 3" << "\t";
	auto clock3 = high_resolution_clock::now();
	std::cout << std::chrono::duration<double>(duration_cast<microseconds>(clock3 - clock2)).count() << std::endl;



	std::vector<RecordGenomicVariant*> records1;

	//3 221 486 994
	unsigned multiplier = (unsigned)((3200000.0)/80000.0);
	unsigned num = (1000)*80000;
	unsigned pos; 
	for (unsigned i = 0; i < num; ++i) {

		std::vector<BinaryNucleotideSequenceModification> rawDef(1);
		BinaryNucleotideSequenceModification & sr = rawDef.front();
		pos = multiplier*i+(rand()%multiplier);
		sr.position = pos;
		sr.category = static_cast<variantCategory>(rand()%4);
		sr.lengthBefore = (rand()%4) + 2;
		sr.lengthChangeOrSeqLength = getLengthParam(rand()%4,sr.category,sr.lengthBefore);
		RecordGenomicVariant * r = new RecordGenomicVariant( BinaryGenomicVariantDefinition(rawDef) );
		r->revision = i;
		records1.push_back(r);
		records1.back()->identifiers.lastId() = nextFreeCaId;
		nextFreeCaId++;
	}
	std::cout << "pass 4" << "\t";
	auto clock4 = high_resolution_clock::now();
	std::cout << std::chrono::duration<double>(duration_cast<microseconds>(clock4 - clock3)).count() << std::endl;

	std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr>>> changesInIndexes;
	tab->fetchAndAdd(records1, changesInIndexes);
	std::cout << "pass 5" << "\t";

	auto clock5 = high_resolution_clock::now();
	std::cout << std::chrono::duration<double>(duration_cast<microseconds>(clock5 - clock4)).count() << std::endl;
	sleep(10);
	std::cout << changesInIndexes.size() << std::endl;
	std::cout << records1.size() << std::endl;
	std::vector<RecordGenomicVariant *> records2;
	for(auto x:changesInIndexes.at(identifierType::CA)) {
		records2.push_back(x.second.asRecordGenomicVariantPtr());
	}
	std::cout << "pass 6" << "\t";
	auto clock6 = high_resolution_clock::now();
	std::cout << std::chrono::duration<double>(duration_cast<microseconds>(clock6 - clock5)).count() << std::endl;
	std::vector<RecordGenomicVariant const *> ind(records2.begin(), records2.end());
	caTab->addIdentifiers(ind);
	std::cout << "pass 7" << "\t";
	auto clock7 = high_resolution_clock::now();
	std::cout << std::chrono::duration<double>(duration_cast<microseconds>(clock7 - clock6)).count() << std::endl;

	delete tab;
	delete caTab;

	auto clock_end = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(clock_end - clock0);
	std::ofstream outfile;

	outfile.open("timinig.txt", std::ios_base::app); // append instead of overwrite
	outfile << std::chrono::duration<double>(duration).count() << "\t" << nextFreeCaId << std::endl;
	outfile.close();

	return 0;
}
