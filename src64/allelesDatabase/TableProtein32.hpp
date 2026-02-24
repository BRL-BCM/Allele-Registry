#ifndef TABLEPROTEIN32_HPP_
#define TABLEPROTEIN32_HPP_

#include <string>
#include <vector>
#include <map>
#include <atomic>
#include "RecordVariant32.hpp"
#include "../commonTools/Header.hpp"

	class TableProtein32
	{
	private:
		struct Pim;
		Pim * pim;
	public:
		// callback returning chunks of query's results
		// record objects must be released by delete
		// record objects left in the vector are automatically delete when the callback returns
		typedef std::function<void(std::vector<RecordProteinVariant32*> &, bool & lastCall)> tCallbackWithResults;
		// ------------------
		TableProtein32(std::string dirPath, Header header, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB, std::atomic<uint64_t> & nextFreeCaId);
		~TableProtein32();
		void query( tCallbackWithResults, unsigned & recordsToSkip, uint64_t first = 0, uint64_t last = std::numeric_limits<uint64_t>::max()
				  , unsigned minChunkSize = 1024, unsigned hintQuerySize = std::numeric_limits<unsigned>::max() ) const;
		// =========================== FETCH methods
		// - records are matched to these ones in database by definition
		// - always return the final caId & identifiers from the database after changes
		// - if record from given vector has caId, it must match, otherwise the record is considered unmatched
		// - caId and identifiers of unmatched records from given vector are cleared
		// fetch records
		void fetch(std::vector<RecordProteinVariant32*> const &) const;
		// fetch records and add new identifiers, returns map with new (added) uint identifiers (including CA IDs!)
		void fetchAndAdd   (std::vector<RecordProteinVariant32*> const &, std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr32>>> &);
		// fetch records and delete identifiers, returns map with matched (deleted) uint identifiers
		void fetchAndDelete(std::vector<RecordProteinVariant32*> const &, std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr32>>> &);
		// removes whole record
		void fetchAndFullDelete(std::vector<RecordProteinVariant32*> const &, std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr32>>> &);
		// =========================== delete identifiers
		void deleteIdentifiers(std::vector<RecordProteinVariant32*> const &, identifierType);
	};



#endif /* TABLEPROTEIN32_HPP_ */
