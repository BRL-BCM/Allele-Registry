#include "TableGenomic64.hpp"
#include <algorithm>
#include <mutex>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <openssl/md5.h>

#include "../apiDb/db.hpp"


	// ================================================ TABLE

	struct TableGenomic::Pim
	{
		std::string const dirPath;
		Header const header;
		std::mutex m;
		std::atomic<uint64_t> & nextFreeCaId;
		boost::interprocess::message_queue * mq;
		DatabaseT<> db;
		Pim(std::string const & pDirPath, Header const & pHeader, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB, std::atomic<uint64_t> & pNextFreeCaId)
		: dirPath(pDirPath), nextFreeCaId(pNextFreeCaId)
		, header(pHeader)
		, db(cpuTaskManager, ioTaskManager, dirPath + "genomic", header, createRecord<RecordGenomicVariant>, cacheInMB)
		{
			mq = NULL;
			try {
				mq = new boost::interprocess::message_queue
				(
					boost::interprocess::open_only,
					"ca_id_queue"
				);
			} catch (boost::interprocess::interprocess_exception &ex) {
				std::cerr << "MESSAGE QUEUE ERROR:\t" << ex.what() << std::endl;
			}
			srand(time(0));
		}
		uint64_t newCaId() const {
			if(mq == NULL) {
				return -1;
			}
			uint64_t ret;
			char dat[8];
			unsigned int priority;
			boost::interprocess::message_queue::size_type recvd_size;
			try {
				bool rcf_ret = mq->try_receive(&dat[0], 8, recvd_size, priority);
				if(rcf_ret) {
					ret = *(reinterpret_cast<uint64_t*>(&dat[0]));
					return ret;
				} else {
					return -1;
				}
			} catch(boost::interprocess::interprocess_exception &ex) {
				std::cerr << "MESSAGE QUEUE ERROR:\t" << ex.what() << std::endl;
				return -1;
			}
		}
	};

	TableGenomic::TableGenomic(std::string dirPath, Header header, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB, std::atomic<uint64_t> & nextFreeCaId)
	{
		if ( (! dirPath.empty()) && dirPath.back() != '/') dirPath += "/";
		pim = new Pim(dirPath, header, cpuTaskManager, ioTaskManager, cacheInMB, nextFreeCaId);
		std::cout << "table genomic:\trecordsCount=" << pim->db.getRecordsCount() << "\ttheLargestKey=" << pim->db.getTheLargestKey() << "\n";
	}

	TableGenomic::~TableGenomic()
	{
		delete pim;
	}


	void TableGenomic::query( tCallbackWithResults callback, unsigned & recordsToSkip, uint64_t first, uint64_t last, unsigned minChunkSize, unsigned hintQuerySize, TableNamed<RecordGenomicVariant> * named ) const
	{
		std::vector<RecordGenomicVariant*> records2;
		std::vector<RecordGenomicVariant const *> records_filtered;
		std::vector<RecordGenomicVariant const *> records_orig;
		std::vector<uint64_t> ids;
		auto visitor = [callback, first, last, &recordsToSkip, &records2, &records_filtered, &records_orig, &ids, minChunkSize, named](std::vector<RecordT<uint64_t> const *> const & records, bool & lastCall)
		{
			for (auto r: records) {
				RecordGenomicVariant const * vr = dynamic_cast<RecordGenomicVariant const *>(r);
				if (vr->definition.firstPosition() + vr->definition.raw()[0].lengthBefore > first && vr->definition.firstPosition() + vr->definition.raw()[0].lengthBefore <= last) {
					ids.push_back(vr->identifiers.lastId());
					records_orig.push_back(vr);
				}
			}
			if(named) {
				auto filtered = named->fetch(ids);
				for(int i=0; i<filtered.size(); i++) {
					if(!filtered[i]) {
						records_filtered.push_back(records_orig[i]);
					}
				}
			} else {
				records_filtered = records_orig;
			}
			for (auto r: records_filtered) {
				RecordGenomicVariant const * vr = r;
				if (recordsToSkip) {
					--recordsToSkip;
				} else {
					records2.push_back(new RecordGenomicVariant(*vr));
				}
			}
			records_filtered.clear();
			records_orig.clear();
			ids.clear();
			if (records2.size() >= minChunkSize || lastCall) {
				// TODO - problem with order between following vectors
				std::sort( records2.begin(), records2.end(), [](RecordGenomicVariant const *r1,RecordGenomicVariant const *r2){ return (r1->definition < r2->definition); } );
				callback(records2,lastCall);
				for (auto r: records2) delete r;
				records2.clear();
			}
		};

		uint64_t const margin = 10000;
		pim->db.readRecordsInOrder( visitor, (first > margin) ? (first-margin) : (0), last, hintQuerySize );
		pim->db.readRecordsInOrder( visitor, (first > margin) ? ((uint64_t(first-margin)<<24)+(uint64_t(1)<<63)) : (uint64_t(1)<<63), (uint64_t(last+1)<<24)+(uint64_t(1)<<63), hintQuerySize );
	}


	void TableGenomic::fetch(std::vector<RecordGenomicVariant*> const & pRecords) const
	{
		std::vector<RecordT<uint64_t>*> records;
		records.reserve(pRecords.size());
		for (unsigned i=0;i<pRecords.size();i++) {
			if(pRecords[i]->definition.raw().size()>1) {
				std::string def = pRecords[i]->definition.toString();
				unsigned char digest[MD5_DIGEST_LENGTH];
				MD5((unsigned char*)&def.c_str()[0], def.size(), digest);
				pRecords[i]->key = ( ((uint64_t(1)<<63)+(uint64_t(pRecords[i]->key)<<24)+(uint64_t(digest[2])<<16)+(uint64_t(digest[1])<<8)+uint64_t(digest[0])) );
				records.push_back(pRecords[i]);
			} else {
				records.push_back(pRecords[i]);
			}
		}

		auto readFunction = [](std::vector<RecordT<uint64_t> const *> const & currentRecords, std::vector<RecordT<uint64_t>*> const & newRecords)
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
		, std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr>>> & changesInIndexes
		)
	{
		std::vector<RecordT<uint64_t>*> records;
		records.reserve(pRecords.size());
		for (unsigned i=0;i<pRecords.size();i++) {
			if(pRecords[i]->definition.raw().size()>1) {
				std::string def = pRecords[i]->definition.toString();
				unsigned char digest[MD5_DIGEST_LENGTH];
				MD5((unsigned char*)&def.c_str()[0], def.size(), digest);
				pRecords[i]->key = ( ((uint64_t(1)<<63)+(uint64_t(pRecords[i]->key)<<24)+(uint64_t(digest[2])<<16)+(uint64_t(digest[1])<<8)+uint64_t(digest[0])) );
				records.push_back(pRecords[i]);
			} else {
				records.push_back(pRecords[i]);
			}
		}

		std::mutex accessToMapWithChanges;

		auto updateFunction = [&records,&changesInIndexes,&accessToMapWithChanges,this](std::vector<RecordT<uint64_t>*> & currentRecords, std::vector<RecordT<uint64_t>*> const & newRecords)->bool
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
//#ifdef RANDOMIZED_CAID
					if ( r->identifiers.lastId() == CanonicalId::null.value ) {
						pim->m.lock();
						auto newId = pim->newCaId();
						pim->m.unlock();
						//ASSERT(newId != -1);
						if(newId != -1) {
							r->identifiers.lastId() = newId;
						} else {
							throw std::logic_error("id server is down");
						}
					}
//#else
					//pim->m.lock();
					//pim->nextFreeCaId++;
					//uint64_t id = pim->nextFreeCaId;
					//pim->m.unlock();
					//if ( r->identifiers.lastId() == CanonicalId::null.value ) r->identifiers.lastId() = id;
					//if ( r->identifiers.lastId() == CanonicalId::null.value ) r->identifiers.lastId() = (pim->nextFreeCaId)++;
//#endif
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
		//if(rand()%20000==0)exit(0);
	}


	void TableGenomic::fetchAndDelete
		( std::vector<RecordGenomicVariant*> const & pRecords
		, std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr>>> & changesInIndexes
		)
	{
		std::vector<RecordT<uint64_t>*> records;
		records.reserve(pRecords.size());
		for (unsigned i=0;i<pRecords.size();i++) {
			if(pRecords[i]->definition.raw().size()>1) {
				std::string def = pRecords[i]->definition.toString();
				unsigned char digest[MD5_DIGEST_LENGTH];
				MD5((unsigned char*)&def.c_str()[0], def.size(), digest);
				pRecords[i]->key = ( ((uint64_t(1)<<63)+(uint64_t(pRecords[i]->key)<<24)+(uint64_t(digest[2])<<16)+(uint64_t(digest[1])<<8)+uint64_t(digest[0])) );
				records.push_back(pRecords[i]);
			} else {
				records.push_back(pRecords[i]);
			}
		}

		std::mutex accessToMapWithChanges;

		auto updateFunction = [&records,&changesInIndexes,&accessToMapWithChanges,this](std::vector<RecordT<uint64_t>*> & currentRecords, std::vector<RecordT<uint64_t>*> const & newRecords)->bool
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
		, std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr>>> & changesInIndexes
		)
	{
		std::vector<RecordT<uint64_t>*> records;
		records.reserve(pRecords.size());
		for (unsigned i=0;i<pRecords.size();i++) {
			if(pRecords[i]->definition.raw().size()>1) {
				std::string def = pRecords[i]->definition.toString();
				unsigned char digest[MD5_DIGEST_LENGTH];
				MD5((unsigned char*)&def.c_str()[0], def.size(), digest);
				pRecords[i]->key = ( ((uint64_t(1)<<63)+(uint64_t(pRecords[i]->key)<<24)+(uint64_t(digest[2])<<16)+(uint64_t(digest[1])<<8)+uint64_t(digest[0])) );
				records.push_back(pRecords[i]);
			} else {
				records.push_back(pRecords[i]);
			}
		}

		std::mutex accessToMapWithChanges;

		auto updateFunction = [&records,&changesInIndexes,&accessToMapWithChanges,this](std::vector<RecordT<uint64_t>*> & currentRecords, std::vector<RecordT<uint64_t>*> const & newRecords)->bool
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


	void TableGenomic::fetchAndFullDelete2
		( std::vector<RecordGenomicVariant*> const & pRecords
		, std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr>>> & changesInIndexes
		)
	{
		std::vector<RecordT<uint64_t>*> records;
		records.reserve(pRecords.size());
		for (unsigned i=0;i<pRecords.size();i++) {
			if(pRecords[i]->definition.raw().size()>1) {
				std::string def = pRecords[i]->definition.toString();
				unsigned char digest[MD5_DIGEST_LENGTH];
				MD5((unsigned char*)&def.c_str()[0], def.size(), digest);
				pRecords[i]->key = ( ((uint64_t(1)<<63)+(uint64_t(pRecords[i]->key)<<24)+(uint64_t(digest[2])<<16)+(uint64_t(digest[1])<<8)+uint64_t(digest[0])) );
				records.push_back(pRecords[i]);
			} else {
				records.push_back(pRecords[i]);
			}
		}

		std::mutex accessToMapWithChanges;

		auto updateFunction = [&records,&changesInIndexes,&accessToMapWithChanges,this](std::vector<RecordT<uint64_t>*> & currentRecords, std::vector<RecordT<uint64_t>*> const & newRecords)->bool
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
					r->identifiers.clear();
					currentRecords.erase(std::find(currentRecords.begin(),currentRecords.end(),dbRec));
					changes = true;
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
		std::vector<RecordT<uint64_t>*> records;
		records.reserve(pRecords.size());
		for (unsigned i=0;i<pRecords.size();i++) {
			if(pRecords[i]->definition.raw().size()>1) {
				std::string def = pRecords[i]->definition.toString();
				unsigned char digest[MD5_DIGEST_LENGTH];
				MD5((unsigned char*)&def.c_str()[0], def.size(), digest);
				pRecords[i]->key = ( ((uint64_t(1)<<63)+(uint64_t(pRecords[i]->key)<<24)+(uint64_t(digest[2])<<16)+(uint64_t(digest[1])<<8)+uint64_t(digest[0])) );
				records.push_back(pRecords[i]);
			} else {
				records.push_back(pRecords[i]);
			}
		}

		auto updateFunction = [idType](std::vector<RecordT<uint64_t>*> & currentRecords, std::vector<RecordT<uint64_t>*> const & newRecords)->bool
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
