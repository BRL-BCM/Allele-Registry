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
	//std::cout << "you can terminate now" << std::endl;
	//char d=(char)(7);
	//printf("%c\n",d);
	//sleep(10);
	//std::cout << "too late" << std::endl;

	auto clock0 = high_resolution_clock::now();
	srand(time(0));

	std::atomic<uint64_t> nextFreeCaId(1);
	TasksManager taskManager (64);
	TasksManager taskManager2(64);
	TasksManager taskManager3(64);
	TasksManager taskManager4(64);
	//TableGenomic * tab = new TableGenomic("./dbTest/", &taskManager, &taskManager2, 128, nextFreeCaId);
	//TableGenomic * tab = new TableGenomic("/media/antonk/secondary/dbAlleles-64bit/", &taskManager, &taskManager2, 128, nextFreeCaId);
	//IndexIdentifierCa * caTab = new IndexIdentifierCa("/usr/local/brl/data/alleleRegistry/dbAlleles-64bit", &taskManager, &taskManager2, 1280);
	std::cout << "pass 1" << "\t";
	auto clock1 = high_resolution_clock::now();
	std::cout << std::chrono::duration<double>(duration_cast<microseconds>(clock1 - clock0)).count() << std::endl;
	IndexIdentifierCa * caTab = new IndexIdentifierCa("/data/WORKSPACE/antonk/data/dbAlleles-convert-genomic-dataNode-64bit-target", &taskManager, &taskManager2, 1280/2);
	nextFreeCaId = caTab->getMaxIdentifier() + 1;
	std::cout << nextFreeCaId << std::endl;
	//TableGenomic * tab = new TableGenomic("/usr/local/brl/data/alleleRegistry/dbAlleles-64bit", &taskManager, &taskManager2, 1280, nextFreeCaId);
	std::cout << "pass 2" << "\t";
	auto clock2 = high_resolution_clock::now();
	std::cout << std::chrono::duration<double>(duration_cast<microseconds>(clock2 - clock1)).count() << std::endl;
	TableGenomic * tab = new TableGenomic("/data/WORKSPACE/antonk/data/dbAlleles-convert-genomic-dataNode-64bit-target", &taskManager3, &taskManager4, 1280/2, nextFreeCaId);
	std::cout << "pass 3" << "\t";
	auto clock3 = high_resolution_clock::now();
	std::cout << std::chrono::duration<double>(duration_cast<microseconds>(clock3 - clock2)).count() << std::endl;



	std::vector<RecordGenomicVariant*> records1;
	//std::vector<RecordGenomicVariant*> records2;
	//std::vector<RecordGenomicVariant*> records3;

	//3 221 486 994
	// 80 000
	unsigned multiplier = (unsigned)((3200000.0)/80000.0);
	unsigned num = (1000)*80000;
	unsigned pos; 
	for (unsigned i = 0; i < num; ++i) {

		std::vector<BinaryNucleotideSequenceModification> rawDef(1);
		BinaryNucleotideSequenceModification & sr = rawDef.front();
		pos = multiplier*i+(rand()%multiplier);
		sr.position = pos;
		sr.category = static_cast<variantCategory>(rand()%4);
		sr.lengthBefore = (rand()%4) + 2;//(rand() % 100) + 2;
		sr.lengthChangeOrSeqLength = getLengthParam(rand()%4,sr.category,sr.lengthBefore);
		//if (sr.category == variantCategory::nonShiftable || sr.category == variantCategory::shiftableInsertion) {
		//	sr.sequence = i*multiplier;
		//	if (sr.lengthChangeOrSeqLength < 16) sr.sequence %= (1u << 2*sr.lengthChangeOrSeqLength);
		//}
		//if (rand() % 17 == 3) {
		//	for (unsigned j = 0; j < rand() % 4; ++j) {
		//		BinaryNucleotideSequenceModification sr;
		//		sr.position = pos + 1000*(j+1);
		//		sr.category = static_cast<variantCategory>(rand()%4);
		//		sr.lengthBefore = (rand() % 100) + 2;
		//		sr.lengthChangeOrSeqLength = getLengthParam(i+j,sr.category,sr.lengthBefore);
		//		if (sr.category == variantCategory::nonShiftable || sr.category == variantCategory::shiftableInsertion) {
		//			sr.sequence = i*multiplier;
		//			if (sr.lengthChangeOrSeqLength < 16) sr.sequence %= (1u << 2*sr.lengthChangeOrSeqLength);
		//		}
		//		rawDef.push_back(sr);
		//	}
		//}
		RecordGenomicVariant * r = new RecordGenomicVariant( BinaryGenomicVariantDefinition(rawDef) );
		r->revision = i;
		records1.push_back(r);
		//records2.push_back(new RecordGenomicVariant(*r));
		//records3.push_back(new RecordGenomicVariant(*r));
		records1.back()->identifiers.lastId() = nextFreeCaId;
		//records2.back()->identifiers.add( Identifier_dbSNP(i+1000000) );
		//records3.back()->identifiers.lastId() = i;
		//records3.back()->identifiers.add( Identifier_dbSNP(i+1000000) );
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
	//delete tab;
	//while(changesInIndexes.size() != 1) {
	//	usleep(1000);
	//}
	sleep(10);
	std::cout << changesInIndexes.size() << std::endl;
	//std::cout << changesInIndexes.at(identifierType::CA).size() << std::endl;
	std::cout << records1.size() << std::endl;
	//if (changesInIndexes.size() != 1 || changesInIndexes.at(identifierType::CA).size() != records1.size()) {
	//	for(auto x:changesInIndexes.at(identifierType::CA)) {
	//		std::cout << x.first << std::endl;
	//	}
	//	throw std::logic_error("Assertion 1");
	//}
	std::vector<RecordGenomicVariant *> records2;
	for(auto x:changesInIndexes.at(identifierType::CA)) {
		records2.push_back(x.second.asRecordGenomicVariantPtr());
	}
	//for(auto x:records1) {
	//	records2.push_back(x);
	//}
	std::cout << "pass 6" << "\t";
	auto clock6 = high_resolution_clock::now();
	std::cout << std::chrono::duration<double>(duration_cast<microseconds>(clock6 - clock5)).count() << std::endl;
	//for(auto y:changesInIndexes) {
	//	y.second.clear();
	//}
	//changesInIndexes.clear();
	std::vector<RecordGenomicVariant const *> ind(records2.begin(), records2.end());
	caTab->addIdentifiers(ind);
	std::cout << "pass 7" << "\t";
	auto clock7 = high_resolution_clock::now();
	std::cout << std::chrono::duration<double>(duration_cast<microseconds>(clock7 - clock6)).count() << std::endl;
	//delete caTab;

	//changesInIndexes.clear();
	//tab->fetchAndAdd(records2, changesInIndexes);
	//caTab->addIdentifiers(records2);
	//if (changesInIndexes.size() != 1 || changesInIndexes.at(identifierType::dbSNP).size() != records1.size()) throw std::logic_error("Assertion 2");

	//for (unsigned i = 0; i < records2.size(); ++i) {
	//	if (records2[i]->identifiers != records3[i]->identifiers) throw std::logic_error("Assertion 3");
	//}

	//std::vector<RecordGenomicVariant*> records4;
	//auto callback = [&records4](std::vector<RecordGenomicVariant*> & records, bool & lastCall)
	//{
	//	records4.insert( records4.end(), records.begin(), records.end() );
	//	records.clear();
	//};
	//unsigned skip = 0;
	//tab->query(callback, skip);

	//if (records3.size() != records4.size()) {
	//	std::cerr << "ERROR: incorrect records count: " << records3.size() << " != " <<  records4.size() << std::endl;
	//}

	//for (unsigned i = 0; i < records3.size(); ++i) {
	//	try {
	//		if (records3[i]->definition  != records4[i]->definition ) throw std::logic_error("Assertion 4");
	//		if (records3[i]->identifiers != records4[i]->identifiers) throw std::logic_error("Assertion 6");
	//	} catch (std::exception const & e) {
	//		std::cerr << "EXCEPTION: " << e.what() << " i=" << i << " " << records3[i]->definition.toString() << " " << records4[i]->definition.toString() << std::endl;
	//		break;
	//	}
	//}


	delete tab;
	delete caTab;

	//std::cout << "OK!" << std::endl;
	auto clock_end = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(clock_end - clock0);
	std::ofstream outfile;

	outfile.open("timinig.txt", std::ios_base::app); // append instead of overwrite
	outfile << std::chrono::duration<double>(duration).count() << "\t" << nextFreeCaId << std::endl;
	outfile.close();

	return 0;
}
