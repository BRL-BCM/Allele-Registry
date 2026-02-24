#include "requests.hpp"


	RequestFetchReference::RequestFetchReference(std::string const & ref, std::string const & pRefBuild, bool details)
	: Request(documentType::reference)
	{
		fRefBuild = (pRefBuild.size() == 0)?"GRCh38":pRefBuild;
		fRefId = ReferenceId::null;
		if(ref.rfind("RS", 0) == 0) {
			if (ref.substr(0,2) != "RS" || ref.size() < 3) {
				throw ExceptionIncorrectRequest("Reference id must consist of prefix 'RS' and decimal number. Given value: '" + ref + "'.");
			}
			for (unsigned i = 2; i < ref.size(); ++i) if (ref[i] < '0' || ref[i] > '9') {
				throw ExceptionIncorrectRequest("Reference id must consist of prefix 'RS' and decimal number. Given value: '" + ref + "'.");
			}
			fRefId = referencesDb->getReferenceId(ref);
		} else if(ref.rfind("chr", 0) == 0) {
			std::string fRefGenome = fRefBuild;
			ReferenceGenome refGenome = parseReferenceGenome(fRefGenome);
			// strip chr prefix
			std::string fChr = ref.substr(3);
			Chromosome chr = parseChromosome(fChr);
			fRefId = referencesDb->getReferenceId(refGenome, chr);
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
		fDetails = details;
	}


	RequestFetchReference::~RequestFetchReference()
	{
		stopProcessingThread();
	}


	void RequestFetchReference::process()
	{
		Document doc = DocumentReference();
		ReferenceId const id = fRefId;
		if (id == ReferenceId::null) {
			doc = DocumentError(errorType::NotFound);
		} else {
			std::vector<std::string> names = referencesDb->getNames(id);
			ReferenceMetadata ref = referencesDb->getMetadata(id);
			doc.reference().chromosome = ref.chromosome;
			if (ref.genomeBuild != "") doc.reference().genomeBuild = parseReferenceGenome(ref.genomeBuild);
			doc.reference().gnId = ref.geneId;
			doc.reference().transcript = referencesDb->isSplicedRefSeq(id);
			for (auto const & name: names) {
				if (name.substr(0,2) == "RS") doc.reference().rsId = boost::lexical_cast<unsigned>(name.substr(2));
				else if (name.substr(0,3) == "ENS") doc.reference().ensemblId = name;
				else if (name.substr(2,1) == "_") doc.reference().ncbiId = name;
			}
			// undocumented
			if (fDetails) {
				doc.reference().sequence = referencesDb->getSequence(id, RegionCoordinates(0,referencesDb->getSequenceLength(id)));
			    if (referencesDb->isSplicedRefSeq(id)) {
			        doc.reference().CDS = referencesDb->getCDS(id);
			        std::vector<std::pair<RegionCoordinates,SplicedRegionCoordinates>> exons = referencesDb->getExons(id);
			        for (auto const & e: exons) {
			            doc.reference().splicedSequence += doc.reference().sequence.substr(e.first.left(), e.first.length());
			        }
			    }
			}
		}
		setResponse(doc);
	}
