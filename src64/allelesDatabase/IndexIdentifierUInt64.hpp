#ifndef INDEXIDENTIFIERUINT64_HPP_
#define INDEXIDENTIFIERUINT64_HPP_

#include "RecordVariant64.hpp"
#include "../commonTools/Header.hpp"

	class IndexIdentifierUInt64 {
	private:
		struct Pim;
		Pim * pim;
	public:
		IndexIdentifierUInt64(std::string dirPath, Header header, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, std::string const & name, unsigned cacheInMB);
		~IndexIdentifierUInt64();
		std::vector<std::vector<RecordVariantPtr>> queryDefinitions(std::vector<uint64_t> const &) const;
		void addIdentifiers   (std::vector<std::pair<uint64_t,RecordVariantPtr>> const &);
		void deleteIdentifiers(std::vector<std::pair<uint64_t,RecordVariantPtr>> const &);
		// method to delete the region of keys, returns Records references from erased entries in callback, RecordsVariantPtr must be deleted by caller
		typedef std::function<void(std::vector<RecordVariantPtr> &, bool & lastCall)> tCallbackWithDeletedRef;
		void deleteEntries( tCallbackWithDeletedRef, uint64_t first, uint64_t last, unsigned minChunkSize );
		// return true if the index was created in this run
		bool isNewDb() const;
	};


#endif /* INDEXIDENTIFIERUINT64_HPP_ */
