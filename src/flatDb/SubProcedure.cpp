#include "SubProcedure.hpp"
#include "Procedure.hpp"
#include "Scheduler.hpp"
#include "../commonTools/bytesLevel.hpp"
#include <boost/lexical_cast.hpp>
#include <algorithm>


namespace flatDb {


	template<typename tKey>
	SubProcedureT<tKey>::~SubProcedureT()
	{
		if (fOwner != nullptr) fOwner->subProcedureWasDeleted();
	}


	template<typename tKey>
	void SubProcedureReadRecordsFromRangeT<tKey>::process(std::vector<std::pair<tKey,uint8_t const *>> const & data)
	{
		dynamic_cast<ProcedureReadRecordsFromRangeT<tKey>*>(this->fOwner)->process(data);
	}


	template<typename tKey>
	void SubProcedureReadRecordsByKeysT<tKey>::process(std::vector<std::pair<tKey,uint8_t const *>> const & data)
	{
		auto iD = data.begin();
		auto compDataElementToKey = [](std::pair<tKey,uint8_t const*> const& e, tKey const& k)->bool{ return (e.first<k); };
		for  ( auto it = fRecords.begin();  it != fRecords.end();  ) {
			tKey const key = (*it)->key;
			// ----- find range with the same key
			auto it2 = it;
			for ( ++it2;  it2 != fRecords.end() && (*it2)->key == key;  ++it2 );
			// ----- get records range from user
			std::vector<Record*> userTemp;
			userTemp.reserve(it2 - it);
			userTemp.assign(it,  it2);
			// ----- get records range from DB
			std::vector<Record const *> dbTemp;
			iD = std::lower_bound( iD, data.end(), key, compDataElementToKey );
			for ( ;  iD != data.end() && iD->first == key;  ++iD ) {
				uint8_t const * ptr = iD->second;
				// read the record data size
				unsigned const recordSize = readUnsignedIntVarSize<1,1,unsigned>(ptr);
				// read the record
				uint8_t const * const prevPtr = ptr;
				Record * r = this->fOwner->scheduler->callbackCreateRecord(key,ptr);
				dbTemp.push_back(r);
				if (prevPtr + recordSize != ptr) {
					throw std::logic_error( std::string("Record length do not match number of bytes read! (1) ")
							+ " Expected: " + boost::lexical_cast<std::string>(recordSize)
							+ " Key: " + boost::lexical_cast<std::string>(key)
							+ " position: " + boost::lexical_cast<std::string>(ptr-iD->second)
							);
				}
			}
			// ----- call callback
			try {
				fCallback(dbTemp, userTemp);
			} catch (std::exception const & e) {
				// TODO - report bug
				std::cerr << "Error durign read by key: " << e.what() << std::endl;
			}
			// ----- free memory
			for ( ; ! dbTemp.empty();  dbTemp.pop_back() ) delete dbTemp.back();
			// ----- move to the next key
			it = it2;
		}
	}


	template<typename tKey>
	bool SubProcedureUpdateRecordsByKeysT<tKey>::process(std::vector<std::pair<tKey,uint8_t const *>> & data, tAllocateBufferFunction callAllocate)
	{
		std::vector<std::pair<tKey,uint8_t const *>> newData;
		newData.reserve(data.size());

		auto iD = data.begin();
		auto compDataElementToKey = [](std::pair<tKey,uint8_t const*> const& e, tKey const& k)->bool{ return (e.first<k); };

		bool hasChanges = false;
		for  ( auto iR = fRecords.begin();  iR != fRecords.end();  ) {
			tKey const key = (*iR)->key;
			// ----- find range with the same key
			auto iR2 = iR;
			for ( ++iR2;  iR2 != fRecords.end() && (*iR2)->key == key;  ++iR2 );
			// ----- get records range from user
			std::vector<Record*> userTemp;
			userTemp.reserve(iR2 - iR);
			userTemp.assign(iR,  iR2);
			// ----- get records range from DB
			std::vector<Record*> dbTemp;
			auto iDprevBegin = iD;
			auto iDprevEnd = iD = std::lower_bound( iD, data.end(), key, compDataElementToKey );
			for ( ;  iD != data.end() && iD->first == key;  ++iD ) {
				uint8_t const * ptr = iD->second;
				// read the record data size
				unsigned const recordSize = readUnsignedIntVarSize<1,1,unsigned>(ptr);
				// read the record
				uint8_t const * const prevPtr = ptr;
				Record * r = this->fOwner->scheduler->callbackCreateRecord(key,ptr);
				dbTemp.push_back(r);
				if (prevPtr + recordSize != ptr) {
					throw std::logic_error( std::string("Record length do not match number of bytes read! (1) ")
							+ " Expected: " + boost::lexical_cast<std::string>(recordSize)
							+ " Key: " + boost::lexical_cast<std::string>(key)
							+ " position: " + boost::lexical_cast<std::string>(ptr-iD->second)
							);
				}
			}
			// ----- call callback
			bool wasUpdated = false;
			try {
				wasUpdated = fCallback(dbTemp, userTemp);
			} catch (std::exception const & e) {
				// TODO - report bug
				std::cerr << "Error durign write by key: " << e.what() << std::endl;
			}
			if ( wasUpdated ) {
				hasChanges = true;
				newData.insert(newData.end(), iDprevBegin, iDprevEnd);
				for (auto r: dbTemp) {
					unsigned const recordSize = r->dataLength();
					unsigned const recordLengthSize = lengthUnsignedIntVarSize<1,1,unsigned>(recordSize);
					uint8_t* ptr = callAllocate(recordLengthSize + recordSize);
					uint8_t* const buf = ptr;
					writeUnsignedIntVarSize<1,1,unsigned>(ptr, recordSize);
					r->saveData(ptr);
					ASSERT( ptr - buf == recordLengthSize + recordSize );
					newData.push_back( std::make_pair(key,buf) );
				}
			} else {
				newData.insert(newData.end(), iDprevBegin, iD);
			}
			// ----- free memory
			for ( ; ! dbTemp.empty();  dbTemp.pop_back() ) delete dbTemp.back();
			// ----- move to the next key
			iR = iR2;
		}
		if (hasChanges) {
			// ----- copy the rest of untouched records
			newData.insert(newData.end(), iD, data.end());
			// ----- replace data by new one
			data.swap(newData);
		}
		return hasChanges;
	}


	template class SubProcedureT<uint32_t>;
	template class SubProcedureT<uint64_t>;
	template class SubProcedureReadRecordsFromRangeT<uint32_t>;
	template class SubProcedureReadRecordsFromRangeT<uint64_t>;
	template class SubProcedureReadRecordsByKeysT<uint32_t>;
	template class SubProcedureReadRecordsByKeysT<uint64_t>;
	template class SubProcedureUpdateRecordsByKeysT<uint32_t>;
	template class SubProcedureUpdateRecordsByKeysT<uint64_t>;
}
