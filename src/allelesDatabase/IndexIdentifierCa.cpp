#include "IndexIdentifierCa.hpp"
#include "../apiDb/db.hpp"
#include "../commonTools/bytesLevel.hpp"
#include <boost/lexical_cast.hpp>

	enum caRecordType : uint8_t
	{
		genomicVariation = 1
	};

	class CaRecord : public RecordT<uint32_t>
	{
	public:
		caRecordType type = caRecordType::genomicVariation;
		BinaryGenomicVariantDefinition definition; // must be sorted by position
		CaRecord(uint32_t caId) : RecordT<uint32_t>(caId), definition(0) { }
		virtual unsigned dataLength() const;
		virtual void saveData(uint8_t *& ptr) const;
		virtual void loadData(uint8_t const *& ptr);
		virtual ~CaRecord() {}
	};

	unsigned CaRecord::dataLength() const
	{
		return (1 + 4 + definition.dataLength());
	}

	void CaRecord::saveData(uint8_t *& ptr) const
	{
//		std::cout << "BEGIN-CA" << std::endl;
		writeUnsignedInteger<1>(ptr, static_cast<unsigned>(type));
		writeUnsignedInteger<4>(ptr, definition.firstPosition());
		definition.saveData(ptr);
//		std::cout << "END-CA" << std::endl;
	}

	void CaRecord::loadData(uint8_t const *& ptr)
	{
		type = static_cast<caRecordType>(readUnsignedInteger<1,unsigned>(ptr));
		definition = BinaryGenomicVariantDefinition( readUnsignedInteger<4,unsigned>(ptr) );
		definition.loadData(ptr);
	}


	struct IndexIdentifierCa::Pim
	{
		std::string const dirPath;
		DatabaseT<> db;
		Pim(std::string const & pDirPath, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB)
		: dirPath(pDirPath)
		, db(cpuTaskManager, ioTaskManager, dirPath + "idCa", createRecord<CaRecord>, cacheInMB)
		{}
	};

	IndexIdentifierCa::IndexIdentifierCa(std::string dirPath, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB) : pim(nullptr)
	{
		if ( (! dirPath.empty()) && dirPath.back() != '/') dirPath += "/";
		pim = new Pim(dirPath, cpuTaskManager, ioTaskManager, cacheInMB);
		std::cout << "index CA:\trecordsCount=" << pim->db.getRecordsCount() << "\ttheLargestKey=" << pim->db.getTheLargestKey() << "\n";
	}

	IndexIdentifierCa::~IndexIdentifierCa()
	{
		delete pim;
	}

	std::vector<RecordGenomicVariant*> IndexIdentifierCa::fetchDefinitions( std::vector<uint32_t> const & caIds) const
	{
		std::vector<RecordT<uint32_t>*> records;
		records.reserve(caIds.size());
		for (auto id: caIds) {
			records.push_back( new CaRecord(id) );
		}

		auto visitor = [](std::vector<RecordT<uint32_t> const *> const & dbRecords, std::vector<RecordT<uint32_t> *> const & records)
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			if (dbRecords.empty()) return;
			if (dbRecords.size() > 1) throw std::logic_error("More than one record with the same CA ID: " + boost::lexical_cast<std::string>(records.front()->key));
			CaRecord const * dbRecord = dynamic_cast<CaRecord const *>( dbRecords.front() );
			for (auto r: records) {
				CaRecord * rec = dynamic_cast<CaRecord*>(r);
				rec->type = dbRecord->type;
				rec->definition = dbRecord->definition;
			}
		};

		pim->db.readRecords(records, visitor);

		std::vector<RecordGenomicVariant*> outRecords(records.size(), nullptr);
		for (unsigned i = 0; i < records.size(); ++i) {
			CaRecord * rec = dynamic_cast<CaRecord*>(records[i]);
			if ( rec->definition.raw().size() > 1 || rec->definition.firstPosition() > 0 ) {  // TODO - null or something
				outRecords[i] = new RecordGenomicVariant(rec->definition);
				outRecords[i]->identifiers.lastId() = rec->key;
			}
			delete rec;
		}

		return outRecords;
	}

	void IndexIdentifierCa::addIdentifiers(std::vector<RecordGenomicVariant const *> const & varRecords)
	{
		std::vector<RecordT<uint32_t>*> records;
		records.reserve(varRecords.size());
		for (auto r: varRecords) {
			CaRecord *cr = new CaRecord(r->identifiers.lastId());
			cr->type = caRecordType::genomicVariation;
			cr->definition = r->definition;
			records.push_back( cr );
		}

		auto visitor = [](std::vector<RecordT<uint32_t>*> & dbRecords, std::vector<RecordT<uint32_t> *> const & records) -> bool
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			if (! dbRecords.empty()) throw std::logic_error("CA ID already exists: " + boost::lexical_cast<std::string>(records.front()->key));
			if (records.size() > 1) throw std::logic_error("More than one record with the same CA ID: " + boost::lexical_cast<std::string>(records.front()->key));
			dbRecords.push_back( records[0] );
			return true;
		};

		pim->db.writeRecords(records, visitor);
	}

	uint32_t IndexIdentifierCa::getMaxIdentifier() const
	{
		return pim->db.getTheLargestKey();
	}

	void IndexIdentifierCa::deleteIdentifiers(std::vector<RecordGenomicVariant*> & varRecords)
	{
		std::vector<RecordT<uint32_t>*> records;
		records.reserve(varRecords.size());
		for (auto r: varRecords) {
			CaRecord *cr = new CaRecord(r->identifiers.lastId());
			cr->type = caRecordType::genomicVariation;
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

	bool IndexIdentifierCa::isNewDb() const
	{
		return (pim->db.isNewDb());
	}
