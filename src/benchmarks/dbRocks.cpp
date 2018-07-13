#include "db_api.hpp"
#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define CHECK(x) { rocksdb::Status s = (x); if ( ! s.ok() ) throw std::runtime_error("Command failed: " TOSTRING(s.ToString())); }


struct DB::Pim {
	rocksdb::DB * db;
};

DB::DB() : pim(new Pim)
{
	  rocksdb::Options options;
	  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
	  options.IncreaseParallelism();
	  options.OptimizeLevelStyleCompaction();
	  // create the DB if it's not already present
	  options.create_if_missing = true;

	CHECK( rocksdb::DB::Open(options, "dbRocks.rdb", &(pim->db)) );
}

DB::~DB()
{
	delete pim->db;
	delete pim;
}

void DB::set(uint32_t key, std::vector<uint32_t> const & value)
{
	char bKey[4];
	char bValue[12];
	reinterpret_cast<uint32_t*>(bKey)[0] = key;
	reinterpret_cast<uint32_t*>(bValue)[0] = value[0];
	reinterpret_cast<uint32_t*>(bValue)[1] = value[1];
	reinterpret_cast<uint32_t*>(bValue)[2] = value[2];
	CHECK( pim->db->Put(rocksdb::WriteOptions(), rocksdb::Slice(bKey,4), rocksdb::Slice(bValue,12)) );
}




