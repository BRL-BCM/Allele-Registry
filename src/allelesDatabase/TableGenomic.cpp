#include "TableGenomic.hpp"
#include <algorithm>
#include <mutex>

#include "../apiDb/db.hpp"


	// ================================================ TABLE

	struct TableGenomic::Pim
	{
		std::string const dirPath;
		std::atomic<uint32_t> & nextFreeCaId;
		DatabaseT<> db;
		Pim(std::string const & pDirPath, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB, std::atomic<uint32_t> & pNextFreeCaId)
		: dirPath(pDirPath), nextFreeCaId(pNextFreeCaId)
		, db(cpuTaskManager, ioTaskManager, dirPath + "genomic", createRecord<RecordGenomicVariant>, cacheInMB)
		{}
	};

	TableGenomic::TableGenomic(std::string dirPath, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB, std::atomic<uint32_t> & nextFreeCaId)
	{
		if ( (! dirPath.empty()) && dirPath.back() != '/') dirPath += "/";
		pim = new Pim(dirPath, cpuTaskManager, ioTaskManager, cacheInMB, nextFreeCaId);
		std::cout << "table genomic:\trecordsCount=" << pim->db.getRecordsCount() << "\ttheLargestKey=" << pim->db.getTheLargestKey() << "\n";
	}

	TableGenomic::~TableGenomic()
	{
		delete pim;
	}


	void TableGenomic::query( tCallbackWithResults callback, unsigned & recordsToSkip, uint32_t first, uint32_t last, unsigned minChunkSize, unsigned hintQuerySize ) const
	{
		std::vector<RecordGenomicVariant*> records2;
		auto visitor = [callback, first, &recordsToSkip, &records2, minChunkSize](std::vector<RecordT<uint32_t> const *> const & records, bool & lastCall)
		{
			for (auto r: records) {
				RecordGenomicVariant const * vr = dynamic_cast<RecordGenomicVariant const *>(r);
				if (vr->definition.firstPosition() + vr->definition.raw()[0].lengthBefore > first) {
					if (recordsToSkip) {
						--recordsToSkip;
					} else {
						records2.push_back(new RecordGenomicVariant(*vr));
					}
				}
			}
			if (records2.size() >= minChunkSize || lastCall) {
				// TODO - problem with order between following vectors
				std::sort( records2.begin(), records2.end(), [](RecordGenomicVariant const *r1,RecordGenomicVariant const *r2){ return (r1->definition < r2->definition); } );
				callback(records2,lastCall);
				for (auto r: records2) delete r;
				records2.clear();
			}
		};

		uint32_t const margin = 10000;
		pim->db.readRecordsInOrder( visitor, (first > margin) ? (first-margin) : (0), last, hintQuerySize );
	}


	void TableGenomic::fetch(std::vector<RecordGenomicVariant*> const & pRecords) const
	{
		std::vector<RecordT<uint32_t>*> records;
		records.insert( records.end(), pRecords.begin(), pRecords.end() );

		auto readFunction = [](std::vector<RecordT<uint32_t> const *> const & currentRecords, std::vector<RecordT<uint32_t>*> const & newRecords)
		{
			// ======================= read data from the db records
			for (auto rBase: newRecords) {
				RecordGenomicVariant * r = dynamic_cast<RecordGenomicVariant*>(rBase);
				RecordGenomicVariant const * dbRec = nullptr;
				for (auto r2Base: currentRecords) {
					RecordGenomicVariant const * r2 = dynamic_cast<RecordGenomicVariant const *>(r2Base);
					if (r->key != r2->key) throw std::logic_error("Key mismatch");
					if (r->definition != r2->definition) continue;
					dbRec = r2;
					break;
				}
				if (dbRec == nullptr) {
					r->identifiers.clear();
				} else if (r->identifiers.lastId() == CanonicalId::null.value || r->identifiers.lastId() == dbRec->identifiers.lastId()) {
					r->identifiers = dbRec->identifiers;
					r->revision    = dbRec->revision;
				} else {
					// CA ID does not match
					r->identifiers.clear();
				}
			}
		};

		pim->db.readRecords(records, readFunction);
	}


	void TableGenomic::fetchAndAdd
		( std::vector<RecordGenomicVariant*> const & pRecords
		, std::map<identifierType,std::vector<std::pair<uint32_t,RecordVariantPtr>>> & changesInIndexes
		)
	{
		std::vector<RecordT<uint32_t>*> records;
		records.insert( records.end(), pRecords.begin(), pRecords.end() );

		std::mutex accessToMapWithChanges;

		auto updateFunction = [&records,&changesInIndexes,&accessToMapWithChanges,this](std::vector<RecordT<uint32_t>*> & currentRecords, std::vector<RecordT<uint32_t>*> const & newRecords)->bool
		{
			// ======================= search for duplicated records in the input, make sure that duplicated records have the same identifiers
			for (unsigned i = 0; i < newRecords.size(); ++i) {
				for (unsigned j = i+1; j < newRecords.size(); ++j) {
					RecordGenomicVariant * r1 = dynamic_cast<RecordGenomicVariant*>(newRecords[i]);
					RecordGenomicVariant * r2 = dynamic_cast<RecordGenomicVariant*>(newRecords[j]);
					if (r1->key != r2->key) throw std::logic_error("Key mismatch");
					if (r1->definition != r2->definition) continue;
					r1->identifiers.exchange(r2->identifiers);
				}
			}

			// ======================= modifications on the db records
			bool changes = false;
			for (auto rBase: newRecords) {
				RecordGenomicVariant * r = dynamic_cast<RecordGenomicVariant*>(rBase);
				RecordGenomicVariant * dbRec = nullptr;
				for (auto r2Base: currentRecords) {
					RecordGenomicVariant * r2 = dynamic_cast<RecordGenomicVariant*>(r2Base);
					if (r->key != r2->key) throw std::logic_error("Key mismatch");
					if (r->definition != r2->definition) continue;
					dbRec = r2;
					break;
				}
				if (dbRec == nullptr) {
					if ( r->identifiers.lastId() == CanonicalId::null.value ) r->identifiers.lastId() = (pim->nextFreeCaId)++;
					dbRec = new RecordGenomicVariant(*r);
					currentRecords.push_back(dbRec);
					changes = true;
					std::lock_guard<std::mutex> synch(accessToMapWithChanges);
					r->identifiers.saveShortIdsToContainer(changesInIndexes, r);
					changesInIndexes[identifierType::CA].push_back(std::make_pair(r->identifiers.lastId(),r));
				} else if ( r->identifiers.lastId() == CanonicalId::null.value || r->identifiers.lastId() == dbRec->identifiers.lastId() ) {
					BinaryIdentifiers const addedIds = dbRec->identifiers.add(r->identifiers);
					if (! addedIds.empty()) {
						changes = true;
						std::lock_guard<std::mutex> synch(accessToMapWithChanges);
						addedIds.saveShortIdsToContainer(changesInIndexes, r);
					}
					r->identifiers = dbRec->identifiers;
				} else {
					// CA ID does not match
					r->identifiers.clear();
				}
			}
			return changes;
		};

		pim->db.writeRecords(records, updateFunction);
	}


	void TableGenomic::fetchAndDelete
		( std::vector<RecordGenomicVariant*> const & pRecords
		, std::map<identifierType,std::vector<std::pair<uint32_t,RecordVariantPtr>>> & changesInIndexes
		)
	{
		std::vector<RecordT<uint32_t>*> records;
		records.insert( records.end(), pRecords.begin(), pRecords.end() );

		std::mutex accessToMapWithChanges;

		auto updateFunction = [&records,&changesInIndexes,&accessToMapWithChanges,this](std::vector<RecordT<uint32_t>*> & currentRecords, std::vector<RecordT<uint32_t>*> const & newRecords)->bool
		{
			// ======================= search for duplicated records in the input, make sure that duplicated records have the same identifiers
			for (unsigned i = 0; i < newRecords.size(); ++i) {
				for (unsigned j = i+1; j < newRecords.size(); ++j) {
					RecordGenomicVariant * r1 = dynamic_cast<RecordGenomicVariant*>(newRecords[i]);
					RecordGenomicVariant * r2 = dynamic_cast<RecordGenomicVariant*>(newRecords[j]);
					if (r1->key != r2->key) throw std::logic_error("Key mismatch");
					if (r1->definition != r2->definition) continue;
					r1->identifiers.exchange(r2->identifiers);
				}
			}

			// ======================= modifications on the db records
			bool changes = false;
			for (auto rBase: newRecords) {
				RecordGenomicVariant * r = dynamic_cast<RecordGenomicVariant*>(rBase);
				RecordGenomicVariant * dbRec = nullptr;
				for (auto r2Base: currentRecords) {
					RecordGenomicVariant * r2 = dynamic_cast<RecordGenomicVariant*>(r2Base);
					if (r->key != r2->key) throw std::logic_error("Key mismatch");
					if (r->definition != r2->definition) continue;
					dbRec = r2;
					break;
				}
				if ( dbRec != nullptr && (r->identifiers.lastId() == CanonicalId::null.value || dbRec->identifiers.lastId() == r->identifiers.lastId()) ) {
					BinaryIdentifiers const deletedIds = dbRec->identifiers.remove(r->identifiers);
					if (! deletedIds.empty()) {
						changes = true;
						std::lock_guard<std::mutex> synch(accessToMapWithChanges);
						deletedIds.saveShortIdsToContainer(changesInIndexes, r);
					}
					r->identifiers = dbRec->identifiers;
				} else {
					// no record with this definition or CA ID mismatch
					r->identifiers.clear();
				}
			}
			return changes;
		};

		pim->db.writeRecords(records, updateFunction);
	}


	void TableGenomic::fetchAndFullDelete
		( std::vector<RecordGenomicVariant*> const & pRecords
		, std::map<identifierType,std::vector<std::pair<uint32_t,RecordVariantPtr>>> & changesInIndexes
		)
	{
		std::vector<RecordT<uint32_t>*> records;
		records.insert( records.end(), pRecords.begin(), pRecords.end() );

		std::mutex accessToMapWithChanges;

		auto updateFunction = [&records,&changesInIndexes,&accessToMapWithChanges,this](std::vector<RecordT<uint32_t>*> & currentRecords, std::vector<RecordT<uint32_t>*> const & newRecords)->bool
		{
			// ======================= modifications on the db records
			bool changes = false;
			for (auto rBase: newRecords) {
				RecordGenomicVariant * r = dynamic_cast<RecordGenomicVariant*>(rBase);
				RecordGenomicVariant * dbRec = nullptr;
				for (auto r2Base: currentRecords) {
					RecordGenomicVariant * r2 = dynamic_cast<RecordGenomicVariant*>(r2Base);
					if (r->key != r2->key) throw std::logic_error("Key mismatch");
					if (r->definition != r2->definition) continue;
					dbRec = r2;
					break;
				}
				if ( dbRec != nullptr && (r->identifiers.lastId() == CanonicalId::null.value || dbRec->identifiers.lastId() == r->identifiers.lastId()) ) {
					BinaryIdentifiers const deletedIds = dbRec->identifiers;
					if (! deletedIds.empty()) {
						changes = true;
						std::lock_guard<std::mutex> synch(accessToMapWithChanges);
						deletedIds.saveShortIdsToContainer(changesInIndexes, r);
					}
					r->identifiers = dbRec->identifiers;
					currentRecords.erase(std::find(currentRecords.begin(),currentRecords.end(),dbRec));
					delete dbRec;
				} else {
					// no record with this definition or CA ID mismatch
					r->identifiers.clear();
				}
			}
			return changes;
		};

		pim->db.writeRecords(records, updateFunction);
	}


	void TableGenomic::deleteIdentifiers(std::vector<RecordGenomicVariant*> const & pRecords, identifierType idType)
	{
		std::vector<RecordT<uint32_t>*> records;
		records.insert( records.end(), pRecords.begin(), pRecords.end() );

		auto updateFunction = [idType](std::vector<RecordT<uint32_t>*> & currentRecords, std::vector<RecordT<uint32_t>*> const & newRecords)->bool
		{
			// ======================= search for duplicated records in the input, make sure that duplicated records have the same identifiers
			for (unsigned i = 0; i < newRecords.size(); ++i) {
				for (unsigned j = i+1; j < newRecords.size(); ++j) {
					RecordGenomicVariant * r1 = dynamic_cast<RecordGenomicVariant*>(newRecords[i]);
					RecordGenomicVariant * r2 = dynamic_cast<RecordGenomicVariant*>(newRecords[j]);
					if (r1->key != r2->key) throw std::logic_error("Key mismatch");
					if (r1->definition != r2->definition) continue;
					r1->identifiers.exchange(r2->identifiers);
				}
			}

			// ======================= modifications on the db records
			bool changes = false;
			for (auto rBase: currentRecords) {
				RecordGenomicVariant * r = dynamic_cast<RecordGenomicVariant*>(rBase);
				BinaryIdentifiers const deletedIds = r->identifiers.remove(idType);
				if (! deletedIds.empty()) changes = true;
			}
			return changes;
		};

		pim->db.writeRecords(records, updateFunction);
	}
