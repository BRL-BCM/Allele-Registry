#include "requests.hpp"



RequestFetchGene::RequestFetchGene(std::string const & idOrSymbol, bool hgncSymbol)
: Request(documentType::gene)
{
	if (hgncSymbol) {
		fHgncSymbol = idOrSymbol;
		return;
	}
	if (idOrSymbol.substr(0,2) != "GN" || idOrSymbol.size() < 3) {
		throw ExceptionIncorrectRequest("Gene id must consist of prefix 'GN' and decimal number. Given value: '" + idOrSymbol + "'.");
	}
	for (unsigned i = 2; i < idOrSymbol.size(); ++i) if (idOrSymbol[i] < '0' || idOrSymbol[i] > '9') {
		throw ExceptionIncorrectRequest("Gene id must consist of prefix 'GN' and decimal number. Given value: '" + idOrSymbol + "'.");
	}
	fGnId = boost::lexical_cast<unsigned>(idOrSymbol.substr(2));
}


RequestFetchGene::~RequestFetchGene()
{
	stopProcessingThread();
}


void RequestFetchGene::process()
{
	Gene gene;
	if (fHgncSymbol == "") {
		// by id
		gene = referencesDb->getGeneById(fGnId);
	} else {
		std::vector<Gene> t = referencesDb->getGenesByName(fHgncSymbol);
		for (Gene const & g: t) {
			if (g.active && g.hgncSymbol == fHgncSymbol) {
				gene = g;
				break;
			}
		}
	}
	Document doc;
	if (gene.hgncId == 0) {
		doc = DocumentError(errorType::NotFound);
	} else {
		doc = DocumentGene();
		doc.gene().gnId = gene.hgncId;
		doc.gene().hgncName = gene.hgncName;
		doc.gene().hgncSymbol = gene.hgncSymbol;
		doc.gene().ncbiId = gene.refSeqId;
	}
	setResponse(doc);
}
