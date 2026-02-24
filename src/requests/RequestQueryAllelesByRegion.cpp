#include "requests.hpp"

RequestQueryAllelesByRegion::RequestQueryAllelesByRegion( ResultSubset const & rs, std::string const & ref, std::string const & pRefGenome, unsigned from, unsigned to, std::string const pAlignment )
: Request(documentType::activeGenomicVariant), fRange(rs)
{
	if (from > to) throw ExceptionIncorrectRequest("Parameter 'begin' cannot be larger than parameter 'end'.");
	fRefGenome = pRefGenome;
	if(pAlignment.compare("left") == 0) {
		fAlignment = alignment::leftAligned;
	} else
	if(pAlignment.compare("right") == 0) {
		fAlignment = alignment::rightAligned;
	} else 
	if(pAlignment.compare("explicit") == 0) {
		fAlignment = alignment::completeAligned;
	} else 
	{
		throw ExceptionIncorrectRequest("alignment should be one of {left, right, explicit}");
	}
	fRefId = ReferenceId::null;
	if(ref.rfind("chr", 0) == 0) {
		if (fRefGenome.length() == 0) throw ExceptionIncorrectRequest("please specify reference genome, Ex: refGenome=GRCh38");
		ReferenceGenome refGenome = parseReferenceGenome(fRefGenome);
		// strip chr prefix
		fChr = ref.substr(3);
		Chromosome chr = parseChromosome(fChr);
		fRefId = referencesDb->getReferenceId(refGenome, chr);
	} else if(ref.rfind("RS", 0) == 0) {
		// prefix is RS, checking for reference id
		fRefId = referencesDb->getReferenceId(ref);
	} else {
		// try to find name
		bool searching = true;
		for(int i=0;i<referencesDb->getNumMetadata() && searching;i++) {
			ReferenceId id(i);
			std::vector<std::string> names = referencesDb->getNames(id);
			for(int j=0;j<names.size();j++) {
				if(ref.compare(names[j]) == 0) {
					// match
					fRefId = id;
					searching = false;
					break;
				}
			}
		}
	}
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
		fillVariantsDetails(docs, fAlignment);
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


