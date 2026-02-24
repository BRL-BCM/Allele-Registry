#include "requests.hpp"

RequestQueryAllelesByRegion::RequestQueryAllelesByRegion( ResultSubset const & rs, std::string const & ref, std::string const & pRefGenome, unsigned from, unsigned to, std::string const pAlignment )
: Request(documentType::activeGenomicVariant), fRange(rs), worker(NULL), done(false)
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
	if(referencesDb->isSplicedRefSeq(fRefId) == false && referencesDb->isProteinReference(fRefId) == false) {
		if(to == std::numeric_limits<unsigned>::max()) {
			throw ExceptionIncorrectRequest("For chromosome sequences, please provide range parameters as illustrated by the following example for GRCh38 chromosome 1: /alleles.json?refseq=NC_000001.11&begin=0&end=10000000. For continued access to the Allele Registry we insist you limit your range query to <=10 million nucleotides.  Please direct inquiries to brl-allele-reg@bcm.edu.");
		}
	}
}

RequestQueryAllelesByRegion::~RequestQueryAllelesByRegion()
{
	stopProcessingThread();
}


void RequestQueryAllelesByRegion::cleanup() {
	m.lock();
	done = true;
	m.unlock();
	if(worker != NULL) {
		if(worker->joinable()) {
			worker->join();
		}
		delete worker;
		worker = NULL;
	}
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
		m.lock();
		docsList.push_back(docs);
		m.unlock();
	};

	worker = new std::thread([this]() -> void{
		while(true) {
			m.lock();
			int __size = docsList.size();
			bool __done = done;
			m.unlock();
			if(__done == true && __size == 0) break;
			if(__size > 0) {
				m.lock();
				auto docs = docsList.front();
				docsList.pop_front();
				m.unlock();
				try {
					fillVariantsDetails(docs);
					addChunkOfResponse(docs);
				} catch (...) { }
			} else {
				usleep(100000);
			}
		}
	});

	unsigned const refLength = referencesDb->getMetadata(fRefId).splicedLength;
	if (fRegion.left() <= refLength && fRegion.right() > refLength) fRegion.setRight(refLength);
	if (referencesDb->isSplicedRefSeq(fRefId)) {
		fRegion = referencesDb->convertToUnsplicedRegion(fRefId, SplicedRegionCoordinates(fRegion));
	}
	GeneralSeqAlignment const ga = referencesDb->getAlignmentFromMainGenome(fRefId, fRegion);
	for (auto const & e: ga.elements) {
		if (fRange.limit == 0) break;
		unsigned const chunkSize = std::min(fRange.limit, 64u*1024u);
		allelesDb->queryVariants(callback, fRange.skip, e.alignment.sourceRefId, e.alignment.sourceRegion().left(), e.alignment.sourceRegion().right(), chunkSize, fRange.limit);
	}

	m.lock();
	done = true;
	m.unlock();
	if(worker != NULL) {
		if(worker->joinable()) {
			worker->join();
		}
		delete worker;
		worker = NULL;
	}

}


