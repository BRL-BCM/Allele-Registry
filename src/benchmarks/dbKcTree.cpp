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
	CHECK( pim->db.open("dbKcTree.kch", kyotocabinet::BasicDB::OWRITER) );
}

DB::~DB()
{
	pim->db.close();
	delete pim;
}

void DB::set(uint32_t key, std::vector<uint32_t> const & value)
{
	char buf[4];
	kyotocabinet::writefixnum( &(buf[0]), key, 4 );
	CHECK( pim->db.set( buf, 4, reinterpret_cast<char const*>(&(value[0])), 4*value.size()) );
}

void DB::set(std::vector<std::pair<uint32_t,uint32_t>> const & keysValues)
{
	std::map<std::string,std::string> data;
	for (auto const & kv: keysValues) {
		char buf[4];
		kyotocabinet::writefixnum( &(buf[0]), kv.first, 4 );
		std::string k(&(buf[0]),&(buf[4]));
		kyotocabinet::writefixnum( &(buf[0]), kv.second, 4 );
		std::string v(&(buf[0]),&(buf[4]));
		data[k] = v;
	}
	CHECK( pim->db.set_bulk(data) );
	CHECK( pim->db.synchronize() );
}
