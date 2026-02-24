#ifndef FIXEDHEIGHTTREE_HPP_
#define FIXEDHEIGHTTREE_HPP_

#include <cstdint>
#include <limits>

template<typename tKey, unsigned tTreeHeight, unsigned leafCapacity, typename tBucketId = uint32_t, typename tLevel = uint8_t>
class FixedHeightTree
{
private:
	tLevel * fLevels;
	tBucketId * fLeaves;
public:
	static unsigned const treeHeight = tTreeHeight;
	static tBucketId const emptyLeaf = std::numeric_limits<tBucketId>::max();
	static inline tKey subtreeLeftmostKey(tKey const key, unsigned const rootLevel)
	{
	    unsigned const height = treeHeight - rootLevel;
	    if (height >= 8*sizeof(tKey)) throw std::overflow_error("FixedHeightTree::subtreeLeftmostKey");
	    return ((key >> height) << height);
	}
	static inline tKey subtreeLeavesCount(unsigned const rootLevel = 0)
	{
	    unsigned const height = treeHeight - rootLevel;
	    if (height >= 8*sizeof(tKey)) throw std::overflow_error("FixedHeightTree::subtreeLeavesCount");
	    return (static_cast<tKey>(1) << height);
	}
	FixedHeightTree(tLevel * levels, tBucketId * leaves) : fLevels(levels), fLeaves(leaves) { }
	// -------
	// return bucket from given key
	tBucketId getBucketId(tKey const key) const
	{
		tKey const leftmost = subtreeLeftmostKey(key, fLevels[key]);
		tKey const size = subtreeLeavesCount(fLevels[key]);
		for (unsigned i = 0; i < size; ++i) {
			tBucketId const bucket = fLeaves[leftmost+i];
			if (bucket != emptyLeaf) return bucket;
		}
		return emptyLeaf;
	}
	// returns (buckets on level < treeHeight, buckets on level = treeHeight)
	std::pair<tBucketId,tBucketId> getMaxBucketId() const
	{
		std::pair<tBucketId,tBucketId> r = std::make_pair(emptyLeaf,emptyLeaf);
		tKey const leavesCount = subtreeLeavesCount();
		for (tKey i = 0; i < leavesCount; ++i) {
			if (fLeaves[i] == emptyLeaf) continue;
			if (fLevels[i] < treeHeight) {
				if (r.first  == emptyLeaf || r.first  < fLeaves[i]) r.first  = fLeaves[i];
			} else {
				if (r.second == emptyLeaf || r.second < fLeaves[i]) r.second = fLeaves[i];
			}
		}
		return r;
	}
	// returns (buckets on level < treeHeight, buckets on level = treeHeight)
	std::pair<unsigned,unsigned> getBucketsCount() const
	{
		std::pair<unsigned,unsigned> r = std::make_pair(0,0);
		tBucketId lastBucketId = emptyLeaf;
		tKey const leavesCount = subtreeLeavesCount();
		for (tKey i = 0; i < leavesCount; ++i) {
			if (fLeaves[i] == emptyLeaf) continue;
			if (fLevels[i] == treeHeight) {
				++(r.second);
			} else if (fLeaves[i] != lastBucketId) {
				lastBucketId = fLeaves[i];
				++(r.first);
			}
		}
		return r;
	}
	// -------
	// dataSizes is destroyed !!!
	// dataSizes - pairs: key -> dataSize (key must be smaller than 2^treeHeight)
	// rootLevel - if set only part of tree is updated (keys must belong to the same subtree)
	void overwriteSubtree
			( std::vector<std::pair<tKey,unsigned>> & dataSizes  // pairs (key,size)
			, tBucketId & nextFreeBucketId   // new bucket ids to use
			, unsigned rootLevel = 0
			)
	{
		// check assertions & calculate subtree parameters
		if ( dataSizes.empty() ) throw std::logic_error("FixedHeightTree::overwriteSubtree called for empty vector of records");
		std::sort( dataSizes.begin(), dataSizes.end() );
		tKey const numberOfLeaves = subtreeLeavesCount(rootLevel);
		tKey const leftmostKey = subtreeLeftmostKey(dataSizes.front().first,rootLevel);
		if ( leftmostKey != subtreeLeftmostKey(dataSizes.back().first,rootLevel) ) throw std::logic_error("FixedHeightTree::overwriteSubtree called with dataSizes from different subtrees");

		// calculate vector of cumulative sizes of leaves
		std::vector<unsigned> cumulativeSizes(numberOfLeaves+1, 0);
		for (auto const & key2size: dataSizes) cumulativeSizes[key2size.first-leftmostKey+1] += key2size.second;
		for (tKey i = 1; i <= numberOfLeaves; ++i) cumulativeSizes[i] += cumulativeSizes[i-1];

		// clear leaves
		for (tKey i = 0; i < numberOfLeaves; ++i) fLeaves[leftmostKey+i] = emptyLeaf;

		// calculate levels and leaves
		std::vector<std::pair<unsigned,unsigned>> intervals(1,std::make_pair(0,numberOfLeaves));
		for ( unsigned level = rootLevel;  level < treeHeight;  ++level ) {
			std::vector<std::pair<unsigned,unsigned>> intervals2;
			for (auto interval: intervals) {
				if ( cumulativeSizes[interval.second] - cumulativeSizes[interval.first] <= leafCapacity ) {
					for (unsigned i = interval.first; i < interval.second; ++i) {
						fLevels[leftmostKey+i] = level;
					}
					if ( cumulativeSizes[interval.second] != cumulativeSizes[interval.first] ) {
						tBucketId const bucketId = nextFreeBucketId++;
						for (unsigned i = interval.first; i < interval.second; ++i) {
							if (cumulativeSizes[i+1] != cumulativeSizes[i]) fLeaves[leftmostKey+i] = bucketId;
						}
					}
				} else {
					unsigned const midpoint = (interval.first + interval.second) / 2;
					intervals2.push_back( std::make_pair(interval.first,midpoint) );
					intervals2.push_back( std::make_pair(midpoint,interval.second) );
				}
			}
			intervals = std::move(intervals2);
		}
		for (auto interval: intervals) {
//			ASSERT( interval.first + 1 == interval.second );
			fLevels[leftmostKey+interval.first] = treeHeight;
			if ( cumulativeSizes[interval.second] != cumulativeSizes[interval.first] ) {
				fLeaves[leftmostKey+interval.first] = nextFreeBucketId++;
			}
		}
	}
};









#endif /* FIXEDHEIGHTTREE_HPP_ */
