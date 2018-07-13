#include "FileWithPages.hpp"
#include <map>
#include <list>
#include <memory>
#include <cstring>
#include <atomic>
// -------- low level file access
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>

namespace flatDb {

	struct FileWithPages::Pim
	{
		std::string path;
		int file = 0;
		unsigned pagesCount = 0;
		std::map<unsigned,unsigned> freePages;       // page->id => number of pages
		std::map<unsigned,std::list<unsigned>> freePagesBySize; // number of pages => pageId
		std::atomic<bool> fileWasResized;
		Pim() { fileWasResized = false; }
		void throw_error(std::string const & msg)
		{
			throw std::runtime_error("A problem occurred for the file " + path + ". " + msg);
		}
		void throw_error(std::string const & msg, int const errnum)
		{
			char const * buf = strerror(errnum);
			if (buf == nullptr) throw_error(msg);
			throw_error(msg + std::string(buf));
		}
	};


	FileWithPages::FileWithPages(std::string const & path, unsigned pPageSize, bool synchronized) : pim(new Pim), pageSize(pPageSize)
	{
		std::unique_ptr<Pim> scopedPtr(pim);
		pim->path = path;
		int flags = O_RDWR | O_CREAT | O_NOATIME;
		if (synchronized) flags |= O_SYNC;
		pim->file = open(path.c_str(), flags, S_IRUSR | S_IWUSR );
		if (pim->file < 0) {
			int errnum = errno;
			pim->throw_error("Cannot open the file. Error returned by open(): ", errnum);
		}
		struct stat buf;
		if (fstat(pim->file,&buf) != 0) {
			int errnum = errno;
			close(pim->file);
			pim->throw_error("Cannot check the size of the file. Error returned by fstat(): ", errnum);
		}
		if (flock(pim->file, LOCK_EX | LOCK_NB ) != 0) {
			int errnum = errno;
			close(pim->file);
			pim->throw_error("Cannot lock the file. Error returned by flock(): ", errnum);
		}
		off_t file_size = buf.st_size;
		pim->pagesCount = file_size / pageSize;
		if (file_size % pageSize) {
			close(pim->file);
			pim->throw_error("The size of the file is not a multiplication of the page size.");
		}
		scopedPtr.release();
	}


	FileWithPages::~FileWithPages()
	{
		close(pim->file);
		delete pim;
	}


	// shrink file and set pages that can be reused
	void FileWithPages::setFreePages(unsigned newNumberOfPages, std::map<unsigned,unsigned> const & freePages)
	{
		if (pim->pagesCount < newNumberOfPages || ! pim->freePages.empty()) throw std::logic_error("Assertion failed for setFreePages");
		uint64_t const pageSize64 = static_cast<int64_t>(pageSize);
		if (pim->pagesCount > newNumberOfPages) {
			if ( ftruncate64(pim->file,newNumberOfPages*pageSize64) != 0 ) {
				pim->throw_error("ftruncate64() failed: ", errno);
			}
			pim->pagesCount = newNumberOfPages;
		}
		pim->freePages = freePages;
		for (auto const & kv: freePages) pim->freePagesBySize[kv.second].push_back(kv.first);
	}


	unsigned FileWithPages::numberOfPages() const
	{
		return pim->pagesCount;
	}


