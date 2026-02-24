#ifndef TABLENAMED_HPP_
#define TABLENAMED_HPP_

#include <string>
#include <vector>
#include "RecordVariant64.hpp"
#include "../commonTools/Header.hpp"

template<typename RecordType>
class TableNamed {
private:
	struct Pim;
	Pim * pim;
public:
	TableNamed(std::string dirPath, std::string fileName, Header header, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB);
	~TableNamed();
	std::vector<bool> fetch(std::vector<uint64_t> const & caIds) const;
	void write(std::vector<RecordType *> const & varRecords, std::vector<bool> const & named);
	void set(std::vector<RecordType *> const & varRecords);
	void unset(std::vector<RecordType *> const & varRecords);
	void upset(std::vector<RecordType *> const & varRecords);
	void upset(std::vector<RecordType const *> const & varRecords);
	void downset(std::vector<RecordType *> const & varRecords);
	uint64_t getMaxIdentifier() const;
	void deleteRecords(std::vector<RecordType *> & varRecords);
	bool isNewDb() const;
};

#endif
