#ifndef FLATDB_FILEWITHPAGES_HPP_
#define FLATDB_FILEWITHPAGES_HPP_

#include <string>
#include <map>

namespace flatDb {

	// class with low-level routines on the file seen as array of pages
	class FileWithPages
	{
	private:
		struct Pim;
		Pim * pim;
	public:
		unsigned const pageSize;
		// all these are not synchronized
		// at the beginning all pages in the file are marked as allocated (not free)
		FileWithPages(std::string const & path, unsigned pageSize, bool synchronized);
		~FileWithPages();
		// shrink file to given number of pages and overwrite the set of free pages (free = not allocated)
		void     setFreePages(unsigned newNumberOfPages, std::map<unsigned,unsigned> const & freePages);
		// return file size as number of pages (including both allocated and free pages)
		unsigned numberOfPages() const;
		// allocates given number of pages and returns number of the first page, adjust file size if needed
		unsigned allocatePages(unsigned pagesCount);
		// releases (mark as free) given range of pages, adjust file size if needed
		void     releasePages(unsigned pageId, unsigned pagesCount);
		// write/read are delegated to system routines, they can be called in many threads
		// these methods do not check if given pages are allocated or free
		void     writePages(unsigned pageId, unsigned pagesCount, void const *);
		void     readPages (unsigned pageId, unsigned pagesCount, void *);
		void flush();
	};

}


#endif /* FLATDB_FILEWITHPAGES_HPP_ */
