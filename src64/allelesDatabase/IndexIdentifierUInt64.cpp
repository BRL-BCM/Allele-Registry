#include "IndexIdentifierUInt64.hpp"
#include "RecordVariant64.hpp"
#include "../commonTools/bytesLevel.hpp"

	class IdRecord64 : public RecordT<uint64_t>
	{
	public:
		BinaryGenomicVariantDefinition defGenomic;
		BinaryProteinVariantDefinition defProtein;
		bool isGenomic;
		IdRecord64(uint64_t id) : RecordT<uint64_t>(id), defGenomic(0), defProtein(0), isGenomic(true) { }
		virtual unsigned dataLength() const;
		virtual void saveData(uint8_t *& ptr) const;
		virtual void loadData(uint8_t const *& ptr);
		virtual ~IdRecord64() {}
		inline bool operator==(IdRecord64 const & o) const
		{
			if (isGenomic != o.isGenomic) return false;
			if (isGenomic) return (defGenomic == o.defGenomic);
			return (defProtein == o.defProtein);
		}
	};

	unsigned IdRecord64::dataLength() const
	{
		if (isGenomic) {
			return ( 1 + 8 + defGenomic.dataLength() );
		} else {
			return ( 1 + 8 + defProtein.dataLength() );
		}
	}

	void IdRecord64::saveData(uint8_t *& ptr) const
	{
		if (isGenomic) {
			writeUnsignedInteger<1>(ptr, 1);
			writeUnsignedInteger<8>(ptr, defGenomic.firstPosition());
			defGenomic.saveData(ptr);
		} else {
			writeUnsignedInteger<1>(ptr, 0);
			writeUnsignedInteger<8>(ptr, defProtein.proteinAccIdAndFirstPosition());
			defProtein.saveData(ptr);
		}
	}

	void IdRecord64::loadData(uint8_t const *& ptr)
	{
		isGenomic = ( readUnsignedInteger<1,unsigned>(ptr) == 1 );
		if (isGenomic) {
			defGenomic = BinaryGenomicVariantDefinition(readUnsignedInteger<8,uint64_t>(ptr));
			defGenomic.loadData(ptr);
		} else {
			defProtein = BinaryProteinVariantDefinition(readUnsignedInteger<8,uint64_t>(ptr));
			defProtein.loadData(ptr);
		}
	}

	class RecordWithDefinitions64 : public RecordT<uint64_t>
	{
	public:
		std::vector<BinaryGenomicVariantDefinition> defsGenomic;
		std::vector<BinaryProteinVariantDefinition> defsProtein;
		RecordWithDefinitions64(uint64_t id) : RecordT<uint64_t>(id) { }
		virtual unsigned dataLength() const { return 0; }
		virtual void saveData(uint8_t *& ptr) const {}
		virtual void loadData(uint8_t const *& ptr) {}
	};

	struct IndexIdentifierUInt64::Pim
	{
		std::string const dirPath;
		Header const header;
		DatabaseT<uint64_t,8> db;
		Pim(std::string const & pDirPath, Header const & pHeader, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, std::string const & name, unsigned cacheInMB)
		: dirPath(pDirPath)
		, header(pHeader)
		, db(cpuTaskManager, ioTaskManager, dirPath + "id" + name, header, createRecord<IdRecord64>, cacheInMB)
		{}
	};


	IndexIdentifierUInt64::IndexIdentifierUInt64(std::string dirPath, Header header, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, std::string const & name, unsigned cacheInMB)
	{
		if ( (! dirPath.empty()) && dirPath.back() != '/') dirPath += "/";
		pim = new Pim(dirPath, header, cpuTaskManager, ioTaskManager, name, cacheInMB);
		std::cout << "index " << name << ":\trecordsCount=" << pim->db.getRecordsCount() << "\ttheLargestKey=" << pim->db.getTheLargestKey() << "\n";
	}

	IndexIdentifierUInt64::~IndexIdentifierUInt64()
	{
		delete pim;
	}


	std::vector<std::vector<RecordVariantPtr>> IndexIdentifierUInt64::queryDefinitions(std::vector<uint64_t> const & ids) const
	{
		std::vector<RecordT<uint64_t>*> records;
		records.reserve(ids.size());
		for (auto id: ids) {
			records.push_back( new RecordWithDefinitions64(id) );
		}

		auto visitor = [](std::vector<RecordT<uint64_t> const *> const & dbRecords, std::vector<RecordT<uint64_t> *> const & records)
		{
			if (records.empty()) throw std::logic_error("No records in visitor's callback!");
			if (dbRecords.empty()) return;
			RecordWithDefinitions64 * rec = dynamic_cast<RecordWithDefinitions64*>(records.front());
			for (auto r2: dbRecords) {
				IdRecord64 const * dbRecord = dynamic_cast<IdRecord64 const *>( r2 );
				if (dbRecord->isGenomic) {
					rec->defsGenomic.push_back( dbRecord->defGenomic );
				} else {
					rec->defsProtein.push_back( dbRecord->defProtein );
				}
			}
			for (unsigned i = 1; i < records.size(); ++i) {
				(dynamic_cast<RecordWithDefinitions64*>(records[i]))->defsGenomic = rec->defsGenomic;
				(dynamic_cast<RecordWithDefinitions64*>(records[i]))->defsProtein = rec->defsProtein;
			}
		};

		pim->db.readRecords(records, visitor);

		std::vector<std::vector<RecordVariantPtr>> output(ids.size());
		for (unsigned i = 0; i < output.size(); ++i) {
			RecordWithDefinitions64 * r = dynamic_cast<RecordWithDefinitions64*>(records[i]);
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


	void IndexIdentifierUInt64::addIdentifiers(std::vector<std::pair<uint64_t,RecordVariantPtr>> const & identifiersToAdd)
	{
		std::vector<RecordT<uint64_t>*> records;
		records.reserve(identifiersToAdd.size());
		for (auto id2rec: identifiersToAdd) {
			IdRecord64 * r = new IdRecord64(id2rec.first);
			if (id2rec.second.isRecordGenomicVariantPtr()) {
				r->isGenomic = true;
				r->defGenomic = id2rec.second.asRecordGenomicVariantPtr()->definition;
			} else {
				r->isGenomic = false;
				r->defProtein = id2rec.second.asRecordProteinVariantPtr()->definition;
			}
			records.push_back( r );
		}

		auto visitor = [](std::vector<RecordT<uint64_t>*> & dbRecords, std::vector<RecordT<uint64_t> *> const & records) -> bool
		{
			if (records.empty()) throw std::logic_error("No records in visitor's callback!");
			bool changes = false;
			for (auto r: records) {
				IdRecord64 * newRecord = dynamic_cast<IdRecord64*>(r);
				for (auto r2: dbRecords) {
					IdRecord64 * dbRecord = dynamic_cast<IdRecord64*>(r2);
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
		//for(auto r: records) {
		//	if(r != nullptr) {
		//		delete r;
		//		r = nullptr;
		//	}
		//}
		//records.clear();
	}


	void IndexIdentifierUInt64::deleteIdentifiers(std::vector<std::pair<uint64_t,RecordVariantPtr>> const & identifiersToDelete)
	{
		std::vector<RecordT<uint64_t>*> records;
		records.reserve(identifiersToDelete.size());
		for (auto id2rec: identifiersToDelete) {
			IdRecord64 * r = new IdRecord64(id2rec.first);
			if (id2rec.second.isRecordGenomicVariantPtr()) {
				r->isGenomic = true;
				r->defGenomic = id2rec.second.asRecordGenomicVariantPtr()->definition;
			} else {
				r->isGenomic = false;
				r->defProtein = id2rec.second.asRecordProteinVariantPtr()->definition;
			}
			records.push_back( r );
		}

		auto visitor = [](std::vector<RecordT<uint64_t>*> & dbRecords, std::vector<RecordT<uint64_t> *> const & records) -> bool
		{
			if (records.empty()) throw std::logic_error("No records in visitor's callback!");
			bool changes = false;
			for (auto r: records) {
				IdRecord64 * delRecord = dynamic_cast<IdRecord64*>(r);
				for (unsigned i = 0; i < dbRecords.size(); ++i) {
					IdRecord64 * dbRecord = dynamic_cast<IdRecord64*>(dbRecords[i]);
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


	void IndexIdentifierUInt64::deleteEntries( tCallbackWithDeletedRef callback, uint64_t first, uint64_t last, unsigned minChunkSize )
	{

		bool globalLastCall = false;
		while (! globalLastCall) {

			// -------- query part of index to delete
			std::vector<RecordT<uint64_t>*> recordsToDelete;
			auto readFunc = [&recordsToDelete,minChunkSize,&globalLastCall](std::vector<RecordT<uint64_t> const *> const & records, bool & lastCall)->void
			{
				for (auto r: records) {
					recordsToDelete.push_back( new IdRecord64(*(dynamic_cast<IdRecord64 const *>(r))) );
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
			auto deleteFunc = [](std::vector<RecordT<uint64_t>*> & currentRecords, std::vector<RecordT<uint64_t>*> const & newRecords)->bool
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
				IdRecord64 * rec = dynamic_cast<IdRecord64*>(r);
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

	bool IndexIdentifierUInt64::isNewDb() const
	{
		return (pim->db.isNewDb());
	}

