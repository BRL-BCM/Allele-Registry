#ifndef TABLEPROTEIN_HPP_
#define TABLEPROTEIN_HPP_

#include <string>
#include <vector>
#include <map>
#include <atomic>
#include "RecordVariant.hpp"

	class TableProtein
	{
	private:
		struct Pim;
		Pim * pim;
	public:
		// callback returning chunks of query's results
		// record objects must be released by delete
		// record objects left in the vector are automatically delete when the callback returns
		typedef std::function<void(std::vector<RecordProteinVariant*> &, bool & lastCall)> tCallbackWithResults;
		// ------------------
		TableProtein(std::string dirPath, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB, std::atomic<uint32_t> & nextFreeCaId);
		~TableProtein();
		void query( tCallbackWithResults, unsigned & recordsToSkip, uint64_t first = 0, uint64_t last = std::numeric_limits<uint64_t>::max()
				  , unsigned minChunkSize = 1024, unsigned hintQuerySize = std::numeric_limits<unsigned>::max() ) const;
		// =========================== FETCH methods
		// - records are matched to these ones in database by definition
		// - always return the final caId & identifiers from the database after changes
		// - if record from given vector has caId, it must match, otherwise the record is considered unmatched
		// - caId and identifiers of unmatched records from given vector are cleared
		// fetch records
		void fetch(std::vector<RecordProteinVariant*> const &) const;
		// fetch records and add new identifiers, returns map with new (added) uint identifiers (including CA IDs!)
		void fetchAndAdd   (std::vector<RecordProteinVariant*> const &, std::map<identifierType,std::vector<std::pair<uint32_t,RecordVariantPtr>>> &);
		// fetch records and delete identifiers, returns map with matched (deleted) uint identifiers
		void fetchAndDelete(std::vector<RecordProteinVariant*> const &, std::map<identifierType,std::vector<std::pair<uint32_t,RecordVariantPtr>>> &);
		// removes whole record
		void fetchAndFullDelete(std::vector<RecordProteinVariant*> const &, std::map<identifierType,std::vector<std::pair<uint32_t,RecordVariantPtr>>> &);
		// =========================== delete identifiers
		void deleteIdentifiers(std::vector<RecordProteinVariant*> const &, identifierType);
	};



#endif /* TABLEPROTEIN_HPP_ */
