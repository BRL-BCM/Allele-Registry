#include "requests.hpp"


	RequestQueryReferences::RequestQueryReferences(ResultSubset rs, std::string const & geneOrName, bool geneName)
	: Request(documentType::reference), fRange(rs), fName(geneOrName), fGene(geneName)
	{}


	RequestQueryReferences::~RequestQueryReferences()
	{
		stopProcessingThread();
	}

	void RequestQueryReferences::process()
	{
		std::vector<ReferenceId> refsIds;
		if (fGene) {
			unsigned geneId = 0;
			bool isNumber = true;
			std::string fName2 = fName;
			if (fName2.substr(0,2) == "GN") fName2 = fName2.substr(2);  // TODO - remove this mess, create parsing stuff
			for (unsigned i = 0; i < fName2.size(); ++i) {
				if (fName2[i] < '0' || fName2[i] > '9') {
					isNumber = false;
					break;
				}
				geneId *= 10;
				geneId += (fName2[i] - '0');
			}
			if (isNumber) {
				refsIds = referencesDb->getReferencesByGene(geneId);
			} else {
				refsIds = referencesDb->getReferencesByGene(fName);
			}
		} else {
			refsIds = referencesDb->getReferencesByName(fName);
		}

		std::vector<Document> docs;
		for ( unsigned i = fRange.skip;  i < refsIds.size(); ++i) {
			if (docs.size() == fRange.limit) break;
			ReferenceId const id = refsIds[i];
			std::vector<std::string> names = referencesDb->getNames(id);
			ReferenceMetadata ref = referencesDb->getMetadata(id);
			DocumentReference doc;
			doc.chromosome = ref.chromosome;
			if (ref.genomeBuild != "")  doc.genomeBuild = parseReferenceGenome(ref.genomeBuild);
			doc.gnId = ref.geneId;
			doc.transcript = referencesDb->isSplicedRefSeq(id);
			for (auto const & name: names) {
				if (name.substr(0,2) == "RS") doc.rsId = boost::lexical_cast<unsigned>(name.substr(2));
				else if (name.substr(0,3) == "ENS") doc.ensemblId = name;
				else if (name.substr(2,1) == "_") doc.ncbiId = name;
			}
			docs.push_back(doc);
		}

		addChunkOfResponse(docs);
	}
