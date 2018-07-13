#ifndef FLATDB_SCHEDULER_HPP_
#define FLATDB_SCHEDULER_HPP_

#include "../apiDb/TasksManager.hpp"
#include "Procedure.hpp"
#include "StorageWithCache.hpp"
#include "DataNode.hpp"


namespace flatDb {

	template<typename tKey> class DataNodeT;
	template<typename tKey> class IndexNodeT;

	template<typename tKey>
	class SchedulerT
	{
	public:
		typedef ProcedureT<tKey> Procedure;
		typedef ProcedureUpdateRecordsByKeysT<tKey> ProcedureUpdateRecordsByKeys;
		typedef SubProcedureT<tKey> SubProcedure;
		typedef DataNodeT<tKey> DataNode;
		typedef IndexNodeT<tKey> IndexNode;
		typedef RecordT<tKey> Record;
		typedef BinT<tKey> Bin;
		unsigned const pagesPerIndexNode;
		TasksManager * const cpuTasksManager;
		TasksManager * const ioTasksManager;
		StorageWithCache * const storage;
		typename Record::tCreateRecordFunction const callbackCreateRecord;
	private:
		struct Pim;
		Pim * pim;
	public:
		SchedulerT(unsigned keySize, unsigned pagesPerIndexNode, TasksManager* cpuTM, TasksManager* ioTM, StorageWithCache*, typename Record::tCreateRecordFunction);
		void schedule(Procedure *);
		void scheduleToReorganize(typename DataNode::SP, unsigned priority);
		void reorganizeAndSynchronize();
		tKey getTheLargestKey() const;
		uint64_t getRecordsCount() const;
		void printStatus() const;
	};

}

#endif /* FLATDB_SCHEDULER_HPP_ */
