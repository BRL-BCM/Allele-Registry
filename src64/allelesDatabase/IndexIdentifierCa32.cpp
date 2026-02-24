#include "IndexIdentifierCa32.hpp"
#include "../apiDb/db.hpp"
#include "../commonTools/bytesLevel.hpp"
#include <boost/lexical_cast.hpp>

	enum caRecordType : uint8_t
	{
		genomicVariation = 1
	};

	class CaRecord32 : public RecordT<uint32_t>
	{
	public:
		caRecordType type = caRecordType::genomicVariation;
		BinaryGenomicVariantDefinition definition; // must be sorted by position
		CaRecord32(uint32_t caId) : RecordT<uint32_t>(caId), definition(0) { }
		virtual unsigned dataLength() const;
		virtual void saveData(uint8_t *& ptr) const;
		virtual void loadData(uint8_t const *& ptr);
		virtual ~CaRecord32() {}
	};

	unsigned CaRecord32::dataLength() const
	{
		return (1 + 4 + definition.dataLength());
	}

	void CaRecord32::saveData(uint8_t *& ptr) const
	{
//		std::cout << "BEGIN-CA" << std::endl;
		writeUnsignedInteger<1>(ptr, static_cast<unsigned>(type));
		writeUnsignedInteger<4>(ptr, definition.firstPosition());
		definition.saveData(ptr);
//		std::cout << "END-CA" << std::endl;
	}

	void CaRecord32::loadData(uint8_t const *& ptr)
	{
		type = static_cast<caRecordType>(readUnsignedInteger<1,unsigned>(ptr));
		definition = BinaryGenomicVariantDefinition( readUnsignedInteger<4,uint32_t>(ptr) );
		definition.loadData(ptr);
	}


	struct IndexIdentifierCa32::Pim
	{
		std::string const dirPath;
		Header const header;
		DatabaseT<uint32_t, 4> db;
		Pim(std::string const & pDirPath, Header const & pHeader, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB)
		: dirPath(pDirPath)
		, header(pHeader)
		, db(cpuTaskManager, ioTaskManager, dirPath + "idCa", header, createRecord<CaRecord32>, cacheInMB)
		{}
	};

	IndexIdentifierCa32::IndexIdentifierCa32(std::string dirPath, Header header, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB) : pim(nullptr)
	{
		if ( (! dirPath.empty()) && dirPath.back() != '/') dirPath += "/";
		pim = new Pim(dirPath, header, cpuTaskManager, ioTaskManager, cacheInMB);
		std::cout << "index CA:\trecordsCount=" << pim->db.getRecordsCount() << "\ttheLargestKey=" << pim->db.getTheLargestKey() << "\n";
	}

	IndexIdentifierCa32::~IndexIdentifierCa32()
	{
		delete pim;
	}

	std::vector<RecordGenomicVariant*> IndexIdentifierCa32::fetchDefinitions( std::vector<uint32_t> const & caIds) const
	{
		std::vector<RecordT<uint32_t>*> records;
		records.reserve(caIds.size());
		for (auto id: caIds) {
			records.push_back( new CaRecord32(id) );
		}

		auto visitor = [](std::vector<RecordT<uint32_t> const *> const & dbRecords, std::vector<RecordT<uint32_t> *> const & records)
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			if (dbRecords.empty()) return;
			if (dbRecords.size() > 1) throw std::logic_error("More than one record with the same CA ID: " + boost::lexical_cast<std::string>(records.front()->key));
			CaRecord32 const * dbRecord = dynamic_cast<CaRecord32 const *>( dbRecords.front() );
			for (auto r: records) {
				CaRecord32 * rec = dynamic_cast<CaRecord32*>(r);
				rec->type = dbRecord->type;
				rec->definition = dbRecord->definition;
			}
		};

		pim->db.readRecords(records, visitor);

		std::vector<RecordGenomicVariant*> outRecords(records.size(), nullptr);
		for (unsigned i = 0; i < records.size(); ++i) {
			CaRecord32 * rec = dynamic_cast<CaRecord32*>(records[i]);
			if ( rec->definition.raw().size() > 1 || rec->definition.firstPosition() > 0 ) {  // TODO - null or something
				outRecords[i] = new RecordGenomicVariant(rec->definition);
				outRecords[i]->identifiers.lastId() = rec->key;
			}
			delete rec;
		}

		return outRecords;
	}

	void IndexIdentifierCa32::addIdentifiers(std::vector<RecordGenomicVariant const *> const & varRecords)
	{
		std::vector<RecordT<uint32_t>*> records;
		records.reserve(varRecords.size());
		for (auto r: varRecords) {
			CaRecord32 *cr = new CaRecord32(r->identifiers.lastId());
			cr->type = caRecordType::genomicVariation;
			cr->definition = r->definition;
			records.push_back( cr );
		}

		auto visitor = [](std::vector<RecordT<uint32_t>*> & dbRecords, std::vector<RecordT<uint32_t> *> const & records) -> bool
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			//if (! dbRecords.empty()) throw std::logic_error("CA ID already exists: " + boost::lexical_cast<std::string>(records.front()->key));
			//if (records.size() > 1) throw std::logic_error("More than one record with the same CA ID: " + boost::lexical_cast<std::string>(records.front()->key));
			if (! dbRecords.empty()) { //throw std::logic_error("CA ID already exists: " + boost::lexical_cast<std::string>(records.front()->key));
				if ( dynamic_cast<CaRecord32*>(dbRecords[0])->definition != dynamic_cast<CaRecord32*>(records[0])->definition ) {
					std::cout << "Conflict (1) CA" << records.front()->key << " with: " << dynamic_cast<CaRecord32*>(dbRecords.front())->definition.toString() << std::endl;
				}
				return false;
			}
			if (records.size() > 1) { //throw std::logic_error("More than one record with the same CA ID: " + boost::lexical_cast<std::string>(records.front()->key));
				for (unsigned i = 1; i < records.size(); ++i) {
					if (dynamic_cast<CaRecord32*>(records[i])->definition != dynamic_cast<CaRecord32*>(records[0])->definition) {
						std::cout << "Conflict (2) CA" << records[0]->key << " with: " << dynamic_cast<CaRecord32*>(records[i])->definition.toString() << std::endl;
					}
				}
			}
			dbRecords.push_back( records[0] );
			return true;
		};

		pim->db.writeRecords(records, visitor);
	}

	uint32_t IndexIdentifierCa32::getMaxIdentifier() const
	{
		return (uint32_t)pim->db.getTheLargestKey();
	}

	void IndexIdentifierCa32::deleteIdentifiers(std::vector<RecordGenomicVariant*> & varRecords)
	{
		std::vector<RecordT<uint32_t>*> records;
		records.reserve(varRecords.size());
		for (auto r: varRecords) {
			CaRecord32 *cr = new CaRecord32(r->identifiers.lastId());
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

	bool IndexIdentifierCa32::isNewDb() const
	{
		return (pim->db.isNewDb());
	}
