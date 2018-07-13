#include "IndexNode.hpp"

#include "../commonTools/assert.hpp"
#include "../commonTools/bytesLevel.hpp"

#include <cstring>
#include <list>


#define XX IndexNodeT<tKey>


namespace flatDb {


	template<typename tKey>
	void XX::calculateKeyBinsFromContent(std::vector<std::pair<tKey,uint8_t const*>> const & content, std::vector<typename XX::Bin> & outputBins) const
	{
		outputBins.clear();
		if ( content.empty() ) return;
		outputBins.reserve(content.size());

		Bin b;
		b.firstKey = content.front().first;
		b.maxKeyOffset = 0;
		b.recordsCount = b.bytesCount = 0;
		outputBins.push_back(b);

		for ( auto kv: content ) {
			uint8_t const* ptr = kv.second;
			unsigned const dataLength = readUnsignedIntVarSize<1,1,unsigned>(ptr);
			Bin b;
			b.firstKey = kv.first;
			b.maxKeyOffset = 0;
			b.recordsCount = 1;
			b.bytesCount = dataLength + lengthUnsignedIntVarSize(dataLength);
			ASSERT(b.bytesCount > 0);
			if (outputBins.back().firstKey == b.firstKey) {
				outputBins.back() += b;
			} else {
				outputBins.push_back(b);
			}
		}
	}


	template<typename tKey>
	std::vector<typename XX::Bin> XX::divideIntoPages(std::vector<typename XX::Bin> const & keyBins) const
	{
		if (keyBins.empty()) return keyBins;

		// leftmost aligned configuration
		std::vector<Bin> pageBins;
		std::vector<unsigned> leftAlignedStartIndexes;
		pageBins.push_back(keyBins.front());
		leftAlignedStartIndexes.push_back(0);
		for (unsigned i = 1; i < keyBins.size(); ++i) {
			Bin bs2 = pageBins.back();
			bs2 += keyBins[i];
			if (bs2.totalSize() > dataPageSize) {
				pageBins.push_back(keyBins[i]);
				leftAlignedStartIndexes.push_back(i);
			} else {
				pageBins.back() = bs2;
			}
		}

		// calculate free space per page
		unsigned freeSpacePerPage = 0;
		for (Bin const & b: pageBins) {
			freeSpacePerPage += (dataPageSize - b.totalSize());
		}
		freeSpacePerPage /= pageBins.size();

		// build balanced bins
		pageBins.clear();
		unsigned iKeyBin = keyBins.size();
		while ( ! leftAlignedStartIndexes.empty() ) {
			pageBins.push_back( keyBins[--iKeyBin] );
			unsigned const leftAlignedStartIndex = leftAlignedStartIndexes.back();
			leftAlignedStartIndexes.pop_back();
			while ( iKeyBin > leftAlignedStartIndex ) {
				pageBins.back() += keyBins[--iKeyBin];
			}
			while ( iKeyBin > 0 && (pageBins.back() + keyBins[iKeyBin-1]).totalSize() <= dataPageSize - freeSpacePerPage ) {
				pageBins.back() += keyBins[--iKeyBin];
			}
		}
		std::reverse(pageBins.begin(), pageBins.end());

		return pageBins;
	}


	template<typename tKey>
	typename XX::SP XX::createEmpty(XX::Scheduler* scheduler, unsigned keySize, unsigned dataPageSize, unsigned indexPageSize)
	{
		SP obj(new IndexNodeT<tKey>( scheduler, keySize, dataPageSize, indexPageSize, 0, true ));
		return obj;
	}


	template<typename tKey>
	typename XX::SP XX::createFromBuffer(XX::Scheduler* scheduler, unsigned keySize, unsigned dataPageSize, unsigned indexPageSize, bool firstCopy, void const * ptr)
	{
		unsigned const maxNumberOfDataNodes = (indexPageSize - 8) / (2 * keySize + 7);
		uint8_t const * p = reinterpret_cast<uint8_t const *>(ptr);

		// parse
		unsigned const crc32 = readUnsignedInteger<4,unsigned>(p);
		if ( crc32 != CRC32(p,indexPageSize-4)) {
			throw std::runtime_error("Incorrect CRC32 for index page");
		}
		unsigned const revision = readUnsignedInteger<4,unsigned>(p);
		std::vector<typename DataNode::SP> entries;
		for (unsigned i = 0; i < maxNumberOfDataNodes; ++i) {
			Bin bin;
			bin.firstKey = readUnsignedInteger<tKey>(p, keySize);
			bin.maxKeyOffset = readUnsignedInteger<tKey>(p, keySize) - bin.firstKey;
			uint64_t recordsCount_bytesCount_pageId = readUnsignedInteger<7,uint64_t>(p);
			unsigned const pageId = recordsCount_bytesCount_pageId % (1u << 18);
			if (pageId == 0) break;
			recordsCount_bytesCount_pageId >>= 18;
			bin.bytesCount = recordsCount_bytesCount_pageId % (1u << 19);
			recordsCount_bytesCount_pageId >>= 19;
			bin.recordsCount = recordsCount_bytesCount_pageId;
			entries.push_back( DataNode::createFromStorage(scheduler,pageId,bin) );
		}

		SP obj(new IndexNodeT<tKey>( scheduler, keySize, dataPageSize, indexPageSize, revision, firstCopy ));
		obj->entries.swap(entries);
		return obj;
	}


