#include "requests.hpp"


RequestQueryAllelesByGene::~RequestQueryAllelesByGene()
{
	stopProcessingThread();
}


void RequestQueryAllelesByGene::process()
{
	std::list<std::vector<Document> > docsList;
	auto _docsList = &docsList;
	std::mutex m;
	auto _m = &m;
	auto callback = [_m,_docsList,this](std::vector<Document> & docs, bool & lastCall)
	{
		if (fRange.limit <= docs.size()) {
			docs.erase( docs.begin() + fRange.limit, docs.end() );
			fRange.limit = 0;
			lastCall = true;
		} else {
			fRange.limit -= docs.size();
		}
		_m->lock();
		_docsList->push_back(docs);
		_m->unlock();
	};

	auto regions = referencesDb->getGeneRegions(fGeneName);
	for (auto & kv: regions) {
		std::sort(kv.second.begin(), kv.second.end());
		for (auto const & reg: kv.second) {
			if (fRange.limit == 0) break;
			unsigned const chunkSize = std::min(fRange.limit, fMaxChunkSize);
			allelesDb->queryVariants(callback, fRange.skip, kv.first, reg.left(), reg.right(), chunkSize, fRange.limit);
		}
	}

	while(docsList.size() > 0) {
		auto docs = docsList.front();
		docsList.pop_front();
		fillVariantsDetails(docs);
		int delta = docs.size();
		docs = allelesDb->filterNamed(docs, false);
		delta -= docs.size();
		fRange.limit += delta;
		addChunkOfResponse(docs);
	}

}
