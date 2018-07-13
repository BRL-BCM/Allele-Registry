#include "db_api.hpp"
#include <kchashdb.h>


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define CHECK(x) if ( ! (x) ) throw std::runtime_error("Command failed: " TOSTRING(x));


struct DB::Pim {
	kyotocabinet::TreeDB db;
};

DB::DB() : pim(new Pim)
{
	CHECK( pim->db.tune_map(1024*1024*1024) );
	CHECK( pim->db.open("dbKcTree2.kch", kyotocabinet::BasicDB::OWRITER) );
}

DB::~DB()
{
	pim->db.close();
	delete pim;
}

void DB::set(uint32_t key, std::vector<uint32_t> const & value)
{
	char buf[12];
	kyotocabinet::writefixnum( &(buf[0]), key, 4 );
	kyotocabinet::writefixnum( &(buf[4]), key, 4 );
	kyotocabinet::writefixnum( &(buf[8]), key, 4 );
	CHECK( pim->db.set( buf, 12, reinterpret_cast<char const*>(&(value[0])), 4*value.size()) );
}
