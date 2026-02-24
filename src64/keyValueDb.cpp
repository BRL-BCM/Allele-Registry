#include "keyValueDb.hpp"
#include <kcpolydb.h>
#include <kchashdb.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define CHECK(x) if ( ! (x) ) throw std::runtime_error("Command failed: " TOSTRING(x));

namespace keyValueDb {

	template<class Record>
	struct Database<Record>::Pim
	{
		kyotocabinet::PolyDB db;
	};

	template<class Record>
	Database<Record>::Database(std::string const & path) : pim(new Pim)
	{
		if (path.substr(path.size()-3) == "kct") {
			kyotocabinet::TreeDB * db2 = new kyotocabinet::TreeDB();
			CHECK( db2->tune_map(1024*1024*1024) );
			CHECK( db2->open(path, kyotocabinet::BasicDB::OWRITER) );
			CHECK( pim->db.set_internal_db(db2) );
		} else if (path.substr(path.size()-3) == "kch") {
			kyotocabinet::HashDB * db2 = new kyotocabinet::HashDB();
			CHECK( db2->tune_map(1024*1024*1024) );
			CHECK( db2->open(path, kyotocabinet::BasicDB::OWRITER) );
			CHECK( pim->db.set_internal_db(db2) );
		} else {
			throw std::logic_error("Unknown DB type: " + path);
		}
	}

	template<class Record>
	Database<Record>::~Database()
	{
		pim->db.close();
		delete pim;
	}

	template<class Record>
	void Database<Record>::startTransaction()
	{
		//CHECK( pim->db.begin_transaction(true) );
	}

	template<class Record>
	void Database<Record>::commitTransaction()
	{
		//CHECK( pim->db.end_transaction(true) );
	//	CHECK( pim->db.synchronize(true) );
	}

	template<class Record>
	void Database<Record>::getRecordsValues(std::vector<Record> & records)
	{
		for (Record & r: records) {
			Buffer key = r.getKey();
			size_t valSize;
			Buffer val(0);
			val.pointer = pim->db.get( reinterpret_cast<char*>(key.pointer), key.position, &valSize);
			if (val.pointer == nullptr) continue;
			val.size = valSize;
			r.saveValue(val);
		}
	}

	template<class Record>
	void Database<Record>::saveRecords(std::vector<Record> const & records)
	{
		for (Record const & r: records) {
			Buffer key = r.getKey();
			Buffer val = r.getValue();
			CHECK( pim->db.set( reinterpret_cast<char const*>(key.pointer), key.position, reinterpret_cast<char const*>(val.pointer), val.position) );
		}
	}

	// =================================== Buffer


	Buffer::Buffer(unsigned initialSize) : pointer(nullptr), size(0), position(0)
	{
		if (initialSize > 0) {
			pointer = new char[initialSize];
			size = initialSize;
		}
	}

	Buffer::~Buffer()
	{
		delete [] reinterpret_cast<char*>(pointer);
	}

	void Buffer::writeBytes(unsigned long long value, unsigned bytesNumber)
	{
		CHECK( position + bytesNumber <= size );
		kyotocabinet::writefixnum( pointer + position, value, bytesNumber );
		position += bytesNumber;
	}

	void Buffer::writeString(std::string const & value)
	{
		CHECK( position + value.size() <= size );
		strncpy( reinterpret_cast<char*>(pointer) + position, value.data(), value.size() );
		position += value.size();
	}

//	void Buffer::writeStringWithZero(std::string const & value)
//	{
//		CHECK( position + value.size() <= size );
//
//	}

	void Buffer::readBytes(unsigned long long & value, unsigned bytesNumber)
	{
		CHECK( position + bytesNumber <= size );
		value = kyotocabinet::readfixnum( pointer + position, bytesNumber );
		position += bytesNumber;
	}

	void Buffer::readBytes(unsigned & value, unsigned bytesNumber)
	{
		CHECK( position + bytesNumber <= size );
		value = kyotocabinet::readfixnum( pointer + position, bytesNumber );
		position += bytesNumber;
	}

//	void Buffer::readStringWithZero(std::string const & value, unsigned bytesNumber);
	void Buffer::readString(std::string & value, unsigned bytesNumber)
	{
		if (bytesNumber == 0) { value = ""; return; }
		CHECK( position + bytesNumber <= size );
		value.assign( reinterpret_cast<char*>(pointer) + position, bytesNumber );
		position += bytesNumber;
	}

}


#include "dbRecords.hpp"

namespace keyValueDb {
	template class Database<Record_caId_def>;
	template class Database<Record_def_caId>;
	template class Database<Record_seqId_seq>;
	template class Database<Record_seq_seqId>;
}
