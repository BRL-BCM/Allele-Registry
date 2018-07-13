#include "Scheduler.hpp"
#include "IndexNode.hpp"
#include "../commonTools/assert.hpp"
#include <mutex>
#include <set>


#define XX SchedulerT<tKey>


namespace flatDb {


	template<typename tKey>
	struct XX::Pim
	{
		typename IndexNode::SP committedDb;
		typename IndexNode::SP currentDb;
		std::mutex accessCommittedDb;
		std::mutex accessCurrentDb;
		std::map<tKey,unsigned> reorganizeFirstKeys;  // firstKey -> priority
		std::mutex              reorganizeAccess;
		unsigned                reorganizePriority = 0;
		enum class SynchState
		{
			  noSynchTask
			, synchTaskScheduled
			, duringSynchTaskExecution
		} synchState = SynchState::noSynchTask;
		uint8_t * bufferForIndexNode = nullptr;
		Pim(unsigned indexNodeSize) : bufferForIndexNode(new uint8_t[indexNodeSize]) {}
		~Pim() { delete [] bufferForIndexNode; }
	};


	template<typename tKey>
	XX::SchedulerT(unsigned keySize, unsigned pPagesPerIndexNode, TasksManager* cpuTM, TasksManager* ioTM, StorageWithCache* pStorage, typename Record::tCreateRecordFunction callback)
	: pagesPerIndexNode(pPagesPerIndexNode), cpuTasksManager(cpuTM), ioTasksManager(ioTM), storage(pStorage), callbackCreateRecord(callback)
	{
		pim = new Pim( pagesPerIndexNode * storage->pageSize );
		typename IndexNode::SP indexNode;
		unsigned const numberOfPages = storage->numberOfPages();
		if ( numberOfPages  >= 2*pagesPerIndexNode ) {
			// ----- load index
			typename IndexNode::SP indexNode2;
			storage->readPages(0, pagesPerIndexNode, pim->bufferForIndexNode);
			try {
				indexNode = IndexNode::createFromBuffer(this, keySize, storage->pageSize, pagesPerIndexNode*storage->pageSize, true, pim->bufferForIndexNode);
			} catch (...) {}
			storage->readPages(pagesPerIndexNode, pagesPerIndexNode, pim->bufferForIndexNode);
			try {
				indexNode2 = IndexNode::createFromBuffer(this, keySize, storage->pageSize, pagesPerIndexNode*storage->pageSize, false, pim->bufferForIndexNode);
			} catch (...) {}
			if (! indexNode) indexNode.swap(indexNode2);
			if (! indexNode) throw std::runtime_error("Cannot find correct index page!!!");
			if ( indexNode2 && ( indexNode->revision < indexNode2->revision
						|| indexNode->revision - indexNode2->revision > std::numeric_limits<unsigned>::max()/2 ) )
			{
				indexNode.swap(indexNode2);
			}
			// ----- find unused data pages
			std::vector<bool> usedDataPages(numberOfPages,false);
			for ( typename DataNode::SP dn: indexNode->entries ) {
				usedDataPages[dn->pageId] = true;
			}
			std::map<unsigned, unsigned> freePages;  // id -> size
			bool freePage = false;
			for (unsigned i = 2*pagesPerIndexNode; i < numberOfPages; ++i) {
				if (usedDataPages[i]) {
					freePage = false;
				} else {
					if (freePage) {
						++(freePages.rbegin()->second);
					} else {
						freePage = true;
						freePages[i] = 1;
					}
				}
			}
			storage->setFreePages(numberOfPages, freePages);
		} else if ( numberOfPages == 0 ) {
			std::vector<unsigned> pages = storage->allocatePages(2*pagesPerIndexNode);
			ASSERT( pages.at(0) == 0 );
			ASSERT( pages.at(pagesPerIndexNode) == pagesPerIndexNode );
			indexNode = IndexNode::createEmpty(this, keySize, storage->pageSize, pagesPerIndexNode*storage->pageSize);
			indexNode->entries.push_back( DataNode::createEmpty(this,0) );
			indexNode->writeToBuffer( pim->bufferForIndexNode );
			storage->writePages( (indexNode->firstCopy ? 0 : pagesPerIndexNode), pagesPerIndexNode, pim->bufferForIndexNode );  // TODO - combine with flush ?
			storage->flush();
		} else {
			throw std::runtime_error("Incorrect file size");
		}
		pim->committedDb = indexNode;
		pim->currentDb = pim->committedDb->createSecondCopy();
	}


