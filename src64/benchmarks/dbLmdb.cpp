#include "db_api.hpp"
#include "liblmdb/lmdb.h"
#include <stdexcept>
#include <cstring>
#include <string>
#include <algorithm>
#include <iostream>
#include <boost/lexical_cast.hpp>


#define CHECK(x) if (int r = (x)) throw std::runtime_error(std::string("LMDB function failed: ") + std::string(#x) + std::string(" => ") + boost::lexical_cast<std::string>(r));


	struct DB::Pim
	{
		MDB_env* env = nullptr;
		MDB_txn* txn = nullptr;
		MDB_dbi dbi;
	};


	DB::DB() : pim(new Pim)
	{
		CHECK( mdb_env_create(&(pim->env)) );
		CHECK( mdb_env_set_mapsize(pim->env, static_cast<size_t>(1024*1024*1024)*128) );
		CHECK( mdb_env_open(pim->env, "db_lmdb", MDB_NORDAHEAD | MDB_NOMEMINIT, 0664) );
	}


	DB::~DB()
	{
		if (pim->env != nullptr) mdb_env_close(pim->env);
		delete pim;
	}


	void DB::set(std::vector<Record> & keysValues)
	{

 		MDB_cursor* cursor = nullptr;

		CHECK( mdb_txn_begin(pim->env, nullptr, 0, &(pim->txn)) );
		CHECK( mdb_dbi_open(pim->txn, nullptr, MDB_DUPSORT | MDB_INTEGERKEY | MDB_CREATE, &(pim->dbi)) );

		CHECK( mdb_cursor_open(pim->txn, pim->dbi, &cursor) );

		MDB_val k, v;
		k.mv_size = 4;
		v.mv_size = 16;
		for (Record & r: keysValues) {
			k.mv_data = &(r.key);
			v.mv_data = &(r.data1);
			CHECK( mdb_cursor_put(cursor, &k, &v, 0) );
		}
		CHECK( mdb_txn_commit(pim->txn) );
		// mdb_cursor_close(cursor); // cursor is freed when write transaction is closed

		mdb_dbi_close(pim->env, pim->dbi);
	}


	void DB::getAndValidate(std::vector<Record> & keysValues)
	{
		std::sort(keysValues.begin(), keysValues.end());

 		MDB_cursor* cursor = nullptr;

		CHECK( mdb_txn_begin(pim->env, nullptr, MDB_RDONLY, &(pim->txn)) );
		CHECK( mdb_dbi_open(pim->txn, nullptr, MDB_DUPSORT | MDB_INTEGERKEY, &(pim->dbi)) );

		CHECK( mdb_cursor_open(pim->txn, pim->dbi, &cursor) );

		MDB_val k, v;
		k.mv_size = 4;
		v.mv_size = 16;
		unsigned i = 0;
		for (Record & r: keysValues) {
			k.mv_data = &(r.key);
			CHECK( mdb_cursor_get(cursor, &k, &v, MDB_SET_KEY) );
			if ( v.mv_size != 16 || std::memcmp(v.mv_data, &(r.data1), 16) ) {
				throw std::runtime_error("Incorrect value!!!");
			}
			if (++i % 10000 == 0) std::cout << "%" << std::flush;
		}
		std::cout << "\n";
		CHECK( mdb_txn_commit(pim->txn) );
		mdb_cursor_close(cursor);

		mdb_dbi_close(pim->env, pim->dbi);
	}

