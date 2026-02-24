#ifndef TABLE_PROTEIN_SEQUENCE_HPP_
#define TABLE_PROTEIN_SEQUENCE_HPP_

#include <string>
#include <vector>
#include <cstdint>
#include "../apiDb/TasksManager.hpp"
#include "../commonTools/Header.hpp"

	class TableProteinSequence
	{
	private:
		struct Pim;
		Pim * pim;
	public:
		static uint32_t const unknownSequence;
		TableProteinSequence(std::string dirPath, Header header, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB);
		~TableProteinSequence();
		void fetch(std::vector<uint64_t> const & seq, std::vector<std::string*> const & out) const;
		void fetch(std::vector<std::string const *> const & seq, std::vector<uint64_t> & out) const;
		void fetchAndAdd(std::vector<std::string const *> const & seq, std::vector<uint64_t> & out) const;
	};




#endif /* TABLE_PROTEIN_SEQUENCE_HPP_ */