	template<typename tKey>
	void XX::schedule(XX::Procedure * proc)
	{
		typename IndexNode::SP indexNode;
		bool const isReadOnly = ( dynamic_cast<XX::ProcedureUpdateRecordsByKeys*>(proc) == nullptr );

		// ===== get proper index node
		if ( isReadOnly ) {
			// it is read only procedure
			std::lock_guard<std::mutex> guard(pim->accessCommittedDb);
			indexNode = pim->committedDb;
		} else {
			// it is update procedure
			pim->accessCurrentDb.lock();
			indexNode = pim->currentDb;
		}

		// ===== create subprocedures
		std::vector<Bin> bins;
		bins.reserve(indexNode->entries.size());
		for (auto dn: indexNode->entries) bins.push_back( dn->bin );
		std::map<unsigned,typename SubProcedure::SP> subprocs = proc->createSubProcedures(bins);
		for (auto & kv: subprocs) {
			typename DataNode::SP dataNode = indexNode->entries[kv.first];
			typename SubProcedure::SP subproc = kv.second;
			dataNode->schedule(subproc);
		}

		// ===== unlock resources
		if ( ! isReadOnly ) {
			pim->accessCurrentDb.unlock();
		}
	}


	template<typename tKey>
	void XX::scheduleToReorganize(typename DataNode::SP dataNode, unsigned priority)
	{
//DebugScope("scheduleToReorganize");
		std::lock_guard<std::mutex> guard(pim->reorganizeAccess);

		if (dataNode != nullptr) {
			pim->reorganizeFirstKeys[dataNode->bin.firstKey] = std::max(priority, pim->reorganizeFirstKeys[dataNode->bin.firstKey]);
		} else if ( pim->reorganizeFirstKeys.empty() ) {
			return;
		}

		if ( pim->synchState == Pim::SynchState::noSynchTask
				|| (pim->synchState == Pim::SynchState::synchTaskScheduled && priority > pim->reorganizePriority) )
		{
			pim->reorganizePriority = priority;
			auto task = [this]()->void{ this->reorganizeAndSynchronize(); };
			cpuTasksManager->addTask(task, priority);
			pim->synchState = Pim::SynchState::synchTaskScheduled;
		}
	}


