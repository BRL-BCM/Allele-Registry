#include "PrefixTreeDbPim.hpp"
#include "../commonTools/bytesLevel.hpp"

#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

typedef unsigned tLocalKey;
unsigned const dataNodeHeaderLength = 2;

// ----------------------- tools

template<typename tKey, class Record>
inline std::map<tKey,std::vector<Record*>> groupByKeys( Range<std::vector<Record*>> range )
{
	std::map<tKey,std::vector<Record*>> results;
	for (auto iR = range.begin; iR != range.end; ++iR) results[(*iR)->key].push_back(*iR);
	return results;
}

template<typename Element>
inline void append(std::vector<Element> & v1, std::vector<Element> const & v2)
{
	v1.insert( v1.end(), v2.begin(), v2.end() );
}

// ================================== indexNode core data

	template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
	void IndexNode<dataPageSize,globalKeySize,tKey>::initializeVariables()
	{
		uint8_t const * ptr = fBuffer + 4;
		fIndexNodeRevision = readUnsignedInteger<unsigned>(ptr,4);
		fIndexNodeLevel = readUnsignedInteger<unsigned>(ptr,4);
		fIndexNodeLeftmostKey = readUnsignedInteger<tKey>(ptr,20);
		fLevels = fBuffer + 256;
		fLeaves = reinterpret_cast<tPageId*>(fLevels + 256);
		LocalTree tree(fLevels,fLeaves);
		fDataPagesCount  = tree.getBucketsCount().first;
		unsigned maxBucketId = tree.getMaxBucketId().first;
		if (maxBucketId == LocalTree::emptyLeaf) {
			fFirstDataPageId = 0;
		} else {
			fFirstDataPageId = tree.getMaxBucketId().first + 1 - fDataPagesCount;
		}
		unsigned const size = LocalTree::subtreeLeavesCount();
		for (unsigned i = 0; i < size; ++i) {
			if (fLevels[i] < LocalTree::treeHeight) continue;
			if (fLeaves[i] == LocalTree::emptyLeaf) continue;
			offspringIndexNodes[fLeaves[i]] = new IndexNode(globalTree,fLeaves[i]);
		}
	}

	template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
	void IndexNode<dataPageSize,globalKeySize,tKey>::saveVariables()
	{
		uint8_t * ptr = fBuffer + 4;
		writeUnsignedInteger(ptr, fIndexNodeRevision, 4);
		writeUnsignedInteger(ptr, fIndexNodeLevel, 4);
		writeUnsignedInteger(ptr, fIndexNodeLeftmostKey, 20);
		unsigned const crc32 = CRC32( fBuffer+4, 3*512-4 );
		ptr = fBuffer;
		writeUnsignedInteger(ptr, crc32, 4);
	}

	template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
	void IndexNode<dataPageSize,globalKeySize,tKey>::switchToNextRevisionOfIndexNode()
	{
		if ( (++fIndexNodeRevision) % 2 == 1) {
			fBuffer += 3*512;
		} else {
			fBuffer -= 3*512;
		}
		uint8_t * oldLevels = fLevels;
		tPageId * oldLeaves = fLeaves;
		fLevels = fBuffer + 256;
		fLeaves = reinterpret_cast<tPageId*>(fLevels + 256);
		memcpy(fLevels, oldLevels, 256*sizeof(uint8_t));
		memcpy(fLeaves, oldLeaves, 256*sizeof(tPageId));
	}

// ===============================  synchronization

	template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
	void IndexNode<dataPageSize,globalKeySize,tKey>::lockReadOnly()
	{
		std::unique_lock<std::mutex> synch(accessMutex);
		while (readWriteLock) waitingReadOnlyThreads.wait(synch);
		++readOnlyLocks;
	}

	template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
	void IndexNode<dataPageSize,globalKeySize,tKey>::lockReadWrite()
	{
		std::unique_lock<std::mutex> synch(accessMutex);
		while (readWriteLock || readOnlyLocks > 0) waitingReadWriteThreads.wait(synch);
		readWriteLock = true;
	}

	template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
	void IndexNode<dataPageSize,globalKeySize,tKey>::unlock()
	{
		std::lock_guard<std::mutex> synch(accessMutex);
		if (readWriteLock) {
			readWriteLock = false;
			waitingReadOnlyThreads.notify_all();
			waitingReadWriteThreads.notify_one();
		} else {
			if (--readOnlyLocks == 0) waitingReadWriteThreads.notify_one();
		}
	}

