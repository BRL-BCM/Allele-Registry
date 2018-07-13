#include "DataNode.hpp"
#include "Scheduler.hpp"
#include "../commonTools/assert.hpp"
#include "../commonTools/bytesLevel.hpp"


#define XX DataNodeT<tKey>


namespace flatDb {


	template<typename tKey>
	std::vector<std::pair<tKey,uint8_t const*>> XX::readRawRecordsFromPage() const
	{
		std::vector<std::pair<tKey,uint8_t const*>> target;
		target.reserve( bin.recordsCount );
		uint8_t const * ptr = fRawData;
		unsigned const bytesPerKey = bin.bytesPerKey();
		for (unsigned i = 0; i < bin.recordsCount; ++i) {
			// read the key
			tKey const key = bin.firstKey + readUnsignedInteger<tKey>(ptr, bytesPerKey);
			// save the record
			target.push_back( std::make_pair(key,ptr) );
			// read the record data size
			unsigned const recordSize = readUnsignedIntVarSize<1,1,unsigned>(ptr);
			// skip the record
			ptr += recordSize;
		}
		return target;
	}


	template<typename tKey>
	typename XX::SP XX::createEmpty(Scheduler* scheduler, tKey firstKey)
	{
		Bin b;
		b.firstKey = firstKey;
		b.maxKeyOffset = 0;
		b.bytesCount = 0;
		b.recordsCount = 0;

		unsigned const pageId = scheduler->storage->allocatePages(1).front();

		// ----- create object
		SP obj(new DataNodeT<tKey>(scheduler,b,pageId));
		obj->fThis = obj;
		return obj;
	}


	template<typename tKey>
	typename XX::SP XX::createNew(Scheduler* scheduler, std::vector<std::pair<tKey,uint8_t const*>> & records)
	{
		ASSERT( ! records.empty() );

		Bin b;
		b.firstKey = records.begin()->first;
		b.maxKeyOffset = records.rbegin()->first - b.firstKey;
		b.recordsCount = records.size();
		b.bytesCount = 0;

		// ----- allocate buffer
		unsigned const pageId = scheduler->storage->allocatePages(1).front();
		uint8_t* ptr;
		scheduler->storage->lockPage_createEmpty(pageId, ptr);

		// ----- save data to buffer (and free records)
		unsigned const bytesPerKey = b.bytesPerKey();
		for (auto const & kv: records) {
			ASSERT( kv.second != nullptr );
			tKey const keyOffset = kv.first - b.firstKey;
			uint8_t const* src = kv.second;
			unsigned const recordLength = readUnsignedIntVarSize<1,1,unsigned>(src);
			b.bytesCount += recordLength + (src - kv.second);
			writeUnsignedInteger(ptr, keyOffset, bytesPerKey);
			writeUnsignedIntVarSize<1,1,unsigned>(ptr, recordLength);
			std::memcpy(ptr, src, recordLength);
			ptr += recordLength;
		}
		records.clear();

		// ----- send page to storage and unlock it from the cache
		scheduler->storage->savePageToStorage(pageId);
		scheduler->storage->unlockPage(pageId);

		// ----- create object
		SP obj(new DataNodeT<tKey>(scheduler,b,pageId));
		obj->fThis = obj;
		return obj;
	}


	template<typename tKey>
	typename XX::SP XX::createFromStorage(Scheduler* scheduler, unsigned pageId, typename XX::Bin bin)
	{
		SP obj(new DataNodeT<tKey>(scheduler,bin,pageId));
		obj->fThis = obj;
		return obj;
	}


	template<typename tKey>
	XX::~DataNodeT()
	{
		std::lock_guard<std::mutex> lockGuard(fAccessToDataNode);
		if (fRawData != nullptr) scheduler->storage->unlockPage(pageId);
		if (fState == DataState::obsolete) scheduler->storage->releasePages(std::vector<unsigned>(1,pageId));
	}