	template<typename tKey>
	void XX::reorganizeAndSynchronize()
	{
//DebugScope("Schedule::reorganize");
		// ===== get keys to reorganize
		std::map<tKey,unsigned> firstKeys;
		{
			std::lock_guard<std::mutex> guard(pim->reorganizeAccess);
			if (pim->synchState != Pim::SynchState::synchTaskScheduled) return; // obsolete task
			pim->synchState = Pim::SynchState::duringSynchTaskExecution;
			firstKeys = pim->reorganizeFirstKeys;
			if (firstKeys.empty()) {
				pim->synchState = Pim::SynchState::noSynchTask;
				return;
			}
		}

		// ===== convert firstKeys to data node indexes
		auto iEBegin = pim->currentDb->entries.begin();
		auto iE = iEBegin;
		std::vector<std::pair<unsigned,unsigned>> indexes;  // data node index -> priority
		indexes.reserve(firstKeys.size());
		for ( auto kv: firstKeys ) {
			while ( (*iE)->bin.firstKey < kv.first ) ++iE;
			ASSERT( (*iE)->bin.firstKey == kv.first );
			indexes.push_back( std::make_pair( iE - iEBegin, kv.second ) );
		}

		// ===== reorganize index node
		// ===== get index node to save to storage and get obsolete data nodes
		typename IndexNode::SP indexNode;
		std::vector<typename DataNode::SP> removedDataNodes;
		{
			std::lock_guard<std::mutex> guard(pim->accessCurrentDb);
			removedDataNodes = pim->currentDb->reorganize(indexes);
			if (! removedDataNodes.empty()) {
				indexNode = pim->currentDb->createSecondCopy();
				pim->currentDb.swap(indexNode);
			}
		}

		if (removedDataNodes.empty()) {
			// ===== no changes
			std::this_thread::yield();

		} else {
			// ===== there are changes to commit

			// ===== get rid of removed data nodes from reorganizeFirstKeys, recalculate priorities
			{
				std::lock_guard<std::mutex> guard(pim->reorganizeAccess);
				// remove data nodes & calculate their synchronization priority
				for (typename DataNode::SP dn: removedDataNodes) {
					auto it = pim->reorganizeFirstKeys.find(dn->bin.firstKey);
					ASSERT(it != pim->reorganizeFirstKeys.end());
					pim->reorganizeFirstKeys.erase(it);
				}
				// recalculate reorganization priority
				pim->reorganizePriority = 0;
				for (auto const & kv: pim->reorganizeFirstKeys) {
					pim->reorganizePriority = std::max(pim->reorganizePriority,kv.second);
				}
			}

			// flush all IO writes
//Stopwatch sw;
			storage->flush();
//std::cout << "Flush: " << sw.get_time_ms() << " ";

			// write new index node
			indexNode->writeToBuffer( pim->bufferForIndexNode );
			storage->writePages( (indexNode->firstCopy ? 0 : pagesPerIndexNode), pagesPerIndexNode, pim->bufferForIndexNode );  // TODO - combine with flush?
//sw.reset_and_restart();
			storage->flush();
//std::cout << sw.get_time_ms() << std::endl;

			// set new committed node
			{
				std::lock_guard<std::mutex> guard(pim->accessCommittedDb);
				pim->committedDb = indexNode;
			}

			// set all old data nodes as obsolete
			for (auto dn: removedDataNodes) dn->markAsObsolete();
		}

		// reschedule reorganize&synchronize task if needed
		{
			std::lock_guard<std::mutex> guard(pim->reorganizeAccess);
			if (pim->reorganizeFirstKeys.empty()) {
				pim->synchState = Pim::SynchState::noSynchTask;
			} else {
				auto task = [this]()->void{ this->reorganizeAndSynchronize(); };
				cpuTasksManager->addTask( task, pim->reorganizePriority );
				pim->synchState = Pim::SynchState::synchTaskScheduled;
			}
		}
	}


	template<typename tKey>
	tKey XX::getTheLargestKey() const
	{
		std::lock_guard<std::mutex> guard(pim->accessCurrentDb);
		if ( pim->currentDb->entries.empty() ) return 0;  // it is not really true
		return (pim->currentDb->entries.back()->bin.lastKey());
	}


	template<typename tKey>
	uint64_t XX::getRecordsCount() const
	{
		std::lock_guard<std::mutex> guard(pim->accessCommittedDb);
		uint64_t r = 0;
		for (auto e: pim->committedDb->entries) r += e->bin.recordsCount;
		return r;
	}


	template<typename tKey>
	void XX::printStatus() const
	{
		std::lock_guard<std::mutex> guard(pim->accessCommittedDb);
		std::lock_guard<std::mutex> guard2(pim->accessCurrentDb);
		DebugVar(pim->committedDb->entries.size());
		if (pim->committedDb->entries.size() != pim->currentDb->entries.size()) {
			std::cout << "Committed & Current Databases have different sizes!" << std::endl;
		} else {
			for (unsigned i = 0; i < pim->committedDb->entries.size(); ++i) {
				if (pim->committedDb->entries[i].get() != pim->currentDb->entries[i].get()) {
					std::cout << "Committed & Current Databases have different nodes on position " << i << std::endl;
					break;
				}
			}
		}
	}


	template class SchedulerT<uint32_t>;
	template class SchedulerT<uint64_t>;
}
