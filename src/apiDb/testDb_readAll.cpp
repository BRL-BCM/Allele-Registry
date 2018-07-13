#include "db.hpp"
#include "TestRecord.hpp"
#include <iostream>
#include <fstream>
#include <array>

void funcRead(std::vector<RecordT<uint32_t> const *> const & records, bool)
{
	for (auto r2: records) {
		TestRecord const * r = dynamic_cast<TestRecord const *>(r2);
		std::cout << *r << std::endl;
	}
}

int main(int argc, char ** argv)
{
	if (argc != 2) {
		std::cout << "Parameters: name of dataset (without extension)" << std::endl;
		return 1;
	}

	std::string dataset = argv[1];

	// create database
	TasksManager * tm = new TasksManager(4);
	TasksManager * tm2 = new TasksManager(4);
	DatabaseT<> * db = new DatabaseT<>(tm, tm2, dataset, createRecord<TestRecord>);

	// read database
	{
		std::cout << "Read records from database:" << std::endl;
		db->readRecordsInOrder(funcRead);
	}

	delete db;
	std::cout << "Database object deleted" << std::endl;
	delete tm;
	std::cout << "Scheduler deleted" << std::endl;
	return 0;
}
