#include "StorageWithCache.hpp"
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
#include "../commonTools/assert.hpp"
#include "../debug.hpp"


namespace flatDb {

	struct CacheEntry {
		uint8_t* buffer = nullptr;
		std::list<unsigned>::iterator placeInLineToDelete;
		bool locked = false;
	};


	struct StorageWithCache::Pim {
		FileWithPages file;
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
		std::vector<CacheEntry> cache;
		std::list<unsigned> pagesToDelete;
		unsigned countOfCachePages = 0;
		unsigned const maxCountOfCachePages;
		// ------------------
		inline bool lockMem(unsigned pageId, bool force)  // force = true - allow for overallocation
		{
			CacheEntry & e = cache[pageId];
			// ===== if already locked, there is nothing to do
			if ( e.locked ) {
				return true;
			}
			e.locked = true;
			// ===== check if buffer is already allocated
			if (e.buffer == nullptr) {
				// it is not, we have to allocate it or take from another page
				if ( countOfCachePages < maxCountOfCachePages || (pagesToDelete.empty() && force) ) {
					e.buffer = new uint8_t[file.pageSize];
					++(countOfCachePages);
				} else if (! pagesToDelete.empty()) {
					std::swap( e.buffer, cache[pagesToDelete.front()].buffer );
					pagesToDelete.pop_front();
				} else {
					e.locked = false; // failure
				}
			} else {
				// it is allocated, just remove if from list to delete
				pagesToDelete.erase( e.placeInLineToDelete );
			}
			return e.locked;
		}
		inline void unlockMem(unsigned pageId)
		{
			CacheEntry & e = cache[pageId];
			// ===== if already unlocked, there is nothing to do
			if ( ! e.locked ) {
				return;
			}
			e.locked = false;
			// ===== check if number of pages is OK
			if (countOfCachePages > maxCountOfCachePages) {
				// too many pages, this one will be freed
				delete [] e.buffer;
				e.buffer = nullptr;
				--(countOfCachePages);
			} else {
				// number of pages OK, schedule this one to be deleted
				e.placeInLineToDelete = pagesToDelete.insert(pagesToDelete.end(), pageId);
			}
		}
		Pim(std::string const & path, unsigned pageSize, uint64_t cacheMemoryInMegabyte)
		: file(path,pageSize,false), maxCountOfCachePages(cacheMemoryInMegabyte * 1024 * 1024 / pageSize) {}
	};


	StorageWithCache::StorageWithCache(std::string const & path, unsigned pPageSize, uint64_t cacheMemoryInMegabyte)
	: pim(new StorageWithCache::Pim(path, pPageSize, cacheMemoryInMegabyte)), pageSize(pPageSize)
	{
		ASSERT(pim->maxCountOfCachePages > 4);  // it is just a guess, minimum 4 cache pages
		pim->cache.resize( pim->file.numberOfPages() );
	}


	StorageWithCache::~StorageWithCache()
	{
		// TODO - free memory from cache
		delete pim;
	}


	// shrink file and set pages that can be reused
	void StorageWithCache::setFreePages(unsigned newNumberOfPages, std::map<unsigned,unsigned> const & freePages)
	{
		pim->file.setFreePages(newNumberOfPages,freePages);
	}


	unsigned StorageWithCache::numberOfPages() const
	{
		return pim->file.numberOfPages();
	}


	// allocate new pages and bind them to unused ids, nothing is read from the file
	// file may be extended
	std::vector<unsigned> StorageWithCache::allocatePages(unsigned pagesCount)
	{
		if (pagesCount == 0) return std::vector<unsigned>(0);
		std::vector<unsigned> pagesIds;
		pagesIds.reserve(pagesCount);
		{
			std::lock_guard<std::mutex> synchAccess(pim->cacheAccess);
			// --- reserve pages in file
			pagesIds.push_back( pim->file.allocatePages(pagesCount) );
			// --- adjust size of cache, if required
			unsigned const minPagesCount = pagesIds.back() + pagesCount;
			if (pim->cache.size() < minPagesCount) pim->cache.resize(minPagesCount);
		}
		while ( pagesIds.size() < pagesCount ) {
			pagesIds.push_back( pagesIds.back() + 1 );
		}
		return pagesIds;
	}


