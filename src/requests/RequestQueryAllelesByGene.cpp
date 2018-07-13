#include "requests.hpp"


RequestQueryAllelesByGene::~RequestQueryAllelesByGene()
{
	stopProcessingThread();
}


void RequestQueryAllelesByGene::process()
{
	auto callback = [this](std::vector<Document> & docs, bool & lastCall)
	{
		if (fRange.limit <= docs.size()) {
			docs.erase( docs.begin() + fRange.limit, docs.end() );
			fRange.limit = 0;
			lastCall = true;
		} else {
			fRange.limit -= docs.size();
		}
		fillVariantsDetails(docs);
		addChunkOfResponse(docs);
	};

	auto regions = referencesDb->getGeneRegions(fGeneName);
	for (auto & kv: regions) {
		std::sort(kv.second.begin(), kv.second.end());
		for (auto const & reg: kv.second) {
			if (fRange.limit == 0) break;
			unsigned const chunkSize = std::min(fRange.limit, 1024u*1024u);
			allelesDb->queryVariants(callback, fRange.skip, kv.first, reg.left(), reg.right(), chunkSize, fRange.limit);
		}
	}
}
