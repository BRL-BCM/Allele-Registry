#include "db.hpp"
#include "TestRecord.hpp"
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
	for (auto r2: newRecords) {
		TestRecord * r = dynamic_cast<TestRecord*>(r2);
		if (r->key != key) throw std::logic_error("Key does not match!!!!");
		currentRecords.push_back(new TestRecord(*r));
	}
	return true;
}


int main(int argc, char ** argv)
{
	if (argc != 3) {
		std::cout << "Parameters: name_of_database(without_extension)  file_with_data" << std::endl;
		return 1;
	}

    std::string dbName = argv[1];
	std::string dataset = argv[2];

	// load data
	{
		std::cout << "Read input data" << std::endl;
		std::ifstream file(dataset);
		if (file.fail()) {
			std::cerr << "Cannot open file " << (dataset) << std::endl;
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
	TasksManager * tm = new TasksManager(1);
	TasksManager * tm2 = new TasksManager(1);
	DatabaseT<> * db = new DatabaseT<>(tm, tm2, dbName, createRecord<TestRecord>);
	std::cout << "Load data into database" << std::endl;
	unsigned const chunkSize = 4*128*1024;
	for (unsigned ir = 0; ir < data.size(); ) {
		std::vector<RecordT<uint32_t>*> records;
		records.reserve(chunkSize);
		for ( ;  ir < data.size() && records.size() < chunkSize;  ++ir ) {
			records.push_back( new TestRecord(data[ir][0], data[ir][1], data[ir][2]) );
		}
		db->writeRecords(records, funcUpdate);
		for (auto r: records) delete r;
		std::cout << "Number of chunks written: " << (ir / chunkSize) << std::endl;
	}

	delete db;
	std::cout << "Database object deleted" << std::endl;
	delete tm;
	std::cout << "Scheduler deleted" << std::endl;
	return 0;
}
