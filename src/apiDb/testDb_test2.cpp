#include "db.hpp"
#include "TestRecord.hpp"
#include <iostream>
#include <fstream>
#include <array>
#include <algorithm>
#include <thread>
#include <random>
#include "../commonTools/Stopwatch.hpp"


bool funcUpdate(std::vector<RecordT<uint32_t>*> & currentRecords, std::vector<RecordT<uint32_t>*> const & newRecords)
{
//	std::cout << "funcUpdate" << std::endl;
//	if (newRecords.empty()) throw std::logic_error("funcUpdate called for empty newRecords");
//	unsigned const key = newRecords.front()->key;
//	for (auto r2: newRecords) {
//		TestRecord * r = dynamic_cast<TestRecord*>(r2);
//		if (r->key != key) throw std::logic_error("Key does not match!!!!");
//		currentRecords.push_back(new TestRecord(*r));
//	}
	return true;
}


int main(int argc, char ** argv)
{
	if (argc != 3) {
		std::cout << "Parameters: name_of_database(without_extension) millionsOfRecordsInChunk" << std::endl;
		return 1;
	}

    std::string dbName = argv[1];
	unsigned const chunkSize = atoi(argv[2])*1024*1024;

	std::uniform_int_distribution<unsigned> randKey;
	std::uniform_int_distribution<uint64_t> randData;
	std::random_device gen;

	// create database
	TasksManager * tm = new TasksManager(2);
	TasksManager * tm2 = new TasksManager(2);
	DatabaseT<> * db = new DatabaseT<>(tm, tm2, dbName, createRecord<TestRecord>);

	std::cout << "Preparing data... " << std::endl;
	std::vector<RecordT<uint32_t>*> records;
	for ( unsigned i = 0;  i < chunkSize;  ++i ) {
		records.push_back( new TestRecord(randKey(gen), randData(gen), randData(gen)) );
	}

	unsigned long long allRecords = db->getRecordsCount();

	records.reserve(chunkSize);
	for (unsigned iter = 1; ; ++iter ) {
		std::cout << "========= Iteration " << iter << std::endl;
		std::cout << "Writing to database... " << std::flush;
		Stopwatch sw;
		db->writeRecords(records, funcUpdate);
		std::cout << sw.save_and_restart_sec() << " seconds\n";
		std::cout << "Done!" << std::endl;
		std::cout << "Total number of records: " << allRecords << std::endl;
	}

	delete db;
	std::cout << "Database object deleted" << std::endl;
	delete tm;
	std::cout << "Scheduler deleted" << std::endl;
	return 0;
}
