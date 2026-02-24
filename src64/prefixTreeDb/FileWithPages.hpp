#ifndef GENOMEDB_FILEWITHPAGES_HPP_
#define GENOMEDB_FILEWITHPAGES_HPP_

#include <string>
#include <map>

namespace genomeDb {

	template<unsigned pageSize>
	class FileWithPages
	{
	private:
		struct Pim;
		Pim * pim;
	public:
		// all these are not synchronized
		// at the beginning all pages in the file are allocated
		FileWithPages(std::string const & path, bool synchronized);
		~FileWithPages();
		unsigned numberOfPages() const;
		unsigned allocatePages(unsigned pagesCount);
		void     releasePages(unsigned pageId, unsigned pagesCount);
		// write/read are delegated to system routines, can be called in many threads
		void     writePages(unsigned pageId, unsigned pagesCount, void const *);
		void     readPages(unsigned pageId, unsigned pagesCount, void *);
		// shrink file and set pages that can be reused
		void setFreePages(unsigned newNumberOfPages, std::map<unsigned,unsigned> const & freePages);
	};

}


#endif /* GENOMEDB_FILEWITHPAGES_HPP_ */
