#include "Procedure.hpp"
#include "SubProcedure.hpp"
#include "Scheduler.hpp"
#include "DataNode.hpp"
#include "../commonTools/assert.hpp"
#include "../commonTools/bytesLevel.hpp"
#include <algorithm>

namespace flatDb {


	template<typename tKey>
	void ProcedureT<tKey>::announceAsCompleted()
	{
		fAccessToCompleted.lock();
		fCompleted = true;
		fAccessToCompleted.unlock();
		fWaitingForCompleted.notify_all();
	}

	template<typename tKey>
	void ProcedureT<tKey>::waitUntilCompleted()
	{
		std::unique_lock<std::mutex> access(fAccessToCompleted);
		while ( ! fCompleted ) {
			fWaitingForCompleted.wait(access);
		}
	}

	template<typename tKey>
	void ProcedureT<tKey>::subProcedureWasDeleted()
	{
		--fCounter;
		if (fCounter == 0) allSubProceduresWereDeleted();
	}


	// ===== ProcedureReadRecordsFromRange

	template<typename tKey>
	void ProcedureReadRecordsFromRangeT<tKey>::process(std::vector<std::pair<tKey,uint8_t const *>> const & data)
	{
		if ( data.empty() ) return;
		auto compDataElementToKey = [](std::pair<tKey,uint8_t const*> const& e, tKey const& k)->bool{ return (e.first<k); };

		std::lock_guard<std::mutex> guard(this->fAccessToCompleted);

		auto it1 = std::lower_bound( data.begin(), data.end(), fFirstKey, compDataElementToKey );
		auto it2 = std::lower_bound( it1, data.end(), fLastKey, compDataElementToKey );
		while ( it2 != data.end() && it2->first == fLastKey ) ++it2;

		std::vector<Record const *> output;
		output.reserve( it2 - it1 );
		for ( ;  it1 != it2;  ++it1 ) {
			uint8_t const * ptr = it1->second;
			// read the record data size
			unsigned const recordSize = readUnsignedIntVarSize<1,1,unsigned>(ptr);
			// read the record
			uint8_t const * const prevPtr = ptr;
			Record * r = this->scheduler->callbackCreateRecord(it1->first,ptr);
			output.push_back(r);
			if (prevPtr + recordSize != ptr) {
				throw std::logic_error( std::string("Record length do not match number of bytes read! (1) ")
						+ " Expected: " + boost::lexical_cast<std::string>(recordSize)
						+ " Key: " + boost::lexical_cast<std::string>(it1->first)
						+ " position: " + boost::lexical_cast<std::string>(ptr-it1->second)
						);
			}
		}

		tKey const lastProcessedKey = data.rbegin()->first;
		bool lastCall = ( lastProcessedKey >= fLastKey );
		try {
			fCallback(output, lastCall);
		} catch (std::exception const & e) {
			// TODO - report bug
			std::cerr << "Error durign read by region: " << e.what() << std::endl;
			lastCall = true;
		}
		if ( lastCall || (lastProcessedKey >= fLastKey) ) {
			fEndOfQuery = true;
		} else {
			fFirstKey = lastProcessedKey + 1;  // lastProcessedKey < fLastKey
		}

		// free memory
		for ( ;  ! output.empty();  output.pop_back() ) delete output.back();
	}


	template<typename tKey>
	void ProcedureReadRecordsFromRangeT<tKey>::allSubProceduresWereDeleted()
	{
		this->fAccessToCompleted.lock();
		bool const completed = fEndOfQuery;
		this->fAccessToCompleted.unlock();
		if (completed) {
			this->announceAsCompleted();
		} else {
			this->scheduler->schedule(this);
		}
	}


	template<typename tKey>
	std::map<unsigned,typename SubProcedureT<tKey>::SP> ProcedureReadRecordsFromRangeT<tKey>::createSubProcedures(std::vector<BinT<tKey>> const & entries)
	{
		ASSERT(! entries.empty());
		this->fAccessToCompleted.lock();
		// to synchronize fFirstKey
		this->fAccessToCompleted.unlock();

		auto it = std::upper_bound(entries.begin(), entries.end(), fFirstKey, [](tKey const & k,Bin const & b)->bool{ return (k < b.firstKey); } );
		if ( it != entries.begin() ) {
			--it;
			if (it->lastKey() >= fFirstKey && it->recordsCount > 0) {  // recordsCount == 0: special case for empty database
				std::map<unsigned,typename SubProcedureT<tKey>::SP> m;
				unsigned const index = it - entries.begin();
				m[index].reset( new SubProcedureReadRecordsFromRange(this) );
				this->fCounter = m.size();
				return m;
			}
			++it;
		}
		while ( it != entries.end() && it->firstKey <= fLastKey) {
			if (it->recordsCount == 0) { // recordsCount == 0: special case for empty database
				++it;
				continue;
			}
			std::map<unsigned,typename SubProcedureT<tKey>::SP> m;
			unsigned const index = it - entries.begin();
			m[index].reset( new SubProcedureReadRecordsFromRange(this) );
			this->fCounter = m.size();
			return m;
		}
		// ----- no bins overlap with given range
		std::vector<Record const *> tEmpty;
		bool lastCall = true;
		try {
			fCallback( tEmpty, lastCall );
		} catch (std::exception const & e) {
			// TODO - report bug
			std::cerr << "Error durign read by region (empty): " << e.what() << std::endl;
			lastCall = true;
		}
		fEndOfQuery = true;
		this->announceAsCompleted();
		std::map<unsigned,typename SubProcedureT<tKey>::SP> m;
		return m;
	}



