#ifndef GENOMEDB_PAGESMANAGER_HPP_
#define GENOMEDB_PAGESMANAGER_HPP_

#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <stdexcept>

	typedef uint32_t tPageId;

template<unsigned pageSize>
struct BufferT
{
	uint8_t* data;
	tPageId  firstPageId;
	tPageId  pagesCount;
	explicit BufferT(uint8_t* pData = nullptr, tPageId pFirstPageId = 0, tPageId pPagesCount = 0)
	: data(pData), firstPageId(pFirstPageId), pagesCount(pPagesCount) {}
	BufferT region(unsigned offsetInPages)
	{
		if (offsetInPages >= pagesCount) throw std::out_of_range("BufferT::region(offsetInPages)");
		return BufferT(data+pageSize*1ull*offsetInPages, firstPageId+offsetInPages, pagesCount-offsetInPages);
	}
	BufferT region(unsigned offsetInPages, unsigned sizeInPages)
	{
		if (offsetInPages+sizeInPages > pagesCount) throw std::out_of_range("BufferT::region(offsetInPages,sizeInPages)");
		return BufferT(data+pageSize*1ull*offsetInPages, firstPageId+offsetInPages, sizeInPages);
	}
	uint64_t offsetInBytes() const { return (static_cast<uint64_t>(pageSize)*firstPageId); }
	uint64_t sizeInBytes() const { return (static_cast<uint64_t>(pageSize)*pagesCount); }
	bool operator<(BufferT const & b) const
	{
		if (firstPageId != b.firstPageId) return (firstPageId < b.firstPageId);
		if (pagesCount  != b.pagesCount ) return (pagesCount  < b.pagesCount );
		return (data < b.data);
	}
};

template<unsigned pageSize>
class PagesManager
{
private:
	struct Pim;
	Pim * pim;
public:
	typedef BufferT<pageSize> Buffer;
	struct Statistics
	{
		uint64_t readsCount   = 0;
		uint64_t readsTimeMs  = 0;
		uint64_t writesCount  = 0;
		uint64_t writesTimeMs = 0;
		uint64_t synchCount   = 0;
		uint64_t synchTimeMs  = 0;
	};
	PagesManager(std::string const & path, bool synchronized = true, uint64_t cacheMemoryInMegabytes = 1024);
	~PagesManager();
	// ========== these functions below are synchronized

	unsigned numberOfPages() const;

	// create empty pages (filled with zeroes) and bind it to given id, nothing is read or write from the file
	Buffer allocateBuffer(unsigned pagesCount);
	// load pages with given id from the file (or cache)
	Buffer loadBuffer(tPageId firstPage, unsigned pagesCount);
	// created or loaded pages cannot overlap!

	// save given pages to the file, given vector is sorted
	// firstPageId DOES NOT have to be the same as returned by createPages or loadPages
	// but given range must be included in existing pages
	void synchBuffer(Buffer);
	void synchBuffers(std::vector<Buffer>);

	// label given pages as free, deleted pages are released
	// firstPageId DOES have to be the same as returned by createPages or loadPages (all pages from allocated range are affected)
	void deleteBuffer(Buffer);
	// release pages, a buffer assigned to them may be deleted (or stays in cache)
	// firstPageId DOES have to be the same as returned by createPages or loadPages (all pages from allocated range are affected)
	void releaseBuffer(Buffer);

	void synchronize();

	// shrink file and set pages that can be reused
	void setFreePages(unsigned newNumberOfPages, std::map<tPageId,unsigned> const & freePages);

	Statistics readAndResetStatistics();
};



#endif /* GENOMEDB_PAGESMANAGER_HPP_ */
