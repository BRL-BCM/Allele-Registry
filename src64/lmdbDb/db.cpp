#include "../lmdb/lmdb.h"
#include "../apiDb/db.hpp"
#include "../commonTools/assert.hpp"
#include "../commonTools/Stopwatch.hpp"
#include "../commonTools/bytesLevel.hpp"
#include <boost/lexical_cast.hpp>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <thread>

//#include <string>
//#include <iostream>



#define CHECK(x) if (int r = (x)) \
	throw std::runtime_error(std::string("LMDB function failed: ") + std::string(#x) + std::string(" => ") + boost::lexical_cast<std::string>(r));

#define templateXX template<typename tKey, unsigned const globalKeySize, unsigned dataPageSize>
#define XX         DatabaseT<tKey,globalKeySize,dataPageSize>


namespace lmdb {

	struct Buffer {
		uint8_t * ptr = nullptr;
		unsigned size = 0;
		Buffer() {}
		Buffer(MDB_val const & b) : ptr(reinterpret_cast<uint8_t*>(b.mv_data)), size(b.mv_size) {}
		operator MDB_val() const
		{
			MDB_val v;
			v.mv_data = ptr;
			v.mv_size = size;
			return v;
		}
	};

	class Cursor
	{
	private:
		friend class TransactionReadOnly;
		friend class TransactionUpdate;
	 	MDB_cursor* cursor = nullptr;
		Cursor(MDB_txn* txn, MDB_dbi& dbi)
		{
			CHECK( mdb_cursor_open(txn, dbi, &cursor) );
		}
	public:
		typedef std::shared_ptr<Cursor> SP;
		~Cursor()
		{
			if (cursor != nullptr) mdb_cursor_close(cursor);
		}
		bool findFirstEqual(Buffer const & pKey)
		{
			MDB_val key = pKey;
			MDB_val data;
			data.mv_data = nullptr;
			data.mv_size = 0;
			int errorCode = mdb_cursor_get( cursor, &key, &data, MDB_SET );
			if (errorCode == MDB_NOTFOUND) return false;
			CHECK(errorCode);
			return true;
		}
		bool findFirstEqualOrGreater(Buffer const & pKey)
		{
			MDB_val key = pKey;
			MDB_val data;
			data.mv_data = nullptr;
			data.mv_size = 0;
			int errorCode = mdb_cursor_get( cursor, &key, &data, MDB_SET_RANGE );
			if (errorCode == MDB_NOTFOUND) return false;
			CHECK(errorCode);
			return true;
		}
		bool moveToNextKey()
		{
			MDB_val key;
			MDB_val data;
			int errorCode = mdb_cursor_get( cursor, &key, &data, MDB_NEXT_NODUP );
			if (errorCode == MDB_NOTFOUND) return false;
			CHECK(errorCode);
			return true;
		}
		bool moveToLastKey()
		{
			MDB_val key;
			MDB_val data;
			int errorCode = mdb_cursor_get( cursor, &key, &data, MDB_LAST );
			if (errorCode == MDB_NOTFOUND) return false;
			CHECK(errorCode);
			return true;
		}
		Buffer getKey()
		{
			MDB_val key;
			MDB_val data;
			CHECK(mdb_cursor_get( cursor, &key, &data, MDB_GET_CURRENT ));
			return Buffer(key);
		}
		std::vector<Buffer> getValues()
		{
			std::vector<Buffer> r;
			MDB_val key;
			MDB_val data;
			CHECK(mdb_cursor_get( cursor, &key, &data, MDB_FIRST_DUP ));
			while (true) {
				r.push_back(data);
				int errorCode = mdb_cursor_get( cursor, &key, &data, MDB_NEXT_DUP );
				if (errorCode == MDB_NOTFOUND) break;
				CHECK(errorCode);
			}
			return r;
		}
		void deleteWholeEntry()
		{
			CHECK(mdb_cursor_del( cursor, MDB_NODUPDATA ));
		}
		void addValue(Buffer const & pKey, Buffer const & value)
		{
			MDB_val key = pKey;
			MDB_val data = value;
			CHECK(mdb_cursor_put( cursor, &key, &data, 0 ));
		}
	};

	class TransactionReadOnly {
	private:
		MDB_txn* txn = nullptr;
		MDB_dbi& dbi;
	public:
		TransactionReadOnly(MDB_env* env, MDB_dbi& pDbi) : dbi(pDbi)
		{
			CHECK( mdb_txn_begin(env, nullptr, MDB_RDONLY, &txn) );
		}
		~TransactionReadOnly()
		{
			// TODO free cursors
			mdb_txn_abort(txn);
		}
		Cursor::SP createCursor()
		{
			return std::unique_ptr<Cursor>(new Cursor(txn, dbi));
		}
	};

	class TransactionUpdate {
	private:
		MDB_txn* txn = nullptr;
		MDB_dbi& dbi;
		std::vector<std::weak_ptr<Cursor>> cursors;
	public:
		TransactionUpdate(MDB_env* env, MDB_dbi& pDbi) : dbi(pDbi)
		{
			CHECK( mdb_txn_begin(env, nullptr, 0, &txn) );
		}
		void commit()
		{
			CHECK( mdb_txn_commit(txn) );
			txn = nullptr;
			for (auto c: cursors) if ( ! c.expired() ) c.lock()->cursor = nullptr;
		}
		~TransactionUpdate()
		{
			if (txn != nullptr) mdb_txn_abort(txn);
		}
		Cursor::SP createCursor()
		{
			Cursor::SP p(new Cursor(txn, dbi));
			cursors.push_back(p);
			return p;
		}
	};

}


	templateXX
	struct XX::Pim
	{
		MDB_env* env = nullptr;
		MDB_dbi dbi;
		tCreateRecord callbackCreateRecord;
		struct BufferWithKey
		{
			uint8_t buf[globalKeySize];
			BufferWithKey() {}
			BufferWithKey(tKey const & key)
			{
				uint8_t * ptr = buf;
				writeUnsignedInteger<globalKeySize>(ptr, key);
			}
			BufferWithKey(lmdb::Buffer const & b)
			{
				ASSERT(b.size == globalKeySize);
				std::memcpy(buf, b.ptr, globalKeySize);
			}
			tKey get() const
			{
				uint8_t const * ptr = buf;
				return (readUnsignedInteger<globalKeySize,tKey>(ptr));
			}
			lmdb::Buffer asBuffer()
			{
				lmdb::Buffer b;
				b.ptr = buf;
				b.size = globalKeySize;
				return b;
			}
		};
		struct BufferWithData
		{
			uint8_t buf[256];
			unsigned size = 0;
			BufferWithData() {}
			void set(Record const * r)
			{
				size = r->dataLength();
				ASSERT(size <= 256);
				uint8_t * ptr = buf;
				r->saveData(ptr);
				ASSERT(ptr - buf == size);
			}
			lmdb::Buffer asBuffer()
			{
				lmdb::Buffer b;
				b.ptr = buf;
				b.size = size;
				return b;
			}
		};
	};


	templateXX
	XX::DatabaseT(TasksManager * cpu, std::string const & dbFile, tCreateRecord createCallback, unsigned cacheSizeInMegabytes)
	: pim(new Pim)
	{
		std::string const dbFilePath = dbFile + ".lmdb";
		pim->callbackCreateRecord = createCallback;
		try {
			// ---- create & open environment (DB file)
			CHECK( mdb_env_create(&(pim->env)) );
			CHECK( mdb_env_set_mapsize(pim->env, static_cast<size_t>(1024*1024*1024)*128) );  // max size = 128 GB
			CHECK( mdb_env_open(pim->env, dbFilePath.c_str(), MDB_NOMEMINIT | MDB_NOSUBDIR , 0664) ); // TODO - add | MDB_WRITEMAP for efficiency?

			// ---- create transaction
			MDB_txn* txn = nullptr;
			CHECK( mdb_txn_begin(pim->env, nullptr, 0, &txn) );

			// ---- open database and finish transaction
			try {
				CHECK( mdb_dbi_open(txn, nullptr, MDB_DUPSORT | MDB_CREATE, &(pim->dbi)) );
				CHECK( mdb_txn_commit(txn) );
			} catch (...) {
				mdb_dbi_close(pim->env, pim->dbi);
				throw;
			}
			// -----
		} catch (...) {
			if (pim->env != nullptr) mdb_env_close(pim->env);
			delete pim;
			throw;
		}
	}


	templateXX
	XX::~DatabaseT()
	{
		mdb_dbi_close(pim->env, pim->dbi);
		mdb_env_close(pim->env);
		delete pim;
	}


	templateXX
	void XX::readRecordsInOrder(tReadFunction visitor, tKey firstKey, tKey lastKey) const
	{
		unsigned const recordsPerChunk = 255*1024;
		std::vector<Record const *> records;
		records.reserve(recordsPerChunk+1024);

		for ( bool lastCall = false;  ! lastCall;  std::this_thread::yield() ) {

			// ----- create transaction and cursor
			lmdb::TransactionReadOnly trans(pim->env, pim->dbi);
			lmdb::Cursor::SP cursor = trans.createCursor();

			if ( cursor->findFirstEqualOrGreater(((typename Pim::BufferWithKey)(firstKey)).asBuffer()) ) {
				firstKey = ((typename Pim::BufferWithKey)(cursor->getKey())).get();
				while (firstKey <= lastKey && records.size() < recordsPerChunk) {
					std::vector<lmdb::Buffer> values = cursor->getValues();
					for (auto value: values) {
						uint8_t const * ptr = value.ptr;
						Record * r = pim->callbackCreateRecord(firstKey, ptr);
						ASSERT(value.size == r->dataLength());
						records.push_back(r);
					}
					if ( ! cursor->moveToNextKey() ) {
						lastCall = true;
						break;
					}
					firstKey = ((typename Pim::BufferWithKey)(cursor->getKey())).get();
				}
			} else {
				lastCall = true;
			}

			// ----- call the callback function
			lastCall = lastCall || (firstKey > lastKey);
			bool const hardLastCall = lastCall;
			visitor(records, lastCall);
			if (hardLastCall) lastCall = true;

			// ----- free memory
			for (auto r: records) delete r;
			records.clear();
		}
	}


	templateXX
	void XX::readRecords(std::vector<Record*> const & pRecords, tReadByKeyFunction visitor) const
	{
		// ===== copy records to local vector
		std::vector<Record*> records;
		records.reserve(pRecords.size());
		std::copy(pRecords.begin(), pRecords.end(), std::back_inserter(records));
		std::sort( records.begin(), records.end() );

		// ===== iterate over records and execute update transactions in 100 ms timeslots
		for ( auto iR = records.begin();  iR != records.end();  std::this_thread::yield() ) {

			// ----- create transaction and cursor
			lmdb::TransactionReadOnly trans(pim->env, pim->dbi);
			lmdb::Cursor::SP cursor = trans.createCursor();
			Stopwatch timer;

			std::vector<Record*> newRecords;
			std::vector<Record const *> currentRecords;
			do {
				// ----- vector with "new" records
				tKey const currKey = (*iR)->key;
				for ( ;  iR != records.end() && (*iR)->key == currKey;  ++iR ) {
					newRecords.push_back(*iR);
				}

				// ----- vector with "db" (current) records
				bool const dbHasKey = cursor->findFirstEqual( ((typename Pim::BufferWithKey)(currKey)).asBuffer());
				if ( dbHasKey ) {
					std::vector<lmdb::Buffer> values = cursor->getValues();
					for (auto value: values) {
						uint8_t const * ptr = value.ptr;
						Record * r = pim->callbackCreateRecord(currKey, ptr);
						ASSERT(value.size == r->dataLength());
						currentRecords.push_back(r);
					}
				}

				// ----- run callback
				visitor(currentRecords, newRecords);

				// ----- free memory
				for (auto r: currentRecords) delete r;
				newRecords.clear();
				currentRecords.clear();

			} while ( timer.get_time_ms() < 10000 && iR != records.end() );  // 10000 ms slot

		}
	}


	templateXX
	void XX::writeRecords(std::vector<Record*> const & pRecords, tUpdateByKeyFunction visitor)
	{
		// ===== copy records to local vector
		std::vector<Record*> records;
		records.reserve(pRecords.size());
		std::copy(pRecords.begin(), pRecords.end(), std::back_inserter(records));
		std::sort( records.begin(), records.end() );

		// ===== iterate over records and execute update transactions in 100 ms timeslots
		for ( auto iR = records.begin();  iR != records.end();  std::this_thread::yield() ) {

			// ----- create transaction and cursor
			lmdb::TransactionUpdate trans(pim->env, pim->dbi);
			lmdb::Cursor::SP cursor = trans.createCursor();
			Stopwatch timer;

			std::vector<Record*> newRecords;
			std::vector<Record*> currentRecords;
			do {
				// ----- vector with "new" records
				tKey const currKey = (*iR)->key;
				for ( ;  iR != records.end() && (*iR)->key == currKey;  ++iR ) {
					newRecords.push_back(*iR);
				}

				// ----- vector with "db" (current) records
				bool const dbHasKey = cursor->findFirstEqual( ((typename Pim::BufferWithKey)(currKey)).asBuffer());
				if ( dbHasKey ) {
					std::vector<lmdb::Buffer> values = cursor->getValues();
					for (auto value: values) {
						uint8_t const * ptr = value.ptr;
						Record * r = pim->callbackCreateRecord(currKey, ptr);
						ASSERT(value.size == r->dataLength());
						currentRecords.push_back(r);
//if (currKey == 65534363175ull)
//{
//	std::cout << "RECORD READ:";
//	for (unsigned i = 0; i < value.size; ++i) std::cout << " " << unsigned(value.ptr[i]);
//	std::cout << std::endl;
//}
					}
				}

				// ----- run callback
				if ( visitor(currentRecords, newRecords) ) {
					if (dbHasKey) cursor->deleteWholeEntry();
					typename Pim::BufferWithKey key(currKey);
					typename Pim::BufferWithData value;
					for (auto r: currentRecords) {
						value.set(r);
//if (currKey == 65534363175ull)
//{
//	std::cout << "RECORD WRITE:";
//	for (unsigned i = 0; i < value.size; ++i) std::cout << " " << unsigned(value.buf[i]);
//	std::cout << std::endl;
//}
						cursor->addValue(key.asBuffer(), value.asBuffer());
					}
				}

				// ----- free memory
				for (auto r: currentRecords) delete r;
				newRecords.clear();
				currentRecords.clear();

			} while ( timer.get_time_ms() < 10000 && iR != records.end() );  // 10000 ms slot

			// ----- save changes
			trans.commit();
		}
	}


	templateXX
	tKey XX::getTheLargestKey() const
	{
		lmdb::TransactionReadOnly trans(pim->env, pim->dbi);
		lmdb::Cursor::SP cursor = trans.createCursor();
		if ( ! cursor->moveToLastKey() ) return 0;
		typename Pim::BufferWithKey buf(cursor->getKey());
		return buf.get();
	}


	template class DatabaseT<uint32_t,4,8*1024>;
	template class DatabaseT<uint64_t,5,8*1024>;