	void  StorageWithCache::releasePages(std::vector<unsigned> pagesIds)
	{
		if (pagesIds.empty()) return;
		std::sort( pagesIds.begin(), pagesIds.end() );
		{
			std::lock_guard<std::mutex> synchAccess(pim->cacheAccess);
			ASSERT(pagesIds.back() < pim->cache.size());
			for (auto pageId: pagesIds) {
				pim->file.releasePages(pageId, 1);
				ASSERT( ! pim->cache[pageId].locked );
			}
		}
	}


	bool StorageWithCache::lockPage_loadFromCache(unsigned pageId, uint8_t*& outPagePtr)
	{
		std::lock_guard<std::mutex> synchAccess(pim->cacheAccess);
		ASSERT(pageId < pim->cache.size());
		outPagePtr = pim->cache[pageId].buffer;
		if (outPagePtr == nullptr) {
			return false;
		}
		pim->lockMem(pageId, true);
		return true;
	}


	bool StorageWithCache::lockPage_loadFromStorage(unsigned pageId, uint8_t*& outPagePtr)
	{
		{
			std::lock_guard<std::mutex> synchAccess(pim->cacheAccess);
			ASSERT(pageId < pim->cache.size());
			if ( ! pim->lockMem(pageId,false) ) return false;
			outPagePtr = pim->cache[pageId].buffer;
		}
		Stopwatch sw;
		pim->file.readPages(pageId, 1, outPagePtr);
		pim->updateTimes(sw, pim->countOfReads, pim->timeOfReads);
		return true;
	}


	void StorageWithCache::lockPage_createEmpty(unsigned pageId, uint8_t*& outPagePtr)
	{
		std::lock_guard<std::mutex> synchAccess(pim->cacheAccess);
		ASSERT(pageId < pim->cache.size());
		pim->lockMem(pageId, true);
		outPagePtr = pim->cache[pageId].buffer;
	}


	// save locked page to the storage
	void StorageWithCache::savePageToStorage(unsigned pageId)
	{
		void * ptr;
		{
			std::lock_guard<std::mutex> synchAccess(pim->cacheAccess);
			ASSERT(pageId < pim->cache.size());
			ASSERT(pim->cache[pageId].locked);
			ptr = pim->cache[pageId].buffer;
		}
		Stopwatch sw;
		pim->file.writePages(pageId,1,ptr);
		pim->updateTimes(sw, pim->countOfWrites, pim->timeOfWrites);
	}


	// release all locks on the page
	// after calling this method, the page can be removed from cache in any moment
	void StorageWithCache::unlockPage(unsigned pageId)
	{
		std::lock_guard<std::mutex> synchAccess(pim->cacheAccess);
		ASSERT(pageId < pim->cache.size());
		pim->unlockMem(pageId);
	}


	// read directly from storage to given buffer (cache is omitted)
	void StorageWithCache::readPages(unsigned firstPageId, unsigned pagesCount, uint8_t* buffer)
	{
		pim->file.readPages(firstPageId, pagesCount, buffer);
	}


	// write directly from given buffer to storage (cache is omitted)
	void StorageWithCache::writePages(unsigned firstPageId, unsigned pagesCount, uint8_t const* buffer)
	{
		pim->file.writePages(firstPageId, pagesCount, buffer);
	}


	// flush all new/modified pages to storage - it returns when everything is flushed
	void StorageWithCache::flush()
	{
		Stopwatch sw;
		pim->file.flush();
		pim->updateTimes(sw, pim->countOfSynch, pim->timeOfSynch);
	}


	typename StorageWithCache::Statistics StorageWithCache::readAndResetStatistics()
	{
		Statistics r;
		std::lock_guard<std::mutex> synchAccess(pim->timersAccess);
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
		return r;
	}

}

