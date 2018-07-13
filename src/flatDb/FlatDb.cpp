#include "../commonTools/Stopwatch.hpp"
#include "../apiDb/db.hpp"
#include "IndexNode.hpp"
#include "Procedure.hpp"
#include "Scheduler.hpp"
#include "StorageWithCache.hpp"
#include <map>
#include <iostream>


#define XX DatabaseT<tKey, globalKeySize, unusedVar>
#define templateXX template<typename tKey, unsigned const globalKeySize, unsigned unusedVar>


	templateXX
	struct XX::Pim
	{
		flatDb::SchedulerT<tKey> * scheduler;
		flatDb::StorageWithCache * storage;
		tCreateRecord callbackCreateRecord;
		bool newDatabaseWasCreated;
	};

	struct ScopeTimesLogger {
		flatDb::StorageWithCache* fStorage;
		std::string fPrefix;
		Stopwatch fStopwatch;
		ScopeTimesLogger(flatDb::StorageWithCache* storage, std::string const & prefix)
		: fStorage(storage), fPrefix(prefix)
		{
			fStorage->readAndResetStatistics();
			fStopwatch.reset_and_restart();
		}
		~ScopeTimesLogger()
		{
//			typename flatDb::StorageWithCache::Statistics stat = fStorage->readAndResetStatistics();
//			std::cout << fPrefix << ": total=" << fStopwatch.get_time_ms();
//			std::cout << " reads="  << stat.readsTimeMs  << "(" << stat.readsCount  << ")";
//			std::cout << " writes=" << stat.writesTimeMs << "(" << stat.writesCount << ")";
//			std::cout << " flush="  << stat.synchTimeMs  << "(" << stat.synchCount  << ")" << std::endl;
		}
	};

	static unsigned calcPriority(unsigned const recordsCount)
	{
		if (recordsCount <=   10) return 400;
		if (recordsCount <=  100) return 300;
		if (recordsCount <= 1000) return 200;
		return 100;
	}


	templateXX
	XX::DatabaseT
		( TasksManager * cpuTaskManager
		, TasksManager * ioTaskManager
		, std::string const & dbFile
		, tCreateRecord funcLoadData
		, unsigned cacheSizeInMegabytes
		)
	: pim(new Pim)
	{
		pim->callbackCreateRecord = funcLoadData;
		pim->storage = new flatDb::StorageWithCache(dbFile, 256*1024, cacheSizeInMegabytes);  // page size = 256 KB
		pim->newDatabaseWasCreated = (pim->storage->numberOfPages() == 0);
		pim->scheduler = new flatDb::SchedulerT<tKey>( globalKeySize, 8, cpuTaskManager, ioTaskManager, pim->storage, pim->callbackCreateRecord ); // index page size = 2 MB
	}


	templateXX
	XX::~DatabaseT()
	{
		// TODO - how to check the list of existing tasks ?
	}


	templateXX
	void XX::readRecordsInOrder(tReadFunction visitor, tKey first, tKey last, unsigned hintQuerySize) const
	{
		ScopeTimesLogger logger(pim->storage, "readRecordsInOrder");
		typedef flatDb::ProcedureReadRecordsFromRangeT<tKey> Proc;
		Proc * proc = new Proc(pim->scheduler, calcPriority(hintQuerySize), visitor, first, last);
		pim->scheduler->schedule(proc);
		proc->waitUntilCompleted();
		delete proc;
	}


	templateXX
	void XX::readRecords(std::vector<Record*> const & pRecords, tReadByKeyFunction visitor) const
	{
		ScopeTimesLogger logger(pim->storage, "readRecords");
		typedef flatDb::ProcedureReadRecordsByKeysT<tKey> Proc;
		Proc * proc = new Proc(pim->scheduler, calcPriority(pRecords.size()), visitor, pRecords);
		pim->scheduler->schedule(proc);
		proc->waitUntilCompleted();
		delete proc;
	}


	templateXX
	void XX::writeRecords(std::vector<Record*> const & pRecords, tUpdateByKeyFunction visitor)
	{
		ScopeTimesLogger logger(pim->storage, "writeRecords");
		typedef flatDb::ProcedureUpdateRecordsByKeysT<tKey> Proc;
		Proc * proc = new Proc(pim->scheduler, calcPriority(pRecords.size()), visitor, pRecords);
		pim->scheduler->schedule(proc);
		proc->waitUntilCompleted();
		delete proc;
//std::this_thread::yield();
//std::this_thread::sleep_for(std::chrono::seconds(1));
//std::this_thread::yield();
//pim->scheduler->printStatus();
	}


	templateXX
	tKey XX::getTheLargestKey() const
	{
		return pim->scheduler->getTheLargestKey();
	}

	templateXX
	uint64_t XX::getRecordsCount() const
	{
		return pim->scheduler->getRecordsCount();
	}

	templateXX
	bool XX::isNewDb() const
	{
		return pim->newDatabaseWasCreated;
	}

	template class DatabaseT<uint32_t,4,8192>;
	template class DatabaseT<uint64_t,8,8192>;



