#include "TableNamed.hpp"
#include "../commonTools/parseHeader.h"

int main() {
	std::cout << "Test Named Table" << std::endl;
	std::string dir = "dbTest";
	std::string filename = "named";
	TasksManager taskManager1(32);
	TasksManager taskManager2(32);
	Header header = parse(dir+"/"+"named.yaml");
	TableNamed<RecordGenomicVariant> * tab = new TableNamed<RecordGenomicVariant>(dir, filename, header, &taskManager1, &taskManager2, 128);

	// C - create
	std::cout << "create" << std::endl;
	{
		std::vector<RecordGenomicVariant *> varRecords;
		RecordGenomicVariant * rec1 = new RecordGenomicVariant(10); // 10 - coordiante
		rec1->identifiers.lastId() = 5; // 5 - ca id
		varRecords.push_back(rec1);
		RecordGenomicVariant * rec2 = new RecordGenomicVariant(20); // 20 - coordinate
		rec2->identifiers.lastId() = 15; // 15 - ca id
		varRecords.push_back(rec2);
		std::vector<bool> named;
		named.push_back(true);
		named.push_back(false);
		tab->write(varRecords, named);
	}

	// R - read
		// read existing
		std::cout << "read existing" << std::endl;
		{
			std::vector<uint64_t> caids;
			caids.push_back(5);
			caids.push_back(15);
			std::vector<bool> ret = tab->fetch(caids);
			for(unsigned i=0;i<caids.size();i++) {
				std::cout << caids[i] << "\t" << ret[i] << std::endl;
			}
		}

		// read non existing
		std::cout << "read non existing" << std::endl;
		{
			std::vector<uint64_t> caids;
			caids.push_back(6);
			caids.push_back(16);
			std::vector<bool> ret = tab->fetch(caids);
			for(unsigned i=0;i<caids.size();i++) {
				std::cout << caids[i] << "\t" << ret[i] << std::endl;
			}
		}

	// U - update
		// update existing
		std::cout << "update existing" << std::endl;
		{
			std::vector<RecordGenomicVariant *> varRecords;
			RecordGenomicVariant * rec1 = new RecordGenomicVariant(10); // 10 - coordiante
			rec1->identifiers.lastId() = 5; // 5 - ca id
			varRecords.push_back(rec1);
			RecordGenomicVariant * rec2 = new RecordGenomicVariant(20); // 20 - coordinate
			rec2->identifiers.lastId() = 15; // 15 - ca id
			varRecords.push_back(rec2);
			std::vector<bool> named;
			named.push_back(false);
			named.push_back(true);
			tab->write(varRecords, named);
		}
		{
			std::vector<uint64_t> caids;
			caids.push_back(5);
			caids.push_back(15);
			std::vector<bool> ret = tab->fetch(caids);
			for(unsigned i=0;i<caids.size();i++) {
				std::cout << caids[i] << "\t" << ret[i] << std::endl;
			}
		}

		// update non existing
		std::cout << "update non existing" << std::endl;
		{
			std::vector<RecordGenomicVariant *> varRecords;
			RecordGenomicVariant * rec1 = new RecordGenomicVariant(30); // 30 - coordiante
			rec1->identifiers.lastId() = 25; // 25 - ca id
			varRecords.push_back(rec1);
			std::vector<bool> named;
			named.push_back(true);
			tab->write(varRecords, named);
		}
		{
			std::vector<uint64_t> caids;
			caids.push_back(5);
			caids.push_back(15);
			caids.push_back(25);
			std::vector<bool> ret = tab->fetch(caids);
			for(unsigned i=0;i<caids.size();i++) {
				std::cout << caids[i] << "\t" << ret[i] << std::endl;
			}
		}

	// upset
	// exist T => T do nothing
	// exist F => F do nothing
	//   new T => T
	//   new F => T
		std::cout << "upset" << std::endl;
		{
			std::vector<RecordGenomicVariant *> varRecords;
			RecordGenomicVariant * rec1 = new RecordGenomicVariant(10); // 10 - coordiante
			rec1->identifiers.lastId() = 5; // 5 - ca id - exist false
			varRecords.push_back(rec1);
			RecordGenomicVariant * rec2 = new RecordGenomicVariant(20); // 20 - coordinate
			rec2->identifiers.lastId() = 15; // 15 - ca id - exist true
			varRecords.push_back(rec2);
			RecordGenomicVariant * rec3 = new RecordGenomicVariant(40); // 40 - coordinate
			rec3->identifiers.lastId() = 45; // 45 - ca id - new neither true or false yet, expected to set true
			varRecords.push_back(rec3);
			tab->upset(varRecords);
		}
		{
			std::vector<uint64_t> caids;
			caids.push_back(5);
			caids.push_back(15);
			caids.push_back(45);
			std::vector<bool> ret = tab->fetch(caids);
			for(unsigned i=0;i<caids.size();i++) {
				std::cout << caids[i] << "\t" << ret[i] << std::endl;
			}
		}

	// downset
	// exist T => F
	// exist F => F
	//   new T => T do nothing
	//   new F => F do nothing
		std::cout << "downset" << std::endl;
		{
			std::vector<RecordGenomicVariant *> varRecords;
			RecordGenomicVariant * rec1 = new RecordGenomicVariant(10); // 10 - coordiante
			rec1->identifiers.lastId() = 5; // 5 - ca id - exist false, set to false
			varRecords.push_back(rec1);
			RecordGenomicVariant * rec2 = new RecordGenomicVariant(20); // 20 - coordinate
			rec2->identifiers.lastId() = 15; // 15 - ca id - exist true, set to false
			varRecords.push_back(rec2);
			RecordGenomicVariant * rec3 = new RecordGenomicVariant(50); // 50 - coordinate
			rec3->identifiers.lastId() = 55; // 55 - ca id - new neither true or false yet, do nothing, dont register, effectively set to false
			varRecords.push_back(rec3);
			tab->downset(varRecords);
		}
		{
			std::vector<uint64_t> caids;
			caids.push_back(5);
			caids.push_back(15);
			caids.push_back(55);
			std::vector<bool> ret = tab->fetch(caids);
			for(unsigned i=0;i<caids.size();i++) {
				std::cout << caids[i] << "\t" << ret[i] << std::endl;
			}
		}

	// D - delete
		// delete existing
		std::cout << "update delete existing" << std::endl;
		{
			std::vector<RecordGenomicVariant *> varRecords;
			RecordGenomicVariant * rec1 = new RecordGenomicVariant(10); // 10 - coordiante
			rec1->identifiers.lastId() = 5; // 5 - ca id
			varRecords.push_back(rec1);
			RecordGenomicVariant * rec2 = new RecordGenomicVariant(20); // 20 - coordinate
			rec2->identifiers.lastId() = 15; // 15 - ca id
			varRecords.push_back(rec2);
			RecordGenomicVariant * rec3 = new RecordGenomicVariant(30); // 30 - coordinate
			rec3->identifiers.lastId() = 25; // 25 - ca id
			varRecords.push_back(rec3);
			tab->deleteRecords(varRecords);
		}
		{
			std::vector<uint64_t> caids;
			caids.push_back(5);
			caids.push_back(15);
			caids.push_back(25);
			std::vector<bool> ret = tab->fetch(caids);
			for(unsigned i=0;i<caids.size();i++) {
				std::cout << caids[i] << "\t" << ret[i] << std::endl;
			}
		}

		// delete non existing
		std::cout << "update delete non existing" << std::endl;
		{
			std::vector<RecordGenomicVariant *> varRecords;
			RecordGenomicVariant * rec1 = new RecordGenomicVariant(10); // 10 - coordiante
			rec1->identifiers.lastId() = 5; // 5 - ca id
			varRecords.push_back(rec1);
			tab->deleteRecords(varRecords);
		}
		{
			std::vector<uint64_t> caids;
			caids.push_back(5);
			caids.push_back(15);
			caids.push_back(25);
			std::vector<bool> ret = tab->fetch(caids);
			for(unsigned i=0;i<caids.size();i++) {
				std::cout << caids[i] << "\t" << ret[i] << std::endl;
			}
		}

	return 0;
}
