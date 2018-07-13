#ifndef FLATDB_BIN_HPP_
#define FLATDB_BIN_HPP_

#include <algorithm>
#include <stdexcept>
#include <limits>


namespace flatDb {


	template<typename tKey>
	struct KeysRangeT
	{
		tKey first;
		tKey count;
		inline tKey last() const
		{
			if (count == 0) throw std::logic_error("KeysRange::last(): count == 0");
			if (std::numeric_limits<tKey>::max() - first < count - 1) return std::numeric_limits<tKey>::max();
			return (first + (count - 1));
		}
	};


	template<typename tKey>
	struct BinT
	{
		tKey firstKey = 0;
		tKey maxKeyOffset = 0;
		unsigned recordsCount = 0;
		unsigned bytesCount = 0;  // includes bytes needed to save record content + bytes needed to save record size, does not include bytes needed to save keys
		inline tKey lastKey() const
		{
			return (firstKey + maxKeyOffset);
		}
		inline unsigned bytesPerKey() const
		{
			unsigned r = 0;
			tKey maxLocalKey = maxKeyOffset;
			while (maxLocalKey > 0) {
				maxLocalKey >>= 8;
				++r;
			}
			return r;
		}
		inline unsigned totalSize() const
		{
			return (bytesCount + recordsCount * bytesPerKey());
		}
		inline BinT<tKey> & operator+=(BinT<tKey> const & b)
		{
			tKey const newFirstKey = std::min(firstKey, b.firstKey);
			maxKeyOffset = std::max(firstKey + maxKeyOffset, b.firstKey + b.maxKeyOffset) - newFirstKey;
			firstKey = newFirstKey;
			recordsCount += b.recordsCount;
			bytesCount += b.bytesCount;
			return (*this);
		}
	};


	template<typename tKey>
	inline BinT<tKey> operator+(BinT<tKey> const & b1, BinT<tKey> const & b2)
	{
		BinT<tKey> b = b1;
		b += b2;
		return b;
	}


	template<typename tKey>
	inline bool operator==(BinT<tKey> const & b1, BinT<tKey> const & b2)
	{
		return (b1.bytesCount == b2.bytesCount && b1.firstKey == b2.firstKey && b1.maxKeyOffset == b2.maxKeyOffset && b1.recordsCount == b2.recordsCount);
	}

}


#endif /* FLATDB_BIN_HPP_ */
