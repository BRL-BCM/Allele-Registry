#include <kchashdb.h>
#include <cstdint>

inline void check(bool result)
{
	if (result) return;
	throw std::runtime_error("Error");
}


int main(int argc, char ** argv)
{

	kyotocabinet::TreeDB db;
	check( db.open("./test2.kct", kyotocabinet::BasicDB::OWRITER) );

	check( db.clear() );

	char buf[4];
	char * key = buf;

	kyotocabinet::writefixnum(key, 1, 4);
	db.set( key,4,"1",1);

	kyotocabinet::writefixnum(key, 10, 4);
	db.set( key,4,"10",2);

	kyotocabinet::writefixnum(key, 123456789, 4);
	db.set( key,4,"123456789",9);

	kyotocabinet::writefixnum(key, 3123456789, 4);
	db.set( key,4,"3123456789",10);

	kyotocabinet::writefixnum(key, 1234567890, 4);
	db.set( key,4,"1234567890",10);

	check( db.close() );
	return 0;
}
