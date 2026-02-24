#include "requests.hpp"
#include "hgvs.hpp"
#include "canonicalization.hpp"

RequestQueryAlleleIntersection::RequestQueryAlleleIntersection( ResultSubset const & rs, std::string const pHgvs, std::string const pAlignment )
: Request(documentType::activeGenomicVariant), fRange(rs)
{
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


	fHgvs = pHgvs;


	HgvsVariant hgvsVar(true);
	decodeHgvs(referencesDb, fHgvs, hgvsVar);

	if(hgvsVar.isGenomic()) {
		unsigned from, to;
		DocumentActiveGenomicVariant doc(canonicalize(referencesDb, hgvsVar.genomic));
		for(auto modification : doc.mainDefinition.modifications) {
			from = modification.region.left();
			to = modification.region.right();
			fRefId = doc.mainDefinition.refId;
			if (from > to) throw ExceptionIncorrectRequest("Parameter 'begin' cannot be larger than parameter 'end'.");
			RegionCoordinates tRegionOrig;
			tRegionOrig.setLeftAndLength(from, to-from);
			fRegionOrig.push_back(tRegionOrig);
			int fThreshold = 10;
			if(from>fThreshold) {from=from-fThreshold;} else {from=0;}
			to=to+fThreshold;
			RegionCoordinates tRegion;
			tRegion.setLeftAndLength(from, to-from);
			fRegion.push_back(tRegion);
		}
	} else { // is protein
		unsigned from, to;
		DocumentActiveProteinVariant doc(canonicalize(referencesDb, hgvsVar.protein));
		for(auto modification : doc.mainDefinition.modifications) {
			from = modification.region.left();
			to = modification.region.right();
			fRefId = doc.mainDefinition.refId;
			if (from > to) throw ExceptionIncorrectRequest("Parameter 'begin' cannot be larger than parameter 'end'.");
			RegionCoordinates tRegionOrig;
			tRegionOrig.setLeftAndLength(from, to-from);
			fRegionOrig.push_back(tRegionOrig);
			int fThreshold = 10;
			if(from>fThreshold) {from=from-fThreshold;} else {from=0;}
			to=to+fThreshold;
			RegionCoordinates tRegion;
			tRegion.setLeftAndLength(from, to-from);
			fRegion.push_back(tRegion);
		}
	}
}

RequestQueryAlleleIntersection::~RequestQueryAlleleIntersection()
{
	stopProcessingThread();
}


void RequestQueryAlleleIntersection::process()
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
		std::vector<Document> fdocs;
		for(auto & doc : docs) {
			try {
				if (doc.isActiveGenomicVariant()) {
					DocumentActiveGenomicVariant const & d(doc.asActiveGenomicVariant());
					bool add = false;
					for(auto & region: fRegionOrig) {
						for(auto & modification: d.mainDefinition.modifications) {
							if(modification.region.left() <= region.left()
							&& modification.region.right() <= region.left())
							{ continue; } // no intersection
							if(modification.region.left() >= region.right()
							&& modification.region.right() >= region.right())
							{ continue; } // no intersection
							add = true;
							break;
						}
					}
					if(add) {
						fdocs.push_back(doc);
					}
				} else if (doc.isActiveProteinVariant()) {
					DocumentActiveProteinVariant const & d(doc.asActiveProteinVariant());
					bool add = false;
					for(auto & region: fRegionOrig) {
						for(auto & modification: d.mainDefinition.modifications) {
							if(modification.region.left() <= region.left()
							&& modification.region.right() <= region.left())
							{ continue; } // no intersection
							if(modification.region.left() >= region.right()
							&& modification.region.right() >= region.right())
							{ continue; } // no intersection
							add = true;
							break;
						}
					}
					if(add) {
						fdocs.push_back(doc);
					}
				}
			} catch (...) { }
		}
		fillVariantsDetails(fdocs, fAlignment);
		int delta = docs.size();
		fdocs = allelesDb->filterNamed(fdocs, false);
		delta -= docs.size();
		fRange.limit += delta;
		addChunkOfResponse(fdocs);
	};

	unsigned skip = 0;
	for(auto & region: fRegionOrig) {
		GeneralSeqAlignment const ga = referencesDb->getAlignmentFromMainGenome(fRefId, region);
		region = ga.elements.front().alignment.sourceRegion();
	}
	for(auto & region: fRegion) {
		unsigned const refLength = referencesDb->getMetadata(fRefId).splicedLength;
		if (region.left() <= refLength && region.right() > refLength) region.setRight(refLength);
		GeneralSeqAlignment const ga = referencesDb->getAlignmentFromMainGenome(fRefId, region);
		for (auto const & e: ga.elements) {
			if (fRange.limit == 0) break;
			unsigned const chunkSize = std::min(fRange.limit, 1024u);
			allelesDb->queryVariants(callback, skip, e.alignment.sourceRefId, e.alignment.sourceRegion().left(), e.alignment.sourceRegion().right(), chunkSize);
		}
	}
}


