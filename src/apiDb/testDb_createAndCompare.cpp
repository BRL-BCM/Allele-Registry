#include "db.hpp"
#include "TestRecord.hpp"
#include "../commonTools/Stopwatch.hpp"
#include <iostream>
#include <fstream>
#include <array>
#include <algorithm>

std::vector<std::array<uint32_t,3>> data;


bool funcUpdate(std::vector<RecordT<uint32_t>*> & currentRecords, std::vector<RecordT<uint32_t>*> const & newRecords)
{
//	std::cout << "funcUpdate" << std::endl;
	if (newRecords.empty()) throw std::logic_error("funcUpdate called for empty newRecords");
	unsigned const key = newRecords.front()->key;
	bool changes = false;
	for (auto rr: newRecords) {
		TestRecord *r = dynamic_cast<TestRecord*>(rr);
		if (r->key != key) throw std::logic_error("Key does not match!!!!");
		bool exist = false;
		for (auto rr2: currentRecords) {
			TestRecord *r2 = dynamic_cast<TestRecord*>(rr2);
			if (r2->key != key) throw std::logic_error("Key does not match!!!!");
			if (r2->fData[0] == r->fData[0] && r2->fData[1] == r->fData[1]) {
				exist = true;
				break;
			}
		}
		if (exist) {
			delete r;
		} else {
//std::cout << "Difference: \n";
//for (auto r: currentRecords) std::cout << " " << *(dynamic_cast<TestRecord*>(r));
//std::cout << "\n";
//for (auto r: newRecords) std::cout << " " << *(dynamic_cast<TestRecord*>(r));
//std::cout << std::endl;
			changes = true;
			currentRecords.push_back(rr);
		}
	}
	return changes;
}


int main(int argc, char ** argv)
{
	if (argc != 3) {
		std::cout << "Parameters: name_of_database(without_extension)  file_with_input_data" << std::endl;
		return 1;
	}

	std::string database = argv[1];
	std::string inputFile = argv[2];

	// load data
	{
		std::cout << "Read input data" << std::endl;
		std::ifstream file(inputFile);
		if (file.fail()) {
			std::cerr << "Cannot open file " << inputFile << std::endl;
			return 2;
		}
		while (! file.eof()) {
			std::array<uint32_t,3> r;
			file >> r[0];
			if (file.eof()) break;
			file >> r[1] >> r[2];
			if (file.fail()) {
				std::cerr << "Error when reading from file after " << data.size() << " records" << std::endl;
				return 2;
			}
			data.push_back(r);
		}
		std::cout << "Number of records: " << data.size() << std::endl;
	}

	// create database
	TasksManager * tm = new TasksManager(4);
	TasksManager * tm2 = new TasksManager(4);
	DatabaseT<> * db = new DatabaseT<>(tm, tm2, database, createRecord<TestRecord>);
	std::cout << "Load data into database" << std::endl;
	unsigned const chunkSize = 1024*1024;
	unsigned chunkId = 0;
	for (unsigned ir = 0; ir < data.size(); ) {
		std::vector<RecordT<uint32_t>*> records;
		records.reserve(chunkSize);
		for ( ;  ir < data.size() && records.size() < chunkSize;  ++ir ) {
			records.push_back( new TestRecord(data[ir][0], data[ir][1], data[ir][2]) );
		}
		Stopwatch stopwatch;
		db->writeRecords(records, funcUpdate);
		std::cout << "Number of chunks written: " << (++chunkId) << "\tTime(ms): " << stopwatch.get_time_ms() <<  std::endl;
	}

	// sort the data
	std::cout << "Sort the data" << std::endl;
	std::sort( data.begin(), data.end() );

	// read database
	{
		std::cout << "Read records from database:" << std::endl;
		unsigned currentRecordId = 0;

		auto funcRead = [&currentRecordId](std::vector<RecordT<uint32_t> const *> const & records, bool lastCall)
		{
			for (auto rr: records) {
				TestRecord const * r = dynamic_cast<TestRecord const *>(rr);
				if (currentRecordId >= data.size()) {
					std::cerr << "Too many records were returned" << std::endl;
					throw std::runtime_error("TEST FAILED");
				}
				if (r->key != data[currentRecordId][0]) {
					std::cerr << "Incorrect key, expected record: [" << data[currentRecordId][0] << "," << data[currentRecordId][1];
					std::cerr << ", " << data[currentRecordId][2] << "], returned record: " << *r << std::endl;
					throw std::runtime_error("TEST FAILED");
				}
				if (r->fData[0] != data[currentRecordId][1] || r->fData[1] != data[currentRecordId][2]) std::cerr << "Incorrect data" << std::endl;
				++currentRecordId;
			}
		};

		Stopwatch stopwatch;
		db->readRecordsInOrder(funcRead);
		std::cout << "Reading time(s): " << stopwatch.get_time_sec() << std::endl;

		if (currentRecordId != data.size()) std::cerr << "Number of records read != number of input records" << std::endl;
	}

	delete db;
	std::cout << "Database object deleted" << std::endl;
	delete tm;
	std::cout << "Scheduler deleted" << std::endl;
	return 0;
}