	template<typename tKey>
	void XX::schedule(typename XX::SubProcedure::SP subproc)
	{
		bool const isUpdate = ! subproc->isReadOnly();

		std::lock_guard<std::mutex> lockGuard(fAccessToDataNode);

		// ===== add subprocedure
		bool requireRawData = false;
		if (isUpdate) {
			ASSERT( fState == DataState::unmodified || fState == DataState::modified );
			fUpdatesToDo.push_back(subproc);
			requireRawData = (fState == DataState::unmodified);
		} else {
			fReadsToDo.push_back(subproc);
			requireRawData = true;
		}

		// ===== detect case, when IO task is needed
		if ( requireRawData ) {
			switch (fCacheState) {
			case CacheState::notCached:
				if (scheduler->storage->lockPage_loadFromCache(pageId, fRawData)) {
					fCacheState = CacheState::cached;
				} else {
					fIoPriority = subproc->priority;
					// TODO scheduler->ioTasksManager->updatePriority(fIoTaskId, fIoPriority);
					SP thisSP = fThis.lock();
					auto readTask = [thisSP]()->void { thisSP->read(); };
					fIoTaskId = scheduler->ioTasksManager->addTask(readTask, fIoPriority);
					fCacheState = CacheState::scheduledForRead;
					return;
				}
				break;
			case CacheState::scheduledForRead:
				if (fIoPriority < subproc->priority) {
					fIoPriority = subproc->priority;
					// TODO scheduler->ioTasksManager->updatePriority(fIoTaskId, fIoPriority);
					SP thisSP = fThis.lock();
					auto readTask = [thisSP]()->void { thisSP->read(); };
					fIoTaskId = scheduler->ioTasksManager->addTask(readTask, fIoPriority);
				}
				return;
			case CacheState::duringReadExecution:
				return;
			case CacheState::cached:
				break;
			}
		}

		// ===== add or update CPU task
		switch (fTasksState) {
		case TasksState::noTasks:
			{
				// create new CPU task
				fCpuPriority = subproc->priority;
				SP thisSP = fThis.lock();
				auto processTask = [thisSP]()->void { thisSP->process(); };
				fCpuTaskId = scheduler->cpuTasksManager->addTask(processTask,fCpuPriority);
			}
			fTasksState = TasksState::scheduledForProcessing;
			break;
		case TasksState::scheduledForProcessing:
			// update priority if needed
			if (fCpuPriority < subproc->priority) {
				fCpuPriority = subproc->priority;
				// TODO scheduler->cpuTasksManager->updatePriority(fCpuTaskId, fCpuPriority);
				SP thisSP = fThis.lock();
				auto processTask = [thisSP]()->void { thisSP->process(); };
				fCpuTaskId = scheduler->cpuTasksManager->addTask(processTask,fCpuPriority);
			}
			break;
		case TasksState::duringReadOnlyProcessing:
		case TasksState::duringUpdateProcessing:
			break;
		}
	}


	template<typename tKey>
	void XX::read()
	{
		{
			std::lock_guard<std::mutex> lockGuard(fAccessToDataNode);
			if (fCacheState != CacheState::scheduledForRead) return; // this task is no longer needed
			fCacheState = CacheState::duringReadExecution;
		}

		// ===== read data from storage
		uint8_t * page;
		if ( ! scheduler->storage->lockPage_loadFromStorage( pageId, page ) ) {
			// ---- schedule reorganize to free cache
			scheduler->scheduleToReorganize(nullptr, 1000); // TODO - hardcoded value
			// ---- reschedule read() task
			fCacheState = CacheState::scheduledForRead;
			SP thisSP = fThis.lock();
			auto readTask = [thisSP]()->void { thisSP->read(); };
			fIoTaskId = scheduler->ioTasksManager->addTask(readTask, fIoPriority);
			return;
		}

		{
			std::lock_guard<std::mutex> lockGuard(fAccessToDataNode);
			ASSERT( fCacheState == CacheState::duringReadExecution );

			// ===== set page
			fRawData = page;
			fCacheState = CacheState::cached;

			// ===== add or update CPU task
			switch (fTasksState) {
			case TasksState::noTasks:
				{
					// create new CPU task
					fCpuPriority = fIoPriority;
					SP thisSP = fThis.lock();
					auto processTask = [thisSP]()->void { thisSP->process(); };
					fCpuTaskId = scheduler->cpuTasksManager->addTask(processTask,fCpuPriority);
				}
				fTasksState = TasksState::scheduledForProcessing;
				break;
			case TasksState::scheduledForProcessing:
				// update priority if needed
				if (fCpuPriority < fIoPriority) {
					fCpuPriority = fIoPriority;
					// TODO scheduler->cpuTasksManager->updatePriority(fCpuTaskId, fCpuPriority);
					SP thisSP = fThis.lock();
					auto processTask = [thisSP]()->void { thisSP->process(); };
					fCpuTaskId = scheduler->cpuTasksManager->addTask(processTask,fCpuPriority);
				}
				break;
			case TasksState::duringReadOnlyProcessing:
			case TasksState::duringUpdateProcessing:
				break;
			}
		}
	}


