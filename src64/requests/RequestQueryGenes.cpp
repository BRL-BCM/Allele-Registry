#include "requests.hpp"


RequestQueryGenes::~RequestQueryGenes()
{
	stopProcessingThread();
}


void RequestQueryGenes::process()
{
	std::vector<Gene> genes;
	if (fName == "") {
		genes = referencesDb->getGenes();
	} else {
		genes = referencesDb->getGenesByName(fName);
	}

	std::vector<Document> docs;

	if (fRange.limit > 0) {
		unsigned skipped = 0;
		for (auto const & gene: genes) {
			if (! gene.active) continue;
			if (skipped < fRange.skip) {
				++skipped;
				continue;
			}
			DocumentGene doc;
			doc.gnId = gene.hgncId;
			doc.hgncName = gene.hgncName;
			doc.hgncSymbol = gene.hgncSymbol;
			doc.ncbiId = gene.refSeqId;
			std::string preferredTranscript = referencesDb->getPreferredTranscriptFromHGNCId(gene.hgncId);
			if(preferredTranscript != "") doc.prefRefSeq = preferredTranscript;
			docs.push_back(doc);
			if ( docs.size() == fRange.limit ) break;
		}
	}

	addChunkOfResponse(docs);
}


