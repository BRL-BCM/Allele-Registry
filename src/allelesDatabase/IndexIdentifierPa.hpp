#ifndef INDEXIDENTIFIERPA_HPP_
#define INDEXIDENTIFIERPA_HPP_

#include "RecordVariant.hpp"
#include "../apiDb/TasksManager.hpp"

	class IndexIdentifierPa {
	private:
		struct Pim;
		Pim * pim;
	public:
		IndexIdentifierPa(std::string dirPath, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB);
		std::vector<RecordProteinVariant*> fetchDefinitions( std::vector<uint32_t> const &) const;
		void addIdentifiers(std::vector<RecordProteinVariant const *> const & records);
		uint32_t getMaxIdentifier() const;
		void deleteIdentifiers(std::vector<RecordProteinVariant*> &);
		// return true if the index was created in this run
		bool isNewDb() const;
		// destructor
		~IndexIdentifierPa();
	};


#endif /* INDEXIDENTIFIERPA_HPP_ */