	template<typename tKey>
	void XX::process()
	{
//DebugScope("DataNode::process")
		{ // ----- update TasksState
			std::lock_guard<std::mutex> lockGuard(fAccessToDataNode);
			if (fTasksState != TasksState::scheduledForProcessing) return; // obsolete task
			if (fUpdatesToDo.empty()) {
				fTasksState = TasksState::duringReadOnlyProcessing;
			} else {
				fTasksState = TasksState::duringUpdateProcessing;
			}
		}

		std::vector<std::pair<tKey,uint8_t const*>> dataRaw;
		unsigned reorganizePriority = 0;
		unsigned wasModified = false;
		auto funcAllocateBuffer = [this](unsigned size)->uint8_t*{ return this->fMemoryForNewContent.allocateBuffer(size); };

		while (true) {
			std::vector<typename SubProcedure::SP> reads;
			std::vector<typename SubProcedure::SP> updates;

			{ // ----- grab subprocedures to process, exit if there is no more to do
				std::lock_guard<std::mutex> lockGuard(fAccessToDataNode);

				TasksState newTasksState = TasksState::noTasks;

				if ( ! fReadsToDo.empty() && fCacheState == CacheState::cached ) {
					newTasksState = TasksState::duringReadOnlyProcessing;
					fReadsToDo.swap(reads);
				}

				if ( ! fUpdatesToDo.empty()
						&& (fCacheState == CacheState::cached || fState == DataState::modified || wasModified) ) {
					newTasksState = TasksState::duringUpdateProcessing;
					fUpdatesToDo.swap(updates);
				}

				if (fTasksState == TasksState::duringUpdateProcessing  && newTasksState != fTasksState) {
					// schedule data node to reorganize if needed
					if ( wasModified && ( fState == DataState::unmodified || fReorganizePriority < reorganizePriority ) ) {
						ASSERT( fState == DataState::unmodified || fState == DataState::modified );
						fState = DataState::modified;
						fReorganizePriority = reorganizePriority;
						scheduler->scheduleToReorganize(fThis.lock(), fReorganizePriority);
					}
					wasModified = false;
				}

				fTasksState = newTasksState;

				if ( fTasksState == TasksState::noTasks ) {
					// data in cache are not needed now
					if ( fCacheState == CacheState::cached && fState == DataState::unmodified ) {
						scheduler->storage->unlockPage(pageId);
						fRawData = nullptr;
						fCacheState = CacheState::notCached;
					}
					// exit
					break;
				}
			}

			// ----- process read subprocedures
			if ( ! reads.empty() ) {
				if (dataRaw.empty()) {
					dataRaw = readRawRecordsFromPage();
				}
				for (auto sp: reads) {
					sp->process(dataRaw);
				}
			}

			// ----- process update subprocedures
			if ( ! updates.empty() ) {
				// prepare fNewContent is needed
				if ( ! (wasModified || fState == DataState::modified) ) {
					if (dataRaw.empty()) {
						dataRaw = readRawRecordsFromPage();
					}
					fNewContent.swap(dataRaw);
				}
				// execute procs
				for (auto sp: updates) {
					if ( sp->process(fNewContent, funcAllocateBuffer) ) {
						wasModified = true;
						fWaitingForCommit.push_back(sp);
						reorganizePriority = std::max(reorganizePriority, sp->priority);
					}
				}
				// return fNewContent to data if possible (when there is no changes)
				if ( ! (wasModified || fState == DataState::modified) ) {
					fNewContent.swap(dataRaw);
				}
			}
		}  // end of while true

	}


	template<typename tKey>
	bool XX::tryToPrepareForReorganize(std::vector<std::pair<tKey,uint8_t const*>> & out)
	{
		std::lock_guard<std::mutex> lockGuard(fAccessToDataNode);
		ASSERT( fState == DataState::unmodified || fState == DataState::modified );
		// there is updates pending, return false
		if ( fTasksState == TasksState::duringUpdateProcessing || ! fUpdatesToDo.empty() ) return false;
		// if it has no new content, we have to load data from storage
		if ( fState == DataState::unmodified ) {
			// try to load data from cache, if it is missing then exit (we do not want IO operation here)
			uint8_t * ptr;
			switch (fCacheState) {
			case CacheState::duringReadExecution:
			case CacheState::scheduledForRead:
				return false;
			case CacheState::notCached:
				if ( ! scheduler->storage->lockPage_loadFromCache(pageId,ptr) ) return false;
				fNewContent = readRawRecordsFromPage();
				fCacheState = CacheState::cached;
				break;
			case CacheState::cached:
				fNewContent = readRawRecordsFromPage();
				break;
			}
			// data was successfully loaded, now the data node may be treated as "modified"
			fState = DataState::modified;
		}
		// move new content to out
		out.insert(out.end(), fNewContent.begin(), fNewContent.end());
		fNewContent.clear();
		fNewContent.shrink_to_fit();
		// mark data node as reorganized
		fState = DataState::reorganized;
		return true;
	}


	template<typename tKey>
	void XX::freeMemory()
	{
		std::lock_guard<std::mutex> lockGuard(fAccessToDataNode);
		ASSERT( fUpdatesToDo.empty() );
		ASSERT( fNewContent.empty() );
		ASSERT( fTasksState != TasksState::duringUpdateProcessing );
		ASSERT( fState == DataState::reorganized );
		fMemoryForNewContent.clear();
		if ( fTasksState == TasksState::noTasks && fCacheState == CacheState::cached ) {
			scheduler->storage->unlockPage(pageId);
			fCacheState = CacheState::notCached;
		}
	}


	template<typename tKey>
	void XX::markAsObsolete()
	{
		std::lock_guard<std::mutex> lockGuard(fAccessToDataNode);
		ASSERT( fUpdatesToDo.empty() );
		ASSERT( fNewContent.empty() );
		ASSERT( fTasksState != TasksState::duringUpdateProcessing );
		ASSERT( fState == DataState::reorganized );
		fWaitingForCommit.clear();
		fState = DataState::obsolete;
	}


	template class DataNodeT<uint32_t>;
	template class DataNodeT<uint64_t>;
}
