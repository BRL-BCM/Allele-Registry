#include "PrefixTreeDbPim.hpp"
#include <map>

#include "../commonTools/Stopwatch.hpp"
#include <iostream>

	template<typename tKey, unsigned const globalKeySize, unsigned dataPageSize>
	DatabaseT<tKey,globalKeySize,dataPageSize>::DatabaseT
		( TasksManager * pTaskManager
		, std::string const & dbFileName
		, tCreateRecord funcLoadData
		, unsigned cacheSizeInMegabytes
		)
	: pim(new Pim)
	{
		std::string const indexFile = dbFileName + ".index";
		std::string const dataFile  = dbFileName + ".data";
		pim->callbackCreateRecord = funcLoadData;
		pim->tasksManager = pTaskManager;
		pim->indexPagesManager = new PagesManager<1536>(indexFile, false, cacheSizeInMegabytes);
		pim->dataPagesManager = new PagesManager<dataPageSize>(dataFile, false, cacheSizeInMegabytes);
		// ----- read index nodes
		unsigned const numberOfIndexPages = pim->indexPagesManager->numberOfPages();
		if (numberOfIndexPages % 2 == 1) throw std::runtime_error("Odd number of index pages");
		if (numberOfIndexPages == 0) {
			std::vector<typename PagesManager<dataPageSize>::Buffer> newDataPages;
			std::vector<PagesManager<1536>::Buffer> newIndexPages;
			pim->root = new IndexNode<dataPageSize,globalKeySize,tKey>(pim,0,std::vector<RecordT<tKey>*>(),newDataPages,newIndexPages);
			pim->dataPagesManager->synchBuffers(newDataPages);
			pim->indexPagesManager->synchBuffers(newIndexPages);
		} else {
			pim->root = new IndexNode<dataPageSize,globalKeySize,tKey>(pim,0);
		}
		// ----- find unused data pages
		unsigned const numberOfDataPages = pim->dataPagesManager->numberOfPages();
std::cout << dataFile <<  ": number of pages = " << numberOfDataPages << std::endl;
		std::vector<bool> usedDataPages(numberOfDataPages,false);
		pim->root->markUsedDataPages(usedDataPages);
		unsigned newNumberOfPages = numberOfDataPages;
		for ( ;  newNumberOfPages > 0;  --newNumberOfPages ) if (usedDataPages[newNumberOfPages-1]) break;
std::cout << dataFile <<  ": new number of pages = " << newNumberOfPages << std::endl;
		std::map<tPageId, unsigned> freePages;  // id -> size
		bool freePage = false;
		for (unsigned i = 0; i < newNumberOfPages; ++i) {
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
std::cout << dataFile <<  ": free regions = " << freePages.size() << std::endl;
		pim->dataPagesManager->setFreePages(newNumberOfPages, freePages);
	}

	template<typename tKey, unsigned const globalKeySize, unsigned dataPageSize>
	DatabaseT<tKey,globalKeySize,dataPageSize>::~DatabaseT()
	{
		// TODO - how to check the list of existing tasks ?
	}

	template<typename tKey, unsigned const globalKeySize, unsigned dataPageSize>
	void DatabaseT<tKey,globalKeySize,dataPageSize>::readRecordsInOrder(tReadFunction visitor, tKey first, tKey last) const
	{
		unsigned const minChunkSize = 1024*1024;
		std::vector<RecordT<tKey> const *> records;
		pim->root->readRecordsInOrder(visitor,records,first,last,minChunkSize);
	}

	template<class Record>
	bool compRecordsByKey(Record const * const r1, Record const * const r2)
	{
		return (r1->key < r2->key);
	}

	template<typename tKey, unsigned const globalKeySize, unsigned dataPageSize>
	void DatabaseT<tKey,globalKeySize,dataPageSize>::readRecords(std::vector<RecordT<tKey>*> const & pRecords, tReadByKeyFunction visitor) const
	{
		std::vector<RecordT<tKey>*> records = pRecords;
		std::sort(records.begin(), records.end(), compRecordsByKey<RecordT<tKey>>);
		auto task = [this,visitor,&records]() { pim->root->readRecords(records.begin(),records.end(),visitor); };
		//std::cout << "Reading " << records.size() << " records" << std::endl;
		//Stopwatch timer;
		unsigned const taskId = pim->tasksManager->addTask(task);
		pim->tasksManager->joinTask(taskId);
		//std::cout << "Total time: " << time << " seconds" << std::endl;
	}

	template<typename tKey, unsigned const globalKeySize, unsigned dataPageSize>
	void DatabaseT<tKey,globalKeySize,dataPageSize>::writeRecords(std::vector<RecordT<tKey>*> const & pRecords, tUpdateByKeyFunction visitor)
	{
		std::vector<RecordT<tKey>*> records = pRecords;
		std::sort(records.begin(), records.end(), compRecordsByKey<RecordT<tKey>>);
		auto task = [this,visitor,&records]() { pim->root->writeRecords(records.begin(),records.end(),visitor); };
		//std::cout << "Writing " << records.size() << " records" << std::endl;
		//Stopwatch timer;
		unsigned const taskId = pim->tasksManager->addTask(task);
		pim->tasksManager->joinTask(taskId);
		//unsigned time = timer.get_time_sec();
		//std::cout << "Total time: " << time << " seconds" << std::endl;
	}

	template<typename tKey, unsigned const globalKeySize, unsigned dataPageSize>
	tKey DatabaseT<tKey,globalKeySize,dataPageSize>::getTheLargestKey() const
	{
		return pim->root->getTheLargestKey();
	}


	template class DatabaseT<uint32_t,4,8192>;
	template class DatabaseT<uint64_t,5,8192>;
