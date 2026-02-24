#include "TableNamed.hpp"
#include "../commonTools/bytesLevel.hpp"
#include "../apiDb/db.hpp"
#include <boost/thread/mutex.hpp>

	class NamedRecord : public RecordT<uint64_t>
	{
	public:
		bool isNamed;
		NamedRecord(uint64_t caId) : RecordT<uint64_t>(caId), isNamed(false) { }
		virtual unsigned dataLength() const;
		virtual void saveData(uint8_t *& ptr) const;
		virtual void loadData(uint8_t const *& ptr);
		virtual ~NamedRecord() {}
		inline bool operator==(NamedRecord const & o) const
		{
			return isNamed == o.isNamed;
		}
		inline bool operator!=(NamedRecord const & o) const
		{
			return isNamed != o.isNamed;
		}
	};

	unsigned NamedRecord::dataLength() const
	{
		return 1;
	}

	void NamedRecord::saveData(uint8_t *& ptr) const
	{
		if(isNamed) {
			writeUnsignedInteger<1>(ptr, 1);
		} else {
			writeUnsignedInteger<1>(ptr, 0);
		}
	}

	void NamedRecord::loadData(uint8_t const *& ptr)
	{
		isNamed = ( readUnsignedInteger<1,unsigned>(ptr) == 1 );
	}

	// ================================================ TABLE

	template<typename RecordType>
	struct TableNamed<RecordType>::Pim
	{
		std::string const dirPath;
		std::string const fileName;
		Header const header;
		DatabaseT<uint64_t,8> db;
		Pim(std::string const & pDirPath, std::string const & pFileName, Header const & pHeader, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB)
		: dirPath(pDirPath)
		, fileName(pFileName)
		, header(pHeader)
		, db(cpuTaskManager, ioTaskManager, dirPath + fileName, header, createRecord<NamedRecord>, cacheInMB)
		{}
	};

	template<typename RecordType>
	TableNamed<RecordType>::TableNamed(std::string dirPath, std::string fileName, Header header, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB)
	{
		if ( (! dirPath.empty()) && dirPath.back() != '/') dirPath += "/";
		pim = new Pim(dirPath, fileName, header, cpuTaskManager, ioTaskManager, cacheInMB);
		//std::cout << "named " << ":\trecordsCount=" << pim->db.getRecordsCount() << "\ttheLargestKey=" << pim->db.getTheLargestKey() << "\n";
	}

	template<typename RecordType>
	TableNamed<RecordType>::~TableNamed()
	{
		delete pim;
	}

	template<typename RecordType>
	std::vector<bool> TableNamed<RecordType>::fetch(std::vector<uint64_t> const & caIds) const
	{
		std::vector<RecordT<uint64_t>*> records;
		records.reserve(caIds.size());
		for (auto id: caIds) {
			records.push_back( new NamedRecord(id) );
		}

		auto visitor = [](std::vector<RecordT<uint64_t> const *> const & dbRecords, std::vector<RecordT<uint64_t> *> const & records)
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			if (dbRecords.empty()) return;
			//if (dbRecords.size() > 1) throw std::logic_error("More than one record with the same CA ID: " + boost::lexical_cast<std::string>(records.front()->key));
			NamedRecord const * dbRecord = dynamic_cast<NamedRecord const *>( dbRecords.front() );
			for (auto r: records) {
				NamedRecord * rec = dynamic_cast<NamedRecord*>(r);
				rec->isNamed = dbRecord->isNamed;
			}
		};

		{
			pim->db.readRecords(records, visitor);
		}

		std::vector<bool> outRecords(records.size(), false);
		for (unsigned i = 0; i < records.size(); ++i) {
			NamedRecord * rec = dynamic_cast<NamedRecord*>(records[i]);
			outRecords[i] = rec->isNamed;
			delete rec;
		}
		return outRecords;
	}

	// correct
	// exist T => F
	// exist F => F
	//   new T => T do nothing
	//   new F => F do nothing
	template<typename RecordType>
	void TableNamed<RecordType>::downset(std::vector<RecordType *> const & varRecords)
	{
		std::vector<RecordT<uint64_t>*> records;
		records.reserve(varRecords.size());
		for (unsigned i=0; i<varRecords.size(); i++) {
			NamedRecord *cr = new NamedRecord(varRecords[i]->identifiers.lastId());
			cr->isNamed = false;
			records.push_back( cr );
		}

		auto visitor = [](std::vector<RecordT<uint64_t>*> & dbRecords, std::vector<RecordT<uint64_t> *> const & records) -> bool
		{
			if (records.empty()) throw std::logic_error("No records in visitor's callback!");

			// ======================= search for duplicated records in the input, make sure that duplicated records have the same identifiers
			for (unsigned i = 0; i < records.size(); ++i) {
				for (unsigned j = i+1; j < records.size(); ++j) {
					NamedRecord * r1 = dynamic_cast<NamedRecord*>(records[i]);
					NamedRecord * r2 = dynamic_cast<NamedRecord*>(records[j]);
					if (r1->key != r2->key) throw std::logic_error("Key mismatch");
				}
			}

			// ======================= modifications on the db records
			bool changes = false;
			for (auto r: records) {
				NamedRecord * newRecord = dynamic_cast<NamedRecord*>(r);
				NamedRecord * dbRec = nullptr;
				for (auto r2: dbRecords) {
					NamedRecord * dbRecord = dynamic_cast<NamedRecord*>(r2);
					if (r->key != r2->key) throw std::logic_error("Key mismatch");
					dbRec = dbRecord;
					break;
				}
				if(dbRec == nullptr) { // not found in database, new record
					dbRec = new NamedRecord(r->key);
					dbRec->isNamed = false;
					dbRecords.push_back(dbRec);
					changes = true;
				} else { // found in database, change T -> F
					if(newRecord->isNamed != false) throw std::logic_error("dbRec incorrect");
					if(newRecord->isNamed == dbRec->isNamed) {
						// good, no change needed
					} else {
						dbRec->isNamed = false;
						changes = true;
					}
				}
				delete newRecord;
			}
			return changes;
		};

		pim->db.writeRecords(records, visitor);

	}

	// exist T => T do nothing
	// exist F => F do nothing
	//   new T => T
	//   new F => T
	template<typename RecordType>
	void TableNamed<RecordType>::upset(std::vector<RecordType *> const & varRecords)
	{
		std::vector<RecordT<uint64_t>*> records;
		records.reserve(varRecords.size());
		for (unsigned i=0; i<varRecords.size(); i++) {
			NamedRecord *cr = new NamedRecord(varRecords[i]->identifiers.lastId());
			cr->isNamed = true;
			records.push_back( cr );
		}

		auto visitor = [](std::vector<RecordT<uint64_t>*> & dbRecords, std::vector<RecordT<uint64_t> *> const & records) -> bool
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			if (! dbRecords.empty()) {
					return false;
			}
			if (records.size() > 1) {
				for (unsigned i = 1; i < records.size(); ++i) {
					if (dynamic_cast<NamedRecord*>(records[i])->isNamed != dynamic_cast<NamedRecord*>(records[0])->isNamed) {
						//std::cout << "conflict" << std::endl;
					}
				}
			}
			//std::cout << ">>" << dynamic_cast<NamedRecord*>(records[0])->key << "\t" << dynamic_cast<NamedRecord*>(records[0])->isNamed << std::endl;
			for (auto r: dbRecords) delete r;
			dbRecords.clear();
			dbRecords.push_back( records[0] );
			return true;
		};

		{
			pim->db.writeRecords(records, visitor);
		}
	}

	template<typename RecordType>
	void TableNamed<RecordType>::upset(std::vector<RecordType const *> const & varRecords)
	{
		std::vector<RecordT<uint64_t>*> records;
		records.reserve(varRecords.size());
		for (unsigned i=0; i<varRecords.size(); i++) {
			NamedRecord *cr = new NamedRecord(varRecords[i]->identifiers.lastId());
			cr->isNamed = true;
			records.push_back( cr );
		}

		auto visitor = [](std::vector<RecordT<uint64_t>*> & dbRecords, std::vector<RecordT<uint64_t> *> const & records) -> bool
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			if (! dbRecords.empty()) {
					return false;
			}
			if (records.size() > 1) {
				for (unsigned i = 1; i < records.size(); ++i) {
					if (dynamic_cast<NamedRecord*>(records[i])->isNamed != dynamic_cast<NamedRecord*>(records[0])->isNamed) {
						//std::cout << "conflict" << std::endl;
					}
				}
			}
			//std::cout << ">>" << dynamic_cast<NamedRecord*>(records[0])->key << "\t" << dynamic_cast<NamedRecord*>(records[0])->isNamed << std::endl;
			for (auto r: dbRecords) delete r;
			dbRecords.clear();
			dbRecords.push_back( records[0] );
			return true;
		};

		{
			pim->db.writeRecords(records, visitor);
		}
	}

	// exist T => F
	// exist F => F
	//   new T => F
	//   new F => F
	template<typename RecordType>
	void TableNamed<RecordType>::unset(std::vector<RecordType *> const & varRecords)
	{
		std::vector<RecordT<uint64_t>*> records;
		records.reserve(varRecords.size());
		for (unsigned i=0; i<varRecords.size(); i++) {
			NamedRecord *cr = new NamedRecord(varRecords[i]->identifiers.lastId());
			cr->isNamed = false;
			records.push_back( cr );
		}

		auto visitor = [](std::vector<RecordT<uint64_t>*> & dbRecords, std::vector<RecordT<uint64_t> *> const & records) -> bool
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			if (! dbRecords.empty()) {
				if (dynamic_cast<NamedRecord*>(records[0])->isNamed == dynamic_cast<NamedRecord*>(dbRecords[dbRecords.size()-1])->isNamed) {
					return false;
				}
			}
			if (records.size() > 1) {
				for (unsigned i = 1; i < records.size(); ++i) {
					if (dynamic_cast<NamedRecord*>(records[i])->isNamed != dynamic_cast<NamedRecord*>(records[0])->isNamed) {
						//std::cout << "conflict" << std::endl;
					}
				}
			}
			//std::cout << ">>" << dynamic_cast<NamedRecord*>(records[0])->key << "\t" << dynamic_cast<NamedRecord*>(records[0])->isNamed << std::endl;
			for (auto r: dbRecords) delete r;
			dbRecords.clear();
			dbRecords.push_back( records[0] );
			return true;
		};

		{
			pim->db.writeRecords(records, visitor);
		}
	}

	// exist T => T
	// exist F => T
	//   new T => T
	//   new F => T
	template<typename RecordType>
	void TableNamed<RecordType>::set(std::vector<RecordType *> const & varRecords)
	{
		std::vector<RecordT<uint64_t>*> records;
		records.reserve(varRecords.size());
		for (unsigned i=0; i<varRecords.size(); i++) {
			NamedRecord *cr = new NamedRecord(varRecords[i]->identifiers.lastId());
			cr->isNamed = true;
			records.push_back( cr );
		}

		auto visitor = [](std::vector<RecordT<uint64_t>*> & dbRecords, std::vector<RecordT<uint64_t> *> const & records) -> bool
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			if (! dbRecords.empty()) {
				if (dynamic_cast<NamedRecord*>(records[0])->isNamed == dynamic_cast<NamedRecord*>(dbRecords[dbRecords.size()-1])->isNamed) {
					return false;
				}
			}
			if (records.size() > 1) {
				for (unsigned i = 1; i < records.size(); ++i) {
					if (dynamic_cast<NamedRecord*>(records[i])->isNamed != dynamic_cast<NamedRecord*>(records[0])->isNamed) {
						//std::cout << "conflict" << std::endl;
					}
				}
			}
			//std::cout << ">>" << dynamic_cast<NamedRecord*>(records[0])->key << "\t" << dynamic_cast<NamedRecord*>(records[0])->isNamed << std::endl;
			for (auto r: dbRecords) delete r;
			dbRecords.clear();
			dbRecords.push_back( records[0] );
			return true;
		};

		{
			pim->db.writeRecords(records, visitor);
		}
	}

	template<typename RecordType>
	void TableNamed<RecordType>::write(std::vector<RecordType *> const & varRecords, std::vector<bool> const & named)
	{
		if(varRecords.size() != named.size()) throw std::logic_error("please make sure the varRecords vector and named vector are the same size");
		std::vector<RecordT<uint64_t>*> records;
		records.reserve(varRecords.size());
		for (unsigned i=0; i<varRecords.size(); i++) {
			NamedRecord *cr = new NamedRecord(varRecords[i]->identifiers.lastId());
			cr->isNamed = named[i];
			records.push_back( cr );
		}

		auto visitor = [](std::vector<RecordT<uint64_t>*> & dbRecords, std::vector<RecordT<uint64_t> *> const & records) -> bool
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			if (! dbRecords.empty()) {
				if (dynamic_cast<NamedRecord*>(records[0])->isNamed == dynamic_cast<NamedRecord*>(dbRecords[dbRecords.size()-1])->isNamed) {
					return false;
				}
			}
			if (records.size() > 1) {
				for (unsigned i = 1; i < records.size(); ++i) {
					if (dynamic_cast<NamedRecord*>(records[i])->isNamed != dynamic_cast<NamedRecord*>(records[0])->isNamed) {
						//std::cout << "conflict" << std::endl;
					}
				}
			}
			//std::cout << ">>" << dynamic_cast<NamedRecord*>(records[0])->key << "\t" << dynamic_cast<NamedRecord*>(records[0])->isNamed << std::endl;
			for (auto r: dbRecords) delete r;
			dbRecords.clear();
			dbRecords.push_back( records[0] );
			return true;
		};

		{
			pim->db.writeRecords(records, visitor);
		}
	}

	template<typename RecordType>
	uint64_t TableNamed<RecordType>::getMaxIdentifier() const
	{
		return (uint64_t)pim->db.getTheLargestKey();
	}

	template<typename RecordType>
	void TableNamed<RecordType>::deleteRecords(std::vector<RecordType *> & varRecords)
	{
		std::vector<RecordT<uint64_t>*> records;
		records.reserve(varRecords.size());
		for (auto r: varRecords) {
			NamedRecord *cr = new NamedRecord(r->identifiers.lastId());
			records.push_back( cr );
		}

		auto visitor = [](std::vector<RecordT<uint64_t>*> & dbRecords, std::vector<RecordT<uint64_t> *> const & records) -> bool
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			if (dbRecords.empty()) return false;
			for (auto r: dbRecords) delete r;
			dbRecords.clear();
			return true;
		};

		{
			pim->db.writeRecords(records, visitor);
		}
	}

	template<typename RecordType>
	bool TableNamed<RecordType>::isNewDb() const
	{
		return (pim->db.isNewDb());
	}

	template class TableNamed<RecordGenomicVariant>;
	template class TableNamed<RecordProteinVariant>;

