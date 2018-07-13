#include "TableSequence.hpp"
#include "tools.hpp"
#include "../commonTools/bytesLevel.hpp"
#include "../apiDb/db.hpp"
#include <limits>
#include <map>


inline unsigned CRC32(std::string const & s)
{
	boost::crc_32_type result;
	result.process_bytes(s.data(), s.size());
	return result.checksum();
}

	class SequenceRecord : public RecordT<uint32_t>
	{
	public:
		std::string * seq = nullptr;
		unsigned internalId = 0;
		SequenceRecord(uint32_t crc24) : RecordT<uint32_t>(crc24) { }
		virtual unsigned dataLength() const;
		virtual void saveData(uint8_t *& ptr) const;
		virtual void loadData(uint8_t const *& ptr);
		virtual ~SequenceRecord() { delete seq; }
	};

	unsigned SequenceRecord::dataLength() const
	{
		unsigned length = 0;
		// internal id
		++length;
		// length of the sequence
		length += (seq != nullptr) ? (lengthUnsignedIntVarSize(seq->size())) : (1);
		// sequence
		length += (seq != nullptr) ? ((seq->size() + 3) / 4) : (0);
		return length;
	}

	void SequenceRecord::saveData(uint8_t *& ptr) const
	{
		writeUnsignedInteger<1>(ptr, internalId);
		if (seq == nullptr) {
			writeUnsignedIntVarSize<1,1,unsigned>(ptr, 0u);
			return;
		}
		writeUnsignedIntVarSize<1,1,unsigned>(ptr, seq->size());
		unsigned const countOfFullSegments = seq->size() / 16;
		for (unsigned i = 0; i < countOfFullSegments; ++i) {
			writeUnsignedInteger<4>(ptr, convertGenomicToBinary(seq->substr(i*16,16)));
		}
		std::string const rest = seq->substr(countOfFullSegments*16);
		writeUnsignedInteger( ptr, convertGenomicToBinary(rest), (rest.size()+3)/4 );
	}

	void SequenceRecord::loadData(uint8_t const *& ptr)
	{
		if (seq == nullptr) seq = new std::string();
		internalId = readUnsignedInteger<1,unsigned>(ptr);
		seq->resize(readUnsignedIntVarSize<1,1,unsigned>(ptr), '?');
		unsigned const countOfFullSegments = seq->size() / 16;
		for (unsigned i = 0; i < countOfFullSegments; ++i) {
			std::string const ss = convertBinaryToGenomic(readUnsignedInteger<4,uint32_t>(ptr), 16);
			seq->replace( i*16, 16, ss );
		}
		unsigned const rest = seq->size() - countOfFullSegments*16;
		std::string const ss = convertBinaryToGenomic( readUnsignedInteger<uint32_t>(ptr,(rest+3)/4), rest );
		seq->replace( countOfFullSegments*16, rest, ss );
	}


	struct TableSequence::Pim
	{
		std::string const dirPath;
		DatabaseT<> db;
		Pim(std::string const & pDirPath, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB)
		: dirPath(pDirPath)
		, db(cpuTaskManager, ioTaskManager, dirPath + "sequence", createRecord<SequenceRecord>, cacheInMB)
		{}
	};

	uint32_t const TableSequence::unknownSequence = std::numeric_limits<uint32_t>::max();

	TableSequence::TableSequence(std::string dirPath, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB)
	{
		if ( (! dirPath.empty()) && dirPath.back() != '/') dirPath += "/";
		pim = new Pim(dirPath, cpuTaskManager, ioTaskManager, cacheInMB);
	}

	TableSequence::~TableSequence()
	{
		delete pim;
	}

	void TableSequence::fetch(std::vector<uint32_t> const & seq, std::vector<std::string*> const & out) const
	{
		if (seq.size() != out.size()) throw std::logic_error("TableSequence: parameter mismatch");
		std::vector<RecordT<uint32_t>*> records( seq.size(), nullptr );
		for (unsigned i = 0; i < seq.size(); ++i) {
			uint32_t const id = seq[i];
			SequenceRecord * r = new SequenceRecord(id >> 8);
			r->internalId = id % 256;
			r->seq = out[i];
			records[i] = r;
		}

		auto visitor = [](std::vector<RecordT<uint32_t> const *> const & dbRecords, std::vector<RecordT<uint32_t> *> const & records)
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			if (dbRecords.empty()) return;
			std::map<unsigned,std::string const *> internalId2seq;
			for (auto r: dbRecords) {
				SequenceRecord const * dbRecord = dynamic_cast<SequenceRecord const *>( r );
				if (dbRecord->seq != nullptr) internalId2seq[dbRecord->internalId] = dbRecord->seq;
			}
			for (auto r: records) {
				SequenceRecord * rec = dynamic_cast<SequenceRecord*>(r);
				if (internalId2seq.count(rec->internalId)) {
					*(rec->seq) = *(internalId2seq[rec->internalId]);
				}
			}
		};

		pim->db.readRecords(records, visitor);

		for (auto rec: records) {
			SequenceRecord * r = dynamic_cast<SequenceRecord*>(rec);
			r->seq = nullptr;
			delete r;
		}
	}

	void TableSequence::fetch(std::vector<std::string const *> const & seq, std::vector<uint32_t> & out) const
	{
		out.clear();
		out.resize(seq.size(), unknownSequence);
		std::vector<RecordT<uint32_t>*> records( seq.size(), nullptr );
		for (unsigned i = 0; i < seq.size(); ++i) {
			uint32_t const crc = CRC32( *(seq[i]) );
			SequenceRecord * r = new SequenceRecord(crc >> 8);
			r->seq = new std::string(*(seq[i]));
			r->internalId = 123456;
			records[i] = r;
		}

		auto visitor = [](std::vector<RecordT<uint32_t> const *> const & dbRecords, std::vector<RecordT<uint32_t> *> const & records)
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			if (dbRecords.empty()) return;
			for (auto r: records) {
				SequenceRecord * rec = dynamic_cast<SequenceRecord*>(r);
				for (auto r2: dbRecords) {
					SequenceRecord const * dbRecord = dynamic_cast<SequenceRecord const *>( r2 );
					if ( *(rec->seq) == *(dbRecord->seq) ) {
						rec->internalId = dbRecord->internalId;
						break;
					}
				}
			}
		};

		pim->db.readRecords(records, visitor);

		for (unsigned i = 0; i < seq.size(); ++i) {
			SequenceRecord * r = dynamic_cast<SequenceRecord*>(records[i]);
			if (r->internalId < 256) {
				out[i] = (r->key << 8) + r->internalId;
			}
			delete r;
		}
	}

	void TableSequence::fetchAndAdd(std::vector<std::string const *> const & seq, std::vector<uint32_t> & out) const
	{
		out.clear();
		out.resize(seq.size(), unknownSequence);
		std::vector<RecordT<uint32_t>*> records( seq.size(), nullptr );
		for (unsigned i = 0; i < seq.size(); ++i) {
			uint32_t const crc = CRC32( *(seq[i]) );
			SequenceRecord * r = new SequenceRecord(crc >> 8);
			r->seq = new std::string(*(seq[i]));
			r->internalId = 123456;
			records[i] = r;
		}

		auto visitor = [](std::vector<RecordT<uint32_t>*> & dbRecords, std::vector<RecordT<uint32_t> *> const & records) -> bool
		{
			if (records.empty()) throw std::logic_error("No records in visitor's callback!");
			bool changes = false;
			for (auto r: records) {
				SequenceRecord * rec = dynamic_cast<SequenceRecord*>(r);
				unsigned nextInternalId = 0;
				for (auto r2: dbRecords) {
					SequenceRecord const * dbRecord = dynamic_cast<SequenceRecord const *>( r2 );
					if ( *(rec->seq) == *(dbRecord->seq) ) {
						rec->internalId = dbRecord->internalId;
						break;
					}
					if (nextInternalId <= dbRecord->internalId) nextInternalId = dbRecord->internalId + 1;
				}
				if (rec->internalId > 255) {
					SequenceRecord * newRecord = new SequenceRecord(rec->key);
					newRecord->internalId = nextInternalId;
					newRecord->seq = new std::string(*(rec->seq));
					dbRecords.push_back(newRecord);
					rec->internalId = newRecord->internalId;
					changes = true;
				}
			}
			return changes;
		};

		pim->db.writeRecords(records, visitor);

		for (unsigned i = 0; i < seq.size(); ++i) {
			SequenceRecord * r = dynamic_cast<SequenceRecord*>(records[i]);
			if (r->internalId < 256) {
				out[i] = (r->key << 8) + r->internalId;
			}
			delete r;
		}
	}
