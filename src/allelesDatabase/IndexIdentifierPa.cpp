#include "IndexIdentifierPa.hpp"
#include "../apiDb/db.hpp"
#include "../commonTools/bytesLevel.hpp"
#include <boost/lexical_cast.hpp>

	enum paRecordType : uint8_t
	{
		proteinVariation = 1
	};

	class PaRecord : public RecordT<uint32_t>
	{
	public:
		paRecordType type = paRecordType::proteinVariation;
		BinaryProteinVariantDefinition definition; // must be sorted by position
		PaRecord(uint32_t paId) : RecordT<uint32_t>(paId), definition(0) { }
		virtual unsigned dataLength() const;
		virtual void saveData(uint8_t *& ptr) const;
		virtual void loadData(uint8_t const *& ptr);
		virtual ~PaRecord() {}
	};

	unsigned PaRecord::dataLength() const
	{
		return (1 + 8 + definition.dataLength());
	}

	void PaRecord::saveData(uint8_t *& ptr) const
	{
//		std::cout << "BEGIN - PA" << std::endl;
		writeUnsignedInteger<1>(ptr, static_cast<unsigned>(type));
		writeUnsignedInteger<8>(ptr, definition.proteinAccIdAndFirstPosition());
		definition.saveData(ptr);
//		std::cout << "END - PA" << std::endl;
	}

	void PaRecord::loadData(uint8_t const *& ptr)
	{
		type = static_cast<paRecordType>(readUnsignedInteger<1,unsigned>(ptr));
		definition = BinaryProteinVariantDefinition( readUnsignedInteger<8,uint64_t>(ptr) );
		definition.loadData(ptr);
	}


	struct IndexIdentifierPa::Pim
	{
		std::string const dirPath;
		DatabaseT<> db;
		Pim(std::string const & pDirPath, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB)
		: dirPath(pDirPath)
		, db(cpuTaskManager, ioTaskManager, dirPath + "idPa", createRecord<PaRecord>, cacheInMB)
		{}
	};

	IndexIdentifierPa::IndexIdentifierPa(std::string dirPath, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB) : pim(nullptr)
	{
		if ( (! dirPath.empty()) && dirPath.back() != '/') dirPath += "/";
		pim = new Pim(dirPath, cpuTaskManager, ioTaskManager, cacheInMB);
		std::cout << "index PA:\trecordsCount=" << pim->db.getRecordsCount() << "\ttheLargestKey=" << pim->db.getTheLargestKey() << "\n";
	}

	IndexIdentifierPa::~IndexIdentifierPa()
	{
		delete pim;
	}

	std::vector<RecordProteinVariant*> IndexIdentifierPa::fetchDefinitions( std::vector<uint32_t> const & paIds) const
	{
		std::vector<RecordT<uint32_t>*> records;
		records.reserve(paIds.size());
		for (auto id: paIds) {
			records.push_back( new PaRecord(id) );
		}

		auto visitor = [](std::vector<RecordT<uint32_t> const *> const & dbRecords, std::vector<RecordT<uint32_t> *> const & records)
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			if (dbRecords.empty()) return;
			if (dbRecords.size() > 1) throw std::logic_error("More than one record with the same PA ID: " + boost::lexical_cast<std::string>(records.front()->key));
			PaRecord const * dbRecord = dynamic_cast<PaRecord const *>( dbRecords.front() );
			for (auto r: records) {
				PaRecord * rec = dynamic_cast<PaRecord*>(r);
				rec->type = dbRecord->type;
				rec->definition = dbRecord->definition;
			}
		};

		pim->db.readRecords(records, visitor);

		std::vector<RecordProteinVariant*> outRecords(records.size(), nullptr);
		for (unsigned i = 0; i < records.size(); ++i) {
			PaRecord * rec = dynamic_cast<PaRecord*>(records[i]);
			if ( rec->definition.raw().size() > 1 || rec->definition.proteinAccIdAndFirstPosition() > 0 ) {  // TODO - null or something
				outRecords[i] = new RecordProteinVariant(rec->definition);
				outRecords[i]->identifiers.lastId() = rec->key;
			}
			delete rec;
		}

		return outRecords;
	}

	void IndexIdentifierPa::addIdentifiers(std::vector<RecordProteinVariant const *> const & varRecords)
	{
		std::vector<RecordT<uint32_t>*> records;
		records.reserve(varRecords.size());
		for (auto r: varRecords) {
			PaRecord *cr = new PaRecord(r->identifiers.lastId());
			cr->type = paRecordType::proteinVariation;
			cr->definition = r->definition;
			records.push_back( cr );
		}

		auto visitor = [](std::vector<RecordT<uint32_t>*> & dbRecords, std::vector<RecordT<uint32_t> *> const & records) -> bool
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			if (! dbRecords.empty()) { //throw std::logic_error("PA ID already exists: " + boost::lexical_cast<std::string>(records.front()->key));
				if ( dynamic_cast<PaRecord*>(dbRecords[0])->definition != dynamic_cast<PaRecord*>(records[0])->definition ) {
					std::cout << "Conflict (1) PA" << records.front()->key << " with: " << dynamic_cast<PaRecord*>(dbRecords.front())->definition.toString() << std::endl;
				}
				return false;
			}
			if (records.size() > 1) { //throw std::logic_error("More than one record with the same PA ID: " + boost::lexical_cast<std::string>(records.front()->key));
				for (unsigned i = 1; i < records.size(); ++i) {
					if (dynamic_cast<PaRecord*>(records[i])->definition != dynamic_cast<PaRecord*>(records[0])->definition) {
						std::cout << "Conflict (2) PA" << records[0]->key << " with: " << dynamic_cast<PaRecord*>(records[i])->definition.toString() << std::endl;
					}
				}
			}
			dbRecords.push_back( records[0] );
			return true;
		};

		pim->db.writeRecords(records, visitor);
	}

	uint32_t IndexIdentifierPa::getMaxIdentifier() const
	{
		return pim->db.getTheLargestKey();
	}

	void IndexIdentifierPa::deleteIdentifiers(std::vector<RecordProteinVariant*> & varRecords)
	{
		std::vector<RecordT<uint32_t>*> records;
		records.reserve(varRecords.size());
		for (auto r: varRecords) {
			PaRecord *cr = new PaRecord(r->identifiers.lastId());
			cr->type = paRecordType::proteinVariation;
			cr->definition = r->definition;
			records.push_back( cr );
		}

		auto visitor = [](std::vector<RecordT<uint32_t>*> & dbRecords, std::vector<RecordT<uint32_t> *> const & records) -> bool
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			if (dbRecords.empty()) return false;
			for (auto r: dbRecords) delete r;
			dbRecords.clear();
			return true;
		};

		pim->db.writeRecords(records, visitor);
	}

	bool IndexIdentifierPa::isNewDb() const
	{
		return (pim->db.isNewDb());
	}

