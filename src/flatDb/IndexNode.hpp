#ifndef FLATDB_INDEXNODE_HPP_
#define FLATDB_INDEXNODE_HPP_

#include "DataNode.hpp"


namespace flatDb {

	template<typename tKey>
	class IndexNodeT
	{
	public:
		typedef std::shared_ptr<IndexNodeT<tKey>> SP;
		typedef RecordT<tKey> Record;
		typedef BinT<tKey> Bin;
		typedef DataNodeT<tKey> DataNode;
		typedef IndexNodeT<tKey> IndexNode;
		typedef SubProcedureUpdateRecordsByKeysT<tKey> SubProcedureUpdateRecordsByKeys;
		typedef SchedulerT<tKey> Scheduler;
		std::vector<typename DataNode::SP> entries;
		Scheduler * const scheduler;
		unsigned const keySize;
		unsigned const dataPageSize;
		unsigned const indexPageSize;
		unsigned const revision;
		bool     const firstCopy;
	private:
		// outputBins is overwritten (bins are ordered according to keys values)
		void calculateKeyBinsFromContent(std::vector<std::pair<tKey,uint8_t const*>> const & content, std::vector<Bin> & outputBins) const;
		// ----
		std::vector<Bin> divideIntoPages(std::vector<Bin> const & bins) const;
		// --- constructor
		IndexNodeT(Scheduler* s, unsigned pKeySize, unsigned pDataPageSize, unsigned pIndexPageSize, unsigned rev, bool pFirstCopy)
		: scheduler(s), keySize(pKeySize), dataPageSize(pDataPageSize), indexPageSize(pIndexPageSize), revision(rev), firstCopy(pFirstCopy) {}
	public:
		// ---- constructors
		static SP createEmpty(Scheduler*, unsigned keySize, unsigned dataPageSize, unsigned indexPageSize);
		static SP createFromBuffer(Scheduler*, unsigned keySize, unsigned dataPageSize, unsigned indexPageSize, bool firstCopy, void const * ptr);
		SP createSecondCopy() const;
		// ---- save node to buffer
		void writeToBuffer(void * ptr) const;
		// ---- returns list of removed data nodes
		// takes vectors of pairs (index of data node, priority)
		std::vector<typename DataNode::SP> reorganize( std::vector<std::pair<unsigned,unsigned>> dataNodesIds );
	};


}


#endif /* FLATDB_INDEXNODE_HPP_ */
