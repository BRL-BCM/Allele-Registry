#ifndef INDEXIDENTIFIERUINT32_HPP_
#define INDEXIDENTIFIERUINT32_HPP_

#include "RecordVariant64.hpp"
#include "../commonTools/Header.hpp"

	class IndexIdentifierUInt32 {
	private:
		struct Pim;
		Pim * pim;
	public:
		IndexIdentifierUInt32(std::string dirPath, Header header, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, std::string const & name, unsigned cacheInMB);
		~IndexIdentifierUInt32();
		unsigned int getRecordsCount();
		unsigned int getLargestKey();
		std::vector<std::vector<RecordVariantPtr>> queryDefinitions(std::vector<uint32_t> const &) const;
		void addIdentifiers   (std::vector<std::pair<uint32_t,RecordVariantPtr>> const &);
		void deleteIdentifiers(std::vector<std::pair<uint32_t,RecordVariantPtr>> const &);
		// method to delete the region of keys, returns Records references from erased entries in callback, RecordsVariantPtr must be deleted by caller
		typedef std::function<void(std::vector<RecordVariantPtr> &, bool & lastCall)> tCallbackWithDeletedRef;
		void deleteEntries( tCallbackWithDeletedRef, uint32_t first, uint32_t last, unsigned minChunkSize );
		// return true if the index was created in this run
		bool isNewDb() const;
	};


#endif /* INDEXIDENTIFIERUINT32_HPP_ */