// ===============================  constructors

	// create new node
	template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
	IndexNode<dataPageSize,globalKeySize,tKey>::IndexNode( typename IndexNode<dataPageSize,globalKeySize,tKey>::GlobalTree::Pim * pTree
			, unsigned level
			, std::vector<RecordT<tKey>*> const & records
			, std::vector<BufferT<dataPageSize>> & newDataPages
			, std::vector<BufferT<1536>> & newIndexPages
			) : globalTree(pTree) // create new index node
	{
		if (level >= globalKeySize) throw std::runtime_error("Index node level is larger than global key");
		if (level > 0 && records.empty()) throw std::runtime_error("Cannot create index node without records");
		{
			IndexBuffer buf = globalTree->indexPagesManager->allocateBuffer(2);
			fPageId = buf.firstPageId;
			fBuffer = buf.data;
			newIndexPages.push_back( buf );
		}

		initializeVariables();
		fIndexNodeLevel = level;

		std::map<tPageId, std::vector<RecordT<tKey>*>> newBuckets;
		if (records.empty()) {
			unsigned const size = LocalTree::subtreeLeavesCount();
			for (unsigned i = 0; i < size; ++i) fLeaves[i] = LocalTree::emptyLeaf;
		} else {
			fIndexNodeLeftmostKey = FullTree::subtreeLeftmostKey(records.front()->key, fIndexNodeLevel*LocalTree::treeHeight);
			overwriteSubtree(records, newDataPages, newIndexPages, newBuckets);
		}

		fDataPagesCount = newBuckets.size();
		if (fDataPagesCount > 0) {
			DataBuffer buf = globalTree->dataPagesManager->allocateBuffer(fDataPagesCount);
			fFirstDataPageId = buf.firstPageId;
			newDataPages.push_back( buf );

			std::map<tPageId,tPageId> old2new;
			unsigned const size = LocalTree::subtreeLeavesCount();
			for (unsigned i = 0; i < size; ++i) {
				if (fLeaves[i] == LocalTree::emptyLeaf) continue;
				if (fLevels[i] == LocalTree::treeHeight) continue;
				if ( old2new.count(fLeaves[i]) == 0 ) {
					unsigned const localIndex = old2new.size();
					tPageId const newPageId = fFirstDataPageId + localIndex;
					old2new[fLeaves[i]] = newPageId;
					writeRecordsToDataNode(buf.region(localIndex).data, newBuckets[fLeaves[i]]);
				}
				fLeaves[i] = old2new[fLeaves[i]];
			}
		}

		saveVariables();
	}

	 // load node from file
	template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
	IndexNode<dataPageSize,globalKeySize,tKey>::IndexNode( typename IndexNode<dataPageSize,globalKeySize,tKey>::GlobalTree::Pim * pTree, tPageId pageId)
	: globalTree(pTree), fPageId(pageId)
	{
		IndexBuffer buf = globalTree->indexPagesManager->loadBuffer(fPageId,2);
		fBuffer = buf.data;
		uint8_t const * ptr = fBuffer;
		unsigned checksum0 = readUnsignedInteger<4,unsigned>(ptr);
		unsigned revision0 = readUnsignedInteger<4,unsigned>(ptr);
		ptr = fBuffer + 3*512;
		unsigned checksum1 = readUnsignedInteger<4,unsigned>(ptr);
		unsigned revision1 = readUnsignedInteger<4,unsigned>(ptr);
		bool ok0 = (checksum0 == CRC32(fBuffer+4      ,3*512-4)) && (revision0 % 2 == 0);
		bool ok1 = (checksum1 == CRC32(fBuffer+4+3*512,3*512-4)) && (revision1 % 2 == 1);
		if (ok0 && ok1) {
			if (revision0 > revision1) ok1 = false; else ok0 = false;
		} else if ( ! ( ok0 || ok1 ) ) {
			throw std::runtime_error("Incorrect Index Page (CRC error)");
		}

		if (ok1) {
			fBuffer += 3*512;
		}

		initializeVariables();
	}

	template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
	IndexNode<dataPageSize,globalKeySize,tKey>::~IndexNode()
	{
		// TODO
	}

 	template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
 	void IndexNode<dataPageSize,globalKeySize,tKey>::readRecordsFromDataNode( uint8_t * buffer
			,  std::map<tKey,std::vector<RecordT<tKey>*>> & target) const
	{
		uint8_t const * ptr = buffer;
		unsigned const recordsCount = readUnsignedInteger<dataNodeHeaderLength,unsigned>(ptr);
		for (unsigned i = 0; i < recordsCount; ++i) {
			// read the key
			tKey const key = fIndexNodeLeftmostKey + readUnsignedInteger<tKey>(ptr, globalKeySize-fIndexNodeLevel);
			// read the record data size
			unsigned const recordSize = readUnsignedIntVarSize<1,1,unsigned>(ptr);
			// read the record
			uint8_t const * const prevPtr = ptr;
			RecordT<tKey> * r = globalTree->callbackCreateRecord(key,ptr);
			target[key].push_back(r);
			if (prevPtr + recordSize != ptr) throw std::logic_error("Record length do not match number of bytes read! (1) Expected: " + boost::lexical_cast<std::string>(recordSize) + " Key: " + boost::lexical_cast<std::string>(key) + " position: " + boost::lexical_cast<std::string>(ptr-buffer));
		}
	}


	template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
	void IndexNode<dataPageSize,globalKeySize,tKey>::readRecordsFromDataNode( uint8_t * buffer
			,  std::vector<RecordT<tKey>*> & target, tKey first, tKey last) const
	{
		uint8_t const * ptr = buffer;
		unsigned const recordsCount = readUnsignedInteger<dataNodeHeaderLength,unsigned>(ptr);
		for (unsigned i = 0; i < recordsCount; ++i) {
			// read the key
			tKey const key = fIndexNodeLeftmostKey + readUnsignedInteger<tKey>(ptr, globalKeySize-fIndexNodeLevel);
			// read the record data size
			unsigned const recordSize = readUnsignedIntVarSize<1,1,unsigned>(ptr);
			// check the key and skip the record if not needed
			if (key < first || key > last) {
				ptr += recordSize;
				continue;
			}
			// read the record
			uint8_t const * const prevPtr = ptr;
			RecordT<tKey> * r = globalTree->callbackCreateRecord(key,ptr);
			target.push_back(r);
			if (prevPtr + recordSize != ptr) throw std::logic_error("Record length do not match number of bytes read! (2)");
		}
	}

	template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
	void IndexNode<dataPageSize,globalKeySize,tKey>::writeRecordsToDataNode(uint8_t * buffer, std::vector<RecordT<tKey>*> & records) const
	{
		uint8_t * ptr = buffer;
		writeUnsignedInteger(ptr, records.size(), dataNodeHeaderLength);
		for (RecordT<tKey> * r: records) {
			// write the key
			writeUnsignedInteger(ptr, r->key - fIndexNodeLeftmostKey, globalKeySize-fIndexNodeLevel);
			// write the record data size
			unsigned const recordSize = r->dataLength();
			writeUnsignedIntVarSize<1,1>(ptr,recordSize);
			// write the record data
			uint8_t * const prevPtr = ptr;
			r->saveData(ptr);
			delete r;
			if (prevPtr + recordSize != ptr) throw std::logic_error("Record length do not match number of bytes written!");
		}
		records.clear();
	}

	template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
	unsigned IndexNode<dataPageSize,globalKeySize,tKey>::lengthOfDataNode(std::vector<RecordT<tKey>*> const & records) const
	{
		unsigned length = dataNodeHeaderLength;
		for (RecordT<tKey> const * r: records) {
			// the key
			length += (globalKeySize-fIndexNodeLevel);
			// the record data size
			unsigned const recordSize = r->dataLength();
			length += lengthUnsignedIntVarSize(recordSize);
			// the record data
			length += recordSize;
		}
		return length;
	}

	template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
	bool IndexNode<dataPageSize,globalKeySize,tKey>::readRecordsInOrder( typename GlobalTree::tReadFunction visitor
			, std::vector<RecordT<tKey> const *> & buffer, tKey first, tKey last, unsigned minChunkSize )
	{
		tKey const leafSize = static_cast<tKey>(1) << (8*(globalKeySize-fIndexNodeLevel-1));
		lockReadOnly();
		DataBuffer dataBuffer = globalTree->dataPagesManager->loadBuffer(fFirstDataPageId, fDataPagesCount);
		tPageId lastPageId = LocalTree::emptyLeaf;
		unsigned const numberOfLeaves = LocalTree::subtreeLeavesCount();
		for (unsigned i = 0; i < numberOfLeaves; ++i) {
			if (fIndexNodeLeftmostKey + i*leafSize > last) break;
			if (fIndexNodeLeftmostKey + (i+1)*leafSize <= first) continue;
			tPageId const pageId = fLeaves[i];
			if ( pageId == LocalTree::emptyLeaf ) continue;
			if (fLevels[i] == LocalTree::treeHeight) {
				if (this->offspringIndexNodes.at(fLeaves[i])->readRecordsInOrder(visitor, buffer, first, last, minChunkSize)) {
					globalTree->dataPagesManager->releaseBuffer(dataBuffer);
					unlock();
					return true;
				}
			} else if (pageId != lastPageId) {
				lastPageId = pageId;
				std::vector<RecordT<tKey>*> records;
				readRecordsFromDataNode(dataBuffer.region(fLeaves[i]-fFirstDataPageId).data, records, first, last);
				buffer.insert( buffer.end(), records.begin(), records.end() );
			}
		}

		globalTree->dataPagesManager->releaseBuffer(dataBuffer);
		unlock();

		bool lastCall = (fIndexNodeLevel == 0);
		if (buffer.size() >= minChunkSize || lastCall) {
			try {
				visitor(buffer, lastCall);
			} catch (...) { // TODO - log it somewhere
				// this is the end, we stop after the first exception
				return true;
			}
			for (auto b: buffer) delete b;
			buffer.clear();
			if (lastCall) return true;
		}

		return false;
	}

	template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
	void IndexNode<dataPageSize,globalKeySize,tKey>::IndexNode::readRecords
		( typename std::vector<RecordT<tKey>*>::const_iterator recordsBegin
		, typename std::vector<RecordT<tKey>*>::const_iterator recordsEnd
		, typename GlobalTree::tReadByKeyFunction visitor )
	{
		PagesManager<dataPageSize> * dataFile  = globalTree->dataPagesManager;
		lockReadOnly();
		LocalTree tree(fLevels, fLeaves);

		// =================== assign records to nodes
		// assign records to page ids
		std::map<tPageId,RecordsRange> indexNodesToVisit;
		std::map<tPageId,RecordsRange> dataNodesToVisit;
		std::vector<RecordsRange> unmatchedRecords;
		// splitting records into nodes
		for (typename std::vector<RecordT<tKey>*>::const_iterator iR = recordsBegin; iR < recordsEnd; ) {
			tLocalKey const localKey = calculateLocalKey((*iR)->key);
			tLocalKey const leftmostKey = LocalTree::subtreeLeftmostKey(localKey,fLevels[localKey]);
			tLocalKey const nextLeftmostKey = leftmostKey + LocalTree::subtreeLeavesCount(fLevels[localKey]);
			RecordsRange range(iR);
			bool hit = false;  // there is a data for at least one of given key
			for (; iR < recordsEnd; ++iR) {
				tLocalKey const currLocalKey = calculateLocalKey((*iR)->key);
				if (currLocalKey >= nextLeftmostKey) break;
				if (fLeaves[currLocalKey] != LocalTree::emptyLeaf) hit = true;
			}
			range.end = iR;
			tPageId pageId = tree.getBucketId(localKey);
			if (fLevels[localKey] == LocalTree::treeHeight) {
				if (pageId == LocalTree::emptyLeaf) {
					unmatchedRecords.push_back(range);
				} else {
					indexNodesToVisit[pageId] = range;
				}
			} else {
				if (pageId == LocalTree::emptyLeaf) {
					unmatchedRecords.push_back(range);
				} else if (! hit) {
					unmatchedRecords.push_back(range);
				} else {
					dataNodesToVisit[pageId] = range;
				}
			}
		}

		// ====== process index subnodes
		for (auto & kv: indexNodesToVisit) {
			IndexNode<dataPageSize,globalKeySize,tKey> * indexNode = offspringIndexNodes[kv.first];
			RecordsRange range = kv.second;
			auto task = [indexNode,range,visitor]() { indexNode->readRecords(range.begin, range.end, visitor); };
			globalTree->tasksManager->addTask(task);
		}

		// ---- load data nodes if needed
		if ( ! dataNodesToVisit.empty() ) {
			DataBuffer dataBuffer = dataFile->loadBuffer(fFirstDataPageId, fDataPagesCount);
			// ====== process data subnodes
			for (auto & kv: dataNodesToVisit) {
				std::map<tKey,std::vector<RecordT<tKey>*>> currentRecords;
				readRecordsFromDataNode(dataBuffer.region(kv.first-fFirstDataPageId).data, currentRecords);
				std::map<tKey,std::vector<RecordT<tKey>*>> searchedRecords = groupByKeys<tKey,RecordT<tKey>>( kv.second );
				for (auto const & kv2: searchedRecords) {
					std::vector<RecordT<tKey> const *> currentRecords2(currentRecords[kv2.first].begin(), currentRecords[kv2.first].end());
					visitor(currentRecords2, kv2.second);
				}
				for (auto kv: currentRecords) for (auto r: kv.second) delete r;
			}
			dataFile->releaseBuffer(dataBuffer);
		}

		// ====== process unmatched records
		for (auto & range: unmatchedRecords) {
			std::map<tKey,std::vector<RecordT<tKey>*>> searchedRecords = groupByKeys<tKey,RecordT<tKey>>( range );
			for (auto const & kv: searchedRecords) {
				visitor(std::vector<RecordT<tKey> const *>(), kv.second);
			}
		}

		unlock();
	}


