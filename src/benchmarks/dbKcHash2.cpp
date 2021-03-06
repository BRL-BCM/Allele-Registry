#include "db_api.hpp"
#include <kchashdb.h>


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define CHECK(x) if ( ! (x) ) throw std::runtime_error("Command failed: " TOSTRING(x));


struct DB::Pim {
	kyotocabinet::HashDB db;
};

DB::DB() : pim(new Pim)
{
	CHECK( pim->db.tune_map(512*1024*1024) );
	CHECK( pim->db.open("dbKcHash2.kch", kyotocabinet::BasicDB::OWRITER) );
}

DB::~DB()
{
	pim->db.close();
	delete pim;
}

void DB::set(uint32_t key, std::vector<uint32_t> const & value)
{
	char buf[3];
	kyotocabinet::writefixnum( &(buf[0]), key/256, 3 );
	size_t size;
	char * ptr = pim->db.get( buf, 3, &size);
	if (ptr == nullptr) {
		ptr = new char[256*3*4];
		for (unsigned i = 0; i < 256*3; ++i) (reinterpret_cast<uint32_t*>(ptr))[i] = 0;
	}
	(reinterpret_cast<uint32_t*>(ptr))[ key%256 * 3 + 0 ] = value[0];
	(reinterpret_cast<uint32_t*>(ptr))[ key%256 * 3 + 1 ] = value[1];
	(reinterpret_cast<uint32_t*>(ptr))[ key%256 * 3 + 2 ] = value[2];
	CHECK( pim->db.set( buf, 3, ptr, 256*3*4) );
	delete [] ptr;
}
