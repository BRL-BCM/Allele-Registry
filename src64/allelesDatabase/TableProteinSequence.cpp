#include "TableProteinSequence.hpp"
#include "tools.hpp"
#include "../commonTools/bytesLevel.hpp"
#include "../apiDb/db.hpp"
#include <limits>
#include <map>
#include <iostream>

inline uint32_t CRC32(std::string const & s)
{
	boost::crc_32_type result;
	result.process_bytes(s.data(), s.size());
	return result.checksum();
}

	class SequenceProteinRecord : public RecordT<uint64_t>
	{
	public:
		std::string * seq = nullptr;
		unsigned internalId = 0;
		SequenceProteinRecord(uint32_t crc24) : RecordT<uint64_t>(crc24) { }
		virtual unsigned dataLength() const;
		virtual void saveData(uint8_t *& ptr) const;
		virtual void loadData(uint8_t const *& ptr);
		virtual ~SequenceProteinRecord() { delete seq; }
	};

	unsigned SequenceProteinRecord::dataLength() const
	{
		unsigned length = 0;
		// internal id
		++length;
		// length of the sequence
		length += (seq != nullptr) ? (lengthUnsignedIntVarSize(seq->size())) : (1);
		// sequence
		length += (seq != nullptr) ? (4*(((seq->size()+6) / 7))) : (0);
		return length;
	}

	void SequenceProteinRecord::saveData(uint8_t *& ptr) const
	{
		writeUnsignedInteger<1>(ptr, internalId);
		if (seq == nullptr) {
			writeUnsignedIntVarSize<1,1,unsigned>(ptr, 0u);
			return;
		}
		writeUnsignedIntVarSize<1,1,unsigned>(ptr, seq->size());
		unsigned const countOfFullSegments = seq->size() / 7;
		for (unsigned i = 0; i < countOfFullSegments; ++i) {
			writeUnsignedInteger<4>(ptr, convertProteinToBinary(seq->substr(i*7,7)));
		}
		std::string const rest = seq->substr(countOfFullSegments*7);
		if(rest.size() > 0) {
			writeUnsignedInteger<4>( ptr, convertProteinToBinary(rest));
		}
	}

	void SequenceProteinRecord::loadData(uint8_t const *& ptr)
	{
		if (seq == nullptr) seq = new std::string();
		internalId = readUnsignedInteger<1,unsigned>(ptr);
		seq->resize(readUnsignedIntVarSize<1,1,unsigned>(ptr), '?');
		unsigned const countOfFullSegments = seq->size() / 7;
		for (unsigned i = 0; i < countOfFullSegments; ++i) {
			std::string const ss = convertBinaryToProtein(readUnsignedInteger<4,uint64_t>(ptr), 7);
			seq->replace( i*7, 7, ss );
		}
		unsigned const rest = seq->size() - countOfFullSegments*7;
		if(rest > 0) {
			std::string const ss = convertBinaryToProtein(readUnsignedInteger<4,uint64_t>(ptr), rest);
			seq->replace( countOfFullSegments*7, rest, ss );
		}
	}


	struct TableProteinSequence::Pim
	{
		std::string const dirPath;
		Header const header;
		DatabaseT<uint64_t,8> db;
		Pim(std::string const & pDirPath, Header const & pHeader, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB)
		: dirPath(pDirPath)
		, header(pHeader)
		, db(cpuTaskManager, ioTaskManager, dirPath + "sequence-protein", header, createRecord<SequenceProteinRecord>, cacheInMB)
		{}
	};

	uint32_t const TableProteinSequence::unknownSequence = std::numeric_limits<uint32_t>::max();

	TableProteinSequence::TableProteinSequence(std::string dirPath, Header header, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB)
	{
		if ( (! dirPath.empty()) && dirPath.back() != '/') dirPath += "/";
		pim = new Pim(dirPath, header, cpuTaskManager, ioTaskManager, cacheInMB);
	}

	TableProteinSequence::~TableProteinSequence()
	{
		delete pim;
	}

	void TableProteinSequence::fetch(std::vector<uint64_t> const & seq, std::vector<std::string*> const & out) const
	{
		if (seq.size() != out.size()) throw std::logic_error("TableProteinSequence: parameter mismatch");
		std::vector<RecordT<uint64_t>*> records( seq.size(), nullptr );
		for (unsigned i = 0; i < seq.size(); ++i) {
			uint64_t const id = seq[i];
			SequenceProteinRecord * r = new SequenceProteinRecord(id >> 8);
			r->internalId = id % 256;
			r->seq = out[i];
			records[i] = r;
		}

		auto visitor = [](std::vector<RecordT<uint64_t> const *> const & dbRecords, std::vector<RecordT<uint64_t> *> const & records)
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			if (dbRecords.empty()) return;
			std::map<unsigned,std::string const *> internalId2seq;
			for (auto r: dbRecords) {
				SequenceProteinRecord const * dbRecord = dynamic_cast<SequenceProteinRecord const *>( r );
				if (dbRecord->seq != nullptr) internalId2seq[dbRecord->internalId] = dbRecord->seq;
			}
			for (auto r: records) {
				SequenceProteinRecord * rec = dynamic_cast<SequenceProteinRecord*>(r);
				if (internalId2seq.count(rec->internalId)) {
					*(rec->seq) = *(internalId2seq[rec->internalId]);
				}
			}
		};

		pim->db.readRecords(records, visitor);

		for (auto rec: records) {
			SequenceProteinRecord * r = dynamic_cast<SequenceProteinRecord*>(rec);
			r->seq = nullptr;
			delete r;
		}
	}

	void TableProteinSequence::fetch(std::vector<std::string const *> const & seq, std::vector<uint64_t> & out) const
	{
		out.clear();
		out.resize(seq.size(), unknownSequence);
		std::vector<RecordT<uint64_t>*> records( seq.size(), nullptr );
		for (unsigned i = 0; i < seq.size(); ++i) {
			uint32_t const crc = CRC32( *(seq[i]) );
			SequenceProteinRecord * r = new SequenceProteinRecord(crc >> 8);
			r->seq = new std::string(*(seq[i]));
			r->internalId = 123456;
			records[i] = r;
		}

		auto visitor = [](std::vector<RecordT<uint64_t> const *> const & dbRecords, std::vector<RecordT<uint64_t> *> const & records)
		{
			if (records.empty()) throw std::logic_error("No records in visitor'c callback!");
			if (dbRecords.empty()) return;
			for (auto r: records) {
				SequenceProteinRecord * rec = dynamic_cast<SequenceProteinRecord*>(r);
				for (auto r2: dbRecords) {
					SequenceProteinRecord const * dbRecord = dynamic_cast<SequenceProteinRecord const *>( r2 );
					if ( *(rec->seq) == *(dbRecord->seq) ) {
						rec->internalId = dbRecord->internalId;
						break;
					}
				}
			}
		};

		pim->db.readRecords(records, visitor);

		for (unsigned i = 0; i < seq.size(); ++i) {
			SequenceProteinRecord * r = dynamic_cast<SequenceProteinRecord*>(records[i]);
			if (r->internalId < 256) {
				out[i] = (r->key << 8) + r->internalId;
			}
			delete r;
		}
	}

	void TableProteinSequence::fetchAndAdd(std::vector<std::string const *> const & seq, std::vector<uint64_t> & out) const
	{
		out.clear();
		out.resize(seq.size(), unknownSequence);
		std::vector<RecordT<uint64_t>*> records( seq.size(), nullptr );
		for (unsigned i = 0; i < seq.size(); ++i) {
			uint32_t const crc = CRC32( *(seq[i]) );
			SequenceProteinRecord * r = new SequenceProteinRecord(crc >> 8);
			r->seq = new std::string(*(seq[i]));
			r->internalId = 123456;
			records[i] = r;
		}

		auto visitor = [](std::vector<RecordT<uint64_t>*> & dbRecords, std::vector<RecordT<uint64_t> *> const & records) -> bool
		{
			if (records.empty()) throw std::logic_error("No records in visitor's callback!");
			bool changes = false;
			for (auto r: records) {
				SequenceProteinRecord * rec = dynamic_cast<SequenceProteinRecord*>(r);
				unsigned nextInternalId = 0;
				for (auto r2: dbRecords) {
					SequenceProteinRecord const * dbRecord = dynamic_cast<SequenceProteinRecord const *>( r2 );
					if ( *(rec->seq) == *(dbRecord->seq) ) {
						rec->internalId = dbRecord->internalId;
						break;
					}
					if (nextInternalId <= dbRecord->internalId) nextInternalId = dbRecord->internalId + 1;
				}
				if (rec->internalId > 255) {
					SequenceProteinRecord * newRecord = new SequenceProteinRecord(rec->key);
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
			SequenceProteinRecord * r = dynamic_cast<SequenceProteinRecord*>(records[i]);
			if (r->internalId < 256) {
				out[i] = (r->key << 8) + r->internalId;
			}
			delete r;
		}
	}
