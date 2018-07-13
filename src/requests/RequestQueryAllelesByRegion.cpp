#include "requests.hpp"



RequestQueryAllelesByRegion::RequestQueryAllelesByRegion( ResultSubset const & rs, std::string const & ref, unsigned from, unsigned to )
: Request(documentType::activeGenomicVariant), fRange(rs)
{
	fRefId = referencesDb->getReferenceId(ref);
	if (from > to) throw ExceptionIncorrectRequest("Parameter 'begin' cannot be larger than parameter 'end'.");
	fRegion.setLeftAndLength(from, to-from);
}


RequestQueryAllelesByRegion::~RequestQueryAllelesByRegion()
{
	stopProcessingThread();
}


void RequestQueryAllelesByRegion::process()
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

	unsigned const refLength = referencesDb->getMetadata(fRefId).splicedLength;
	if (fRegion.left() <= refLength && fRegion.right() > refLength) fRegion.setRight(refLength);
	if (referencesDb->isSplicedRefSeq(fRefId)) {
		fRegion = referencesDb->convertToUnsplicedRegion(fRefId, SplicedRegionCoordinates(fRegion));
	}
	GeneralSeqAlignment const ga = referencesDb->getAlignmentFromMainGenome(fRefId, fRegion);
	for (auto const & e: ga.elements) {
		if (fRange.limit == 0) break;
		unsigned const chunkSize = std::min(fRange.limit, 1024u*1024u);
		allelesDb->queryVariants(callback, fRange.skip, e.alignment.sourceRefId, e.alignment.sourceRegion().left(), e.alignment.sourceRegion().right(), chunkSize, fRange.limit);
	}
}