template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
void IndexNode<dataPageSize,globalKeySize,tKey>::writeRecords
		( typename std::vector<RecordT<tKey>*>::const_iterator recordsBegin
		, typename std::vector<RecordT<tKey>*>::const_iterator recordsEnd
		, typename GlobalTree::tUpdateByKeyFunction visitor
		)
{
//	std::cout << "writeRecords for indexNode: " << fPageId  << std::endl;
//std::cout << "root level / min key = " << fIndexNodeLevel << " " << fIndexNodeLeftmostKey << std::endl;
//for (auto r: records) {
//	std::cout << r->key() << " " << std::flush;
//	if ( shiftRight(r->key(), 8*(globalKeySize-fIndexNodeLevel)) != shiftRight(fIndexNodeLeftmostKey, 8*(globalKeySize-fIndexNodeLevel)) ) {
//		throw std::runtime_error("record out of indexNode scope !!!");
//	}
//}
	PagesManager<dataPageSize> * dataFile  = globalTree->dataPagesManager;
	PagesManager<1536> * indexFile = globalTree->indexPagesManager;
	lockReadWrite();
	LocalTree tree(fLevels, fLeaves);
	DataBuffer dataBuffer;

	// =================== assign records to nodes
	// assign records to page ids
	std::map<tPageId,RecordsRange> indexNodesToVisit;
	std::map<tPageId,RecordsRange> dataNodesToVisit;
	std::map<tPageId,RecordsRange> dataNodesToExtend;
	// subtrees to create (key -> leftmost subtree key)
	std::map<tLocalKey,RecordsRange> indexNodesToCreate;
	std::map<tLocalKey,RecordsRange> dataNodesToCreate;
	// splitting records into nodes
	for (typename std::vector<RecordT<tKey>*>::const_iterator iR = recordsBegin; iR < recordsEnd; ) {
		tLocalKey const localKey = calculateLocalKey((*iR)->key);
		tLocalKey const leftmostKey = LocalTree::subtreeLeftmostKey(localKey,fLevels[localKey]);
		tLocalKey const nextLeftmostKey = leftmostKey + LocalTree::subtreeLeavesCount(fLevels[localKey]);
		RecordsRange range(iR);
		bool hit = false;  // there is a data for at least one of given key
		for (; iR < recordsEnd; ++iR) {
			tLocalKey const currLocalKey = calculateLocalKey((*iR)->key);
			if (currLocalKey >= nextLeftmostKey) break;
			if (fLeaves[currLocalKey] != LocalTree::emptyLeaf) hit = true;
		}
		range.end = iR;
		tPageId pageId = tree.getBucketId(localKey);
		if (fLevels[localKey] == LocalTree::treeHeight) {
			if (pageId == LocalTree::emptyLeaf) {
				indexNodesToCreate[localKey] = range;
			} else {
				indexNodesToVisit[pageId] = range;
			}
		} else {
			if (pageId == LocalTree::emptyLeaf) {
				dataNodesToCreate[leftmostKey] = range;
			} else if (! hit) {
				dataNodesToExtend[pageId] = range;
			} else {
				dataNodesToVisit[pageId] = range;
			}
		}
	}

	std::vector<std::vector<RecordT<tKey>*>> subtreesToOverwrite;
	std::vector<tKey> subtreesToDelete;

	// ====== process index subnodes
	for (auto & kv: indexNodesToVisit) {
		IndexNode<dataPageSize,globalKeySize,tKey> * indexNode = offspringIndexNodes[kv.first];
		RecordsRange range = kv.second;
		auto task = [indexNode,range,visitor]() { indexNode->writeRecords(range.begin, range.end, visitor); };
		globalTree->tasksManager->addTask(task);
	}

	for (auto & kv: indexNodesToCreate) {
		std::map<tKey,std::vector<RecordT<tKey>*>> searchedRecords = groupByKeys<tKey,RecordT<tKey>>( kv.second );
		std::vector<RecordT<tKey>*> newRecords;
		for (auto const & kv2: searchedRecords) {
			std::vector<RecordT<tKey>*> temp;
			if ( visitor(temp, kv2.second) ) append(newRecords, temp);
		}
		if (! newRecords.empty()) {
			subtreesToOverwrite.push_back(std::move(newRecords));
		}
	}

	// ---- load data nodes if needed
	if ( ! dataNodesToVisit.empty() ) dataBuffer = dataFile->loadBuffer(fFirstDataPageId, fDataPagesCount);

	// ====== process data subnodes
	for (auto & kv: dataNodesToVisit) {
		std::map<tKey,std::vector<RecordT<tKey>*>> currentRecords;
		readRecordsFromDataNode(dataBuffer.region(kv.first-fFirstDataPageId).data, currentRecords);
		std::map<tKey,std::vector<RecordT<tKey>*>> searchedRecords = groupByKeys<tKey,RecordT<tKey>>( kv.second );
		bool changes = false;
		for (auto const & kv2: searchedRecords) {
			if ( visitor(currentRecords[kv2.first], kv2.second) ) changes = true;
		}
		if (changes) {
			std::vector<RecordT<tKey>*> newRecords;
			for (auto const & kv2: currentRecords) append(newRecords, kv2.second);
			if (newRecords.empty()) {
				// delete node
				subtreesToDelete.push_back(searchedRecords.begin()->first);
			} else {
				subtreesToOverwrite.push_back(std::move(newRecords));
			}
		} else {
			for (auto kv: currentRecords) for (auto r: kv.second) delete r;
		}
	}

	for (auto & kv: dataNodesToExtend) {
		std::map<tKey,std::vector<RecordT<tKey>*>> searchedRecords = groupByKeys<tKey,RecordT<tKey>>( kv.second );
		std::vector<RecordT<tKey>*> newRecords;
		for (auto const & kv2: searchedRecords) {
			std::vector<RecordT<tKey>*> temp;
			if ( visitor(temp, kv2.second) ) append(newRecords, temp);
		}
		if (! newRecords.empty()) {
			if (dataBuffer.pagesCount == 0) dataBuffer = dataFile->loadBuffer(fFirstDataPageId, fDataPagesCount);
			readRecordsFromDataNode(dataBuffer.region(kv.first-fFirstDataPageId).data, newRecords);
			subtreesToOverwrite.push_back(std::move(newRecords));
		}
	}

	for (auto & kv: dataNodesToCreate) {
		std::map<tKey,std::vector<RecordT<tKey>*>> searchedRecords = groupByKeys<tKey,RecordT<tKey>>( kv.second );
		std::vector<RecordT<tKey>*> newRecords;
		for (auto const & kv2: searchedRecords) {
			std::vector<RecordT<tKey>*> temp;
			if ( visitor(temp, kv2.second) ) append(newRecords, temp);
		}
		if (! newRecords.empty()) {
			subtreesToOverwrite.push_back(std::move(newRecords));
		}
	}

	if (! (subtreesToOverwrite.empty() && subtreesToDelete.empty()) ) {
		// load data if not loaded yet
		if (dataBuffer.pagesCount == 0) dataBuffer = dataFile->loadBuffer(fFirstDataPageId, fDataPagesCount);
		std::vector<DataBuffer> newDataPages;
		std::vector<IndexBuffer> newIndexPages;
		// save changes
		switchToNextRevisionOfIndexNode();
		std::map<tPageId, std::vector<RecordT<tKey>*>> newBuckets;
		for (auto const & subtree: subtreesToOverwrite) {
			overwriteSubtree(subtree, newDataPages, newIndexPages, newBuckets);
		}
		for (auto key: subtreesToDelete) {
			deleteSubtree(key);
		}
		LocalTree tree(fLevels,fLeaves);
		unsigned const newDataPagesCount = tree.getBucketsCount().first;
		DataBuffer newDataBuffer = dataFile->allocateBuffer(newDataPagesCount);
		unsigned const size = tree.subtreeLeavesCount();
		std::map<tPageId,tPageId> oldToNew;
		for (unsigned i = 0; i < size; ++i) {
			if (fLeaves[i] == LocalTree::emptyLeaf) continue;
			if (fLevels[i] == LocalTree::treeHeight) continue;
			if ( oldToNew.count(fLeaves[i]) == 0 ) {
				unsigned const localKey = oldToNew.size();
				tPageId const newPageId = newDataBuffer.firstPageId + localKey;
				oldToNew[fLeaves[i]] = newPageId;
				// fill the data page
				if (newBuckets.count(fLeaves[i])) {
					writeRecordsToDataNode(newDataBuffer.region(localKey).data, newBuckets[fLeaves[i]]);
				} else {
					memcpy( newDataBuffer.region(localKey).data, dataBuffer.region(fLeaves[i]-fFirstDataPageId).data, dataPageSize);
				}
			}
			fLeaves[i] = oldToNew[fLeaves[i]];
		}
		fFirstDataPageId = newDataBuffer.firstPageId;
		fDataPagesCount = newDataBuffer.pagesCount;
		saveVariables();
		newDataPages.push_back(newDataBuffer);
		dataFile->synchBuffers(newDataPages);
		newDataPages.pop_back();
		indexFile->synchBuffers(newIndexPages);
		dataFile->synchronize();
		indexFile->synchronize();
		indexFile->synchBuffer( IndexBuffer(fBuffer, fPageId + fIndexNodeRevision%2, 1) );
		indexFile->synchronize();
		dataFile->deleteBuffer(dataBuffer);
		dataBuffer = newDataBuffer;
		for (auto & b: newDataPages) dataFile->releaseBuffer(b);
	}

	dataFile->releaseBuffer(dataBuffer);
	unlock();
}

