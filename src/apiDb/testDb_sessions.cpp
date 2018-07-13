#include "db.hpp"
#include "TestRecord.hpp"
#include "../commonTools/assert.hpp"
#include "../commonTools/Stopwatch.hpp"
#include <iostream>
#include <fstream>
#include <array>
#include <algorithm>
#include <thread>
#include <random>
#include <chrono>


typedef RecordT<uint32_t> Record;


struct SessionReadByKeys
{
	static uint64_t const xxxValue;
	static std::default_random_engine generator;
	DatabaseT<> * fDb;
	unsigned const fRecordsPerRegion;
	std::vector<Record*> fRecords;
	std::thread fThread;
	SessionReadByKeys
		( DatabaseT<> * db
		, std::vector<std::array<uint32_t,3>> const & data
		, unsigned totalRecordsCount
		, unsigned recordsCountPerRegion
		, bool evenNumbers = true
		, bool oddNumbers = true
		) : fDb(db), fRecordsPerRegion(recordsCountPerRegion)
	{
		std::uniform_int_distribution<unsigned> distribution(0,data.size()-1);
		fRecords.reserve(totalRecordsCount);
		while (fRecords.size() < totalRecordsCount) {
			unsigned iData = distribution(generator);
			unsigned recordsInRegion = 0;
			for ( ;  iData < data.size();  ++iData ) {
				if (! evenNumbers && data[iData][1] % 2 == 0) continue;
				if (! oddNumbers  && data[iData][1] % 2 == 1) continue;
				fRecords.push_back( new TestRecord(data[iData][0], data[iData][1], data[iData][2]) );
				if (++recordsInRegion >= recordsCountPerRegion) break;
				if (fRecords.size() >= totalRecordsCount) break;
			}
		}
	}
	~SessionReadByKeys()
	{
		if (this->fThread.joinable()) this->fThread.join();
		for (auto r: fRecords) delete r;
	}
	void start()
	{
		uint64_t xxx = xxxValue;
		auto funcRead = [xxx](std::vector<Record const *> const & dbRecords, std::vector<Record*> const & records)
		{
			ASSERT(! records.empty());
			unsigned const key = records.front()->key;
			for (auto r: dbRecords) ASSERT(r->key == key);
			for (auto r: records  ) ASSERT(r->key == key);
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
					std::cerr << "ERROR: Record does not exists: " << *r1 << std::endl;
				} else {
					r1->fData[0] = r1->fData[1] = xxxValue;
				}
			}
		};

		auto funcTest = [this,funcRead]()
		{
			Stopwatch timer;
			fDb->readRecords(fRecords, funcRead);
			std::cout << "Reads=" << fRecords.size() << "/" << fRecordsPerRegion << "; time[sec]=" << timer.get_time_sec() << std::endl;

			for (auto rx: fRecords) {
				TestRecord * r = dynamic_cast<TestRecord*>(rx);
				if (r->fData[0] != xxxValue || r->fData[1] != xxxValue) std::cerr << "ERROR: Omitted record: " << *r << std::endl;
			}
		};

		this->fThread = std::thread(funcTest);
	}
	void join()
	{
		if (this->fThread.joinable()) this->fThread.join();
	}
};


uint64_t const SessionReadByKeys::xxxValue = 1234567890123456789ull;
std::default_random_engine SessionReadByKeys::generator(std::chrono::system_clock::now().time_since_epoch().count());


int main(int argc, char ** argv)
{
	if (argc != 3) {
		std::cout << "Parameters: name of dataset (without extension), name of file with records" << std::endl;
		return 1;
	}

	std::string const dataset = argv[1];
	std::string const filename = argv[2];

	std::vector<std::array<uint32_t,3>> data;

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
				std::cerr << "Error when reading from file after " << data.size() << " records" << std::endl;
				return 2;
			}
			data.push_back(r);
		}
		std::cout << "Number of records: " << data.size() << std::endl;
	}

	// open database
	TasksManager * tm = new TasksManager(4);
	TasksManager * tm2 = new TasksManager(4);
	DatabaseT<> * db = new DatabaseT<>(tm, tm2, dataset, createRecord<TestRecord>);

	// run sessions
	{
		std::vector<SessionReadByKeys*> tests;
		tests.push_back(new SessionReadByKeys( db, data, 1000000, 1 ) );
		tests.push_back(new SessionReadByKeys( db, data, 1000000, 10 ) );
		tests.push_back(new SessionReadByKeys( db, data, 1000000, 100 ) );
		tests.push_back(new SessionReadByKeys( db, data, 1000000, 1000 ) );
		tests.push_back(new SessionReadByKeys( db, data, 1000000, 10000 ) );
		tests.push_back(new SessionReadByKeys( db, data, 1000000, 100000 ) );
		tests.push_back(new SessionReadByKeys( db, data, 1000000, 1000000 ) );
		tests.push_back(new SessionReadByKeys( db, data, 1000, 1 ) );
		tests.push_back(new SessionReadByKeys( db, data, 1000, 10 ) );
		tests.push_back(new SessionReadByKeys( db, data, 1000, 100 ) );
		tests.push_back(new SessionReadByKeys( db, data, 1000, 1000 ) );
		tests.push_back(new SessionReadByKeys( db, data, 1000, 3 ) );
		tests.push_back(new SessionReadByKeys( db, data, 1000, 30 ) );
		tests.push_back(new SessionReadByKeys( db, data, 1000, 300 ) );
		tests.push_back(new SessionReadByKeys( db, data, 10, 10 ) );
		tests.push_back(new SessionReadByKeys( db, data, 10,  9 ) );
		tests.push_back(new SessionReadByKeys( db, data, 10,  8 ) );
		tests.push_back(new SessionReadByKeys( db, data, 10,  7 ) );
		tests.push_back(new SessionReadByKeys( db, data, 10,  6 ) );
		tests.push_back(new SessionReadByKeys( db, data, 10,  5 ) );
		tests.push_back(new SessionReadByKeys( db, data, 10,  4 ) );
		tests.push_back(new SessionReadByKeys( db, data, 10,  3 ) );
		tests.push_back(new SessionReadByKeys( db, data, 10,  2 ) );
		tests.push_back(new SessionReadByKeys( db, data, 10,  1 ) );
		for (auto t: tests) t->start();
		for (auto t: tests) delete t;
	}

	delete db;
	std::cout << "Database object deleted" << std::endl;
	delete tm;
	std::cout << "Scheduler deleted" << std::endl;
	return 0;
}
