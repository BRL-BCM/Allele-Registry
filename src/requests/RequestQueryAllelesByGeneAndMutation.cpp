#include "requests.hpp"
#include "hgvs.hpp"
#include "canonicalization.hpp"


struct RequestQueryAllelesByGeneAndMutation::Pim
{
	std::string fGene;
	std::string fMutation;
	void fetchWholeDocuments( Request * pThis, std::vector<std::string> const & hgvses, std::vector<Document> & documents)
	{
		documents.clear();

		for (auto const & hgvs: hgvses) {
			try {
				HgvsVariant v(true);
				::decodeHgvs(referencesDb, hgvs, v);
				if (v.isGenomic()) {
					documents.push_back( DocumentActiveGenomicVariant(canonicalize(referencesDb,v.genomic)) );
				} else {
					documents.push_back( DocumentActiveProteinVariant(canonicalize(referencesDb,v.protein)) );
				}
			} catch (...) {
				documents.push_back( DocumentError::createFromCurrentException() );
			}
		}

		// ===== get all data
		pThis->mapVariantsToMainGenome(documents);
		allelesDb->fetchVariantsByDefinition(documents);
		pThis->fillVariantsDetails(documents);
	}
};


RequestQueryAllelesByGeneAndMutation::RequestQueryAllelesByGeneAndMutation(std::string const & gene, std::string const & mutation)
: Request(documentType::activeGenomicVariant), pim(new Pim)
{
	pim->fGene = gene;
	pim->fMutation = mutation;
}


RequestQueryAllelesByGeneAndMutation::~RequestQueryAllelesByGeneAndMutation()
{
	stopProcessingThread();
	delete pim;
}


void RequestQueryAllelesByGeneAndMutation::process()
{
	// ====== get gene
	Gene gene;
	{
		std::vector<Gene> t = referencesDb->getGenesByName(pim->fGene);
		for (Gene const & g: t) {
			if (g.active && g.hgncSymbol == pim->fGene) {
				gene = g;
				break;
			}
		}
		if (gene.hgncId == 0) throw ExceptionUnknownGeneName(pim->fGene);
	}

	// ===== get references
	std::vector<ReferenceId> transcripts;
	std::vector<ReferenceId> proteins;
	for (auto refId: gene.assignedReferences) {
		if ( referencesDb->isProteinReference(refId) ) proteins.push_back(refId);
		else if ( referencesDb->isSplicedRefSeq(refId) ) transcripts.push_back(refId);
	}

	// ===== generate all possible HGVS expressions
	if (pim->fMutation.size() < 2 || pim->fMutation[1] != '.' || (pim->fMutation[0] != 'c' && pim->fMutation[0] != 'p')) {
		throw ExceptionIncorrectRequest("Variation must start form 'c.' or 'p.' prefix.");
	}
	std::vector<std::string> hgvses;
	if (pim->fMutation.front() == 'c') {
		for (auto refId: transcripts) hgvses.push_back( referencesDb->getNames(refId).front() + ":" + pim->fMutation );
	} else {
		for (auto refId: proteins) hgvses.push_back( referencesDb->getNames(refId).front() + ":" + pim->fMutation );
	}

	// ===== convert hgvs-es to documents
	std::vector<Document> documents;
	pim->fetchWholeDocuments(this, hgvses, documents);

	// ===== query corresponding transcript or protein variants
	std::vector<std::string> hgvses2;
	for (auto const & doc: documents) {
		if (doc.isActiveGenomicVariant()) {
			for ( auto const & dt: doc.asActiveGenomicVariant().definitionsOnTranscripts ) {
				if ( dt.geneId == gene.hgncId && dt.proteinHgvsCanonical != "" ) {
					hgvses2.push_back(dt.proteinHgvsCanonical);
				}
			}
		} else if (doc.isActiveProteinVariant()) {
			for ( auto const & dp: doc.asActiveProteinVariant().definitionOnProtein.hgvsMatchingTranscriptVariants ) {
				hgvses2.push_back(dp);
			}
		}
	}
	std::sort(hgvses2.begin(),hgvses2.end());
	hgvses2.resize( std::unique(hgvses2.begin(),hgvses2.end()) - hgvses2.begin() );
	std::vector<Document> documents2;
	pim->fetchWholeDocuments(this, hgvses2, documents2);
	// hgvses/documents - transcripts, hgvses2/documents2 - proteins
	if (pim->fMutation.front() == 'p') {
		// protein are first
		hgvses.swap(hgvses2);
		documents.swap(documents2);
	}

	// ===== build response
	auto const textLabels = getTextLabels();
	std::string out = "{\n";
	if (! hgvses.empty()) {
		out += "\"" + textLabels[label::canonicalAlleles] + "\": [\n";
		for (unsigned i = 0; i < hgvses.size(); ++i) {
			if (i > 0) out += ",";
			out += "{ \"hgvs\": \"" + hgvses[i] + "\",\n";
			out += "  \"result\": " + outputBuilder.createOutput(documents[i]) + "\n";
			out += "}";
		}
		out += "]\n";
	}
	if (! hgvses2.empty()) {
		if (out.size() > 2) out += ",";
		out += "\"" + textLabels[label::proteinAlleles] + "\": [\n";
		for (unsigned i = 0; i < hgvses2.size(); ++i) {
			if (i > 0) out += ",";
			out += "{ \"hgvs\": \"" + hgvses2[i] + "\",\n";
			out += "  \"result\": " + outputBuilder.createOutput(documents2[i]) + "\n";
			out += "}";
		}
		out += "]\n";
	}
	out += "}";
	setResponse(out);
}
