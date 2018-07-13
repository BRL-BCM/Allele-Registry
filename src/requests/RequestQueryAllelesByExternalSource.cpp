#include "requests.hpp"
#include "../externalSources/externalSources.hpp"


struct RequestQueryAllelesByExternalSource::Pim
{
	ResultSubset rs;
	std::string srcName;
	std::vector<std::string> params;
};


RequestQueryAllelesByExternalSource::~RequestQueryAllelesByExternalSource()
{
	stopProcessingThread();
	delete pim;
}


RequestQueryAllelesByExternalSource::RequestQueryAllelesByExternalSource(ResultSubset rs, std::string const & srcName, std::vector<std::string> params)
: Request(documentType::activeGenomicVariant), pim(new Pim)
{
	pim->rs = rs;
	pim->srcName = srcName;
	pim->params = params;
}


void RequestQueryAllelesByExternalSource::process()
{
	// ====== query ids
	std::vector<uint32_t> ids;
	externalSources::queryLinksBySource(ids, pim->srcName, pim->params);
	unsigned const right = std::min( static_cast<unsigned>(ids.size()), pim->rs.right() );
	if (pim->rs.skip < right) {
		for ( unsigned i = pim->rs.skip;  i < right;  ++i ) {
			ids[i-pim->rs.skip] = ids[i];
		}
		ids.resize( right - pim->rs.skip );
	} else {
		ids.clear();
	}

	// ====== query genomic variants
	std::vector<Document> documents;
	documents.reserve(ids.size());
	for (uint32_t id: ids) {
		documents.push_back(DocumentActiveGenomicVariant());
		documents.back().asActiveGenomicVariant().caId = CanonicalId(id);
	}
	allelesDb->fetchVariantsByCaPaIds(documents);

	// ====== missing variants are queried as protein variants
	std::vector<Document> docsProteins;
	for ( unsigned i = 0;  i < ids.size();  ++i ) {
		if ( documents[i].isError() ) {
			docsProteins.push_back( DocumentActiveProteinVariant() );
			docsProteins.back().asActiveProteinVariant().caId = CanonicalId(ids[i]);
		}
	}
	allelesDb->fetchVariantsByCaPaIds(docsProteins);
	std::reverse(docsProteins.begin(),docsProteins.end());
	for ( auto & doc: documents ) {
		if ( doc.isError() ) {
			doc = docsProteins.back();
			docsProteins.pop_back();
		}
	}

	// ====== fill the documents
	allelesDb->fetchVariantsByDefinition(documents);
	fillVariantsDetails(documents);
	addChunkOfResponse(documents);
}


