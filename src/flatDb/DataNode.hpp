#ifndef FLATDB_DATANODE_HPP_
#define FLATDB_DATANODE_HPP_

#include <vector>
#include <map>
#include <stack>
#include <memory>
#include <mutex>
#include "Bin.hpp"
#include "SubProcedure.hpp"
#include "../commonTools/assert.hpp"

namespace flatDb {


	template<typename tKey>
	class SchedulerT;


	class MemoryManager
	{
	private:
		std::stack<uint8_t*> fChunks;
		uint8_t* fPtr = nullptr;
	public:
		unsigned const chunkSize;
		MemoryManager(unsigned pChunkSize) : chunkSize(pChunkSize) {}
		void clear() { for ( ; ! fChunks.empty(); fChunks.pop() ) delete [] fChunks.top(); fPtr = nullptr; }
		~MemoryManager() { clear(); }
		uint8_t* allocateBuffer(unsigned const size)
		{
			ASSERT(size <= chunkSize);
			if ( fPtr == nullptr || chunkSize - (fPtr - fChunks.top()) < size ) {
				fPtr = new uint8_t[chunkSize];
				fChunks.push(fPtr);
			}
			uint8_t* r = fPtr;
			fPtr += size;
			return r;
		}
	};


	template<typename tKey>
	class DataNodeT
	{
	public:
		typedef std::shared_ptr<DataNodeT<tKey>> SP;
		typedef SubProcedureT<tKey> SubProcedure;
		typedef SubProcedureReadRecordsByKeysT<tKey> SubProcedureReadRecordsByKeys;
		typedef SubProcedureReadRecordsFromRangeT<tKey> SubProcedureReadRecordsFromRange;
		typedef SubProcedureUpdateRecordsByKeysT<tKey> SubProcedureUpdateRecordsByKeys;
		typedef RecordT<tKey> Record;
		typedef BinT<tKey> Bin;
		typedef SchedulerT<tKey> Scheduler;
		Scheduler* const scheduler;
		Bin const bin;
		unsigned const pageId;        // page id in the file
	private:
		MemoryManager fMemoryForNewContent;
		std::weak_ptr<DataNodeT<tKey>> fThis;
		std::mutex fAccessToDataNode;
		enum class DataState
		{
			  unmodified     // no modifications
			, modified       // there is a new content
			, reorganized    // new node was created and the new content was moved to it
			, obsolete       // new content was committed, this node will be deallocated in destructor
		} fState = DataState::unmodified;
		enum class CacheState
		{
			  notCached
			, scheduledForRead
			, duringReadExecution
			, cached
		} fCacheState = CacheState::notCached;
		enum class TasksState
		{
			  noTasks
			, scheduledForProcessing
			, duringUpdateProcessing
			, duringReadOnlyProcessing
		} fTasksState = TasksState::noTasks;
		unsigned fIoTaskId = 0;
		unsigned fCpuTaskId = 0;
		unsigned fIoPriority = 0;
		unsigned fCpuPriority = 0;
		unsigned fReorganizePriority = 0;
		uint8_t * fRawData = nullptr;
		// subprocedures to process
		std::vector<typename SubProcedure::SP> fReadsToDo;
		std::vector<typename SubProcedure::SP> fUpdatesToDo;
		// processed subprocedures waiting for reorganization
		std::vector<typename SubProcedure::SP> fWaitingForCommit;
		// modified content of the DataNode
		std::vector<std::pair<tKey,uint8_t const*>> fNewContent; // organized by keys
		std::vector<std::pair<tKey,uint8_t const*>> readRawRecordsFromPage() const;
		DataNodeT(Scheduler * pScheduler, Bin const & pBin, unsigned pPageId)
		: scheduler(pScheduler), bin(pBin), pageId(pPageId), fMemoryForNewContent(64*1024) {} // TODO - should depend on data page size?
	public:
		// constructors
		static SP createEmpty(Scheduler*, tKey firstKey);
		static SP createNew(Scheduler*, std::vector<std::pair<tKey,uint8_t const*>> & records); // records are deleted !
		static SP createFromStorage(Scheduler*, unsigned pageId, Bin bin);
		// destructor - synchronized
		~DataNodeT();
		// synchronized access
		void schedule(typename SubProcedure::SP);
		void process();
		void read();
		bool tryToPrepareForReorganize(std::vector<std::pair<tKey,uint8_t const*>> & out); // move current content to out (append it to out, it is deleted from the object)
		void freeMemory();
		void markAsObsolete();
	};

}

#endif /* FLATDB_DATANODE_HPP_ */