	template<typename tKey>
	typename XX::SP XX::createSecondCopy() const
	{
		SP obj(new IndexNodeT<tKey>( scheduler, keySize, dataPageSize, indexPageSize, revision+1, !firstCopy ));
		obj->entries = entries;
		return obj;
	}


	template<typename tKey>
	void XX::writeToBuffer(void * ptr) const
	{
		unsigned const maxNumberOfDataNodes = (indexPageSize - 8) / (2 * keySize + 7);
		ASSERT(maxNumberOfDataNodes >= entries.size());

		// write page but CRC
		uint8_t * pBegin = reinterpret_cast<uint8_t*>(ptr);
		uint8_t * p = pBegin + 4;
		writeUnsignedInteger<4>(p, revision);
		for (auto dn: entries) {
			writeUnsignedInteger(p, dn->bin.firstKey, keySize);
			writeUnsignedInteger(p, dn->bin.lastKey(), keySize);
			uint64_t recordsCount_bytesCount_pageId = dn->bin.recordsCount; // 19 bits
			recordsCount_bytesCount_pageId <<= 19;
			recordsCount_bytesCount_pageId += dn->bin.bytesCount; // 19 bits
			recordsCount_bytesCount_pageId <<= 18;
			recordsCount_bytesCount_pageId += dn->pageId; // 18 bits
			writeUnsignedInteger<7>(p, recordsCount_bytesCount_pageId);
		}
		std::memset(p, 0, pBegin + indexPageSize - p);

		// write CRC32
		unsigned const crc32 = CRC32( pBegin+4, indexPageSize-4 );
		writeUnsignedInteger<4>(pBegin, crc32);
	}


	template<typename tKey>
	std::vector<typename XX::DataNode::SP> XX::reorganize( std::vector<std::pair<unsigned,unsigned>> dataNodesIds )
	{
		ASSERT( ! dataNodesIds.empty() );
		std::sort(dataNodesIds.begin(), dataNodesIds.end());

		// ===== create new vector of data nodes
		std::vector<typename DataNode::SP> newEntries;
		newEntries.reserve(entries.size());
		std::vector<typename DataNode::SP> removedDataNodes;
		unsigned nextDataNodeToCopy = 0;
		for ( auto iI = dataNodesIds.begin();  iI != dataNodesIds.end();  ) {

			std::vector<std::pair<tKey,uint8_t const*>> allRecords;

			// ----- find first data node
			for (;  iI != dataNodesIds.end() && ! entries[iI->first]->tryToPrepareForReorganize(allRecords);  ++iI );
			if (iI == dataNodesIds.end()) break;

			// ----- read whole range of data nodes to overwrite
			unsigned first = iI->first;
			unsigned last = first;
			++iI;
			while (iI != dataNodesIds.end() && iI->first == last + 1 && entries[iI->first]->tryToPrepareForReorganize(allRecords)) {
				++iI;
				++last;
			}

			// ----- copy untouched nodes from original vector
			for ( ;  nextDataNodeToCopy < first;  ++nextDataNodeToCopy ) {
				newEntries.push_back( entries[nextDataNodeToCopy] );
			}
			nextDataNodeToCopy = last + 1;

			// ----- create new nodes
			// calculate key page bins
			std::vector<Bin> keyBins;
			calculateKeyBinsFromContent( allRecords, keyBins );
			std::vector<Bin> pageBins = divideIntoPages(keyBins);
			// build new DataNodes
			auto iR = allRecords.begin();
			for (Bin const & b: pageBins) {
				auto iR2 = std::upper_bound(iR, allRecords.end(), b.lastKey(), [](tKey const& k,std::pair<tKey,uint8_t const*> const& r)->bool{return (k<r.first);} );
				std::vector<std::pair<tKey,uint8_t const*>> records;
				records.reserve(iR2-iR);
				records.assign(iR,iR2);
				newEntries.push_back( DataNode::createNew(scheduler,records) );
				ASSERT( newEntries.back()->bin == b );
				iR = iR2;
			}
			allRecords.clear();

			// ----- update removed nodes
			for ( unsigned i = first;  i <= last;  ++i ) {
				removedDataNodes.push_back( entries[i] );
				entries[i]->freeMemory();
			}
		}
		// ----- copy the rest of the entries
		for ( ;  nextDataNodeToCopy < entries.size();  ++nextDataNodeToCopy ) {
			newEntries.push_back(entries[nextDataNodeToCopy]);
		}

		// ===== the case when database is empty now
		if (newEntries.empty()) {
			newEntries.push_back( DataNode::createEmpty(this->scheduler,0) );
		}

		// ===== replace entries with new one & return removed nodes
		entries.swap(newEntries);
		return removedDataNodes;
	}


	template class IndexNodeT<uint32_t>;
	template class IndexNodeT<uint64_t>;
}

