#ifndef INDEXNODE_HPP_
#define INDEXNODE_HPP_

#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <functional>
#include <limits>
#include <mutex>
#include <condition_variable>
#include "../apiDb/db.hpp"
#include "FixedHeightTree.hpp"
#include "PagesManager.hpp"

template<class Container>
struct Range
{
	typename Container::const_iterator begin;
	typename Container::const_iterator end;
//	bool initialized;
	Range() {} // : initialized(false) {}
	Range(Container const & cont) : begin(cont.begin()), end(cont.end()) {}
	Range(typename Container::const_iterator pBegin) : begin(pBegin), end(pBegin) {} //, initialized(true) {}
//	void extend(typename Container::const_iterator it)
//	{
//		if (! initialized) {
//			begin = end = it;
//			initialized = true;
//		} else if (it != end) {
//			throw std::logic_error("Range::extend(iterator)");
//		}
//		++end;
//	}
};

template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
class IndexNode
{
public:
	typedef DatabaseT<tKey,globalKeySize,dataPageSize> GlobalTree;
	typedef BufferT<dataPageSize>  DataBuffer;
	typedef BufferT<1536> IndexBuffer;
	typedef Range<std::vector<RecordT<tKey>*>> RecordsRange;
private:
	typedef FixedHeightTree<unsigned,8,dataPageSize> LocalTree;
	typedef FixedHeightTree<tKey,8*globalKeySize,dataPageSize> FullTree;

	typename GlobalTree::Pim * globalTree;
	tPageId fPageId;
	uint8_t * fBuffer;

	unsigned fIndexNodeLevel;
	tKey fIndexNodeLeftmostKey;
	unsigned fIndexNodeRevision;

	uint8_t * fLevels;
	tPageId * fLeaves;
	tPageId fFirstDataPageId;
	unsigned fDataPagesCount;
	std::unordered_map<tPageId,IndexNode<dataPageSize,globalKeySize,tKey>*> offspringIndexNodes;

	void initializeVariables();
	void saveVariables();
	void switchToNextRevisionOfIndexNode();

	// --------------------------- synchronization
	std::mutex accessMutex;
	unsigned readOnlyLocks = 0;
	bool readWriteLock = false;
	std::condition_variable waitingReadOnlyThreads;
	std::condition_variable waitingReadWriteThreads;
	void lockReadOnly();
	void lockReadWrite();
	void unlock();
	// ----------------------------------

	inline unsigned calculateLocalKey(tKey globalKey) const { return ((globalKey >> ((globalKeySize-1-fIndexNodeLevel)*8)) % 256); }

	// --------------- helpers
	// overwrite given subtree by given records
	// new pages are created and returned (should be saved & released)
	// new index nodes are registered in globalTree (if created)
	void overwriteSubtree( std::vector<RecordT<tKey>*> const & records
			, std::vector<DataBuffer> & newDataPages
			, std::vector<IndexBuffer> & newIndexPages
			, std::map<tPageId, std::vector<RecordT<tKey>*>> & newBuckets
			);

	void deleteSubtree( tKey pKey );
	// load records from node
	void readRecordsFromDataNode(uint8_t * buffer, std::map<tKey,std::vector<RecordT<tKey>*>> &) const;
	void readRecordsFromDataNode(uint8_t * buffer, std::vector<RecordT<tKey>*> &, tKey first = 0, tKey last = std::numeric_limits<tKey>::max()) const;
	// save records to node, the vector is emptied and the memory is released
	void writeRecordsToDataNode(uint8_t * buffer, std::vector<RecordT<tKey>*>  &) const;
	// calculate the length of data node with given records
	unsigned lengthOfDataNode(std::vector<RecordT<tKey>*> const &) const;

public:
	// create new node, returned buffers must be saved
	IndexNode( typename GlobalTree::Pim * pTree, unsigned level, std::vector<RecordT<tKey>*> const & records
			 , std::vector<DataBuffer> & newDataPages, std::vector<IndexBuffer> & newIndexPages );
	// load node from file
	IndexNode( typename GlobalTree::Pim * pTree, tPageId pageId);
	// destructor
	~IndexNode();
	// standard access function
	// returns true if query terminated
	bool readRecordsInOrder( typename GlobalTree::tReadFunction visitor, std::vector<RecordT<tKey> const *> & buffer
							,  tKey first, tKey last, unsigned minChunkSize );
	// records from given range must by sorted by keys
	void readRecords ( typename std::vector<RecordT<tKey>*>::const_iterator recordsBegin, typename std::vector<RecordT<tKey>*>::const_iterator recordsEnd, typename GlobalTree::tReadByKeyFunction   visitor);
	void writeRecords( typename std::vector<RecordT<tKey>*>::const_iterator recordsBegin, typename std::vector<RecordT<tKey>*>::const_iterator recordsEnd, typename GlobalTree::tUpdateByKeyFunction visitor);

	tKey getTheLargestKey();
	void markUsedDataPages(std::vector<bool> &) const;
};


#endif /* INDEXNODE_HPP_ */
