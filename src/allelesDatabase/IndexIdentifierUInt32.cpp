#include "IndexIdentifierUInt32.hpp"
#include "RecordVariant.hpp"
#include "../commonTools/bytesLevel.hpp"

	class IdRecord : public RecordT<uint32_t>
	{
	public:
		BinaryGenomicVariantDefinition defGenomic;
		BinaryProteinVariantDefinition defProtein;
		bool isGenomic;
		IdRecord(uint32_t id) : RecordT<uint32_t>(id), defGenomic(0), defProtein(0), isGenomic(true) { }
		virtual unsigned dataLength() const;
		virtual void saveData(uint8_t *& ptr) const;
		virtual void loadData(uint8_t const *& ptr);
		virtual ~IdRecord() {}
		inline bool operator==(IdRecord const & o) const
		{
			if (isGenomic != o.isGenomic) return false;
			if (isGenomic) return (defGenomic == o.defGenomic);
			return (defProtein == o.defProtein);
		}
	};

	unsigned IdRecord::dataLength() const
	{
		if (isGenomic) {
			return ( 1 + 4 + defGenomic.dataLength() );
		} else {
			return ( 1 + 8 + defProtein.dataLength() );
		}
	}

	void IdRecord::saveData(uint8_t *& ptr) const
	{
		if (isGenomic) {
			writeUnsignedInteger<1>(ptr, 1);
			writeUnsignedInteger<4>(ptr, defGenomic.firstPosition());
			defGenomic.saveData(ptr);
		} else {
			writeUnsignedInteger<1>(ptr, 0);
			writeUnsignedInteger<8>(ptr, defProtein.proteinAccIdAndFirstPosition());
			defProtein.saveData(ptr);
		}
	}

	void IdRecord::loadData(uint8_t const *& ptr)
	{
		isGenomic = ( readUnsignedInteger<1,unsigned>(ptr) == 1 );
		if (isGenomic) {
			defGenomic = BinaryGenomicVariantDefinition(readUnsignedInteger<4,uint32_t>(ptr));
			defGenomic.loadData(ptr);
		} else {
			defProtein = BinaryProteinVariantDefinition(readUnsignedInteger<8,uint64_t>(ptr));
			defProtein.loadData(ptr);
		}
	}

	class RecordWithDefinitions : public RecordT<uint32_t>
	{
	public:
		std::vector<BinaryGenomicVariantDefinition> defsGenomic;
		std::vector<BinaryProteinVariantDefinition> defsProtein;
		RecordWithDefinitions(uint32_t id) : RecordT<uint32_t>(id) { }
		virtual unsigned dataLength() const { return 0; }
		virtual void saveData(uint8_t *& ptr) const {}
		virtual void loadData(uint8_t const *& ptr) {}
	};

	struct IndexIdentifierUInt32::Pim
	{
		std::string const dirPath;
		DatabaseT<> db;
		Pim(std::string const & pDirPath, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, std::string const & name, unsigned cacheInMB)
		: dirPath(pDirPath)
		, db(cpuTaskManager, ioTaskManager, dirPath + "id" + name, createRecord<IdRecord>, cacheInMB)
		{}
	};


	IndexIdentifierUInt32::IndexIdentifierUInt32(std::string dirPath, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, std::string const & name, unsigned cacheInMB)
	{
		if ( (! dirPath.empty()) && dirPath.back() != '/') dirPath += "/";
		pim = new Pim(dirPath, cpuTaskManager, ioTaskManager, name, cacheInMB);
		std::cout << "index " << name << ":\trecordsCount=" << pim->db.getRecordsCount() << "\ttheLargestKey=" << pim->db.getTheLargestKey() << "\n";
	}

	IndexIdentifierUInt32::~IndexIdentifierUInt32()
	{
		delete pim;
	}


	std::vector<std::vector<RecordVariantPtr>> IndexIdentifierUInt32::queryDefinitions(std::vector<uint32_t> const & ids) const
	{
		std::vector<RecordT<uint32_t>*> records;
		records.reserve(ids.size());
		for (auto id: ids) {
			records.push_back( new RecordWithDefinitions(id) );
		}

		auto visitor = [](std::vector<RecordT<uint32_t> const *> const & dbRecords, std::vector<RecordT<uint32_t> *> const & records)
		{
			if (records.empty()) throw std::logic_error("No records in visitor's callback!");
			if (dbRecords.empty()) return;
			RecordWithDefinitions * rec = dynamic_cast<RecordWithDefinitions*>(records.front());
			for (auto r2: dbRecords) {
				IdRecord const * dbRecord = dynamic_cast<IdRecord const *>( r2 );
				if (dbRecord->isGenomic) {
					rec->defsGenomic.push_back( dbRecord->defGenomic );
				} else {
					rec->defsProtein.push_back( dbRecord->defProtein );
				}
			}
			for (unsigned i = 1; i < records.size(); ++i) {
				(dynamic_cast<RecordWithDefinitions*>(records[i]))->defsGenomic = rec->defsGenomic;
				(dynamic_cast<RecordWithDefinitions*>(records[i]))->defsProtein = rec->defsProtein;
			}
		};

		pim->db.readRecords(records, visitor);

		std::vector<std::vector<RecordVariantPtr>> output(ids.size());
		for (unsigned i = 0; i < output.size(); ++i) {
			RecordWithDefinitions * r = dynamic_cast<RecordWithDefinitions*>(records[i]);
			for (BinaryGenomicVariantDefinition const & vd: r->defsGenomic) {
				output[i].push_back( new RecordGenomicVariant(vd) );
			}
			for (BinaryProteinVariantDefinition const & vd: r->defsProtein) {
				output[i].push_back( new RecordProteinVariant(vd) );
			}
			delete r;
		}

		return output;
	}


	void IndexIdentifierUInt32::addIdentifiers(std::vector<std::pair<uint32_t,RecordVariantPtr>> const & identifiersToAdd)
	{
		std::vector<RecordT<uint32_t>*> records;
		records.reserve(identifiersToAdd.size());
		for (auto id2rec: identifiersToAdd) {
			IdRecord * r = new IdRecord(id2rec.first);
			if (id2rec.second.isRecordGenomicVariantPtr()) {
				r->isGenomic = true;
				r->defGenomic = id2rec.second.asRecordGenomicVariantPtr()->definition;
			} else {
				r->isGenomic = false;
				r->defProtein = id2rec.second.asRecordProteinVariantPtr()->definition;
			}
			records.push_back( r );
		}

		auto visitor = [](std::vector<RecordT<uint32_t>*> & dbRecords, std::vector<RecordT<uint32_t> *> const & records) -> bool
		{
			if (records.empty()) throw std::logic_error("No records in visitor's callback!");
			bool changes = false;
			for (auto r: records) {
				IdRecord * newRecord = dynamic_cast<IdRecord*>(r);
				for (auto r2: dbRecords) {
					IdRecord * dbRecord = dynamic_cast<IdRecord*>(r2);
					if (*newRecord == *dbRecord) {
						delete newRecord;
						newRecord = nullptr;
						break;
					}
				}
				if (newRecord != nullptr) {
					dbRecords.push_back(newRecord);
					changes = true;
				}
			}
			return changes;
		};

		pim->db.writeRecords(records, visitor);
	}


	void IndexIdentifierUInt32::deleteIdentifiers(std::vector<std::pair<uint32_t,RecordVariantPtr>> const & identifiersToDelete)
	{
		std::vector<RecordT<uint32_t>*> records;
		records.reserve(identifiersToDelete.size());
		for (auto id2rec: identifiersToDelete) {
			IdRecord * r = new IdRecord(id2rec.first);
			if (id2rec.second.isRecordGenomicVariantPtr()) {
				r->isGenomic = true;
				r->defGenomic = id2rec.second.asRecordGenomicVariantPtr()->definition;
			} else {
				r->isGenomic = false;
				r->defProtein = id2rec.second.asRecordProteinVariantPtr()->definition;
			}
			records.push_back( r );
		}

		auto visitor = [](std::vector<RecordT<uint32_t>*> & dbRecords, std::vector<RecordT<uint32_t> *> const & records) -> bool
		{
			if (records.empty()) throw std::logic_error("No records in visitor's callback!");
			bool changes = false;
			for (auto r: records) {
				IdRecord * delRecord = dynamic_cast<IdRecord*>(r);
				for (unsigned i = 0; i < dbRecords.size(); ++i) {
					IdRecord * dbRecord = dynamic_cast<IdRecord*>(dbRecords[i]);
					if (*delRecord == *dbRecord) {
						delete dbRecord;
						dbRecords[i] = dbRecords.back();
						dbRecords.pop_back();
						changes = true;
						break;
					}
				}
				delete delRecord;
			}
			return changes;
		};

		pim->db.writeRecords(records, visitor);
	}


	void IndexIdentifierUInt32::deleteEntries( tCallbackWithDeletedRef callback, uint32_t first, uint32_t last, unsigned minChunkSize )
	{

		bool globalLastCall = false;
		while (! globalLastCall) {

			// -------- query part of index to delete
			std::vector<RecordT<uint32_t>*> recordsToDelete;
			auto readFunc = [&recordsToDelete,minChunkSize,&globalLastCall](std::vector<RecordT<uint32_t> const *> const & records, bool & lastCall)->void
			{
				for (auto r: records) {
					recordsToDelete.push_back( new IdRecord(*(dynamic_cast<IdRecord const *>(r))) );
				}
				if (lastCall) {
					globalLastCall = lastCall;
				} else {
					lastCall = (recordsToDelete.size() >= minChunkSize);
				}
			};
			pim->db.readRecordsInOrder(readFunc, first, last);

			// ------- delete entries from index
			std::vector<RecordVariantPtr> refsToDelete;
			auto deleteFunc = [](std::vector<RecordT<uint32_t>*> & currentRecords, std::vector<RecordT<uint32_t>*> const & newRecords)->bool
			{
				for (auto & r: currentRecords) {
					delete r;
				}
				currentRecords.clear();
				return true;
			};
			pim->db.writeRecords(recordsToDelete,deleteFunc);

			// ------ return deleted entries
			std::vector<RecordVariantPtr> records;
			for (auto r: recordsToDelete) {
				IdRecord * rec = dynamic_cast<IdRecord*>(r);
				if (rec->isGenomic) {
					records.push_back( new RecordGenomicVariant(rec->defGenomic) );
				} else {
					records.push_back( new RecordProteinVariant(rec->defProtein) );
				}
				delete rec;
			}
			callback(records, globalLastCall);
		}
	}

	bool IndexIdentifierUInt32::isNewDb() const
	{
		return (pim->db.isNewDb());
	}