template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
void IndexNode<dataPageSize,globalKeySize,tKey>::overwriteSubtree
	( std::vector<RecordT<tKey>*> const & records
	, std::vector<DataBuffer> & newDataPages
	, std::vector<IndexBuffer> & newIndexPages
	, std::map<tPageId, std::vector<RecordT<tKey>*>> & newBuckets
	)
{
	if (records.empty()) throw std::logic_error("overwriteSubtree for empty dataset");
	unsigned const rootLevel = fLevels[calculateLocalKey(records.front()->key)];
	std::map<tLocalKey,std::vector<RecordT<tKey>*>> recordsByLocaLKeys;
	for (RecordT<tKey> * r: records) recordsByLocaLKeys[calculateLocalKey(r->key)].push_back(r);

	LocalTree tree(fLevels,fLeaves);

	// clear leaves
	unsigned const subtreeLeftmost = tree.subtreeLeftmostKey(recordsByLocaLKeys.begin()->first, rootLevel);
	unsigned const subtreeSize     = tree.subtreeLeavesCount(rootLevel);

	for (unsigned i = 0; i < subtreeSize; ++i) {
		unsigned const key = subtreeLeftmost + i;
		if (fLeaves[key] == LocalTree::emptyLeaf) continue;
	}

	// calculate levels and buckets
	{
		std::vector<std::pair<unsigned,unsigned>> dataSizes;
		for (auto const & kv: recordsByLocaLKeys) {
			unsigned dataSize = dataNodeHeaderLength;
			dataSize += kv.second.size() * (globalKeySize-fIndexNodeLevel);
			for (auto const r: kv.second) {
				unsigned const recordLength = r->dataLength();
				dataSize += recordLength + lengthUnsignedIntVarSize<1,1>(recordLength);
			}
			dataSizes.push_back( std::make_pair(kv.first,dataSize) );
		}
		std::pair<tPageId,tPageId> temp = tree.getMaxBucketId();
		if (temp.first == LocalTree::emptyLeaf) temp.first = 0;
		if (temp.second == LocalTree::emptyLeaf) temp.second = 0;
		tPageId nextTempPageId = std::max(temp.first, temp.second) + 1;
		tree.overwriteSubtree(dataSizes, nextTempPageId, rootLevel);
	}

	// create IndexNodes if neccessary
	for (unsigned i = 0; i < subtreeSize; ++i) {
		unsigned const key = subtreeLeftmost + i;
		if (fLeaves[key] == LocalTree::emptyLeaf) continue;
		if (fLevels[key] < LocalTree::treeHeight) {
			append(newBuckets[fLeaves[key]], recordsByLocaLKeys[key]);
		} else {
			IndexNode * newIndexNode = new IndexNode(globalTree, fIndexNodeLevel+1, recordsByLocaLKeys[key], newDataPages, newIndexPages);
			fLeaves[key] = newIndexNode->fPageId;
			offspringIndexNodes[fLeaves[key]] = newIndexNode;
		}
	}
}


