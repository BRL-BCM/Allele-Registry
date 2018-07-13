#include "PagesManager.hpp"
#include "FileWithPages.hpp"
#include <map>
#include <list>
#include <algorithm>
#include <stdexcept>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <cstring>


#include "../commonTools/Stopwatch.hpp"
#include <iostream>

	struct CacheEntry {
		uint8_t* buffer = nullptr;
		unsigned numberOfLocks = 0;
		std::list<std::pair<tPageId,tPageId>>::iterator placeInLineToDelete;
		bool isBeingCreated = false;
		bool isBeingDeleted = false;
	};

	template<unsigned pageSize>
	struct PagesManager<pageSize>::Pim {
		genomeDb::FileWithPages<pageSize> file;
		// ---- measurements
		std::mutex timersAccess;
		uint64_t countOfSynch  = 0;
		uint64_t timeOfSynch   = 0;
		uint64_t countOfWrites = 0;
		uint64_t timeOfWrites  = 0;
		uint64_t countOfReads  = 0;
		uint64_t timeOfReads   = 0;
		void updateTimes(Stopwatch timer, uint64_t & count, uint64_t & time)
		{
			uint64_t t = timer.get_time_ms();
			{
				std::lock_guard<std::mutex> synchAccess(timersAccess);
				time += t;
				++count;
			}
		}
		// ---- memory & cache
		std::mutex cacheAccess;
		std::map<std::pair<tPageId,tPageId>,CacheEntry> cache;
		std::list<std::pair<tPageId,tPageId>> pagesToDelete;
		unsigned countOfCachePages = 0;
		unsigned const maxCountOfCachePages;
		// ------------------
		void checkCache()
		{
			while (countOfCachePages > maxCountOfCachePages) {
				if (pagesToDelete.empty()) return;
				auto it = cache.find(pagesToDelete.back());
				pagesToDelete.pop_back();
				if (it == cache.end() || it->second.numberOfLocks > 0) throw std::logic_error("cannot remove page from the cache: assertion failed");
				delete [] it->second.buffer;
				countOfCachePages -= it->first.second;
				cache.erase(it);
			}
		}
		CacheEntry * acquireCacheEntry(Buffer b)
		{
			std::pair<tPageId,tPageId> key = std::make_pair(b.firstPageId,b.pagesCount);
			auto it = cache.find(key);
			if (it == cache.end()) {
				CacheEntry e;
				e.isBeingCreated = true;
				e.buffer = new uint8_t[pageSize*b.pagesCount];
				countOfCachePages += key.second;
				it = cache.emplace(key, e).first;
				checkCache();
			} else {
				if (it->second.numberOfLocks == 0) pagesToDelete.erase(it->second.placeInLineToDelete);
			}
			++(it->second.numberOfLocks);
			return &(it->second);
		}
		void releaseCacheEntry(Buffer b)
		{
			std::pair<tPageId,tPageId> key = std::make_pair(b.firstPageId,b.pagesCount);
			auto it = cache.find(key);
			if (it == cache.end() || it->second.numberOfLocks == 0) throw std::logic_error("Releasing page that is not locked!");
			if (--(it->second.numberOfLocks) == 0) {
				if (it->second.isBeingDeleted) {
					delete [] it->second.buffer;
					countOfCachePages -= key.second;
					cache.erase(it);
					file.releasePages(b.firstPageId, b.pagesCount);
				} else {
					pagesToDelete.push_front(key);
					it->second.placeInLineToDelete = pagesToDelete.begin();
					if (pagesToDelete.size() == 1) checkCache();
				}
			}

		}
		Pim(std::string const & path, bool synchronized, uint64_t cacheMemoryInMegabyte)
		: file(path,synchronized), maxCountOfCachePages(cacheMemoryInMegabyte * 1024 * 1024 / pageSize) {}
	};

	template<unsigned pageSize>
	PagesManager<pageSize>::PagesManager(std::string const & path, bool synchronized, uint64_t cacheMemoryInMegabyte)
	: pim(new Pim(path, synchronized, cacheMemoryInMegabyte))
	{}

	template<unsigned pageSize>
	PagesManager<pageSize>::~PagesManager()
	{
		// TODO - free memory from cache
		delete pim;
	}

	template<unsigned pageSize>
	unsigned PagesManager<pageSize>::numberOfPages() const
	{
		std::lock_guard<std::mutex> synchAccess(pim->cacheAccess);
		return pim->file.numberOfPages();
	}

	// create empty page (filled with zeroes) and bind it to given id, nothing is read from the file
	// file may be extended
	template<unsigned pageSize>
	BufferT<pageSize> PagesManager<pageSize>::allocateBuffer(unsigned pagesCount)
	{
		Buffer b;
		b.pagesCount = pagesCount;
		if (b.pagesCount == 0) {
			b.firstPageId = 0;
			b.data = nullptr;
			return b;
		}
		{
			std::lock_guard<std::mutex> synchAccess(pim->cacheAccess);
			b.firstPageId = pim->file.allocatePages(pagesCount);
			CacheEntry * entry = pim->acquireCacheEntry(b);
			b.data = entry->buffer;
			memset(b.data, 0, pageSize*pagesCount);
			entry->isBeingCreated = false;
		}
//std::cout << "allocate\t" << b.firstPageId << "\t" << b.pagesCount << "\n";
		return b;
	}

	// load page with given id from the file (or cache)
	template<unsigned pageSize>
	BufferT<pageSize> PagesManager<pageSize>::loadBuffer(tPageId firstPage, unsigned pagesCount)
	{
		Buffer b;
		b.firstPageId = firstPage;
		b.pagesCount = pagesCount;
		if (pagesCount == 0) {
			b.data = nullptr;
			return b;
		}
		CacheEntry * entry = nullptr;
		bool loadFromDisk = false;
		{
			std::lock_guard<std::mutex> synchAccess(pim->cacheAccess);
			entry = pim->acquireCacheEntry(b);
			loadFromDisk = entry->isBeingCreated && (entry->numberOfLocks == 1);
			b.data = entry->buffer;
		}
		if (loadFromDisk) {
			Stopwatch timer;
			pim->file.readPages(b.firstPageId, b.pagesCount, entry->buffer);
			pim->updateTimes(timer,pim->countOfReads,pim->timeOfReads);
			pim->cacheAccess.lock();
			entry->isBeingCreated = false;
			pim->cacheAccess.unlock();
		} else {
			bool isBeingCreated = entry->isBeingCreated;
			while (isBeingCreated) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				pim->cacheAccess.lock();
				isBeingCreated = entry->isBeingCreated;
				pim->cacheAccess.unlock();
			}
		}
		return b;
	}

	// save given pages to the file
	template<unsigned pageSize>
	void PagesManager<pageSize>::synchBuffer(BufferT<pageSize> b)
	{
		synchBuffers(std::vector<Buffer>(1,b));
	}

	// save given pages to the file, given vector is sorted
	template<unsigned pageSize>
	void PagesManager<pageSize>::synchBuffers(std::vector<BufferT<pageSize>> bs)
	{
		if (bs.empty()) return;
		std::sort(bs.begin(), bs.end());
//		std::vector<BufferT<pageSize>> bs2(1, bs[0]);  //pwritev must be used in the layer below
//		for (auto const & b: bs) {
//			if (b.firstPageId <= bs2.back().firstPageId + bs2.back().pagesCount) {
//				bs.back().pagesCount =
//			} else {
//				bs.push_back(b);
//			}
//		}
// TODO - joining buffers
//		std::vector< std::pair<tPageId,unsigned> > toSynch;
//		toSynch.push_back(pages.front());
//		for (unsigned i = 1; i < pages.size(); ++i) {
//			tPageId const nextPageId =  toSynch.back().first + toSynch.back().second;
//			if (pages[i].first < nextPageId) throw std::runtime_error("Synchronization of overlapping regions");
//			if (pages[i].first == nextPageId) {
//				toSynch.back().second += pages[i].second;
//			} else {
//				toSynch.push_back( pages[i] );
//			}
//		}
		// write data
		Stopwatch timer;
		for (Buffer const & b: bs) {
			if (b.pagesCount == 0) continue;
			pim->file.writePages(b.firstPageId, b.pagesCount, b.data);
		}
		pim->updateTimes(timer,pim->countOfWrites,pim->timeOfWrites);
		pim->countOfWrites += bs.size();
	}

	template<unsigned pageSize>
	void PagesManager<pageSize>::deleteBuffer(BufferT<pageSize> b)
	{
		if (b.pagesCount == 0) return;
//		std::cout << "delete\t" << b.firstPageId << "\t" << b.pagesCount << "\n";
		std::lock_guard<std::mutex> synchAccess(pim->cacheAccess);
		CacheEntry * entry = pim->acquireCacheEntry(b);
		entry->isBeingDeleted = true;
		pim->releaseCacheEntry(b);
		pim->releaseCacheEntry(b);
	}

	// free pages, a buffer assigned to it is deleted (may stay in cache)
	template<unsigned pageSize>
	void  PagesManager<pageSize>::releaseBuffer(BufferT<pageSize> b)
	{
		if (b.pagesCount == 0) return;
		std::lock_guard<std::mutex> synchAccess(pim->cacheAccess);
		pim->releaseCacheEntry(b);
	}

	template<unsigned pageSize>
	void  PagesManager<pageSize>::synchronize()
	{
		Stopwatch timer;
	//	fdatasync(pim->file);
		pim->updateTimes(timer,pim->countOfSynch,pim->timeOfSynch);
	}

	// shrink file and set pages that can be reused
	template<unsigned pageSize>
	void PagesManager<pageSize>::setFreePages(unsigned newNumberOfPages, std::map<tPageId,unsigned> const & freePages)
	{
		pim->file.setFreePages(newNumberOfPages,freePages);
	}

	template<unsigned pageSize>
	typename PagesManager<pageSize>::Statistics PagesManager<pageSize>::readAndResetStatistics()
	{
		pim->timersAccess.lock();
		Statistics r;
		r.synchCount   = pim->countOfSynch;
		r.synchTimeMs  = pim->timeOfSynch;
		r.writesCount  = pim->countOfWrites;
		r.writesTimeMs = pim->timeOfWrites;
		r.readsCount   = pim->countOfReads;
		r.readsTimeMs  = pim->timeOfReads;
		pim->countOfSynch  = 0;
		pim->timeOfSynch   = 0;
		pim->countOfWrites = 0;
		pim->timeOfWrites  = 0;
		pim->countOfReads  = 0;
		pim->timeOfReads   = 0;
		pim->timersAccess.unlock();
		return r;
	}

	template class PagesManager<512>;
	template class PagesManager<1024>;
	template class PagesManager<3*512>;
	template class PagesManager<2*1024>;
	template class PagesManager<4*1024>;
	template class PagesManager<8*1024>;
//	template class PagesManager<16*1024>;
//	template class PagesManager<32*1024>;
//	template class PagesManager<64*1024>;
//	template class PagesManager<128*1024>;
//	template class PagesManager<256*1024>;
//	template class PagesManager<512*1024>;
//	template class PagesManager<1024*1024>;