	template<typename tKey>
	std::map<unsigned,typename SubProcedureT<tKey>::SP> ProcedureReadRecordsByKeysT<tKey>::createSubProcedures(std::vector<BinT<tKey>> const & entries)
	{
		ASSERT(! entries.empty());
		if (fRecords.empty()) {
			this->announceAsCompleted();
			std::map<unsigned,typename SubProcedureT<tKey>::SP> m;
			return m;
		}

		std::sort(fRecords.begin(), fRecords.end(), [](Record* r1, Record* r2)->bool{ return (r1->key < r2->key);} );
		std::map<unsigned,typename SubProcedureT<tKey>::SP> m;
		auto iE = entries.begin();
		for ( auto iR = fRecords.begin();  iR != fRecords.end(); ) {
			iE = std::upper_bound( iE, entries.end(), (*iR)->key, [](tKey const & k, Bin const & b)->bool{ return (k < b.firstKey); } );
			if (iE != entries.begin()) --iE;
			// iE - bin for subprocedure
			unsigned const index = iE - entries.begin();
			auto iR2 = iR;
			if (++iE == entries.end()) {
				iR2 = fRecords.end();
			} else {
				tKey const nextFirstKey = iE->firstKey;
				while ( iR2 != fRecords.end() && (*iR2)->key < nextFirstKey ) ++iR2;
			}
			// copy records
			std::vector<Record*> records;
			records.reserve(iR2 - iR);
			records.assign(iR,iR2);
			m[index].reset( new SubProcedureReadRecordsByKeys(this,fCallback,records) );
			// move to the next range of keys
			iR = iR2;
		}

		this->fCounter = m.size();
		return m;
	}


	template<typename tKey>
	std::map<unsigned,typename SubProcedureT<tKey>::SP> ProcedureUpdateRecordsByKeysT<tKey>::createSubProcedures(std::vector<BinT<tKey>> const & entries)
	{
		ASSERT(! entries.empty());
		if (fRecords.empty()) {
			this->announceAsCompleted();
			std::map<unsigned,typename SubProcedureT<tKey>::SP> m;
			return m;
		}

		std::sort(fRecords.begin(), fRecords.end(), [](Record* r1, Record* r2)->bool{ return (r1->key < r2->key);} );
		std::map<unsigned,typename SubProcedureT<tKey>::SP> m;
		auto iE = entries.begin();
		for ( auto iR = fRecords.begin();  iR != fRecords.end(); ) {
			iE = std::upper_bound( iE, entries.end(), (*iR)->key, [](tKey const & k, Bin const & b)->bool{ return (k < b.firstKey); } );
			if (iE != entries.begin()) --iE;
			// iE - bin for subprocedure
			unsigned const index = iE - entries.begin();
			auto iR2 = iR;
			if (++iE == entries.end()) {
				iR2 = fRecords.end();
			} else {
				tKey const nextFirstKey = iE->firstKey;
				while ( iR2 != fRecords.end() && (*iR2)->key < nextFirstKey ) ++iR2;
			}
			// copy records
			std::vector<Record*> records;
			records.reserve(iR2 - iR);
			records.assign(iR,iR2);
			m[index].reset( new SubProcedureUpdateRecordsByKeys(this,fCallback,records) );
			// move to the next range of keys
			iR = iR2;
		}

		this->fCounter = m.size();
		return m;
	}


	template class ProcedureT<uint32_t>;
	template class ProcedureT<uint64_t>;
	template class ProcedureReadRecordsFromRangeT<uint32_t>;
	template class ProcedureReadRecordsFromRangeT<uint64_t>;
	template class ProcedureReadRecordsByKeysT<uint32_t>;
	template class ProcedureReadRecordsByKeysT<uint64_t>;
	template class ProcedureUpdateRecordsByKeysT<uint32_t>;
	template class ProcedureUpdateRecordsByKeysT<uint64_t>;
}
