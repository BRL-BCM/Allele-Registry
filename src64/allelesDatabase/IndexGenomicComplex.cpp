#include "IndexGenomicComplex.hpp"


	struct IndexGenomicComplex::Pim
	{

	};


	IndexGenomicComplex::IndexGenomicComplex(std::string dirPath, TasksManager & taskManager) : pim(new Pim)
	{
		// TODO
	}

	IndexGenomicComplex::~IndexGenomicComplex()
	{
		delete pim;
	}

	void IndexGenomicComplex::addComplexDefinitions(std::vector<RecordGenomicVariant const*> const & records)
	{
		// TODO
	}

	// returns variants with at least one region crossing given interval and the first region's end <= from
	std::vector<RecordGenomicVariant *> IndexGenomicComplex::queryComplexDefinitions(uint32_t from, uint32_t to)
	{
		std::vector<RecordGenomicVariant*> v;
		return v;
		// TODO
	}
