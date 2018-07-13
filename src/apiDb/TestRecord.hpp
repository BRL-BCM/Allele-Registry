#ifndef APIDB_TESTRECORD_HPP_
#define APIDB_TESTRECORD_HPP_


#include <cstdint>
#include <cstring>
#include <ostream>
#include <iostream>
#include "db.hpp"


class TestRecord : public RecordT<uint32_t>
{
public:
	uint64_t fData[2];
	TestRecord(unsigned key, uint64_t data0 = 1, uint64_t data1 = 2) : RecordT<uint32_t>(key) { fData[0] = data0; fData[1] = data1; }
	virtual unsigned dataLength() const { return 16; }
	virtual void saveData(uint8_t *& buf) const { memcpy(buf, &(fData[0]), 16); buf += 16; }
	virtual void loadData(uint8_t const *& buf) { memcpy(&(fData[0]), buf, 16); buf += 16; }
	virtual ~TestRecord() {}
};

//uint32_t recordDataLength(Record const * r) { return reinterpret_cast<TestRecord const *>(r)->dataLength();  }
//void recordSaveData(Record const * r, uint8_t *& ptr)  { ptr += reinterpret_cast<TestRecord const *>(r)->saveData(ptr);  }
RecordT<uint32_t>* recordLoadData(uint32_t key, uint8_t const *& ptr)  { TestRecord * r = new TestRecord(key); r->loadData(ptr); return r;  }
//void recordDelete(Record * r)  { delete reinterpret_cast<TestRecord *>(r);  }


inline std::ostream& operator<<(std::ostream & str, TestRecord const & r)
{
	str << "[" << r.key << "->" << r.fData[0] << "," << r.fData[1] << "]";
	return str;
}



/*
inline std::ostream& operator<<(std::ostream & str, VariantRecord const & r)
{
	str << "[" << r.key << "->" << r.category << "," << r.lengthBefore << "," << r.lengthChangeOrSeqLength << "," << r.sequence << "]";
	return str;
}
*/

#endif /* APIDB_TESTRECORD_HPP_ */
