#ifndef FLATDB_STORAGEWITHCACHE_HPP_
#define FLATDB_STORAGEWITHCACHE_HPP_

#include <string>
#include <vector>
#include <map>

namespace flatDb {

	// class representing storage with cache
	class StorageWithCache
	{
	private:
		struct Pim;
		Pim * pim;
	public:
		struct Statistics {
			uint64_t synchCount;
			uint64_t synchTimeMs;
			uint64_t writesCount;
			uint64_t writesTimeMs;
			uint64_t readsCount;
			uint64_t readsTimeMs;
		};
		unsigned const pageSize;

		// ===== these methods are not synchronized

		// file must be ready to use, given object is deleted in destructor
		// all pages in file are marked as allocated
		StorageWithCache(std::string const & path, unsigned pageSize, uint64_t cacheMemoryInMegabyte);

		~StorageWithCache();

		// shrink file to given number of pages and overwrite the set of free pages (free = not allocated)
		void     setFreePages(unsigned newNumberOfPages, std::map<unsigned,unsigned> const & freePages);

		// returns file size as number of pages (including both allocated and free pages)
		unsigned numberOfPages() const;

		// ===== all methods below are synchronized, they can be called in many threads

		// allocates given number of pages and returns vector of assigned pages ids, adjusts file size if needed
		std::vector<unsigned> allocatePages(unsigned pagesCount);

		// releases (mark as free) given pages, adjusts file size if needed
		// pages are unlocked first
		void releasePages(std::vector<unsigned> pagesIds);

		// tries to read page from cache (no IO operations are performed)
		// returns true <=> the page is in the cache: the routine locks it and set the given pointer
		// returns false <=> the page is not in the cache: the given pointer is set to nullptr
		bool lockPage_loadFromCache(unsigned pageId, uint8_t*& outPagePtr);

		// the routine locks the page in cache and set the given pointer
		// the page is load from storage to cache, if it is not there (may block on IO)
		// the function return false, if there is no more space in cache, the given pointer is set to nullptr
		bool lockPage_loadFromStorage(unsigned pageId, uint8_t*& outPagePtr);

		// the routine locks the page in cache and set the given pointer
		// the page is created in cache, if it is not there (no IO operations are performed)
		void lockPage_createEmpty(unsigned pageId, uint8_t*& outPagePtr);

		// save locked page to the storage
		void savePageToStorage(unsigned pageId);

		// release all locks on the page
		// after calling this method, the page can be removed from cache in any moment
		void unlockPage(unsigned pageId);

		// read directly from storage to given buffer (cache is omitted)
		void readPages(unsigned firstPageId, unsigned pagesCount, uint8_t* buffer);

		// write directly from given buffer to storage (cache is omitted)
		void writePages(unsigned firstPageId, unsigned pagesCount, uint8_t const* buffer);

		// flush all new/modified pages to storage - it returns when everything is flushed
		void flush();

		// get statistics
		StorageWithCache::Statistics readAndResetStatistics();
	};

}



#endif /* FLATDB_STORAGEWITHCACHE_HPP_ */
