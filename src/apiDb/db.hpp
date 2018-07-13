#ifndef APIDB_DB_HPP_
#define APIDB_DB_HPP_

#include <vector>
#include <string>
#include <functional>
#include <limits>
#include <cstdint>

#include "TasksManager.hpp"


	template<typename tKey>
	class RecordT
	{
	public:
		// function is called for each consecutive chunks of records
		// records and function calls are always in correct order
		// first parameter: records already in the database
		// second parameter: = true for last callback, if set to true inside the callback the query terminates
		// returned vector can be modified, adjust it if you want to use the records object and/or delete them by yourself (set to nullptr or remove from the vector)
		// records left in the vector are automatically deleted when the function returns, the rest of records must be deleted by user
		typedef std::function<void(std::vector<RecordT<tKey> const *> const &, bool & lastCall)> tReadFromRangeFunction;

		// function is called exactly once for each key value given by user
		// first parameter: records already in the database, second parameter: subset of records given by user
		// No records can be deleted
		typedef std::function<void(std::vector<RecordT<tKey> const *> const &, std::vector<RecordT<tKey>*> const &)> tReadByKeyFunction;

		// function is called exactly once for each key value given by user
		// returns true if there are some changes to save, false otherwise (if there are changes, "false" may or may not reject them)
		// first parameter: records already in the database, second parameter: subset of records given by user
		// Records from the DB side can be deleted/created if needed, these actions are mapped to DB operations
		typedef std::function<bool(std::vector<RecordT<tKey>*> & currentRecords, std::vector<RecordT<tKey>*> const & newRecords)> tUpdateByKeyFunction;

		// function required to create a new record
		typedef std::function<RecordT<tKey>*(tKey key, uint8_t const *& ptr)> tCreateRecordFunction;
	public:
		typedef tKey Key;
		tKey const key;
		RecordT(tKey pKey) : key(pKey) {}
		virtual unsigned dataLength() const = 0; //{ throw std::logic_error("RecordTemplate::dataLength() not implemented"); }
		virtual void saveData(uint8_t *& ptr) const = 0; //{ throw std::logic_error("RecordTemplate::saveData() not implemented"); }
		virtual void loadData(uint8_t const *& ptr) = 0; //{ throw std::logic_error("RecordTemplate::loadData() not implemented"); }
		virtual ~RecordT() {}
	};


	template<typename tRecord>
	RecordT<typename tRecord::Key> * createRecord(typename tRecord::Key key, uint8_t const *& ptr)
	{
		tRecord * record = new tRecord(key);
		record->loadData(ptr);
		return record;
	}


	template<typename tKey = uint32_t, unsigned const globalKeySize = 4, unsigned dataPageSize = 8*1024>
	class DatabaseT
	{
	public:
		typedef RecordT<tKey> Record;
		struct Pim;

		// ===================================== LOW LEVEL FUNCTIONS FOR CONVERTIONS RECORD <-> BINARY DATA
		typedef typename Record::tCreateRecordFunction tCreateRecord;
		typedef typename Record::tReadFromRangeFunction tReadFunction;
		typedef typename Record::tReadByKeyFunction tReadByKeyFunction;
		typedef typename Record::tUpdateByKeyFunction tUpdateByKeyFunction;

	private:
		Pim * pim;
	public:

		// ===================================== METHODS
		DatabaseT(TasksManager * cpuTaskManager, TasksManager * ioTaskManager,std::string const & dbFile, tCreateRecord, unsigned cacheSizeInMegabytes = 128);
		~DatabaseT();
		void readRecordsInOrder(tReadFunction visitor, tKey first = 0, tKey last = std::numeric_limits<tKey>::max(), unsigned hintQuerySize = std::numeric_limits<unsigned>::max()) const;
		void readRecords(std::vector<Record*> const & records, tReadByKeyFunction visitor) const;
		void writeRecords(std::vector<Record*> const & records, tUpdateByKeyFunction visitor);
		tKey getTheLargestKey() const;
		uint64_t getRecordsCount() const;
		bool isNewDb() const;
	};



#endif /* APIDB_DB_HPP_ */
