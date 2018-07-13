#include "db.hpp"
#include "TestRecord.hpp"
#include "../commonTools/assert.hpp"
#include "../commonTools/Stopwatch.hpp"
#include <iostream>
#include <fstream>
#include <array>
#include <algorithm>


typedef RecordT<uint32_t> Record;


int main(int argc, char ** argv)
{
	if (argc != 3) {
		std::cout << "Parameters: name of dataset (without extension), name of file with records" << std::endl;
		return 1;
	}

	std::string const dataset = argv[1];
	std::string const filename = argv[2];

	std::vector<Record*> records;

	// load data
	{
		std::cout << "Read input data" << std::endl;
		std::ifstream file(filename);
		if (file.fail()) {
			std::cerr << "Cannot open file " << filename << std::endl;
			return 2;
		}
		while (! file.eof()) {
			std::array<uint32_t,3> r;
			file >> r[0];
			if (file.eof()) break;
			file >> r[1] >> r[2];
			if (file.fail()) {
				std::cerr << "Error when reading from file after " << records.size() << " records" << std::endl;
				return 2;
			}
			records.push_back(new TestRecord(r[0],r[1],r[2]));
		}
		std::cout << "Number of records: " << records.size() << std::endl;
	}

	// open database
	TasksManager * tm = new TasksManager(4);
	TasksManager * tm2 = new TasksManager(4);
	DatabaseT<> * db = new DatabaseT<>(tm, tm2, dataset, createRecord<TestRecord>);

	// read database
	{
		std::cout << "Read records from database:" << std::endl;
		Stopwatch timer;
		unsigned omittedRecords = 0;

		auto funcRead = [&omittedRecords](std::vector<Record const *> const & dbRecords, std::vector<Record*> const & records)
		{
			ASSERT(! records.empty());
			unsigned const key = records.front()->key;
			for (auto r: dbRecords) ASSERT(r->key == key);
			for (auto r: records  ) ASSERT(r->key == key);
			ASSERT(dbRecords.size() == records.size());
			for (auto & r1x: records) {
				TestRecord * r1 = dynamic_cast<TestRecord*>(r1x);
				ASSERT(r1 != nullptr);
				bool exists = false;
				for (auto r2x: dbRecords) {
					if (r2x == nullptr) continue;
					TestRecord const* r2 = dynamic_cast<TestRecord const*>(r2x);
					ASSERT(r2 != nullptr);
					if (r1->fData[0] == r2->fData[0] && r1->fData[1] == r2->fData[1]) {
						exists = true;
						break;
					}
				}
				if (! exists) {
					std::cerr << "Record does not exists: " << *r1 << std::endl;
					++omittedRecords;
				}
			}
		};

		db->readRecords(records, funcRead);
		std::cout << "Total time[sec]: " << timer.get_time_sec() << std::endl;
		std::cout << "Number of omitted records: " << omittedRecords << std::endl;
		if (omittedRecords) std::cerr << "ERROR" << std::endl;
		else std::cerr << "OK" << std::endl;
	}

	delete db;
	std::cout << "Database object deleted" << std::endl;
	delete tm;
	std::cout << "Scheduler deleted" << std::endl;
	return 0;
}