	unsigned FileWithPages::allocatePages(unsigned numberOfPages)
	{
		auto it = pim->freePagesBySize.lower_bound(numberOfPages);
		if (it != pim->freePagesBySize.end()) {
			unsigned const pageId = it->second.front();
			it->second.pop_front();
			pim->freePages.erase(pageId);
			if (it->first > numberOfPages) {
				pim->freePagesBySize[it->first-numberOfPages].push_front(pageId+numberOfPages);
				pim->freePages[pageId+numberOfPages] = it->first-numberOfPages;
			}
			if (it->second.empty()) pim->freePagesBySize.erase(it);
			return pageId;
		}
		unsigned const pageId = pim->pagesCount;
		// we want to increase in 32 MB blocks
		unsigned const pagesCountPerBlock = std::max( 1u, 32*1024*1024 / pageSize );  // = number of pages in  32 MB or 1 if page size >  32 MB
		// we do not want to increase by more than 256 MB
		unsigned const maxBlockCountIncrease = std::max( 1u,256*1024*1024 / (pagesCountPerBlock * pageSize) );  // = number of blocks in 256 MB or 1 if block size > 256 MB
		unsigned const blockCountIncrease = std::min( maxBlockCountIncrease, std::max(numberOfPages/pagesCountPerBlock+1, pim->pagesCount/pagesCountPerBlock/4) );
		unsigned const pagesCountIncrease = blockCountIncrease * pagesCountPerBlock;
		uint64_t const pageSize64 = pageSize;
		if ( fallocate(pim->file, 0, pim->pagesCount*pageSize64, pagesCountIncrease*pageSize64) != 0 ) {
			pim->throw_error("fallocate() failed: ", errno);
		}
		pim->fileWasResized.store(true);
		pim->pagesCount += pagesCountIncrease;
		pim->freePages[pageId + numberOfPages] = pagesCountIncrease - numberOfPages;
		pim->freePagesBySize[pagesCountIncrease - numberOfPages].push_front(pageId + numberOfPages);
		return pageId;
	}


	void FileWithPages::releasePages(unsigned pageId, unsigned numberOfPages)
	{
		auto it = pim->freePages.lower_bound(pageId);
		if ( it != pim->freePages.end() ) {
			if (it->first < pageId + numberOfPages) throw std::logic_error("The page is already released! (0)");
			// --- connect with next range of free pages, if possible
			if (it->first == pageId + numberOfPages) {
				numberOfPages += it->second;
				pim->freePagesBySize[it->second].remove(it->first);
				if (pim->freePagesBySize[it->second].empty()) pim->freePagesBySize.erase(it->second);
				auto it2 = it;
				++it;
				pim->freePages.erase(it2);
			}
		}
		if (it != pim->freePages.begin()) {
			--it;
			if (it->first + it->second > pageId) throw std::logic_error("The page is already released! (2)");
			// --- connect with previous range of free pages, if possible
			if (it->first + it->second == pageId) {
				numberOfPages += it->second;
				pageId = it->first;
				pim->freePagesBySize[it->second].remove(it->first);
				if (pim->freePagesBySize[it->second].empty()) pim->freePagesBySize.erase(it->second);
				pim->freePages.erase(it);
			}
		}
		pim->freePages[pageId] = numberOfPages;
		pim->freePagesBySize[numberOfPages].push_front(pageId);
	}


	void FileWithPages::writePages(unsigned pageId, unsigned pagesCount, void const * buf)
	{
		uint64_t const pageSize64 = static_cast<int64_t>(pageSize);
		uint64_t offsetBegin = pageId*pageSize64;
		uint64_t const offsetEnd = offsetBegin + pagesCount*pageSize64;
		while (offsetBegin < offsetEnd) {
			ssize_t const result = pwrite(pim->file, buf, offsetEnd-offsetBegin, offsetBegin);
			if ( result < 0 ) pim->throw_error("Cannot write to the file: ", errno);
			offsetBegin += result;
			buf = static_cast<uint8_t const *>(buf) + result;
		}
	}


	void FileWithPages::readPages(unsigned pageId, unsigned pagesCount, void * buf)
	{
		uint64_t const pageSize64 = static_cast<int64_t>(pageSize);
		uint64_t offsetBegin = pageId*pageSize64;
		uint64_t const offsetEnd = offsetBegin + pagesCount*pageSize64;
		while (offsetBegin < offsetEnd) {
			ssize_t const result = pread(pim->file, buf, offsetEnd-offsetBegin, offsetBegin);
			if ( result < 0 ) pim->throw_error("Cannot read() on the file: ", errno);
			offsetBegin += result;
			buf = static_cast<uint8_t*>(buf) + result;
		}
	}

	void FileWithPages::flush()
	{
		if (pim->fileWasResized.exchange(false)) {
			int errnum = fsync(pim->file);
			if ( errnum != 0 ) pim->throw_error("fsync() failed", errnum);
		} else {
			int errnum = fdatasync(pim->file);
			if ( errnum != 0 ) pim->throw_error("fdatasync() failed", errnum);
		}
	}

}


