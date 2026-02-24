#ifndef INDEXIDENTIFIERCA32_HPP_
#define INDEXIDENTIFIERCA32_HPP_

#include "RecordVariant64.hpp"
#include "../apiDb/TasksManager.hpp"
#include "../commonTools/Header.hpp"

	class IndexIdentifierCa32 {
	private:
		struct Pim;
		Pim * pim;
	public:
		IndexIdentifierCa32(std::string dirPath, Header header, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB);
		std::vector<RecordGenomicVariant*> fetchDefinitions( std::vector<uint32_t> const &) const;
		void addIdentifiers(std::vector<RecordGenomicVariant const *> const & records);
		uint32_t getMaxIdentifier() const;
		void deleteIdentifiers(std::vector<RecordGenomicVariant*> &);
		// return true if the index was created in this run
		bool isNewDb() const;
		// destructor
		~IndexIdentifierCa32();
	};


#endif /* INDEXIDENTIFIERCA32_HPP_ */
