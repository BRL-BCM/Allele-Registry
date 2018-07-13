#ifndef TABLESEQUENCE_HPP_
#define TABLESEQUENCE_HPP_

#include <string>
#include <vector>
#include <cstdint>
#include "../apiDb/TasksManager.hpp"

	class TableSequence
	{
	private:
		struct Pim;
		Pim * pim;
	public:
		static uint32_t const unknownSequence;
		TableSequence(std::string dirPath, TasksManager * cpuTaskManager, TasksManager * ioTaskManager, unsigned cacheInMB);
		~TableSequence();
		void fetch(std::vector<uint32_t> const & seq, std::vector<std::string*> const & out) const;
		void fetch(std::vector<std::string const *> const & seq, std::vector<uint32_t> & out) const;
		void fetchAndAdd(std::vector<std::string const *> const & seq, std::vector<uint32_t> & out) const;
	};




#endif /* TABLESEQUENCE_HPP_ */
