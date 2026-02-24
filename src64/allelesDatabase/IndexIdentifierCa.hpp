#ifndef INDEXIDENTIFIERCA_HPP_
#define INDEXIDENTIFIERCA_HPP_

#include "RecordVariant64.hpp"
#include "../apiDb/TasksManager.hpp"
#include "../commonTools/Header.hpp"

	class IndexIdentifierCa {
	private:
		struct Pim;
		Pim * pim;
	public:
		IndexIdentifierCa(std::string dirPath, Header header, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB);
		std::vector<RecordGenomicVariant*> fetchDefinitions( std::vector<uint64_t> const &) const;
		void addIdentifiers(std::vector<RecordGenomicVariant const *> const & records);
		uint64_t getMaxIdentifier() const;
		void deleteIdentifiers(std::vector<RecordGenomicVariant*> &);
		// return true if the index was created in this run
		bool isNewDb() const;
		// destructor
		~IndexIdentifierCa();
	};


#endif /* INDEXIDENTIFIERCA_HPP_ */
