#ifndef INDEXIDENTIFIERCA_HPP_
#define INDEXIDENTIFIERCA_HPP_

#include "RecordVariant.hpp"
#include "../apiDb/TasksManager.hpp"

	class IndexIdentifierCa {
	private:
		struct Pim;
		Pim * pim;
	public:
		IndexIdentifierCa(std::string dirPath, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB);
		std::vector<RecordGenomicVariant*> fetchDefinitions( std::vector<uint32_t> const &) const;
		void addIdentifiers(std::vector<RecordGenomicVariant const *> const & records);
		uint32_t getMaxIdentifier() const;
		void deleteIdentifiers(std::vector<RecordGenomicVariant*> &);
		// return true if the index was created in this run
		bool isNewDb() const;
		// destructor
		~IndexIdentifierCa();
	};


#endif /* INDEXIDENTIFIERCA_HPP_ */
