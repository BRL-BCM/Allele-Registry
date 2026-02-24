#include "requests.hpp"
#include "hgvs.hpp"
#include "canonicalization.hpp"
#include <boost/thread.hpp>


void func(std::vector<std::pair<int, Document*> > const & list, Request * pThis, AllelesDatabase * allelesDb, std::vector<Document*> * out) {
	std::vector<Document> doc;
	for(int j=0;j<list.size();j++) {
		doc.push_back(*list[j].second);
		delete list[j].second;
	}
	pThis->mapVariantsToMainGenome(doc);
	allelesDb->fetchVariantsByDefinition(doc);
	pThis->fillVariantsDetails(doc);
	doc = allelesDb->filterNamed(doc);
	for(int j=0;j<doc.size();j++) {
		(*out)[list[j].first] = new Document(doc[j]);
	}
}

struct RequestQueryAllelesByGeneAndMutation::Pim
{
	std::string fGene;
	std::string fMutation;
	void fetchWholeDocuments( Request * pThis, std::vector<std::string> const & hgvses, std::vector<Document> & documents)
	{
		std::vector<Document*> * tmp = new std::vector<Document*>();
		tmp->resize(hgvses.size());
		for(int i=0;i<tmp->size();i++) {
			(*tmp)[i] = NULL;
		}
		documents.clear();

		int num_threads = hgvses.size();
		std::vector<std::vector<std::pair<int, Document*> > > lists(num_threads);

		for (int i=0;i<hgvses.size();i++) {
			std::string hgvs = hgvses[i];
			try {
				HgvsVariant v(true);
				::decodeHgvs(referencesDb, hgvs, v);
				if (v.isGenomic()) {
					lists[i%num_threads].push_back( std::make_pair(i, new Document(DocumentActiveGenomicVariant(canonicalize(referencesDb,v.genomic)))) );
				} else {
					lists[i%num_threads].push_back( std::make_pair(i, new Document(DocumentActiveProteinVariant(canonicalize(referencesDb,v.protein)))) );
				}
			} catch (...) {
				lists[i%num_threads].push_back( std::make_pair(i, new Document(DocumentError::createFromCurrentException())) );
			}
		}

		std::vector<boost::thread *> threads;
		// ===== get all data
		for(int i=0;i<lists.size();i++) {
			threads.push_back(new boost::thread(func,lists[i],pThis,allelesDb,tmp));
		}

		for(int i=0;i<threads.size();i++) {
			threads[i]->join();
			delete threads[i];
		}

		threads.clear();

		for(auto x : *tmp) {
			documents.push_back(Document(*x));
			delete x;
		}

		delete tmp;

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
				hgvses2.push_back(dp.transcriptHgvs);
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
