#include "TableProtein32.hpp"
#include <algorithm>
#include <mutex>

#include "../apiDb/db.hpp"


	// ================================================ TABLE

	struct TableProtein32::Pim
	{
		std::string const dirPath;
		Header const header;
		std::atomic<uint64_t> & nextFreeCaId;
		DatabaseT<uint64_t,8> db;
		Pim(std::string const & pDirPath, Header const & pHeader, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB, std::atomic<uint64_t> & pNextFreeCaId)
		: dirPath(pDirPath), nextFreeCaId(pNextFreeCaId)
		, header(pHeader)
		, db(cpuTaskManager, ioTaskManager, dirPath + "protein", header, createRecord<RecordProteinVariant32>, cacheInMB)
		{}
	};

	TableProtein32::TableProtein32(std::string dirPath, Header header, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB, std::atomic<uint64_t> & nextFreeCaId)
	{
		if ( (! dirPath.empty()) && dirPath.back() != '/') dirPath += "/";
		pim = new Pim(dirPath, header, cpuTaskManager, ioTaskManager, cacheInMB, nextFreeCaId);
		std::cout << "table protein:\trecordsCount=" << pim->db.getRecordsCount() << "\ttheLargestKey=" << pim->db.getTheLargestKey() << "\n";
	}

	TableProtein32::~TableProtein32()
	{
		delete pim;
	}


	void TableProtein32::query( tCallbackWithResults callback, unsigned & recordsToSkip, uint64_t first, uint64_t last, unsigned minChunkSize, unsigned hintQuerySize ) const
	{
		std::vector<RecordProteinVariant32*> records2;
		auto visitor = [callback, first, &recordsToSkip, &records2, minChunkSize](std::vector<RecordT<uint64_t> const *> const & records, bool & lastCall)
		{
			for (auto r: records) {
				RecordProteinVariant32 const * vr = dynamic_cast<RecordProteinVariant32 const *>(r);
				if (vr->definition.proteinAccIdAndFirstPosition() + vr->definition.raw()[0].lengthBefore > first) {
					if (recordsToSkip) {
						--recordsToSkip;
					} else {
						records2.push_back(new RecordProteinVariant32(*vr));
					}
				}
			}
			if (records2.size() >= minChunkSize || lastCall) {
				// TODO - problem with order between following vectors
				std::sort( records2.begin(), records2.end(), [](RecordProteinVariant32 const *r1,RecordProteinVariant32 const *r2){ return (r1->definition < r2->definition); } );
				callback(records2,lastCall);
				for (auto r: records2) delete r;
				records2.clear();
			}
		};

		uint64_t const margin = 10000;
		pim->db.readRecordsInOrder( visitor, (first > margin) ? (first-margin) : (0), last, hintQuerySize );
	}


	void TableProtein32::fetch(std::vector<RecordProteinVariant32*> const & pRecords) const
	{
		std::vector<RecordT<uint64_t>*> records;
		records.insert( records.end(), pRecords.begin(), pRecords.end() );

		auto readFunction = [](std::vector<RecordT<uint64_t> const *> const & currentRecords, std::vector<RecordT<uint64_t>*> const & newRecords)
		{
			// ======================= read data from the db records
			for (auto rBase: newRecords) {
				RecordProteinVariant32 * r = dynamic_cast<RecordProteinVariant32*>(rBase);
				RecordProteinVariant32 const * dbRec = nullptr;
				for (auto r2Base: currentRecords) {
					RecordProteinVariant32 const * r2 = dynamic_cast<RecordProteinVariant32 const *>(r2Base);
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


	void TableProtein32::fetchAndAdd
		( std::vector<RecordProteinVariant32*> const & pRecords
		, std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr32>>> & changesInIndexes
		)
	{
		std::vector<RecordT<uint64_t>*> records;
		records.insert( records.end(), pRecords.begin(), pRecords.end() );

		std::mutex accessToMapWithChanges;

		auto updateFunction = [&records,&changesInIndexes,&accessToMapWithChanges,this](std::vector<RecordT<uint64_t>*> & currentRecords, std::vector<RecordT<uint64_t>*> const & newRecords)->bool
		{
//if (newRecords.front()->key == 65534363175ull)
//{
//	std::cout << newRecords.front()->key << "\t";
//	for (auto r: newRecords) std::cout<< dynamic_cast<RecordProteinVariant32*>(r)->definition.toString() << ",";
//	std::cout << "\t";
//	for (auto r: currentRecords) std::cout<< dynamic_cast<RecordProteinVariant32*>(r)->definition.toString() << ",";
//	std::cout << std::endl;
//}
			// ======================= search for duplicated records in the input, make sure that duplicated records have the same identifiers
			for (unsigned i = 0; i < newRecords.size(); ++i) {
				for (unsigned j = i+1; j < newRecords.size(); ++j) {
					RecordProteinVariant32 * r1 = dynamic_cast<RecordProteinVariant32*>(newRecords[i]);
					RecordProteinVariant32 * r2 = dynamic_cast<RecordProteinVariant32*>(newRecords[j]);
					if (r1->key != r2->key) throw std::logic_error("Key mismatch");
					if (r1->definition != r2->definition) continue;
					r1->identifiers.exchange(r2->identifiers);
					// ---- TODO - temp workaround for conflicting PA
					if ( r1->identifiers.lastId() != r2->identifiers.lastId() ) {
						r1->identifiers.lastId() = r2->identifiers.lastId() = std::min( r1->identifiers.lastId(), r2->identifiers.lastId() );
					}
				}
			}

			// ======================= modifications on the db records
			bool changes = false;
			for (auto rBase: newRecords) {
				RecordProteinVariant32 * r = dynamic_cast<RecordProteinVariant32*>(rBase);
				RecordProteinVariant32 * dbRec = nullptr;
				for (auto r2Base: currentRecords) {
					RecordProteinVariant32 * r2 = dynamic_cast<RecordProteinVariant32*>(r2Base);
					if (r->key != r2->key) throw std::logic_error("Key mismatch");
					if (r->definition != r2->definition) continue;
					dbRec = r2;
					break;
				}
				if (dbRec == nullptr) {
					if ( r->identifiers.lastId() == CanonicalId::null.value ) r->identifiers.lastId() = (pim->nextFreeCaId)++;
					dbRec = new RecordProteinVariant32(*r);
					currentRecords.push_back(dbRec);
					changes = true;
					std::lock_guard<std::mutex> synch(accessToMapWithChanges);
					r->identifiers.saveShortIdsToContainer(changesInIndexes, r);
					changesInIndexes[identifierType::PA].push_back(std::make_pair(r->identifiers.lastId(),r));
				} else if ( r->identifiers.lastId() == CanonicalId::null.value || r->identifiers.lastId() == dbRec->identifiers.lastId() ) {
					BinaryIdentifiers32 const addedIds = dbRec->identifiers.add(r->identifiers);
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


	void TableProtein32::fetchAndDelete
		( std::vector<RecordProteinVariant32*> const & pRecords
		, std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr32>>> & changesInIndexes
		)
	{
		std::vector<RecordT<uint64_t>*> records;
		records.insert( records.end(), pRecords.begin(), pRecords.end() );

		std::mutex accessToMapWithChanges;

		auto updateFunction = [&records,&changesInIndexes,&accessToMapWithChanges,this](std::vector<RecordT<uint64_t>*> & currentRecords, std::vector<RecordT<uint64_t>*> const & newRecords)->bool
		{
			// ======================= search for duplicated records in the input, make sure that duplicated records have the same identifiers
			for (unsigned i = 0; i < newRecords.size(); ++i) {
				for (unsigned j = i+1; j < newRecords.size(); ++j) {
					RecordProteinVariant32 * r1 = dynamic_cast<RecordProteinVariant32*>(newRecords[i]);
					RecordProteinVariant32 * r2 = dynamic_cast<RecordProteinVariant32*>(newRecords[j]);
					if (r1->key != r2->key) throw std::logic_error("Key mismatch");
					if (r1->definition != r2->definition) continue;
					r1->identifiers.exchange(r2->identifiers);
				}
			}

			// ======================= modifications on the db records
			bool changes = false;
			for (auto rBase: newRecords) {
				RecordProteinVariant32 * r = dynamic_cast<RecordProteinVariant32*>(rBase);
				RecordProteinVariant32 * dbRec = nullptr;
				for (auto r2Base: currentRecords) {
					RecordProteinVariant32 * r2 = dynamic_cast<RecordProteinVariant32*>(r2Base);
					if (r->key != r2->key) throw std::logic_error("Key mismatch");
					if (r->definition != r2->definition) continue;
					dbRec = r2;
					break;
				}
				if ( dbRec != nullptr && (r->identifiers.lastId() == CanonicalId::null.value || dbRec->identifiers.lastId() == r->identifiers.lastId()) ) {
					BinaryIdentifiers32 const deletedIds = dbRec->identifiers.remove(r->identifiers);
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

	void TableProtein32::fetchAndFullDelete
		( std::vector<RecordProteinVariant32*> const & pRecords
		, std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr32>>> & changesInIndexes
		)
	{
		std::vector<RecordT<uint64_t>*> records;
		records.insert( records.end(), pRecords.begin(), pRecords.end() );

		std::mutex accessToMapWithChanges;

		auto updateFunction = [&records,&changesInIndexes,&accessToMapWithChanges,this](std::vector<RecordT<uint64_t>*> & currentRecords, std::vector<RecordT<uint64_t>*> const & newRecords)->bool
		{
			// ======================= modifications on the db records
			bool changes = false;
			for (auto rBase: newRecords) {
				RecordProteinVariant32 * r = dynamic_cast<RecordProteinVariant32*>(rBase);
				RecordProteinVariant32 * dbRec = nullptr;
				for (auto r2Base: currentRecords) {
					RecordProteinVariant32 * r2 = dynamic_cast<RecordProteinVariant32*>(r2Base);
					if (r->key != r2->key) throw std::logic_error("Key mismatch");
					if (r->definition != r2->definition) continue;
					dbRec = r2;
					break;
				}
				if ( dbRec != nullptr && (r->identifiers.lastId() == CanonicalId::null.value || dbRec->identifiers.lastId() == r->identifiers.lastId()) ) {
					BinaryIdentifiers32 const deletedIds = dbRec->identifiers;
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


	void TableProtein32::deleteIdentifiers(std::vector<RecordProteinVariant32*> const & pRecords, identifierType idType)
	{
		std::vector<RecordT<uint64_t>*> records;
		records.insert( records.end(), pRecords.begin(), pRecords.end() );

		auto updateFunction = [idType](std::vector<RecordT<uint64_t>*> & currentRecords, std::vector<RecordT<uint64_t>*> const & newRecords)->bool
		{
			// ======================= search for duplicated records in the input, make sure that duplicated records have the same identifiers
			for (unsigned i = 0; i < newRecords.size(); ++i) {
				for (unsigned j = i+1; j < newRecords.size(); ++j) {
					RecordProteinVariant32 * r1 = dynamic_cast<RecordProteinVariant32*>(newRecords[i]);
					RecordProteinVariant32 * r2 = dynamic_cast<RecordProteinVariant32*>(newRecords[j]);
					if (r1->key != r2->key) throw std::logic_error("Key mismatch");
					if (r1->definition != r2->definition) continue;
					r1->identifiers.exchange(r2->identifiers);
				}
			}

			// ======================= modifications on the db records
			bool changes = false;
			for (auto rBase: currentRecords) {
				RecordProteinVariant32 * r = dynamic_cast<RecordProteinVariant32*>(rBase);
				BinaryIdentifiers32 const deletedIds = r->identifiers.remove(idType);
				if (! deletedIds.empty()) changes = true;
			}
			return changes;
		};

		pim->db.writeRecords(records, updateFunction);
	}


