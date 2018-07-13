#ifndef INDEXGENOMICCOMPLEX_HPP_
#define INDEXGENOMICCOMPLEX_HPP_

#include "TableGenomic.hpp"

	class IndexGenomicComplex {
	private:
		struct Pim;
		Pim * pim;
	public:
		IndexGenomicComplex(std::string dirPath, TasksManager & taskManager);
		void addComplexDefinitions(std::vector<RecordGenomicVariant const*> const &);
		// returns variants with at least one region crossing given interval and the first region's end <= from
		std::vector<RecordGenomicVariant *> queryComplexDefinitions(uint32_t from, uint32_t to);
		~IndexGenomicComplex();
	};



#endif /* INDEXGENOMICCOMPLEX_HPP_ */
