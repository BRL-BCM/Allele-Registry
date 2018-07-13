#ifndef TABLEGENOMIC_HPP_
#define TABLEGENOMIC_HPP_

#include <string>
#include <vector>
#include <map>
#include <atomic>
#include "RecordVariant.hpp"

	class TableGenomic
	{
	private:
		struct Pim;
		Pim * pim;
	public:
		// callback returning chunks of query's results
		// record objects must be released by delete
		// record objects left in the vector are automatically delete when the callback returns
		typedef std::function<void(std::vector<RecordGenomicVariant*> &, bool & lastCall)> tCallbackWithResults;
		// ------------------
		TableGenomic(std::string dirPath, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB, std::atomic<uint32_t> & nextFreeCaId);
		~TableGenomic();
		void query( tCallbackWithResults, unsigned & recordsToSkip, uint32_t first = 0, uint32_t last = std::numeric_limits<uint32_t>::max()
				  , unsigned minChunkSize = 1024, unsigned hintQuerySize = std::numeric_limits<unsigned>::max() ) const;
		// =========================== FETCH methods
		// - records are matched to these ones in database by definition
		// - always return the final caId & identifiers from the database after changes
		// - if record from given vector has caId, it must match, otherwise the record is considered unmatched
		// - caId and identifiers of unmatched records from given vector are cleared
		// fetch records
		void fetch(std::vector<RecordGenomicVariant*> const &) const;
		// fetch records and add new identifiers, returns map with new (added) uint identifiers (including CA IDs!)
		void fetchAndAdd   (std::vector<RecordGenomicVariant*> const &, std::map<identifierType,std::vector<std::pair<uint32_t,RecordVariantPtr>>> &);
		// fetch records and delete identifiers, returns map with matched (deleted) uint identifiers
		void fetchAndDelete(std::vector<RecordGenomicVariant*> const &, std::map<identifierType,std::vector<std::pair<uint32_t,RecordVariantPtr>>> &);
		// removes whole record
		void fetchAndFullDelete(std::vector<RecordGenomicVariant*> const &, std::map<identifierType,std::vector<std::pair<uint32_t,RecordVariantPtr>>> &);
		// =========================== delete identifiers
		void deleteIdentifiers(std::vector<RecordGenomicVariant*> const &, identifierType);

	};



#endif /* TABLEGENOMIC_HPP_ */