template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
void IndexNode<dataPageSize,globalKeySize,tKey>::deleteSubtree
	( tKey pKey	)
{
	unsigned localKey = calculateLocalKey(pKey);
	unsigned const rootLevel = fLevels[localKey];
	LocalTree tree(fLevels,fLeaves);

	// clear leaves
	unsigned const subtreeLeftmost = tree.subtreeLeftmostKey(localKey, rootLevel);
	unsigned const subtreeSize     = tree.subtreeLeavesCount(rootLevel);

	for (unsigned i = 0; i < subtreeSize; ++i) {
		unsigned const key = subtreeLeftmost + i;
		if (fLeaves[key] == LocalTree::emptyLeaf) continue;
		// if (fLevels[key] == LocalTree::treeHeight) offspringIndexNodes.erase(fLeaves[key]); // TODO - free index node
		fLeaves[key] = LocalTree::emptyLeaf;
	}
}


template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
tKey IndexNode<dataPageSize,globalKeySize,tKey>::getTheLargestKey()
{
	lockReadOnly();
	tKey result = fIndexNodeLeftmostKey;
	unsigned key = LocalTree::subtreeLeavesCount();
	do {
		--key;
		if (fLeaves[key] == LocalTree::emptyLeaf) continue;
		if (fLevels[key] == LocalTree::treeHeight) {
			result = offspringIndexNodes[fLeaves[key]]->getTheLargestKey();
			break;
		} else {
			DataBuffer dataBuffer = globalTree->dataPagesManager->loadBuffer(fFirstDataPageId, fDataPagesCount);
			std::vector<RecordT<tKey>*> records;
			readRecordsFromDataNode(dataBuffer.region(fLeaves[key]-fFirstDataPageId).data, records);
			for (auto r: records) if (r->key > result) result = r->key;
			globalTree->dataPagesManager->releaseBuffer(dataBuffer);
			break;
		}
	} while (key > 0);
	unlock();
	return result;
}


template<unsigned dataPageSize, unsigned const globalKeySize, typename tKey>
void IndexNode<dataPageSize,globalKeySize,tKey>::markUsedDataPages(std::vector<bool> & usedPages) const
{
	for (unsigned i = 0; i < fDataPagesCount; ++i) usedPages[fFirstDataPageId+i] = true;
	unsigned const leavesCount = LocalTree::subtreeLeavesCount();
	for (unsigned i = 0; i < leavesCount; ++i) {
		if (fLevels[i] == LocalTree::treeHeight && fLeaves[i] != LocalTree::emptyLeaf) {
			offspringIndexNodes.at(fLeaves[i])->markUsedDataPages(usedPages);
		}
	}
}


//template class IndexNode<4096,4,uint32_t>;
template class IndexNode<8192,4,uint32_t>;
//template class IndexNode<4096,5,uint64_t>;
template class IndexNode<8192,5,uint64_t>;

