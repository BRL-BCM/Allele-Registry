#include "requests.hpp"



RequestQueryAlleles::~RequestQueryAlleles()
{
	stopProcessingThread();
}


void RequestQueryAlleles::process()
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

	if (fProteinAlleles) {
		allelesDb->queryProteinVariants(callback, fRange.skip);
	} else {
		allelesDb->queryGenomicVariants(callback, fRange.skip);
	}
}

