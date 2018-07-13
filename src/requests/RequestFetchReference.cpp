#include "requests.hpp"


	RequestFetchReference::RequestFetchReference(std::string const & id, bool details)
	: Request(documentType::reference)
	{
		if (id.substr(0,2) != "RS" || id.size() < 3) {
			throw ExceptionIncorrectRequest("Reference id must consist of prefix 'RS' and decimal number. Given value: '" + id + "'.");
		}
		for (unsigned i = 2; i < id.size(); ++i) if (id[i] < '0' || id[i] > '9') {
			throw ExceptionIncorrectRequest("Reference id must consist of prefix 'RS' and decimal number. Given value: '" + id + "'.");
		}
		fRsId = boost::lexical_cast<unsigned>(id.substr(2));
		fDetails = details;
	}


	RequestFetchReference::~RequestFetchReference()
	{
		stopProcessingThread();
	}


	void RequestFetchReference::process()
	{
		Document doc = DocumentReference();
		std::string rsId = boost::lexical_cast<std::string>(fRsId);
		while (rsId.size() < 6) rsId = "0" + rsId;
		rsId = "RS" + rsId;
		std::vector<ReferenceId> ids = referencesDb->getReferencesByName(rsId);
		ReferenceId const id = (ids.empty()) ? (ReferenceId::null) : (ids.front());
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
